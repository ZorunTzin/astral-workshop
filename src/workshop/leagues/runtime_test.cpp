#define main ExistingAuraTests
#include "../combat/aura_test.cpp"
#undef main
#include "runtime.h"
std::map<unsigned,Unit> units;unsigned serial=100,removed=0,targets=0;bool blocked=false,relocate=false;
void* __fastcall LFind(void*,int t,unsigned id){if(t==0&&id==player.id)return &player;auto it=units.find(id);return it!=units.end()&&it->second.type==unsigned(t)?&it->second:nullptr;}
int __fastcall LX(void* p){return static_cast<Unit*>(p)->x;}int __fastcall LY(void* p){return static_cast<Unit*>(p)->y;}
int __fastcall LLevel(const void* p) noexcept{return static_cast<const Unit*>(p)->room;}
void* __fastcall LRoom(void* p){return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(p)->room));}
void* __fastcall LAt(void* room,int,int){return blocked?nullptr:room;}
void* __fastcall LFree(void* room,League::Point* p,League::Point* out,int,int,int,int){if(relocate){p->x+=3;p->y+=4;}return blocked?nullptr:room;}
void* __fastcall LAlloc(int type,int cls,int x,int y,void*,void* room,unsigned short flags,int mode,int,void*){auto& u=units[++serial];u.id=serial;u.type=type;u.cls=cls;u.mode=mode;u.room=(flags&1)?int(uintptr_t(room)):0;u.x=x;u.y=y;u.stats[6]=u.stats[7]=1000;return &u;}
void* __fastcall LSpawn(void* g,void* room,int x,int y,int cls,int,int,unsigned short){if(blocked)return nullptr;return LAlloc(1,cls,x,y,g,room,1,1,0,nullptr);}
void* __fastcall LMissile(void* g,void* raw){auto& p=*static_cast<Pulse::Params*>(raw);return LAlloc(3,p.missile,p.x,p.y,g,LRoom(p.owner),1,1,0,nullptr);}
void __fastcall LNotice(void*,unsigned,unsigned){}
void __fastcall LDelete(void*,void* p){++removed;units.erase(static_cast<Unit*>(p)->id);}
void __fastcall LTarget(void*,void*){++targets;}
void* __fastcall LApply(void* raw){auto& c=*static_cast<Curses::Curse*>(raw);auto* list=static_cast<List*>(Apply(raw));list->stats[c.stat]=c.value;return list;}
void tick(unsigned n){Frame(n);League::frame(game);}
void reset(){units.clear();League::Events.clear();League::Mobs.clear();League::Deaths.clear();League::Retirements.clear();League::Game=game;League::Last=100;League::Next=0;League::Testing=false;League::Visited.fill(true);League::Pending={};League::RetryAt={};League::Attempts={};player.room=2;player.lists.clear();player.mode=1;player.stats[6]=1000;Frame(100);}
void kill(unsigned id){auto& u=units.at(id);u.mode=12;u.stats[6]=0;D2RL::Lifecycle::MonsterDeathEvent e{};e.structSize=sizeof(e);e.monster={D2RL::Lifecycle::UnitType::Monster,id,u.cls,0};e.levelId=u.room;League::death(nullptr,&e,nullptr);}

