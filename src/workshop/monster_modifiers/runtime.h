#pragma once
#include "engine.h"
#include "effects_bindings.h"
#include "caption.h"
namespace MonsterRuntime {
using namespace MonsterModifiers;
using NativeInit=void(__fastcall*)(void*,int,int);
using NativeEvent=void(__fastcall*)(void*,void*,int,int);
using NativePair=void(__fastcall*)(void*,void*);
using NativeMissile=void(__fastcall*)(void*,void*,void*);
using PickFn=void(__fastcall*)(void*,void*,int,int,int,void*);
inline PickFn OriginalPick=nullptr;
using RollFn=int(__fastcall*)(void*,void*,unsigned char*,void*);
using SpawnFn=void*(__fastcall*)(void*,void*,int,int,int,int,int,unsigned short);
using DropFn=std::uintptr_t(__fastcall*)(void*,void*,void*,unsigned short,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t);
inline RollFn OriginalRoll=nullptr;inline SpawnFn Spawn=nullptr;inline NativeInit Init=nullptr,OriginalTeleport=nullptr;
inline NativeEvent OriginalFireMode=nullptr,OriginalFireExplosion=nullptr,OriginalMulti=nullptr;
inline NativePair OriginalStart=nullptr,OriginalSuccess=nullptr,OriginalHurt=nullptr,OriginalMode=nullptr;
inline NativeMissile OriginalMissile=nullptr;inline DropFn OriginalDrop=nullptr;inline NativeEvent* Callbacks=nullptr;
inline int(*AdditionalLootRolls)(void*,void*)=nullptr;
inline int(*GoldLootScale)(void*,void*)=nullptr;
inline Engine Effects;inline void* Game=nullptr;inline unsigned LastFrame=0,NextScan=0,NextRegen=0,Depth=0,DropDepth=0,WideVolley=0;
struct Extra {Id id=Invalid;Mask initialized=0;int lifePercent=100;bool counted=false;};inline std::vector<Extra> Extras;
struct Child {Id id=Invalid,owner=Invalid;Action kind=Action::Totem;unsigned until=0;int strength=0;};inline std::vector<Child> Children;
struct Field {Id owner=Invalid;Action kind=Action::BurningGround;int x=0,y=0,damage=0;unsigned next=0,end=0;};inline std::vector<Field> Fields;
inline Id Key(void* u){return u?Identity(static_cast<unsigned*>(u)[0],Barbarian::Id(u)):Invalid;}
inline void* Find(Id id){return id==Invalid?nullptr:Curses::Find(Game,Type(id),Number(id));}
inline Mask NativeMaskOf(void* u){Mask mask=0;for(unsigned n=0;n<NativeIds.size();++n)if(MonsterModifiersNative::RawHas(u,static_cast<unsigned char>(NativeIds[n])))mask|=Mask(1)<<n;return mask;}
inline void Reset(void* game){unsigned now=Ramp::Frame(game);if(Game!=game||now<LastFrame){Game=game;MonsterCaption::Clear();Effects.clear();Extras.clear();Children.clear();Fields.clear();NextScan=NextRegen=0;}LastFrame=now;}
inline Actor* Track(void* game,void* u){Reset(game);return Effects.add(Key(u),NativeMaskOf(u),Ramp::Frame(game));}
inline Mask MaskOf(void* u){auto* a=Effects.find(Key(u));return a?a->mask:NativeMaskOf(u);}
inline bool ExtraHas(void* u,unsigned char native){auto* a=Effects.find(Key(u));if(!a)return false;for(unsigned n=0;n<NativeIds.size();++n)if(NativeIds[n]==native)return (a->mask&(Mask(1)<<n))!=0;return false;}
inline bool Near(void* a,void* b,double radius){if(!a||!b||!Barbarian::SameAuraArea(a,b))return false;double x=Assassin::Coordinate(a,false)-Assassin::Coordinate(b,false),y=Assassin::Coordinate(a,true)-Assassin::Coordinate(b,true);return x*x+y*y<=radius*radius;}
// HP can remain positive after the engine switches a monster to a corpse mode.
inline bool Living(void* u){if(!AuraFeatures::Alive(u))return false;unsigned mode=static_cast<unsigned*>(u)[3];return Curses::Monster(u)?mode!=0&&mode!=12:mode!=0&&mode!=17;}
inline bool Opponent(void* u){return Living(u)&&(Curses::Player(u)||(Curses::Monster(u)&&Curses::Player(Curses::SkeletonOwner(Game,u))));}
inline void Visual(void* owner,int missile,int x,int y,unsigned frames){
 if(!owner)return;Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=owner;p.missile=missile;p.level=1;p.x=x;p.y=y;p.targetX=x+1;p.targetY=y;p.range=frames;
 if(void* m=Pulse::OnCreate(Game,&p))if(Amazon::PublishMissiles){void* list[]={m};Amazon::PublishMissiles(owner,list,1,nullptr,nullptr,0);}
}
inline void Area(Id owner,int x,int y,int damage,int element,double radius,int divisor=1){
 void* source=Find(owner);if(!source)return;std::vector<Id> targets;
 for(int type:{0,1})AuraFeatures::Units(Game,type,[&](void* u){if(!Opponent(u)||!Barbarian::SameAuraArea(source,u))return;double dx=Assassin::Coordinate(u,false)-x,dy=Assassin::Coordinate(u,true)-y;if(dx*dx+dy*dy<=radius*radius&&targets.size()<256)targets.push_back(Key(u));});
 ++Depth;
 for(Id id:targets)if(void* target=Find(id)){
  Pulse::Params p{};p.flags=0x8021;p.owner=source;p.origin=source;p.target=target;p.missile=Hit;p.level=1;p.x=Assassin::Coordinate(target,false)-1;p.y=Assassin::Coordinate(target,true);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
  if(void* m=Pulse::OnCreate(Game,&p)){
   for(int stat:Amazon::DamageStats)Ramp::SetUnit(m,stat,0,0);
   const int lo=element==1?48:element==2?54:element==3?50:element==4?52:element==5?57:21;
   int value=MissileDamage(damage,element)/divisor;
   Ramp::SetUnit(m,lo,value,0);Ramp::SetUnit(m,lo+1,value,0);if(element==5)Ramp::SetUnit(m,59,25,0);if(element==2)Ramp::SetUnit(m,56,25,0);
   const int before=Ramp::GetUnit(target,6,0),mode=static_cast<unsigned*>(m)[3];
   const int result=Curses::NativeMissileHit(Game,m,target,0);
   if(Ramp::Log){static unsigned reports=0;if(reports++<120){char line[240]{};std::snprintf(line,sizeof(line),"Monster area hit: owner=%u ownerMode=%u targetType=%u target=%u element=%d missileMode=%d damage256=%d result=%d hpBefore=%d hpAfter=%d poisonRate=%d poisonLength=%d",Number(owner),static_cast<unsigned*>(source)[3],Type(id),Number(id),element,mode,value,result,before,Ramp::GetUnit(target,6,0),Ramp::GetUnit(target,57,0),Ramp::GetUnit(target,59,0));Ramp::Log(line);}}

  }
 }
 --Depth;
}
inline bool Protected(void* target){if(!GuardianCombat::Hostile(Game,target)||static_cast<unsigned*>(target)[1]==TotemClass)return false;for(const auto& c:Children)if(c.kind==Action::Totem&&c.until>LastFrame){void* u=Find(c.id);if(Living(u)&&Near(u,target,12))return true;}return false;}
inline void SpawnChild(const Event& e){
 void* owner=Find(e.owner);void* target=Find(e.target);if(!Spawn||!owner||Children.size()>=512)return;
 int cls=e.action==Action::RuneStone?RuneClass:TotemClass;if(e.action!=Action::Totem&&e.action!=Action::RuneStone){cls=-1;for(auto& p:CloneClasses)if(unsigned(p[0])==static_cast<unsigned*>(owner)[1]){cls=p[1];break;}if(cls<0)return;}
 int x=Assassin::Coordinate(owner,false),y=Assassin::Coordinate(owner,true);void* child=nullptr;
 // Each stone occupies a separate corner. Native placement still handles walls.
 if(e.action==Action::RuneStone){constexpr int offsets[3][2]={{4,0},{-2,4},{-2,-4}};x+=offsets[std::clamp(e.damage,0,2)][0];y+=offsets[std::clamp(e.damage,0,2)][1];}
 // Let native collision and room placement choose a legal nearby subtile.
 for(auto delta:std::array<std::pair<int,int>,8>{{{3,0},{0,3},{-3,0},{0,-3},{3,3},{-3,3},{3,-3},{-3,-3}}}){
  child=Spawn(Game,SoulHarvest::Room(owner),x+delta.first,y+delta.second,cls,1,-1,0);if(child)break;
 }
 if(Ramp::Log){static unsigned reports=0;if(reports++<80){char text[160]{};std::snprintf(text,sizeof(text),"Monster helper spawn: owner=%u action=%u class=%d success=%d",Number(e.owner),unsigned(e.action),cls,child?1:0);Ramp::Log(text);}}
 if(!child)return;int hp=Ramp::GetUnit(owner,7,0);
 int percent=e.action==Action::RitualSummon?RitualLifePercent(e.damage):e.action==Action::RuneStone?5:e.action==Action::Totem?2:e.action==Action::MirrorImage?25:20;
 hp=int(std::clamp<std::int64_t>(1LL*hp*percent/100,256,0x3fffffff));
 Ramp::SetUnit(child,7,hp,0);Ramp::SetUnit(child,6,hp,0);Ramp::SetUnit(child,12,Ramp::GetUnit(owner,12,0),0);
 if(e.action!=Action::Totem&&e.action!=Action::RuneStone){for(int stat:{21,22,23,24,48,49,50,51,52,53,54,55})Ramp::SetUnit(child,stat,Ramp::GetUnit(owner,stat,0),0);if(target)Assassin::SetTarget(child,target);}
 Children.push_back({Key(child),e.owner,e.action,e.duration?LastFrame+e.duration:LastFrame+750,e.action==Action::RitualSummon?RitualDamagePercent(e.damage,MaskOf(owner)):0});
}
// Reject before the native damage call, including application of poison damage.
// Pets use their player owner's location so summons cannot bypass the duel.
inline bool IgnoreDamage(void* game,void* attacker,void* defender,void*){
 if(!game||!Curses::Monster(defender))return false;Reset(game);
 if(!(MaskOf(defender)&Bit(Duelist)))return false;
 void* actor=attacker;
 if(Curses::Monster(actor)){void* owner=Curses::SkeletonOwner(game,actor);if(Curses::Player(owner))actor=owner;}
 return !actor||!Barbarian::SameAuraArea(actor,defender)||!InsideArena(Assassin::Coordinate(actor,false)-Assassin::Coordinate(defender,false),Assassin::Coordinate(actor,true)-Assassin::Coordinate(defender,true));
}
inline bool Edible(void* owner,void* corpse){
 if(!corpse||corpse==owner||!Curses::Monster(corpse)||static_cast<unsigned*>(corpse)[3]!=12||Ramp::GetUnit(corpse,172,0)!=0||Curses::Player(Curses::SkeletonOwner(Game,corpse)))return false;
 // Private helper corpses never provide an infinite source of full heals.
 unsigned cls=static_cast<unsigned*>(corpse)[1];if(cls==TotemClass||cls==RuneClass)return false;
 for(const auto& pair:CloneClasses)if(cls==unsigned(pair[1]))return false;
 if(Curses::OriginalCheck&&Curses::OriginalCheck(corpse,Barbarian::RedeemedState))return false;
 return Near(owner,corpse,12);
}
inline bool Eat(void* owner,void* corpse){
 if(!Living(owner)||Ramp::GetUnit(owner,6,0)>=Ramp::GetUnit(owner,7,0)||!Edible(owner,corpse)||!RedemptionLife::Original)return false;
 // Skip the Redemption chance roll, but retain native corpse eligibility and
 // consumption. A failed consume must never grant the heal.
 if(!RedemptionLife::Original(Game,owner,corpse,131,1,1))return false;
 Ramp::SetUnit(owner,6,Ramp::GetUnit(owner,7,0),0);
 if(SoulHarvest::Refresh)SoulHarvest::Refresh(owner);return true;
}
inline void EatNearby(void* owner){
 if(!Living(owner)||Ramp::GetUnit(owner,6,0)>=Ramp::GetUnit(owner,7,0))return;
 std::vector<Id> corpses;AuraFeatures::Units(Game,1,[&](void* u){if(corpses.size()<128&&Edible(owner,u))corpses.push_back(Key(u));});
 for(Id id:corpses)if(Eat(owner,Find(id))){Visual(owner,Explosion,Assassin::Coordinate(owner,false),Assassin::Coordinate(owner,true),12);break;}
}
struct World:Host {
 bool get(Id id,Unit& out)override{void* u=Find(id);if(!u)return false;out={id,std::max(1,Ramp::GetUnit(u,12,0)),Assassin::Coordinate(u,false),Assassin::Coordinate(u,true),Living(u)&&GuardianCombat::Hostile(Game,u)};return true;}
 Id nearestEnemy(Id owner,int)override{void* u=Find(owner);return u?Key(MonsterModifierSpells::Target(Game,u)):Invalid;}
 unsigned liveChildren(Id owner,Action kind)override{unsigned count=0;for(auto& c:Children)if(c.owner==owner&&c.kind==kind&&c.until>LastFrame&&Living(Find(c.id)))++count;return count;}
 void emit(const Event& e)override{
  void* owner=Find(e.owner);if(!owner)return;void* target=Find(e.target);
  if(Ramp::Log){static unsigned reports=0;if(reports++<160){char text[160]{};std::snprintf(text,sizeof(text),"Monster effect: owner=%u action=%u damage=%d frame=%u",Number(e.owner),unsigned(e.action),e.damage,LastFrame);Ramp::Log(text);}}
  if(e.action<=Action::ChainLightning){unsigned index=e.action==Action::FireBall?0:e.action==Action::LightningBolt?1:e.action==Action::ChainLightning?3:e.damage==15+4*std::max(1,Ramp::GetUnit(owner,12,0))?4:2;if(target)MonsterModifierSpells::Cast(Game,owner,target,index);return;}
  ++Depth;
  if(e.action==Action::Arena){
   // Low-frequency stationary markers: visible boundary, no collision/damage.
   for(int i=0;i<24;++i){double angle=i*6.283185307179586/24;Visual(owner,ArenaMarker,e.x+int(std::lround(ArenaRadius*std::cos(angle))),e.y+int(std::lround(ArenaRadius*std::sin(angle))),e.duration);}
  }else if(e.action==Action::ConsumeCorpse)EatNearby(owner);
  else if(e.action==Action::RuneStone)SpawnChild(e);
  else if(e.action==Action::RitualSummon){
   // Retire every stone, including corpses, after the one-shot deadline.
   for(auto& child:Children)if(child.owner==e.owner&&child.kind==Action::RuneStone)child.until=LastFrame;
   if(e.damage>0){Visual(owner,GravityField,e.x,e.y,25);SpawnChild(e);}
  }else if(e.action==Action::DeathOrb)Visual(owner,FireOrb,e.x,e.y,75);
  else if(e.action==Action::FireExplosion){Visual(owner,Explosion,e.x,e.y,12);Area(e.owner,e.x,e.y,e.damage,1,9,2);}
  else if(e.action==Action::BurningGround||e.action==Action::PoisonPool){
   if(Fields.size()<1024){Fields.push_back({e.owner,e.action,e.x,e.y,e.damage,LastFrame,LastFrame+e.duration});Visual(owner,e.action==Action::PoisonPool?PoisonField:FireField,e.x,e.y,e.duration);}
  }else if(e.action==Action::GravityWarning)Visual(owner,GravityField,e.x,e.y,25);
  else if(e.action==Action::GravityPull){
   std::vector<Id> victims;for(int t:{0,1})AuraFeatures::Units(Game,t,[&](void* u){if(Opponent(u)&&Near(u,owner,GravityRadius)&&victims.size()<256)victims.push_back(Key(u));});
   for(Id id:victims)if(void* u=Find(id)){
    int x=Assassin::Coordinate(u,false),y=Assassin::Coordinate(u,true),ox=Assassin::Coordinate(owner,false),oy=Assassin::Coordinate(owner,true);
    // Pull only part-way; keep the engine's normal collision checks.
    Amazon::Relocate(Game,u,nullptr,GravityDestination(x,ox),GravityDestination(y,oy),0,0);
    Curses::Curse debuff{owner,u,0,1,75,67,-30,Assassin::GravitySlow,nullptr};Curses::OriginalApply(&debuff);
   }
  }else if(e.action==Action::GravityExplosion)Area(e.owner,e.x,e.y,e.damage,0,6);
  else if(e.action==Action::TeleportStrike){
   void* victim=nullptr;double best=900;AuraFeatures::Units(Game,0,[&](void* u){if(!Opponent(u)||!Barbarian::SameAuraArea(owner,u))return;double dx=Assassin::Coordinate(u,false)-e.x,dy=Assassin::Coordinate(u,true)-e.y,d=dx*dx+dy*dy;if(d<best){best=d;victim=u;}});
   if(victim){Amazon::Relocate(Game,owner,nullptr,Assassin::Coordinate(victim,false),Assassin::Coordinate(victim,true),0,0);Assassin::SetTarget(owner,victim);
    if(Near(owner,victim,4)){alignas(8) unsigned char info[0x80]{};Barbarian::ModeInfo(owner,4,info);std::memcpy(info+0x10,&victim,8);int vx=Assassin::Coordinate(victim,false),vy=Assassin::Coordinate(victim,true);std::memcpy(info+0x18,&vx,4);std::memcpy(info+0x1c,&vy,4);Barbarian::ModeChange(Game,info,0);int hp=Ramp::GetUnit(owner,6,0),maximum=Ramp::GetUnit(owner,7,0);Ramp::SetUnit(owner,6,std::min(maximum,hp+maximum/20),0);}
   }
  }else if(e.action>=Action::SpectralFire&&e.action<=Action::SpectralPoison){Visual(owner,Explosion,e.x,e.y,8);Area(e.owner,e.x,e.y,e.damage,1+int(e.action)-int(Action::SpectralFire),4.5);}
  else if(e.action==Action::MirrorImage||e.action==Action::PackMember||e.action==Action::Totem)SpawnChild(e);
  --Depth;
 }
};inline World WorldHost;
inline void Missing(void* game,void* unit,void* eventUnit,unsigned column){
 if(!(MaskOf(unit)&Bit(Raid))||!Callbacks)return;
 for(int id:NativeIds)if(!MonsterModifiersNative::RawHas(unit,static_cast<unsigned char>(id))){auto fn=Callbacks[id*6+column];if(fn)fn(game,eventUnit,id,1);}
}
inline void __fastcall Pick(void* game,void* unit,int champion,int,int,void* overrides){
 const auto range=RollRange(game?static_cast<unsigned char*>(game)[0x104]:0);
 OriginalPick(game,unit,champion,range.first,range.second,overrides);
}
inline int __fastcall RollNative(void* game,void* unit,unsigned char* used,void* overrides){
 // Native 0x49e470 unconditionally reads the weight-map header at +8.
 // Superunique top-ups have no overrides, but must still pass an empty map,
 // not nullptr. The native roll only reads this header and does not own it.
 std::array<std::uintptr_t,2> emptyWeights{};
 if(!overrides)overrides=emptyWeights.data();
 Reset(game);if(!unit||Depth)return OriginalRoll(game,unit,used,overrides);
 auto* actor=Track(game,unit);if(!actor)return OriginalRoll(game,unit,used,overrides);if(actor->mask&Bit(Raid))return 0;
 int vanilla=OriginalRoll(game,unit,used,overrides);Mask eligible=All&~((Mask(1)<<13)-1);eligible&=~actor->mask;
 if(!eligible)return vanilla;
 // Existing native picker still owns eligibility for existing affixes and
 // the outer native loop owns difficulty counts/champion chance.
 unsigned ticket=Random(actor->seed)% ((Count-1)*6+1);
 if(ticket<13*6&&vanilla)return vanilla;
 Mod choice=MonsterModifiers::Roll(eligible,actor->mask,actor->seed);if(choice==Count)return vanilla;
 actor->mask=Expand(actor->mask|Bit(choice));
 if(Ramp::Log){static unsigned logged=0;if(logged++<64){char line[128]{};std::snprintf(line,sizeof(line),"Monster custom modifier: unit=%u class=%u mod=%u mask=%x",Barbarian::Id(unit),static_cast<unsigned*>(unit)[1],unsigned(choice),actor->mask);Ramp::Log(line);}}
 // Native ID 3 has no initializer or callbacks. Never index outside its
 // fixed callback table; custom mechanics stay in the typed sidecar.
 return 3;
}
inline void __fastcall TeleportInit(void* u,int id,int unique){if(id==26&&unique&&!(MonsterModifiersNative::Rank(u)&4))return;OriginalTeleport(u,id,unique);}
inline void __fastcall FireMode(void* game,void* u,int id,int unique){if(id==9&&unique&&!(MonsterModifiersNative::Rank(u)&4)){Track(game,u);if(!Living(u))Effects.death(WorldHost,Key(u),Ramp::Frame(game));return;}OriginalFireMode(game,u,id,unique);}
inline void __fastcall FireExplosionNative(void* game,void* u,int id,int unique){if(id==9&&unique&&!(MonsterModifiersNative::Rank(u)&4)){Track(game,u);Effects.death(WorldHost,Key(u),Ramp::Frame(game));return;}OriginalFireExplosion(game,u,id,unique);}
inline void __fastcall Multi(void* game,void* missile,int id,int unique){OriginalMulti(game,missile,id,unique);if(!unique||Depth)return;Reset(game);++WideVolley;OriginalMulti(game,missile,id,unique);--WideVolley;}
inline bool Projectile(void*,void* raw){if(!WideVolley)return true;auto* p=static_cast<Pulse::Params*>(raw);if(!p||!Curses::Monster(p->owner))return true;void* target=Assassin::GetTarget(Game,p->owner);if(!target)return true;
 // The second native pair uses twice the lateral spread of the first pair.
 int tx=Assassin::Coordinate(target,false),ty=Assassin::Coordinate(target,true);p->targetX=tx+2*(p->targetX-tx);p->targetY=ty+2*(p->targetY-ty);return true;}
inline void __fastcall Start(void* game,void* u){OriginalStart(game,u);Reset(game);Missing(game,u,u,0);}
inline void __fastcall Success(void* game,void* u){OriginalSuccess(game,u);Reset(game);Missing(game,u,u,3);}
inline void __fastcall Hurt(void* game,void* u){OriginalHurt(game,u);Reset(game);Missing(game,u,u,4);if((MaskOf(u)&Bit(Raid))&&!MonsterModifiersNative::RawHas(u,17)&&static_cast<unsigned*>(u)[3]==3){auto fn=Callbacks[17*6+2];if(fn)fn(game,u,17,1);}}
inline void __fastcall Mode(void* game,void* u){OriginalMode(game,u);Reset(game);bool first=Effects.find(Key(u))&&!Effects.find(Key(u))->died;Missing(game,u,u,1);if(!Living(u)){if(Effects.find(Key(u)))Effects.death(WorldHost,Key(u),Ramp::Frame(game));if(first&&(MaskOf(u)&Bit(Raid))&&!MonsterModifiersNative::RawHas(u,18)){auto fn=Callbacks[18*6+2];if(fn)fn(game,u,18,1);}}}
inline void __fastcall Missile(void* game,void* u,void* missile){OriginalMissile(game,u,missile);Reset(game);Missing(game,u,missile,5);}
inline std::uintptr_t __fastcall Drop(void* game,void* unit,void* killer,unsigned short tc,std::uintptr_t a5,std::uintptr_t a6,std::uintptr_t a7,std::uintptr_t a8,std::uintptr_t a9,std::uintptr_t a10){
 Reset(game);const int goldScale=!DropDepth&&killer&&(Curses::Player(killer)||Curses::Monster(killer))&&GoldLootScale?GoldLootScale(game,unit):1;
 const int oldGold=goldScale>1?Ramp::GetUnit(killer,79,0):0;
 if(goldScale>1)Ramp::SetUnit(killer,79,int(std::min<std::int64_t>(10000000,(std::int64_t{100}+std::max(0,oldGold))*goldScale-100)),0);
 const int rolls=DropDepth||a8||a9||!Curses::Monster(unit)?1:LootRolls(MaskOf(unit))+(AdditionalLootRolls?AdditionalLootRolls(game,unit):0);++DropDepth;
 auto result=OriginalDrop(game,unit,killer,tc,a5,a6,a7,a8,a9,a10);for(int n=1;n<rolls;++n)OriginalDrop(game,unit,killer,tc,a5,a6,a7,a8,a9,a10);--DropDepth;if(goldScale>1)Ramp::SetUnit(killer,79,oldGold,0);return result;
}
inline void Damage(void* game,void* attacker,void* defender,void* packet){
 if(!game||!packet)return;Reset(game);
 if(Curses::Monster(attacker)&&Ramp::GetUnit(attacker,172,0)==0&&!Curses::Player(Curses::SkeletonOwner(game,attacker))&&!(MonsterModifiersNative::Rank(attacker)&4))AuraFeatures::ScaleResolvedDamage(packet,DamagePercent(MaskOf(attacker)),100);
 if(Protected(defender))AuraFeatures::ScaleResolvedDamage(packet,1,10);
 for(auto& child:Children)if(child.id==Key(attacker)&&child.kind==Action::MirrorImage){AuraFeatures::ScaleResolvedDamage(packet,35,100);break;}
 for(const auto& child:Children)if(child.id==Key(attacker)&&child.kind==Action::RitualSummon){AuraFeatures::ScaleResolvedDamage(packet,child.strength,100);break;}
 bool pack=false;for(const auto& child:Children)if(child.id==Key(attacker)&&child.kind==Action::PackMember){pack=true;break;}
 if(Curses::Monster(attacker)&&GuardianCombat::Hostile(game,attacker)&&((MonsterModifiersNative::Rank(attacker)&16)||pack))for(auto& actor:Effects.actors)if((actor.mask&Bit(Leader))&&!actor.died&&Near(Find(actor.id),attacker,12)){AuraFeatures::ScaleResolvedDamage(packet,130,100);break;}
}
inline void After(void* game,void* attacker,void* defender,void* packet){
 if(Depth||!game||!packet)return;Reset(game);
 if(Curses::Monster(defender)&&(MonsterModifiersNative::Rank(defender)&8)&&!(MonsterModifiersNative::Rank(defender)&4)&&!Living(defender)){Track(game,defender);Effects.death(WorldHost,Key(defender),Ramp::Frame(game));}
 if(!GuardianCombat::Hostile(game,attacker))return;if(!(MonsterModifiersNative::Rank(attacker)&8)||(MonsterModifiersNative::Rank(attacker)&4))return;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);if(!(flags&1)||(flags&32))return;
 Track(game,attacker);Effects.hit(WorldHost,Key(attacker),Key(defender),Ramp::Frame(game),true);
}
// Scale the already initialized native life once, preserving the wounded fraction.
// Repeated frame scans must never multiply life again.
inline void ApplyLife(void* unit,Extra& extra){
 const int percent=LifePercent(MaskOf(unit));if(percent==extra.lifePercent)return;
 const int maximum=Ramp::GetUnit(unit,7,0),current=Ramp::GetUnit(unit,6,0);
 const int changed=int(std::clamp<std::int64_t>(std::int64_t(maximum)*percent/extra.lifePercent,256,0x3fffffff));
 Ramp::SetUnit(unit,7,changed,0);Ramp::SetUnit(unit,6,maximum>0?int(std::clamp<std::int64_t>(std::int64_t(current)*changed/maximum,0,changed)):changed,0);extra.lifePercent=percent;
}
inline void CompleteSuperunique(void* unit,Extra& extra){
 if(extra.counted)return;extra.counted=true;if(!(MonsterModifiersNative::Rank(unit)&2))return;
 auto* actor=Effects.find(Key(unit));if(!actor)return;auto range=RollRange(static_cast<unsigned char*>(Game)[0x104]);
 unsigned desired=range.first+Random(actor->seed)%(range.second-range.first+1);
 void* data=nullptr;std::memcpy(&data,static_cast<unsigned char*>(unit)+0x10,8);if(!data)return;auto* mods=static_cast<unsigned char*>(data)+0x20;
 // Named bosses retain their fixed affixes even when those exceed the normal range.
 for(unsigned attempts=0;ModifierCount(MaskOf(unit))<desired&&attempts<20;++attempts){unsigned slot=0;unsigned char used[256]{};for(;slot<9&&mods[slot];++slot)used[mods[slot]]=1;if(slot==9)break;
  int id=RollNative(Game,unit,used,nullptr);if(id<=0||id>=256)break;mods[slot]=static_cast<unsigned char>(id);Init(unit,id,1);Track(Game,unit);
 }
}
inline void Frame(void* game){
 Reset(game);const unsigned now=Ramp::Frame(game);if(Depth||now<NextScan)return;NextScan=now+5;
 AuraFeatures::Units(game,1,[&](void* unit){unsigned rank=MonsterModifiersNative::Rank(unit);if(!(rank&8)||(rank&4))return;auto* a=Track(game,unit);if(!a)return;
  if(!Living(unit)){Effects.death(WorldHost,Key(unit),now);return;}
  if(!GuardianCombat::Hostile(game,unit))return;
  Extra* extra=nullptr;for(auto& x:Extras)if(x.id==Key(unit)){extra=&x;break;}if(!extra){Extras.push_back({Key(unit),0});extra=&Extras.back();}
  CompleteSuperunique(unit,*extra);
  if(MaskOf(unit)&Bit(Raid)){
   for(unsigned n=0;n<NativeIds.size();++n)if(!(extra->initialized&(Mask(1)<<n))){extra->initialized|=Mask(1)<<n;if(!MonsterModifiersNative::RawHas(unit,NativeIds[n]))Init(unit,NativeIds[n],1);}
  }
  ApplyLife(unit,*extra);
  MonsterCaption::Publish(unit,MaskOf(unit));
 });
 Effects.tick(WorldHost,now);
 for(auto& field:Fields)if(now<field.end&&now>=field.next){field.next=now+25;Area(field.owner,field.x,field.y,field.damage,field.kind==Action::PoisonPool?5:1,3,field.kind==Action::BurningGround?2:1);}
 Fields.erase(std::remove_if(Fields.begin(),Fields.end(),[&](const Field& f){return now>=f.end;}),Fields.end());
 for(auto& child:Children){void* unit=Find(child.id);if(!unit){child.id=Invalid;continue;}if(now>=child.until||(child.kind==Action::RuneStone&&!Living(Find(child.owner)))){if(void* room=SoulHarvest::Room(unit))Barbarian::RoomDelete(room,1,Number(child.id));Barbarian::DeleteUnit(game,unit);child.id=Invalid;}}
 Children.erase(std::remove_if(Children.begin(),Children.end(),[](const Child& c){return c.id==Invalid;}),Children.end());
 if(now>=NextRegen){NextRegen=now+25;AuraFeatures::Units(game,1,[&](void* u){if(Protected(u)){int maximum=Ramp::GetUnit(u,7,0);Ramp::SetUnit(u,6,std::min(maximum,Ramp::GetUnit(u,6,0)+maximum/10),0);}});}
}
#include "runtime_native.inc"
}
