#define main BaseAuraTests
#include "aura_test.cpp"
#undef main
Unit arrows[31],targets[2];int created=1,packets=0;
int main(){
 BaseAuraTests();std::memset(game,0,sizeof(game));player={};player.cls=0;player.id=42;player.stats[6]=1000;
 Curses::X=[](void* u){return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u){return static_cast<Unit*>(u)->y;};
 Curses::Owner=[](void*,void*)->void*{return nullptr;};SoulHarvest::Room=[](void* u)->void*{return reinterpret_cast<void*>(uintptr_t(static_cast<Unit*>(u)->room));};
 Curses::Find=[](void*,int type,unsigned id)->void*{if(type==0&&id==42)return &player;for(auto& a:arrows)if(type==3&&a.id==id)return &a;return nullptr;};
 for(int i=0;i<2;++i){auto& t=targets[i];t={};t.type=1;t.id=500+i;t.x=25+i*3;t.stats[6]=1000;void* ptr=&t;std::memcpy(game+0x2630+8*i,&ptr,8);}
 Amazon::OriginalHit=[](void*,void*,void*,int){return 1;};
 Amazon::PublishMissiles=[](void* anchor,void** m,unsigned char n,void* origin,void*,unsigned short token){assert(n==1&&anchor==m[0]&&origin==anchor&&token==0);++packets;};
 Pulse::OriginalCreate=[](void*,void* raw)->void*{auto p=static_cast<Pulse::Params*>(raw);assert(created<31);auto& a=arrows[created];a={};a.type=3;a.id=100+created++;a.cls=p->missile;a.x=p->x;a.y=p->y;return &a;};
 Frame(100);Amazon::Session=nullptr;Amazon::Reset(game);
 auto& first=arrows[0];first.type=3;first.id=100;first.cls=Amazon::Split0;first.stats[21]=321;first.stats[48]=99;first.stats[57]=90;first.stats[56]=75;first.stats[59]=125;
 Pulse::Params p{};p.owner=&player;p.missile=first.cls;p.level=1;Amazon::Created(game,&p,&first);
 assert(!Amazon::Flights[100].published);Amazon::PublishFlight(game,&first);Amazon::PublishFlight(game,&first);assert(packets==1);
 int begin=0,count=1;
 for(int stage=0;stage<5;++stage){
  int before=created;
  for(int i=begin;i<begin+count;++i){auto& a=arrows[i];p.missile=a.cls;Amazon::Created(game,&p,&a);Amazon::BeforeTick(game,&a);Amazon::BeforeTick(game,&a);assert(Amazon::Stage(a.cls)==stage&&a.stats[21]==53);assert(a.stats[48]==16&&a.stats[57]==15&&a.stats[56]==75&&a.stats[59]==125);Amazon::MissileHit(game,&a,&targets[stage%2],0);Amazon::MissileHit(game,&a,&targets[stage%2],0);}
  Frame(101+stage);Amazon::Frame(game);
  assert(created-before==(stage<4?count*2:0));begin=before;count*=2;
 }
 assert(created==31);puts("PASS: initial split visual once, impact-anchored child packets, 1/2/4/8/16 stages, inherited payload and terminal stop.");
}
