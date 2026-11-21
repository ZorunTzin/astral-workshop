#define main BaseAuraTests
#include "aura_test.cpp"
#undef main
Unit guardian,enemy,friendly,ordinary;
unsigned castCount=0;int castSkill=0,castLevel=0;void* castCaster=nullptr;
int __fastcall X(void* u){return static_cast<Unit*>(u)->x;}
int __fastcall Y(void* u){return static_cast<Unit*>(u)->y;}
void* __fastcall Room(void* u){return reinterpret_cast<void*>(static_cast<std::uintptr_t>(static_cast<Unit*>(u)->room));}
void* __fastcall PetOwner(void* u){return u==&guardian||u==&ordinary?&player:nullptr;}
void* __fastcall NoOwner(void*,void*){return nullptr;}
void* __fastcall FindAll(void*,int type,unsigned id){
 if(type==0&&id==player.id)return &player;
 for(Unit* p:{&guardian,&enemy,&friendly,&ordinary})if(type==1&&p->id==id)return p;
 return nullptr;
}
bool nativeReach=true;
int __fastcall Reach(void*){return 2;}
int __fastcall InRange(void*,void*,int,void*){return nativeReach?1:0;}
int __fastcall Cast(void* caster,int skill,int level,void* target,int){
 if(GuardianCombat::BindNativeSkills){void* list=nullptr;std::memcpy(&list,static_cast<unsigned char*>(caster)+0x100,8);void* node=nullptr;std::memcpy(&node,static_cast<unsigned char*>(list)+0x18,8);assert(node);void* rec=nullptr;std::memcpy(&rec,node,8);assert(*static_cast<short*>(rec)==skill);}assert(target==&enemy);++castCount;castSkill=skill;castLevel=level;castCaster=caster;return 1;}
