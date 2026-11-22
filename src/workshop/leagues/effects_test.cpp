#define main ExistingAuraTests
#include "../combat/aura_test.cpp"
#undef main
Unit enemy,missile;unsigned impacts=0,visuals=0;
void* __fastcall Create(void*,void* raw){auto& p=*static_cast<Pulse::Params*>(raw);if(p.missile!=LeagueEffects::NovaHit){++visuals;return nullptr;}missile.stats.clear();missile.stats[21]=10*256;missile.stats[22]=20*256;return &missile;}
int __fastcall Impact(void*,void* m,void* t,int){assert(t==&enemy);assert(LeagueEffects::Depth&&Curses::ProcDepth);for(int stat:{48,50,54}){assert(GetUnit(m,stat,0)==10*256);assert(GetUnit(m,stat+1,0)==20*256);}assert(GetUnit(m,21,0)==0);++impacts;return 1;}
int main(){
 ExistingAuraTests();Frame(1000);player.lists.clear();player.stats[6]=1000;enemy.type=1;enemy.id=23;enemy.stats[6]=1000;
 Curses::X=[](void* u)->int{return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u)->int{return static_cast<Unit*>(u)->y;};Assassin::NativePositions=false;
 Curses::Owner=[](void*,void*)->void*{return nullptr;};Curses::MinionOwner=[](void*)->void*{return nullptr;};Barbarian::AuraArea=[](const void*)noexcept->int{return 1;};
 Curses::Find=[](void*,int t,unsigned id)->void*{return t==0&&id==player.id?&player:t==1&&id==enemy.id?&enemy:nullptr;};
 void* e=&enemy;std::memcpy(game+0x2630+8*(enemy.id&127),&e,8);
 Pulse::OriginalCreate=Create;Pulse::AfterCreate=nullptr;Pulse::FilterCreate=nullptr;Curses::NativeMissileHit=Impact;Amazon::PublishMissiles=nullptr;
 alignas(8) unsigned char packet[0x140]{};unsigned short hit=1;std::memcpy(packet+4,&hit,2);int damage=100;std::memcpy(packet+0x134,&damage,4);
 player.stats[LeagueCraft::NovaStat]=100;
 LeagueEffects::Hit(game,&player,&enemy,packet);assert(impacts==0&&LeagueEffects::Queue.size()==1);
 LeagueEffects::Frame(game);assert(impacts==1&&visuals==96&&LeagueEffects::Queue.empty());LeagueEffects::Frame(game);assert(impacts==1);
 hit=0;std::memcpy(packet+4,&hit,2);LeagueEffects::Hit(game,&player,&enemy,packet);assert(LeagueEffects::Queue.empty());
 hit=0x21;std::memcpy(packet+4,&hit,2);LeagueEffects::Hit(game,&player,&enemy,packet);assert(LeagueEffects::Queue.empty());
 Unit spell;spell.type=3;spell.cls=1;while(std::find(std::begin(LeagueCraft::WeaponMissiles),std::end(LeagueCraft::WeaponMissiles),int(spell.cls))!=std::end(LeagueCraft::WeaponMissiles))++spell.cls;
 Amazon::CurrentHitMissile=&spell;assert(!LeagueEffects::Weapon(&player));player.stats[LeagueCraft::SpellStat]=16;
 for(int off:{0x18,0x20,0x24,0x2c,0x30,0x34,0x38,0x134})std::memcpy(packet+off,&damage,4);
 LeagueEffects::Before(game,&player,&enemy,packet);for(int off:{0x18,0x20,0x24,0x2c,0x30,0x34,0x38,0x134})assert(Barbarian::ReadDamage(packet,off)==116);
 hit=1;std::memcpy(packet+4,&hit,2);LeagueEffects::Hit(game,&player,&enemy,packet);assert(LeagueEffects::Queue.empty());
 spell.cls=LeagueCraft::WeaponMissiles[0];assert(LeagueEffects::Weapon(&player));LeagueEffects::Before(game,&player,&enemy,packet);assert(Barbarian::ReadDamage(packet,0x18)==116);
 Amazon::CurrentHitMissile=nullptr;LeagueEffects::Hit(game,&player,&enemy,packet);assert(!LeagueEffects::Queue.empty());Frame(1);LeagueEffects::Frame(game);assert(LeagueEffects::Queue.empty()&&impacts==1);
 puts("PASS: League universal spell channels; melee/ranged classification; successful-only deferred Trinity Nova; 100% weapon damage per element; recursion guard, no duplicate frame impact and session cleanup.");
}
