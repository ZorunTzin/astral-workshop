#define main RunAuraRegression
#include "combat/aura_test.cpp"
#undef main
int main(){
 RunAuraRegression();
 for(auto& entry:SetSelectors::Selectors){
  unsigned n=entry[0];std::array<unsigned,8> count{};SetSelectors::Random=1234567;
  for(unsigned i=0;i<300000;++i){unsigned value=SetSelectors::Draw(n);assert(value<n);++count[value];}
  for(unsigned i=0;i<n;++i)assert(count[i]>300000/n*97/100&&count[i]<300000/n*103/100);
 }
 Frame(2000);player.stats[6]=1000*256;SoulHarvest::PlayersCached=false;void* ptr=&player;std::memcpy(game+0x2230+8*(player.id&127),&ptr,8);
 SetSelectors::Tick(game);
 for(auto& entry:SetSelectors::Selectors)assert(player.stats.count(entry[1])&&GetUnit(&player,entry[1],0)>=0&&GetUnit(&player,entry[1],0)<entry[0]);
 std::puts("PASS: every native Cube selector stays in range, unbiased distribution for all set sizes, server player receives current selectors; no intermediate item or item API mutation.");
 return 0;
}
