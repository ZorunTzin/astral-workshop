#pragma once
// Runs on UI thread. Slots remain owned/initialized by the native Materials bank.
namespace LeagueStash {
#include "stash_tooltips.h"
inline bool active=false,wasOpen=false;
// Match the actual native stack unit (or empty-slot preview), never merely the
// item code: an inventory orb hovered while the league tab is open already has
// its normal spelldesc and must not receive a duplicate description.
inline int tooltipIndex(const Items::ItemInfo& info) noexcept {
 __try {
  if(!active||!manager||!find||!child)return -1;
  auto bank=static_cast<unsigned char*>(find(*manager,"BankExpansionLayout"));
  if(!bank||!bank[0x51])return -1;
  auto group=child(bank,"advancedstash_materials");if(!group)return -1;
  for(int m=0;m<3;++m)for(int t=0;t<4;++t){
   char name[32];std::snprintf(name,sizeof(name),"LeagueOrb%d%d",m,t);
   auto slot=static_cast<unsigned char*>(child(group,name));if(!slot)continue;
   for(unsigned offset: {0x600u,0x608u}){
    auto unit=*reinterpret_cast<const unsigned**>(slot+offset);
    if(unit&&unit[0]==4&&unit[1]==info.classId&&unit[2]==info.runtimeId)return m*4+t;
   }
  }
 }__except(EXCEPTION_EXECUTE_HANDLER){}
 return -1;
}
inline void __cdecl tooltip(const PluginContext* ctx,SharedEvents::ItemTooltipEvent* event,void*) noexcept {
 if(!event||!event->text||!items)return;
 Items::ItemInfo info{.structSize=Items::ItemInfoSize};
 if(items->getItemInfo(ctx,event->item,&info)!=Items::Result::Success)return;
 const int index=tooltipIndex(info);if(index<0)return;
 const char* text=OrbStashDescriptions[index];const auto length=std::strlen(text);
 if(length>=event->capacity)return;
 std::memcpy(event->text,text,length+1);event->length=static_cast<unsigned>(length);
}
#include "stash_visibility.h"
inline void tick() noexcept {
 __try {
  auto bank=static_cast<unsigned char*>(find(*manager,"BankExpansionLayout"));if(!bank)return;
  bool open=bank[0x51]!=0;auto tabs=static_cast<unsigned char*>(child(bank,"BankTabs"));
  if(!open||!tabs){active=false;wasOpen=false;return;}
  if(!wasOpen)active=false;wasOpen=true;
  if(*reinterpret_cast<int*>(tabs+0x16c4)!=3)active=false;
  auto group=static_cast<unsigned char*>(child(bank,"advancedstash_materials"));if(!group)return;
  auto nodes=*reinterpret_cast<unsigned char***>(group+0x58);auto count=*reinterpret_cast<size_t*>(group+0x60);if(count>4096)return;
  for(size_t i=0;i<count;++i){auto n=nodes[i];if(!n)continue;auto name=*reinterpret_cast<const char**>(n+8);if(name)show(n,!std::strncmp(name,"LeagueOrb",9)?active:!active,true);}
  // Materials cube grid begins at y=708 and intercepts rows 3/4 behind our panel.
  if(*reinterpret_cast<int*>(tabs+0x16c4)==3)show(child(bank,"advancedstash_horadriccube"),!active,true);
  show(child(bank,"LeagueBackground"),active);show(child(bank,"LeagueMaterialsBack"),active,true);
 }__except(EXCEPTION_EXECUTE_HANDLER){active=false;}
}
inline void select(const PluginContext* c,void* back) noexcept {
 __try {
  auto bank=static_cast<unsigned char*>(find(*manager,"BankExpansionLayout"));if(!bank||!bank[0x51])return;
  auto tabs=child(bank,"BankTabs");if(!tabs)return;
  const unsigned char guard[]={0x48,0x89,0x5c,0x24,0x18,0x89,0x54,0x24,0x10,0x55,0x56,0x57};
  auto address=c->exeBase+0x878fb0;if(std::memcmp(reinterpret_cast<void*>(address),guard,sizeof(guard)))return;
  reinterpret_cast<void(__fastcall*)(void*,int,bool)>(address)(tabs,3,true);
  active=!back;wasOpen=true;tick();
 }__except(EXCEPTION_EXECUTE_HANDLER){active=false;}
}
}
