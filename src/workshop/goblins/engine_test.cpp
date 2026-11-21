#include "engine.h"
#include <cassert>
#include <cstdio>
int main(){using namespace Goblins;
 for(int town:{1,40,75,103,109})assert(!WorldArea(town));
 for(int special:{0,39,133,136,145,165,171})assert(!WorldArea(special)&&!TrialArea(special));
 for(int world:{2,38,41,74,76,102,104,108,110,132})assert(WorldArea(world));
 Schedule s;assert(s.takeWorld(2));assert(!s.takeWorld(2));assert(!s.takeWorld(1));
 assert(s.takeTrial(166,1));assert(!s.takeTrial(166,1));assert(s.takeTrial(166,2));s.trialSpawned[166]=true;assert(!s.takeTrial(166,3));assert(s.takeTrial(167,1));
 Escape e;e.start(0,100);assert(!e.tick(199,100)&&!e.channel);assert(!e.tick(200,100)&&e.channel);assert(!e.tick(274,100));assert(e.tick(275,100));
 e={};e.start(0,100);unsigned t=200;int hp=100;
 for(int i=0;i<3;++i){assert(!e.tick(t,hp)&&e.channel);assert(!e.tick(t+10,--hp)&&!e.channel);t+=10+RetryDelay;}
 assert(e.interrupted==3);assert(!e.tick(t,hp)&&e.channel);assert(!e.tick(t+10,--hp)&&e.channel);assert(!e.tick(t+75,0));assert(e.tick(t+75,hp));
 unsigned seed=123,world=0,trial=0;for(int i=0;i<100000;++i){world+=Random(seed)%100<WorldChance;trial+=Random(seed)%100<TrialChance;}
 assert(world>2700&&world<3300&&trial>800&&trial<1200);
 puts("Goblin schedule, rarity and escape tests passed.");
}