Unit clientPlayer;int clientInventoryToken=0;bool clientOnly=false;
void* __fastcall SClient(unsigned id,unsigned type){return id==player.id&&type==0?&clientPlayer:nullptr;}
int inventoryToken=0,bankToken=0,depositCalls=0;bool bankReady=false,depositWorks=false;
std::map<unsigned,std::array<unsigned char,96>> itemData;
std::vector<unsigned> backpack;
void* __fastcall SInventory(void* p){return p==&clientPlayer?&clientInventoryToken:&inventoryToken;}
void* __fastcall SFirst(void*){return backpack.empty()?nullptr:LFind(game,4,backpack.front());}
void* __fastcall SNext(void* p){auto it=std::find(backpack.begin(),backpack.end(),Barbarian::Id(p));return it!=backpack.end()&&++it!=backpack.end()?LFind(game,4,*it):nullptr;}
void* __fastcall SOwner(void* p){return std::find(backpack.begin(),backpack.end(),Barbarian::Id(p))!=backpack.end()?&inventoryToken:nullptr;}
unsigned __fastcall SCode(void* p){return static_cast<Unit*>(p)->cls;}
void* __fastcall SBank(void* inv){return bankReady&&(!clientOnly||inv==&clientInventoryToken)?&bankToken:nullptr;}
void* __fastcall SEnd(void*){return nullptr;}
unsigned __fastcall SKind(void*){return 1;}
unsigned __fastcall SId(void*){return 900;}
bool __fastcall SFalse(void*){return false;}
bool __fastcall STrue(void*){return true;}
int __fastcall SDeposit(void*,void*,unsigned id,unsigned target,int,int,int){assert(target==900);++depositCalls;if(!depositWorks)return 0;backpack.erase(std::find(backpack.begin(),backpack.end(),id));units.erase(id);return 1;}
void* SLocal();void* __fastcall SDestination(void*);
void storageTests(){
 using namespace LeagueStorage;
 LocalPlayer=nullptr;Inventory=SInventory;First=SFirst;Next=SNext;Owner=SOwner;Code=SCode;FirstBank=SBank;NextBank=SEnd;BankKind=SKind;BankId=SId;BankTransient=SFalse;Stackable=STrue;Deposit=SDeposit;
 auto& target=units[900];target.type=0;target.id=900;// Server lookup establishes ownership; stash proxies need no game back-pointer.
 for(char m='n';m<='p';++m)for(char r='1';r<='4';++r){auto* u=static_cast<Unit*>(LAlloc(4,D2RL::Items::MakeItemCode('L',m,r),0,0,game,LRoom(&player),1,0,0,nullptr));backpack.push_back(u->id);auto* d=itemData[u->id].data();std::memcpy(reinterpret_cast<unsigned char*>(u)+0x10,&d,8);assert(currency(u->cls));}
 assert(!currency(D2RL::Items::MakeItemCode('L','n','5')));
 // Inventory-born/withdrawn items must never be swept into the stash.
 bankReady=true;depositWorks=true;collect(game,&player);assert(depositCalls==0&&backpack.size()==12);
 for(auto id:backpack){auto* u=&units.at(id);u->mode=3;void* ptr=u;std::memcpy(game+0x2e30+8*(id&127),&ptr,8);}
 observeGround(game);assert(Ground.size()==12);
 for(auto id:backpack){units.at(id).mode=0;void* ptr=nullptr;std::memcpy(game+0x2e30+8*(id&127),&ptr,8);}
 bankReady=false;depositWorks=false;

 collect(game,&player);assert(backpack.size()==12&&depositCalls==0);
 bankReady=true;collect(game,&player);assert(backpack.size()==12&&depositCalls==12);
 clientOnly=true;clientPlayer.id=player.id;ClientFind=nullptr;LocalPlayer=SLocal;Destination=SDestination;bankReady=false;
 depositWorks=true;collect(game,&player);assert(backpack.empty()&&depositCalls==24&&Deposited==12);
 collect(game,&player);assert(depositCalls==24);
 assert(Ground.empty());
 // Even a same-ID withdrawal after deposit cannot inherit pickup eligibility.
 auto* withdrawn=static_cast<Unit*>(LAlloc(4,D2RL::Items::MakeItemCode('L','n','3'),0,0,game,LRoom(&player),1,0,0,nullptr));backpack.push_back(withdrawn->id);auto* wd=itemData[withdrawn->id].data();std::memcpy(reinterpret_cast<unsigned char*>(withdrawn)+0x10,&wd,8);
 collect(game,&player);assert(depositCalls==24&&backpack.size()==1);backpack.clear();

 Deposit=nullptr;units.clear();
 std::puts("PASS: all 12 orb codes; absent bank and failed deposit retain items; retry deposits once without duplicates.");
}

