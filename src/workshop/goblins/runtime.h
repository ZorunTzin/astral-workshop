#pragma once
#include "engine.h"
#include "bindings.h"
#include <cmath>
#include <filesystem>
#include <fstream>
namespace Goblins {
inline void* Game=nullptr;inline unsigned Last=0,Next=0,Seed=0;
inline Schedule Visits;
inline bool TestBloodMoor=false,TestPlaced=false;
inline int TestFamily=0;inline unsigned NextTest=0;
inline bool ReadTestMode(){
 HMODULE module=GetModuleHandleW(L"d2rl-workshop-combat.dll");if(!module)return false;
 wchar_t path[32768]{};DWORD n=GetModuleFileNameW(module,path,32768);if(!n||n>=32768)return false;
 std::ifstream file(std::filesystem::path(path).parent_path().parent_path()/"config"/"workshop-goblins-test.txt");
 int enabled=0;return (file>>enabled)&&enabled==1;
}
struct Runner {unsigned id;int cls,level;Escape escape;unsigned born,nextMove;bool wasChannel=false;unsigned lastTick=0;bool paused=false;int x=0,y=0;unsigned portal=0xffffffffU;bool trialWave=false;};
inline std::vector<Runner> Runners;
struct Death {unsigned id;int cls,level;};
inline std::mutex DeathMutex;inline std::vector<Death> Deaths;
struct Split {int cls,level,x,y;unsigned remaining=2,next=0;bool trialWave=false;};
inline std::vector<Split> Splits;
inline bool GoblinClass(int cls){return std::find(std::begin(Classes),std::end(Classes),cls)!=std::end(Classes);}
inline int ChildClass(int cls){return cls==Classes[6]?Classes[7]:cls==Classes[7]?Classes[8]:-1;}
inline void __cdecl OnDeath(const D2RL::PluginContext*,const D2RL::Lifecycle::MonsterDeathEvent* e,void*) noexcept {
 if(!e||e->structSize<D2RL::Lifecycle::MonsterDeathEventRequiredSize||e->monster.type!=D2RL::Lifecycle::UnitType::Monster||!GoblinClass(int(e->monster.classId)))return;
 std::lock_guard<std::mutex> lock(DeathMutex);if(Deaths.size()<512)Deaths.push_back({e->monster.id,int(e->monster.classId),e->levelId});
}
inline void SpawnTrialWave(void*,unsigned);
inline bool TrialWaveBusy();
inline int TrialWaveRemaining();
inline bool Install(const D2RL::PluginContext* c){
 Arena::SpawnGoblinWave=SpawnTrialWave;Arena::GoblinWaveBusy=TrialWaveBusy;Arena::GoblinWaveRemaining=TrialWaveRemaining;
 const D2RL::LifecycleService* api=nullptr;
 if(c->QueryService(&api)!=D2RL::ServiceQueryResult::Success||!D2RL::HasLifecycleServiceField(api,D2RL::LifecycleServiceSize)||!api->registerMonsterDeathListener)return false;
 D2RL::Lifecycle::MonsterDeathListener registration{D2RL::Lifecycle::MonsterDeathListenerSize,0,&OnDeath,nullptr};D2RL::Lifecycle::ListenerHandle handle=0;
 return api->registerMonsterDeathListener(c,&registration,&handle)==D2RL::Lifecycle::Result::Success;
}
inline void Log(const char* event,int level,int cls){if(!Arena::Context)return;char text[160]{};std::snprintf(text,sizeof(text),"Goblins: %s level=%d class=%d",event,level,cls);Arena::Context->LogInfo(text);}
inline bool Move(void* game,void* unit,int mode,int x,int y){
 if(!Barbarian::ModeInfo||!Barbarian::ModeChange)return false;
 alignas(8) unsigned char info[0x80]{};Barbarian::ModeInfo(unit,mode,info);
 void* target=nullptr;std::memcpy(info+0x10,&target,8);std::memcpy(info+0x18,&x,4);std::memcpy(info+0x1c,&y,4);
 return Barbarian::ModeChange(game,info,0)!=0;
}
// Publish removal before freeing the server unit; otherwise clients retain a ghost.
inline void Remove(void* unit){
 if(!unit)return;
 if(void* room=SoulHarvest::Room(unit))Barbarian::RoomDelete(room,static_cast<unsigned*>(unit)[0],Barbarian::Id(unit));
 Barbarian::DeleteUnit(Game,unit);
}
inline void ClosePortal(Runner& r){
 if(r.portal!=0xffffffffU){
  if(void* unit=Curses::Find(Game,1,r.id))if(int(static_cast<unsigned*>(unit)[1])==r.cls)AuraBuffs::Expire(Game,unit,EscapeState);
  r.portal=0xffffffffU;
 }
}
inline void OpenPortal(Runner& r,void* unit){
 if(r.portal!=0xffffffffU||!Curses::OriginalApply)return;
 Curses::Curse effect{unit,unit,0,1,int(r.escape.end-Ramp::Frame(Game)+1),-1,0,EscapeState,nullptr};
 if(Curses::OriginalApply(&effect))r.portal=r.id;
}
inline bool Spawn(void* player,int level,unsigned now,int family=-1,bool trialWave=false){
 const int cls=Classes[family>=0&&family<NaturalCount?family:Random(Seed)%NaturalCount];
 const int px=Assassin::Coordinate(player,false),py=Assassin::Coordinate(player,true);
 static constexpr int dx[]={10,7,0,-7,-10,-7,0,7},dy[]={0,7,10,7,0,-7,-10,-7};
 unsigned start=Random(Seed)%8;
 for(unsigned i=0;i<8;++i){unsigned n=(start+i)%8;
  void* unit=MonsterRuntime::Spawn(Game,SoulHarvest::Room(player),px+dx[n],py+dy[n],cls,1,-1,0);
  if(!unit)continue;
  if(Arena::Level(unit)!=level){Remove(unit);continue;}
  if(TrialArea(level))Arena::WaveProgression(unit);
  Runner r{Barbarian::Id(unit),cls,level,{},now,now,false};r.escape.start(now,Ramp::GetUnit(unit,6,0));r.trialWave=trialWave;r.lastTick=now;r.x=Assassin::Coordinate(unit,false);r.y=Assassin::Coordinate(unit,true);Runners.push_back(r);
  if(TrialArea(level)&&!trialWave)Visits.trialSpawned[level]=true;
  Log("spawn",level,cls);return true;
 }
 Log("no free spawn position",level,cls);return false;
}
inline bool WaveActive(){return Arena::GoblinWave(Arena::Current)&&(Arena::Current.phase==Arena::Phase::Countdown||Arena::Current.phase==Arena::Phase::Fighting);}
inline int TrialWaveRemaining(){int n=0;for(auto& r:Runners)if(r.trialWave)++n;for(auto& s:Splits)if(s.trialWave)n+=s.remaining;return n;}
inline bool TrialWaveBusy(){return std::any_of(Runners.begin(),Runners.end(),[](const Runner& r){return r.trialWave;})||std::any_of(Splits.begin(),Splits.end(),[](const Split& s){return s.trialWave;});}
inline void TrackWave(void* unit){
 Arena::Enemies.push_back(Barbarian::Id(unit));
 // Goblin lifecycle owns escape/unload/splits, not ordinary enemy recovery.
 int hp=int(std::clamp<int64_t>(int64_t(Ramp::GetUnit(unit,7,0))*Arena::Current.lifePercent()/100,256,0x3fffffff));
 Ramp::SetUnit(unit,7,hp,0);Ramp::SetUnit(unit,6,hp,0);
 Runners.back().escape.start(Ramp::Frame(Game),hp);
}
inline void SpawnTrialWave(void* player,unsigned now){
 // Defer until Goblins::Frame has initialized this session after Arena::Frame.
 if(Game!=Arena::Game)return;
 for(int n=0;n<8&&Arena::Pending>0;++n){
  if(!Spawn(player,Arena::Level(player),now,-1,true))break;
  auto* unit=Curses::Find(Game,1,Runners.back().id);if(unit)TrackWave(unit);
  --Arena::Pending;
 }
 if(!Arena::Pending){Arena::Current.phase=Arena::Phase::Fighting;Arena::Publish("Loot goblins: catch them before they escape!");}
}
inline void SpawnChildren(const std::vector<unsigned>& players,unsigned now){
 for(auto it=Splits.begin();it!=Splits.end();){
  auto& job=*it;if(job.trialWave&&!WaveActive()){it=Splits.erase(it);continue;}if(now<job.next){++it;continue;}job.next=now+25;
  if(TrialArea(job.level)&&Arena::Current.stage==job.level-165&&Arena::Current.phase==Arena::Phase::Choice){++it;continue;}
  void* anchor=nullptr;
  for(unsigned id:players){void* p=Curses::Find(Game,0,id);if(!p||Arena::Level(p)!=job.level)continue;
   int dx=Assassin::Coordinate(p,false)-job.x,dy=Assassin::Coordinate(p,true)-job.y;
   if(dx*dx+dy*dy<=2500){anchor=p;break;}}
  if(!anchor){++it;continue;}
  constexpr int dx[]={1,1,0,-1,-1,-1,0,1},dy[]={0,1,1,1,0,-1,-1,-1};
  unsigned start=Random(Seed)%8;
  for(unsigned attempt=0;attempt<16&&job.remaining;++attempt){unsigned n=(start+attempt)%8;int radius=2+int(attempt/8)*2;
   void* unit=MonsterRuntime::Spawn(Game,SoulHarvest::Room(anchor),job.x+dx[n]*radius,job.y+dy[n]*radius,job.cls,1,-1,0);
   if(!unit)continue;if(Arena::Level(unit)!=job.level){Remove(unit);continue;}
   if(TrialArea(job.level))Arena::WaveProgression(unit);
   Runner child{Barbarian::Id(unit),job.cls,job.level,{},now,now};child.escape.start(now,Ramp::GetUnit(unit,6,0));child.trialWave=job.trialWave;child.lastTick=now;child.x=Assassin::Coordinate(unit,false);child.y=Assassin::Coordinate(unit,true);Runners.push_back(child);if(job.trialWave)TrackWave(unit);--job.remaining;
   Log("gelatinous child",job.level,job.cls);
  }
  if(!job.remaining)it=Splits.erase(it);else ++it;
 }
}
inline void Frame(void* game){
 if(!game||!Arena::Enabled||!Arena::Level||!MonsterRuntime::Spawn||!Barbarian::DeleteUnit||!Barbarian::RoomDelete||!Barbarian::ModeInfo||!Barbarian::ModeChange)return;
 const unsigned now=Ramp::Frame(game);
 if(Game!=game||now<Last){Game=game;Visits={};Runners.clear();Splits.clear();{std::lock_guard<std::mutex> lock(DeathMutex);Deaths.clear();}Next=0;Seed=now^unsigned(GetTickCount())^0xa341316c;TestBloodMoor=ReadTestMode();TestPlaced=false;TestFamily=0;NextTest=0;}
 Last=now;if(now<Next)return;Next=now+5;
 // Capture identities before any spawn/delete mutates the game's unit chains.
 std::vector<unsigned> players;
 AuraFeatures::Units(game,0,[&](void* p){if(MonsterRuntime::Living(p))players.push_back(Barbarian::Id(p));});
 for(unsigned id:players){
  void* player=Curses::Find(game,0,id);if(!player)continue;int level=Arena::Level(player);
  if(TestBloodMoor&&level==2){
   Visits.visited[2]=true;
   // One encounter at a time: all seven can be tested without racing other timers.
   if(!TestPlaced&&now>=100&&now>=NextTest&&Runners.empty()&&Splits.empty()){
    NextTest=now+50;
    if(Spawn(player,level,now,TestFamily)){
     ++TestFamily;TestPlaced=TestFamily==NaturalCount;
     if(TestPlaced)Log("TEST: all seven variants placed",level,0);
    }
   }
  }
  bool trial=TrialArea(level),roll=false;
  if(trial){if(!Arena::GoblinWave(Arena::Current)&&Arena::Current.phase==Arena::Phase::Fighting&&Arena::Current.stage==level-165)roll=Visits.takeTrial(level,Arena::Current.wave);}
  else if(now>=100)roll=Visits.takeWorld(level);
  if(roll&&Random(Seed)%100<(trial?TrialChance:WorldChance))Spawn(player,level,now);
 }
 std::vector<Death> deaths;{std::lock_guard<std::mutex> lock(DeathMutex);deaths.swap(Deaths);}
 for(auto it=Runners.begin();it!=Runners.end();){
  auto& r=*it;void* unit=Curses::Find(game,1,r.id);
  if(r.trialWave&&!WaveActive()){ClosePortal(r);if(unit&&int(static_cast<unsigned*>(unit)[1])==r.cls)Remove(unit);it=Runners.erase(it);continue;}
  // Death events survive corpse removal. Missing units alone never cause a split.
  const bool confirmed=std::any_of(deaths.begin(),deaths.end(),[&](const Death& d){return d.id==r.id&&d.cls==r.cls&&(d.level<0||d.level==r.level);});
  const bool same=unit&&int(static_cast<unsigned*>(unit)[1])==r.cls;
  if(same){r.x=Assassin::Coordinate(unit,false);r.y=Assassin::Coordinate(unit,true);}
  if(confirmed||(same&&!MonsterRuntime::Living(unit))){
   ClosePortal(r);
   int child=ChildClass(r.cls);if(child>=0)Splits.push_back({child,r.level,r.x,r.y,2,now,r.trialWave});
   it=Runners.erase(it);continue;
  }
  if(!unit){ClosePortal(r);
   if(r.trialWave&&now-r.lastTick>FirstChannelDelay+ChannelDuration+125){Arena::RetiredEnemies.push_back({r.id,r.cls,0});it=Runners.erase(it);continue;}
   ++it;continue;}
  if(!same){ClosePortal(r);it=Runners.erase(it);continue;}
  if(Arena::Level(unit)!=r.level){ClosePortal(r);Remove(unit);it=Runners.erase(it);continue;}
  
  const unsigned elapsed=now-r.lastTick;r.lastTick=now;
  // The card overlay prevents normal play: preserve the chase while choosing.
  if(TrialArea(r.level)&&Arena::Current.stage==r.level-165&&(Arena::Current.phase==Arena::Phase::Choice||(r.trialWave&&Arena::Current.phase==Arena::Phase::Countdown))){
   r.escape.due+=elapsed;r.escape.end+=elapsed;r.nextMove+=elapsed;ClosePortal(r);r.wasChannel=false;
   if(!r.paused)Move(game,unit,1,Assassin::Coordinate(unit,false),Assassin::Coordinate(unit,true));
   r.paused=true;++it;continue;
  }
  r.paused=false;
  if(r.escape.tick(now,Ramp::GetUnit(unit,6,0))){Log("escaped",r.level,r.cls);ClosePortal(r);Remove(unit);it=Runners.erase(it);continue;}
  if(r.escape.channel){if(!r.wasChannel){Move(game,unit,1,r.x,r.y);OpenPortal(r,unit);Log("escape channel",r.level,r.cls);}r.wasChannel=true;++it;continue;}
  ClosePortal(r);r.wasChannel=false;
  if(now>=r.nextMove){
   r.nextMove=now+10;void* nearest=nullptr;double distance=1e30;
   int x=Assassin::Coordinate(unit,false),y=Assassin::Coordinate(unit,true);
   for(unsigned id:players){void* p=Curses::Find(game,0,id);if(!p||Arena::Level(p)!=r.level)continue;
    double dx=x-Assassin::Coordinate(p,false),dy=y-Assassin::Coordinate(p,true),d=dx*dx+dy*dy;
    if(d<distance){nearest=p;distance=d;}}
   if(nearest){
    double dx=x-Assassin::Coordinate(nearest,false),dy=y-Assassin::Coordinate(nearest,true),length=std::sqrt(dx*dx+dy*dy);
    if(length<1){dx=1;dy=0;length=1;}
    dx=dx/length*8;dy=dy/length*8;
    // Try angled alternatives when a wall blocks the direct retreat.
    for(int attempt=0;attempt<5;++attempt){double angle=(attempt==0?0:(attempt%2?1:-1)*((attempt+1)/2)*0.65);
     int tx=x+int(std::round(dx*std::cos(angle)-dy*std::sin(angle))),ty=y+int(std::round(dx*std::sin(angle)+dy*std::cos(angle)));
     if(Move(game,unit,2,tx,ty))break;
    }
   }
  }
  ++it;
 }
 SpawnChildren(players,now);
}
}
