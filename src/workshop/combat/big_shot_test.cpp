#define main ExistingAuraTests
#include "aura_test.cpp"
#undef main
Unit carrier,extraCarrier,ring,impact,weapon,enemies[6];bool failCreate=false,useExtra=false;int direct=0,area=0,visuals=0,deleted=0;unsigned code=0;
int main(){
 assert(BigShot::ManaCost(1)==2560&&BigShot::ManaCost(2)==2688&&BigShot::ManaCost(25)==5632);
 ExistingAuraTests();std::memset(game,0,sizeof(game));player={};player.cls=0;player.id=42;player.stats[6]=1000;player.stats[8]=100*256;player.stats[9]=900*256;
 for(auto w:AmazonMasteries::Weapons)if(w.kind==1)code=w.code;
 LeagueStorage::Inventory=[](void*)->void*{return &player;};LeagueStorage::First=[](void*)->void*{return &weapon;};LeagueStorage::Next=[](void*)->void*{return nullptr;};LeagueStorage::Code=[](void*){return code;};AmazonMasteries::Body=[](void*)->unsigned char{return 4;};weapon.type=4;weapon.mode=1;
 Curses::X=[](void* u){return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u){return static_cast<Unit*>(u)->y;};
 SoulHarvest::Room=[](void* u)->void*{return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(u)->room));};
 Curses::Owner=[](void*,void*)->void*{return nullptr;};
 Curses::Find=[](void*,int type,unsigned id)->void*{if(type==0&&id==42)return &player;if(type==3&&id==100&&carrier.type==3)return &carrier;if(type==3&&id==101&&extraCarrier.type==3)return &extraCarrier;for(auto& e:enemies)if(type==1&&e.id==id)return &e;return nullptr;};
 Assassin::GetTarget=[](void*,void*)->void*{return &enemies[0];};
 Barbarian::RoomDelete=[](void*,unsigned,unsigned){};Barbarian::DeleteUnit=[](void*,void* u){static_cast<Unit*>(u)->type=99;++deleted;};
 Curses::Record=[](unsigned char,int)->void*{static unsigned char row[0x2ec]{};*reinterpret_cast<int*>(row+0x19c)=901;*reinterpret_cast<int*>(row+0x198)=904;*reinterpret_cast<int*>(row+0x190)=902;*reinterpret_cast<int*>(row+0x194)=903;return row;};
 Curses::Evaluate=[](unsigned char,void*,int formula,int,int level){return formula==902?128:formula==903?256:(95+5*level)*148/100;};
 Pulse::OriginalCreate=[](void*,void* raw)->void*{auto p=static_cast<Pulse::Params*>(raw);
  if(p->missile==BigShot::Arrow){if(failCreate)return nullptr;assert(p->x==20&&p->y==20&&p->targetX==120&&p->targetY==20&&p->range==125);auto& c=useExtra?extraCarrier:carrier;c={};c.type=3;c.cls=BigShot::Arrow;c.id=useExtra?101:100;c.stats[21]=100*256+128;c.stats[22]=200*256+256;return &c;}
  if(p->missile==BigShot::Nova){auto c=static_cast<Unit*>(p->origin);assert((c==&carrier||c==&extraCarrier)&&p->x==c->x&&p->y==c->y&&p->range==8);ring={};ring.type=3;ring.cls=BigShot::Nova;return &ring;}
  assert(p->missile==BigShot::HitMissile&&p->target);impact={};impact.type=3;impact.cls=BigShot::HitMissile;impact.stats[60]=7;impact.stats[62]=5;return &impact;
 };
 Amazon::PublishMissiles=[](void* owner,void** shots,unsigned char,void* origin,void*,unsigned short){if(shots[0]==&ring){assert((owner==&carrier||owner==&extraCarrier)&&origin==owner);++visuals;}else assert(owner==&player&&(shots[0]==&carrier||shots[0]==&extraCarrier));};
 Curses::NativeMissileHit=[](void*,void* m,void* target,int)->int{assert(target==&enemies[0]||target==&enemies[1]||target==&enemies[2]);assert(GetUnit(m,60,0)==7&&GetUnit(m,62,0)==5);int damage=GetUnit(m,21,0);if(damage==148*256/3+128)++direct;else {assert(damage==(148*256/3+128)/2);++area;}return 1;};
 for(int i=0;i<6;++i){auto& e=enemies[i];e={};e.type=1;e.id=500+i;e.stats[6]=1000;e.x=25;e.y=20;void* ptr=&e;std::memcpy(game+0x2630+8*i,&ptr,8);}
 enemies[1].y=24;enemies[2].y=29;enemies[5].y=30;enemies[3].stats[172]=1;enemies[4].room=2;
 Frame(100);BigShot::Game=nullptr;failCreate=true;assert(BigShot::Cast(game,&player,BigShot::Skill,1)==0&&player.stats[8]==100*256);
 player.stats[8]=10*256-1;failCreate=false;assert(BigShot::Cast(game,&player,BigShot::Skill,1)==0&&player.stats[9]==900*256);player.stats[8]=100*256;
 failCreate=false;assert(BigShot::Cast(game,&player,BigShot::Skill,1)==1&&player.stats[8]==90*256);
 carrier.x=24;Frame(101);BigShot::Frame(game);BigShot::Frame(game);assert(direct==1&&area==0);
 carrier.x=25;
 for(unsigned f=102;f<225;++f){Frame(f);BigShot::Frame(game);BigShot::Frame(game);}
 assert(direct==2&&area==30&&visuals==160);Frame(225);BigShot::Frame(game);assert(deleted==1);
 // Immediate and overlapping casts must both succeed without replacing a flight.
 Frame(226);player.stats[8]=100*256;player.stats[9]=900*256;
 assert(BigShot::Cast(game,&player,BigShot::Skill,1)==1);
 useExtra=true;assert(BigShot::Cast(game,&player,BigShot::Skill,1)==1);
 assert(player.stats[8]==80*256&&BigShot::Flights[0].active&&BigShot::Flights[1].active);
 carrier.type=99;Frame(227);BigShot::Frame(game);
 assert(!BigShot::Flights[0].active&&BigShot::Flights[1].active);
 extraCarrier.x=25;Frame(238);BigShot::Frame(game);assert(visuals==176);
 player.stats[6]=0;Frame(239);BigShot::Frame(game);assert(deleted==2);
 // The client keeps normal arrows unchanged, but Big Shot must be allowed
 // to self-remove at zero lifetime even without a later server delete packet.
 BigShot::OriginalClientTick=[](void* u,float elapsed){
  auto m=static_cast<Unit*>(u);unsigned flags=0;std::memcpy(&flags,static_cast<unsigned char*>(u)+0x124,4);
  if(m->cls==BigShot::Arrow){assert(flags==0x00200040u);m->stats[999]-=int(elapsed);if(m->stats[999]<=0)m->type=99;}
  else assert(flags==0x40u);
 };
 Unit visual;visual.type=3;visual.cls=BigShot::Arrow;visual.stats[999]=125;unsigned flags=0x40;
 std::memcpy(reinterpret_cast<unsigned char*>(&visual)+0x124,&flags,4);
 for(int i=0;i<124;++i)BigShot::ClientTick(&visual,1.0f);
 assert(visual.type==3&&visual.stats[999]==1);BigShot::ClientTick(&visual,1.0f);assert(visual.type==99);
 Unit ordinary;ordinary.type=3;ordinary.cls=Amazon::Split0;
 std::memcpy(reinterpret_cast<unsigned char*>(&ordinary)+0x124,&flags,4);BigShot::ClientTick(&ordinary,1.0f);
 assert(BigShot::SegmentDistance(5,2,0,0,10,0)==4);assert(BigShot::SegmentDistance(12,0,0,0,10,0)==4);
 // A level-2 cast requires and consumes exactly 10.5 mana, not 10 or 11.
 player.stats[6]=1000;player.stats[8]=2687;BigShot::Flights={};
 assert(BigShot::Cast(game,&player,BigShot::Skill,2)==0&&player.stats[8]==2687);
 player.stats[8]=2688;assert(BigShot::Cast(game,&player,BigShot::Skill,2)==1&&player.stats[8]==0);
 assert(player.stats[9]==900*256);
 puts("PASS: Big Shot direction, failed cast rollback, no cooldown and concurrent casts, exact one-third weapon damage, swept width, single direct hits, ten moving novae, half damage, area/friendly filters, wall expiry, death cleanup and client expiry flag isolation.");
}
