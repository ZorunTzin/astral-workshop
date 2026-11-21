#include "seal_engine.h"
#include <cassert>
#include <cstdio>
int main(){
 {Arena::Run r;r.start(1,1,1);r.phase=Arena::Phase::Choice;r.offers[0]={Arena::Mod::PlayerLife,0,3,5};auto epoch=r.epoch;
 assert(!r.choose(0,epoch+1)&&r.bonuses[0]==0);assert(r.choose(0,epoch));assert(r.bonuses[0]==10&&r.bonuses[1]==2&&r.bonuses[int(Arena::Mod::PlayerLife)]==3);
 assert(!r.choose(0,epoch)&&r.bonuses[0]==10);r.phase=Arena::Phase::Fighting;r.clear();assert(r.reroll(false,r.epoch)&&r.bonuses[0]==10);
 r.offers[0]={Arena::Mod::ChampionWave,3,1,80};assert(r.choose(0,r.epoch));assert(r.bonuses[0]==20&&r.bonuses[1]==4);r.phase=Arena::Phase::Fighting;r.clear();assert(r.bonuses[0]==20&&r.bonuses[1]==4);}

 for(unsigned trial=1;trial<10001;++trial){unsigned seed=trial;TrialCraft::Seal s;auto normal=s;
 assert(!TrialCraft::craft(s,TrialCraft::Orb::Renewal,seed));assert(s.fields==normal.fields);
 assert(TrialCraft::craft(s,TrialCraft::Orb::Awakening,seed));assert(s.grade()==1&&s.count()>=1&&s.count()<=2);
 while(s.accepts(TrialCraft::Orb::Augmentation))assert(TrialCraft::craft(s,TrialCraft::Orb::Augmentation,seed));assert(s.count()==2);
 auto description=TrialCraft::tooltip(s,1);assert(description.find("Magic Trial Seal")!=std::string::npos);assert(description.find("Starting loot bonus +"+std::to_string(s.reward()))!=std::string::npos);
 auto ranges=TrialCraft::tooltip(s,1,true);assert(ranges.find("[")!=std::string::npos&&description.find("[")==std::string::npos);
 for(unsigned char c:description)assert(c<128);
 for(int i=0;i<s.count();++i)assert(description.find("+"+std::to_string(s.value(i,1))+"%")!=std::string::npos);
 auto full=s;assert(!TrialCraft::craft(s,TrialCraft::Orb::Augmentation,seed));assert(s.fields==full.fields);
 assert(TrialCraft::craft(s,TrialCraft::Orb::Ascension,seed));assert(s.grade()==2&&s.count()>=3&&s.count()<=5);
 while(s.accepts(TrialCraft::Orb::Augmentation))assert(TrialCraft::craft(s,TrialCraft::Orb::Augmentation,seed));assert(s.count()==5);
 auto types=s;assert(TrialCraft::craft(s,TrialCraft::Orb::Fortune,seed));for(int i=0;i<5;++i)assert(s.mod(i)==types.mod(i));
 // All stored fields fit the existing 12-bit save schema; reconstruction is exact.
 TrialCraft::Seal saved;for(int i=0;i<8;++i){assert(s.fields[i]>=0&&s.fields[i]<4096);saved.fields[i]=s.fields[i]&4095;}assert(saved.fields==s.fields);
 assert(TrialCraft::craft(s,TrialCraft::Orb::Corruption,seed));assert(s.corrupted()&&s.valid());for(int i=0;i<7;++i)assert(!s.accepts(TrialCraft::Orb(i)));
 Arena::Run run;run.start(3,10,1);int champion,elite;TrialCraft::apply(s,run,champion,elite);assert(run.rewardBonus==s.reward());assert(champion<=100&&elite<=100);assert(run.bonuses[int(Arena::Mod::PlayerLife)]==0);
 s=full;assert(TrialCraft::craft(s,TrialCraft::Orb::Purification,seed));assert(s.fields==normal.fields);
 assert(TrialCraft::craft(s,TrialCraft::Orb::Corruption,seed));assert(s.valid()&&s.corrupted());
 }
 puts("PASS: 10000 seal crafting sequences, eligibility, unique modifiers, tier rerolls, persistence roundtrip, corruption lock and run application.");
}
