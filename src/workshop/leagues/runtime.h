#pragma once
#include "engine.h"
#include "bindings.h"
#include "storage.h"
#include "map.h"
#include "balance_bindings.h"
namespace League {
// Native argument 7 is allocation flags (bit 0 attaches to room); argument 8 is animation mode.
using AllocFn=void*(__fastcall*)(int,int,int,int,void*,void*,unsigned short,int,int,void*);
using RoomAtFn=void*(__fastcall*)(void*,int,int);
using FreeFn=void*(__fastcall*)(void*,Point*,Point*,int,int,int,int);
inline AllocFn Allocate=nullptr;inline RoomAtFn RoomAt=nullptr;inline FreeFn FreePosition=nullptr;
inline void* Game=nullptr;inline unsigned Last=0,Next=0,Seed=1;inline bool Testing=false;
inline std::array<bool,133> Visited{};
inline std::array<unsigned,133> Pending{},Attempts{},RetryAt{};
struct Mob {unsigned id;int cls;unsigned event;Point p;};
struct Event {Encounter state;unsigned pillar=0xffffffffu;std::vector<unsigned> visuals;int queued=0;unsigned retry=0;};
inline std::vector<Event> Events;inline std::vector<Mob> Mobs;
struct Retired {int type,cls;unsigned id;};inline std::vector<Retired> Retirements;
struct Death {unsigned id;int cls,area;Point p;};inline std::vector<Death> Deaths;
inline Point position(void* u){return {Assassin::Coordinate(u,false),Assassin::Coordinate(u,true)};}
inline void remove(void* unit){if(!unit)return;if(void* room=SoulHarvest::Room(unit))Barbarian::RoomDelete(room,static_cast<unsigned*>(unit)[0],Barbarian::Id(unit));Barbarian::DeleteUnit(Game,unit);}
inline bool readTest(){
 wchar_t path[32768]{};auto mod=GetModuleHandleW(L"d2rl-workshop-combat.dll");if(!mod||!GetModuleFileNameW(mod,path,32768))return false;
 std::ifstream f(std::filesystem::path(path).parent_path().parent_path()/"config"/"workshop-leagues-test.txt");int v=0;return (f>>v)&&v==1;
}
inline void __cdecl death(const D2RL::PluginContext*,const D2RL::Lifecycle::MonsterDeathEvent* e,void*) noexcept {
 if(!Game||!e||e->structSize<D2RL::Lifecycle::MonsterDeathEventRequiredSize||Deaths.size()>=512)return;
 // Observation-only callback; spawning/removal is deferred to the next game tick.
 auto* unit=Curses::Find(Game,1,e->monster.id);if(!unit)return;
 Deaths.push_back({e->monster.id,int(e->monster.classId),e->levelId,position(unit)});
}
inline void log(const char* action,int area,int kind){if(Arena::Context){char text[160];std::snprintf(text,sizeof(text),"League: %s area=%d mechanic=%d",action,area,kind);Arena::Context->LogInfo(text);}}
inline bool same(void* u,int cls){return u&&static_cast<int*>(u)[1]==cls;}
inline unsigned living(unsigned index){unsigned n=0;for(auto& mob:Mobs)if(mob.event==index){auto* u=Curses::Find(Game,1,mob.id);if(same(u,mob.cls)&&MonsterRuntime::Living(u))++n;}return n;}
inline void retire(int type,int cls,unsigned id){auto* u=Curses::Find(Game,type,id);if(u){if(same(u,cls))remove(u);}else if(id!=0xffffffffu)Retirements.push_back({type,cls,id});}
inline void finish(unsigned index){
 auto& e=Events[index];e.state.phase=Phase::Complete;e.queued=0;e.state.kills.clear();
 retire(2,Pillars[int(e.state.kind)],e.pillar);e.pillar=0xffffffffu;
 for(auto id:e.visuals)retire(3,RiftMissile,id);e.visuals.clear();
 for(auto it=Mobs.begin();it!=Mobs.end();)if(it->event==index){retire(1,it->cls,it->id);it=Mobs.erase(it);}else ++it;
 log("finished",e.state.area,int(e.state.kind));
}
// Normalize native spawn stats to the zone; retain native player-count multipliers.
inline void balance(void* unit,int area){
 unsigned diff=static_cast<unsigned char*>(Game)[0x104];if(diff>2||area<0||area>=133)return;
 int old=std::clamp(Ramp::GetUnit(unit,12,0),1,511),level=std::clamp(ZoneBaseline[diff][area][0],1,511);
 auto scale=[&](int stat,int curve){int value=Ramp::GetUnit(unit,stat,0);if(value<=0)return;int64_t result=int64_t(value)*LevelCurve[diff][level][curve]*(curve==1?140:70)/(int64_t(std::max(1,LevelCurve[diff][old][curve]))*100);Ramp::SetUnit(unit,stat,int(std::clamp<int64_t>(result,1,0x3fffffff)),0);};
 scale(7,0);
 Ramp::SetUnit(unit,6,Ramp::GetUnit(unit,7,0),0); // Heal only at spawn, after setting the new maximum.
 for(int stat:{21,22,23,24,48,49,50,51,52,53,54,55,57,58})scale(stat,1);
 // Exp coefficients vary by family; recover native player-count scaling from the original row.
 int cls=static_cast<int*>(unit)[1],coefficient=100;
 for(int m=0;m<3;++m)for(int r=0;r<3;++r)for(int v=0;v<VariantCounts[m];++v)if(Classes[m][r][v]==cls)coefficient=ExperienceCoefficients[diff][m][v];
 int base=std::max(1,LevelCurve[diff][old][2]*coefficient/100),xp=Ramp::GetUnit(unit,13,0);
 Ramp::SetUnit(unit,13,int(std::clamp<int64_t>(int64_t(std::max(1,xp))*ZoneBaseline[diff][area][1]*2/base,1,0x3fffffff)),0);
 Ramp::SetUnit(unit,12,level,0);
}
inline void rankElite(void* unit,int rank){
 if(!MonsterRuntime::Init||!MonsterRuntime::OriginalRoll)return;
 auto* data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(unit)+0x10);if(!data)return;
 // Both League elite tiers use the existing unique affix runtime, including custom modifiers.
 auto& flags=*reinterpret_cast<unsigned short*>(data+0x1a);flags=(flags&~(2u|4u|16u))|8u;
 auto* mods=data+0x20;std::memset(mods,0,9);unsigned char used[256]{};
 auto range=MonsterModifiers::RollRange(static_cast<unsigned char*>(Game)[0x104]);
 unsigned count=range.first+random(Seed)%(range.second-range.first+1);
 if(rank==2)count=range.second;
 for(unsigned i=0;i<count&&i<9;++i){int mod=MonsterRuntime::RollNative(Game,unit,used,nullptr);if(mod<=0||mod>=256)break;mods[i]=static_cast<unsigned char>(mod);used[mod]=1;MonsterRuntime::Init(unit,mod,1);}
 MonsterRuntime::Track(Game,unit);Ramp::SetUnit(unit,6,Ramp::GetUnit(unit,7,0),0);
}
inline bool spawn(void* player,unsigned index,Point at,int rank,int variant){
 if(Mobs.size()>=MaxLiving*3||living(index)>=MaxLiving)return false;
 auto& e=Events[index];int cls=Classes[int(e.state.kind)][rank][variant];void* room=RoomAt(SoulHarvest::Room(player),at.x,at.y);if(!room)return false;
 void* unit=MonsterRuntime::Spawn(Game,room,at.x,at.y,cls,1,-1,0);if(!unit)return false;
 if(Arena::Level(unit)!=e.state.area){remove(unit);return false;}
 Mobs.push_back({Barbarian::Id(unit),cls,index,position(unit)});++e.state.spawned;
 balance(unit,e.state.area); // Preserve rank HP multipliers and native loot.
 if(rank>0)rankElite(unit,rank);
 Assassin::SetTarget(unit,player);return true;
}
inline void visual(void* player,Event& e,Point at,unsigned duration){
 Pulse::Params p{};p.flags=0x8021;p.owner=player;p.origin=player;p.missile=RiftMissile;p.level=1;p.x=at.x;p.y=at.y;p.targetX=at.x+1;p.targetY=at.y;p.range=duration;
 if(void* v=Pulse::OnCreate(Game,&p)){e.visuals.push_back(Barbarian::Id(v));if(Amazon::PublishMissiles){void* list[]={v};Amazon::PublishMissiles(player,list,1,nullptr,nullptr,0);}}
}
inline bool place(void* player,int kind,unsigned now,int direction){
 Point p=position(player);constexpr int dx[]={12,-12,0,0,9,-9,9,-9},dy[]={0,0,12,-12,9,9,-9,-9};
 for(int attempt=0;attempt<8;++attempt){int n=(direction+attempt)%8;Point at{p.x+dx[n],p.y+dy[n]};
  auto room=FreePosition(SoulHarvest::Room(player),&at,&at,3,0x3e01,0xc01,0);if(!room)continue;
  room=RoomAt(room,at.x,at.y);if(!room)continue;
  // Require an unchanged free position across the entire footprint and bypass ring.
  // Reject narrow corridors instead of allowing the allocator to nudge a probe.
  bool clear=true;
  for(int x=-5;x<=5&&clear;++x)for(int y=-5;y<=5;++y){
   Point wanted{at.x+x,at.y+y},found=wanted;
   if(!FreePosition(room,&found,&found,1,0x3e01,0xc01,0)||found.x!=wanted.x||found.y!=wanted.y){clear=false;break;}
  }
  if(!clear)continue;
  auto* pillar=Allocate(2,Pillars[kind],at.x,at.y,Game,room,1,0,0,nullptr);if(!pillar)continue;
  if(Arena::Level(pillar)!=Arena::Level(player)){remove(pillar);continue;}
  Event e;e.state.kind=Kind(kind);e.state.area=Arena::Level(player);e.state.center=at;e.state.lastSeen=now;e.state.rifts={at,Point{at.x+12,at.y+7},Point{at.x-12,at.y+7}};e.pillar=Barbarian::Id(pillar);Events.push_back(e);auto index=unsigned(Events.size()-1);
  if(kind==1){auto& event=Events.back();event.state.activate(now);event.queued=9;for(auto q:event.state.rifts)visual(player,event,q,Durations[kind]*TicksPerSecond);for(int edge=0;edge<2;++edge)for(int k=1;k<4;++k){auto a=event.state.rifts[edge],b=event.state.rifts[edge+1];visual(player,event,{a.x+(b.x-a.x)*k/4,a.y+(b.y-a.y)*k/4},Durations[kind]*TicksPerSecond);}}
  log("placed",e.state.area,kind);return true;
 }return false;
}
inline void frame(void* game){
 if(!game||!Arena::Enabled||!Arena::Level||!Allocate||!MonsterRuntime::Spawn||!Assassin::SetTarget)return;
 unsigned now=Ramp::Frame(game);LeagueStorage::frame(game,now);
 if(Game!=game||now<Last){Game=game;Events.clear();Mobs.clear();Deaths.clear();Retirements.clear();Visited={};Pending={};Attempts={};RetryAt={};Next=0;Seed=unsigned(GetTickCount())^now;Testing=readTest();log(Testing?"session TEST Blood Moor enabled":"session random encounters enabled",0,-1);}
 Last=now;if(now<Next)return;Next=now+5;
 for(auto it=Retirements.begin();it!=Retirements.end();){auto* u=Curses::Find(game,it->type,it->id);if(u){if(same(u,it->cls))remove(u);it=Retirements.erase(it);}else ++it;}
 std::vector<unsigned> players;AuraFeatures::Units(game,0,[&](void* p){if(MonsterRuntime::Living(p))players.push_back(Barbarian::Id(p));});
 for(auto id:players){auto* p=Curses::Find(game,0,id);if(!p)continue;int area=Arena::Level(p);if(!eligible(area)||now<100)continue;
  if(!Visited[area]){Visited[area]=true;
   if(Testing&&area==2)Pending[area]=7;
   else Pending[area]=rollMechanics(Seed);
   log("independent roll mask",area,int(Pending[area]));
   if(Pending[area])log("placement queued",area,-1);
  }
  if(!Pending[area]||now<RetryAt[area])continue;
  RetryAt[area]=now+50;++Attempts[area];
  for(int kind=0;kind<3;++kind)if(Pending[area]&(1u<<kind)){
   if(place(p,kind,now,kind*2)){Pending[area]&=~(1u<<kind);}
   else if(Attempts[area]==1||Attempts[area]%10==0)log("placement blocked; retrying",area,kind);
  }
  if(!Pending[area])log("all queued encounters placed",area,-1);
  else if(!(Testing&&area==2)&&Attempts[area]>=20){log("placement abandoned after 20 attempts",area,-1);Pending[area]=0;}
 }
 auto deaths=std::move(Deaths);Deaths.clear();
 for(auto& d:deaths){
  auto it=std::find_if(Mobs.begin(),Mobs.end(),[&](auto& m){return m.id==d.id&&m.cls==d.cls;});
  if(it!=Mobs.end()){auto& e=Events[it->event];bool beyond=e.state.kind==Kind::Beyond;++e.state.killed;Mobs.erase(it);if(beyond)continue;} // Night/Breach feed Beyond, Beyond itself cannot recurse.
  for(auto& e:Events)if(e.state.area==d.area&&e.state.clustered(d.p,now)){e.state.center=d.p;e.queued+=e.state.groups%4==0?1:4;log("Beyond cluster",d.area,2);}
 }
 
 for(unsigned i=0;i<Events.size();++i){auto& e=Events[i];auto& s=e.state;if(s.phase==Phase::Complete)continue;
  void* player=nullptr;for(auto id:players){auto* p=Curses::Find(game,0,id);if(p&&Arena::Level(p)==s.area){player=p;break;}}
  if(!player){if(s.phase==Phase::Active&&now-s.lastSeen>250)finish(i);continue;} // Waiting pillars survive waypoint scouting.
  s.lastSeen=now;
  if(s.phase==Phase::Waiting){auto* pillar=Curses::Find(game,2,e.pillar);if(same(pillar,Pillars[int(s.kind)])&&static_cast<unsigned*>(pillar)[3]!=0){s.activate(now);log("activated",s.area,int(s.kind));}else continue;}
  if(s.expired(now)){finish(i);continue;}
  if(s.kind==Kind::Night){
   if(now>=s.next&&s.spawned<180){s.next=now+75;e.queued=std::min(e.queued+6,12);++s.stage;}
  }else if(s.kind==Kind::Breach&&e.queued==0&&living(i)==0){
   if(s.stage>=3){finish(i);continue;}++s.stage;e.queued=s.stage==3?3:12;
  }
  if(now>=e.retry){e.retry=now+10;
   for(int count=0;count<4&&e.queued>0&&living(i)<MaxLiving;++count){
    int variant=int(random(Seed)%VariantCounts[int(s.kind)]),rank=0;Point at=s.center;
    if(s.kind==Kind::Night){at=position(player);at.x+=3*(int(random(Seed)%25)-12);at.y+=3*(int(random(Seed)%25)-12);rank=s.stage>=20&&random(Seed)%20==0?2:random(Seed)%7==0?1:0;}
    if(s.kind==Kind::Breach){at=s.rifts[(s.spawned+count)%3];rank=s.stage==3?2:s.stage>0?1:0;}
    if(s.kind==Kind::Beyond)rank=s.groups%4==0?2:s.groups>4?1:0;
    at.x+=int(random(Seed)%5)-2;at.y+=int(random(Seed)%5)-2;
    if(rank>0&&random(Seed)%4!=0)rank=0; // 75% fewer elite promotions in every mechanic.
    if(spawn(player,i,at,rank,variant))--e.queued;
   }
  }
  // Continuously pursue players in the encounter area, including across rooms.
  for(auto& m:Mobs)if(m.event==i){auto* unit=Curses::Find(game,1,m.id);if(!same(unit,m.cls)||!MonsterRuntime::Living(unit))continue;m.p=position(unit);Assassin::SetTarget(unit,player);if(s.kind==Kind::Night&&distance2(m.p,position(player))>625){auto target=position(player);Goblins::Move(game,unit,2,target.x,target.y);}}
 }
 std::vector<LeagueMap::Marker> markers;
 for(auto& e:Events)if(e.state.phase!=Phase::Complete&&e.pillar!=0xffffffffu){
  for(auto id:players){auto* p=Curses::Find(game,0,id);if(p&&Arena::Level(p)==e.state.area){auto* u=Curses::Find(game,2,e.pillar);if(same(u,Pillars[int(e.state.kind)])){if(LeagueMap::ClientX&&LeagueMap::ClientY)markers.push_back({LeagueMap::ClientX(u),LeagueMap::ClientY(u),int(e.state.kind)});}break;}}
 }
 LeagueMap::publish(std::move(markers));
 // A short-lived stat list refreshes only inside active Night areas. It cannot leak to town.
 for(auto id:players){auto* p=Curses::Find(game,0,id);if(!p)continue;bool dark=false;for(auto& e:Events)if(e.state.kind==Kind::Night&&e.state.phase==Phase::Active&&e.state.area==Arena::Level(p)){dark=true;break;}
  if(dark&&now%25<5&&Curses::OriginalApply){Curses::Curse effect{p,p,0,1,50,80,-100,DarknessState,nullptr};Curses::OriginalApply(&effect);}else if(!dark)AuraBuffs::Expire(game,p,DarknessState);
 }
}
inline bool install(const D2RL::PluginContext* c){
 if(!LeagueMap::install(c))c->LogInfo("League colored map markers unavailable: native/overlay contract mismatch.");
 if(!LeagueStorage::install(c))c->LogInfo("League orb storage unavailable: native contract mismatch.");
 const unsigned char operate[]={0x40,0x53,0x48,0x83,0xec,0x40,0x48,0x8b,0xd9,0x48,0x8b,0x49,0x08};
 if(!c->api->checkExpectedBytes(c,0x5900c0,operate,sizeof(operate)))return false;
 const unsigned char alloc[]={0x40,0x55,0x53,0x57,0x48,0x8d,0xac,0x24,0x70,0xfc,0xff,0xff};
 const unsigned char room[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x48,0x83,0xec,0x20};
 const unsigned char free[]={0x48,0x83,0xec,0x58,0x8b,0x84,0x24,0x90,0,0,0};
 if(!c->api->checkExpectedBytes(c,0x490410,alloc,sizeof(alloc))||!c->api->checkExpectedBytes(c,0x38dfb0,room,sizeof(room))||!c->api->checkExpectedBytes(c,0x3632f0,free,sizeof(free)))return false;
 Allocate=reinterpret_cast<AllocFn>(c->exeBase+0x490410);RoomAt=reinterpret_cast<RoomAtFn>(c->exeBase+0x38dfb0);FreePosition=reinterpret_cast<FreeFn>(c->exeBase+0x3632f0);
 const D2RL::LifecycleService* api=nullptr;if(c->QueryService(&api)!=D2RL::ServiceQueryResult::Success){Allocate=nullptr;return false;}
 D2RL::Lifecycle::MonsterDeathListener reg{D2RL::Lifecycle::MonsterDeathListenerSize,0,death,nullptr};D2RL::Lifecycle::ListenerHandle handle=0;bool ok=api->registerMonsterDeathListener(c,&reg,&handle)==D2RL::Lifecycle::Result::Success;if(!ok)Allocate=nullptr;else c->LogInfo("League encounters ready: native SecretDoor activation and orb storage retry.");return ok;
}
}
