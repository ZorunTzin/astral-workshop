#pragma once
#include <algorithm>
#include <cstdint>
namespace MythicalModel {
inline int ElementExtra(int physical,int percent,int resist,int current,int total){
 const auto room=std::max(0,0x3fffffff-std::max(current,total));
 return static_cast<int>(std::min<std::int64_t>(room,std::int64_t{std::max(0,physical)}*percent*(100-std::clamp(resist,-100,100))/10000));
}
inline bool Bonded(unsigned mask){return mask&&(mask&(mask-1));}
inline int Multiplier(bool highlord,bool orion,bool tsukuyomi,unsigned roll){
 if(tsukuyomi&&roll<3)return 5;
 if((highlord||orion)&&roll<10)return 3;
 return 1;
}
}
