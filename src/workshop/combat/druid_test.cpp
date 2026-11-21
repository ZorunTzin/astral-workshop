#define main BaseAuraTests
#include "aura_test.cpp"
#undef main
unsigned morphOn=0,morphOff=0,spellHits=0,appearanceOn=0,appearanceOff=0;Unit foe,pet,projectile;
void* __fastcall DruidCreate(void*,void* raw){auto& p=*static_cast<Pulse::Params*>(raw);if(p.missile==Druid::HitMissile){projectile.stats.clear();return &projectile;}return nullptr;}
int __fastcall DruidImpact(void*,void* m,void* t,int){assert(m==&projectile&&t==&foe);assert(GetUnit(m,54,0)==167*256&&GetUnit(m,55,0)==177*256);++spellHits;return 1;}
void PlaceDruid(Unit& u,int type,unsigned id){u.type=type;u.id=id;void* ptr=&u;std::memcpy(game+(type?0x2630:0x2230)+8*(id&127),&ptr,8);}

int __fastcall DruidMorph(void*,void* u,int skill,int){assert(skill==Druid::Morph);auto& unit=*static_cast<Unit*>(u);if(GetList(u,Druid::AvatarState)){unit.lists.erase(Druid::AvatarState);++morphOff;}else{auto& l=unit.lists[Druid::AvatarState];l.state=Druid::AvatarState;l.expiry=Ramp::Frame(game)+75;++morphOn;}return 1;}
int main(){
 BaseAuraTests();
 Curses::Evaluate=[](unsigned char v,void* u,int f,int sid,int l)->int{if(f==230)return 100;if(f==240)return (100+GetUnit(u,Druid::PhysicalSpellStat,0));return Evaluate(v,u,f,sid,l);};
 Curses::OriginalApply=[](void* raw)->void*{auto& r=*static_cast<Curses::Curse*>(raw);if(r.state==Druid::PrimalState)assert(r.skill==239&&r.level==20);if(r.state==Druid::AlphaState)assert(r.skill==224&&r.level==20);return Apply(raw);};
 Frame(1000);player.stats[6]=1000*256;player.stats[7]=1000*256;player.stats[9]=2000*256;player.stats[8]=2000*256;player.lists.clear();AuraBuffs::Players={};
 AuraBuffs::FeaturePulse=Druid::BuffPulse;AuraBuffs::FeatureDisable=Druid::Disable;Sorceress::NativeMorph=DruidMorph;
 GuardianCombat::ToggleState=[](void*,int state,int active){if(state==Druid::AvatarState){if(active)++appearanceOn;else ++appearanceOff;}};
 for(int sid:{221,224,239}){player.levels[sid]=20;assert(AuraBuffs::Toggle(game,&player,sid,20)==1);}
 assert(AuraBuffs::Reservation(game,&player)==335*256);assert(GetUnit(&player,329,0)==75&&GetUnit(&player,331,0)==75&&GetUnit(&player,330,0)==0);
 assert(GetUnit(&player,40,0)==2&&GetUnit(&player,42,0)==2&&GetUnit(&player,44,0)==2&&GetUnit(&player,46,0)==2);
 assert(GetUnit(&player,25,0)==120);assert(GetUnit(&player,171,0)==230); // Only Primal Fury adds defense, not Avatar.
 assert(GetUnit(&player,Druid::PhysicalSpellStat,0)==75);assert(Druid::SpellDamage(game,&player,240,20,210)==367);
 assert(GetUnit(&player,105,0)==30&&GetUnit(&player,99,0)==55);
 for(unsigned t=1005;t<1500;t+=5){Frame(t);AuraBuffs::OnFrame(game);}assert(morphOn==1&&morphOff==0&&appearanceOn==1&&appearanceOff==0);assert(GetList(&player,Druid::AvatarState));
 assert(AuraBuffs::Toggle(game,&player,221,20)==1&&morphOff==1&&!GetList(&player,Druid::AvatarState));
 assert(AuraBuffs::Reservation(game,&player)==195*256);assert(appearanceOn==1&&appearanceOff==1);
 unsigned next=0,count=0;for(unsigned t=0;t<75;t+=5)if(Druid::WinterTick(t,next,75))++count;assert(count==3&&!Druid::WinterTick(75,next,75));
 next=125;count=0;for(unsigned t=0;t<=500;++t)if(Druid::Due(t,next,125))++count;assert(count==4);assert(Druid::Due(1000,next,125)&&next==1125); // No burst catch-up.
 int hits=0;for(int n=1;n<=20;++n)assert(Druid::FifthHit(hits)==(n%5==0));
 Druid::Zone z;z.x=z.y=0;z.tx=10;z.ty=0;assert(Druid::Cone(z,8,0)&&Druid::Cone(z,8,4));assert(!Druid::Cone(z,-2,0)&&!Druid::Cone(z,0,8)&&!Druid::Cone(z,13,0));
 z.level=20;assert(Druid::Cone(z,17,0)&&!Druid::Cone(z,19,0));
 Druid::Seeds[0].owner=10;Druid::Marks[0].target=11;Druid::Reset(game);Frame(1);Druid::Reset(game);assert(Druid::Seeds[0].owner==Druid::Invalid&&Druid::Marks[0].target==Druid::Invalid);

 // Exercise full cast -> scheduled zone -> native impact with no enemies behind the cone.
 Assassin::NativePositions=false;Assassin::GetTarget=[](void*,void*)->void*{return &foe;};Curses::X=[](void* u)->int{return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u)->int{return static_cast<Unit*>(u)->y;};
 Curses::Owner=[](void*,void*)->void*{return nullptr;};Curses::MinionOwner=[](void* u)->void*{return u==&pet?&player:nullptr;};Barbarian::AuraArea=[](const void*) noexcept ->int{return 1;};
 Pulse::OriginalCreate=DruidCreate;Curses::NativeMissileHit=DruidImpact;Pulse::AfterCreate=nullptr;Pulse::FilterCreate=nullptr;Amazon::PublishMissiles=nullptr;
 foe.stats[6]=10000;foe.x=player.x+8;foe.y=player.y;PlaceDruid(foe,1,23);
 for(unsigned phase=0;phase<5;++phase){unsigned start=2000+phase*150;Frame(start);Druid::Zones={};Druid::Next=start+phase;unsigned before=spellHits;int mp=GetUnit(&player,8,0);assert(Druid::Cast(game,&player,230,20)==1);assert(GetUnit(&player,8,0)==mp-29*256);for(unsigned t=start;t<start+130;++t){Frame(t);Druid::Frame(game);}assert(spellHits-before==5);}
 // Different corpse buffs must not extend one another's expiry.
 pet.cls=357;pet.stats[6]=500*256;pet.stats[7]=1000*256;pet.x=player.x;pet.y=player.y;PlaceDruid(pet,1,24);PlaceDruid(player,0,player.id);
 Frame(3000);Druid::VineBuff(game,&player,20,true);assert(GetUnit(&pet,68,0)==25);unsigned solarEnd=static_cast<List*>(GetList(&pet,Druid::SolarState))->expiry;
 Frame(3100);Druid::VineBuff(game,&player,20,false);assert(static_cast<List*>(GetList(&pet,Druid::SolarState))->expiry==solarEnd);assert(GetList(&pet,Druid::VineState));Frame(3126);assert(!GetList(&pet,Druid::SolarState)&&GetList(&pet,Druid::VineState));
 puts("PASS: Druid simultaneous reservations, native morph reversal/renewal, no lightning/defense from Avatar, max-resist steps, three frost ticks, bounded Raven cadence, fifth-hit trigger, cone bounds and session cleanup.");
}
