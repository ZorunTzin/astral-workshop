#define main OriginalAuraTest
#include "aura_test.cpp"
#undef main
#include <set>
static unsigned VisualPackets=0;
static bool SteeringReady=false;
static Unit VisualArrow;
static unsigned Landings=0;
int main(){
 OriginalAuraTest();
 Curses::X=[](void* u)->int{return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u)->int{return static_cast<Unit*>(u)->y;};
 // A simultaneous native fan must get distinct directions, while another
 // owner and a later attack begin their own independent fan sequence.
 Frame(100);Amazon::Session=nullptr;std::set<std::pair<int,int>> directions;
 for(int n=0;n<7;++n){Pulse::Params p{};p.owner=&player;p.missile=Amazon::ChargedBolt;p.skill=24;p.level=20;p.target=&mercenary;
  assert(Amazon::Filter(game,&p));assert(p.initializeFunction==0);assert(!p.target&&p.x==player.x&&p.y==player.y);directions.emplace(p.targetX,p.targetY);
 }
 assert(directions.size()==7);
 Pulse::Params toxic{};toxic.owner=&player;toxic.missile=Amazon::Toxic;toxic.skill=14;toxic.level=20;
 Amazon::Filter(game,&toxic);assert(toxic.initializeFunction==0&&toxic.initializeArgument==0&&toxic.velocity==8192);
 // A native end event schedules exactly one return, without a collision or
 // a lookup in the item/missile unit tables. The snapshot survives destruction.
 Amazon::Flights={};Amazon::Pending={};
 Unit arrow;arrow.type=3;arrow.cls=Amazon::MultiArrow;arrow.id=123;arrow.x=45;arrow.y=29;
 Pulse::Params multi{};multi.owner=&player;multi.missile=Amazon::MultiArrow;multi.level=1;
 Amazon::Created(game,&multi,&arrow);
 // Model the native skill applying damage after the create callback.
 arrow.stats[21]=246;arrow.stats[22]=480;
 Amazon::BeforeTick(game,&arrow);
 Amazon::FinishTick(game,123,1);assert(Amazon::Pending[0].owner==Amazon::Invalid);
 Amazon::FinishTick(game,123,2);assert(Amazon::Pending[0].owner==player.id&&Amazon::Pending[0].x==45);
 Amazon::FinishTick(game,123,2);assert(Amazon::Pending[1].owner==Amazon::Invalid);
 assert(Amazon::Pending[0].damage[0]==246&&Amazon::Pending[0].damage[1]==480);
 // Exercise the real native-tick wrapper: the original callback may clear
 // missile identity, but the end event must still use the pre-call snapshot.
 Amazon::Flights={};Amazon::Pending={};Amazon::Created(game,&multi,&arrow);
 Curses::BeforeMissileTick=Amazon::BeforeTick;Curses::AfterMissileTick=Amazon::FinishTick;
 Curses::MissileTick(game,&arrow,[](void*,void* raw)->int{auto* u=static_cast<Unit*>(raw);u->id=0;u->x=0;return 2;});
 assert(Amazon::Pending[0].missile==123&&Amazon::Pending[0].x==45);
 Curses::BeforeMissileTick=nullptr;Curses::AfterMissileTick=nullptr;
 // Half-damage returns halve cold damage too, but never shorten poison or
 // freeze duration; full-damage children copy every channel without compounding.
 Unit missile;Amazon::Impact impact;
 for(auto& value:impact.damage)value=200;
 Amazon::CopyDamage(&missile,impact,50);
 assert(missile.stats[21]==100&&missile.stats[54]==100&&missile.stats[55]==100);
 assert(missile.stats[56]==200&&missile.stats[59]==200);
 Amazon::CopyDamage(&missile,impact,100);assert(missile.stats[54]==200);
 assert(Amazon::Stage(Amazon::Split3)==3&&Amazon::Stage(Amazon::Return)==-1);
 // A single native volley publication owns client enumeration. No game-unit
 // hash traversal, prediction token, origin override or duplicate damage shot.
 Amazon::PublishMissiles=[](void* owner,void** arrows,unsigned char count,void* origin,void* target,unsigned short token){
  assert(owner==&player&&count==1&&arrows[0]==&VisualArrow);
  assert(!origin&&token==0&&SteeringReady);
  assert(!target);++VisualPackets;
 };
 SteeringReady=true;Amazon::PublishVisual(game,&player,&VisualArrow);assert(VisualPackets==1);
 Amazon::PublishVisual(game,&mercenary,&VisualArrow);assert(VisualPackets==1);
 Amazon::PublishVisual(game,&player,nullptr);assert(VisualPackets==1);
 // A child is one real arrow created at the impact and then published,
 // never a second cast from the player. It inherits all damage channels.
 Pulse::OriginalCreate=[](void*,void* raw)->void*{
  auto* p=static_cast<Pulse::Params*>(raw);
  assert(p->missile==Amazon::Split1&&!p->target);
  assert(p->targetX==mercenary.x&&p->targetY==mercenary.y);
  VisualArrow.type=3;VisualArrow.id=998;VisualArrow.cls=Amazon::Split1;
  return &VisualArrow;
 };
 Amazon::Shoot(game,&player,&mercenary,impact,Amazon::Split1,22,100);
 assert(VisualPackets==2&&VisualArrow.stats[21]==200);
 // Returns likewise publish only after their half-damage snapshot is applied.
 Pulse::OriginalCreate=[](void*,void* raw)->void*{auto* p=static_cast<Pulse::Params*>(raw);assert(p->missile==Amazon::Return&&!p->target);assert((p->flags&0x8000)&&!(p->flags&0x400));assert(p->range==Amazon::ReturnFrames(p->targetX-p->x,p->targetY-p->y));for(auto b:p->graphicsDescriptor)assert(b==0);VisualArrow.cls=Amazon::Return;return &VisualArrow;};
 Amazon::Shoot(game,&player,&player,impact,Amazon::Return,12,50);
 assert(VisualPackets==3&&VisualArrow.stats[21]==100);
 Amazon::PublishMissiles=nullptr;Curses::Cast=nullptr;Pulse::OriginalCreate=nullptr;
 // Pounce uses the selected enemy coordinates, not the player's walk target.
 Curses::Owner=[](void*,void*)->void*{return nullptr;};
 SoulHarvest::Room=[](void* u)->void*{return reinterpret_cast<void*>(std::uintptr_t(static_cast<Unit*>(u)->room));};
 Unit victim;victim.type=1;victim.id=33;victim.x=70;victim.y=80;victim.stats[6]=1000;
 Amazon::Relocate=[](void* g,void* u,void* room,int x,int y,int a,int b){
  assert(g==game&&!room&&!a&&!b&&x==70&&y==80);++Landings;
  auto* actor=static_cast<Unit*>(u);actor->x=x-1;actor->y=y;
 };
 assert(Amazon::LandAtTarget(game,&player,&victim)&&Landings==1);
 victim.room=2;assert(!Amazon::LandAtTarget(game,&player,&victim)&&Landings==1);
 victim.room=1;victim.stats[6]=0;assert(!Amazon::LandAtTarget(game,&player,&victim)&&Landings==1);
 victim.stats[6]=1000;player.x=20;player.y=20;
 Amazon::Relocate=[](void*,void*,void*,int,int,int,int){};
 assert(!Amazon::LandAtTarget(game,&player,&victim)); // rejected landing cannot release nova
 Amazon::Relocate=nullptr;
 // Ground targeting chooses the nearest live hostile, rejecting other areas
 // and candidates beyond the twelve-yard search; exact-boundary hits count.
 void* nearest=nullptr;double distance=Amazon::PounceSearchRadius*Amazon::PounceSearchRadius;
 victim.x=38;victim.y=20;victim.stats[6]=1000;
 Amazon::ConsiderPounceTarget(game,&player,&victim,nearest,distance);assert(nearest==&victim);
 Unit closer;closer.type=1;closer.id=34;closer.x=25;closer.y=20;closer.stats[6]=1000;
 Amazon::ConsiderPounceTarget(game,&player,&closer,nearest,distance);assert(nearest==&closer&&distance==25);
 closer.stats[6]=0;nearest=nullptr;distance=324;
 Amazon::ConsiderPounceTarget(game,&player,&closer,nearest,distance);assert(!nearest);
 victim.x=39;Amazon::ConsiderPounceTarget(game,&player,&victim,nearest,distance);assert(!nearest);
 victim.x=25;victim.room=2;Amazon::ConsiderPounceTarget(game,&player,&victim,nearest,distance);assert(!nearest);
 assert(Amazon::PounceRadius==9.0);

 // Both movement and frame fallback may see the same outgoing arrow.
 // They must publish one visual only, without creating another damage unit.
 Amazon::Flights={};VisualArrow.type=3;VisualArrow.id=991;VisualArrow.cls=Amazon::MultiArrow;
 Amazon::Created(game,&multi,&VisualArrow);VisualPackets=0;
 Amazon::PublishMissiles=[](void* owner,void** arrows,unsigned char count,void*,void*,unsigned short){
  assert(owner==&player&&count==1&&arrows[0]==&VisualArrow);++VisualPackets;
 };
 Amazon::PublishFlight(game,&VisualArrow);Amazon::PublishFlight(game,&VisualArrow);
 assert(VisualPackets==1&&Amazon::Flights[991%Amazon::Flights.size()].published);
 Amazon::PublishMissiles=nullptr;
 // Every axis/diagonal return stops at or before its fixed destination;
 // the remaining gap is less than one native movement step.
 for(int dx=-80;dx<=80;++dx)for(int dy=-80;dy<=80;++dy){
  const double d=std::hypot(double(dx),double(dy));if(d<1.125)continue;
  const double travelled=Amazon::ReturnFrames(dx,dy)*18.0/16.0;
  assert(travelled<=d+1e-9&&d-travelled<1.125+1e-9);
 }
 // Starting a new game or rolling back its frame clears pending IDs, avoiding
 // stale targets and projectiles leaking into a different session.
 Amazon::Pending[0].owner=123;Amazon::Flights[0].id=456;Frame(1);Amazon::Reset(game);
 assert(Amazon::Pending[0].owner==Amazon::Invalid&&Amazon::Flights[0].id==Amazon::Invalid);
 AuraBuffs::Players={};player.lists.clear();player.stats[6]=1000;player.stats[9]=500*256;player.stats[8]=500*256;
 AuraBuffs::FeaturePulse=Amazon::BuffPulse;
 for(int sid:{8,17,18,28}){player.levels[sid]=20;assert(AuraBuffs::Cost(AuraBuffs::Index(sid),20)==75);assert(AuraBuffs::Toggle(game,&player,sid,20)==1);}
 assert(AuraBuffs::Reservation(game,&player)==300*256);
 Frame(200);AuraBuffs::OnFrame(game);
 for(int sid:{8,17,18,28})assert(GetList(&player,AuraBuffs::Definitions[AuraBuffs::Index(sid)].state));
 for(int sid:{8,17,18,28})assert(AuraBuffs::Toggle(game,&player,sid,20)==1);
 assert(AuraBuffs::Reservation(game,&player)==0);
 std::puts("PASS: independent radial fan, native initializer removal, exactly-once end-of-flight return, return damage channels, session cleanup and four concurrent Amazon reservations.");
}
