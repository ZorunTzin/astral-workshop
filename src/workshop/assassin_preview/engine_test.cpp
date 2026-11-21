#include "engine.h"
#include <cassert>
#include <iostream>
#include <map>
using namespace AssassinPreview;
struct Fake:Host {
 bool living=true,landing=true;Point origin{};int budget=10000,reserved=0,pulls=0,visuals=0;
 std::vector<Unit> enemies{{2,{1,0},1000000,1000000},{3,{2,0},1000000,1000000}};
 std::vector<Id> clones;std::vector<Hit> hits;Engine* engine=nullptr;bool recurse=false;
 bool alive(Id)override{return living;}
 Point position(Id)override{return origin;}
 std::vector<Unit> targets(Id,Point c,double r)override{std::vector<Unit> v;for(auto u:enemies)if(u.hp>0&&Distance(u.pos,c)<=r)v.push_back(u);return v;}
 bool find(Id,Id id,Unit& u)override{for(auto x:enemies)if(x.id==id){u=x;return true;}return false;}
 int failedClones=0,failedHits=0;
 bool clone(Id,Id target,int)override{if(failedClones>0){--failedClones;return false;}clones.push_back(target);return true;}
 bool jump(Id,Id)override{return landing;}
 Result hit(const Hit& r)override{if(failedHits>0){--failedHits;return {true,0};}hits.push_back(r);assert(r.allowOnHit);if(recurse&&engine)engine->onHit(1,r.target,r.attackRating,100,1);return {true,100};}
 bool reserve(Id,int n)override{if(n>budget)return false;reserved=n;return true;}
 void pullSlow(Id,Id,Point,int slow,Tick)override{assert(slow==40);++pulls;}
 void visual(Skill,Id,Point,Tick)override{++visuals;}
 int count(Skill s)const{return int(std::count_if(hits.begin(),hits.end(),[&](auto h){return h.skill==s;}));}
};
int main(){
 // The patch reaches exactly two yards, but not beyond its damage boundary.
 {Fake h;h.enemies={{2,{2,0},1000,1000},{3,{2.01,0},1000,1000}};Engine e(h);e.reset(1);e.toggle(1,Skill::Spikes,20,0);e.tick(1,0);e.tick(1,25);assert(h.count(Skill::Spikes)==1&&h.hits[0].target==2);}

 assert(Weapon(Skill::Omnislash,20)==700);assert(Weapon(Skill::Queen,20)==130);
 assert(Weapon(Skill::ATMG,20)==30);assert(Weapon(Skill::Spikes,20)==125);
 assert(Weapon(Skill::Afterimage,20)==90);assert(Accuracy(Skill::Omnislash,20)==600);
 assert(Reservation(20,0)==75&&Reservation(20,100)==37);
 Unit full{2,{},100,100};assert(ForcedCritical(20,full)&&CritBonus(20)==30);full.hp=99;assert(!ForcedCritical(20,full));
 {Fake h;Engine e(h);e.reset(1);assert(e.omnislash(1,20,0));assert(!e.omnislash(1,20,1));for(Tick t=0;t<=90;++t)e.tick(1,t);assert(h.count(Skill::Omnislash)==28&&h.clones.size()==14);assert(!e.omnislash(1,20,249));assert(e.omnislash(1,20,250));}
 // Clone target order is independent of hash order and skips dead targets.
 {Fake h;std::reverse(h.enemies.begin(),h.enemies.end());Engine e(h);e.reset(1);e.omnislash(1,1,0);e.tick(1,0);assert(h.clones.back()==2);h.enemies[1].hp=0;e.tick(1,6);assert(h.clones.back()==3);}
 {Fake h;h.enemies.resize(1);Engine e(h);e.reset(1);e.setExpose(1,20);e.omnislash(1,1,0);for(Tick t=0;t<=60;++t)e.tick(1,t);assert(h.hits.size()==10&&h.clones.size()==10);for(auto hit:h.hits)assert(hit.target==2&&hit.forceCritical&&hit.criticalBonus==30);}
 // One survivor still receives ten damaging primaries after placement/hit failures.
 {Fake h;h.enemies.resize(1);h.failedClones=3;h.failedHits=2;Engine e(h);e.reset(1);e.omnislash(1,1,0);for(Tick t=0;t<250;++t)e.tick(1,t);assert(h.hits.size()==10);}
 // A temporary empty query must not cancel the remaining strike budget.
 {Fake h;h.enemies.resize(1);Engine e(h);e.reset(1);e.omnislash(1,1,0);e.tick(1,0);h.enemies[0].pos={20,0};for(Tick t=1;t<40;++t)e.tick(1,t);h.enemies[0].pos={1,0};for(Tick t=40;t<250;++t)e.tick(1,t);assert(h.hits.size()==10);}
 // Never leave a permanent job on an invulnerable or disappeared target.
 {Fake h;h.enemies.resize(1);h.failedHits=1000;Engine e(h);e.reset(1);e.omnislash(1,1,0);for(Tick t=0;t<=250;++t)e.tick(1,t);auto n=h.clones.size();e.tick(1,500);assert(h.clones.size()==n&&h.hits.empty());}
 {Fake h;h.landing=false;Engine e(h);e.reset(1);e.omnislash(1,1,0);e.tick(1,0);assert(h.hits.size()==2&&h.clones.size()==1);}
 {Fake h;Engine e(h);e.reset(1);e.toggle(1,Skill::Queen,20,0);e.tick(1,24);assert(h.hits.empty());e.tick(1,25);assert(h.hits.size()==2);e.tick(1,25);assert(h.hits.size()==2);e.tick(1,50);assert(h.hits.size()==4);e.toggle(1,Skill::Queen,20,50);assert(h.reserved==0);}
 {Fake h;Engine e(h);e.reset(1);e.place(1,Skill::ATMG,20,{},0);for(Tick t=1;t<=25;++t)e.tick(1,t);assert(h.count(Skill::ATMG)==8);for(Tick t=26;t<=250;++t)e.tick(1,t);assert(h.count(Skill::ATMG)==80);e.tick(1,300);assert(h.count(Skill::ATMG)==80);}
 {Fake h;Engine e(h);e.reset(1);e.toggle(1,Skill::Overclock,20,0);e.place(1,Skill::ATMG,20,{},0);for(Tick t=1;t<=25;++t)e.tick(1,t);assert(h.count(Skill::ATMG)==13);}
 {Fake h;Engine e(h);e.reset(1);e.toggle(1,Skill::Spikes,20,0);h.origin={.1,0};e.tick(1,0);h.origin={.2,0};e.tick(1,5);for(Tick t=6;t<=80;++t)e.tick(1,t);assert(h.count(Skill::Spikes)==42);}
 {Fake h;Engine e(h);h.engine=&e;h.recurse=true;e.reset(1);e.toggle(1,Skill::Afterimage,20,0);e.onHit(1,2,123,100,0);e.tick(1,5);assert(h.hits.empty());e.tick(1,6);assert(h.hits.size()==1&&h.hits[0].weaponPercent==90&&h.hits[0].attackRating==123);e.tick(1,30);assert(h.hits.size()==1);e.onHit(1,2,123,100,31,true);e.tick(1,40);assert(h.hits.size()==1);}
 {Fake h;Engine e(h);e.reset(1);e.toggle(1,Skill::DeathMark,20,0);e.onHit(1,2,0,100,0);e.onHit(1,2,0,200,100);e.tick(1,124);assert(h.hits.empty());e.tick(1,125);assert(h.hits.size()==2);for(auto r:h.hits)assert(r.flatPhysical==300&&r.weaponPercent==0);e.tick(1,126);assert(h.hits.size()==2);}
 {Fake h;Engine e(h);e.reset(1);e.toggle(1,Skill::DeathMark,20,0);e.onHit(1,2,0,100,0);h.enemies[0].hp=0;e.tick(1,1);assert(h.hits.size()==1&&h.hits[0].target==3&&h.hits[0].flatPhysical==100);}
 {Fake h;Engine e(h);e.reset(1);e.place(1,Skill::Gravity,20,{},0);for(Tick t=0;t<=200;++t)e.tick(1,t);assert(h.pulls==6);e.tick(1,225);assert(h.pulls==6);}
 {Fake h;Engine e(h);e.reset(1);h.budget=100;assert(e.toggle(1,Skill::Queen,20,0));assert(!e.toggle(1,Skill::Afterimage,20,0));h.budget=200;assert(e.toggle(1,Skill::Afterimage,20,0));assert(h.reserved==150);h.budget=100;e.reconcile(1,0);assert(h.reserved==75);h.living=false;e.tick(1,1);assert(h.reserved==0);}
 {Fake h;Engine e(h);e.reset(1);e.toggle(1,Skill::Queen,20,0);e.tick(2,0);assert(h.reserved==0);e.tick(2,100);assert(h.hits.empty());}
 std::cout<<"Assassin staged engine: all 20 scenario groups passed.\n";
}
