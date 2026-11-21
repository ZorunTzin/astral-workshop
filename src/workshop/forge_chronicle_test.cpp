#include <array>
#include <cstring>
#include <cstdio>
#include <set>
#include <cassert>
#include <cstdint>
struct PluginContext {void LogInfo(const char*)const{}};
struct Word{const char* name="test";};std::array<Word,220> forgeWords{};
constexpr int forgeMarkerStat=443,forgeLevelStat=471;
int marker=2,value=0;
int readStat(void*,int stat,unsigned){return stat==forgeMarkerStat?marker:value;}
auto readItemStat=readStat;
#define WORKSHOP_CHRONICLE_TEST
#include "forge_chronicle_native.inc"
std::set<unsigned> discovered;int sends=0,nativePickups=0,adds=0;void* expectedPlayer{};
ForgeChronicle::Packet lastPacket;
bool __fastcall add(void* p,unsigned char type,unsigned recipe,unsigned short dropper,unsigned stamp){assert(p==expectedPlayer&&type==2&&dropper==0&&stamp==123);++adds;return discovered.insert(recipe).second;}
void* __fastcall client(void* p){assert(p==expectedPlayer);return p;}
void* __fastcall player(void*,int n){assert(n==0);return expectedPlayer;}
void __fastcall send(void*,const void* p){++sends;lastPacket=*static_cast<const ForgeChronicle::Packet*>(p);}
long long __fastcall clockNow(){return 123LL*60000000000LL;}
void __fastcall pickup(void*,void*){++nativePickups;}
void* cubeResult{};
void* __fastcall cube(void*,void* p,void*,void*){assert(p==expectedPlayer);return cubeResult;}
int main(){
 for(int i=0;i<220;++i)assert(ForgeChronicle::identity(2+i/128,i%128)==i);
 for(auto pair:std::array<std::array<int,2>,7>{{{1,12},{0,0},{4,0},{2,-1},{2,128},{3,92},{3,127}}})assert(ForgeChronicle::identity(pair[0],pair[1])==-1);
 std::set<unsigned> mapped;for(auto row:forgeChronicleRows)assert(mapped.insert(row.row).second);assert(mapped.size()==220);
 alignas(8) std::array<unsigned char,128> pu{},iu{},data{};
 auto* p=reinterpret_cast<unsigned*>(pu.data());auto* item=reinterpret_cast<unsigned*>(iu.data());
 p[0]=0;p[2]=7;item[0]=4;item[2]=99;*reinterpret_cast<void**>(iu.data()+0x10)=data.data();expectedPlayer=p;
 chronicleAdd=add;chronicleClient=client;chroniclePlayer=player;chronicleSend=send;chronicleClock=clockNow;originalChroniclePickup=pickup;
 for(unsigned i=0;i<220;++i)forgeChronicleNativeIds[i]=static_cast<unsigned short>(60000+i);
 recordRuneForge(p,item);assert(sends==0);forgeChronicleReady=true;
 recordRuneForge(p,item);assert(sends==1&&discovered.size()==1&&lastPacket.recipe==60000&&lastPacket.player==7&&lastPacket.item==99&&lastPacket.type==2&&lastPacket.minutes==123);
 recordRuneForge(p,item);assert(sends==1&&adds==2); // repeated craft/pickup does not notify/count twice
 marker=3;value=91;chroniclePickup(p,item);assert(sends==2&&lastPacket.recipe==60219&&nativePickups==1);
 chroniclePickup(p,item);assert(sends==2&&nativePickups==2);
 marker=1;value=20;recordRuneForge(p,item);assert(sends==2); // no guessed identity for old items
 *reinterpret_cast<unsigned*>(data.data()+0x18)=0x04000000;chroniclePickup(p,item);assert(nativePickups==2&&sends==2); // native runeword excluded
 *reinterpret_cast<unsigned*>(data.data()+0x18)=0;chroniclePickup(p,item);assert(nativePickups==3); // ordinary Unique/Set discovery delegated
 marker=3;value=92;recordRuneForge(p,item);assert(sends==2);
 originalChronicleCube=cube;marker=2;value=42;
 assert(chronicleCube(nullptr,p,nullptr,nullptr)==nullptr&&sends==2); // failed cube output
 cubeResult=item;assert(chronicleCube(nullptr,p,nullptr,nullptr)==item&&sends==3&&lastPacket.recipe==60042);
 chronicleCube(nullptr,p,nullptr,nullptr);assert(sends==3); // repeat successful forge
 std::puts("PASS: all 220 forge IDs, variant mapping, packet layout, duplicate suppression, legacy/invalid IDs, native runeword exclusion and Unique/Set delegation");
}
