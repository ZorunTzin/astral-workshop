#pragma once
// Bounded timers retain identities, never native pointers across server frames.
namespace Druid {
#include "druid_bindings.h"
constexpr unsigned Invalid=0xffffffffU;
inline AuraBuffs::SkillFn NativeRaven=nullptr;
inline void* Game=nullptr;inline unsigned Last=0,Next=0,Depth=0;
struct Player {unsigned id=Invalid,raven=0,visual=0;int hits=0;};
struct Zone {unsigned owner=Invalid,next=0,end=0;int x=0,y=0,tx=0,ty=0,level=1,area=0;};
struct Seed {unsigned owner=Invalid,id=Invalid,end=0;int x=0,y=0,level=1,area=0;};
struct Mark {unsigned target=Invalid,owner=Invalid,until=0;int level=0;};
struct Vine {unsigned id=Invalid,next=0;};
inline std::array<Player,16> Players{};inline std::array<Zone,64> Zones{};
inline std::array<Seed,256> Seeds{};inline std::array<Mark,512> Marks{};inline std::array<Vine,128> Vines{};
inline bool Due(unsigned now,unsigned& next,unsigned interval){if(now<next)return false;next=now+interval;return true;}
inline bool FifthHit(int& hits){if(++hits<5)return false;hits=0;return true;}
inline bool WinterTick(unsigned now,unsigned& next,unsigned end){if(now>=end||now<next)return false;next+=25;return true;}
inline void Reset(void* g){unsigned now=Ramp::Frame(g);if(Game!=g||now<Last){Game=g;Players={};Zones={};Seeds={};Marks={};Vines={};Next=0;Depth=0;}Last=now;}
inline int X(void* u){return Assassin::Coordinate(u,false);}inline int Y(void* u){return Assassin::Coordinate(u,true);}
inline Player* Track(void* u){unsigned id=Barbarian::Id(u);for(auto& p:Players)if(p.id==id)return &p;for(auto& p:Players)if(p.id==Invalid){p={};p.id=id;return &p;}return nullptr;}
inline int Level(void* g,void* u,int helper){return std::clamp(Curses::EvalCalc4(g,u,helper,1,0),0,1000);}
inline int Active(void* u,int sid){int i=AuraBuffs::Index(sid);if(i<0)return 0;for(auto& p:AuraBuffs::Players)if(p.id==Barbarian::Id(u))return p.costs[i]?p.levels[i]:0;return 0;}
inline bool Near(void* a,void* b,int radius){if(!a||!b||!Barbarian::SameAuraArea(a,b))return false;auto x=std::int64_t{X(a)}-X(b),y=std::int64_t{Y(a)}-Y(b);return x*x+y*y<=radius*radius;}
inline void* List(void* g,void* source,void* target,int state,int frames=75,int skillLevel=1){
 void* list=Ramp::OriginalList(target,state);if(!list){const int skill=state==PrimalState?239:state==AlphaState||state==CompanionState?224:state==VineState?231:state==SolarState?241:221;Curses::Curse r{source,target,skill,skillLevel,frames,-1,0,state,nullptr};list=Curses::OriginalApply(&r);}
 if(list){int end=int(Ramp::Frame(g))+frames;Ramp::SetExpiry(list,end);Ramp::Schedule(g,target,12,end,0,0,0);}return list;
}
inline void* Visual(void* g,void* u,int missile,int x,int y,int tx,int ty,int frames){
 Pulse::Params p{};p.flags=0x8021;p.owner=u;p.origin=u;p.missile=missile;p.level=1;p.x=x;p.y=y;p.targetX=tx;p.targetY=ty;p.range=frames;
 void* m=Pulse::OnCreate(g,&p);if(m&&Amazon::PublishMissiles){void* list[]={m};Amazon::PublishMissiles(u,list,1,nullptr,nullptr,0);}return m;
}
inline void Deal(void* g,void* u,void* target,int low,int high,int element){
 if(!GuardianCombat::Hostile(g,target)||!Barbarian::SameAuraArea(u,target))return;
 Pulse::Params p{};p.flags=0x8021;p.owner=u;p.origin=u;p.target=target;p.missile=HitMissile;p.level=1;p.x=X(target)-1;p.y=Y(target);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
 if(void* m=Pulse::OnCreate(g,&p)){
  for(int s:{21,22,48,49,50,51,52,53,54,55,57,58})Ramp::SetUnit(m,s,0,0);
  const int stat=element==1?48:element==2?54:21;
  int bonus=element==1?Ramp::GetUnit(u,329,0):element==2?Ramp::GetUnit(u,331,0):0;
  Ramp::SetUnit(m,stat,int(std::min<std::int64_t>(0x3fffffff,std::int64_t{low}*256*(100+std::max(0,bonus))/100)),0);
  Ramp::SetUnit(m,stat+1,int(std::min<std::int64_t>(0x3fffffff,std::int64_t{high}*256*(100+std::max(0,bonus))/100)),0);
  if(element==2)Ramp::SetUnit(m,56,50,0);
  ++Depth;++Curses::ProcDepth;Curses::NativeMissileHit(g,m,target,0);--Curses::ProcDepth;--Depth;
 }
}
// calc4 contains the same hard-point synergy (and physical aura modifier) as the UI.
inline int SpellDamage(void* g,void* owner,int skill,int level,int base){int scale=Curses::EvalCalc4(g,owner,skill,level,100);return int(std::clamp<std::int64_t>(std::int64_t{base}*scale/100,0,0x3fffff));}
inline void Area(void* g,void* u,int x,int y,int radius,int lo,int hi,int element,bool slowed=false){
 AuraFeatures::Units(g,1,[&](void* target){if(!GuardianCombat::Hostile(g,target)||!Barbarian::SameAuraArea(u,target))return;auto dx=std::int64_t{X(target)}-x,dy=std::int64_t{Y(target)}-y;if(dx*dx+dy*dy>radius*radius)return;
 bool slow=slowed&&(Ramp::OriginalList(target,SlowState)||Ramp::GetUnit(target,67,0)<0);Deal(g,u,target,slow?lo*3/2:lo,slow?hi*3/2:hi,element);});
}
inline bool BuffPulse(void* g,void* u,unsigned i,int l){
 int sid=AuraBuffs::Definitions[i].skill;if(sid!=221&&sid!=224&&sid!=239)return false;Reset(g);Track(u);
 int state=AuraBuffs::Definitions[i].state;void* list=Ramp::OriginalList(u,state);const bool fresh=!list;
 if(sid==221&&!list&&Sorceress::NativeMorph){
  // Use native removal callbacks for both entering and leaving a morph.
  if(Ramp::OriginalList(u,WolfState))Sorceress::NativeMorph(g,u,223,1);
  if(Ramp::OriginalList(u,BearState))Sorceress::NativeMorph(g,u,228,1);
  Sorceress::NativeMorph(g,u,Morph,l);list=Ramp::OriginalList(u,state);
 }
 if(!list)list=List(g,u,u,state,75,l);if(!list)return false;
 const unsigned now=Ramp::Frame(g),expiry=std::max(now+75,AuraBuffs::Next(g,u,i,l)+25);Ramp::SetExpiry(list,int(expiry));Ramp::Schedule(g,u,12,int(expiry),0,0,0);
 if(sid==239){for(auto [stat,value]:{std::pair<int,int>{25,20+5*l},{68,10+l},{119,50+10*l},{171,30+10*l},{39,10+l},{41,10+l},{43,10+l},{45,10+l}})Curses::Write(g,list,stat,value);}
 if(sid==221){for(auto [stat,value]:{std::pair<int,int>{PhysicalSpellStat,15+3*l},{329,15+3*l},{331,15+3*l},{105,10+l},{76,20+2*l},{99,15+2*l},{40,l/10},{42,l/10},{44,l/10},{46,l/10}})Curses::Write(g,list,stat,value);// Renewing stats must not restart the golem walk animation.
  if(fresh)Sorceress::Appearance(u,state,true);}
 if(SoulHarvest::Refresh)SoulHarvest::Refresh(u);return true;
}
inline void Disable(void* g,void* u,unsigned i){int sid=AuraBuffs::Definitions[i].skill;if(sid==221){if(Ramp::OriginalList(u,AvatarState)&&Sorceress::NativeMorph)Sorceress::NativeMorph(g,u,Morph,std::max(1,AuraBuffs::Level(g,u,i)));Sorceress::Appearance(u,AvatarState,false);}if(sid==239)if(auto* p=Track(u))p->hits=0;}
inline int Cast(void* g,void* u,int sid,int level){
 if(sid!=230&&sid!=240)return -1;if(!Curses::Player(u))return 0;Reset(g);level=std::clamp(level,1,1000);const int mana=(9+level)*256;if(Ramp::GetUnit(u,8,0)<mana)return 0;
 int tx=Curses::X(u),ty=Curses::Y(u);if(void* t=Assassin::GetTarget?Assassin::GetTarget(g,u):nullptr){tx=X(t);ty=Y(t);}const int x=X(u),y=Y(u);if(tx==x&&ty==y)tx=x+10;
 bool created=false;double angle=std::atan2(double(ty-y),double(tx-x));
 if(sid==230){for(auto& z:Zones)if(z.owner==Invalid||Last>=z.end){z={Barbarian::Id(u),Last,Last+25*(3+level/10),x,y,tx,ty,level,Barbarian::AuraArea?Barbarian::AuraArea(u):0};created=true;break;}
  if(created)for(int n=-3;n<=3;++n){double a=angle+n*.16;Visual(g,u,IceVisual,x,y,x+int((12+3*(level/5)/2)*std::cos(a)),y+int((12+3*(level/5)/2)*std::sin(a)),12+3*(level/5)/2);}
 }else{for(int n=0,count=std::min(103,3+level/10);n<count;++n)for(auto& s:Seeds)if(s.owner==Invalid){double a=angle+(n-(count-1)*.5)*.20;void* m=Visual(g,u,SeedMissile,x,y,x+int(30*std::cos(a)),y+int(30*std::sin(a)),25);if(m){s={Barbarian::Id(u),Barbarian::Id(m),Last+25,x,y,level,Barbarian::AuraArea?Barbarian::AuraArea(u):0};created=true;}break;}}
 if(created){Ramp::SetUnit(u,8,Ramp::GetUnit(u,8,0)-mana,0);if(SoulHarvest::Refresh)SoulHarvest::Refresh(u);}return created?1:0;
}
inline void Hit(void* g,void* a,void* d,void* damage){
 if(Depth||!a||!d||!damage)return;Reset(g);unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(damage)+4,2);if(!(flags&1)||(flags&0x4020))return;
 if(Curses::Player(a)&&GuardianCombat::Hostile(g,d)){
  const int lvl=Active(a,239);const int used=Barbarian::UsedSkill(a);
  // Only weapon melee packets count. Spell/projectile recursion cannot charge fury.
  if(lvl>0&&!(Barbarian::ReadDamage(damage,0)&2)&&used!=230&&used!=240){
   if(auto* p=Track(a))if(FifthHit(p->hits)){Visual(g,a,FireVisual,X(d),Y(d),X(d)+1,Y(d),10);Area(g,a,X(d),Y(d),5,30+15*lvl,60+25*lvl,1);}
  }
 }
 if(!Curses::Monster(a))return;void* owner=Curses::SkeletonOwner(g,a);if(!Curses::Player(owner))return;
 if(int alpha=Active(owner,224))AuraFeatures::ScaleResolvedDamage(damage,115+3*(alpha-1),100);
 if(static_cast<unsigned*>(a)[1]==PoisonClass){int l=Level(g,owner,Level222);for(auto& m:Marks)if((m.target==Barbarian::Id(d)&&m.owner==Barbarian::Id(owner))||m.target==Invalid||Last>=m.until){m={Barbarian::Id(d),Barbarian::Id(owner),Last+100,l};break;}}
 for(auto& m:Marks)if(m.target==Barbarian::Id(d)&&m.owner==Barbarian::Id(owner)&&Last<m.until){AuraFeatures::ScaleResolvedDamage(damage,115+m.level,100);break;}
}
inline void Projectile(void*,void* raw,void* missile)noexcept{
 auto* p=static_cast<Pulse::Params*>(raw);if(!p||!missile||!Curses::Player(p->owner))return;int lvl=Active(p->owner,221);if(!lvl)return;
 // Only the physical components of Druid elemental spells; never weapon damage.
 if(p->skill!=229&&p->skill!=240&&p->skill!=244&&p->skill!=245&&p->skill!=249)return;
 for(int stat:{21,22}){int value=Ramp::GetUnit(missile,stat,0);Ramp::SetUnit(missile,stat,int(std::min<std::int64_t>(0x3fffffff,std::int64_t{value}*(115+3*lvl)/100)),0);}
}
inline bool Cone(const Zone& z,int x,int y){double dx=x-z.x,dy=y-z.y,ax=z.tx-z.x,ay=z.ty-z.y,dist=dx*dx+dy*dy,dot=dx*ax+dy*ay;double radius=12+3*(z.level/5)/2;return dist<=radius*radius&&(dist<1||(dot>0&&dot*dot>=dist*(ax*ax+ay*ay)*.60));}
inline void Heal(void* u,int percent){int max=Ramp::GetUnit(u,7,0),hp=Ramp::GetUnit(u,6,0);if(hp>0){Ramp::SetUnit(u,6,int(std::min<std::int64_t>(max,std::int64_t{hp}+std::int64_t{max}*percent/100)),0);if(SoulHarvest::Refresh)SoulHarvest::Refresh(u);}}
inline void VineBuff(void* g,void* owner,int level,bool solar){
 if(solar){int max=Ramp::GetUnit(owner,9,0),mp=Ramp::GetUnit(owner,8,0);Ramp::SetUnit(owner,8,int(std::min<std::int64_t>(max,std::int64_t{mp}+std::int64_t{max}*(3+level/2)/100)),0);}else Heal(owner,3+level/2);
 for(int type:{0,1})AuraFeatures::Units(g,type,[&](void* u){if((u!=owner&&Curses::SkeletonOwner(g,u)!=owner)||!AuraFeatures::Alive(u)||!Near(owner,u,30))return;if(!solar&&u==owner)return;
  if(void* list=List(g,owner,u,solar?SolarState:VineState,125)){
   if(solar)for(int stat:{67,68,105})Curses::Write(g,list,stat,5+level);
   else{Heal(u,3+level/2);Curses::Write(g,list,74,int(std::int64_t{Ramp::GetUnit(u,7,0)}*(2+level/10)/2500));}
  }
 });
}
inline void Companions(void* g,void* owner){
 int alpha=Active(owner,224);
 AuraFeatures::Units(g,1,[&](void* pet){if(!AuraFeatures::Alive(pet)||Curses::SkeletonOwner(g,pet)!=owner||!Near(owner,pet,40))return;
  int cls=static_cast<unsigned*>(pet)[1];if(alpha)if(void* list=List(g,owner,pet,CompanionState)){Curses::Write(g,list,68,5+alpha-1);Curses::Write(g,list,67,10+2*(alpha-1));}
  if(cls!=CarrionClass&&cls!=SolarClass)return;
  Vine* timer=nullptr;for(auto& v:Vines)if(v.id==Barbarian::Id(pet)){timer=&v;break;}if(!timer)for(auto& v:Vines)if(v.id==Invalid||!Curses::Find(g,1,v.id)){v={Barbarian::Id(pet),0};timer=&v;break;}if(!timer||Last<timer->next)return;timer->next=Last+75;
  void* corpse=nullptr;AuraFeatures::Units(g,1,[&](void* c){if(corpse||static_cast<unsigned*>(c)[3]!=12||Ramp::GetUnit(c,172,0)||Curses::Player(Curses::SkeletonOwner(g,c))||!Near(pet,c,12))return;if(Curses::OriginalCheck&&Curses::OriginalCheck(c,Barbarian::RedeemedState))return;corpse=c;});
  if(corpse&&RedemptionLife::Original&&RedemptionLife::Original(g,pet,corpse,131,1,1)){int lvl=std::max(1,Level(g,owner,cls==CarrionClass?Level231:Level241));VineBuff(g,owner,lvl,cls==SolarClass);}
 });
}
inline void Frame(void* g){
 if(!g)return;Reset(g);
 for(auto& s:Seeds)if(s.owner!=Invalid){void* owner=Curses::Find(g,0,s.owner);if(!AuraFeatures::Alive(owner)||(s.area&&Barbarian::AuraArea(owner)!=s.area)){s={};continue;}
  void* missile=Curses::Find(g,3,s.id);if(missile){s.x=X(missile);s.y=Y(missile);}bool impact=false;
  AuraFeatures::Units(g,1,[&](void* t){if(impact||!GuardianCombat::Hostile(g,t)||!Barbarian::SameAuraArea(owner,t))return;int dx=X(t)-s.x,dy=Y(t)-s.y;if(dx*dx+dy*dy<=9)impact=true;});
  if(impact||!missile||Last>=s.end){if(impact){Area(g,owner,s.x,s.y,5,SpellDamage(g,owner,240,s.level,20+10*(s.level-1)),SpellDamage(g,owner,240,s.level,30+10*(s.level-1)),0,true);Visual(g,owner,AvatarVisual,s.x,s.y,s.x+1,s.y,10);if(missile)Curses::NativeMissileHit(g,missile,nullptr,0);}s={};}
 }
 if(Last<Next)return;Next=Last+5;
 for(auto& z:Zones)if(z.owner!=Invalid){if(Last>=z.end){z={};continue;}void* owner=Curses::Find(g,0,z.owner);if(!AuraFeatures::Alive(owner)||(z.area&&Barbarian::AuraArea(owner)!=z.area)){z={};continue;}if(!WinterTick(Last,z.next,z.end))continue;
  AuraFeatures::Units(g,1,[&](void* t){if(!GuardianCombat::Hostile(g,t)||!Barbarian::SameAuraArea(owner,t)||!Cone(z,X(t),Y(t)))return;Deal(g,owner,t,SpellDamage(g,owner,230,z.level,15+8*(z.level-1)),SpellDamage(g,owner,230,z.level,25+8*(z.level-1)),2);if(void* list=List(g,owner,t,SlowState,40)){Curses::Write(g,list,67,-35);Curses::Write(g,list,68,-35);}});
  for(int n=0;n<5;++n){double a=std::atan2(double(z.ty-z.y),double(z.tx-z.x))+(n-2)*.20;int r=(4+(n%3)*3)*(12+3*(z.level/5)/2)/12;int x=z.x+int(r*std::cos(a)),y=z.y+int(r*std::sin(a));Visual(g,owner,IceGround,x,y,x+1,y,30);}
 }
 AuraFeatures::Units(g,0,[&](void* u){if(!AuraFeatures::Alive(u))return;auto* p=Track(u);if(!p)return;int level=Active(u,221);
  if(level&&!Ramp::OriginalList(u,AvatarState)){int i=AuraBuffs::Index(221);if(auto* ap=AuraBuffs::Track(g,u)){AuraBuffs::Disable(g,u,*ap,i);AuraBuffs::SyncReservation(g,u,*ap);}level=0;}
  if(int ravenLevel=Active(u,224)){level=ravenLevel;if(Due(Last,p->raven,125)){++Depth;if(NativeRaven)NativeRaven(g,u,Raven,level);--Depth;}}
  else{p->raven=Last+125;p->visual=0;}Companions(g,u);
 });
}
} // namespace Druid
