#pragma once
// The native matcher runs before Cube inputs are consumed. GateStat is unsaved
// and defaults to zero, so recipes fail closed if this plugin is unavailable.
namespace LeagueCraft {
using MatchFn=int(__fastcall*)(void*,void*,void*,int,void*,void*);
using PointerFn=void*(__fastcall*)(void*);
using SetFn=void(__fastcall*)(void*,int,int,unsigned);
inline MatchFn OriginalMatch=nullptr;
inline PointerFn First=nullptr,Next=nullptr,AsItem=nullptr;
inline SetFn Set=nullptr;
inline int Match(void* game,void* inv,void* recipe,int index,void* results,void* used){
 auto r=static_cast<unsigned char*>(recipe);int param=0,cost=0;
 if(r){std::memcpy(&param,r+4,4);std::memcpy(&cost,r+8,4);}
 if(!r||r[3]!=15||param!=GateStat)return OriginalMatch(game,inv,recipe,index,results,used);
 if(index!=0)return OriginalMatch(game,inv,recipe,index,results,used);
 if(cost!=2&&cost!=4&&cost!=6&&cost!=10)return 0;
 void* target=nullptr;unsigned count=0,orbs=0,equipment=0;
 for(void* node=First(inv);node&&count++<4096;node=Next(node)){
  void* item=AsItem(node);if(!item)continue;
  auto data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
  if(!data||data[0x55]!=3)continue;
  unsigned code=affixCode(item);bool orb=false;
  for(char m='n';m<='p';++m)for(char t='1';t<='4';++t)if(code==Items::MakeItemCode('L',m,t))orb=true;
  if(orb)++orbs;else{target=item;++equipment;}
 }
 if(orbs!=1||equipment!=1||!target)return 0;
 // Null player calculates the conservative requirement across character classes.
 const int required=sigilRequiredLevel(target,nullptr);
 if(required<0||required>120-cost)return 0;
 const int previous=readItemStat(target,GateStat,0);
 Set(target,GateStat,120-required,0);
 const int result=OriginalMatch(game,inv,recipe,index,results,used);
 Set(target,GateStat,previous,0);
 return result;
}
#ifndef LEAGUE_CRAFT_FIXTURE
inline bool Install(const PluginContext* c){
 const unsigned char guard[]={0x44,0x89,0x4c,0x24,0x20,0x55,0x53,0x56,0x41,0x55,0x48,0x8b,0xec,0x48,0x83,0xec,0x68};
 First=reinterpret_cast<PointerFn>(c->exeBase+0x388c10);Next=reinterpret_cast<PointerFn>(c->exeBase+0x38aba0);AsItem=reinterpret_cast<PointerFn>(c->exeBase+0x38aab0);Set=reinterpret_cast<SetFn>(c->exeBase+0x2f7d10);
 auto hook=MakeInlineHook(0x529d50,guard,sizeof(guard),reinterpret_cast<void*>(Match),reinterpret_cast<void**>(&OriginalMatch));
 return c->api->installInlineHook(c,&hook);
}
#endif
}
