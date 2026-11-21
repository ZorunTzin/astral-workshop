#pragma once
// Shared Assassin scheduler used by the native adapter and deterministic tests.
// Coordinates and radii are yards; time is the native 25 Hz server frame.
// A future adapter resolves typed unit IDs afresh, validates room/session/hostility,
// creates visuals and dispatches actual native hit/proc events. No retained pointers.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>
namespace AssassinPreview {
using Id=std::uint32_t;
using Tick=std::uint64_t;
constexpr Id Invalid=std::numeric_limits<Id>::max();
constexpr int Fps=25;
enum class Skill {Expose,Omnislash,Queen,ATMG,Spikes,Afterimage,DeathMark,Gravity,Overclock,Count};
struct Point {double x=0,y=0;};
struct Unit {Id id=Invalid;Point pos;std::int64_t hp=0,maxHp=0;};
inline int Level(int l){return std::clamp(l,0,1000);}
inline int Weapon(Skill s,int l){l=Level(l);switch(s){case Skill::Omnislash:return 200+25*l;case Skill::Queen:return 30+5*l;case Skill::ATMG:return 10+l;case Skill::Spikes:return 25+5*l;case Skill::Afterimage:return 30+3*l;default:return 0;}}
inline int Accuracy(Skill s,int l){return s==Skill::Omnislash?200+20*Level(l):100+10*Level(l);}
inline int Reservation(int l,int efficiency){return std::max(1,int((15LL+3LL*Level(l))*100/(100LL+std::max(0,efficiency))));}
inline int CritBonus(int l){return l>0?10+Level(l):0;}
inline bool ForcedCritical(int l,const Unit& u){return l>0&&u.hp>0&&u.hp>=u.maxHp;}
inline double Distance(Point a,Point b){return std::hypot(a.x-b.x,a.y-b.y);}
struct Hit {
 Skill skill;Id owner,target;int weaponPercent=0,attackRating=0;std::int64_t flatPhysical=0;
 bool allowOnHit=true,forceCritical=false;int criticalBonus=0;
 // This lineage tag MUST survive native callbacks so generated attacks cannot recurse.
 bool secondary=true,primary=true;
};
struct Result {bool landed=false;std::int64_t damage=0;};
struct Host {
 virtual ~Host()=default;
 virtual bool alive(Id owner)=0;
 virtual Point position(Id owner)=0;
 virtual std::vector<Unit> targets(Id owner,Point center,double radius)=0;
 virtual bool find(Id owner,Id target,Unit& out)=0;
 virtual bool clone(Id owner,Id target,int level) {return false;}
 virtual bool jump(Id owner,Id target)=0; // Reject walls/invalid rooms, never raw teleport.
 virtual Result hit(const Hit& request)=0; // Native AR, block, resist, critical and proc dispatch.
 virtual bool reserve(Id owner,int totalMana)=0; // Atomic budget check and transient reservation.
 virtual void pullSlow(Id owner,Id target,Point center,int slowPercent,Tick until)=0;
 virtual void visual(Skill skill,Id owner,Point center,Tick duration)=0;
};
struct Buff {Id owner;Skill skill;int level;Tick serial,next,drop;Point last;};
struct Echo {Id owner,target;int level,ar;Tick due;};
struct Mark {Id owner,target;int level;Tick until;std::int64_t damage=0;Point center;};
struct Slash {Id owner;int level,left;Tick next,until;unsigned cursor=0;};
struct Cooldown {Id owner;Tick until;};
struct Trap {Id owner;Skill skill;int level,shots=80,credit=0;Point center;Tick born,last,until,next;};
struct Patch {Id owner;int level;Point center;Tick next,until;};
class Engine {
 Host& h;std::uint64_t session=0;Tick last=0,serial=0;unsigned random=0x29ad47;
 std::vector<Buff> buffs;std::vector<Echo> echoes;std::vector<Mark> marks;
 std::vector<Slash> slashes;std::vector<Cooldown> cooldowns;std::vector<Trap> traps;std::vector<Patch> patches;
 bool dispatching=false;
 int lvl(Id o,Skill s)const{for(auto& b:buffs)if(b.owner==o&&b.skill==s)return b.level;return 0;}
 int total(Id o,int efficiency)const{int n=0;for(auto& b:buffs)if(b.owner==o)n+=Reservation(b.level,efficiency);return n;}
 std::vector<Unit> targets(Id o,Point c,double r){auto v=h.targets(o,c,r);v.erase(std::remove_if(v.begin(),v.end(),[&](auto& u){return u.hp<=0||Distance(u.pos,c)>r;}),v.end());return v;}
 void store(Id o,Id t,std::int64_t damage){for(auto& m:marks)if(m.owner==o&&m.target==t)m.damage+=std::clamp<std::int64_t>(damage,0,std::numeric_limits<int>::max()-m.damage);}
 Result strike(Skill s,Id o,Id t,int l,int ar=-1,std::int64_t flat=0,bool primary=true){
  if(dispatching||!h.alive(o))return {};
  Unit u;if(!h.find(o,t,u)||u.hp<=0)return {};
  Hit req{s,o,t,Weapon(s,l),ar<0?Accuracy(s,l):ar,flat};req.primary=primary;
  req.forceCritical=ForcedCritical(expose(o),u);req.criticalBonus=CritBonus(expose(o));
  // The host may dispatch item procs, but callbacks into this engine are suppressed.
  struct Guard {bool& value;Guard(bool& v):value(v){value=true;}~Guard(){value=false;}} guard(dispatching);
  auto result=h.hit(req);
  if(result.landed&&s!=Skill::DeathMark)store(o,t,result.damage);
  return result;
 }
 std::vector<std::pair<Id,int>> passives;
 int expose(Id o)const{for(auto& p:passives)if(p.first==o)return p.second;return 0;}
 void explode(Mark m){if(!h.alive(m.owner)||m.damage<=0)return;h.visual(Skill::DeathMark,m.owner,m.center,12);for(auto& u:targets(m.owner,m.center,6))strike(Skill::DeathMark,m.owner,u.id,m.level,0,m.damage);}
 template<class T,class F>static void remove(std::vector<T>& v,F f){v.erase(std::remove_if(v.begin(),v.end(),f),v.end());}
public:
 explicit Engine(Host& host):h(host){}
 void reset(std::uint64_t newSession){
  for(auto& b:buffs)h.reserve(b.owner,0);
  buffs.clear();echoes.clear();marks.clear();slashes.clear();cooldowns.clear();traps.clear();patches.clear();passives.clear();session=newSession;last=0;serial=0;dispatching=false;
 }
 void forget(Id o){
  remove(buffs,[&](auto& x){return x.owner==o;});remove(echoes,[&](auto& x){return x.owner==o;});
  remove(marks,[&](auto& x){return x.owner==o;});remove(slashes,[&](auto& x){return x.owner==o;});
  remove(traps,[&](auto& x){return x.owner==o;});remove(patches,[&](auto& x){return x.owner==o;});
 }
 void setExpose(Id o,int l){remove(passives,[&](auto& p){return p.first==o;});if(l>0)passives.push_back({o,Level(l)});}
 bool toggle(Id o,Skill s,int l,Tick now,int efficiency=0){
  if(!h.alive(o)||l<=0||!(s==Skill::Queen||s==Skill::Spikes||s==Skill::Afterimage||s==Skill::DeathMark||s==Skill::Overclock))return false;
  if(lvl(o,s)){remove(buffs,[&](auto& b){return b.owner==o&&b.skill==s;});if(s==Skill::DeathMark)remove(marks,[&](auto& m){return m.owner==o;});h.reserve(o,total(o,efficiency));return true;}
  if(buffs.size()>=128||!h.reserve(o,total(o,efficiency)+Reservation(l,efficiency)))return false;
  buffs.push_back({o,s,Level(l),++serial,now+25,now,h.position(o)});return true;
 }
 // Reservations are owned by the established aura subsystem in the native host.
 void syncBuff(Id o,Skill s,int l,Tick now){
  auto it=std::find_if(buffs.begin(),buffs.end(),[&](auto& b){return b.owner==o&&b.skill==s;});
  if(l<=0){if(it!=buffs.end())buffs.erase(it);if(s==Skill::DeathMark)remove(marks,[&](auto& m){return m.owner==o;});return;}
  if(it!=buffs.end()){it->level=Level(l);return;}
  if(buffs.size()<128)buffs.push_back({o,s,Level(l),++serial,now+25,now,h.position(o)});
 }
 void reconcile(Id o,int efficiency){
  while(!h.reserve(o,total(o,efficiency))){auto it=buffs.end();for(auto b=buffs.begin();b!=buffs.end();++b)if(b->owner==o&&(it==buffs.end()||b->serial>it->serial))it=b;if(it==buffs.end())break;if(it->skill==Skill::DeathMark)remove(marks,[&](auto& m){return m.owner==o;});buffs.erase(it);}
 }
 bool omnislash(Id o,int l,Tick now){
  if(l<=0||!h.alive(o)||slashes.size()>=64||targets(o,h.position(o),8).empty())return false;
  for(auto& c:cooldowns)if(c.owner==o&&now<c.until)return false;
  remove(cooldowns,[&](auto& c){return c.owner==o||now>=c.until;});cooldowns.push_back({o,now+250});slashes.push_back({o,Level(l),10+Level(l)/5,now,now+250});return true;
 }
 bool place(Id o,Skill s,int l,Point center,Tick now){
  if(l<=0||!h.alive(o)||!(s==Skill::ATMG||s==Skill::Gravity))return false;
  int owned=0;for(auto& t:traps)if(t.owner==o&&t.skill==s)++owned;
  if(owned>=5){auto it=std::find_if(traps.begin(),traps.end(),[&](auto& t){return t.owner==o&&t.skill==s;});traps.erase(it);}
  if(traps.size()>=128)return false;
  traps.push_back({o,s,Level(l),80,0,center,now,now,s==Skill::Gravity?now+200:now+750,now+25});h.visual(s,o,center,s==Skill::Gravity?200:750);return true;
 }
 // The adapter calls this once for a resolved PRIMARY attack, with actual damage.
 // Proc callbacks use secondary=true. A proc may deal damage but never reseed these loops.
 void onHit(Id o,Id target,int ar,std::int64_t damage,Tick now,bool secondary=false){
  if(dispatching||secondary||!h.alive(o))return;
  Unit u;if(!h.find(o,target,u))return;
  int l=lvl(o,Skill::DeathMark);
  if(l){auto it=std::find_if(marks.begin(),marks.end(),[&](auto& m){return m.owner==o&&m.target==target;});if(it==marks.end()&&marks.size()<2048)marks.push_back({o,target,l,now+125,0,u.pos});store(o,target,damage);}
  l=lvl(o,Skill::Afterimage);if(l&&echoes.size()<1024)echoes.push_back({o,target,l,ar,now+6});
 }
 void tick(std::uint64_t game,Tick now){
  if(session!=game||now<last)reset(game);last=now;
  // Death and owner disappearance remove reservations and all owned work.
  for(auto& b:buffs)if(!h.alive(b.owner))h.reserve(b.owner,0);
  remove(buffs,[&](auto& b){return !h.alive(b.owner);});
  for(auto& b:buffs){
   if(b.skill==Skill::Queen&&now>=b.next){b.next=now+25;auto v=targets(b.owner,h.position(b.owner),8);int n=0;for(auto& u:v){if(n++>=std::min(8,3+b.level/10))break;strike(b.skill,b.owner,u.id,b.level);h.visual(b.skill,b.owner,u.pos,12);}}
   if(b.skill==Skill::Spikes&&now>=b.drop){auto pos=h.position(b.owner);b.drop=now+5;if(patches.size()<4096){int count=0;for(auto& p:patches)if(p.owner==b.owner)++count;if(count<64){patches.push_back({b.owner,b.level,pos,now+25,now+75});h.visual(b.skill,b.owner,pos,75);}}b.last=pos;}
  }
  for(auto& e:echoes)if(now>=e.due&&h.alive(e.owner)){strike(Skill::Afterimage,e.owner,e.target,e.level,e.ar);h.visual(Skill::Afterimage,e.owner,h.position(e.owner),6);}
  remove(echoes,[&](auto& e){return now>=e.due||!h.alive(e.owner);});
  for(auto& s:slashes)if(now>=s.next&&now<s.until&&s.left>0&&h.alive(s.owner)){
   s.next=now+6;
   auto v=targets(s.owner,h.position(s.owner),8);if(v.empty())continue;
   const auto center=h.position(s.owner);
   std::sort(v.begin(),v.end(),[&](const Unit& a,const Unit& b){const auto da=Distance(center,a.pos),db=Distance(center,b.pos);return da==db?a.id<b.id:da<db;});
   // Spread ten clones over available enemies; wrap when there are fewer.
   const auto target=v[s.cursor++%v.size()];
   // Failed placement and transient target loss do not spend a strike.
   if(!h.clone(s.owner,target.id,s.level))continue;
   const auto hit=strike(Skill::Omnislash,s.owner,target.id,s.level);
   if(!hit.landed||hit.damage<=0)continue;
   for(const auto& u:targets(s.owner,target.pos,2))if(u.id!=target.id)
    strike(Skill::Omnislash,s.owner,u.id,s.level,-1,0,false);
   --s.left;s.next=now+6;
  }
  remove(slashes,[&](auto& s){return s.left<=0||now>=s.until||!h.alive(s.owner);});
  for(auto& t:traps){if(!h.alive(t.owner)||now>t.until)continue;
   if(t.skill==Skill::ATMG){
    const int boost=lvl(t.owner,Skill::Overclock);const int rate=100+(boost?25+2*boost:0);
    // Fractional frame accumulator: 8 shots/sec is 3.125 frames, not 3 or 4.
    // Cap elapsed work after stalls rather than firing a catch-up storm.
    t.credit+=int(std::min<Tick>(now-t.last,5))*8*rate;t.last=now;
    auto v=targets(t.owner,t.center,8);if(v.empty()){t.credit=std::min(t.credit,2499);continue;}
    int count=0;while(t.credit>=2500&&t.shots>0&&count++<8){t.credit-=2500;strike(t.skill,t.owner,v.front().id,t.level);--t.shots;}t.credit=std::min(t.credit,2499);
   }else if(now>=t.next){t.next=now+75;for(auto& u:targets(t.owner,t.center,6))h.pullSlow(t.owner,u.id,t.center,std::min(60,20+t.level),std::min(t.until,now+75));}
  }
  remove(traps,[&](auto& t){return !h.alive(t.owner)||now>=t.until||t.shots<=0;});
  for(auto& p:patches)if(h.alive(p.owner)&&now>=p.next&&now<=p.until){p.next=now+25;for(auto& u:targets(p.owner,p.center,2))strike(Skill::Spikes,p.owner,u.id,p.level);}
  remove(patches,[&](auto& p){return now>=p.until||!h.alive(p.owner);});
  // Remove completed records before explosion callbacks; they cannot detonate twice.
  std::vector<Mark> done;for(auto& m:marks){Unit u;bool found=h.find(m.owner,m.target,u);if(found)m.center=u.pos;if(!h.alive(m.owner)||!lvl(m.owner,Skill::DeathMark)){m.owner=Invalid;continue;}if(!found||u.hp<=0||now>=m.until){done.push_back(m);m.owner=Invalid;}}
  remove(marks,[](auto& m){return m.owner==Invalid;});for(auto& m:done)explode(m);
 }
};
} // namespace AssassinPreview
