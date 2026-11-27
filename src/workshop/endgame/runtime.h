#pragma once
#include "engine.h"
#include "bindings.h"
#include "boss_bindings.h"
#include "../progression/bindings.h"
#include <D2RLPlugin/lifecycle_events.h>
#pragma comment(lib,"user32.lib")
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
namespace Arena {
using namespace D2RL;
inline const PluginContext* Context=nullptr;
}
#include "seal_portal.h"
namespace Arena {
inline const OverlayService* OverlayApi=nullptr;
inline Reimplementations::GetLevelIdFromUnitFn Level=nullptr;
inline Overlay::CallbackHandle OverlayHandle=0;
inline bool Enabled=false;

// Disabled test override; production uses native monster life and Trial scaling.
inline constexpr bool LowLifePlaytest=false;
inline int PlaytestLife(const Run& run){return int(int64_t(10*256)*(100+12*(run.wave-1))*(100+run.bonuses[0])/10000);}
inline unsigned ChoiceAvailableAt=0,NextRegen=0,NextProc=0,NextWaveDiagnostic=0;
inline std::array<int,3> WaveClasses{};
inline unsigned ProcDepth=0;
inline bool (*LeagueClass)(int)=nullptr;
inline int (*LeaguePrepareClass)(int)=nullptr;
inline void (*LeagueSpawned)(void*)=nullptr;
inline void (*PrepareLeagueWave)()=nullptr;
inline bool (*LeagueBusy)()=nullptr;
inline bool LeagueWave(const Run& run){return run.bonuses[int(Mod::LeagueWave)]!=0;}
inline bool GoblinWave(const Run& run){return run.bonuses[int(Mod::LootGoblinWave)]!=0;}
inline void (*SpawnGoblinWave)(void*,unsigned)=nullptr;
inline bool (*GoblinWaveBusy)()=nullptr;
inline int (*GoblinWaveRemaining)()=nullptr;
inline bool BossWave(const Run& run){return !GoblinWave(run)&&!LeagueWave(run)&&run.wave>0&&run.wave%10==0;}
inline int WaveCount(const Run& run){return run.count()+(BossWave(run)?8:0);}
// Explicit ranks on milestone escorts override the random one-wave event.
inline int MilestoneRank(const Run& run,int index){if(!BossWave(run)||index<run.count())return 0;int n=index-run.count();return n==0?3:n<=5?1:2;}
inline bool TrackedClass(int cls){return (LeagueClass&&LeagueClass(cls))||std::find(std::begin(MonsterClasses),std::end(MonsterClasses),cls)!=std::end(MonsterClasses)||std::find(std::begin(BossClasses),std::end(BossClasses),cls)!=std::end(BossClasses);}
inline bool NaturalRanks=true,ChampionPack=false;
inline Run Current;inline void* Game=nullptr;inline unsigned Last=0,Next=0,SpawnDeadline=0,ReadyAt=0,PortalCleanupAt=0;
inline unsigned Used=0,Attempt=0;inline int Pending=0;
inline std::vector<unsigned> ConfirmedDead;
struct DeathNotice {void* game;unsigned id;int level;};
inline std::mutex DeathMutex;inline std::vector<DeathNotice> DeathQueue;
inline void __cdecl OnDeath(const PluginContext*,const Lifecycle::MonsterDeathEvent* e,void*) noexcept {
 if(!e||e->structSize<Lifecycle::MonsterDeathEventRequiredSize||e->monster.type!=Lifecycle::UnitType::Monster||!TrackedClass(int(e->monster.classId)))return;
 std::lock_guard<std::mutex> lock(DeathMutex);
 if(DeathQueue.size()<512)DeathQueue.push_back({Game,e->monster.id,FirstLevel+Current.stage-1});
}
struct SpawnRecord{unsigned id;int cls;unsigned missingSince=0;int rank=0;};
inline std::vector<SpawnRecord> SpawnRecords;inline std::vector<SpawnRecord> RetiredEnemies;
inline std::vector<unsigned> Enemies;inline int EnterX=0,EnterY=0;
// UI callbacks only read a copied view and enqueue a versioned choice. All game
// mutations, spawning and reward creation run on the authoritative game thread.
struct View {Run run;int alive=0;bool visible=false;std::string note;bool choiceReady=false;};
inline View Display;inline std::mutex ViewMutex;
inline std::atomic<std::uint64_t> Command{0};
inline bool ArenaLevel(void* u){return u&&Level&&Level(u)>=FirstLevel&&Level(u)<FirstLevel+5;}
inline void* Player(){return Curses::Find(Game,0,Current.owner);}
// Record confirmed death while the native unit still exists. Never infer a kill
// from disappearance: room unloading can temporarily hide a living monster.
inline void ObserveDeath(void* game,void* unit){
 if(!Enabled||game!=Game||(Current.phase!=Phase::Fighting&&Current.phase!=Phase::Countdown)||!unit||!Curses::Monster(unit))return;
 unsigned id=Barbarian::Id(unit);
 if(std::find(Enemies.begin(),Enemies.end(),id)==Enemies.end()||MonsterRuntime::Living(unit))return;
 std::lock_guard<std::mutex> lock(DeathMutex);
 if(DeathQueue.size()<512)DeathQueue.push_back({game,id,FirstLevel+Current.stage-1});
}
inline void AfterDamage(void* game,void*,void* defender,void*){ObserveDeath(game,defender);}
inline void RequestPanelSync();
inline int Remaining(){if(GoblinWave(Current)&&GoblinWaveRemaining)return Pending+GoblinWaveRemaining();int n=0;for(unsigned id:Enemies)if(std::find(ConfirmedDead.begin(),ConfirmedDead.end(),id)==ConfirmedDead.end())++n;return n;}
inline void Publish(const char* note=""){
 std::lock_guard<std::mutex> lock(ViewMutex);Display={Current,Remaining(),ArenaLevel(Player()),note,Last>=ChoiceAvailableAt};RequestPanelSync();
}
inline void ClearBuff(void* player){if(!player)return;if(void* list=Ramp::OriginalList(player,BuffState)){for(auto spec:StatBonuses)if(Positive[spec.mod])Curses::Write(Game,list,spec.stat,0);Curses::Write(Game,list,80,0);AuraBuffs::Expire(Game,player,BuffState);int hp=Ramp::GetUnit(player,6,0),cap=Ramp::GetUnit(player,7,0);if(hp>cap)Ramp::SetUnit(player,6,cap,0);if(SoulHarvest::Refresh)SoulHarvest::Refresh(player);}}
inline void ApplyBuff(void* player){if(!player)return;if(void* list=Barbarian::Buff(Game,player,0,1,BuffState,50)){
 for(auto spec:StatBonuses)if(Positive[spec.mod])Curses::Write(Game,list,spec.stat,0);
 for(int stat=0;stat<400;++stat){int value=0;for(auto spec:StatBonuses)if(Positive[spec.mod]&&!Attributes[spec.mod]&&spec.stat==stat)value+=Current.bonuses[spec.mod]*spec.mult;if(value)Curses::Write(Game,list,stat,value);}
 for(int stat=0;stat<4;++stat){int percent=0;for(auto spec:StatBonuses)if(Attributes[spec.mod]&&spec.stat==stat)percent+=Current.bonuses[spec.mod];if(percent)Curses::Write(Game,list,stat,int(std::min<int64_t>(1000000,int64_t(Ramp::GetUnit(player,stat,0))*percent/100)));}
 Curses::Write(Game,list,80,Current.lootBonus());}}
inline void MonsterBonuses(void* u){
 // Run/walk, attack, cast and recovery; applied once per spawn.
 for(int stat:{67,68,99,102,105})Ramp::SetUnit(u,stat,Ramp::GetUnit(u,stat,0)+Current.speedPercent(),0);
 for(auto spec:StatBonuses)if(!Positive[spec.mod]&&Current.bonuses[spec.mod]){
  int base=Ramp::GetUnit(u,spec.stat,0),add=Current.bonuses[spec.mod]*spec.mult;
  if(spec.mod==int(Mod::MonsterResist)||spec.mod==int(Mod::MonsterPhysical))add=std::max(0,std::min(add,100-base));
  if(spec.mod==int(Mod::MonsterPoison))add=add*256/25;
  Ramp::SetUnit(u,spec.stat,base+add,0);
 }
 if(Current.bonuses[int(Mod::MonsterPoison)])Ramp::SetUnit(u,59,75,0);
 for(auto pair:{std::pair<int,int>{31,int(Mod::MonsterArmor)},{19,int(Mod::MonsterAim)}})Ramp::SetUnit(u,pair.first,int(int64_t(Ramp::GetUnit(u,pair.first,0))*(100+Current.bonuses[pair.second])/100),0);
}
inline void RemoveEnemies(){
 for(unsigned id:Enemies)if(void* u=Curses::Find(Game,1,id)){
  if(!ArenaLevel(u))continue;
  // Same paired room/client deletion path used by expiring private summons.
  if(Barbarian::RoomDelete)Barbarian::RoomDelete(SoulHarvest::Room(u),1,id);
  if(Barbarian::DeleteUnit)Barbarian::DeleteUnit(Game,u);
 }
 Enemies.clear();ConfirmedDead.clear();Pending=0;
}
inline bool IsArenaReturnPortal(void* u){
 return u&&static_cast<unsigned*>(u)[0]==2&&static_cast<unsigned*>(u)[1]==60&&ArenaLevel(u);
}
inline void RemoveReturnPortals(unsigned now){
 // Native portal entry needs the destination object even while its room is
 // preloaded. Never delete it until the owner has arrived and a full second
 // has elapsed; only touch the currently occupied trial, not other tiers.
 void* player=Player();
 if(!PortalCleanupAt||now<PortalCleanupAt||!player||!ArenaLevel(player)||
    Level(player)!=FirstLevel+Current.stage-1)return;
 // Server unit tables have 128 buckets per type. Collect before deleting so
 // native unlinking cannot invalidate the traversal; leave town-side objects
 // and blue town portals untouched. Retry on subsequent frames for room load.
 std::vector<void*> portals;
 auto* raw=static_cast<unsigned char*>(Game);
 for(unsigned bucket=0;bucket<128;++bucket){
  void* u=nullptr;std::memcpy(&u,raw+0x2230+2*0x400+8*bucket,8);
  for(unsigned links=0;u&&links<4096;++links){
   void* next=nullptr;std::memcpy(&next,static_cast<unsigned char*>(u)+0x158,8);
   if(IsArenaReturnPortal(u)&&Level(u)==Level(player))portals.push_back(u);u=next;
  }
 }
 for(void* u:portals){
  if(Barbarian::RoomDelete)Barbarian::RoomDelete(SoulHarvest::Room(u),2,Barbarian::Id(u));
  if(Barbarian::DeleteUnit)Barbarian::DeleteUnit(Game,u);
 }
}
inline bool IsTrialGroundLoot(void* u){return u&&static_cast<unsigned*>(u)[0]==4&&static_cast<unsigned*>(u)[3]==3&&ArenaLevel(u)&&Level(u)==FirstLevel+Current.stage-1;}
inline void ClearGroundLoot(){
 if(!Game||!Player()||Current.phase!=Phase::Choice||!ArenaLevel(Player()))return;
 // Native GetUnitBucket (RVA 0x4925b0): items use 0x5c6*8, missiles 0x646*8.
 std::vector<unsigned> ids;unsigned removed=0;
 auto* raw=static_cast<unsigned char*>(Game);
 for(unsigned bucket=0;bucket<128;++bucket){void* u=nullptr;std::memcpy(&u,raw+0x2e30+8*bucket,8);
  for(unsigned links=0;u&&links<4096;++links){void* next=nullptr;std::memcpy(&next,static_cast<unsigned char*>(u)+0x158,8);if(IsTrialGroundLoot(u))ids.push_back(Barbarian::Id(u));u=next;}}
 for(unsigned id:ids)if(void* u=Curses::Find(Game,4,id))if(IsTrialGroundLoot(u)){
  if(Barbarian::RoomDelete)Barbarian::RoomDelete(SoulHarvest::Room(u),4,id);
  if(Barbarian::DeleteUnit){Barbarian::DeleteUnit(Game,u);++removed;}
 }
 if(Context){char line[150]{};std::snprintf(line,sizeof(line),"Arena ground cleanup: matched=%u removed=%u",unsigned(ids.size()),removed);Context->LogInfo(line);}
}
inline void End(bool death){if(!Current.finish(death))return;ClearBuff(Player());RemoveEnemies();Command=0;Publish(death?"Run ended. Loot already dropped by enemies remains on the ground.":"Run complete. Collect enemy drops and use a town portal to leave.");}
inline void StartWave(unsigned now){
 std::array<int,sizeof(MonsterClasses)/sizeof(int)> pool{};std::copy(std::begin(MonsterClasses),std::end(MonsterClasses),pool.begin());
 for(int i=0;i<3;++i){int j=i+(Current.random()>>8)%(pool.size()-i);std::swap(pool[i],pool[j]);WaveClasses[i]=pool[i];}
 if(LeagueWave(Current)&&PrepareLeagueWave)PrepareLeagueWave();
 ChampionPack=NaturalRanks&&((Current.random()>>8)%100)<8;
 NextRegen=now+25;NextProc=now;NextWaveDiagnostic=now+250;
Current.phase=Phase::Countdown;Pending=WaveCount(Current);Attempt=0;ReadyAt=now+75;SpawnDeadline=now+400;Enemies.clear();SpawnRecords.clear();ConfirmedDead.clear();Command=0;}
inline bool RankWaveMonster(void* u,int rank=0){
 if(rank==3)return true;
 bool champion=rank==1||(rank==0&&Current.bonuses[int(Mod::ChampionWave)]!=0),elite=rank==2||(rank==0&&Current.bonuses[int(Mod::EliteWave)]!=0);
 if(!champion&&!elite)return true;
 if(!MonsterRuntime::Init||(!champion&&!MonsterRuntime::OriginalRoll))return false;
 void* data=nullptr;std::memcpy(&data,static_cast<unsigned char*>(u)+0x10,8);if(!data)return false;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(data)+0x1a,2);
 flags=(flags&~(2u|4u|16u))|8u|(champion?4u:0u);std::memcpy(static_cast<unsigned char*>(data)+0x1a,&flags,2);
 auto* mods=static_cast<unsigned char*>(data)+0x20;std::memset(mods,0,9);
 unsigned char used[256]{};std::array<std::uintptr_t,2> weights{};
 for(int id:{1,2,4})MonsterRuntime::Init(u,id,1);
 auto range=MonsterModifiers::RollRange(static_cast<unsigned char*>(Game)[0x104]);
 int desired=champion?1:range.first+int((Current.random()>>8)%(range.second-range.first+1));
 for(int slot=0;slot<desired;++slot){int mod=champion?16:MonsterRuntime::RollNative(Game,u,used,weights.data());
  if(mod==0&&(MonsterRuntime::MaskOf(u)&MonsterModifiers::Bit(MonsterModifiers::Raid)))break;
  if(mod<=0||mod>=256)return false;mods[slot]=static_cast<unsigned char>(mod);used[mod]=1;MonsterRuntime::Init(u,mod,1);}
 return true;
}
// Apply once after native rank initialization, also on replacement spawns.
inline void WaveProgression(void* unit){
 const int oldLevel=Ramp::GetUnit(unit,12,0),xp=Ramp::GetUnit(unit,13,0);
 Ramp::SetUnit(unit,13,int(std::min<int64_t>(0x3fffffff,1LL*Progression::ScaleExperience(xp,oldLevel,Current.wave)*Current.stagePercent()/100)),0);
 Ramp::SetUnit(unit,12,Progression::WaveLevel(Current.wave),0);
}
// Keep a two-tile margin inside the private 32-tile collision square.
inline bool SafePosition(int x,int y){
 const int left=(2000+100*(Current.stage-1))*5,bottom=2000*5;
 return x>=left+10&&x<=left+150&&y>=bottom+10&&y<=bottom+150;
}
inline void* SpawnInside(void* player,int x,int y,int cls){
 if(!SafePosition(x,y))return nullptr;
 if(LeagueWave(Current)&&LeaguePrepareClass)cls=LeaguePrepareClass(cls);
 auto u=MonsterRuntime::Spawn(Game,SoulHarvest::Room(player),x,y,cls,1,-1,0);
 if(u&&(!SafePosition(Assassin::Coordinate(u,false),Assassin::Coordinate(u,true))||Level(u)!=Level(player))){
  if(Barbarian::RoomDelete)Barbarian::RoomDelete(SoulHarvest::Room(u),1,Barbarian::Id(u));
  if(Barbarian::DeleteUnit)Barbarian::DeleteUnit(Game,u);return nullptr;
 }
 return u;
}
inline void SpawnWave(void* player,unsigned now){
 if(now<ReadyAt)return;
 if(GoblinWave(Current)){if(SpawnGoblinWave)SpawnGoblinWave(player,now);return;}
 // Ring placement is collision-aware; try a bounded number per frame and retry
 // blocked positions. Failed spawns never count as defeated enemies.
 int tries=0;while(Pending>0&&tries++<8&&now<SpawnDeadline){
  double angle=(Current.random()%6283)/1000.;int radius=2*((Attempt>80?12:28)+(Current.random()>>8)%17);
  int x=Assassin::Coordinate(player,false)+int(std::cos(angle)*radius),y=Assassin::Coordinate(player,true)+int(std::sin(angle)*radius);
  int index=WaveCount(Current)-Pending;
  int rank=Current.bonuses[int(Mod::ChampionWave)]||Current.bonuses[int(Mod::EliteWave)]?0:ChampionPack&&index<3?1:NaturalRanks&&((Current.random()>>8)%100)<std::min(100,2+SealElite)?2:NaturalRanks&&((Current.random()>>8)%100)<SealChampion?1:0;
  int milestone=MilestoneRank(Current,index);if(milestone)rank=milestone;
  int cls=rank==3?BossClasses[Current.stage-1]:WaveClasses[rank==1?0:index%3];
  void* u=SpawnInside(player,x,y,cls);++Attempt;if(!u)continue;if(!RankWaveMonster(u,rank)){if(Barbarian::DeleteUnit)Barbarian::DeleteUnit(Game,u);continue;}
  WaveProgression(u);
  int hp=LowLifePlaytest?PlaytestLife(Current):int(std::clamp<std::int64_t>(int64_t(Ramp::GetUnit(u,7,0))*Current.lifePercent()/100,256,0x3fffffff));
  if(LowLifePlaytest)Ramp::SetUnit(u,74,0,0);
  Ramp::SetUnit(u,7,hp,0);Ramp::SetUnit(u,6,hp,0);MonsterBonuses(u);if(LeagueWave(Current)&&LeagueSpawned)LeagueSpawned(u);
  Assassin::SetTarget(u,player);Enemies.push_back(Barbarian::Id(u));SpawnRecords.push_back({Barbarian::Id(u),int(static_cast<unsigned*>(u)[1]),0,rank});--Pending;
 }
 if(!Pending){Current.phase=Phase::Fighting;Publish();}
 else if(now>=SpawnDeadline){SpawnDeadline=now+250;if(Context)Context->LogWarn("Arena: blocked placement; preserving wave and retrying remaining spawns.");Publish("Waiting for free ground for remaining enemies.");}
}
inline void RecoverMissing(void* player,unsigned now){
 for(auto& record:SpawnRecords){
  if(std::find(ConfirmedDead.begin(),ConfirmedDead.end(),record.id)!=ConfirmedDead.end())continue;
  if(Curses::Find(Game,1,record.id)){record.missingSince=0;continue;}
  if(!record.missingSince){record.missingSince=now;continue;}
  if(now-record.missingSince<125)continue;
  // Replace, never credit, an unavailable enemy. If the old unit returns from
  // room storage later, retire it without drops to avoid duplicate enemies.
  auto tracked=std::find(Enemies.begin(),Enemies.end(),record.id);if(tracked==Enemies.end())continue;
  for(int attempt=0;attempt<8;++attempt){double angle=(Current.random()%6283)/1000.;int radius=2*(28+Current.random()%17);
   int x=Assassin::Coordinate(player,false)+int(std::cos(angle)*radius),y=Assassin::Coordinate(player,true)+int(std::sin(angle)*radius);
   void* u=SpawnInside(player,x,y,record.cls);if(!u)continue;
   if(!RankWaveMonster(u,record.rank)){if(Barbarian::DeleteUnit)Barbarian::DeleteUnit(Game,u);continue;}
   WaveProgression(u);
  int hp=LowLifePlaytest?PlaytestLife(Current):int(std::clamp<int64_t>(int64_t(Ramp::GetUnit(u,7,0))*Current.lifePercent()/100,256,0x3fffffff));
   if(LowLifePlaytest)Ramp::SetUnit(u,74,0,0);Ramp::SetUnit(u,7,hp,0);Ramp::SetUnit(u,6,hp,0);MonsterBonuses(u);if(LeagueWave(Current)&&LeagueSpawned)LeagueSpawned(u);Assassin::SetTarget(u,player);
   unsigned old=record.id;record.id=Barbarian::Id(u);record.missingSince=0;*tracked=record.id;if(old!=record.id)RetiredEnemies.push_back({old,record.cls,0});
   char line[180]{};std::snprintf(line,sizeof(line),"Arena recovered missing enemy: trial=%d wave=%d old=%u replacement=%u class=%d",Current.stage,Current.wave,old,record.id,record.cls);if(Context)Context->LogInfo(line);return;
  }
  record.missingSince=now;return; // bounded retry, no completion or reward credit
 }
}
inline void Frame(void* game){
 if(!Enabled||!game)return;unsigned now=Ramp::Frame(game);
 if(Game!=game||now<Last){Game=game;Current={};SpawnRecords.clear();RetiredEnemies.clear();Used=0;Next=0;Enemies.clear();ConfirmedDead.clear();{std::lock_guard<std::mutex> lock(DeathMutex);DeathQueue.clear();}PortalCleanupAt=0;Command=0;Publish();}
 Last=now;if(now<Next)return;Next=now+5;
 for(auto it=RetiredEnemies.begin();it!=RetiredEnemies.end();){void* u=Curses::Find(game,1,it->id);if(u){if(static_cast<unsigned*>(u)[1]==unsigned(it->cls)&&std::find(Enemies.begin(),Enemies.end(),it->id)==Enemies.end()&&Barbarian::DeleteUnit)Barbarian::DeleteUnit(game,u);it=RetiredEnemies.erase(it);}else ++it;}
 RemoveReturnPortals(now);
 {std::lock_guard<std::mutex> lock(DeathMutex);
  for(const auto& d:DeathQueue)if(d.game==Game&&d.level==FirstLevel+Current.stage-1&&std::find(Enemies.begin(),Enemies.end(),d.id)!=Enemies.end()&&std::find(ConfirmedDead.begin(),ConfirmedDead.end(),d.id)==ConfirmedDead.end())ConfirmedDead.push_back(d.id);
  DeathQueue.clear();
 }
 void* player=Current.owner==~0u?nullptr:Player();
 if(Current.phase==Phase::Idle){
  AuraFeatures::Units(game,0,[&](void* u){if(player||!ArenaLevel(u)||!MonsterRuntime::Living(u))return;player=u;});
  if(!player)return;
  if(*(static_cast<unsigned char*>(game)+0x104)!=2)return;
  int stage=Level(player)-FirstLevel+1;
  if(Used&(1u<<(stage-1)))return;
  Current.start(stage,Barbarian::Id(player),now^Barbarian::Id(player)^unsigned(GetTickCount()));SealPortal::Apply(game,now,Current);Used|=1u<<(stage-1);
  PortalCleanupAt=now+25;
  EnterX=Assassin::Coordinate(player,false);EnterY=Assassin::Coordinate(player,true);StartWave(now);Publish("Prepare: first wave begins in three seconds.");return;
 }
 if(Current.phase==Phase::Finished||Current.phase==Phase::Failed){
  Publish("Run ended. Collect enemy drops; use a town portal to leave.");
  // Do not let old portals restart cleared trials for free. Other tier portals
  // remain available once the prior payout has completed.
  if(player&&!ArenaLevel(player)){Current={};Publish();}
  return;
 }
 if(!player){End(true);return;}
 if(!MonsterRuntime::Living(player)){End(true);return;}
 if(!ArenaLevel(player)||Level(player)!=FirstLevel+Current.stage-1){End(true);return;}
 ApplyBuff(player);
 if(Current.phase==Phase::Countdown){SpawnWave(player,now);return;}
 if(Current.phase==Phase::Fighting){
  if(now>=NextRegen){NextRegen=now+25;for(unsigned id:Enemies)if(void* u=Curses::Find(game,1,id))if(MonsterRuntime::Living(u)){int hp=Ramp::GetUnit(u,6,0),cap=Ramp::GetUnit(u,7,0);Ramp::SetUnit(u,6,std::min(cap,hp+int(int64_t(cap)*Current.bonuses[int(Mod::MonsterRegen)]/100)),0);}}
  bool alive=false,missing=false;for(unsigned id:Enemies){
   if(std::find(ConfirmedDead.begin(),ConfirmedDead.end(),id)!=ConfirmedDead.end())continue;
   void* u=Curses::Find(game,1,id);if(!u){missing=true;continue;}
   if(MonsterRuntime::Living(u)){alive=true;Assassin::SetTarget(u,player);}else ConfirmedDead.push_back(id);
  }
  if(GoblinWave(Current)){alive=GoblinWaveBusy?GoblinWaveBusy():true;missing=false;}
  if(LeagueWave(Current)&&LeagueBusy&&LeagueBusy())alive=true;
  if(!alive&&!missing){RemoveEnemies();Current.clear();ChoiceAvailableAt=now+75;Context->LogInfo("Arena: wave cleared; modifier cards available in three seconds.");for(auto offer:Current.offers){char line[180]{};std::snprintf(line,sizeof(line),"Arena offer: wave=%d name=%s positive=%d tier=%d",Current.wave,Names[int(offer.mod)],Positive[int(offer.mod)]?1:0,offer.tier);Context->LogInfo(line);}Publish("Wave cleared. Choose one modifier or secure your reward.");}
  else {
   if(!alive&&missing)RecoverMissing(player,now);
   if(now>=NextWaveDiagnostic){NextWaveDiagnostic=now+250;for(unsigned id:Enemies){if(std::find(ConfirmedDead.begin(),ConfirmedDead.end(),id)!=ConfirmedDead.end())continue;void* u=Curses::Find(game,1,id);char line[256]{};std::snprintf(line,sizeof(line),"Arena remaining: trial=%d wave=%d id=%u present=%d class=%u mode=%u hp=%d x=%d y=%d",Current.stage,Current.wave,id,u?1:0,u?static_cast<unsigned*>(u)[1]:0,u?static_cast<unsigned*>(u)[3]:0,u?Ramp::GetUnit(u,6,0):0,u?Assassin::Coordinate(u,false):0,u?Assassin::Coordinate(u,true):0);if(Context)Context->LogInfo(line);}}
   Publish(missing?"A wave enemy is temporarily unavailable; return to the combat area.":"");
  }
  return;
 }
 if(Current.phase==Phase::Choice){Publish();if(now<ChoiceAvailableAt)return;auto cmd=Command.exchange(0);if(cmd){unsigned epoch=unsigned(cmd>>32),choice=unsigned(cmd&0xffffffffu);if(epoch==Current.epoch){if(choice==7){ClearGroundLoot();Publish();}else if(choice==4)End(false);else if(choice==5||choice==6){if(Current.reroll(choice==6,epoch))Publish();}else if(Current.choose(int(choice)-1,epoch)){StartWave(now);Publish("Modifier added. Next wave begins in three seconds.");}}}
 }
}
inline bool IgnoreDamage(void* game,void* attacker,void* defender,void*){
 if(!Enabled||game!=Game||!defender||!ArenaLevel(defender))return false;
 if(Current.phase!=Phase::Choice&&Current.phase!=Phase::Countdown)return false;
 return Current.phase==Phase::Choice&&Curses::Player(defender)&&Barbarian::Id(defender)==Current.owner;
}
inline void ChildBonuses(void* owner,void* child){
 if(!Enabled||!ArenaLevel(owner)||std::find(Enemies.begin(),Enemies.end(),Barbarian::Id(owner))==Enemies.end())return;
 // HP and damage were inherited from the scaled parent; only apply missing
 // Trial attributes. Helpers grant no additional farmable experience.
 MonsterBonuses(child);Ramp::SetUnit(child,13,0,0);
}
inline void Damage(void* game,void* attacker,void* defender,void* packet){
 if(!Enabled||game!=Game||(Current.phase!=Phase::Fighting&&Current.phase!=Phase::Countdown)||!attacker||!packet)return;
 bool enemy=Curses::Monster(attacker)&&std::find(Enemies.begin(),Enemies.end(),Barbarian::Id(attacker))!=Enemies.end();
 if(!enemy&&Curses::Monster(attacker)&&ArenaLevel(attacker))for(const auto& child:MonsterRuntime::Children)
  if(child.id==MonsterRuntime::Key(attacker)&&std::find(Enemies.begin(),Enemies.end(),MonsterRuntime::Number(child.owner))!=Enemies.end()){enemy=true;break;}
 if(enemy){
  int bonus=Current.damagePercent();if(Ramp::GetUnit(attacker,6,0)<Ramp::GetUnit(attacker,7,0)/2)bonus=int(int64_t(bonus)*(100+Current.bonuses[int(Mod::MonsterRage)])/100);
  AuraFeatures::ScaleResolvedDamage(packet,bonus,100);
  if(Barbarian::ReadDamage(packet,0x134)>0){int hp=Ramp::GetUnit(attacker,6,0),cap=Ramp::GetUnit(attacker,7,0);Ramp::SetUnit(attacker,6,std::min(cap,hp+int(int64_t(cap)*Current.bonuses[int(Mod::MonsterLeech)]/100)),0);}
 }
 unsigned now=Ramp::Frame(game);if(ProcDepth||Curses::ProcDepth||now<NextProc||!Curses::Player(attacker)||Barbarian::Id(attacker)!=Current.owner||!defender||!GuardianCombat::Hostile(game,defender)||!Curses::Cast)return;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);if(!(flags&1)||(flags&0x22)||Barbarian::ReadDamage(packet,0x134)<=0)return;
 for(auto proc:{std::pair<Mod,int>{Mod::ChainReaction,53},{Mod::FireSpark,47},{Mod::FrostPulse,597}}){int chance=std::min(100,Current.bonuses[int(proc.first)]);if(chance&&(Current.random()>>8)%100<unsigned(chance)){
  NextProc=now+25;++ProcDepth;++Curses::ProcDepth;Curses::Cast(attacker,proc.second,std::min(20,3+Current.wave/2),defender,0);--Curses::ProcDepth;--ProcDepth;break;
 }}
}
inline void Text(const PluginContext* c,const Overlay::Frame* f,float x,float y,const std::string& text,Overlay::Color color,float size=20){Overlay::TextRequest t{.structSize=Overlay::TextRequestSize,.canvas=f->canvas,.position={x,y},.color=color,.textSize=size,.text=text.c_str(),.textLength=unsigned(text.size())};OverlayApi->drawText(c,&t);}
// Overlay coordinates and click rectangles share one layout, including DPI scaling.
inline Overlay::Rect ChoiceRect(int i,float width,float height){
 float scale=std::min(width/1600.f,height/900.f),w=1260*scale,x=(width-w)/2,y=(height-560*scale)/2;
 if(i==3)return {width/2-225*scale,y+445*scale,width/2+225*scale,y+505*scale};
 float left=x+i*430*scale;return {left,y+110*scale,left+400*scale,y+405*scale};
}
inline int ChoiceAt(float x,float y,float width,float height){for(int i=0;i<4;++i){auto r=ChoiceRect(i,width,height);if(x>=r.left&&x<=r.right&&y>=r.top&&y<=r.bottom)return i;}return -1;}
inline bool QueueChoice(unsigned epoch,int choice){std::uint64_t empty=0;return Command.compare_exchange_strong(empty,(std::uint64_t(epoch)<<32)|std::uint64_t(choice+1));}
#include "panel.h"
inline void __cdecl Draw(const PluginContext* c,const Overlay::Frame* f,void*) noexcept {
 try{RequestPanelSync(); // Refresh native children even while the modal pauses game frames.
  View view;{std::lock_guard<std::mutex> lock(ViewMutex);view=Display;}
  static bool held=false;static unsigned shownEpoch=0;
  if(!view.visible){held=false;shownEpoch=0;return;}
  const auto& r=view.run;bool choice=r.phase==Phase::Choice;
  float scale=std::min(f->screenWidth/1600.f,f->screenHeight/900.f);
  Overlay::Color gold{.92f,.79f,.43f,1},white{.9f,.9f,.95f,1};
  auto box=[&](Overlay::Rect rect,Overlay::Color color,float rounding){Overlay::FilledRectangleRequest b{.structSize=Overlay::FilledRectangleRequestSize,.canvas=f->canvas,.rect=rect,.color=color,.rounding=rounding};OverlayApi->drawFilledRectangle(c,&b);};
  char line[240]{};
  if(choice)return; // Choice cards are native UI, not overlay drawing.
  if(choice){
   POINT mouse{};GetCursorPos(&mouse);HWND window=GetForegroundWindow();DWORD pid{};GetWindowThreadProcessId(window,&pid);
   bool foreground=pid==GetCurrentProcessId();ScreenToClient(window,&mouse);RECT client{};GetClientRect(window,&client);
   float mx=client.right?mouse.x*f->screenWidth/client.right:0,my=client.bottom?mouse.y*f->screenHeight/client.bottom:0;
   int hover=foreground?ChoiceAt(mx,my,f->screenWidth,f->screenHeight):-1;
   bool down=foreground&&(GetAsyncKeyState(VK_LBUTTON)&0x8000);
   // The killing click must be released before a newly opened card can select.
   if(shownEpoch!=r.epoch){shownEpoch=r.epoch;held=down;}
   if(down&&!held&&hover>=0)QueueChoice(r.epoch,hover);held=down;
   box({0,0,f->screenWidth,f->screenHeight},{.015f,.01f,.03f,.86f},0);
   float x=(f->screenWidth-1260*scale)/2,y=(f->screenHeight-560*scale)/2;
   Text(c,f,x,y,"ASTRAL TRIALS  /  CHOOSE YOUR NEXT MODIFIER",gold,28*scale);
   std::snprintf(line,sizeof(line),"Wave %d cleared  |  Score %d  |  Choose one card to begin wave %d",r.completed,r.score,r.wave+1);Text(c,f,x,y+48*scale,line,white,19*scale);
   constexpr Overlay::Color colors[]={{.9f,.9f,.9f,1},{.45f,.6f,1,1},{1,.95f,.35f,1},{.85f,.65f,.3f,1},{1,.38f,.12f,1}};
   const char* effects[]={"Monster maximum life","Monster damage","Monster movement speed","Your maximum life","Your weapon damage","Your faster cast rate"};
   for(int i=0;i<3;++i){auto o=r.offers[i];auto rect=ChoiceRect(i,f->screenWidth,f->screenHeight);auto color=colors[o.tier];
    box(rect,color,12*scale);box({rect.left+2*scale,rect.top+2*scale,rect.right-2*scale,rect.bottom-2*scale},hover==i?Overlay::Color{.16f,.09f,.23f,1}:Overlay::Color{.065f,.04f,.11f,1},10*scale);
    float tx=rect.left+24*scale,ty=rect.top+25*scale;
    Text(c,f,tx,ty,Tiers[o.tier],color,19*scale);Text(c,f,tx,ty+42*scale,Names[int(o.mod)],gold,27*scale);
    Text(c,f,tx,ty+95*scale,EffectNames[int(o.mod)],white,20*scale);std::snprintf(line,sizeof(line),"+%d%%",o.value);Text(c,f,tx,ty+126*scale,line,white,33*scale);
    std::snprintf(line,sizeof(line),"Reward bonus +%d%%",o.reward);Text(c,f,tx,ty+185*scale,line,gold,19*scale);
    std::snprintf(line,sizeof(line),"CLICK TO CHOOSE");Text(c,f,tx,ty+232*scale,line,color,17*scale);
   }
   auto end=ChoiceRect(3,f->screenWidth,f->screenHeight);box(end,hover==3?Overlay::Color{.25f,.18f,.10f,1}:Overlay::Color{.12f,.09f,.07f,1},8*scale);
   std::snprintf(line,sizeof(line),"FINISH RUN");Text(c,f,end.left+25*scale,end.top+18*scale,line,gold,20*scale);
   Text(c,f,x,y+530*scale,"Modifiers last for this run. Enemy life scales with Trial tier, wave and modifiers.",white,16*scale);
  }else{
   shownEpoch=0;held=false;float x=f->screenWidth*.12f,y=f->screenHeight*.08f;
   box({x,y,f->screenWidth*.88f,y+105*scale},{.045f,.035f,.085f,.94f},8);
   std::snprintf(line,sizeof(line),"ASTRAL TRIAL %d  |  WAVE %d  |  ENEMIES %d  |  SCORE %d",r.stage,r.wave,view.alive,r.score);Text(c,f,x+20*scale,y+12*scale,line,gold,23*scale);
   Text(c,f,x+20*scale,y+46*scale,r.phase==Phase::Countdown?"Next wave in preparation":r.phase==Phase::Fighting?"Defeat every enemy.":"Run ended - use a town portal to leave.",white,18*scale);
   if(!view.note.empty())Text(c,f,x+20*scale,y+78*scale,view.note,white,15*scale);
  }
 }catch(...){}
}
inline int NativeLootBonus(void* game,void* unit){
 if(!Enabled||game!=Game||(Current.phase!=Phase::Fighting&&Current.phase!=Phase::Countdown)||!unit||!ArenaLevel(unit)||std::find(Enemies.begin(),Enemies.end(),Barbarian::Id(unit))==Enemies.end())return 0;
 ObserveDeath(game,unit);return Current.extraLootRolls();
}
inline bool Install(const PluginContext* c){
 MonsterRuntime::ChildSpawned=ChildBonuses;
 Context=c;auto module=GetModuleHandleW(L"D2RCore.dll");Level=reinterpret_cast<Reimplementations::GetLevelIdFromUnitFn>(GetProcAddress(module,MAKEINTRESOURCEA(2000)));
 if(!Level){c->LogError("Arena: level export missing");return false;}
 auto service=[&](auto* ptr,const char* name){auto result=c->QueryService(ptr);if(result!=ServiceQueryResult::Success){c->LogError(name);return false;}return true;};
 if(!service(&OverlayApi,"Arena: Overlay unavailable"))return false;
 if(!SealPortal::Install(c)){c->LogError("Trial seal portal hook guard failed");return false;}
 if(!InstallChoicePanel(c)){c->LogError("Arena: native choice panel registration failed");return false;}
 Overlay::CallbackRegistration draw{.structSize=Overlay::CallbackRegistrationSize,.phase=Overlay::Phase::AfterGameUi,.callback=Draw};
 if(OverlayApi->registerFrameCallback(c,&draw,&OverlayHandle)!=Overlay::Result::Success){c->LogError("Arena: overlay registration failed");return false;}
 const LifecycleService* lifecycle=nullptr;
 if(c->QueryService(&lifecycle)!=ServiceQueryResult::Success||!HasLifecycleServiceField(lifecycle,LifecycleServiceSize)||!lifecycle->registerMonsterDeathListener)return false;
 Lifecycle::MonsterDeathListener deaths{Lifecycle::MonsterDeathListenerSize,0,&OnDeath,nullptr};Lifecycle::ListenerHandle deathHandle=0;
 if(lifecycle->registerMonsterDeathListener(c,&deaths,&deathHandle)!=Lifecycle::Result::Success)return false;
 MonsterRuntime::AdditionalLootRolls=NativeLootBonus;Enabled=true;c->LogInfo("Astral Trials 0.20.0: five arenas, six modifiers/five tiers, authoritative waves and native monster loot enabled");return true;
}
}
