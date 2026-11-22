#pragma once
#include "bindings.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
namespace League {
constexpr unsigned TicksPerSecond=25;
enum class Kind {Night,Breach,Beyond};
enum class Phase {Waiting,Active,Complete};
struct Point {int x=0,y=0;};
inline int distance2(Point a,Point b){int x=a.x-b.x,y=a.y-b.y;return x*x+y*y;}
struct Kill {Point p;unsigned frame;};
struct Encounter {
 Kind kind=Kind::Night;Phase phase=Phase::Waiting;int area=0,stage=0,spawned=0,killed=0,groups=0,pending=0;unsigned started=0,until=0,next=0,lastSeen=0;Point center;std::array<Point,3> rifts{};std::vector<Kill> kills;
 bool activate(unsigned now){if(phase!=Phase::Waiting)return false;phase=Phase::Active;started=now;until=now+Durations[int(kind)]*TicksPerSecond;next=now;return true;}
 bool expired(unsigned now)const{return phase==Phase::Active&&now>=until;}
 bool clustered(Point p,unsigned now){
  if(kind!=Kind::Beyond||phase!=Phase::Active||expired(now)||groups>=12)return false;
  kills.erase(std::remove_if(kills.begin(),kills.end(),[&](const Kill& k){return now-k.frame>125;}),kills.end());
  kills.push_back({p,now});if(kills.size()>64)kills.erase(kills.begin());
  int nearby=0;for(auto& k:kills)if(distance2(k.p,p)<=100)++nearby;
  if(nearby<5)return false;
  kills.erase(std::remove_if(kills.begin(),kills.end(),[&](const Kill& k){return distance2(k.p,p)<=100;}),kills.end());++groups;return true;
 }
};
inline bool eligible(int area){return area>=2&&area<=132&&area!=39&&area!=40&&area!=75&&area!=103&&area!=109;}
inline unsigned random(unsigned& seed){if(!seed)seed=0x91d401a7;seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
inline unsigned rollMechanics(unsigned& seed){unsigned mask=0;for(int kind=0;kind<3;++kind)if(random(seed)%100<AreaChance)mask|=1u<<kind;return mask;}
}
