#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include "modifiers.h"
namespace Arena {
enum class Phase {Idle,Countdown,Fighting,Choice,Finished,Failed};
inline constexpr const char* Tiers[]={"White","Magic","Rare","Unique","Legendary"};
inline constexpr int Strength[]={1,2,3,4,6},Weights[]={50,28,14,6,2};
struct Offer {Mod mod{};int tier=0,value=0,reward=0;};
struct Run {
 Phase phase=Phase::Idle;unsigned owner=~0u,seed=1,epoch=0;int stage=0,wave=0,completed=0,score=0,rewardBonus=0;
 std::array<int,ModCount> bonuses{};std::array<bool,ModCount> banned{};int rerolls=3,banishes=3;std::array<Offer,3> offers{};
 unsigned random(){seed=1664525u*seed+1013904223u;return seed;}
 void start(int s,unsigned id,unsigned entropy){*this={};stage=s;owner=id;seed=entropy?entropy:1;phase=Phase::Countdown;wave=1;++epoch;}
 int value(Mod mod,int tier)const{if(mod==Mod::ChampionWave||mod==Mod::EliteWave||mod==Mod::LootGoblinWave)return 1;return TierValues[int(mod)][tier]*(100+10*(stage-1))/100;}
 int reward(Mod mod,int tier)const{return (Positive[int(mod)]?5:10)*(1<<tier);}
 void roll(){
  std::array<int,ModCount> bad{},good{};int nb=0,ng=0;
  for(int i=0;i<ModCount;++i)if(i!=int(Mod::ChampionWave)&&i!=int(Mod::EliteWave)&&i!=int(Mod::LootGoblinWave)&&EnabledMods[i]&&!banned[i]&&bonuses[i]<Caps[i]){if(Positive[i])good[ng++]=i;else bad[nb++]=i;}
  bool beneficial=ng>0&&((random()>>8)%100)<20;int goodSlot=beneficial?int((random()>>8)%3):-1;
  for(int i=0;i<3;++i){int id;
   if(i==goodSlot)id=good[(random()>>8)%ng];
   else if(nb){int pick=(random()>>8)%nb;id=bad[pick];bad[pick]=bad[--nb];}
   else id=int(Mod::MonsterLife);
   int r=(random()>>8)%100,t=0;while(t<4&&r>=Weights[t])r-=Weights[t++];auto m=Mod(id);
   offers[i]={m,t,std::min(value(m,t),Caps[id]-bonuses[id]),reward(m,t)};
  }
  // Rare one-wave events: 3% champion / 1% elite per selection, never overwrite the positive slot.
  unsigned event=(random()>>8)%100;if(event<4){int slot=(goodSlot+1+3)%3;Mod m=event<3?Mod::ChampionWave:Mod::EliteWave;int t=event<3?3:4;if(!banned[int(m)])offers[slot]={m,t,1,reward(m,t)};}
  if(event==4&&!banned[int(Mod::LootGoblinWave)]){int slot=goodSlot>=0?goodSlot:0;offers[slot]={Mod::LootGoblinWave,4,1,reward(Mod::LootGoblinWave,4)};}
  ++epoch;
 }
 bool reroll(bool banish,unsigned expected){if(phase!=Phase::Choice||expected!=epoch||(banish?banishes:rerolls)<=0)return false;if(banish){for(auto o:offers)banned[int(o.mod)]=true;--banishes;}else --rerolls;roll();return true;}
 bool clear(){if(phase!=Phase::Fighting)return false;completed=wave;bonuses[int(Mod::ChampionWave)]=bonuses[int(Mod::EliteWave)]=bonuses[int(Mod::LootGoblinWave)]=0;score+=int(std::min<int64_t>(50000,int64_t(100+20*(wave-1))*(100+10*(stage-1)+rewardBonus)/100));phase=Phase::Choice;roll();return true;}
 void addModifierBurden(){bonuses[0]=std::min(Caps[0],bonuses[0]+10);bonuses[1]=std::min(Caps[1],bonuses[1]+2);}
 bool choose(int choice,unsigned expected){if(phase!=Phase::Choice||expected!=epoch||choice<0||choice>=3)return false;
  addModifierBurden();auto o=offers[choice];bonuses[int(o.mod)]=std::min(Caps[int(o.mod)],bonuses[int(o.mod)]+o.value);rewardBonus=std::min(2000,rewardBonus+o.reward);++wave;++epoch;phase=Phase::Countdown;return true;}
 bool finish(bool death=false){if(phase==Phase::Idle||phase==Phase::Finished||phase==Phase::Failed)return false;if(death)score/=2;phase=death?Phase::Failed:Phase::Finished;++epoch;return true;}
 double stageScale()const{constexpr double scales[]={1.2,1.5,1.9,2.4,3.0};return scales[std::clamp(stage,1,5)-1];}
 int lifePercent()const{return int(std::min(100000.,100*stageScale()*(1+.12*(wave-1))*(1+bonuses[0]/100.)));}
 int damagePercent()const{return int(std::min(100000.,100*stageScale()*(1+.08*(wave-1))*(1+bonuses[1]/100.)));}
 int count()const{return std::min(72,(10+wave*2+stage*2)*(100+bonuses[int(Mod::MonsterCount)])/100);}
 int lootBonus()const{return std::min(2000,10*std::max(0,wave-1)+20*std::max(0,stage-1)+rewardBonus);}
 int rewards()const{return completed?std::min(30,std::max(1,score/500)):0;}
 int maxRune()const{return std::min(24,8+2*stage+completed/3);}
};
}