void Place(Unit& u,unsigned cls,unsigned id){u.type=1;u.cls=cls;u.id=id;u.stats[6]=1000;void* p=&u;std::memcpy(game+0x2630+8*(id&127),&p,8);}
int main(){
 BaseAuraTests();Frame(100);player.stats[6]=1000;player.stats[12]=12;player.levels[110]=7;
 Curses::Find=FindAll;Curses::X=X;Curses::Y=Y;Curses::Cast=Cast;Curses::MinionOwner=PetOwner;Curses::Owner=NoOwner;SoulHarvest::Room=Room;
 Place(guardian,GuardianCombat::MonsterClass,20);Place(enemy,1,21);Place(friendly,1,22);Place(ordinary,357,23);
 guardian.stats[7]=1234*256;guardian.stats[21]=100;guardian.stats[22]=250;
 friendly.stats[172]=1;enemy.x=27;
 GuardianCombat::Tick(game);assert(castCount==1&&castSkill==GuardianCombat::CombatSpells[0][0]&&castLevel==7&&castCaster==&guardian);
 Frame(109);GuardianCombat::Tick(game);assert(castCount==1);
 Frame(110);GuardianCombat::Tick(game);assert(castCount==1);
 player.stats[12]=18;Frame(150);GuardianCombat::Tick(game);
 assert(castCount==2&&castSkill==GuardianCombat::CombatSpells[1][0]);
 Frame(200);GuardianCombat::Tick(game);assert(castCount==3&&castSkill==GuardianCombat::CombatSpells[1][0]);
 player.stats[12]=30;Frame(250);GuardianCombat::Tick(game);assert(castCount==4&&castSkill==GuardianCombat::CombatSpells[2][0]);
 Frame(279);GuardianCombat::Tick(game);assert(castCount==4);
 Frame(350);enemy.room=2;GuardianCombat::Tick(game);assert(castCount==4);
 Frame(375);enemy.room=1;enemy.stats[172]=1;GuardianCombat::Tick(game);assert(castCount==4);
 Frame(400);enemy.stats[172]=0;enemy.stats[6]=0;GuardianCombat::Tick(game);assert(castCount==4);
 Frame(425);enemy.stats[6]=1000;player.levels[110]=0;GuardianCombat::Tick(game);assert(castCount==4);
 Frame(450);player.levels[110]=7;player.stats[6]=0;GuardianCombat::Tick(game);assert(castCount==4);
 // Match the real pet layout: learned skill nodes exist, but used-skill is
 // null. Triggered native skills must see the selected node during the call.
 alignas(8) unsigned char skillList[0x28]{},nodes[4][0x58]{};short ids[4]{};
 for(int i=0;i<4;++i){ids[i]=static_cast<short>(GuardianCombat::CombatSpells[i][0]);void* rec=&ids[i];std::memcpy(nodes[i],&rec,8);if(i<3){void* next=nodes[i+1];std::memcpy(nodes[i]+8,&next,8);}}
 void* list=skillList;void* head=nodes[0];std::memcpy(skillList,&head,8);std::memcpy(reinterpret_cast<unsigned char*>(&guardian)+0x100,&list,8);
 GuardianCombat::BindNativeSkills=true;
 assert(GuardianCombat::Cast(&guardian,GuardianCombat::CombatSpells[0][0],7,&enemy)==1);
 void* after=nullptr;std::memcpy(&after,skillList+0x18,8);assert(after==nullptr);
 assert(GuardianCombat::Cast(&guardian,999,7,&enemy)==0);
 // A pre-existing AI skill is also restored after the temporary cast.
 void* original=nodes[2];std::memcpy(skillList+0x18,&original,8);
 assert(GuardianCombat::Cast(&guardian,GuardianCombat::CombatSpells[0][0],7,&enemy)==1);
 std::memcpy(&after,skillList+0x18,8);assert(after==original);
 // Zeal takes priority at contact distance and uses a bounded staggered burst.
 Frame(500);player.stats[6]=1000;enemy.x=22;player.stats[12]=30;
 GuardianCombat::Casters={};GuardianCombat::NextScan=0;
 const auto old=castCount;GuardianCombat::Tick(game);
 assert(castCount==old+1&&castSkill==GuardianCombat::CombatSpells[3][0]);
 Frame(504);GuardianCombat::Tick(game);assert(castCount==old+1);
 for(unsigned f:{505U,510U,515U,520U}){Frame(f);GuardianCombat::Tick(game);}
 assert(castCount==old+5);
 Frame(524);GuardianCombat::Tick(game);assert(castCount==old+5);
 assert(GuardianCombat::ZealHits(1)==2&&GuardianCombat::ZealHits(3)==4&&GuardianCombat::ZealHits(20)==5);
 // Never repeat a queued hit against a dead or distant target.
 GuardianCombat::Casters={};GuardianCombat::NextScan=0;Frame(600);GuardianCombat::Tick(game);
 const auto afterStart=castCount;enemy.x=100;Frame(605);GuardianCombat::Tick(game);assert(castCount==afterStart);
 enemy.x=22;GuardianCombat::Casters={};GuardianCombat::NextScan=0;Frame(700);GuardianCombat::Tick(game);
 const auto beforeDeath=castCount;enemy.stats[6]=0;Frame(705);GuardianCombat::Tick(game);assert(castCount==beforeDeath);
 // A large enemy may be in true weapon range with centers more than four
 // subtiles apart. Native reach must override the old fixed-distance cutoff.
 Frame(800);player.stats[6]=1000;enemy.stats[6]=1000;enemy.x=27;
 GuardianCombat::Casters={};GuardianCombat::NextScan=0;
 Curses::Reach=Reach;Curses::InRange=InRange;nativeReach=true;
 const auto beforeLarge=castCount;GuardianCombat::Tick(game);
 assert(castCount==beforeLarge+1&&castSkill==GuardianCombat::CombatSpells[3][0]);
 assert(guardian.lists[GuardianCombat::ZealState].stats[68]==100);
 assert(guardian.lists[GuardianCombat::ZealState].expiry==830);
 nativeReach=false;Frame(805);GuardianCombat::Tick(game);assert(castCount==beforeLarge+1);
 Curses::Reach=nullptr;Curses::InRange=nullptr;
 assert(GuardianCombat::Select(12,500,0)==0&&GuardianCombat::Select(18,10,0)==3&&GuardianCombat::Select(30,500,0)==2);
 std::puts("PASS: native reach overrides center distance, timed Zeal attack animation, native learned-skill selection with null/current AI slot and restoration, staggered native Zeal, burst cap and cancellation, guardian-only autonomous combat, exact caster/target/level, character-level unlocks, range-based choice, cooldowns, room/friendly/dead-target guards, respec and owner-death guards; ordinary Valkyries unchanged.");return 0;
}
