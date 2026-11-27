#define LEAGUE_TEST_EMBED
#include "runtime_test.cpp"
#undef main
int main(){
 ExistingLeagueTests();League::BindTraits();reset();LeagueTraits::Seed=12345;
 int counts[7]{};for(int i=0;i<30000;++i){int cls=LeagueTraits::Choose(League::Classes[0][0][0]);auto c=LeagueTraits::Info(cls);++counts[c?c->trait:0];}
 for(int n:{counts[0],counts[1],counts[2]})assert(n>9500&&n<10500);
 unsigned seed=99;int swarm=0;for(int i=0;i<10000;++i){int n=League::beyondCount(seed,1);assert(n==4||(n>=10&&n<=15));swarm+=n>=10;}assert(swarm>1800&&swarm<2200);
 auto make=[&](int base,int trait){auto u=static_cast<Unit*>(LAlloc(1,LeagueTraits::Variant(base,trait),20,20,game,LRoom(&player),1,1,0,nullptr));u->stats[21]=100*256;u->stats[22]=200*256;LeagueTraits::Attach(game,u,&player,0);return u;};
 auto giant=make(League::Classes[0][0][0],LeagueTraits::Giant);assert(giant->stats[7]==2500&&giant->stats[67]==-35);
 auto plague=make(League::Classes[0][0][0],LeagueTraits::Plague);assert(plague->stats[57]==1536&&plague->stats[58]==1536&&plague->stats[59]==100);
 alignas(8) unsigned char packet[0x180]{};auto damage=[&](void* a,void* d){std::memset(packet,0,sizeof(packet));int amount=100;unsigned flags=1;std::memcpy(packet+4,&flags,4);std::memcpy(packet+0x18,&amount,4);std::memcpy(packet+0x134,&amount,4);LeagueTraits::Damage(game,a,d,packet);return Barbarian::ReadDamage(packet,0x134);};
 assert(damage(giant,&player)==150);
 auto ward=make(League::Classes[1][0][0],LeagueTraits::Warden);assert(damage(&player,ward)==50);ward->x+=13;assert(damage(&player,ward)==100);
 auto hunter=make(League::Classes[2][0][0],LeagueTraits::Hunter);Frame(200);damage(hunter,&player);LeagueTraits::Frame(game);assert(hunter->stats[67]==60);LeagueTraits::Frame(game);assert(hunter->stats[67]==60);Frame(326);LeagueTraits::Frame(game);assert(hunter->stats[67]==0);
 reset();League::Event event;event.state.kind=League::Kind::Breach;event.state.area=2;event.state.activate(100);League::Events.push_back(event);
 auto brood=make(League::Classes[1][0][0],LeagueTraits::Brood);LeagueTraits::Death(brood->id,brood->cls);brood->stats[6]=0;assert(LeagueTraits::Busy(0));
 blocked=true;Frame(110);LeagueTraits::Frame(game);assert(LeagueTraits::Busy(0)&&League::Mobs.empty());blocked=false;
 Frame(120);LeagueTraits::Frame(game);Frame(130);LeagueTraits::Frame(game);assert(League::Mobs.size()==2&&!LeagueTraits::Busy(0));
 for(auto m:League::Mobs){auto& u=units.at(m.id);assert(u.stats[7]==350&&u.stats[67]==60&&u.stats[68]==30&&u.stats[13]==0);assert(damage(&u,&player)==50);LeagueTraits::Death(u.id,u.cls);u.stats[6]=0;}
 Frame(140);LeagueTraits::Frame(game);assert(League::Mobs.size()==2);
 reset();Arena::Current.start(1,player.id,73);Arena::Current.bonuses[int(Arena::Mod::LeagueWave)]=1;Arena::PrepareLeagueWave();int kind=LeagueTraits::Kind(Arena::WaveClasses[0]);for(int cls:Arena::WaveClasses)assert(LeagueTraits::Kind(cls)==kind);assert(!Arena::BossWave(Arena::Current));
 reset();player.room=Arena::FirstLevel;player.x=player.y=10080;Arena::Game=game;Arena::Current.start(1,player.id,73);Arena::Current.bonuses[int(Arena::Mod::LeagueWave)]=1;Arena::Current.phase=Arena::Phase::Fighting;Arena::Enemies.clear();Arena::SpawnRecords.clear();
 auto trialBrood=make(League::Classes[1][0][0],LeagueTraits::Brood);auto actor=LeagueTraits::Find(trialBrood);actor->x=actor->y=10080;actor->event=~0u;trialBrood->x=trialBrood->y=10080;trialBrood->stats[6]=0;LeagueTraits::Death(trialBrood->id,trialBrood->cls);assert(Arena::LeagueBusy());
 Frame(150);LeagueTraits::Frame(game);assert(Arena::LeagueBusy());Frame(160);LeagueTraits::Frame(game);assert(!Arena::LeagueBusy()&&Arena::Enemies.size()==2&&Arena::SpawnRecords.size()==2);
 for(auto id:Arena::Enemies){auto u=LFind(game,1,id);assert(Arena::TrackedClass(static_cast<Unit*>(u)->cls));assert(Arena::SafePosition(LX(u),LY(u)));}
 puts("PASS: exclusive thirds, poison, giant life/damage/slow, distance-based Warden, timed nonstacking Blood Hunter, exactly two weak fast no-recursion children, blocked retries, 20% Beyond swarms and pure League Trial roster.");
}
