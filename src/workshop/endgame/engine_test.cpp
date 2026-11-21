#include "engine.h"
#include <cassert>
#include <set>
#include <cstdio>
int main(){using namespace Arena;
 Run r;r.start(1,9,123);assert(r.lifePercent()==120&&r.damagePercent()==120&&!r.clear());
 r.phase=Phase::Fighting;assert(r.clear()&&!r.clear());auto score=r.score;auto epoch=r.epoch;
 std::set<int> unique;for(auto o:r.offers){unique.insert(int(o.mod));assert(o.tier>=0&&o.tier<5&&o.value>0&&o.reward>0);if(int(o.mod)<3)assert(o.reward==(10<<o.tier));}assert(unique.size()==3);
 assert(!r.choose(3,epoch)&&!r.choose(0,epoch-1));assert(r.choose(0,epoch));assert(!r.choose(1,epoch)&&r.wave==2&&r.score==score);
 r.phase=Phase::Fighting;r.clear();assert(r.score>score);int full=r.score;assert(r.finish(true)&&r.score==full/2&&!r.finish(true));
 double before=100;for(int i=1;i<=5;++i){Run x;x.start(i,0,1);assert(x.lifePercent()>=before*1.19);before=x.lifePercent();for(int m=0;m<6;++m)for(int t=1;t<5;++t)assert(x.value(Mod(m),t)>x.value(Mod(m),t-1));}
 Run x;x.start(5,0,1);for(int i=0;i<100;++i){x.phase=Phase::Fighting;assert(x.clear());assert(x.choose(i%3,x.epoch));}assert(x.rewards()<=30&&x.maxRune()<=24&&x.lifePercent()<=100000&&x.damagePercent()<=100000);
 Run g;g.start(2,0,44);int goblinOffers=0;
 for(int i=0;i<10000;++i){g.roll();for(auto o:g.offers)if(o.mod==Mod::LootGoblinWave){++goblinOffers;assert(o.tier==4&&o.value==1);}}
 assert(goblinOffers>40&&goblinOffers<160);
 g.banned[int(Mod::LootGoblinWave)]=true;for(int i=0;i<1000;++i){g.roll();for(auto o:g.offers)assert(o.mod!=Mod::LootGoblinWave);}
 g.phase=Phase::Choice;g.offers[0]={Mod::LootGoblinWave,4,1,80};assert(g.choose(0,g.epoch));assert(g.bonuses[int(Mod::LootGoblinWave)]==1);
 g.phase=Phase::Fighting;assert(g.clear());assert(g.bonuses[int(Mod::LootGoblinWave)]==0);
 std::puts("PASS: arena progression, configured tier scaling, distinct offers, stale/double choice guards, five qualities, bounded rewards, wave scoring and half-loot death.");}
