#pragma once
// Retry only observed ground pickups of the twelve League currencies through the engine's normal deposit path.
// No item deletion, synthetic balance, or save-file manipulation is performed here.
namespace LeagueStorage {
using Pointer=void*(__fastcall*)(void*);
using Integer=unsigned(__fastcall*)(void*);
using Boolean=bool(__fastcall*)(void*);
using ClientFindFn=void*(__fastcall*)(unsigned,unsigned);
inline ClientFindFn ClientFind=nullptr;
inline void* localPlayer(){
 auto* core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
 const unsigned char guard[]={0xff,0x15,0xfe,0x39,0x2e,0x00};
 if(!core||std::memcmp(core+0x41caa4,guard,sizeof(guard)))return nullptr;
 auto index=*reinterpret_cast<unsigned(__fastcall**)()>(core+0x7004a8);
 auto lookup=*reinterpret_cast<void*(__fastcall**)(unsigned)>(core+0x7004d8);
 return index&&lookup?lookup(index()):nullptr;
}
inline void*(*LocalPlayer)()=localPlayer;
using DepositFn=int(__fastcall*)(void*,void*,unsigned,unsigned,int,int,int);
inline Pointer Destination=nullptr;
inline Pointer Inventory=nullptr,First=nullptr,Next=nullptr,Owner=nullptr,FirstBank=nullptr,NextBank=nullptr;
inline Integer Code=nullptr,BankKind=nullptr,BankId=nullptr;
inline Boolean BankTransient=nullptr,Stackable=nullptr;
inline DepositFn Deposit=nullptr;
inline const D2RL::PluginContext* Context=nullptr;inline unsigned Reports=0,SeenChecks=0;
inline void report(const char* message){if(Context&&Reports++<24)Context->LogInfo(message);}
inline unsigned NextTick=0,LastTick=0,Deposited=0;inline void* Session=nullptr;
struct GroundItem {unsigned id,code;};
inline std::vector<GroundItem> Ground;
inline bool currency(unsigned code){
 for(char m='n';m<='p';++m)for(char row='1';row<='4';++row)
  if(code==D2RL::Items::MakeItemCode('L',m,row))return true;
 return false;
}
inline bool observed(unsigned id,unsigned code){return std::any_of(Ground.begin(),Ground.end(),[&](auto& g){return g.id==id&&g.code==code;});}
inline void forget(unsigned id){Ground.erase(std::remove_if(Ground.begin(),Ground.end(),[&](auto& g){return g.id==id;}),Ground.end());}
inline void observeGround(void* game){
 // Verified native item bucket RVA 0x4925b0: items start at game + 0x2e30.
 for(unsigned b=0;b<128;++b){void* u=nullptr;std::memcpy(&u,static_cast<unsigned char*>(game)+0x2e30+8*b,8);
  for(unsigned n=0;u&&n<4096;++n){void* next=nullptr;std::memcpy(&next,static_cast<unsigned char*>(u)+0x158,8);
   unsigned mode=static_cast<unsigned*>(u)[3];if(*static_cast<unsigned*>(u)==4&&(mode==3||mode==5)&&currency(Code(u))&&!observed(Barbarian::Id(u),Code(u)))Ground.push_back({Barbarian::Id(u),Code(u)});u=next;
  }
 }
 Ground.erase(std::remove_if(Ground.begin(),Ground.end(),[&](auto& g){
  void* u=Curses::Find(game,4,g.id);if(!u||Code(u)!=g.code)return true;
  unsigned mode=static_cast<unsigned*>(u)[3];if(mode==3||mode==5)return false;
  auto d=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(u)+0x10);
  return mode!=0||!d||d[0x55]!=0; // Cursor, Cube or stash means manual handling: stop retrying.
 }),Ground.end());
}
inline void* bank(void* game,void* inventory){
 unsigned count=0;
 for(void* n=FirstBank(inventory);n&&count++<256;n=NextBank(n)){
  if(Context&&!(SeenChecks&2)){char line[160];std::snprintf(line,sizeof(line),"League storage: node id=%u kind=%u inactive=%d serverFound=%d",BankId(n),BankKind(n),BankTransient(n),Curses::Find(game,0,BankId(n))!=nullptr);report(line);}
  if(!BankKind(n)||BankTransient(n))continue;
  auto* unit=Curses::Find(game,0,BankId(n));
  if(unit&&*static_cast<unsigned*>(unit)==0)return unit;
 }
 return nullptr;
}
inline void collect(void* game,void* player){
 void* inv=Inventory(player);if(!inv)return;
 // Snapshot IDs before mutation: the deposit may unlink/destroy an item.
 std::vector<unsigned> ids;unsigned count=0;
 for(void* item=First(inv);item&&count++<4096;item=Next(item)){
  if(*static_cast<unsigned*>(item)==4&&currency(Code(item))&&!(SeenChecks&1)){
   auto d=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);char line[180];
   std::snprintf(line,sizeof(line),"League storage: found orb mode=%u page=%u ownerMatches=%d nativeStackable=%d",static_cast<unsigned*>(item)[3],d?unsigned(d[0x55]):255,Owner(item)==inv,Stackable(item));report(line);SeenChecks|=1;
  }
  if(*static_cast<unsigned*>(item)!=4||static_cast<unsigned*>(item)[3]!=0||Owner(item)!=inv||!currency(Code(item)))continue;
  auto data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
  if(data&&data[0x55]==0&&Stackable(item)&&observed(Barbarian::Id(item),Code(item)))ids.push_back(Barbarian::Id(item));
 }
 if(ids.empty())return;
 void* target=bank(game,inv);
 // The native shared-bank node list is maintained on the client inventory.
 // Resolve its IDs back through the authoritative server game before depositing.
 if(!target){auto* client=LocalPlayer?LocalPlayer():nullptr;if(!client&&ClientFind)client=ClientFind(Barbarian::Id(player),0);if(client&&Barbarian::Id(client)==Barbarian::Id(player)){
  if(Destination){auto* proxy=Destination(client);if(proxy)target=Curses::Find(game,0,Barbarian::Id(proxy));}
  void* clientInventory=Inventory(client);if(!target&&clientInventory&&clientInventory!=inv)target=bank(game,clientInventory);
 }}
 if(!target){if(!(SeenChecks&2)){report("League storage: orbs retained; server/core-client destination unavailable.");SeenChecks|=2;}return;} // Keep every orb; retry when native storage exists.
 unsigned targetId=Barbarian::Id(target);
 for(auto id:ids){
  void* item=Curses::Find(game,4,id);if(!item||Owner(item)!=inv||!currency(Code(item)))continue;
  int result=Deposit(game,player,id,targetId,0,0,0);
  if(Context&&Reports<24){char line[160];std::snprintf(line,sizeof(line),"League storage: deposit orb=%u bank=%u result=%d",id,targetId,result);report(line);}
  auto* after=Curses::Find(game,4,id);
  if(!after||Owner(after)!=inv){++Deposited;forget(id);}
 }
}
inline void frame(void* game,unsigned now){
 if(!Deposit)return;
 if(Session!=game||now<LastTick){Session=game;NextTick=0;Deposited=0;Reports=0;SeenChecks=0;Ground.clear();}
 LastTick=now;observeGround(game);if(now<NextTick)return;NextTick=now+25;
 AuraFeatures::Units(game,0,[&](void* p){if(MonsterRuntime::Living(p))collect(game,p);});
}
inline bool install(const D2RL::PluginContext* c){
 const unsigned char destGuard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x85,0xc9,0x75,0x1b};
 if(!c->api->checkExpectedBytes(c,0x46da50,destGuard,sizeof(destGuard)))return false;
 Destination=reinterpret_cast<Pointer>(c->exeBase+0x46da50);
 const unsigned char clientGuard[]={0x4c,0x63,0xca,0x48,0x8d,0x5,0x36,0x93,0x98,0x2,0x8b,0xd1,0x44,0x8b,0xc1,0x49};
 if(!c->api->checkExpectedBytes(c,0x9a5d0,clientGuard,sizeof(clientGuard)))return false;
 ClientFind=reinterpret_cast<ClientFindFn>(c->exeBase+0x9a5d0);
 const unsigned char InventoryGuard[]={0x48,0x89,0x5c,0x24,0x18,0x56,0x48,0x83,0xec,0x20,0x48,0x8b};
 if(!c->api->checkExpectedBytes(c,0x34a360,InventoryGuard,sizeof(InventoryGuard)))return false;
 const unsigned char FirstGuard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x85,0xc9};
 if(!c->api->checkExpectedBytes(c,0x388c10,FirstGuard,sizeof(FirstGuard)))return false;
 const unsigned char NextGuard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x85,0xc9};
 if(!c->api->checkExpectedBytes(c,0x38aba0,NextGuard,sizeof(NextGuard)))return false;
 const unsigned char CodeGuard[]={0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48,0x8b};
 if(!c->api->checkExpectedBytes(c,0x36ef50,CodeGuard,sizeof(CodeGuard)))return false;
 const unsigned char OwnerGuard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x85,0xc9};
 if(!c->api->checkExpectedBytes(c,0x38ac50,OwnerGuard,sizeof(OwnerGuard)))return false;
 const unsigned char FirstBankGuard[]={0x48,0x8b,0x41,0x68,0xc3};
 if(!c->api->checkExpectedBytes(c,0x388e00,FirstBankGuard,sizeof(FirstBankGuard)))return false;
 const unsigned char NextBankGuard[]={0x48,0x8b,0x41,0x10,0xc3};
 if(!c->api->checkExpectedBytes(c,0x38cd70,NextBankGuard,sizeof(NextBankGuard)))return false;
 const unsigned char BankKindGuard[]={0x33,0xc0,0x80,0x79,0xa,0x1,0xf,0x94,0xc0,0xc3};
 if(!c->api->checkExpectedBytes(c,0x38cd90,BankKindGuard,sizeof(BankKindGuard)))return false;
 const unsigned char BankTransientGuard[]={0xf,0xb6,0x41,0x8,0xc3};
 if(!c->api->checkExpectedBytes(c,0x38cd60,BankTransientGuard,sizeof(BankTransientGuard)))return false;
 const unsigned char BankIdGuard[]={0x8b,0x1,0xc3};
 if(!c->api->checkExpectedBytes(c,0x2ef880,BankIdGuard,sizeof(BankIdGuard)))return false;
 const unsigned char StackableGuard[]={0x48,0x89,0x5c,0x24,0x8,0x57,0x48,0x83,0xec,0x20,0x41,0xb8,0xf8,0xd,0x0,0x0};
 if(!c->api->checkExpectedBytes(c,0x46dc00,StackableGuard,sizeof(StackableGuard)))return false;
 const unsigned char DepositGuard[]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c};
 if(!c->api->checkExpectedBytes(c,0x4f92f0,DepositGuard,sizeof(DepositGuard)))return false;
 Inventory=reinterpret_cast<Pointer>(c->exeBase+0x34a360);
 First=reinterpret_cast<Pointer>(c->exeBase+0x388c10);
 Next=reinterpret_cast<Pointer>(c->exeBase+0x38aba0);
 Code=reinterpret_cast<Integer>(c->exeBase+0x36ef50);
 Owner=reinterpret_cast<Pointer>(c->exeBase+0x38ac50);
 FirstBank=reinterpret_cast<Pointer>(c->exeBase+0x388e00);
 NextBank=reinterpret_cast<Pointer>(c->exeBase+0x38cd70);
 BankKind=reinterpret_cast<Integer>(c->exeBase+0x38cd90);
 BankTransient=reinterpret_cast<Boolean>(c->exeBase+0x38cd60);
 BankId=reinterpret_cast<Integer>(c->exeBase+0x2ef880);
 Stackable=reinterpret_cast<Boolean>(c->exeBase+0x46dc00);
 Deposit=reinterpret_cast<DepositFn>(c->exeBase+0x4f92f0);
 Context=c;
 c->LogInfo("League orb storage: guarded native deposit retry enabled.");return true;
}
}
