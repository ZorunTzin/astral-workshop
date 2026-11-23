#pragma once
// Extra bonuses live on native shrine lists: expiration and replacement remain native.
namespace WorkshopShrines {
inline void* Session=nullptr;
inline unsigned Last=0,Next=0;
// Handler ABI and table verified against native-aura-snapshot.bin (D2R 3.4).
struct Operation {void* game;void* object;void* player;};
using Handler=void(__fastcall*)(unsigned char,Operation*,void*);
using PromoteFn=void(__fastcall*)(void*,void*);
inline Handler Exploding=nullptr,Poison=nullptr;
inline PromoteFn Promote=nullptr;
struct Pack {unsigned owner;int area,x,y,remaining;unsigned attempt=0;};
inline std::vector<Pack> Packs;
inline void Reset(void* game){unsigned now=Ramp::Frame(game);if(Session!=game||now<Last){Session=game;Next=0;Packs.clear();}Last=now;}
inline int ObjectCoordinate(void* unit,bool y){
 if(!Assassin::NativePositions)return y?Curses::Y(unit):Curses::X(unit);
 // Shrine objects have StaticPath, unlike moving players/monsters.
 void* path=nullptr;std::memcpy(&path,static_cast<unsigned char*>(unit)+0x38,8);
 int value=0;if(path)std::memcpy(&value,static_cast<unsigned char*>(path)+(y?0x14:0x10),4);return value;
}
inline void Blast(Operation* op){
 if(!op||!op->game||!Curses::Player(op->player)||!op->object)return;
 int x=ObjectCoordinate(op->object,false),y=ObjectCoordinate(op->object,true),area=Arena::Level(op->object);
 std::vector<unsigned> targets;
 AuraFeatures::Units(op->game,1,[&](void* target){
  int64_t dx=Assassin::Coordinate(target,false)-x,dy=Assassin::Coordinate(target,true)-y;
  if(GuardianCombat::Hostile(op->game,target)&&Arena::Level(target)==area&&dx*dx+dy*dy<=24*24)targets.push_back(Barbarian::Id(target));
 });
 ++Curses::ProcDepth;
 for(unsigned id:targets)if(auto target=Curses::Find(op->game,1,id)){
  if(!GuardianCombat::Hostile(op->game,target))continue;
  // A resolved shrine blast bypasses elemental immunity and never targets allies.
  alignas(8) unsigned char damage[0x180]{};unsigned short flags=1;int amount=100000*256;
  std::memcpy(damage+4,&flags,2);std::memcpy(damage+0x18,&amount,4);std::memcpy(damage+0x134,&amount,4);
  Curses::OnDamage(op->game,op->player,target,damage);
 }
 --Curses::ProcDepth;
}
inline void __fastcall Explosion(unsigned char version,Operation* op,void* row){Exploding(version,op,row);Blast(op);}
inline void __fastcall Gas(unsigned char version,Operation* op,void* row){Poison(version,op,row);Blast(op);}
inline void __fastcall Monster(unsigned char,Operation* op,void*){
 if(!op||!op->game||!op->object||!Curses::Player(op->player))return;
 Reset(op->game);
 Packs.push_back({Barbarian::Id(op->player),Arena::Level(op->object),ObjectCoordinate(op->object,false),ObjectCoordinate(op->object,true),5});
}
inline void SpawnPacks(void* game){
 for(auto it=Packs.begin();it!=Packs.end();){
  auto player=Curses::Find(game,0,it->owner);
  if(!player){it=Packs.erase(it);continue;}
  if(Arena::Level(player)!=it->area||!AuraFeatures::Alive(player)){++it;continue;}
  for(int tries=0;tries<20&&it->remaining;++tries){
   unsigned n=it->attempt++;double a=(n%32)*6.283185307179586/32;int radius=8+(n/32)%12;
   League::Point point{it->x+int(radius*std::cos(a)),it->y+int(radius*std::sin(a))};
   auto room=League::RoomAt(SoulHarvest::Room(player),point.x,point.y);if(!room)continue;
   room=League::FreePosition(room,&point,&point,3,0x3e01,0xc01,0);if(!room)continue;
   room=League::RoomAt(room,point.x,point.y);if(!room)continue;
   constexpr int classes[]={0,5,19}; // Native skeleton, zombie and fallen families support minion packs.
   auto unit=MonsterRuntime::Spawn(game,room,point.x,point.y,classes[(5-it->remaining)%3],1,-1,0);if(!unit)continue;
   if(Arena::Level(unit)!=it->area){
    if(auto actual=SoulHarvest::Room(unit))Barbarian::RoomDelete(actual,1,Barbarian::Id(unit));
    Barbarian::DeleteUnit(game,unit);continue;
   }
   Promote(game,unit); // Native Monster Shrine promotion also creates and links its minions.
   Assassin::SetTarget(unit,player);--it->remaining;
  }
  if(!it->remaining)it=Packs.erase(it);else ++it;
 }
}
inline bool Install(const D2RL::PluginContext* c){
 constexpr unsigned table=0x1d3f1d0;
 constexpr unsigned rvas[]={0x591600,0x591700,0x591a90};
 Handler replacements[]={Monster,Explosion,Gas};
 for(int i=0;i<3;++i)if(*reinterpret_cast<Handler*>(c->exeBase+table+16*(20+i))!=reinterpret_cast<Handler>(c->exeBase+rvas[i]))return false;
 const unsigned char guard[]={0x40,0x55,0x41,0x54,0x41,0x57,0x48,0x8d,0x6c,0x24,0xb9};
 if(!c->api->checkExpectedBytes(c,0x49ac30,guard,sizeof(guard)))return false;
 Promote=reinterpret_cast<PromoteFn>(c->exeBase+0x49ac30);
 Exploding=reinterpret_cast<Handler>(c->exeBase+rvas[1]);Poison=reinterpret_cast<Handler>(c->exeBase+rvas[2]);
 for(int i=0;i<3;++i){auto old=reinterpret_cast<Handler>(c->exeBase+rvas[i]);if(!c->api->patchWrite(c,table+16*(20+i),&old,8,reinterpret_cast<uint64_t>(replacements[i]),8))return false;}
 c->LogInfo("Shrines: five elite packs; 100k enemy-only poison/explosion blasts; fire unchanged.");return true;
}
inline bool Set(void* game,void* list,int stat,int value){
 if(Curses::Read(game,list,stat)==value)return false;
 Curses::Write(game,list,stat,value);return true;
}
inline void Player(void* game,void* unit){
 if(!Curses::Player(unit)||!AuraFeatures::Alive(unit))return;
 bool changed=false;
 for(int state=128;state<=137;++state){
  auto list=Ramp::OriginalList(unit,state);if(!list)continue;
  auto put=[&](int stat,int value){changed=Set(game,list,stat,value)||changed;};
  switch(state){
   case 128:put(76,20);break;
   case 129:put(256,25);put(257,25);break;
   case 130:put(42,10);break;
   case 131:put(40,10);break;
   case 132:put(44,10);break;
   case 133:put(46,10);break;
   case 134:put(127,std::min(2,int(static_cast<unsigned char*>(game)[0x104])));break;
   case 135:put(105,50);break;
   case 136:for(int stat:{93,96,99,102})put(stat,30);break;
   case 137:put(79,100);put(80,50);break;
  }
 }
 if(changed){if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);Curses::Update(unit,"Shrine bonuses",0);}
 // Native regeneration is fixed-point HP per frame, at 25 frames/second.
 if(auto list=Ramp::OriginalList(unit,128)){
  int regen=std::max(0,Ramp::GetUnit(unit,7,0))/2500;
  if(Set(game,list,74,regen))Curses::Update(unit,"Armor shrine regeneration",0);
  if(!Curses::RegenPending(game,unit))Ramp::Schedule(game,unit,3,int(Ramp::Frame(game)+1),0,0,0);
 }
}
inline void Frame(void* game){
 if(!game)return;unsigned now=Ramp::Frame(game);
 Reset(game);
 if(now<Next)return;Next=now+5;
 AuraFeatures::Units(game,0,[&](void* unit){Player(game,unit);});
 SpawnPacks(game);
}
}
