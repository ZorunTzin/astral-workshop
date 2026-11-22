#include <cassert>
#include <initializer_list>
#include <cstring>
#include <cstdio>
#include "craft_bindings.h"
#define LEAGUE_CRAFT_FIXTURE
namespace Items {constexpr unsigned MakeItemCode(char a,char b,char c){return unsigned(a)|(unsigned(b)<<8)|(unsigned(c)<<16);}}
struct Item {unsigned char pad[16]{};unsigned char* data;unsigned code;int required=0,gate=0;};
unsigned affixCode(void* p){return static_cast<Item*>(p)->code;}
int sigilRequiredLevel(void* p,void*){return static_cast<Item*>(p)->required;}
int readItemStat(void* p,int,int){return static_cast<Item*>(p)->gate;}
#include "craft_native.inc"
unsigned char da[96]{},db[96]{};Item a{{},da,1},b{{},db,Items::MakeItemCode('L','n','1')};int calls=0;
int __fastcall Native(void*,void*,void* r,int,void*,void*){++calls;int cost;std::memcpy(&cost,static_cast<char*>(r)+8,4);return a.gate>=cost;}
int main(){
 LeagueCraft::First=[](void*)->void*{return &a;};LeagueCraft::Next=[](void* p)->void*{return p==&a?&b:nullptr;};LeagueCraft::AsItem=[](void* p)->void*{return p;};LeagueCraft::Set=[](void* p,int,int v,unsigned){static_cast<Item*>(p)->gate=v;};LeagueCraft::OriginalMatch=Native;
 da[0x55]=db[0x55]=3;unsigned char recipe[16]{};recipe[3]=15;int stat=265;std::memcpy(recipe+4,&stat,4);
 for(int cost:{2,4,6,10}){std::memcpy(recipe+8,&cost,4);for(int base=0;base<=122;++base){a.required=base;a.gate=7;int old=calls;int result=LeagueCraft::Match(nullptr,nullptr,recipe,0,nullptr,nullptr);assert(result==(base+cost<=120));assert(a.gate==7);assert(calls-old==(base+cost<=120?1:0));}}
 a.required=119;int cost=2;std::memcpy(recipe+8,&cost,4);int old=calls;assert(!LeagueCraft::Match(nullptr,nullptr,recipe,0,nullptr,nullptr)&&calls==old);
 a.required=0;db[0x55]=0;assert(!LeagueCraft::Match(nullptr,nullptr,recipe,0,nullptr,nullptr));db[0x55]=3;
 // Invalid recipe costs fail closed, leaving ingredients untouched.
 cost=1;std::memcpy(recipe+8,&cost,4);assert(!LeagueCraft::Match(nullptr,nullptr,recipe,0,nullptr,nullptr));
 puts("PASS: all four orb level costs, exact 120 boundary, 119+2 rejection before native matching, missing cube orb and temporary stat restoration.");
}
