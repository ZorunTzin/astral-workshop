#define main BaseAuraTests
#include "../combat/aura_test.cpp"
#undef main
Unit victim,missile,pet;
unsigned rankBits=8;
int main(){
 BaseAuraTests();std::memset(game,0,sizeof(game));player={};victim={};missile={};pet={};Frame(100);
 player.id=42;player.stats[6]=player.stats[7]=256000;player.stats[Gull::GullStat]=1;
 victim.type=1;victim.id=800;victim.cls=12;victim.stats[172]=0;missile.type=3;missile.id=801;pet.type=1;pet.id=802;
 SoulHarvest::Refresh=nullptr;SoulHarvest::MarkGfxDirty=nullptr;Curses::UnitTimer=nullptr;
 Arena::Level=[](const void* p) noexcept{return static_cast<const Unit*>(p)->room;};
 Curses::Find=[](void*,int type,unsigned id)->void*{for(auto u:{&player,&victim,&missile,&pet})if(int(u->type)==type&&u->id==id)return u;return nullptr;};
 Curses::Owner=[](void*,void* p)->void*{return p==&missile?&pet:p==&pet?&player:nullptr;};
 Curses::MinionOwner=[](void* p)->void*{return p==&pet?&player:nullptr;};
 Gull::Rank=[](void*){return rankBits;};Gull::ListExpiry=[](void* l){return static_cast<List*>(l)->expiry;};
 Gull::Game=nullptr;Gull::Reset(game);
 for(int kind=0;kind<10;++kind){
  player.lists.clear();game[0x104]=2;Gull::Grant(game,&player,kind);Gull::Player(game,&player);
  auto list=GetList(&player,Gull::GullStates[kind]);assert(list&&Gull::ListExpiry(list)==1600);
  for(int i=0;i<100;++i)Gull::Player(game,&player);
  assert(Gull::ListExpiry(list)==1600);
  if(kind==0)assert(GetUnit(&player,171,0)==150&&GetUnit(&player,76,0)==30&&GetUnit(&player,74,0)==153);
  if(kind==1)assert(GetUnit(&player,25,0)==300&&GetUnit(&player,119,0)==300&&GetUnit(&player,256,0)==37&&GetUnit(&player,257,0)==37);
  if(kind==2)assert(GetUnit(&player,41,0)==112&&GetUnit(&player,42,0)==15);
  if(kind==6)assert(GetUnit(&player,127,0)==6);
  if(kind==7)assert(GetUnit(&player,27,0)==600&&GetUnit(&player,105,0)==75);
  if(kind==8)assert(GetUnit(&player,154,0)==100&&GetUnit(&player,93,0)==45&&GetUnit(&player,105,0)==0);
  if(kind==9)assert(GetUnit(&player,85,0)==75&&GetUnit(&player,79,0)==150&&GetUnit(&player,80,0)==75);
 }
 // Different shrine types coexist; refreshing the same type does not stack.
 Gull::Grant(game,&player,7);Gull::Player(game,&player);assert(GetUnit(&player,85,0)==75&&GetUnit(&player,105,0)==75);
 player.stats[Gull::GullStat]=0;Gull::Player(game,&player);assert(GetUnit(&player,105,0)==50);player.stats[Gull::GullStat]=1;
 Frame(11351);Gull::Player(game,&player);assert(GetUnit(&player,105,0)==0);
 // Native shrine extension occurs once, including across an equip toggle.
 Frame(12000);player.lists.clear();auto& natural=player.lists[128];natural.expiry=19500;natural.stats[171]=100;
 WorkshopShrines::Player(game,&player);Gull::Player(game,&player);assert(natural.expiry==23250&&GetUnit(&player,171,0)==150);
 for(int i=0;i<100;++i)Gull::Player(game,&player);assert(natural.expiry==23250);
 player.stats[Gull::GullStat]=0;Gull::Player(game,&player);assert(GetUnit(&player,171,0)==100);player.stats[Gull::GullStat]=1;Gull::Player(game,&player);assert(natural.expiry==23250);
 Frame(12100);natural.expiry=19600;Gull::Player(game,&player);assert(natural.expiry==23350);
 // Champions/uniques, projectile and summon owner attribution, observation-only queue.
 Gull::Observe(game,&victim,&missile);assert(Gull::Pending.size()==1);Gull::Observe(game,&victim,&missile);assert(Gull::Pending.size()==1);
 ++victim.id;rankBits=12;Gull::Observe(game,&victim,&pet);assert(Gull::Pending.size()==2);
 ++victim.id;rankBits=16;Gull::Observe(game,&victim,&player);assert(Gull::Pending.size()==2);
 rankBits=8;victim.stats[172]=1;Gull::Observe(game,&victim,&player);assert(Gull::Pending.size()==2);victim.stats[172]=0;
 player.stats[Gull::GullStat]=0;Gull::Observe(game,&victim,&player);assert(Gull::Pending.size()==2);
 player.stats[Gull::GullStat]=1;player.stats[6]=0;Gull::Player(game,&player);Gull::Observe(game,&victim,&player);assert(Gull::Pending.size()==2);
 Frame(1);Gull::Reset(game);assert(Gull::Pending.empty()&&Gull::Extensions.empty());
 puts("PASS: ten shrine buffs, 150% values, 60-second duration, native shrine extension once, no equipment/refresh compounding, no instant-effect shrines, coexistence, expiry, death, elite filtering, projectile/pet attribution and duplicate/session guards.");
}
