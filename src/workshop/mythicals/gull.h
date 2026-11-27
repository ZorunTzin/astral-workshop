#pragma once
// Native shrine lists remain authoritative. A separate list supplies the +50%
// bonus, avoiding repeated multiplication and removing it immediately on unequip.
namespace Gull {
#include "gull_bindings.h"
inline void* Game=nullptr;inline unsigned Last=0,Next=0;
inline unsigned (*ListExpiry)(void*)=Ramp::ExpireFrame;
inline unsigned (*Rank)(void*)=MonsterModifiersNative::Rank;
constexpr int Stats[]={171,25,119,39,41,43,45,127,27,154,85,76,74,256,257,40,42,44,46,105,93,96,99,102,79,80};
struct Extension {unsigned owner;int state;void* list;unsigned end;};
struct Kill {unsigned owner,monster;int cls;};
inline std::vector<Extension> Extensions;
inline std::vector<Kill> Pending;
inline std::array<std::pair<unsigned,int>,512> Seen{};inline unsigned SeenCursor=0;
inline bool Equipped(void* unit){return Curses::Player(unit)&&Ramp::GetUnit(unit,GullStat,0)>0;}
inline void Reset(void* game){const auto now=Ramp::Frame(game);if(Game!=game||now<Last){Game=game;Next=0;Extensions.clear();Pending.clear();Seen.fill({~0u,-1});SeenCursor=0;}Last=now;}
inline void Refresh(void* unit){if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);Curses::Update(unit,"The Gull",0);}
inline void Clear(void* game,void* unit,int state){if(auto list=Ramp::OriginalList(unit,state)){for(int stat:Stats)Curses::Write(game,list,stat,0);AuraBuffs::Expire(game,unit,state);Refresh(unit);}}
inline void* Apply(void* game,void* unit,int state,int frames){
 Curses::Curse request{unit,unit,0,1,frames,-1,0,state,nullptr};auto list=Curses::OriginalApply(&request);
 if(list){Ramp::SetExpiry(list,int(Ramp::Frame(game)+frames));Ramp::Schedule(game,unit,12,int(Ramp::Frame(game)+frames),0,0,0);}
 return list;
}
inline void Base(void* game,void* unit,void* list,int kind){
 auto put=[&](int stat,int value){WorkshopShrines::Set(game,list,stat,Equipped(unit)&&stat!=154?value*3/2:value);};
 switch(kind){
 case 0:put(171,100);put(76,20);put(74,std::max(0,Ramp::GetUnit(unit,7,0))/2500);break;
 case 1:put(25,200);put(119,200);put(256,25);put(257,25);break;
 case 2:put(41,75);put(42,10);break;
 case 3:put(39,75);put(40,10);break;
 case 4:put(43,75);put(44,10);break;
 case 5:put(45,75);put(46,10);break;
 case 6:put(127,2+std::min(2,int(static_cast<unsigned char*>(game)[0x104])));break;
 case 7:put(27,400);put(105,50);break;
 case 8:put(154,100);for(int s:{93,96,99,102})put(s,30);break;
 case 9:put(85,50);put(79,100);put(80,50);break;
 }
}
inline void Grant(void* game,void* unit,int kind){
 if(!Equipped(unit)||!AuraFeatures::Alive(unit)||kind<0||kind>=10)return;
 Clear(game,unit,GullStates[kind]);
 if(auto list=Apply(game,unit,GullStates[kind],1500)){Base(game,unit,list,kind);if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(unit,GullStates[kind],1);}
 Refresh(unit);
}
inline void Extend(void* game,void* unit,int state,void* list){
 const unsigned now=Ramp::Frame(game),end=ListExpiry(list),owner=Barbarian::Id(unit);if(end<=now)return;
 auto it=std::find_if(Extensions.begin(),Extensions.end(),[&](const Extension& e){return e.owner==owner&&e.state==state;});
 if(it!=Extensions.end()&&it->list==list&&it->end==end)return;
 unsigned extended=now+(end-now)*3/2;
 Ramp::SetExpiry(list,int(extended));Ramp::Schedule(game,unit,12,int(extended),0,0,0);
 if(it==Extensions.end())Extensions.push_back({owner,state,list,extended});else *it={owner,state,list,extended};
 Refresh(unit);
}
inline void Player(void* game,void* unit){
 if(!Curses::Player(unit))return;
 if(!AuraFeatures::Alive(unit)){for(int state:GullStates)Clear(game,unit,state);Clear(game,unit,GullAmplifier);return;}
 for(int i=0;i<10;++i)if(auto list=Ramp::OriginalList(unit,GullStates[i]))if(ListExpiry(list)>Ramp::Frame(game)+1)Base(game,unit,list,i);
 if(!Equipped(unit)){Clear(game,unit,GullAmplifier);return;}
 std::array<int,std::size(Stats)> bonus{};bool active=false;
 for(int i=0;i<10;++i){
  for(int custom=0;custom<1;++custom){
   int state=custom?GullStates[i]:128+i;auto list=Ramp::OriginalList(unit,state);if(!list||(custom&&ListExpiry(list)<=Ramp::Frame(game)+1))continue;active=true;
   if(!custom)Extend(game,unit,state,list);
   for(unsigned n=0;n<std::size(Stats);++n){
    int stat=Stats[n];if(stat==154)continue; // Infinite stamina has no magnitude to amplify.
    int value=Curses::Read(game,list,stat);
    if(!custom&&i==6&&stat==127)value+=2; // Native shrine set/rem callbacks provide these two levels.
    bonus[n]+=std::max(0,value)/2;
   }
  }
 }
 if(!active){Clear(game,unit,GullAmplifier);return;}
 auto list=Ramp::OriginalList(unit,GullAmplifier);if(!list)list=Apply(game,unit,GullAmplifier,15);if(!list)return;
 bool changed=false;for(unsigned n=0;n<std::size(Stats);++n)changed=WorkshopShrines::Set(game,list,Stats[n],bonus[n])||changed;
 Ramp::SetExpiry(list,int(Ramp::Frame(game)+15));Ramp::Schedule(game,unit,12,int(Ramp::Frame(game)+15),0,0,0);
 if(changed)Refresh(unit);
 if((bonus[12]>0||Ramp::OriginalList(unit,GullStates[0]))&&!Curses::RegenPending(game,unit))Ramp::Schedule(game,unit,3,int(Ramp::Frame(game)+1),0,0,0);
}
inline void Observe(void* game,void* monster,void* killer){
 if(!game||!Curses::Monster(monster)||!(Rank(monster)&8)||Ramp::GetUnit(monster,172,0)!=0)return;
 Reset(game);void* owner=killer;
 for(int i=0;i<4&&owner&&!Curses::Player(owner);++i){
  void* previous=owner;
  if(Curses::Monster(owner))owner=Curses::SkeletonOwner(game,owner);
  else if(static_cast<unsigned*>(owner)[0]==3&&Curses::Owner)owner=Curses::Owner(game,owner);else return;
  if(owner==previous)return;
 }
 if(!Equipped(owner)||!AuraFeatures::Alive(owner)||Arena::Level(owner)!=Arena::Level(monster))return;
 std::pair<unsigned,int> identity{Barbarian::Id(monster),int(static_cast<unsigned*>(monster)[1])};
 if(std::find(Seen.begin(),Seen.end(),identity)!=Seen.end())return;
 Seen[SeenCursor++%Seen.size()]=identity;
 if(Pending.size()<512)Pending.push_back({Barbarian::Id(owner),identity.first,identity.second});
}
inline void __cdecl Death(const D2RL::PluginContext*,const D2RL::Lifecycle::MonsterDeathEvent* e,void*) noexcept {
 if(!Game||!e||e->structSize<D2RL::Lifecycle::MonsterDeathEventRequiredSize)return;
 try{auto monster=Curses::Find(Game,1,e->monster.id);if(!monster||static_cast<unsigned*>(monster)[1]!=e->monster.classId)return;
 auto killer=Curses::Find(Game,int(e->killer.type),e->killer.id);Observe(Game,monster,killer);}catch(...){}
}
inline void Frame(void* game){
 if(!game)return;Reset(game);unsigned now=Ramp::Frame(game);if(now<Next)return;Next=now+5;
 for(const auto& kill:Pending)if(auto owner=Curses::Find(game,0,kill.owner))Grant(game,owner,int(SetSelectors::Draw(10)));
 Pending.clear();AuraFeatures::Units(game,0,[&](void* unit){Player(game,unit);});
}
inline bool Install(const D2RL::PluginContext* c){
 const D2RL::LifecycleService* api=nullptr;if(c->QueryService(&api)!=D2RL::ServiceQueryResult::Success||!api||!api->registerMonsterDeathListener)return false;
 D2RL::Lifecycle::MonsterDeathListener reg{D2RL::Lifecycle::MonsterDeathListenerSize,0,Death,nullptr};D2RL::Lifecycle::ListenerHandle handle=0;
 return api->registerMonsterDeathListener(c,&reg,&handle)==D2RL::Lifecycle::Result::Success;
}
}
