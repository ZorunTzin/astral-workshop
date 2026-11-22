#pragma once
// Runs on UI thread. Slots remain owned/initialized by the native Materials bank.
namespace LeagueStash {
inline bool active=false,wasOpen=false;
inline bool show(void* widget,bool yes){
 if(!widget)return false;auto fn=reinterpret_cast<void(__fastcall*)(void*,bool)>((*reinterpret_cast<void***>(widget))[10]);
 const unsigned char guard[]={0x88,0x51,0x51,0xc3};if(!fn||std::memcmp(reinterpret_cast<void*>(fn),guard,4))return false;fn(widget,yes);return true;
}
inline void tick() noexcept {
 __try {
  auto bank=static_cast<unsigned char*>(find(*manager,"BankExpansionLayout"));if(!bank)return;
  bool open=bank[0x51]!=0;auto tabs=static_cast<unsigned char*>(child(bank,"BankTabs"));
  if(!open||!tabs){active=false;wasOpen=false;return;}
  if(!wasOpen)active=false;wasOpen=true;
  if(*reinterpret_cast<int*>(tabs+0x16c4)!=3)active=false;
  auto group=static_cast<unsigned char*>(child(bank,"advancedstash_materials"));if(!group)return;
  auto nodes=*reinterpret_cast<unsigned char***>(group+0x58);auto count=*reinterpret_cast<size_t*>(group+0x60);if(count>4096)return;
  for(size_t i=0;i<count;++i){auto n=nodes[i];if(!n)continue;auto name=*reinterpret_cast<const char**>(n+8);if(name)show(n,!std::strncmp(name,"LeagueOrb",9)?active:!active);}
  // Materials cube grid begins at y=708 and intercepts rows 3/4 behind our panel.
  if(*reinterpret_cast<int*>(tabs+0x16c4)==3)show(child(bank,"advancedstash_horadriccube"),!active);
  show(child(bank,"LeagueBackground"),active);show(child(bank,"LeagueMaterialsBack"),active);
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
