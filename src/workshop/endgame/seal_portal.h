#pragma once
#include "seal_engine.h"
#include "seal_native_guards.h"
namespace Arena {
inline int SealChampion=0,SealElite=0;
namespace SealPortal {
using PortalFn=int(__fastcall*)(void*,void*,void*,int,int,int,void**,int,int);
using PointerFn=void*(__fastcall*)(void*);using CodeFn=unsigned(__fastcall*)(void*);
inline PortalFn Original=nullptr;inline PointerFn Inventory=nullptr,First=nullptr,Next=nullptr;inline CodeFn Code=nullptr;
struct Snapshot{void* game=nullptr;unsigned owner=0,frame=0;TrialCraft::Seal seal;bool ready=false;};
inline std::array<Snapshot,5> Pending{};
inline void* Session=nullptr;inline unsigned LastFrame=0,Opened=0;
inline void Reset(void* game,unsigned frame){if(Session!=game||frame<LastFrame){Session=game;Opened=0;Pending={};}LastFrame=frame;}
inline int __fastcall Open(void* game,void* player,void* room,int x,int y,int level,void** portal,int object,int permanent){
 if(game)Reset(game,Ramp::Frame(game));
 const bool trial=level>=FirstLevel&&level<FirstLevel+5;
 if(trial&&(Opened&(1u<<(level-FirstLevel))))return 0;
 TrialCraft::Seal seal;bool captured=false;
 if(game&&player&&*static_cast<unsigned*>(player)==0&&level>=FirstLevel&&level<FirstLevel+5){
  void* inventory=Inventory(player);unsigned count=0;char wanted[4]={'W','a',char('1'+level-FirstLevel),0};char blank[4]={'W','t',wanted[2],0};
  for(void* item=inventory?First(inventory):nullptr;item&&count++<4096;item=Next(item)){
   if(*static_cast<unsigned*>(item)!=4)continue;
   auto data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
   if(!data||data[0x55]!=3)continue;
   unsigned code=Code(item);if(code==D2RL::Items::MakeItemCode(blank)){captured=true;break;}
   if(code!=D2RL::Items::MakeItemCode(wanted))continue;
   for(int i=0;i<TrialCraft::FieldCount;++i)seal.fields[i]=Ramp::GetUnit(item,TrialCraft::FirstStat+i,0);
   captured=seal.valid();break;
  }
 }
 int result=Original(game,player,room,x,y,level,portal,object,permanent);
 if(result&&trial)Opened|=1u<<(level-FirstLevel);
 if(result&&captured){Pending[level-FirstLevel]={game,Barbarian::Id(player),Ramp::Frame(game),seal,true};if(Context)Context->LogInfo("Trial portal captured saved seal modifiers");}
 return result;
}
inline void Apply(void* game,unsigned now,Run& run){
 SealChampion=SealElite=0;auto& s=Pending[run.stage-1];
 if(s.ready&&s.game==game&&s.owner==run.owner&&now>=s.frame){TrialCraft::apply(s.seal,run,SealChampion,SealElite);s.ready=false;}
}
inline bool Install(const PluginContext* c){
 auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
 using namespace TrialCraft;
 if(std::memcmp(base+InventoryRva,InventoryGuard,sizeof(InventoryGuard))||std::memcmp(base+FirstRva,FirstGuard,sizeof(FirstGuard))||std::memcmp(base+NextRva,NextGuard,sizeof(NextGuard))||std::memcmp(base+CodeRva,CodeGuard,sizeof(CodeGuard)))return false;
 Inventory=reinterpret_cast<PointerFn>(base+InventoryRva);First=reinterpret_cast<PointerFn>(base+FirstRva);Next=reinterpret_cast<PointerFn>(base+NextRva);Code=reinterpret_cast<CodeFn>(base+CodeRva);
 auto h=MakeInlineHook(PortalRva,PortalGuard,sizeof(PortalGuard),reinterpret_cast<void*>(Open),reinterpret_cast<void**>(&Original));
 return c->api->installInlineHook(c,&h);
}
}
}
