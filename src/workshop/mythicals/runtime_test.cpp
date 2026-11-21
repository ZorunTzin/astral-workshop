#define main ExistingAuraTests
#include "../combat/aura_test.cpp"
#undef main
#include "runtime.h"
int __fastcall MythGet(void* p,int stat,unsigned layer){if(stat==MarkerStat)return static_cast<Unit*>(p)->levels[layer];return GetUnit(p,stat,layer);}
void* __fastcall NoOwner(void*,void*){return nullptr;}
void MythTick(unsigned n){Frame(n);SoulHarvest::PlayersCached=false;Mythicals::Frame(game);}
int main(){
 ExistingAuraTests();player={};player.stats[6]=10000;player.stats[7]=10000;player.mode=1;
 Ramp::GetUnit=MythGet;Curses::Owner=NoOwner;SetSelectors::Random=123456;
 std::memset(game+0x2230,0,1024);void* u=&player;std::memcpy(game+0x2230+8*(player.id&127),&u,8);
 player.levels[Power28]=1;MythTick(100);assert(!GetList(&player,BondState));
 player.levels[Power28]=2;MythTick(105);assert(!GetList(&player,BondState));
 player.levels[Power28]=1;player.levels[Power29]=1;MythTick(110);
 assert(GetList(&player,BondState));assert(MythGet(&player,329,0)==30&&MythGet(&player,331,0)==30&&MythGet(&player,105,0)==40);
 MythTick(115);assert(MythGet(&player,105,0)==40); // No accumulation each frame.
 player.levels.erase(Power29);MythTick(120);Frame(121);assert(!GetList(&player,BondState));
 player.levels[Power29]=1;MythTick(125);assert(GetList(&player,BondState));
 player.mode=17;MythTick(130);Frame(146);assert(!GetList(&player,BondState));player.mode=1;
 player.levels.clear();player.levels[Power0]=1;Unit monster;monster.type=1;monster.id=20;monster.stats[6]=10000;monster.stats[39]=50;monster.stats[43]=100;
 alignas(8) unsigned char packet[0x200]{};
 auto put=[&](unsigned off,int v){std::memcpy(packet+off,&v,4);};auto get=[&](unsigned off){int v;std::memcpy(&v,packet+off,4);return v;};
 put(4,1);put(0x18,1000);put(0x134,1000);Mythicals::Hit(game,&player,&monster,packet);
 assert(get(0x20)==125&&get(0x24)==250&&get(0x2c)==0&&get(0x134)==1375);
 player.levels.clear();put(0x134,1000);Mythicals::Hit(game,&player,&monster,packet);assert(get(0x134)==1000);
 player.stats[7]=256000;player.stats[9]=128000;player.levels[43]=1;MythTick(200);
 assert(GetList(&player,RenewalState));assert(MythGet(&player,74,0)==512);assert(MythGet(&player,27,0)==400);
 MythTick(205);assert(MythGet(&player,74,0)==512); // No stacking on renewal.
 player.levels.clear();player.levels[44]=1;MythTick(210);
 assert(MythGet(&player,74,0)==1024&&MythGet(&player,26,0)==768&&MythGet(&player,27,0)==0);
 player.stats[7]=512000;MythTick(215);assert(MythGet(&player,74,0)==2048);
 player.levels.clear();MythTick(220);Frame(236);assert(!GetList(&player,RenewalState));
 player.levels[44]=1;MythTick(240);player.mode=17;MythTick(245);Frame(261);assert(!GetList(&player,RenewalState));
 std::puts("PASS: Mythical native damage, resistance, distinct ring pairing, non-stacking renewal, unequip and death expiry.");
}
