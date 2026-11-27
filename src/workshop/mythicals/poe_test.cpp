#define main BaseAuraTests
#include "../combat/aura_test.cpp"
#undef main
#include <set>
Unit enemy,arrow;
int main(){
 BaseAuraTests();player={};enemy={};arrow={};std::memset(game,0,sizeof(game));Frame(100);
 player.id=42;player.stats[6]=player.stats[7]=1000*256;enemy.type=1;enemy.id=100;
 Curses::X=[](void* p){return static_cast<Unit*>(p)->x;};Curses::Y=[](void* p){return static_cast<Unit*>(p)->y;};
 Barbarian::AuraArea=[](const void* p)noexcept{return static_cast<const Unit*>(p)->room;};
 Arena::Level=[](const void* p)noexcept{return static_cast<const Unit*>(p)->room;};
 Curses::Find=[](void*,int type,unsigned id)->void*{for(auto p:{&player,&enemy,&arrow})if(int(p->type)==type&&p->id==id)return p;return nullptr;};
 using namespace PoeMythicals;Session=nullptr;Reset(game);unsigned char packet[0x180]{};
 player.stats[Sin]=1;Write(packet,0x18,100);Write(packet,0x20,200);Write(packet,0x2c,300);Write(packet,0x34,400);Write(packet,0x30,50);Write(packet,0x118,25);
 BeforeResolve(game,&player,&enemy,packet);assert(Read(packet,0x30)==950&&Read(packet,0x18)==100&&Read(packet,0x20)==0&&Read(packet,0x2c)==0&&Read(packet,0x34)==0&&Read(packet,0x118)==0);
 CastingSkill=38;Write(packet,0x20,200);Write(packet,0x30,0);BeforeResolve(game,&player,&enemy,packet);assert(Read(packet,0x30)==200&&Read(packet,0x20)==0);CastingSkill=-1;
 // The native adapter must receive neutral resistance while retaining absorbs.
 OriginalComponent=[](DamageInfo* info,const Component* c,int){assert(c->resist==-1&&info->defenderMonster==1);assert(info->reduction[2]==0);};
 int damage=1000;Component component{};component.value=&damage;component.resist=37;component.reduction=2;DamageInfo info{game,nullptr,&player,&enemy,0,1,packet,{0,0,100}};
 Resist(&info,&component,0);assert(damage==900&&component.resist==37&&info.reduction[2]==100);
 player.stats[Sin]=0;game[0x104]=2;enemy.stats[6]=100*256;
 Curses::Owner=[](void*,void*)->void*{return nullptr;};
 Curses::MinionOwner=[](void*)->void*{return nullptr;};
 for(int variant=1;variant<=4;++variant){
  player.stats[Prototype]=variant;int stat=PrototypeStat(&player);player.stats[stat]=25;enemy.stats[stat]=150;
  AuraTarget(game,&enemy,{&player});assert(Stat(&enemy,stat)==-75);
  for(int i=0;i<20;++i)AuraTarget(game,&enemy,{&player});assert(Stat(&enemy,stat)==-75);
  enemy.stats[stat]+=10;AuraTarget(game,&enemy,{&player});assert(Stat(&enemy,stat)==-75);
  player.stats[stat]=120;AuraTarget(game,&enemy,{&player});assert(Stat(&enemy,stat)==20);
  for(unsigned off:{0x18u,0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u})Write(packet,off,100);BeforeResolve(game,&player,&enemy,packet);for(unsigned off:{0x18u,0x20u,0x2cu,0x30u,0x34u})assert(Read(packet,off)==100);
  enemy.x+=20;AuraTarget(game,&enemy,{&player});assert(Stat(&enemy,stat)==160);enemy.x-=20;
  AuraTarget(game,&enemy,{&player});player.stats[Prototype]=0;AuraTarget(game,&enemy,{&player});assert(Stat(&enemy,stat)==160);
 }
 player.stats.clear();player.stats[6]=player.stats[7]=1000*256;player.stats[Dream]=1;player.stats[31]=1000;CastingSkill=-1;Write(packet,0x18,1000);Display(game,&player);assert(GetUnit(&player,25,0)==10);BeforeResolve(game,&player,&enemy,packet);assert(Read(packet,0x18)==1000);
 player.stats[Dream]=0;player.stats[Rathpith]=1;Display(game,&player);assert(GetUnit(&player,25,0)==0&&GetUnit(&player,314,0)==30);assert(Spell(38)&&!Spell(672)&&!Spell(58)&&!Spell(8));CastingSkill=38;Write(packet,0x20,1000);BeforeResolve(game,&player,&enemy,packet);assert(Read(packet,0x20)==1300);CastingSkill=-1;
 NativeSkills[0]=[](void*,void*,int,int){return 1;};Cast<0>(game,&player,38,1);assert(player.stats[6]==900*256&&player.stats[7]==1000*256);NativeSkills[0]=[](void*,void*,int,int){return 0;};Cast<0>(game,&player,38,1);assert(player.stats[6]==900*256);player.stats[6]=257;Sacrifice(&player);assert(player.stats[6]==256);
 player.stats[Rathpith]=0;Display(game,&player);assert(GetUnit(&player,314,0)==0&&GetUnit(&player,25,0)==0);assert(static_cast<List*>(GetList(&player,DisplayState))->expiry==Ramp::Frame(game)+1); player.stats[Rathpith]=0;player.stats[Aegis]=1;player.stats[6]=1000*256;player.stats[31]=1000;Blocked(game,&player);assert(Ward(&player)->value==20*256);std::memset(packet,0,sizeof(packet));packet[4]=1;Write(packet,0x18,30*256);Write(packet,0x134,30*256);Absorb(game,&player,packet);assert(Read(packet,0x18)==10*256&&Read(packet,0x134)==10*256&&Ward(&player)->value==0);Blocked(game,&player);Frame(226);Absorb(game,&player,packet);assert(Ward(&player)->value==0);
 Frame(300);player.stats[Aegis]=0;player.stats[Svalinn]=1;BlockRate=[](void*,int){return 75;};Moving=[](void*){return 0;};Dodge=[](void*,void*,int){return 0;};OriginalBlock=[](void*,void*,void*,int,int){return 7;};assert(CappedBlockRate(&player,1)==65);SetSelectors::Random=123456;int blocks=0;for(int i=0;i<10000;++i)blocks+=Block(game,&enemy,&player,1,1);assert(blocks>8500&&blocks<9000);player.stats[Svalinn]=0;assert(Block(game,&enemy,&player,1,1)==7);
 player.stats[Nimis]=1;arrow.type=3;arrow.id=400;arrow.cls=0;arrow.stats[21]=100;arrow.stats[56]=80;
 assert(Eligible(0)&&!Eligible(BigShot::HitMissile)&&!Eligible(AmazonUnity::Arrow)&&!Eligible(Amazon::Split0)&&Eligible(Amazon::MultiArrow)&&Eligible(Amazon::MultiBolt));
 Pulse::Params normal{};normal.owner=&player;normal.missile=Amazon::MultiArrow;normal.target=&enemy;normal.flags=0x402;Filter(game,&normal);assert(normal.target==&enemy&&normal.flags==0x402);
 ExtraSpawn=true;std::set<std::pair<int,int>> dirs;for(int i=0;i<20;++i){Pulse::Params p{};p.flags=0x402;p.target=&enemy;p.owner=&player;p.missile=0;p.skill=0;p.level=1;Filter(game,&p);dirs.emplace(p.targetX,p.targetY);assert(p.range==25&&!p.target&&p.flags==0x8021);}assert(dirs.size()>15);ExtraSpawn=false;
 Pulse::Params multi{};multi.owner=&player;multi.missile=Amazon::MultiArrow;multi.skill=12;multi.level=10;Created(game,&multi,&arrow);BeforeTick(game,&arrow);BeforeTick(game,&arrow);assert(Extras.size()==1&&Extras[0].damage[0]==100&&Extras[0].cls==Amazon::MultiArrow);FinishTick(game,400,2);assert(Pending.empty());
 auto oldCreate=Pulse::OriginalCreate;auto oldFilter=Pulse::FilterCreate;auto oldAfter=Pulse::AfterCreate;
 static Unit bonus;static int extraCount=0;
 Pulse::OriginalCreate=[](void*,void* raw)->void*{auto p=static_cast<Pulse::Params*>(raw);assert(p->missile==0&&p->skill==12&&p->level==10&&p->flags==0x8021&&p->range==25&&!p->target);bonus.type=3;bonus.id=450;bonus.cls=0;++extraCount;return &bonus;};
 Pulse::FilterCreate=&Filter;Pulse::AfterCreate=&Created;
 PoeMythicals::Frame(game);assert(extraCount==1&&bonus.stats[21]==100&&bonus.stats[56]==80&&Extras.empty());assert(!Flights[450].extra&&!Flights[450].returning);
 BeforeTick(game,&bonus);assert(Extras.empty());Flights[450]={};
 Pulse::OriginalCreate=oldCreate;Pulse::FilterCreate=oldFilter;Pulse::AfterCreate=oldAfter;ExtraSpawn=true;
 Pulse::Params p{};p.owner=&player;p.missile=0;p.skill=0;p.level=1;Created(game,&p,&arrow);ExtraSpawn=false;assert(Read(&arrow,0x124)&0x00200000);
 static int published=0;Amazon::PublishMissiles=[](void* anchor,void** shots,unsigned char count,void* origin,void* target,unsigned short){assert(anchor==&arrow&&origin==&arrow&&shots[0]==&arrow&&count==1&&!target);++published;};
 PoeMythicals::Frame(game);PoeMythicals::Frame(game);assert(published==1);Amazon::PublishMissiles=nullptr;
 BeforeTick(game,&arrow);FinishTick(game,400,2);FinishTick(game,400,2);assert(Pending.size()==1&&Pending[0].damage[0]==100&&Pending[0].damage[10]==80);
 ReturnSpawn=true;++arrow.id;Created(game,&p,&arrow);ReturnSpawn=false;BeforeTick(game,&arrow);FinishTick(game,401,2);assert(Pending.size()==1);assert(ReturnFrames(0,1,0,0)==1&&ReturnFrames(0,1,10000,0)==125);
 static std::vector<int> drops;NativeDrop=[](void*,void*,void*,unsigned short tc,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t)->std::uintptr_t{drops.push_back(tc);return 99;};assert(Drop(game,&enemy,&player,TestTC,0,0,0,1,1,0)==99);assert(drops==std::vector<int>({TestTC}));drops.clear();Drop(game,&enemy,&player,1,0,0,0,0,0,0);assert(drops==std::vector<int>({1}));
 static D2RL::ItemService service{};static int code=88;
 service.getItemInfo=[](const D2RL::PluginContext*,D2RL::ItemHandle,D2RL::Items::ItemInfo* out)noexcept{char c[4]={'Y',char('0'+code/10),char('0'+code%10),0};out->code=D2RL::Items::MakeItemCode(c);return D2RL::Items::Result::Success;};Items=&service;
 for(const char* element:{"fire","cold","lightning","poison"}){char text[256]{};D2RL::SharedEvents::ItemTooltipEvent e{};e.text=text;e.capacity=sizeof(text);PrototypeTooltip(nullptr,&e,nullptr);assert(std::string(text)==std::string("\xee\x81\xbe" "4Nearby enemies have ")+element+" resistance equal to yours"+"\xee\x81\xbe" "0");assert(e.length==std::strlen(text));++code;}
 char text[256]{};D2RL::SharedEvents::ItemTooltipEvent e{};e.text=text;e.capacity=sizeof(text);code=87;PrototypeTooltip(nullptr,&e,nullptr);assert(!e.length);code=88;e.capacity=5;PrototypeTooltip(nullptr,&e,nullptr);assert(!e.length);
 alignas(8) unsigned char crushingContext[24]{};Write(crushingContext,8,4);Write(crushingContext,12,10);Write(crushingContext,16,10);crushingContext[20]=1;
 OriginalCrushing=[](void*,void*,void*,void* p)->int{assert(Read(p,8)==20&&Read(p,12)==50&&Read(p,16)==50&&static_cast<unsigned char*>(p)[20]==1);return 7;};
 assert(Crushing(game,&player,&enemy,crushingContext)==7&&Read(crushingContext,8)==4);
 Frame(1);Reset(game);assert(Pending.empty()&&Flights[400].id==Invalid);
 puts("PASS: Original Sin conversion, four actual Prototype resistance auras, noncompounding, range/unequip cleanup and preserved physical damage, native absorbs retained, armor scaling, life cost and floor, barrier consumption/expiry, lucky block distribution, safe missile exclusions and single return, native test drops and session reset.");
}
