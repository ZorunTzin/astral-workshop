#pragma once
// Visibility alone leaves overlapping virtual-tab slots eligible for mouse input.
// Guard both native trivial setters before changing either property.
inline bool show(void* widget,bool yes,bool interactive=false){
 if(!widget)return false;
 auto table=*reinterpret_cast<void***>(widget);
 using Setter=void(__fastcall*)(void*,bool);
 auto visible=reinterpret_cast<Setter>(table[10]);
 auto enabled=reinterpret_cast<Setter>(table[9]);
 const unsigned char visibilityGuard[]={0x88,0x51,0x51,0xc3};
 const unsigned char enabledGuard[]={0x88,0x51,0x50,0xc3};
 if(!visible||std::memcmp(reinterpret_cast<void*>(visible),visibilityGuard,4))return false;
 if(interactive&&(!enabled||std::memcmp(reinterpret_cast<void*>(enabled),enabledGuard,4)))return false;
 if(interactive)enabled(widget,yes);
 visible(widget,yes);return true;
}
