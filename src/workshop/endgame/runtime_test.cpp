#define main ExistingAuraTests
#include "../combat/aura_test.cpp"
#undef main
#include <memory>
std::vector<std::unique_ptr<Unit>> mobs;
unsigned char spawnedRankData[128]{};
int created=0,deleted=0,portalsDeleted=0,arenaCasts=0;bool full=false,blockSpawn=false;
D2RL::PluginContext ctx{};
int __fastcall AX(void* p){return static_cast<Unit*>(p)->x;}
int __fastcall AY(void* p){return static_cast<Unit*>(p)->y;}
void* __fastcall ARoom(void* p){return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(p)->room));}
int __fastcall ALevel(const void* p) noexcept{return static_cast<const Unit*>(p)->room;}
void* __fastcall AFind(void*,int type,unsigned id){if(!type&&id==player.id)return &player;for(auto& p:mobs)if(p&&p->id==id)return p.get();return nullptr;}
void __fastcall ATarget(void*,void*){}
void __fastcall ADelete(void*,void* p){if(static_cast<Unit*>(p)->type==2){++portalsDeleted;return;}for(auto& u:mobs)if(u.get()==p){u.reset();++deleted;}}
void __fastcall ARoomDelete(void*,unsigned,unsigned){}
void* __fastcall ASpawn(void*,void*,int x,int y,int cls,int,int,unsigned short){if(blockSpawn)return nullptr;auto p=std::make_unique<Unit>();p->type=1;p->id=100+unsigned(mobs.size());p->cls=cls;p->room=player.room;p->x=x;p->y=y;p->stats[6]=p->stats[7]=1000;void* rankData=spawnedRankData;std::memcpy(p->native,&rankData,8);auto raw=p.get();mobs.push_back(std::move(p));return raw;}
D2RL::Inventory::Result __cdecl ALocal(const D2RL::PluginContext*,D2RL::PlayerHandle* p) noexcept{*p=10;return D2RL::Inventory::Result::Success;}
D2RL::Items::Result __cdecl AItem(const D2RL::PluginContext*,D2RL::PlayerHandle,const D2RL::Items::ItemCreateSpec* spec,D2RL::ItemHandle* item) noexcept{if(full)return D2RL::Items::Result::InvalidArgument;assert(spec->quality==D2RL::Items::Quality::Normal);assert(spec->destination.container==D2RL::Items::ItemContainer::Ground);assert(spec->destination.placement==D2RL::Items::Placement::Exact);assert(spec->destination.x==unsigned(player.x)&&spec->destination.y==unsigned(player.y));++created;*item=created;return D2RL::Items::Result::Success;}
Unit craftedSeal;unsigned char sealData[0x80]{};int portalResult=1;
void* __fastcall SealInv(void*){return &craftedSeal;}
void* __fastcall SealFirst(void* inv){return inv;}
void* __fastcall SealNext(void*){return nullptr;}
unsigned __fastcall SealCode(void*){return D2RL::Items::MakeItemCode("Wa1");}
int __fastcall SealOpen(void*,void*,void*,int,int,int,void**,int,int){return portalResult;}
void PortalCraftTests(){
 using namespace Arena::SealPortal;Inventory=SealInv;First=SealFirst;Next=SealNext;Code=SealCode;Original=SealOpen;
 craftedSeal={};craftedSeal.type=4;void* data=sealData;std::memcpy(craftedSeal.native,&data,8);sealData[0x55]=3;
 TrialCraft::Seal seal;seal.fields={23,5,11,61,0,0,0,0};assert(seal.valid());
 for(int i=0;i<8;++i)craftedSeal.stats[444+i]=seal.fields[i];
 player={};Frame(200);Pending={};portalResult=0;
 Open(game,&player,nullptr,0,0,Arena::FirstLevel,nullptr,60,1);assert(!Pending[0].ready);
 portalResult=1;sealData[0x55]=0;Open(game,&player,nullptr,0,0,Arena::FirstLevel,nullptr,60,1);assert(!Pending[0].ready);
 sealData[0x55]=3;Open(game,&player,nullptr,0,0,Arena::FirstLevel,nullptr,60,1);assert(Pending[0].ready);
 Arena::Run run;run.start(1,player.id,1);Apply(game,201,run);assert(run.bonuses[0]==150&&run.bonuses[1]==6&&run.bonuses[16]==10);assert(Arena::SealChampion==5);assert(run.rewardBonus==180);assert(!Pending[0].ready);
 run.start(1,player.id,1);Apply(game,201,run);assert(run.rewardBonus==0&&Arena::SealChampion==0);
 puts("PASS: crafted seal portal snapshot only from Cube, success-only transfer, exact tier modifiers, rank chance and one-time consumption.");
}
int main(){
 static_assert(!Arena::LowLifePlaytest,"Trials must use native monster life in production");
 ExistingAuraTests();PortalCraftTests();Arena::NaturalRanks=false;
 for(auto size: {std::pair<float,float>{1280,720},{1920,1080},{3840,2160}}){for(int i=0;i<4;++i){auto r=Arena::ChoiceRect(i,size.first,size.second);assert(Arena::ChoiceAt((r.left+r.right)/2,(r.top+r.bottom)/2,size.first,size.second)==i);}assert(Arena::ChoiceAt(0,0,size.first,size.second)==-1);}
 assert(Arena::QueueChoice(5,2));assert(!Arena::QueueChoice(5,0));assert(Arena::Command.exchange(0)==((uint64_t(5)<<32)|3));
 player={};player.x=100;player.y=120;player.room=Arena::FirstLevel;player.stats[6]=player.stats[7]=10000;game[0x104]=2;
 Curses::X=AX;Curses::Y=AY;Curses::Find=AFind;SoulHarvest::Room=ARoom;Assassin::SetTarget=ATarget;MonsterRuntime::Spawn=ASpawn;Barbarian::RoomDelete=ARoomDelete;Barbarian::DeleteUnit=ADelete;
 Arena::Level=ALevel;
 Unit portal{};portal.type=2;portal.cls=60;portal.room=Arena::FirstLevel;assert(Arena::IsArenaReturnPortal(&portal));portal.room=109;assert(!Arena::IsArenaReturnPortal(&portal));portal.room=Arena::FirstLevel;portal.cls=59;assert(!Arena::IsArenaReturnPortal(&portal));
 Unit townPortal{};townPortal.type=2;townPortal.cls=60;townPortal.room=109;
 Unit redPortal{};redPortal.type=2;redPortal.cls=60;redPortal.room=Arena::FirstLevel;
 void* head=&redPortal;void* next=&townPortal;std::memcpy(game+0x2a30,&head,8);std::memcpy(reinterpret_cast<char*>(&redPortal)+0x158,&next,8);
 next=&portal;std::memcpy(reinterpret_cast<char*>(&townPortal)+0x158,&next,8);
 Arena::Game=nullptr;

Arena::Enabled=true;Arena::Context=&ctx;void* p=&player;std::memcpy(game+0x2230+8*(player.id&127),&p,8);

 player.room=109;Frame(900);Arena::Frame(game);assert(portalsDeleted==0&&Arena::Current.phase==Arena::Phase::Idle);
 player.room=Arena::FirstLevel;Frame(1000);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Countdown&&mobs.empty());
 assert(portalsDeleted==0);
 Frame(1020);Arena::Frame(game);assert(portalsDeleted==0);
 // A preloaded second trial must retain its destination portal too.
 Unit otherPortal{};otherPortal.type=2;otherPortal.cls=60;otherPortal.room=Arena::FirstLevel+1;
 next=&otherPortal;std::memcpy(reinterpret_cast<char*>(&portal)+0x158,&next,8);
 Frame(1025);Arena::Frame(game);assert(portalsDeleted==1);
 std::memset(game+0x2a30,0,8);
 Frame(1070);Arena::Frame(game);assert(mobs.empty());
 for(int f=1075;f<=1100;f+=5){Frame(f);Arena::Frame(game);}assert(Arena::Current.phase==Arena::Phase::Fighting&&Arena::Enemies.size()==14);
 Arena::Run hpRun;hpRun.start(1,1,1);assert(Arena::PlaytestLife(hpRun)==2560);hpRun.bonuses[0]=20;assert(Arena::PlaytestLife(hpRun)==3072);hpRun.wave=2;assert(Arena::PlaytestLife(hpRun)==3440);
 for(const auto& u:mobs)if(u){int dx=u->x-player.x,dy=u->y-player.y;double distance=std::sqrt(double(dx*dx+dy*dy));assert(distance>=54&&distance<=89);}
 assert(mobs[0]->stats[7]==1200&&mobs[0]->stats[6]==1200);alignas(8) char packet[0x180]{};int damage=100;std::memcpy(packet+0x18,&damage,4);std::memcpy(packet+0x134,&damage,4);Arena::Damage(game,mobs[0].get(),&player,packet);assert(Barbarian::ReadDamage(packet,0x134)==120);

 MonsterRuntime::AdditionalLootRolls=Arena::NativeLootBonus;
 MonsterRuntime::OriginalDrop=[](void*,void*,void*,unsigned short,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t)->std::uintptr_t{++created;return 77;};
 Arena::Current.rewardBonus=400;
 assert(Arena::NativeLootBonus(game,mobs[0].get())==4);assert(Arena::NativeLootBonus(game,&player)==0);
 MonsterRuntime::Drop(game,mobs[0].get(),&player,1,0,0,0,0,0,0);assert(created==5);
 MonsterRuntime::Drop(game,mobs[0].get(),&player,1,0,0,0,1,0,0);assert(created==6);
 Arena::ApplyBuff(&player);assert(GetUnit(&player,80,0)==400);
 Arena::Current.rewardBonus=0;created=0;
 Curses::Owner=[](void*,void*)->void*{return nullptr;};
 Curses::Cast=[](void*,int skill,int level,void*,int)->int{assert(skill==53&&level>=3&&level<=20&&Curses::ProcDepth==1);++arenaCasts;return 1;};
 Arena::Current.bonuses[int(Arena::Mod::ChainReaction)]=100;Arena::NextProc=0;unsigned short landed=1;std::memcpy(packet+4,&landed,2);
 Arena::Damage(game,&player,mobs[0].get(),packet);assert(arenaCasts==1);for(int i=0;i<100;++i)Arena::Damage(game,&player,mobs[0].get(),packet);assert(arenaCasts==1);
 ++Curses::ProcDepth;Arena::NextProc=0;Arena::Damage(game,&player,mobs[0].get(),packet);--Curses::ProcDepth;assert(arenaCasts==1);
 Frame(1125);for(int i=0;i<100;++i)Arena::Damage(game,&player,mobs[0].get(),packet);assert(arenaCasts==2);Frame(1100);Arena::Current.bonuses[int(Arena::Mod::ChainReaction)]=0;
 Unit armored;armored.type=1;armored.stats[31]=100;armored.stats[39]=90;Arena::Current.bonuses[int(Arena::Mod::MonsterArmor)]=20;Arena::Current.bonuses[int(Arena::Mod::MonsterResist)]=40;Arena::MonsterBonuses(&armored);assert(armored.stats[31]==120&&armored.stats[39]==100);Arena::Current.bonuses[int(Arena::Mod::MonsterArmor)]=Arena::Current.bonuses[int(Arena::Mod::MonsterResist)]=0;
 // A dead unit removed before the next scan is captured via post-damage or loot.
 unsigned postId=mobs[1]->id,dropId=mobs[2]->id;
 Arena::ObserveDeath(game,mobs[1].get());assert(Arena::DeathQueue.empty()); // living must not count
 mobs[1]->stats[6]=0;Arena::AfterDamage(game,&player,mobs[1].get(),packet);mobs[1].reset();
 mobs[2]->stats[6]=0;Arena::NativeLootBonus(game,mobs[2].get());mobs[2].reset();
 for(auto& u:mobs)if(u)u->stats[6]=0;
 unsigned vanished=mobs[0]->id;mobs[0].reset();Frame(1105);Arena::Frame(game);
 assert(Arena::Current.phase==Arena::Phase::Fighting); // absence alone is not a kill
 SoulHarvest::Session=nullptr;D2RL::Lifecycle::MonsterDeathEvent death{};death.structSize=sizeof(death);death.levelId=-1;death.monster={D2RL::Lifecycle::UnitType::Monster,vanished,unsigned(Arena::MonsterClasses[0]),0};
 Arena::OnDeath(&ctx,&death,nullptr);Arena::OnDeath(&ctx,&death,nullptr); // duplicate callbacks remain one kill
 Frame(1110);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Choice&&deleted==11&&Arena::Current.completed==1);
 assert(!Arena::Display.choiceReady);Frame(1130);Arena::Frame(game);assert(!Arena::Display.choiceReady);Frame(1135);Arena::Frame(game);assert(!Arena::Display.choiceReady);Frame(1185);Arena::Frame(game);assert(Arena::Display.choiceReady);
 assert(Arena::IgnoreDamage(game,nullptr,&player,nullptr));auto epoch=Arena::Current.epoch;
 Arena::Command=(uint64_t(epoch-1)<<32)|1;Frame(1190);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Choice);
 Arena::Current.bonuses[3]=20;Arena::Current.bonuses[4]=30;Arena::Current.bonuses[5]=10;Arena::ApplyBuff(&player);assert(GetUnit(&player,76,0)==20&&GetUnit(&player,105,0)==10);
 Arena::Command=(uint64_t(epoch)<<32)|4;Frame(1195);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Finished);assert(GetUnit(&player,76,0)==0&&GetUnit(&player,105,0)==0&&GetUnit(&player,80,0)==0);
 Frame(1200);Arena::Frame(game);assert(created==0);
 player.room=109;Frame(1230);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Idle);
 player.room=Arena::FirstLevel;Frame(1235);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Idle); // old portal cannot farm again
 player.room=Arena::FirstLevel+1;Frame(1240);Arena::Frame(game);assert(Arena::Current.stage==2&&Arena::Current.phase==Arena::Phase::Countdown);
 player.stats[6]=0;Frame(1245);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Failed&&Arena::Enemies.empty());
 Arena::Run scales;for(int i=1;i<=5;++i){scales.stage=i;const double expected[]={1.2,1.5,1.9,2.4,3.0};assert(scales.stageScale()==expected[i-1]);}
 Arena::Run reward;reward.start(1,1,1);assert(reward.lootBonus()==0);reward.wave=5;reward.rewardBonus=8;assert(reward.lootBonus()==48);reward.wave=100;assert(reward.lootBonus()==998);

 assert(reward.reward(Arena::Mod::MonsterPhysical,0)==10 && reward.reward(Arena::Mod::MonsterPhysical,4)==160);
 alignas(8) unsigned char ranked[0x180]{},rankData[0x80]{};*reinterpret_cast<unsigned*>(ranked)=1;void* rankPtr=rankData;std::memcpy(ranked+0x10,&rankPtr,8);
 MonsterRuntime::Init=[](void*,int id,int unique){assert(unique==1);assert(id==1||id==2||id==4||id==16||id==5);};
 MonsterRuntime::OriginalRoll=[](void*,void*,unsigned char*,void* weights)->int{assert(weights);return 5;};
 Arena::Current.bonuses[int(Arena::Mod::ChampionWave)]=1;assert(Arena::RankWaveMonster(ranked));assert(MonsterModifiersNative::Rank(ranked)==12&&rankData[0x20]==16&&rankData[0x21]==0);
 Arena::Current.bonuses[int(Arena::Mod::ChampionWave)]=0;Arena::Current.bonuses[int(Arena::Mod::EliteWave)]=1;assert(Arena::RankWaveMonster(ranked));assert(MonsterModifiersNative::Rank(ranked)==8&&rankData[0x20]==5&&rankData[0x21]==0);Arena::Current.bonuses[int(Arena::Mod::EliteWave)]=0;
 Arena::Run once;once.start(1,1,1);once.phase=Arena::Phase::Choice;once.offers[0]={Arena::Mod::ChampionWave,3,1,80};assert(once.choose(0,once.epoch));assert(once.bonuses[int(Arena::Mod::ChampionWave)]==1);once.phase=Arena::Phase::Fighting;assert(once.clear());assert(once.bonuses[int(Arena::Mod::ChampionWave)]==0);
 Arena::Run rolling;rolling.start(1,1,12345);int positiveScreens=0;
 for(int n=0;n<100000;++n){rolling.roll();int positive=0;for(int i=0;i<3;++i){positive+=Arena::Positive[int(rolling.offers[i].mod)];for(int j=0;j<i;++j)assert(rolling.offers[j].mod!=rolling.offers[i].mod);}for(auto o:rolling.offers){if(o.mod==Arena::Mod::ChampionWave)assert(o.tier==3&&o.value==1);if(o.mod==Arena::Mod::EliteWave)assert(o.tier==4&&o.value==1);}assert(positive<=1);positiveScreens+=positive;}
 // 20% ordinary positive slots plus a 1% goblin event when no positive slot exists.
 assert(positiveScreens>19800&&positiveScreens<21800);
 rolling.bonuses[int(Arena::Mod::ChainReaction)]=100;for(int n=0;n<1000;++n){rolling.roll();for(auto o:rolling.offers)assert(o.mod!=Arena::Mod::ChainReaction);}
 Arena::Run actions;actions.start(1,1,55);actions.phase=Arena::Phase::Choice;actions.roll();
 auto oldEpoch=actions.epoch;assert(actions.reroll(false,oldEpoch));assert(actions.rerolls==2&&actions.banishes==3&&actions.wave==1&&actions.rewardBonus==0);assert(!actions.reroll(false,oldEpoch));
 for(int j=0;j<3;++j){auto old=actions.offers;assert(actions.reroll(true,actions.epoch));for(auto o:old)assert(actions.banned[int(o.mod)]);for(int n=0;n<1000;++n){actions.roll();for(auto o:actions.offers){assert(!actions.banned[int(o.mod)]);assert(o.mod!=Arena::Mod::PlayerRecovery);}}}
 assert(actions.banishes==0&&!actions.reroll(true,actions.epoch));assert(actions.reroll(false,actions.epoch));assert(actions.reroll(false,actions.epoch));assert(!actions.reroll(false,actions.epoch));
 actions.start(1,1,5);assert(actions.rerolls==3&&actions.banishes==3);for(bool b:actions.banned)assert(!b);
 assert(actions.value(Arena::Mod::PlayerLife,4)==20&&actions.value(Arena::Mod::PlayerStats,4)==20&&actions.value(Arena::Mod::MonsterMagic,4)==300);
 for(int n=0;n<100;++n){Arena::StartWave(1000);assert(Arena::WaveClasses[0]!=Arena::WaveClasses[1]&&Arena::WaveClasses[1]!=Arena::WaveClasses[2]&&Arena::WaveClasses[0]!=Arena::WaveClasses[2]);}
 // Lost-unit recovery waits five seconds and requires a real replacement kill.
 player.stats[6]=10000;player.room=Arena::FirstLevel;Arena::Current.start(1,player.id,99);Arena::Current.phase=Arena::Phase::Fighting;
 Arena::Enemies={999999};Arena::ConfirmedDead.clear();Arena::SpawnRecords={{999999,Arena::MonsterClasses[0],0}};Arena::Next=0;
 Frame(2000);Arena::Frame(game);assert(Arena::Enemies[0]==999999);
 Frame(2120);Arena::Frame(game);assert(Arena::Enemies[0]==999999&&Arena::Current.completed==0);
 Frame(2125);Arena::Frame(game);assert(Arena::Enemies[0]!=999999&&Arena::Current.phase==Arena::Phase::Fighting&&Arena::Current.completed==0);
 auto replacement=AFind(game,1,Arena::Enemies[0]);assert(replacement&&GetUnit(replacement,6,0)==1200);
 Frame(2130);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Fighting);
 SetUnit(replacement,6,0,0);Frame(2135);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Choice&&Arena::Current.completed==1);
 std::puts("PASS: missing enemy recovery waits, replaces without credit, then unlocks cards only after replacement death.");
 // Spawn obstruction must never delete a partially created wave or end the run.
 Arena::StartWave(3000);blockSpawn=true;Frame(3500);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Countdown&&Arena::Pending>0);
 blockSpawn=false;for(int f=3505;f<3600&&Arena::Pending;f+=5){Frame(f);Arena::Frame(game);}assert(Arena::Current.phase==Arena::Phase::Fighting&&!Arena::Enemies.empty());
 Arena::Current.phase=Arena::Phase::Countdown;assert(!Arena::IgnoreDamage(game,&player,mobs.back().get(),nullptr));assert(!Arena::IgnoreDamage(game,mobs.back().get(),&player,nullptr));
 mobs.back()->stats[6]=0;Arena::ObserveDeath(game,mobs.back().get());assert(!Arena::DeathQueue.empty());
 Unit ground{};ground.type=4;ground.mode=3;ground.room=Arena::FirstLevel;assert(Arena::IsTrialGroundLoot(&ground));ground.mode=0;assert(!Arena::IsTrialGroundLoot(&ground));ground.mode=3;ground.room=109;assert(!Arena::IsTrialGroundLoot(&ground));
 std::puts("PASS: blocked placement retries, countdown enemies vulnerable, early deaths recorded, ground cleanup excludes inventory and other levels.");
 // Exercise cleanup on an actual isolated item table, including other-level and inventory items.
 alignas(8) unsigned char itemGame[0x4000]{};auto savedGame=Arena::Game;Arena::Game=itemGame;Arena::Current.phase=Arena::Phase::Choice;
 Unit* itemUnits[3]{};
 for(int i=0;i<3;++i){auto item=std::make_unique<Unit>();item->type=4;item->id=9000+i;item->mode=i==1?0:3;item->room=i==2?109:Arena::FirstLevel;itemUnits[i]=item.get();mobs.push_back(std::move(item));}
 void* firstItem=itemUnits[0];std::memcpy(itemGame+0x2e30,&firstItem,8);
 for(int i=0;i<2;++i){void* following=itemUnits[i+1];std::memcpy(reinterpret_cast<char*>(itemUnits[i])+0x158,&following,8);}
 Unit missile{};missile.type=3;missile.mode=3;missile.room=Arena::FirstLevel;void* missileHead=&missile;std::memcpy(itemGame+0x3230,&missileHead,8);
 Arena::ClearGroundLoot();assert(missile.type==3);assert(!AFind(itemGame,4,9000)&&AFind(itemGame,4,9001)&&AFind(itemGame,4,9002));Arena::Game=savedGame;
 Arena::Current.bonuses[int(Arena::Mod::ChampionWave)]=Arena::Current.bonuses[int(Arena::Mod::EliteWave)]=0;
 assert(Arena::RankWaveMonster(ranked,1)&&rankData[0x20]==16);assert(Arena::RankWaveMonster(ranked,2)&&rankData[0x20]==5&&rankData[0x21]==0);
 std::puts("PASS: cleanup removes only local ground items; natural champion/elite ranking uses one native modifier.");
 // Milestone extras are additive; exact escort ranks survive event cards.
 for(int stage=1;stage<=5;++stage)for(int wave:{9,10,11,20}){
  Arena::Current.start(stage,player.id,91);Arena::Current.wave=wave;
  int ranks[4]{};for(int index=0;index<Arena::WaveCount(Arena::Current);++index)++ranks[Arena::MilestoneRank(Arena::Current,index)];
  assert(ranks[0]==Arena::Current.count());bool milestone=wave%10==0;
  assert(ranks[1]==(milestone?5:0)&&ranks[2]==(milestone?2:0)&&ranks[3]==(milestone?1:0));
  assert(Arena::TrackedClass(Arena::BossClasses[stage-1]));
 }
 Arena::Current.start(1,player.id,91);Arena::Current.wave=10;player.room=Arena::FirstLevel;
 Arena::Current.bonuses[int(Arena::Mod::ChampionWave)]=1;
 assert(Arena::RankWaveMonster(ranked,2)&&rankData[0x20]==5); // guaranteed elites stay elites
 Arena::StartWave(5000);for(unsigned now=5075;Arena::Pending&&now<5300;now+=5)Arena::SpawnWave(&player,now);
 assert(!Arena::Pending&&Arena::Enemies.size()==unsigned(Arena::Current.count()+8));
 auto boss=std::find_if(Arena::SpawnRecords.begin(),Arena::SpawnRecords.end(),[](auto r){return r.rank==3;});assert(boss!=Arena::SpawnRecords.end()&&boss->cls==Arena::BossClasses[0]);
 unsigned bossId=boss->id;
 for(unsigned id:Arena::Enemies)if(id!=bossId)SetUnit(AFind(game,1,id),6,0,0);
 Arena::Next=0;Frame(5310);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Fighting);
 // Even if the boss corpse disappears before scanning, its lifecycle death completes the wave.
 death={};Arena::OnDeath(&ctx,&death,nullptr); // undersized events ignored
 death.structSize=sizeof(death);death.monster={D2RL::Lifecycle::UnitType::Monster,bossId,unsigned(Arena::BossClasses[0]),0};
 SetUnit(AFind(game,1,bossId),6,0,0);Arena::OnDeath(&ctx,&death,nullptr);ADelete(game,AFind(game,1,bossId));
 Frame(5315);Arena::Frame(game);assert(Arena::Current.phase==Arena::Phase::Choice&&Arena::Current.completed==10);
 std::puts("PASS: wave 10/20 add one act boss, five champions and two elites; boss death is required and recorded after corpse removal.");
 std::printf("PASS: 100000 selections, %d positive screens; at most one positive; distinct enemies, proc cooldown and recursion guard.\n",positiveScreens);
 std::puts("PASS: arena countdown, authoritative spawning/clear, native monster HP with Trial life scaling, red-return-portal filtering, tier damage, stale choice, pause protection, temporary buff cleanup, native loot scaling, output-buffer safety, loot-MF cleanup, no Finish item creation, used portal and death cleanup.");
}
