#define main ExistingAuraTests
#include "../combat/aura_test.cpp"
#undef main
#include "runtime.h"
std::map<unsigned,Unit> mobs;unsigned serial=200,moves=0,deletes=0,spawns=0;bool spawnBlocked=false;int movedMode=0,movedX=0,movedY=0;
std::map<unsigned,Unit> portals;unsigned portalSerial=10000,portalCreates=0,removalNotices=0;unsigned notifiedId=0,notifiedType=0;
void* __fastcall GFind(void*,int type,unsigned id){if(type==0&&id==player.id)return &player;if(type==3){auto p=portals.find(id);return p==portals.end()?nullptr:&p->second;}auto it=mobs.find(id);return type==1&&it!=mobs.end()?&it->second:nullptr;}
int __fastcall GX(void* u){return static_cast<Unit*>(u)->x;}int __fastcall GY(void* u){return static_cast<Unit*>(u)->y;}
int __fastcall GLevel(const void* u) noexcept{return static_cast<const Unit*>(u)->room;}
void* __fastcall GRoom(void* u){return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(u)->room));}
void __fastcall GInfo(void*,int mode,void*){movedMode=mode;}
int __fastcall GMove(void*,void* info,int){++moves;std::memcpy(&movedX,static_cast<char*>(info)+0x18,4);std::memcpy(&movedY,static_cast<char*>(info)+0x1c,4);return 1;}
void __fastcall GRoomDelete(void*,unsigned type,unsigned id){++removalNotices;notifiedId=id;notifiedType=type;}
void __fastcall GDelete(void*,void* raw){auto* unit=static_cast<Unit*>(raw);assert(notifiedId==unit->id&&notifiedType==unit->type);notifiedId=0;if(unit->type==3){portals.erase(unit->id);return;}++deletes;mobs.erase(unit->id);}
void* __fastcall GPortal(void*,void* raw){auto& p=*static_cast<Pulse::Params*>(raw);assert(p.missile==Goblins::EscapePortal&&p.range==Goblins::ChannelDuration+10);++portalCreates;auto& u=portals[++portalSerial];u.id=portalSerial;u.type=3;u.cls=p.missile;u.room=static_cast<Unit*>(p.owner)->room;return &u;}
void* __fastcall GSpawn(void*,void* room,int x,int y,int cls,int,int,unsigned short){++spawns;if(spawnBlocked)return nullptr;auto& u=mobs[++serial];u.type=1;u.cls=cls;u.id=serial;u.mode=1;u.room=int(uintptr_t(room));u.x=x;u.y=y;u.stats[6]=u.stats[7]=10000;return &u;}
unsigned PortalCount(){unsigned count=0;for(auto& [id,u]:mobs){auto it=u.lists.find(Goblins::EscapeState);if(it!=u.lists.end()&&it->second.expiry>Ramp::Frame(game)+1)++count;}return count;}
void Tick(unsigned now){::Frame(now);Goblins::Frame(game);}
void Clean(unsigned now=100){mobs.clear();portals.clear();Goblins::TestFamily=0;Goblins::NextTest=0;Goblins::TestPlaced=false;Goblins::Game=game;Goblins::Last=now;Goblins::Next=0;Goblins::Seed=123;Goblins::Visits={};Goblins::Visits.visited[2]=true;Goblins::Runners.clear();Goblins::Splits.clear();Goblins::Deaths.clear();Goblins::TestBloodMoor=false;player.room=2;}
void Notify(unsigned id,int cls,int level=2){D2RL::Lifecycle::MonsterDeathEvent event{};event.structSize=sizeof(event);event.monster={D2RL::Lifecycle::UnitType::Monster,id,unsigned(cls),0};event.levelId=level;Goblins::OnDeath(nullptr,&event,nullptr);}
int main(){
 ExistingAuraTests();player={};player.stats[6]=1000;player.room=2;player.x=20;player.y=20;
 Curses::Find=GFind;Curses::X=GX;Curses::Y=GY;SoulHarvest::Room=GRoom;Arena::Level=GLevel;Arena::Enabled=true;
 Barbarian::RoomDelete=GRoomDelete;Pulse::OriginalCreate=GPortal;Pulse::FilterCreate=nullptr;Pulse::AfterCreate=nullptr;Amazon::PublishMissiles=nullptr;Barbarian::ModeInfo=GInfo;Barbarian::ModeChange=GMove;Barbarian::DeleteUnit=GDelete;MonsterRuntime::Spawn=GSpawn;
 void* ptr=&player;std::memcpy(game+0x2230+8*(player.id&127),&ptr,8);Clean();
 assert(Goblins::Spawn(&player,2,100,0));auto id=serial;auto& goblin=mobs.at(id);
 Tick(105);assert(moves==1&&movedMode==2);assert((movedX-goblin.x)*(goblin.x-player.x)+(movedY-goblin.y)*(goblin.y-player.y)>0);
 Tick(300);assert(movedMode==1&&Goblins::Runners[0].escape.channel);assert(PortalCount()==1);goblin.stats[6]-=100;Tick(305);assert(!Goblins::Runners[0].escape.channel&&movedMode==2);
 assert(PortalCount()==0);Tick(430);assert(Goblins::Runners[0].escape.channel);Tick(505);assert(deletes==1&&Goblins::Runners.empty()&&PortalCount()==0&&removalNotices==1);
 assert(Goblins::Spawn(&player,2,600,0));mobs.at(serial).mode=12;Tick(605);assert(Goblins::Runners.empty()&&deletes==1);
 assert(Goblins::Spawn(&player,2,700,0));id=serial;auto saved=mobs.at(id);mobs.erase(id);Tick(710);assert(Goblins::Runners.size()==1&&deletes==1);
 mobs[id]=saved;mobs[id].cls=1;Tick(715);assert(Goblins::Runners.empty()&&deletes==1);
 spawnBlocked=true;unsigned before=spawns;assert(!Goblins::Spawn(&player,2,800,0));assert(spawns==before+8&&Goblins::Runners.empty());spawnBlocked=false;
 player.room=166;assert(Goblins::Spawn(&player,166,900,0));assert(Goblins::Visits.trialSpawned[166]&&Arena::Enemies.empty());
 Arena::Current.stage=1;Arena::Current.phase=Arena::Phase::Choice;Tick(1000);assert(Goblins::Runners[0].paused&&Goblins::Runners[0].escape.due==1200);
 Tick(2000);assert(Goblins::Runners.size()==1&&Goblins::Runners[0].escape.due==2200);Arena::Current.phase=Arena::Phase::Fighting;Tick(2100);assert(!Goblins::Runners[0].paused&&!Goblins::Runners[0].escape.channel);
 Tick(1);assert(Goblins::Runners.empty()&&!Goblins::Visits.trialSpawned[166]);Clean();
 Goblins::TestBloodMoor=true;Goblins::TestPlaced=false;spawnBlocked=true;before=spawns;
 Tick(1000);assert(Goblins::TestFamily==0&&!Goblins::TestPlaced);spawnBlocked=false;Tick(1025);assert(spawns==before+8);
 for(int family=0;family<7;++family){
  unsigned now=1050+family*100;Tick(now);assert(Goblins::Runners.size()==1&&Goblins::Runners[0].cls==Goblins::Classes[family]);
  before=spawns;Tick(now+5);assert(spawns==before);mobs.at(Goblins::Runners[0].id).mode=12;Tick(now+10);
 }
 assert(Goblins::TestPlaced&&Goblins::TestFamily==7);before=spawns;Tick(1800);assert(spawns==before);
 // Confirmed death after corpse removal, including duplicate delivery, splits exactly once.
 Clean();assert(Goblins::Spawn(&player,2,100,6));id=serial;Notify(id,Goblins::Classes[6]);Notify(id,Goblins::Classes[6]);mobs.erase(id);Tick(105);
 assert(Goblins::Runners.size()==2&&Goblins::Splits.empty());for(auto& r:Goblins::Runners)assert(r.cls==Goblins::Classes[7]);
 auto middle=Goblins::Runners;for(auto& r:middle){Notify(r.id,r.cls);mobs.erase(r.id);}Tick(110);assert(Goblins::Runners.size()==4);for(auto& r:Goblins::Runners)assert(r.cls==Goblins::Classes[8]);
 for(auto& r:Goblins::Runners)mobs.at(r.id).mode=12;before=spawns;Tick(115);assert(Goblins::Runners.empty()&&Goblins::Splits.empty()&&spawns==before);
 // A visible corpse is sufficient even without a lifecycle event. Retry blocked ground.
 Clean();assert(Goblins::Spawn(&player,2,100,6));mobs.at(serial).mode=12;spawnBlocked=true;Tick(105);assert(Goblins::Runners.empty()&&Goblins::Splits.size()==1&&Goblins::Splits[0].remaining==2);
 spawnBlocked=false;player.room=1;Tick(130);assert(Goblins::Runners.empty()&&Goblins::Splits.size()==1);player.room=2;Tick(155);assert(Goblins::Runners.size()==2&&Goblins::Splits.empty());
 // Unloaded Sire is not dead; escaping never creates children.
 Clean();assert(Goblins::Spawn(&player,2,100,6));id=serial;saved=mobs.at(id);mobs.erase(id);before=spawns;Tick(105);assert(Goblins::Runners.size()==1&&spawns==before&&Goblins::Splits.empty());
 mobs[id]=saved;Tick(300);Tick(375);assert(Goblins::Runners.empty()&&Goblins::Splits.empty()&&spawns==before);
 // Unrelated/replayed death cannot manufacture a split; pending work clears with the session.
 Notify(id,Goblins::Classes[6]);Tick(380);assert(Goblins::Splits.empty()&&spawns==before);
 Goblins::Splits.push_back({Goblins::Classes[7],2,20,20,2,400});Tick(1);assert(Goblins::Splits.empty());
 // Death during channel cleans the portal but leaves native death/loot alone.
 Clean();assert(Goblins::Spawn(&player,2,100,0));id=serial;Tick(300);assert(PortalCount()==1);before=deletes;mobs.at(id).mode=12;Tick(305);assert(PortalCount()==0&&deletes==before);
 // Every family sends cleanup even if the corpse was already removed.
 for(int cls:Goblins::Classes){
  Clean();int family=int(std::find(std::begin(Goblins::Classes),std::end(Goblins::Classes),cls)-std::begin(Goblins::Classes));
  if(family>=Goblins::NaturalCount)continue;
  assert(Goblins::Spawn(&player,2,100,family));id=serial;Tick(300);assert(PortalCount()==1);
  Notify(id,cls);Notify(id,cls);mobs.erase(id);Tick(305);assert(PortalCount()==0);
 }
 // Unrelated deaths never clean up an active goblin's portal.
 Clean();assert(Goblins::Spawn(&player,2,100,0));Tick(300);Notify(serial,19);Tick(305);assert(PortalCount()==1);assert(Goblins::Deaths.empty());
 // A trial choice removes the channel visual and recreates it on resume.
 Clean();player.room=166;assert(Goblins::Spawn(&player,166,100,0));Arena::Current.stage=1;Arena::Current.phase=Arena::Phase::Fighting;Tick(300);assert(PortalCount()==1);
 Arena::Current.phase=Arena::Phase::Choice;Tick(305);assert(PortalCount()==0);Tick(400);Arena::Current.phase=Arena::Phase::Fighting;Tick(405);assert(PortalCount()==1);Tick(475);assert(PortalCount()==0&&Goblins::Runners.empty());
 // Goblin wave overrides milestones, tracks splits and can finish by escape.
 Clean();::Frame(100);Arena::Game=game;Arena::Current.start(1,player.id,77);Arena::Current.wave=10;Arena::Current.bonuses[int(Arena::Mod::LootGoblinWave)]=1;
 assert(!Arena::BossWave(Arena::Current)&&Arena::WaveCount(Arena::Current)==Arena::Current.count());
 player.room=166;Arena::Pending=2;Arena::Enemies.clear();Goblins::SpawnTrialWave(&player,100);
 assert(Arena::Pending==0&&Arena::Current.phase==Arena::Phase::Fighting&&Arena::Enemies.size()==2&&Goblins::TrialWaveBusy());
 for(auto& r:Goblins::Runners)assert(r.trialWave&&Goblins::GoblinClass(r.cls));
 Tick(300);Tick(375);assert(!Goblins::TrialWaveBusy());
 Clean();::Frame(100);player.room=166;Arena::Current.phase=Arena::Phase::Fighting;
 assert(Goblins::Spawn(&player,166,100,6,true));id=serial;Notify(id,Goblins::Classes[6],166);mobs.erase(id);Tick(105);
 assert(Goblins::Runners.size()==2&&Goblins::TrialWaveBusy());for(auto& r:Goblins::Runners)assert(r.trialWave&&r.cls==Goblins::Classes[7]);
 Arena::Current.phase=Arena::Phase::Finished;Tick(110);assert(!Goblins::TrialWaveBusy());
 // Unloaded event goblins eventually escape; their old units are retired if reloaded.
 Clean();::Frame(100);player.room=166;Arena::Current.phase=Arena::Phase::Fighting;
 assert(Goblins::Spawn(&player,166,100,0,true));mobs.erase(serial);Tick(505);assert(!Goblins::TrialWaveBusy()&&!Arena::RetiredEnemies.empty());
 Arena::Current={};
 std::puts("PASS: seven test spawns, 1->2->4 split chain, terminal death, duplicate/removed corpse events, blocked placement retries, unload/escape guards, trial pause and session reset.");
}
