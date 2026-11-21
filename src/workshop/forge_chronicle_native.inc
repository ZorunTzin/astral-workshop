// D2R build 93847: native Chronicle insertion, notification and persistence.
// Byte guards below are checked before hooks/patches; no item flags or save widths change.
#include "forge_chronicle_engine.h"
struct ForgeChronicleRow {unsigned row;const char* name;};
#include "forge_chronicle_ids.inc"
static_assert(std::size(forgeChronicleRows)==ForgeChronicle::Count);
std::array<unsigned short,ForgeChronicle::Count> forgeChronicleNativeIds{};
bool forgeChronicleReady=false,forgeChronicleInstalled=false;
const PluginContext* chronicleContext{};
using ChronicleAddFn=bool(__fastcall*)(void*,unsigned char,unsigned,unsigned short,unsigned);
using ChronicleClientFn=void*(__fastcall*)(void*);
using ChroniclePlayerFn=void*(__fastcall*)(void*,int);
using ChronicleSendFn=void(__fastcall*)(void*,const void*);
using ChronicleClockFn=long long(__fastcall*)();
using ChroniclePickupFn=void(__fastcall*)(void*,void*);
using ChronicleCubeFn=void*(__fastcall*)(void*,void*,void*,void*);
ChronicleCubeFn originalChronicleCube{};
ChronicleAddFn chronicleAdd{};ChronicleClientFn chronicleClient{};
ChroniclePlayerFn chroniclePlayer{};ChronicleSendFn chronicleSend{};
ChronicleClockFn chronicleClock{};ChroniclePickupFn originalChroniclePickup{};

