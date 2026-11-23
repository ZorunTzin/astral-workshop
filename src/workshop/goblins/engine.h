#pragma once
#include <array>
#include <cstdint>
namespace Goblins {
constexpr unsigned WorldChance=30,TrialChance=3; // percent, per area / wave (not per frame or player)
constexpr unsigned FirstChannelDelay=200,ChannelDuration=75,RetryDelay=125;
inline bool WorldArea(int level){return level>=2&&level<=132&&level!=39&&level!=40&&level!=75&&level!=103&&level!=109;}
inline bool TrialArea(int level){return level>=166&&level<=170;}
struct Schedule {
 std::array<bool,171> visited{},trialSpawned{};
 std::array<int,5> wave{};
 bool takeWorld(int level){if(!WorldArea(level)||visited[level])return false;visited[level]=true;return true;}
 bool takeTrial(int level,int number){if(!TrialArea(level)||number<1||trialSpawned[level]||wave[level-166]>=number)return false;wave[level-166]=number;return true;}
};
struct Escape {
 unsigned due=0,end=0;int hp=0;unsigned interrupted=0;bool channel=false;
 void start(unsigned now,int life){due=now+FirstChannelDelay;hp=life;}
 // Return true only when escape completes. Lethal damage always wins.
 bool tick(unsigned now,int life){
  if(life<=0)return false;
  if(channel&&life<hp&&interrupted<3){++interrupted;channel=false;due=now+RetryDelay;}
  hp=life;
  if(!channel&&now>=due){channel=true;end=now+ChannelDuration;}
  return channel&&now>=end;
 }
};
inline unsigned Random(unsigned& state){if(!state)state=0x9e3779b9u;state^=state<<13;state^=state>>17;state^=state<<5;return state;}
}
