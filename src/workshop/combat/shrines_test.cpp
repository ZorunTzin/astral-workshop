#define main ExistingAuraTests
#include "aura_test.cpp"
#undef main
std::map<unsigned,Unit> mobs;
unsigned serial=100,eliteCalls=0,blastCalls=0;
bool blocked=false;
void* __fastcall SFind(void*,int t,unsigned id){if(t==0&&id==player.id)return &player;auto i=mobs.find(id);return i!=mobs.end()?&i->second:nullptr;}
int __fastcall SX(void* p){return static_cast<Unit*>(p)->x;}
int __fastcall SY(void* p){return static_cast<Unit*>(p)->y;}
int __fastcall SLevel(const void* p) noexcept{return static_cast<const Unit*>(p)->room;}
void* __fastcall SRoom(void* p){return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(p)->room));}
void* __fastcall SAt(void* r,int,int){return blocked?nullptr:r;}
void* __fastcall SFree(void* r,League::Point*,League::Point*,int,int,int,int){return blocked?nullptr:r;}
void* __fastcall SSpawn(void*,void* room,int x,int y,int cls,int,int,unsigned short){auto& u=mobs[++serial];u.id=serial;u.type=1;u.cls=cls;u.x=x;u.y=y;u.room=int(uintptr_t(room));u.stats[6]=u.stats[7]=1000*256;return &u;}
void __fastcall STarget(void*,void*){}
void __fastcall SPromote(void*,void*){++eliteCalls;}
void* __fastcall SOwner(void*,void*){return nullptr;}
void __fastcall SDamage(void*,void*,void* target,void* packet){assert(Barbarian::ReadDamage(packet,0x134)==100000*256);++blastCalls;static_cast<Unit*>(target)->stats[6]=0;}
int main(){
 setbuf(stdout,nullptr);ExistingAuraTests();std::memset(game,0,sizeof(game));Frame(100);player={};player.stats[6]=1000*256;player.stats[7]=1000*256;
 SoulHarvest::Refresh=nullptr;Curses::UnitTimer=nullptr;
 for(int state=128;state<=137;++state){auto& l=player.lists[state];l.state=state;l.expiry=7600;}
 player.lists[128].stats[171]=100;player.lists[137].stats[85]=50;
 WorkshopShrines::Player(game,&player);
 assert(GetUnit(&player,76,0)==20&&GetUnit(&player,74,0)==(1000*256)/2500);
 assert(GetUnit(&player,256,0)==25&&GetUnit(&player,257,0)==25);
 for(int s:{40,42,44,46})assert(GetUnit(&player,s,0)==10);
 assert(GetUnit(&player,105,0)==50);for(int s:{93,96,99,102})assert(GetUnit(&player,s,0)==30);
 assert(GetUnit(&player,79,0)==100&&GetUnit(&player,80,0)==50&&GetUnit(&player,85,0)==50);
 assert(GetUnit(&player,171,0)==100);
 for(int diff=0;diff<3;++diff){game[0x104]=diff;WorkshopShrines::Player(game,&player);WorkshopShrines::Player(game,&player);assert(GetUnit(&player,127,0)==diff);}
 assert(GetUnit(&player,93,0)==30);Frame(7600);WorkshopShrines::Player(game,&player);assert(GetUnit(&player,93,0)==0&&GetUnit(&player,76,0)==0&&GetUnit(&player,127,0)==0);
 Curses::Find=SFind;Curses::X=SX;Curses::Y=SY;Curses::Owner=SOwner;SoulHarvest::Room=SRoom;Arena::Level=SLevel;
 League::RoomAt=SAt;League::FreePosition=SFree;MonsterRuntime::Spawn=SSpawn;WorkshopShrines::Promote=SPromote;Assassin::SetTarget=STarget;
 // Native objects use StaticPath (+0x10/+0x14), not DynamicPath fixed points.
 Unit shrine;shrine.type=2;alignas(8) unsigned char path[32]{};void* ptr=path;
 int ox=123,oy=456;std::memcpy(path+0x10,&ox,4);std::memcpy(path+0x14,&oy,4);std::memcpy(reinterpret_cast<unsigned char*>(&shrine)+0x38,&ptr,8);
 Assassin::NativePositions=true;assert(WorkshopShrines::ObjectCoordinate(&shrine,false)==123&&WorkshopShrines::ObjectCoordinate(&shrine,true)==456);Assassin::NativePositions=false;
 WorkshopShrines::Operation op{game,&player,&player};WorkshopShrines::Monster(0,&op,nullptr);
 blocked=true;WorkshopShrines::SpawnPacks(game);assert(eliteCalls==0&&WorkshopShrines::Packs.size()==1);
 blocked=false;WorkshopShrines::SpawnPacks(game);assert(eliteCalls==5&&WorkshopShrines::Packs.empty());WorkshopShrines::SpawnPacks(game);assert(eliteCalls==5);
 mobs.clear();serial=100;
 for(int i=0;i<4;++i){auto u=static_cast<Unit*>(SSpawn(game,SRoom(&player),player.x+i,player.y,0,0,0,0));void* ptr=u;std::memcpy(game+0x2630+8*(u->id&127),&ptr,8);if(i==1)u->stats[172]=1;if(i==2)u->x+=100;if(i==3)u->room=2;}
 Curses::OriginalDamage=SDamage;Curses::AdditionalIncomingHit=nullptr;Curses::AdditionalAfterDamage=nullptr;Curses::AdditionalIgnoreDamage=nullptr;
 WorkshopShrines::Blast(&op);assert(blastCalls==1);assert(player.stats[6]==1000*256);
 std::puts("PASS: shrine bonuses, native +2 skill delta, no stacking, expiration, blocked spawn retry, exactly five promotions, enemy-only 100k radius/area filtering.");
}