void recordRuneForge(void* player,void* item){
 if(!forgeChronicleReady||!readItemStat||!player||!item||*static_cast<unsigned*>(player)!=0||*static_cast<unsigned*>(item)!=4)return;
 int index=ForgeChronicle::identity(readItemStat(item,forgeMarkerStat,0),readItemStat(item,forgeLevelStat,0));
 if(index<0)return; // Legacy marker 1 has no recoverable recipe identity.
 void* client=chronicleClient(player);if(!client)return;
 const unsigned stamp=static_cast<unsigned>(chronicleClock()/60000000000LL);
 unsigned short recipe=forgeChronicleNativeIds[index];
 // The game's own add routine rejects repeats, preserving the first timestamp.
 if(!chronicleAdd(player,2,recipe,0,stamp))return;
 ForgeChronicle::Packet packet;
 packet.player=static_cast<unsigned*>(player)[2];packet.item=static_cast<unsigned*>(item)[2];
 packet.recipe=recipe;packet.minutes=stamp;
 chronicleSend(client,&packet);
 if(chronicleContext){char msg[160];std::snprintf(msg,sizeof(msg),"Chronicle Runeforged discovered: %s (forge %d, native %u)",forgeWords[index].name,index,recipe);chronicleContext->LogInfo(msg);}
}
// Native cube output creation: (game, player, recipe, output context) -> committed item.
// Property application passes a null unit, so discovery belongs AFTER this call.
void* __fastcall chronicleCube(void* game,void* player,void* recipe,void* output){
 void* item=originalChronicleCube(game,player,recipe,output);
 if(item)recordRuneForge(player,item);
 return item;
}
void __fastcall chroniclePickup(void* client,void* item){
 if(item&&*static_cast<unsigned*>(item)==4&&readItemStat){
  int index=ForgeChronicle::identity(readItemStat(item,forgeMarkerStat,0),readItemStat(item,forgeLevelStat,0));
  auto data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
  // The replacement category records forged equipment, never ordinary runewords.
  if(index<0&&data&&(*reinterpret_cast<unsigned*>(data+0x18)&0x04000000u))return;
  originalChroniclePickup(client,item); // Unique and Set discovery remains native.
  if(index>=0&&client)recordRuneForge(chroniclePlayer(client,0),item);
  return;
 }
 originalChroniclePickup(client,item);
}
#ifndef WORKSHOP_CHRONICLE_TEST
void __cdecl auditForgeChronicle(const PluginContext* ctx,const Lifecycle::DataTablesLoadedEvent*,void*) noexcept {
 forgeChronicleReady=false;
 if(!forgeChronicleInstalled)return;
 const DataTableService* tables{};if(ctx->QueryService(&tables)!=ServiceQueryResult::Success)return;
 DataTables::TableView view{.structSize=DataTables::TableViewSize};
 if(tables->getTable(ctx,DataTables::Bank::Rotw,DataTables::TableId::Runes,&view)!=DataTables::Result::Success||view.rowSize!=0xe8||view.rowCount!=ForgeChronicle::Count){ctx->LogError("Chronicle forge table shape mismatch; discovery disabled");return;}
 for(unsigned i=0;i<std::size(forgeChronicleRows);++i){
  const auto& expected=forgeChronicleRows[i];
  auto row=static_cast<const unsigned char*>(view.rows)+expected.row*view.rowSize;
  if(std::strncmp(reinterpret_cast<const char*>(row),expected.name,64)!=0||row[0x44]!=0){ctx->LogError("Chronicle forge identity/disabled-crafting audit failed; discovery disabled");return;}
  std::memcpy(&forgeChronicleNativeIds[i],row+0x46,2);
  for(unsigned j=0;j<i;++j)if(forgeChronicleNativeIds[j]==forgeChronicleNativeIds[i]){ctx->LogError("Chronicle native recipe identity collision; discovery disabled");return;}
 }
 forgeChronicleReady=true;
 ctx->LogInfo("Chronicle Runeforged audit PASS: 220 identities; ordinary runeword crafting remains disabled");
}
bool installForgeChronicle(const PluginContext* ctx){
 auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
 struct Guard{unsigned rva;const char* bytes;unsigned size;};
 const Guard guards[]={
 {0x3471a0,"\x48\x89\x5c\x24\x10\x48\x89\x6c\x24\x18\x56\x57\x41\x56",14},
 {0x48fde0,"\x40\x53\x48\x83\xec\x20\x48\x8b\xd9\x48\x85\xc9\x74\x1e",14},
 {0x484c90,"\x48\x89\x5c\x24\x10\x57\x48\x83\xec\x20\x8b\xfa\x48\x8b",14},
 {0x477eb0,"\x48\x89\x5c\x24\x10\x57\x48\x83\xec\x20\x48\x8b\xda\x48",14},
 {0x122c310,"\x48\x83\xec\x28\x48\x8d\x4c\x24\x30\xff\x15\xc9\xc0\x65",14},
 {0x4247d0,"\x48\x89\x5c\x24\x20\x57\x48\x83\xec\x60\x48\x8b\x05\xe7",14},
 {0x3d8b94,"\x74\x6a",2},
 {0x5269c0,"\x40\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xac\x24\x08\xea\xff\xff",21}};
 for(const auto& g:guards)if(std::memcmp(exe+g.rva,g.bytes,g.size)){ctx->LogError("Chronicle native signature mismatch; integration not installed");return false;}
 chronicleAdd=reinterpret_cast<ChronicleAddFn>(exe+0x3471a0);
 chronicleClient=reinterpret_cast<ChronicleClientFn>(exe+0x48fde0);
 chroniclePlayer=reinterpret_cast<ChroniclePlayerFn>(exe+0x484c90);
 chronicleSend=reinterpret_cast<ChronicleSendFn>(exe+0x477eb0);
 chronicleClock=reinterpret_cast<ChronicleClockFn>(exe+0x122c310);
 chronicleContext=ctx;
 const LifecycleService* life{};if(ctx->QueryService(&life)!=ServiceQueryResult::Success)return false;
 Lifecycle::DataTablesLoadedListener listener{.structSize=Lifecycle::DataTablesLoadedListenerSize,.callback=auditForgeChronicle};
 Lifecycle::ListenerHandle handle{};if(life->registerDataTablesLoadedListener(ctx,&listener,&handle)!=Lifecycle::Result::Success)return false;
 auto hook=MakeInlineHook(0x4247d0,guards[5].bytes,guards[5].size,reinterpret_cast<void*>(chroniclePickup),reinterpret_cast<void**>(&originalChroniclePickup));
 if(!ctx->api->installInlineHook(ctx,&hook))return false;
 auto cubeHook=MakeInlineHook(0x5269c0,guards[7].bytes,guards[7].size,reinterpret_cast<void*>(chronicleCube),reinterpret_cast<void**>(&originalChronicleCube));
 if(!ctx->api->installInlineHook(ctx,&cubeHook))return false;
 // Only the Chronicle catalog's skip-incomplete-row branch is removed.
 // Runes.complete stays 0 everywhere; the socket crafting code is unchanged.
 if(!ctx->PatchNop(0x3d8b94,guards[6].bytes,2,2))return false;
 forgeChronicleInstalled=true;
 ctx->LogInfo("Native Chronicle Runeforged integration enabled");return true;
}

#endif
