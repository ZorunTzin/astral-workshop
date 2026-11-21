#pragma once
#include "engine.h"
#include <cstring>
namespace MonsterCaption {
// The native panel consumes UTF-8 text. Keep each added line short enough for
// the existing health/resistance panel; ordinary native modifiers stay intact.
inline constexpr const char* Names[]={"Plaguebearer","Gravity Core","Mirror Image","Pack Leader","Glittering","Raid Boss","Totemic","Duelist","Ritualist","Corpse Eater"};
inline bool Append(char* out,std::size_t capacity,const char* original,MonsterModifiers::Mask mask){
 if(!out||!capacity)return false;out[0]=0;
 const std::size_t n=original?strnlen(original,capacity):0;if(n>=capacity)return false;
 if(n)std::memcpy(out,original,n);std::size_t used=n;out[used]=0;unsigned count=0;
 for(unsigned i=0;i<sizeof(Names)/sizeof(Names[0]);++i)if(mask&MonsterModifiers::Bit(MonsterModifiers::Mod(13+i))){
  const char* separator=(count%2)?" / ":used?"\n":"";std::size_t a=std::strlen(separator),b=std::strlen(Names[i]);
  if(used+a+b>=capacity){out[0]=0;return false;}
  std::memcpy(out+used,separator,a);used+=a;std::memcpy(out+used,Names[i],b);used+=b;out[used]=0;++count;
 }
 return count!=0;
}
}
