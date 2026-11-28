#include <cstring>
#include <cassert>
#include <cstdio>
#include "stash_visibility.h"
__declspec(noinline) void __fastcall Visible(void* w,bool b){static_cast<unsigned char*>(w)[0x51]=b;}
__declspec(noinline) void __fastcall Enabled(void* w,bool b){static_cast<unsigned char*>(w)[0x50]=b;}
int main(){
 void* table[11]{};table[9]=reinterpret_cast<void*>(Enabled);table[10]=reinterpret_cast<void*>(Visible);
 alignas(8) unsigned char slot[128]{};*reinterpret_cast<void***>(slot)=table;
 assert(show(slot,true,true)&&slot[0x50]&&slot[0x51]);
 assert(show(slot,false,true)&&!slot[0x50]&&!slot[0x51]);
 assert(show(slot,true,true)&&slot[0x50]&&slot[0x51]);
 slot[0x50]=0;assert(show(slot,true)&&!slot[0x50]&&slot[0x51]); // Decorative background remains noninteractive.
 table[9]=nullptr;assert(!show(slot,false,true)&&slot[0x51]);
 assert(!show(nullptr,true,true));
 puts("PASS: hidden virtual stash slots reject input, active slots restore it, decoration stays noninteractive, invalid setters fail closed.");
}
