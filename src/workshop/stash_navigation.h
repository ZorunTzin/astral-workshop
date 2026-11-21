#pragma once
#include <cstdint>
constexpr int stashStride(bool fast,bool control){return fast?(control?100:10):(control?5:1);}
constexpr int stashSteps(uint64_t current,uint64_t total,int delta){
    if(!total||current>=total||delta==0||delta < -100||delta>100)return 0;
    auto available=delta<0?current:total-current-1;
    auto requested=static_cast<uint64_t>(delta<0?-delta:delta);
    return static_cast<int>(available<requested?available:requested);
}
