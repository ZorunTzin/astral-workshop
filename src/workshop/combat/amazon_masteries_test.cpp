#define main ExistingAuraTests
#include "aura_test.cpp"
#undef main
Unit equipped;
unsigned weaponCode=0;unsigned char slot=4;
void* __fastcall inventory(void*){return &player;}
void* __fastcall first(void*){return &equipped;}
void* __fastcall next(void*){return nullptr;}
unsigned __fastcall code(void*){return weaponCode;}
unsigned char __fastcall body(void*){return slot;}
int __fastcall masteryLevel(unsigned char,void* u,int formula,int,int){for(auto d:AmazonMasteries::Definitions)if(d.helper==formula)return static_cast<Unit*>(u)->levels[d.skill];return 0;}
int main(){
 ExistingAuraTests();player={};player.cls=0;player.id=42;player.stats[6]=1000;player.stats[9]=10000;Frame(100);
 Curses::Evaluate=masteryLevel;LeagueStorage::Inventory=inventory;LeagueStorage::First=first;LeagueStorage::Next=next;LeagueStorage::Code=code;AmazonMasteries::Body=body;
 equipped.type=4;equipped.mode=1;
 unsigned weapons[3]{};for(auto w:AmazonMasteries::Weapons)weapons[w.kind]=w.code;
 for(auto d:AmazonMasteries::Definitions)player.levels[d.skill]=1;
 weaponCode=weapons[0];auto spear=AmazonMasteries::Definitions[0].skill,bow=AmazonMasteries::Definitions[1].skill,javelin=AmazonMasteries::Definitions[2].skill;
 assert(AmazonMasteries::cast(game,&player,spear,1)==1);
 assert(GetUnit(&player,76,0)==20&&GetUnit(&player,16,0)==30&&GetUnit(&player,36,0)==3);
 for(int stat:{39,41,43,45})assert(GetUnit(&player,stat,0)==6);
 assert(AmazonMasteries::allowed(game,&player,10));assert(!AmazonMasteries::allowed(game,&player,15));assert(!AmazonMasteries::allowed(game,&player,6));
 assert(AmazonMasteries::allowed(game,&player,bow));
 weaponCode=weapons[1];assert(AmazonMasteries::cast(game,&player,bow,1)==1);
 assert(GetUnit(&player,76,0)==0&&GetUnit(&player,36,0)==0&&GetUnit(&player,96,0)==11);
 for(int stat:{48,50,54})assert(GetUnit(&player,stat,0)==3);
 for(int stat:{49,51,55})assert(GetUnit(&player,stat,0)==6);
 weaponCode=weapons[2];assert(!AmazonMasteries::allowed(game,&player,0));AmazonMasteries::frame(game);assert(GetUnit(&player,96,0)==0);
 assert(AmazonMasteries::cast(game,&player,javelin,1)==1);assert(GetUnit(&player,20,0)==4&&GetUnit(&player,76,0)==14);
 slot=11;assert(!AmazonMasteries::allowed(game,&player,0));Frame(105);AmazonMasteries::frame(game);assert(GetUnit(&player,76,0)==0);slot=4;
 Frame(110);AmazonMasteries::frame(game);assert(GetUnit(&player,76,0)==14);
 player.levels[javelin]=20;Frame(115);AmazonMasteries::frame(game);assert(GetUnit(&player,76,0)==90);
 assert(AmazonMasteries::cast(game,&player,javelin,20)==1);assert(GetUnit(&player,76,0)==0);assert(AmazonMasteries::allowed(game,&player,0));
 AmazonMasteries::cast(game,&player,javelin,20);player.stats[6]=0;Frame(120);AmazonMasteries::frame(game);assert(GetUnit(&player,76,0)==0);
 player.stats[6]=1000;AmazonMasteries::cast(game,&player,javelin,20);player.levels[javelin]=0;Frame(125);AmazonMasteries::frame(game);assert(GetUnit(&player,76,0)==0);
 assert(GetUnit(&player,9,0)==10000);
 puts("PASS: exclusive Masteries, exact bonus bundles, weapon/skill gates, inactive swap slots, suspended bonuses, level updates, toggle, death and respec cleanup; no mana cost.");
}
