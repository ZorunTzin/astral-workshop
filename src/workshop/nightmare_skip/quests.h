#pragma once
namespace NightmareSkip {
inline bool eligibleBaseline(unsigned difficulty,unsigned progress,unsigned baal,bool untouched){
    return difficulty==0&&progress<5&&!(baal&1)&&untouched;
}
inline bool completionReady(bool eligible,unsigned difficulty,unsigned progress,unsigned baal,bool untouched){
    return eligible&&difficulty==0&&progress==5&&(baal&1)&&untouched;
}
inline constexpr unsigned QuestRows[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,35,36,37,38,39,40,41};
template<class Words> bool untouchedNightmare(const Words& words){for(unsigned q:QuestRows)if(words[q])return false;return true;}
template<class Words> void completeNightmare(Words& words){for(unsigned q:QuestRows)words[q]=0x8001;}
template<class Words> bool rewardPending(const Words& words){if(words[37]&(1u<<7))return false;for(unsigned q:QuestRows)if(!(words[q]&1))return false;return true;}
}
