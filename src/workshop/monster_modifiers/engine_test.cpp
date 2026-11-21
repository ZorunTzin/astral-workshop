#include "engine.h"
#include "caption_text.h"
#include <cassert>
#include <cstdio>
using namespace MonsterModifiers;
struct TestHost:Host {std::vector<Event> events;bool living=true;unsigned stones=3;Id enemy=2;bool get(Id id,Unit& u)override{u={id,10,10,10,living};return true;}Id nearestEnemy(Id,int)override{return enemy;}void emit(const Event& e)override{events.push_back(e);}unsigned liveChildren(Id,Action kind)override{return kind==Action::RuneStone?stones:0;}};
int main(){
 char caption[4096]{};
 assert(!MonsterCaption::Append(caption,sizeof(caption),"Fire Enchanted",Bit(Fire)));
 assert(MonsterCaption::Append(caption,sizeof(caption),"Life 123 / Fire Enchanted",Bit(Poison)|Bit(Mirror)));
 assert(std::strcmp(caption,"Life 123 / Fire Enchanted\nPlaguebearer / Mirror Image")==0);
 assert(MonsterCaption::Append(caption,sizeof(caption),nullptr,All));
 for(auto* name:MonsterCaption::Names)assert(std::strstr(caption,name));
 char tiny[8]{};assert(!MonsterCaption::Append(tiny,sizeof(tiny),"old",All));
 assert(Identity(0,42)!=Identity(1,42));
 assert(Type(Identity(1,42))==1&&Number(Identity(1,42))==42);

 for(unsigned seed=1;seed<1000;++seed){unsigned rng=seed;Mask used=0;for(unsigned n=0;n<Count;++n){auto m=Roll(All,used,rng);if(m==Count){assert(Expand(used)==All);break;}assert(!(Expand(used)&Bit(m)));used|=Bit(m);}}
 assert(Expand(Bit(Raid)|Bit(Fire))==All);assert(LootRolls(All)==5);assert(LootRolls(Bit(Glitter))==2);
 assert(DamageTaken(1000,Bit(Stone),false)==250);assert(DamageTaken(1000,0,true)==100);assert(DamageTaken(1000,Bit(Stone),true)==25);
 assert(ModifierCount(All)==23&&LifePercent(All)==675&&DamagePercent(All)==215);
 assert(LifePercent(0)==100&&DamagePercent(0)==100);
 assert(LifePercent(Bit(Cold)|Bit(Fire))==150&&DamagePercent(Bit(Cold)|Bit(Fire))==110);
 assert(GravityRadius==24&&GravityDestination(24,0)==8&&GravityDestination(0,24)==16);
 assert(MissileDamage(100,1)==25600&&MissileDamage(100,5)==1024);
 assert(RollRange(0)==std::make_pair(1,2)&&RollRange(1)==std::make_pair(2,4)&&RollRange(2)==std::make_pair(3,5));
 Engine e;TestHost h;e.add(1,Bit(Gifted)|Bit(Lightning)|Bit(Cold)|Bit(Totems),0);
 e.tick(h,24);assert(h.events.empty());e.tick(h,25);assert(h.events.size()==3);e.tick(h,25);assert(h.events.size()==3);
 e.tick(h,1000);assert(h.events.size()==7); // no 40-cast catch-up burst
 unsigned totems=0;for(auto x:h.events)totems+=x.action==Action::Totem;assert(totems==1);
 e.clear();h.events.clear();e.add(1,Bit(Fire)|Bit(Poison),0);e.death(h,1,10);e.death(h,1,10);assert(h.events.size()==4);assert(e.pending.size()==1);
 h.living=false;e.tick(h,84);assert(h.events.size()==4);e.tick(h,85);assert(h.events.size()==5);e.tick(h,86);assert(h.events.size()==5);
 e.clear();assert(e.actors.empty()&&e.pending.empty());
 h.living=true;h.events.clear();e.add(1,Bit(Spectral),0);for(int n=0;n<4;++n)e.hit(h,1,2,n,true);assert(h.events.size()==1);e.hit(h,1,2,4,false);assert(h.events.size()==1);
 assert(InsideArena(12,0)&&InsideArena(0,-12)&&!InsideArena(12,1)&&!InsideArena(13,0));
 assert(RitualLifePercent(3)==400&&RitualDamagePercent(3)==300&&RitualDamagePercent(3,All)==645);
 for(unsigned stones=0;stones<=3;++stones){
  e.clear();h.events.clear();h.stones=stones;e.add(1,Bit(Ritualist),0);
  e.tick(h,0);assert(h.events.size()==3);for(auto event:h.events)assert(event.action==Action::RuneStone);
  e.tick(h,249);assert(h.events.size()==3);h.enemy=Invalid;e.tick(h,250);
  assert(h.events.size()==4&&h.events.back().action==Action::RitualSummon&&h.events.back().damage==int(stones));
  h.enemy=2;e.tick(h,10000);assert(h.events.size()==4);
 }
 e.clear();h.events.clear();e.add(1,Bit(CorpseEater),0);e.tick(h,0);e.tick(h,74);assert(h.events.size()==1);e.tick(h,75);assert(h.events.size()==2);
 e.clear();h.events.clear();e.add(1,Bit(Ritualist),0);e.tick(h,0);e.death(h,1,100);e.tick(h,250);assert(h.events.size()==3);
 e.clear();h.events.clear();h.enemy=Invalid;e.add(1,Bit(Duelist),0);
 for(unsigned frame=0;frame<=500;frame+=5)e.tick(h,frame);
 assert(h.events.size()==26);for(const auto& event:h.events)assert(event.action==Action::Arena&&event.duration==40);
 e.death(h,1,501);e.tick(h,525);assert(h.events.size()==26);h.enemy=2;
 puts("PASS: modifier uniqueness, all-modifier raid expansion, 2x/5x loot policy, multiplicative reductions, spell cadence, no catch-up storm, totem cadence and exactly-once delayed death explosion.");
}
