#define main ExistingAuraTests
#include "aura_test.cpp"
#undef main
Unit spirits[6],enemies[6],unityArrow,equipped;
unsigned weaponCode=0;
void* shotOrigin=nullptr;
std::vector<void*> volleyOrigins;
int spawnCount=0,failAt=-1,shots=0,removed=0;
std::vector<unsigned> shotFrames;
int main(){
 ExistingAuraTests();
 LeagueStorage::Inventory=[](void*)->void*{return &player;};LeagueStorage::First=[](void*)->void*{return &equipped;};LeagueStorage::Next=[](void*)->void*{return nullptr;};LeagueStorage::Code=[](void*){return weaponCode;};AmazonMasteries::Body=[](void*)->unsigned char{return 4;};equipped.type=4;equipped.mode=1;player={};player.cls=0;player.id=42;player.stats[6]=1000;player.stats[8]=100*256;player.stats[9]=900*256;player.x=20;player.y=20;Frame(100);
 AmazonMasteries::Players={};AmazonUnity::Session=nullptr;
 for(auto w:AmazonMasteries::Weapons)if(w.kind==1)weaponCode=w.code;
 Curses::X=[](void* u){return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u){return static_cast<Unit*>(u)->y;};
 Curses::Find=[](void*,int type,unsigned id)->void*{if(type==0&&id==player.id)return &player;for(auto& u:spirits)if(type==1&&u.type==1&&u.id==id)return &u;return nullptr;};
 SoulHarvest::Room=[](void* u)->void*{return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(u)->room));};
 League::RoomAt=[](void* r,int,int)->void*{return r;};
 League::FreePosition=[](void* r,League::Point*,League::Point*,int,int,int,int)->void*{return r;};
 MonsterRuntime::Spawn=[](void*,void*,int x,int y,int cls,int,int,unsigned short)->void*{if(spawnCount==failAt)return nullptr;auto& u=spirits[spawnCount++%6];u={};u.type=1;u.cls=cls;u.id=100+spawnCount;u.x=x;u.y=y;return &u;};
 Barbarian::RoomDelete=[](void*,unsigned,unsigned){};
 Barbarian::DeleteUnit=[](void*,void* u){static_cast<Unit*>(u)->type=99;++removed;};
 Barbarian::ModeInfo=[](void*,int,void*){};Barbarian::ModeChange=[](void*,void*,int){return 1;};
 GuardianCombat::ToggleState=[](void*,int,int){};
 Pulse::OriginalCreate=[](void*,void* raw)->void*{auto p=static_cast<Pulse::Params*>(raw);assert(p->owner==&player&&p->target==nullptr);auto nearest=static_cast<Unit*>(AmazonUnity::NearestEnemy(game,p->origin));assert(nearest&&p->targetX==nearest->x&&p->targetY==nearest->y);assert(p->origin!=&player);shotOrigin=p->origin;assert(p->x==static_cast<Unit*>(shotOrigin)->x&&p->y==static_cast<Unit*>(shotOrigin)->y);unityArrow={};unityArrow.type=3;unityArrow.cls=AmazonUnity::Arrow;unityArrow.stats[21]=400*256+128;unityArrow.stats[22]=800*256+256;unityArrow.stats[56]=100;shotFrames.push_back(Ramp::Frame(game));++shots;return &unityArrow;};
 Amazon::PublishMissiles=[](void* visualOwner,void**,unsigned char,void* origin,void*,unsigned short){assert(visualOwner==shotOrigin&&visualOwner!=&player);assert(origin==shotOrigin);volleyOrigins.push_back(origin);assert(unityArrow.stats[21]==400*256*19/100+128&&unityArrow.stats[22]==800*256*19/100+256&&unityArrow.stats[56]==100);};
 Curses::Record=[](unsigned char,int)->void*{static unsigned char row[0x2ec]{};*reinterpret_cast<int*>(row+0x190)=901;*reinterpret_cast<int*>(row+0x194)=902;*reinterpret_cast<int*>(row+0x19c)=903;return row;};
 Curses::Evaluate=[](unsigned char,void*,int formula,int,int){return formula==901?128:formula==902?256:19;};
 Curses::Owner=[](void*,void*)->void*{return nullptr;};
 for(int i=0;i<6;++i){auto& e=enemies[i];e={};e.type=1;e.id=500+i;e.stats[6]=1000;e.x=20+2*AmazonUnity::Offsets[i][0];e.y=20+2*AmazonUnity::Offsets[i][1];void* ptr=&e;std::memcpy(game+0x2630+8*i,&ptr,8);}
 // Selected mouse enemy is away from the caster; it must not move the ring.
 Assassin::GetTarget=[](void*,void*)->void*{return &enemies[3];};
 assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==1);assert(spawnCount==6&&shots==1&&player.stats[8]==75*256);
 for(auto& u:spirits)assert(AmazonUnity::Ghost(&u));assert(!AmazonUnity::Ghost(&player));
 assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==0);
 for(int i=0;i<6;++i){assert(spirits[i].x==20+AmazonUnity::Offsets[i][0]&&spirits[i].y==20+AmazonUnity::Offsets[i][1]);assert(AmazonUnity::NearestEnemy(game,&spirits[i])==&enemies[i]);}
 // Reacquire independently: dead, friendly and other-area targets are rejected.
 enemies[0].stats[6]=0;assert(AmazonUnity::NearestEnemy(game,&spirits[0])!=&enemies[0]);enemies[0].stats[6]=1000;
 enemies[0].stats[172]=1;assert(AmazonUnity::NearestEnemy(game,&spirits[0])!=&enemies[0]);enemies[0].stats[172]=0;
 enemies[0].room=2;assert(AmazonUnity::NearestEnemy(game,&spirits[0])!=&enemies[0]);enemies[0].room=1;
 int oldx=enemies[0].x;enemies[0].x=1000;assert(AmazonUnity::NearestEnemy(game,&spirits[0])!=&enemies[0]);enemies[0].x=oldx;
 // Moving the caster cannot move the stationary formation.
 player.x=21;
 for(unsigned f=101;f<225;++f){Frame(f);AmazonUnity::Frame(game);AmazonUnity::Frame(game);}
 assert(shots==60&&removed==0);
 for(int v=0;v<10;++v)for(int a=0;a<6;++a)for(int b=a+1;b<6;++b)assert(volleyOrigins[v*6+a]!=volleyOrigins[v*6+b]);
 for(unsigned i=0;i<60;++i){
  assert(shotFrames[i]==100+(i*25+11)/12);
  assert(volleyOrigins[i]==&spirits[i%6]);
  if(i)assert(shotFrames[i]>shotFrames[i-1]);
 }
 assert(shotFrames.back()==223);
 Frame(225);AmazonUnity::Frame(game);assert(removed==6);
 Frame(349);assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==0);
 player.x=20;Frame(350);spawnCount=0;failAt=3;
 assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==0);assert(removed==9&&shots==60&&player.stats[8]==75*256);
 spawnCount=0;failAt=-1;assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==1);
 player.stats[6]=0;AmazonUnity::Frame(game);assert(removed==15);
 player.stats[6]=1000;Frame(600);spawnCount=0;assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==1);
 player.room=2;AmazonUnity::Frame(game);assert(removed==21);
 for(auto& e:enemies)e.stats[6]=0;
 assert(!AmazonUnity::NearestEnemy(game,&spirits[0]));
 AmazonUnity::Formation empty;empty.actors[0]=spirits[0].id;int before=shots;
 AmazonUnity::ShootNext(game,&player,empty);assert(shots==before&&empty.shot==1);
 // A nearby free tile returned by native placement is accepted, including
 // a missing ideal room. All arrows must originate at those actual positions.
 player.room=1;Frame(850);spawnCount=0;
 for(auto& e:enemies)e.stats[6]=1000;
 League::RoomAt=[](void* r,int x,int y)->void*{return x==28&&y==20?nullptr:r;};
 League::FreePosition=[](void* r,League::Point* at,League::Point*,int,int,int,int)->void*{++at->x;return r;};
 int beforePlaced=shots;assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==1);
 assert(shots==beforePlaced+1&&spawnCount==6);
 for(int i=0;i<6;++i)for(int j=i+1;j<6;++j)assert(spirits[i].x!=spirits[j].x||spirits[i].y!=spirits[j].y);
 for(unsigned frame=851;frame<975;++frame){Frame(frame);AmazonUnity::Frame(game);}
 assert(shots==beforePlaced+60);Frame(975);AmazonUnity::Frame(game);
 // Fully blocked terrain fails before spawning or spending mana/cooldown.
 Frame(1100);player.stats[8]=100*256;player.stats[9]=900*256;spawnCount=0;
 League::FreePosition=[](void*,League::Point*,League::Point*,int,int,int,int)->void*{return nullptr;};
 assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==0);
 assert(spawnCount==0&&player.stats[8]==100*256);
 League::FreePosition=[](void* r,League::Point*,League::Point*,int,int,int,int)->void*{return r;};
 assert(AmazonUnity::Cast(game,&player,AmazonUnity::Skill,1)==1);
 assert(player.stats[9]==900*256);
 puts("PASS: Unity nearby placement fallback, actual arrow origins, blocked-terrain rollback, player-centred hexagon, independent nearest-enemy selection, 60 individually timed clockwise arrows, damage scaling, cooldown, rollback, death and area cleanup.");
}
