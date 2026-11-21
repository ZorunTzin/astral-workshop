#define main aura_regression_main
#include "aura_test.cpp"
#undef main
#include <tuple>
struct ItemList { unsigned long long padding=0; unsigned type=4; unsigned padding2=0; std::map<std::pair<int,unsigned>,int> values; };
int __fastcall ItemGet(unsigned char,void* p,int stat,unsigned layer){return static_cast<ItemList*>(p)->values[{stat,layer}];}
void __fastcall ItemSet(unsigned char,void* p,int stat,int value,unsigned layer){static_cast<ItemList*>(p)->values[{stat,layer}]=value;}
int main(){
 Ramp::Get=ItemGet;ItemAuras::OriginalSet=ItemSet;
 ItemList item; item.values[{387,123}]=2;item.values[{97,123}]=2;
 ItemAuras::Set(3,&item,151,12,123);
 assert(ItemGet(3,&item,151,123)==0);
 assert(ItemGet(3,&item,387,123)==14&&ItemGet(3,&item,97,123)==14);
 // Reloaded converted properties contain ordinary skill stats, not aura stats.
 ItemAuras::Set(3,&item,97,14,123);
 assert(ItemGet(3,&item,97,123)==14);
 ItemAuras::Set(3,&item,151,0,124);assert(ItemGet(3,&item,97,124)==0);
 ItemAuras::Set(3,&item,151,5,512);assert(ItemGet(3,&item,151,512)==5);
 item.type=0;ItemAuras::Set(3,&item,151,6,99);
 assert(ItemGet(3,&item,151,99)==6&&ItemGet(3,&item,97,99)==0);
 puts("PASS: saved item aura becomes all-class skill, existing oskill preserved, converted reload unchanged, non-item/zero/invalid-layer forwarding.");
}
