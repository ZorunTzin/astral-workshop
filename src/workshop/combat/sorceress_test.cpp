#define main BaseAuraTests
#include "aura_test.cpp"
#undef main
unsigned sorcCasts=0,stormCreates=0,eruptionVisuals=0;int sorcSkill=0,sorcLevel=0,lastStorm=0;
int __fastcall SorcCast(void*,int skill,int level,void*,int){assert(Curses::ProcDepth>0);if(skill==Sorceress::EruptionVisualSkill){++eruptionVisuals;return 1;}if(skill==Sorceress::FireProc)return 0;if(skill==Sorceress::FrostNova){++sorcCasts;sorcSkill=skill;sorcLevel=level;}else{++stormCreates;lastStorm=skill;}return 1;}
int __fastcall SorcListLevel(void*){return 20;}
int SorcLevel(void*,void*,int){return 20;}
void __fastcall SorcRegister(void*,void*,int,int,int,int,int,void*,const short* ev,const short* fn,int){assert(ev[0]==7&&ev[1]==8&&fn[0]==33&&fn[1]==33);}
void* __fastcall SorcOwner(void* unit){return static_cast<Unit*>(unit)->cls==Sorceress::FrostClass?&player:nullptr;}
unsigned groundCreates=0,burnHits=0,morphOn=0,morphOff=0;int lastRange=0;Unit burnMissile;
unsigned eruptionHits=0;Unit eruptionMissile;
void* eruptionTarget=nullptr;
void* __fastcall SorcFind(void* g,int type,unsigned id){if(type==1&&eruptionTarget&&static_cast<Unit*>(eruptionTarget)->id==id)return eruptionTarget;return Find(g,type,id);}
int __fastcall Morph(void*,void* unit,int skill,int level){
 auto& u=*static_cast<Unit*>(unit);if(GetList(unit,Sorceress::AvatarState)){u.lists.erase(Sorceress::AvatarState);++morphOff;return 0;}
 Curses::Curse request{unit,unit,skill,level,75,-1,0,Sorceress::AvatarState,nullptr};Apply(&request);++morphOn;return 1;
}
void* __fastcall CreateSorc(void*,void* raw){
 const auto& p=*static_cast<Pulse::Params*>(raw);assert(p.owner==&player&&p.level==20);
 if(p.missile==Sorceress::EruptionVisual){++eruptionVisuals;assert(p.range==75);return nullptr;}
 if(p.missile==Sorceress::EruptionHit){assert(p.skill==49&&p.range==1&&p.target&&p.target==p.origin);return &eruptionMissile;}
 if(p.missile==Sorceress::GroundMissile){++groundCreates;assert(p.range>=55&&p.range<=75);int dx=p.x-20,dy=p.y-20;assert(dx*dx+dy*dy<=576);return nullptr;}
 if(p.missile==Sorceress::BurnMissile){assert(p.skill==Sorceress::BurnSkill);return &burnMissile;}
 assert(p.missile==Sorceress::FireMaker||p.missile==Sorceress::ColdMaker||p.missile==Sorceress::LightningMaker);
 assert(p.range==48&&Curses::ProcDepth>0);++stormCreates;lastStorm=p.skill;lastRange=p.range;return nullptr;
}
int __fastcall BurnHit(void*,void* missile,void*,int){if(missile==&eruptionMissile){assert(Sorceress::EruptionDispatch&&Curses::ProcDepth==0);++eruptionHits;return 1;}assert(missile==&burnMissile);++burnHits;return 1;}
void* __fastcall SorcRecord(unsigned char version,int skill){
 void* row=Record(version,skill);if(skill==36||skill==39||skill==43){int lo=10001,hi=10002;std::memcpy(static_cast<unsigned char*>(row)+0x190,&lo,4);std::memcpy(static_cast<unsigned char*>(row)+0x194,&hi,4);}return row;
}
int __fastcall SorcEvaluate(unsigned char version,void* unit,int formula,int skill,int level){
 if(formula==10001||formula==10002){int lo=skill==43?3+level:25+5*level,hi=skill==43?60+10*level:50+10*level;return (formula==10001?lo:hi)*(100+GetUnit(unit,329,0))/100;}
 return Evaluate(version,unit,formula,skill,level);
}
int main(){
 BaseAuraTests();
 player.lists.clear();player.stats[6]=100*256;player.stats[7]=200*256;player.stats[8]=10*256;player.stats[9]=20*256;
 Curses::Curse enchanted{&player,&player,52,10,100,-1,0,16,nullptr};Apply(&enchanted);
 assert(Sorceress::Event(game,7,&player,&player,52,10));
 assert(GetUnit(&player,6,0)==115*256&&GetUnit(&player,8,0)==20*256);
 assert(Sorceress::Event(game,8,&player,&player,52,10));assert(GetUnit(&player,6,0)==115*256);
 player.lists.clear();Sorceress::Event(game,7,&player,&player,52,10);assert(GetUnit(&player,6,0)==115*256);
Frame(1000);player.lists.clear();player.stats.clear();player.stats[6]=player.stats[7]=1000*256;player.stats[8]=player.stats[9]=2000*256;
 Curses::Record=SorcRecord;Curses::Evaluate=SorcEvaluate;Curses::Register=SorcRegister;Curses::ListLevel=SorcListLevel;Curses::Cast=SorcCast;Sorceress::TestLevel=SorcLevel;Sorceress::Session=nullptr;
 Sorceress::NativeMorph=Morph;Sorceress::NativeStorm=[](void*,void*,int skill,int)->int{assert(skill==Sorceress::FireProc);++stormCreates;lastStorm=skill;return 1;};Pulse::OriginalCreate=CreateSorc;
 Curses::X=[](void* p)->int{return static_cast<Unit*>(p)->x;};Curses::Y=[](void* p)->int{return static_cast<Unit*>(p)->y;};
 AuraBuffs::Players={};AuraBuffs::FeaturePulse=Sorceress::Pulse;AuraBuffs::FeatureDisable=Sorceress::Disable;
 for(int skill:{36,39,43}){player.levels[skill]=20;assert(AuraBuffs::Toggle(game,&player,skill,20)==1);}
 assert(GetUnit(&player,171,0)==230&&GetUnit(&player,119,0)==300);assert(GetUnit(&player,54,0)==125&&GetUnit(&player,55,0)==250);
 assert(GetUnit(&player,105,0)==80&&GetUnit(&player,48,0)==125&&GetUnit(&player,76,0)==85);
 assert(AuraBuffs::Reservation(game,&player)==675*256); // All three forms use the Frozen Queen reservation.
 const int reserved=AuraBuffs::Reservation(game,&player),mana=GetUnit(&player,8,0);
 for(unsigned t=1005;t<=1500;t+=5){Frame(t);AuraBuffs::OnFrame(game);}
 assert(AuraBuffs::Reservation(game,&player)==reserved&&GetUnit(&player,8,0)==mana&&GetUnit(&player,6,0)==1000*256);
 for(int state:{Sorceress::AvatarState,Sorceress::QueenState,Sorceress::MasterState})assert(GetList(&player,state));
 Unit target;target.type=1;target.stats[6]=1000;
 for(int i=0;i<2000;++i)assert(Sorceress::Event(game,7,&player,&target,39,20));
 assert(stormCreates>400&&stormCreates<600&&lastStorm==Sorceress::ColdProc);
 const auto old=stormCreates;Sorceress::Depth=1;Sorceress::Event(game,7,&player,&target,39,20);Sorceress::Depth=0;assert(stormCreates==old);
 assert(Sorceress::Event(game,7,&player,&target,52,20));
 assert(AuraBuffs::Toggle(game,&player,39,20)==1);Frame(1502);Sorceress::Event(game,7,&player,&target,39,20);assert(stormCreates==old);
 // Each guardian has an independent 25-frame cooldown, including same-frame hits.
 Curses::MinionOwner=SorcOwner;Curses::Owner=[](void*,void*)->void*{return nullptr;};Unit pet;pet.type=1;pet.cls=Sorceress::FrostClass;pet.stats[6]=1000;
 unsigned char damage[0x140]{};*reinterpret_cast<unsigned short*>(damage+4)=1;
 for(int i=0;i<3;++i)Sorceress::Hit(game,&pet,&target,damage);
 assert(sorcCasts==1&&sorcSkill==Sorceress::FrostNova);
 Unit pet2;pet2.type=1;pet2.cls=Sorceress::FrostClass;pet2.id=pet.id+1;pet2.stats[6]=1000;
 Sorceress::Hit(game,&pet2,&target,damage);assert(sorcCasts==2);
 Frame(1526);Sorceress::Hit(game,&pet,&target,damage);Sorceress::Hit(game,&pet2,&target,damage);assert(sorcCasts==2);
 Frame(1527);Sorceress::Hit(game,&pet,&target,damage);assert(sorcCasts==3);
 Sorceress::Depth=1;Sorceress::Hit(game,&pet,&target,damage);Sorceress::Depth=0;assert(sorcCasts==3);
 *reinterpret_cast<unsigned short*>(damage+4)=0;Sorceress::Hit(game,&pet,&target,damage);assert(sorcCasts==3);
 player.stats[329]=50;assert(Sorceress::AddedDamage(game,&player,36,20,false)==187);player.stats[329]=0;
 assert(morphOn==1&&morphOff==0);assert(AuraBuffs::Toggle(game,&player,36,20)==1);assert(morphOff==1&&!GetList(&player,Sorceress::AvatarState));
 Frame(1504);assert(AuraBuffs::Toggle(game,&player,36,20)==1);assert(morphOn==2);
 unsigned beforeFire=stormCreates;for(int i=0;i<100;++i)Sorceress::Event(game,7,&player,&target,36,20);
 assert(stormCreates>beforeFire&&lastStorm==Sorceress::FireProc);
 // Native nova creates dozens of missiles, but exactly one burn zone per cast.
 Curses::X=[](void*)->int{return 20;};Curses::Y=[](void*)->int{return 20;};
 Pulse::Params params{};params.owner=&player;params.missile=Sorceress::ConflagrationMissile;params.level=20;
 for(int i=0;i<64;++i)Sorceress::Projectile(game,&params,nullptr);
 unsigned zones=0;for(auto& z:Sorceress::Zones)if(z.owner!=0xffffffffU)++zones;assert(zones==1);
  // Visual flames stay inside the damage disk; burn hits occur at 1/2/3 seconds.
 Curses::NativeMissileHit=BurnHit;Barbarian::AuraArea=[](const void*) noexcept ->int{return 1;};
 void* tp=&target;std::memcpy(game+0x2630,&tp,8);
 for(unsigned t=1509;t<=1579;t+=5){Frame(t);Sorceress::Tick(game);}
 assert(groundCreates==0&&burnHits==3);Frame(1584);Sorceress::Tick(game);assert(burnHits==3&&groundCreates==0);
 // Reproduce real polling phase offsets; the old implementation lost tick 3
 // whenever the last poll fell beyond frame start+75.
 for(unsigned phase=0;phase<5;++phase){
  unsigned start=2000+phase*100;Frame(start);Sorceress::Zones={};Sorceress::Next=start+phase;
  Sorceress::Projectile(game,&params,nullptr);unsigned before=burnHits;
  for(unsigned t=start+phase;t<=start+85;t+=5){Frame(t);Sorceress::Tick(game);}
  assert(burnHits-before==3);
 }
 eruptionTarget=&target;Curses::Find=SorcFind;
 // Three pulses per tornado even when the polling phase is shifted.
 for(unsigned phase=0;phase<5;++phase){
  unsigned start=3000+phase*100;Frame(start);Sorceress::Eruptions={};Sorceress::Next=start+phase;
  assert(Sorceress::StartEruption(game,&player,&target,20));
  for(int spam=0;spam<100;++spam)assert(Sorceress::StartEruption(game,&player,&target,20));
  unsigned before=eruptionHits;
  for(unsigned t=start+phase;t<=start+85;t+=5){Frame(t);Sorceress::Tick(game);}
  assert(eruptionHits-before==3);
 }
 assert(eruptionVisuals==505);
 assert(Sorceress::AttackEvents>0);
 // Snapshot scaling keeps ten percent of weapon damage and adds all three elements.
 eruptionMissile.stats.clear();eruptionMissile.stats[21]=100*256;
 for(int st:{48,49,50,51,54,55})eruptionMissile.stats[st]=100*256;
 Pulse::Params ep{};ep.owner=&player;ep.level=20;ep.missile=Sorceress::EruptionHit;
 Sorceress::Projectile(game,&ep,&eruptionMissile);
 assert(GetUnit(&eruptionMissile,21,0)==10*256);
 for(int st:{48,50,54})assert(GetUnit(&eruptionMissile,st,0)==110*256);
 for(int st:{49,51,55})assert(GetUnit(&eruptionMissile,st,0)==115*256);
 assert(!Sorceress::EruptionDispatch&&!Sorceress::EruptionProcWindow);
 // Delayed Storm missiles cannot re-trigger the active forms.
 unsigned stormsBefore=stormCreates;
 for(int i=0;i<1000;++i)for(int form:{36,39,43})Sorceress::Event(game,8,&player,&target,form,20);
 assert(stormCreates==stormsBefore);
 // A delayed update delivers at most one pulse, never several catch-up hits.
 Frame(4000);Sorceress::Eruptions={};Sorceress::Next=0;
 assert(Sorceress::StartEruption(game,&player,&target,20));
 unsigned hitsBefore=eruptionHits;Frame(4050);Sorceress::Tick(game);assert(eruptionHits==hitsBefore+1);
 Frame(4055);Sorceress::Tick(game);assert(eruptionHits==hitsBefore+1);
 Frame(4075);Sorceress::Tick(game);assert(eruptionHits==hitsBefore+2);
 // Consecutive successful skill hits empower, but never restart pulse timing.
 Frame(5000);Sorceress::Eruptions={};Sorceress::EruptionCombos={};
 assert(Sorceress::StartEruption(game,&player,&target,20));
 assert(Sorceress::ComboPower(player.id,5000)==1);
 const unsigned scheduledPulse=Sorceress::Eruptions[0].next;
 Frame(5010);assert(Sorceress::StartEruption(game,&player,&target,20));
 assert(Sorceress::ComboPower(player.id,5010)==2);
 Frame(5020);assert(Sorceress::StartEruption(game,&player,&target,20));
 assert(Sorceress::ComboPower(player.id,5020)==3);
 assert(Sorceress::Eruptions[0].next==scheduledPulse);
 assert(Sorceress::ComboPower(player.id,5094)==3);
 assert(Sorceress::ComboPower(player.id,5095)==1);
 Sorceress::AddComboHit(player.id,5095);assert(Sorceress::ComboPower(player.id,5095)==1);
 Sorceress::AddComboHit(player.id,5096);assert(Sorceress::ComboPower(player.id,5096)==2);
 for(int i=0;i<100;++i)Sorceress::AddComboHit(player.id,5097);
 assert(Sorceress::ComboPower(player.id,5097)==3);
 // Moving to a new enemy replaces the center, preserves cadence and refreshes lifetime.
 Frame(5100);Sorceress::Eruptions={};Sorceress::Next=0;
 Curses::X=[](void* p)->int{return static_cast<Unit*>(p)->x;};Curses::Y=[](void* p)->int{return static_cast<Unit*>(p)->y;};
 target.x=20;target.y=20;assert(Sorceress::StartEruption(game,&player,&target,20));
 Frame(5110);target.x=40;target.y=50;assert(Sorceress::StartEruption(game,&player,&target,20));
 unsigned activeZones=0;for(const auto& z:Sorceress::Eruptions)if(z.owner!=0xffffffffU){++activeZones;assert(z.x==40&&z.y==50&&z.next==5125&&z.until==5185);}
 assert(activeZones==1);
 // Triple both weapon and added elemental components, not proc probability.
 eruptionMissile.stats.clear();eruptionMissile.stats[21]=100*256;
 for(int st:{48,49,50,51,54,55})eruptionMissile.stats[st]=100*256;
 Sorceress::EruptionPower=3;Sorceress::Projectile(game,&ep,&eruptionMissile);Sorceress::EruptionPower=1;
 assert(GetUnit(&eruptionMissile,21,0)==30*256);
 for(int st:{48,50,54})assert(GetUnit(&eruptionMissile,st,0)==330*256);
 for(int st:{49,51,55})assert(GetUnit(&eruptionMissile,st,0)==345*256);
 std::memset(game+0x2630,0,8);
 Frame(10);Sorceress::Reset(game);for(auto& z:Sorceress::Zones)assert(z.owner==0xffffffffU);
 puts("PASS: three persistent reservations, renewal without drains, toggle off, attack procs, independent one-second Frost Nova cooldowns, recursion guard and native morph reversal, unshifted damage, native storm dispatch/fallback, bounded long-lived flames and exactly three burn ticks at every polling phase.");
}
