#define main ExistingAuraTests
#include "aura_test.cpp"
#undef main
Unit dummy;int spawned=0,deleted=0;
int main(){
 ExistingAuraTests();std::memset(game,0,sizeof(game));player={};player.id=42;player.stats[6]=1000;player.room=1;
 void* ptr=&player;std::memcpy(game+0x2230,&ptr,8);
 Arena::Level=[](const void* u) noexcept {return static_cast<const Unit*>(u)->room;};
 SoulHarvest::Room=[](void* u)->void*{return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(u)->room));};
 Curses::Find=[](void*,int type,unsigned id)->void*{if(type==0&&id==42)return &player;if(type==1&&id==dummy.id&&dummy.type==1)return &dummy;return nullptr;};
 League::RoomAt=[](void* r,int,int)->void*{return r;};League::FreePosition=[](void* r,League::Point*,League::Point*,int,int,int,int)->void*{return r;};
 MonsterRuntime::Spawn=[](void*,void*,int x,int y,int cls,int,int,unsigned short)->void*{dummy={};dummy.type=1;dummy.cls=cls;dummy.id=100+ ++spawned;dummy.room=2;dummy.x=x;dummy.y=y;return &dummy;};
 Barbarian::RoomDelete=[](void*,unsigned,unsigned){};Barbarian::DeleteUnit=[](void*,void* u){static_cast<Unit*>(u)->type=99;++deleted;};
 Curses::X=[](void* u){return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u){return static_cast<Unit*>(u)->y;};TrainingDummy::Game=nullptr;Frame(10);TrainingDummy::Frame(game);assert(spawned==0);
 player.room=2;Frame(15);TrainingDummy::Frame(game);assert(spawned==1&&dummy.stats[6]==256000000&&dummy.stats[7]==256000000);
 const int x=dummy.x,y=dummy.y;dummy.stats[6]=12345;
 Frame(260);TrainingDummy::Frame(game);assert(dummy.stats[6]==12345&&spawned==1);
 Frame(265);TrainingDummy::Frame(game);assert(dummy.stats[6]==256000000);
 // A killed target returns at the same anchor at the next ten-second reset.
 dummy.stats[6]=0;dummy.mode=12;Frame(515);TrainingDummy::Frame(game);assert(spawned==2&&deleted==1&&dummy.x==x&&dummy.y==y&&dummy.stats[6]==256000000);
 player.room=1;Frame(520);TrainingDummy::Frame(game);player.room=2;Frame(525);TrainingDummy::Frame(game);assert(spawned==2);
 Frame(1);TrainingDummy::Frame(game);assert(!TrainingDummy::Placed&&spawned==2);
 puts("PASS: dummy entrance spawn, exact HP, ten-second heal, death respawn at fixed anchor, no duplicates and session reset.");
}
