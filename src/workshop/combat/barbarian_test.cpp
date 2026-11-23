#define main BaseAuraTests
#include "aura_test.cpp"
#undef main
#include <set>
Unit target,ally,ancient,tickMissile;std::array<Unit,6> bodies;
bool ancientDeleted=false,deleteQueued=false;
unsigned rockCreates=0,rainCasts=0,petAiCalls=0,pathSteps=0;
unsigned tickCount=0,unsummons=0,redeems=0,ancientCasts=0;int lastTick=0;
std::set<unsigned> consumed;
int __fastcall BX(void* u){return static_cast<Unit*>(u)->x;}
int __fastcall BY(void* u){return static_cast<Unit*>(u)->y;}
void* __fastcall BRoom(void* u){return reinterpret_cast<void*>(std::uintptr_t(static_cast<Unit*>(u)->room));}
void* __fastcall BMinion(void* u){return u==&ancient?&player:nullptr;}
void* __fastcall BNoOwner(void*,void*){return nullptr;}
void* __fastcall BFind(void*,int type,unsigned id){
 if(type==0){if(id==player.id)return &player;if(id==ally.id)return &ally;}
 if(type==1){if(id==target.id)return &target;if(id==ancient.id&&!ancientDeleted)return &ancient;for(auto& b:bodies)if(id==b.id)return &b;}
 return nullptr;
}
int BLevel(void*,void* u,int skill){return static_cast<Unit*>(u)->levels[skill];}
void Place(Unit& u,unsigned cls,unsigned id){u.type=1;u.cls=cls;u.id=id;u.stats[6]=1000*256;u.stats[7]=1000*256;void* p=&u;std::memcpy(game+0x2630+8*(id&127),&p,8);}
void* __fastcall BCreate(void*,void* raw){auto& p=*static_cast<Pulse::Params*>(raw);if(p.missile==Barbarian::RockControl){assert(p.skill==Barbarian::RainSkill&&p.range>0);++rockCreates;return &tickMissile;}if(p.missile==Barbarian::RockHit){assert(p.skill==140&&p.range==1);tickMissile.stats[21]=500;tickMissile.stats[22]=1000;tickMissile.stats[48]=250;tickMissile.stats[56]=75;lastTick=100;return &tickMissile;}assert(p.skill==Barbarian::TickSkill&&p.missile==Barbarian::TickMissile&&p.range==1);lastTick=GetUnit(p.owner,AuraBuffs::ChargeStat,0);return &tickMissile;}
int __fastcall BMissile(void*,void*,void* u,int){if(Barbarian::StompDispatch)assert(Curses::ProcDepth==0);++tickCount;static_cast<Unit*>(u)->stats[6]-=lastTick*256;return 1;}
int __fastcall BCast(void*,int skill,int level,void* target,int){if(skill==Barbarian::RainSkill){assert(level>=1&&target);++rainCasts;return 1;}if(skill==3){++unsummons;static_cast<Unit*>(target)->stats[6]=0;return 1;}for(int s:Barbarian::AncientAttacks)if(skill==s){++ancientCasts;return 1;}assert(skill==Barbarian::PullSkill);return 1;}
int __fastcall BRedeem(void*,void*,void* u,int skill,int,int skip){assert(skill==131&&skip==1);if(!consumed.insert(static_cast<Unit*>(u)->id).second)return 0;++redeems;static_cast<Unit*>(u)->mode=0;return 1;}
void __fastcall BRemove(void*,void* owner,unsigned id,int kill){assert(owner==&player&&kill==0&&id==ancient.id);}
void __fastcall BRoomDelete(void* room,unsigned type,unsigned id){assert(room==BRoom(&ancient)&&type==1&&id==ancient.id);deleteQueued=true;}
void __fastcall BDelete(void*,void* unit){assert(unit==&ancient&&deleteQueued&&!ancientDeleted);++unsummons;ancientDeleted=true;ancient.stats[6]=0;}
void __fastcall BModeInfo(void* unit,int mode,void* raw){void* skills=nullptr;std::memcpy(&skills,static_cast<unsigned char*>(unit)+0x100,8);if(skills){void* empty=nullptr;std::memcpy(static_cast<unsigned char*>(skills)+0x18,&empty,8);}std::memcpy(static_cast<unsigned char*>(raw)+8,&unit,8);std::memcpy(raw,&mode,4);}
int __fastcall BModeChange(void*,void* raw,int){void* unit=nullptr;std::memcpy(&unit,static_cast<unsigned char*>(raw)+8,8);unsigned mode=0;std::memcpy(&mode,raw,4);if(mode==14&&GuardianCombat::BindNativeSkills&&Barbarian::UsedSkill(unit)!=Barbarian::AncientAttacks[0])return 0;static_cast<Unit*>(unit)->mode=mode;if(mode!=1)++ancientCasts;return 1;}
void __fastcall BPetAi(void*,void*,void*){++petAiCalls;}
void* __fastcall BPath(void* u){return u;}
void __fastcall BStep(void* path,unsigned char step){assert(path==&ancient&&step==1);++pathSteps;}
int __fastcall BMissileTick(void*,void*){assert(Curses::ProcDepth>0);return 1;}
int __fastcall BWarcry(void*,void*,int skill,int){assert(skill!=131);if(skill==Barbarian::RainSkill){++rainCasts;return 1;}return 789;}
alignas(8) unsigned char privateExtra[0x200]{};
bool rejectBuff=false;
void* __fastcall BExtra(unsigned char,int cls){assert(cls>=int(Barbarian::AncientClasses[0])&&cls<=int(Barbarian::AncientClasses[2]));return privateExtra;}
void* __fastcall BAuraApply(void* raw);
void* __fastcall BEligibleApply(void* raw){auto& request=*static_cast<Curses::Curse*>(raw);assert((privateExtra[5]&2)!=0);assert(privateExtra[4]==0x20);return rejectBuff?nullptr:BAuraApply(raw);}
int __fastcall BAuraArea(const void* raw) noexcept{return static_cast<const Unit*>(raw)->room/100+1;}
int __fastcall BFormula(unsigned char version,void* u,int formula,int skill,int level){return formula==1901?12:formula==1902?20:Evaluate(version,u,formula,skill,level);}
int __fastcall BAuraOwnerType(void*){return 0;}
int __fastcall BAuraOwnerId(void* raw){return static_cast<Unit*>(static_cast<List*>(raw)->owner)->id;}
void* __fastcall BAuraApply(void* raw){auto* list=static_cast<List*>(Apply(raw));list->owner=static_cast<Curses::Curse*>(raw)->caster;return list;}
alignas(8) unsigned char packet[0x180]{};
void Damage(int physical=100){std::memset(packet,0,sizeof(packet));unsigned short flags=1;int fixed=physical*256;std::memcpy(packet+4,&flags,2);std::memcpy(packet+0x18,&fixed,4);std::memcpy(packet+0x134,&fixed,4);}
void Fresh(unsigned frame){Frame(frame);Barbarian::Session=nullptr;Barbarian::Reset(game);target.stats[6]=target.stats[7]=1000*256;target.stats[36]=0;player.levels.clear();player.lists.clear();std::memset(player.native,0,sizeof(player.native));}
int main(){
 BaseAuraTests();player.cls=4;player.stats[6]=1000*256;player.stats[7]=1000*256;
 ally.type=0;ally.cls=3;ally.id=99;ally.stats[6]=1000*256;
 Place(target,1,20);ancient.type=1;ancient.cls=Barbarian::AncientClasses[0];ancient.id=40;ancient.stats[6]=1000*256;
 Curses::Find=BFind;Curses::X=BX;Curses::Y=BY;Curses::MinionOwner=BMinion;Curses::Owner=BNoOwner;Curses::Cast=BCast;SoulHarvest::Room=BRoom;
 Barbarian::TestLevel=BLevel;Pulse::OriginalCreate=BCreate;Curses::NativeMissileHit=BMissile;
 assert(Barbarian::Chance(0)==0&&Barbarian::Chance(1)==12&&Barbarian::Chance(20)==50&&Barbarian::Chance(1000)==100);
 assert(Barbarian::CarnagePermille(0,1,100)==0&&Barbarian::CarnagePermille(1,50,100)==1100);
 assert(Barbarian::CarnagePermille(20,50,100)==3000);
 assert(Barbarian::CarnagePermille(1,99,100)==22); // Preserve the 0.2 percentage point increment.
 Fresh(100);player.levels[128]=1;target.stats[6]=500*256;Damage();Barbarian::Before(game,&player,&target,packet);
 // Hit-reaction events must leave the packet unchanged. Actual Rage scaling
 // occurs on the resolved packet immediately before native HP subtraction.
 assert(Barbarian::ReadDamage(packet,0x18)==100*256);
 Barbarian::Hit(game,&player,&target,packet);
 assert(Barbarian::ReadDamage(packet,0x18)==210*256&&Barbarian::ReadDamage(packet,0x134)==210*256);
 target.stats[6]=0;Barbarian::After(game,&player,&target,packet);assert(GetUnit(&player,68,0)==21);assert(player.lists[Barbarian::CarnageState].expiry==350);
 // Berserker grants Rage as an oskill; damage and kill buffs work on every class.
 for(unsigned cls=0;cls<7;++cls){
  Fresh(100);player.cls=cls;player.levels[128]=1;target.stats[6]=500*256;Damage();
  Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);
  assert(Barbarian::ReadDamage(packet,0x18)==210*256);
  target.stats[6]=0;Barbarian::After(game,&player,&target,packet);assert(GetUnit(&player,68,0)==21);
 }
 player.cls=4;
 // Normal damage-event completion occurs before HP subtraction. Confirm death
 // on the next frame, then start the full ten-second timer once.
 Fresh(100);player.levels[128]=1;Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);
 assert(!GetList(&player,Barbarian::CarnageState));target.stats[6]=0;Frame(105);Barbarian::Tick(game);
 assert(player.lists[Barbarian::CarnageState].expiry==355);Frame(110);Barbarian::Tick(game);assert(player.lists[Barbarian::CarnageState].expiry==355);
 // Return to a fresh timer for the early-expiry regression.
 Fresh(100);player.levels[128]=1;Barbarian::GrantCarnage(game,&player);
 // An unexpectedly early native state expiry must not shorten Rage's timer.
 player.lists[Barbarian::CarnageState].expiry=101;Frame(125);Barbarian::Tick(game);
 assert(GetUnit(&player,68,0)==21&&player.lists[Barbarian::CarnageState].expiry==350);
 Frame(350);Barbarian::Tick(game);assert(GetUnit(&player,68,0)==0);
 Fresh(100);player.levels[127]=45;Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);
 auto* t=Barbarian::Track(&target,100);assert(t->bleed==70*256&&t->bleedUntil==225);
 Frame(101);Damage(10);Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);assert(t->bleed==70*256&&t->bleedUntil==225);
 Frame(125);Barbarian::Tick(game);assert(tickCount==1&&lastTick==70);assert(GetUnit(&player,AuraBuffs::ChargeStat,0)==0);
 target.x++;Frame(150);Barbarian::Tick(game);assert(tickCount==2&&lastTick==210);
 Barbarian::Depth=1;Damage();Barbarian::Hit(game,&player,&target,packet);Barbarian::Depth=0;
 Curses::ExtraInstantMissile=Barbarian::TickMissile;tickMissile.cls=Barbarian::TickMissile;assert(Curses::OnMissileTick(game,&tickMissile)==2);
 // Independent Impales consume only on later hits, never their applying hit.
 Fresh(200);player.levels[135]=45;
 alignas(8) unsigned char skillList[0x20]{},node[0x10]{};short skillRecord=2;void* p=&skillRecord;std::memcpy(node,&p,8);p=node;std::memcpy(skillList+0x18,&p,8);p=skillList;std::memcpy(reinterpret_cast<unsigned char*>(&player)+0x100,&p,8);
 Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);
 t=Barbarian::Track(&target,200);assert(!t->bleed&&t->impales[0].hits==5&&t->impales[0].amount==10*256);
 for(int i=0;i<6;++i){Damage();Barbarian::Hit(game,&ally,&target,packet);assert(Barbarian::ReadDamage(packet,0x134)==(i<5?110:100)*256);}
 for(int i=0;i<12;++i){Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);}
 for(const auto& i:t->impales)assert(!i.hits||i.amount==10*256); // Stored extra never recursively snapshots itself.
 Frame(401);Damage();Barbarian::Hit(game,&ally,&target,packet);assert(Barbarian::ReadDamage(packet,0x134)==100*256);
 // Quake can carry both mastery effects; an ordinary Paladin cannot create them.
 Fresh(450);player.levels[127]=player.levels[135]=45;skillRecord=137;p=skillList;std::memcpy(reinterpret_cast<unsigned char*>(&player)+0x100,&p,8);
 Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);
 t=Barbarian::Track(&target,450);assert(t->bleed&&t->impales[0].hits==5);
 // Physical spells such as War Cry do not gain melee Bleeding.
 Fresh(475);player.levels[127]=player.levels[135]=45;skillRecord=154;p=skillList;std::memcpy(reinterpret_cast<unsigned char*>(&player)+0x100,&p,8);
 Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);t=Barbarian::Track(&target,475);assert(!t->bleed&&!t->impales[0].hits);
 // Item ailments work for all player classes without mastery points.
 assert(Barbarian::CombinedChance(60,50)==100&&Barbarian::CombinedChance(0,25)==25);
 assert(Barbarian::CombinedChance(0,-3)==0);
 for(unsigned cls=0;cls<8;++cls){
  Fresh(480);player.cls=cls;player.stats[256]=player.stats[257]=100;
  Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);
  t=Barbarian::Track(&target,480);assert(t->bleed==70*256&&t->impales[0].hits==5&&t->impales[0].amount==10*256);
 }
 Fresh(481);Damage();unsigned short blocked=33;std::memcpy(packet+4,&blocked,2);Barbarian::Hit(game,&player,&target,packet);
 t=Barbarian::Track(&target,481);assert(!t->bleed&&!t->impales[0].hits);
 Fresh(482);Damage(0);Barbarian::Hit(game,&player,&target,packet);t=Barbarian::Track(&target,482);assert(!t->bleed&&!t->impales[0].hits);
 // Removing equipment removes its proc chance; no learned mastery is created.
 Fresh(483);player.stats[256]=player.stats[257]=0;Damage();Barbarian::Hit(game,&player,&target,packet);t=Barbarian::Track(&target,483);assert(!t->bleed&&!t->impales[0].hits);
 player.cls=4;
 // Jab uses custom Bleed on a successful direct weapon hit; blocked hits do not apply it.
 Fresh(485);player.cls=0;player.levels[10]=91;skillRecord=10;p=skillList;std::memcpy(reinterpret_cast<unsigned char*>(&player)+0x100,&p,8);
 Damage();Barbarian::Before(game,&player,&target,packet);Barbarian::Hit(game,&player,&target,packet);Barbarian::After(game,&player,&target,packet);
 t=Barbarian::Track(&target,485);assert(t->bleed==70*256);
 Fresh(486);player.cls=0;player.levels[10]=91;p=skillList;std::memcpy(reinterpret_cast<unsigned char*>(&player)+0x100,&p,8);
 Damage();std::memcpy(packet+4,&blocked,2);Barbarian::Hit(game,&player,&target,packet);assert(!Barbarian::Track(&target,486)->bleed);
 player.cls=4;
 // Ground Stomp: five separate successful hit opportunities, once per target
 // per pulse, rather than overlapping ring projectiles applying extra damage.
 Fresh(490);player.levels[140]=20;skillRecord=140;p=skillList;std::memcpy(reinterpret_cast<unsigned char*>(&player)+0x100,&p,8);
 Barbarian::OriginalWarcry=BWarcry;unsigned beforeStomp=tickCount;
 assert(Barbarian::StompRadius(1)==20&&Barbarian::StompRadius(20)==20&&Barbarian::StompRadius(1000)==20);
 assert(Barbarian::StompCount(1)==5&&Barbarian::StompCount(9)==5&&Barbarian::StompCount(10)==6&&Barbarian::StompCount(20)==7);
 Damage();unsigned short blockedStomp=33;std::memcpy(packet+4,&blockedStomp,2);Barbarian::Hit(game,&player,&target,packet);assert(Barbarian::Stomps[0].id==0xffffffffU);
 Damage();Barbarian::Hit(game,&player,&target,packet);assert(Barbarian::Stomps[0].left==7);assert(!Barbarian::StartStomp(game,&player,20));
 for(unsigned frame=490;frame<=520;frame+=5){Frame(frame);Barbarian::StompTick(game,frame);Barbarian::StompTick(game,frame);}
 assert(tickCount==beforeStomp+7&&rockCreates==7*16&&target.stats[6]==300*256);
 Frame(600);Barbarian::StompTick(game,600);assert(tickCount==beforeStomp+7);
 assert(!Barbarian::StartStomp(game,&player,20));Frame(615);assert(Barbarian::StartStomp(game,&player,20));
 assert(Barbarian::StompDamagePercent(1)==60&&Barbarian::StompDamagePercent(20)==250);
 assert(Barbarian::StompDamagePercent(20,20,20)==750&&Barbarian::StompDamagePercent(20,10,0)==375);
 // A proc caused by a pulse cannot recursively start another Stomp.
 Barbarian::StompDispatch=true;assert(!Barbarian::StartStomp(game,&player,20));Barbarian::StompDispatch=false;
 // Out-of-range enemies are excluded; dying casters cancel remaining pulses.
 Fresh(610);target.x=200;target.y=200;beforeStomp=tickCount;assert(Barbarian::StartStomp(game,&player,1));Barbarian::StompTick(game,610);assert(tickCount==beforeStomp);
 player.stats[6]=0;Frame(615);Barbarian::StompTick(game,615);assert(Barbarian::Stomps[0].id==0xffffffffU);player.stats[6]=1000*256;target.x=target.y=20;
 Pulse::Params ring{};ring.skill=Barbarian::RainSkill;ring.missile=Barbarian::RockControl;ring.level=20;
 Barbarian::FilterProjectile(game,&ring);assert(ring.range==27&&(ring.flags&0x8000));
 // Full weapon damage is scaled without changing poison/cold durations.
 Pulse::Params scaledHit{};scaledHit.missile=Barbarian::RockHit;scaledHit.level=20;
 tickMissile.stats[21]=500;tickMissile.stats[22]=1000;tickMissile.stats[48]=250;tickMissile.stats[56]=75;
 Barbarian::ProjectileCreated(game,&scaledHit,&tickMissile);
 assert(GetUnit(&tickMissile,21,0)==1250&&GetUnit(&tickMissile,22,0)==2500&&GetUnit(&tickMissile,48,0)==625&&GetUnit(&tickMissile,56,0)==75);
 assert(Curses::ProcDepth==0); // Native on-striking events remain enabled.
 assert(Barbarian::IgnoreDamage(game,&target,&ancient,packet)&&!Barbarian::IgnoreDamage(game,&ancient,&target,packet));
 // Native corpse success, five-corpse cap, one heal per consumed corpse.
 Fresh(500);RedemptionLife::Original=BRedeem;Barbarian::OriginalWarcry=BWarcry;player.stats[6]=100*256;
 for(unsigned i=0;i<bodies.size();++i){Place(bodies[i],1,50+i);bodies[i].mode=12;bodies[i].stats[6]=0;}
 assert(Barbarian::Warcry(game,&player,131,10)==1&&redeems==5);assert(GetUnit(&player,6,0)==350*256&&GetUnit(&player,18,0)==50);assert(player.lists[Barbarian::FeastState].expiry==950);
 assert(Barbarian::Warcry(game,&player,138,10)==789);
 // Feast reserves exactly the requested amount, succeeds without corpses,
 // reuses normal budget/level/death handling, and releases mana on toggle-off.
 for(auto& b:bodies)b.mode=0;
 Fresh(600);AuraBuffs::Session=nullptr;AuraBuffs::Players={};player.levels[131]=20;
 player.stats[6]=1000*256;player.stats[9]=100*256;player.stats[8]=100*256;
 auto feastIndex=AuraBuffs::Index(131);assert(feastIndex>=0);
 assert(AuraBuffs::Cost(feastIndex,1)==18&&AuraBuffs::Cost(feastIndex,20)==75&&AuraBuffs::Cost(feastIndex,20,100)==37);
 AuraBuffs::FeaturePulse=Barbarian::FeastPulse;
 assert(AuraBuffs::Toggle(game,&player,131,20)==1);
 assert(GetUnit(&player,9,0)==25*256&&GetList(&player,Barbarian::FeastAuraState));
 player.levels[131]=21;Frame(606);AuraBuffs::OnFrame(game);assert(GetUnit(&player,9,0)==22*256);
 assert(AuraBuffs::Toggle(game,&player,131,21)==1&&GetUnit(&player,9,0)==100*256);
 player.stats[9]=10*256;assert(AuraBuffs::Toggle(game,&player,131,21)==0);
 AuraBuffs::FeaturePulse=nullptr;
 // Timed Ancients are removed through native Unsummon, not freed by hand.
 for(auto& b:bodies)b.mode=0;
 alignas(8) unsigned char ancientList[0x20]{},ancientNode[0x10]{};short ancientRecord=Barbarian::AncientAttacks[0];
 p=&ancientRecord;std::memcpy(ancientNode,&p,8);p=ancientNode;std::memcpy(ancientList,&p,8);p=ancientList;std::memcpy(reinterpret_cast<unsigned char*>(&ancient)+0x100,&p,8);GuardianCombat::BindNativeSkills=true;
 Fresh(700);Barbarian::RemovePet=BRemove;Barbarian::RoomDelete=BRoomDelete;Barbarian::DeleteUnit=BDelete;Barbarian::ModeInfo=BModeInfo;Barbarian::ModeChange=BModeChange;Barbarian::GetPath=BPath;Barbarian::SetStep=BStep;Place(ancient,Barbarian::AncientClasses[0],40);player.stats[6]=1000*256;player.levels[Barbarian::CallSkill]=10;Barbarian::Tick(game);
 assert(Barbarian::Ancients[0].until==1700&&ancientCasts>0&&Barbarian::Stomps[0].id==0xffffffffU);
 auto& seqSlot=Barbarian::Ancients[0];seqSlot.castAt=0;ancient.mode=14;unsigned oldCasts=ancientCasts;Frame(800);
 Barbarian::AncientAttack(game,&player,&ancient,0,seqSlot,800);assert(ancientCasts==oldCasts&&Barbarian::Stomps[0].id==0xffffffffU);ancient.mode=1;
 // A private active Ancient defers pet AI without changing its animation.
 // Ordinary pets and inactive Ancients still forward to the original AI.
 Barbarian::OriginalPetAi=BPetAi;ancient.mode=14;unsigned beforeSchedule=scheduled;
 Barbarian::PetAi(game,&ancient,nullptr);assert(!petAiCalls&&scheduled==beforeSchedule+1&&ancient.mode==14);
 ancient.mode=1;Barbarian::PetAi(game,&ancient,nullptr);assert(petAiCalls==1);
 target.mode=14;Barbarian::PetAi(game,&target,nullptr);assert(petAiCalls==2);
 Frame(1700);Barbarian::Tick(game);assert(unsummons==1&&ancientDeleted);Frame(1725);Barbarian::Tick(game);assert(unsummons==1);
 // Live regression: Might exists on the player but native propagation omits
 // untargetable Ancients. Share the computed state, not all player bonuses.
 Barbarian::AuraArea=BAuraArea;
 Fresh(2000);ancientDeleted=false;ancient.mode=1;ancient.room=player.room;
 Curses::ListOwnerType=BAuraOwnerType;Curses::ListOwnerId=BAuraOwnerId;Curses::OriginalApply=BAuraApply;
 const auto might=AuraBuffs::Index(98);auto* auraRow=static_cast<unsigned char*>(Record(3,98));
 unsigned filter=0x12003;std::memcpy(auraRow+0x70,&filter,4);
 std::array<short,6> auraStats{25,136,-1,-1,-1,-1};std::memcpy(auraRow+0x74,auraStats.data(),12);
 int rangeFormula=50;std::memcpy(auraRow+0x84,&rangeFormula,4);short auraState=33;std::memcpy(auraRow+0xa2,&auraState,2);
 Curses::Curse ownAura{&player,&player,98,4,51,-1,0,33,nullptr};BAuraApply(&ownAura);
 player.lists[33].stats[25]=35;player.lists[33].stats[136]=4;player.stats[25]=777;
 for(auto cls:Barbarian::AncientClasses){
   ancient.cls=cls;ancient.lists.clear();ancient.x=player.x+2;ancient.y=player.y;
   Place(ancient,cls,40);Barbarian::ShareAura(game,&player,might,4);
   assert(ancient.lists[33].stats[25]==35&&ancient.lists[33].stats[136]==4);
   Barbarian::ShareAura(game,&player,might,4);assert(GetUnit(&ancient,25,0)==35);
 }
 player.lists[33].stats[25]=15;Barbarian::ShareAura(game,&player,might,4);assert(GetUnit(&ancient,25,0)==15);
 auto expiry=ancient.lists[33].expiry;Frame(2005);ancient.x=player.x+51;
 Barbarian::ShareAura(game,&player,might,4);assert(ancient.lists[33].expiry==expiry);
 ancient.x=player.x;ancient.room=player.room+1;Barbarian::ShareAura(game,&player,might,4);assert(ancient.lists[33].expiry>expiry);
 expiry=ancient.lists[33].expiry;Frame(2010);ancient.room=player.room+100;
 Barbarian::ShareAura(game,&player,might,4);assert(ancient.lists[33].expiry==expiry);
 ancient.room=player.room;ancient.cls=1;Barbarian::ShareAura(game,&player,might,4);assert(ancient.lists[33].expiry==expiry);
 ancient.cls=Barbarian::AncientClasses[0];ancient.lists[33].owner=&ally;ancient.lists[33].stats[25]=99;
 Barbarian::ShareAura(game,&player,might,4);assert(ancient.lists[33].stats[25]==99&&ancient.lists[33].expiry==expiry);
 ancient.lists[33].owner=&player;player.lists.erase(33);
 Barbarian::ShareAura(game,&player,might,4);Frame(expiry);assert(!GetList(&ancient,33));
 Curses::OriginalApply=Apply;
 // Native calc4 is +0x19c, so calc5 must be +0x1a0 (not +0x194).
 Curses::Evaluate=BFormula;
 auto* qRecord=static_cast<unsigned char*>(Record(3,137));int qFormula=1902;std::memcpy(qRecord+0x1a0,&qFormula,4);
 assert(Barbarian::QuakePoints(game,&player,20)==20);
 for(const auto& pair:Barbarian::Helpers)if(pair[0]==127){auto* hRecord=static_cast<unsigned char*>(Record(3,pair[1]));int formula=1901;std::memcpy(hRecord+0x1a0,&formula,4);assert(Barbarian::HelperCalc(game,&player,127,5)==12);}
 Curses::Evaluate=Evaluate;
 // Native buff eligibility must restore flags on both success and failure.
 Barbarian::AncientExtra=BExtra;Curses::OriginalApply=BEligibleApply;
 privateExtra[4]=0x20;privateExtra[5]=0x40;ancient.cls=Barbarian::AncientClasses[0];
 Curses::Curse safeBuff{&player,&ancient,98,4,51,-1,0,33,nullptr};
 assert(Barbarian::ApplyFriendlyBuff(game,safeBuff)&&privateExtra[4]==0x20&&privateExtra[5]==0x40);
 rejectBuff=true;assert(!Barbarian::ApplyFriendlyBuff(game,safeBuff)&&privateExtra[5]==0x40);
 rejectBuff=false;Barbarian::AncientExtra=nullptr;Curses::OriginalApply=Apply;
 // Quake scales its weapon snapshot, then adds constant quarter-base damage.
 assert(Barbarian::QuakeWeaponPercent(1,0)==16&&Barbarian::QuakeWeaponPercent(20,0)==48);
 assert(Barbarian::QuakeWeaponPercent(1,20)==33&&Barbarian::QuakeWeaponPercent(20,40)==145);
 Pulse::Params quakeHit{};quakeHit.skill=137;quakeHit.missile=Barbarian::QuakeMissile;quakeHit.level=1;
 tickMissile.stats[21]=100*256;tickMissile.stats[22]=200*256;tickMissile.stats[48]=40*256;tickMissile.stats[56]=75;
 Barbarian::ProjectileCreated(game,&quakeHit,&tickMissile);
 assert(GetUnit(&tickMissile,21,0)==100*256/6+2*256&&GetUnit(&tickMissile,22,0)==200*256/6+3*256);
 assert(GetUnit(&tickMissile,48,0)==40*256/6&&GetUnit(&tickMissile,56,0)==75);

 std::puts("PASS: Carnage formula/kill buff; strongest physical Bleeding and moving multiplier; single native tick; independent five-hit Impales without recursive snapshots; expiry; Quake masteries; native corpse consumption/cap/heal; ordinary warcry forwarding; five Stomp pulses, radius/death/non-recursion guards; private Ancient invulnerability and native pet removal.");
}