int eliteInitializers=0;
void __fastcall EInit(void*,int,int){++eliteInitializers;}
int __fastcall ERoll(void*,void*,unsigned char*,void* weights){assert(weights);return 6;}
void* SLocal(){return &clientPlayer;}
void* __fastcall SDestination(void* p){assert(p==&clientPlayer);return LFind(game,0,900);}
int main(){
 ExistingAuraTests();Curses::Find=LFind;Curses::X=LX;Curses::Y=LY;Curses::OriginalApply=LApply;SoulHarvest::Room=LRoom;Arena::Level=LLevel;Arena::Enabled=true;Barbarian::RoomDelete=LNotice;Barbarian::DeleteUnit=LDelete;Assassin::SetTarget=LTarget;MonsterRuntime::Spawn=LSpawn;League::Allocate=LAlloc;League::RoomAt=LAt;League::FreePosition=LFree;Pulse::OriginalCreate=LMissile;Pulse::FilterCreate=nullptr;Pulse::AfterCreate=nullptr;Amazon::PublishMissiles=nullptr;
 player={};player.id=10;void* pp=&player;std::memcpy(game+0x2230+8*(player.id&127),&pp,8);reset();
 // A genuine native object-mode change is required to activate Night.
 assert(League::place(&player,0,100,0));assert(units.at(League::Events[0].pillar).mode==0&&units.at(League::Events[0].pillar).room==player.room);tick(105);assert(League::Mobs.empty());auto id=League::Events[0].pillar;units[id].mode=1;tick(125);assert(League::Events[0].state.phase==League::Phase::Active&&!League::Mobs.empty());assert(GetUnit(&player,80,0)==-100);assert(targets>0);
 // Leaving the event area removes darkness immediately and retires the event after grace.
 player.room=1;tick(130);Frame(131);assert(GetUnit(&player,80,0)==0);tick(400);assert(League::Events[0].state.phase==League::Phase::Complete&&League::Mobs.empty());
 // Initial breach defenders, stronger waves and boss wave; event ends only after all are dead.
 reset();assert(League::place(&player,1,100,0));assert(League::Events[0].visuals.size()==9);
 unsigned now=110;
 for(int stage=0;stage<4;++stage){for(int n=0;n<15;++n){tick(now);now+=10;}assert(League::Events[0].state.stage==stage);assert(!League::Mobs.empty());auto mobs=League::Mobs;for(auto& m:mobs){int rank=stage==3?2:stage>0?1:0;assert(std::find(std::begin(League::Classes[1][rank]),std::end(League::Classes[1][rank]),m.cls)!=std::end(League::Classes[1][rank])||std::find(std::begin(League::Classes[1][0]),std::end(League::Classes[1][0]),m.cls)!=std::end(League::Classes[1][0]));kill(m.id);}tick(now);now+=10;}
 assert(League::Events[0].state.phase==League::Phase::Complete&&League::Events[0].visuals.empty());
 // Beyond needs five nearby recent ordinary kills. Its own kills cannot recursively spawn.
 reset();assert(League::place(&player,2,100,0));units[League::Events[0].pillar].mode=1;tick(105);
 for(int n=0;n<5;++n){auto* u=static_cast<Unit*>(LAlloc(1,19,20+n,20,game,LRoom(&player),1,1,0,nullptr));kill(u->id);}tick(115);assert(League::Events[0].state.groups==1&&League::Mobs.size()==4);
 auto mobs=League::Mobs;for(auto& m:mobs)kill(m.id);tick(120);assert(League::Events[0].state.groups==1);
 // Other League deaths contribute to Beyond clusters.
 for(int kind:{0,1}){reset();assert(League::place(&player,2,100,0));units[League::Events[0].pillar].mode=1;tick(105);assert(League::place(&player,kind,106,0));
 for(int n=0;n<5;++n){auto* u=static_cast<Unit*>(LAlloc(1,League::Classes[kind][0][0],20+n,20,game,LRoom(&player),1,1,0,nullptr));League::Mobs.push_back({u->id,int(u->cls),1});kill(u->id);}tick(115);assert(League::Events[0].state.groups==1);}
 League::Encounter e;e.kind=League::Kind::Beyond;assert(e.activate(1));for(int n=0;n<5;++n)assert(!e.clustered({n*100,0},n+1));e.kills.clear();for(int n=0;n<4;++n)assert(!e.clustered({0,0},n+1));assert(!e.clustered({0,0},200));assert(!e.clustered({0,0},5000));
 // Unloaded entities are retired when they become visible again; recycled IDs stay safe.
 reset();assert(League::place(&player,0,100,0));auto hidden=units.at(League::Events[0].pillar);units.erase(hidden.id);League::finish(0);assert(League::Retirements.size()==1);units[hidden.id]=hidden;tick(110);assert(!units.count(hidden.id)&&League::Retirements.empty());
 reset();assert(League::place(&player,0,100,0));hidden=units.at(League::Events[0].pillar);units.erase(hidden.id);League::finish(0);hidden.cls+=1;units[hidden.id]=hidden;tick(110);assert(units.count(hidden.id)&&League::Retirements.empty());
 // Night never exceeds the active monster cap, and timeout removes all event entities.
 reset();assert(League::place(&player,0,100,0));units[League::Events[0].pillar].mode=1;for(unsigned n=105;n<2300;n+=5){tick(n);assert(League::Mobs.size()<=League::MaxLiving);}tick(2400);assert(League::Mobs.empty()&&League::Events[0].state.phase==League::Phase::Complete);
 // Native free-position search writes the second argument, not a separate output.
 reset();relocate=true;assert(!League::place(&player,0,100,0));assert(League::Events.empty());relocate=false;
 // Entering at a blocked area boundary must not permanently consume the forced spawns.
 reset();League::Testing=true;League::Visited[2]=false;blocked=true;tick(105);assert(League::Events.empty()&&League::Pending[2]==7);blocked=false;tick(150);assert(League::Events.empty());tick(155);assert(League::Events.size()==3&&League::Pending[2]==0);tick(210);assert(League::Events.size()==3);
 // Failed placement produces neither an event nor partial items, and no towns/trials roll.
 reset();blocked=true;assert(!League::place(&player,0,100,0)&&League::Events.empty());blocked=false;for(int area:{1,39,40,75,103,109,166,170})assert(!League::eligible(area));
 reset();assert(League::place(&player,0,100,0));tick(1);assert(League::Events.empty()&&League::Mobs.empty());
 // Native level-12 monster in Blood Moor is normalized; XP doubles the zone average.
 reset();auto* balanced=static_cast<Unit*>(LAlloc(1,League::Classes[1][0][1],0,0,game,LRoom(&player),1,1,0,nullptr));
 balanced->stats[6]=400;balanced->stats[12]=12;balanced->stats[13]=League::LevelCurve[0][12][2]*League::ExperienceCoefficients[0][1][1]/100;balanced->stats[21]=100;balanced->stats[22]=200;
 League::balance(balanced,2);assert(balanced->stats[12]==1&&balanced->stats[13]==2*League::ZoneBaseline[0][2][1]);assert(balanced->stats[7]<1000&&balanced->stats[21]<140);
 assert(balanced->stats[6]==balanced->stats[7]);
 unsigned seed=12345,counts[3]={},masks[8]={};for(int i=0;i<100000;++i){auto mask=League::rollMechanics(seed);++masks[mask];for(int k=0;k<3;++k)if(mask&(1u<<k))++counts[k];}for(auto n:counts)assert(n>11500&&n<12500);assert(masks[3]&&masks[5]&&masks[6]&&masks[7]);
 // Native elite flag and random affix initializer for both elite ranks.
 auto* ed=itemData[balanced->id].data();std::memcpy(reinterpret_cast<unsigned char*>(balanced)+0x10,&ed,8);
 MonsterRuntime::OriginalRoll=ERoll;MonsterRuntime::Init=EInit;
 League::rankElite(balanced,2);assert((MonsterModifiersNative::Rank(balanced)&8)&&!(MonsterModifiersNative::Rank(balanced)&4));assert(eliteInitializers==2&&ed[0x20]&&balanced->stats[6]==balanced->stats[7]);
 MonsterRuntime::OriginalRoll=nullptr;MonsterRuntime::Init=nullptr;
 // Waiting pillars survive leaving the area to scout another waypoint.
 reset();assert(League::place(&player,2,100,0));player.room=1;tick(400);assert(League::Events[0].state.phase==League::Phase::Waiting);
 storageTests();
 std::puts("PASS: pillar activation, darkness restoration, breach escalation/completion, Beyond spatial/time windows and no recursion, blocked positions, area exclusions, session reset.");
}
