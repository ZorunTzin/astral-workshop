#include "aura_buffs.h"
#include <cassert>
#include <map>
#include <vector>
struct List { void* owner{};int state{};unsigned expiry=0x7fffffff;std::map<int,int> stats; };
struct Unit { unsigned type=0,cls=3,id=10,mode=1;unsigned char native[0x150]{};std::map<int,int> stats;std::map<int,List> lists;std::map<int,int> levels; int x=20,y=20,room=1; };
Unit player,mercenary;
alignas(8) unsigned char game[0x4000]{};
std::map<int,std::array<unsigned char,0x2ec>> records;
unsigned pulses=0,itemCalls=0,scheduled=0;
void Frame(unsigned n){*reinterpret_cast<unsigned*>(game+0x170)=n;}
int __fastcall GetUnit(void* p,int stat,unsigned){auto& u=*static_cast<Unit*>(p);int n=u.stats[stat];for(auto& [_,list]:u.lists)if(list.expiry>Ramp::Frame(game))n+=list.stats[stat];return stat==9?std::max(0,n):n;}
void __fastcall SetUnit(void* p,int stat,int n,unsigned){static_cast<Unit*>(p)->stats[stat]=n;}
void* __fastcall GetList(void* p,int state){auto& u=*static_cast<Unit*>(p);auto i=u.lists.find(state);return i==u.lists.end()||i->second.expiry<=Ramp::Frame(game)?nullptr:&i->second;}
int __fastcall Get(unsigned char,void* p,int stat,unsigned){return static_cast<List*>(p)->stats[stat];}
void __fastcall Set(unsigned char,void* p,int stat,int n,unsigned){static_cast<List*>(p)->stats[stat]=n;}
void __fastcall Expiry(void* p,int frame){static_cast<List*>(p)->expiry=frame;}
void __fastcall Schedule(void*,void*,int,int,int,int,int){++scheduled;}
void __fastcall Update(void*,const char*,int){}
void* __fastcall Find(void*,int type,unsigned id){return type==0&&id==player.id?&player:nullptr;}
void* __fastcall Apply(void* raw){auto& c=*static_cast<Curses::Curse*>(raw);auto& u=*static_cast<Unit*>(c.target);auto& l=u.lists[c.state];l.owner=c.target;l.state=c.state;l.expiry=Ramp::Frame(game)+c.duration;return &l;}
void* __fastcall Record(unsigned char,int skill){auto& r=records[skill];*reinterpret_cast<int*>(r.data()+0x19c)=skill;*reinterpret_cast<int*>(r.data()+0x180)=50;return r.data();}
int __fastcall Evaluate(unsigned char,void* u,int formula,int,int){if(formula==50)return 50;for(auto& d:AuraBuffs::Definitions)if(d.levelHelper==formula)return static_cast<Unit*>(u)->levels[d.skill];return 0;}
int __fastcall Aura(void*,void* u,int skill,int){++pulses;auto& unit=*static_cast<Unit*>(u);int i=AuraBuffs::Index(skill);if(i<0)for(const auto& legacy:AuraBuffs::LegacySkills)if(skill==legacy[1])i=AuraBuffs::Index(legacy[0]);assert(i>=0);auto& l=unit.lists[AuraBuffs::Definitions[i].state];l.expiry=Ramp::Frame(game)+51;return 1;}
int __fastcall Ordinary(void*,void*,int,int){return 123;}
void __fastcall Item(void* g,void* u,int,int skill,int){++itemCalls;AuraBuffs::Toggle(g,u,skill,GetUnit(u,151,skill));}
int main(){
    Ramp::GetUnit=GetUnit;Ramp::SetUnit=SetUnit;Ramp::OriginalList=GetList;
    Ramp::Get=Get;Ramp::Set=Set;Ramp::SetExpiry=Expiry;Ramp::Schedule=Schedule;
    Curses::OriginalApply=Apply;Curses::Find=Find;Curses::Update=Update;
    Curses::Record=Record;Curses::Evaluate=Evaluate;
    AuraBuffs::Native.fill(Aura);AuraBuffs::OriginalBuff=Ordinary;AuraBuffs::OriginalItem=Item;
    game[0x106]=3;Frame(1);player.stats[6]=1000;player.stats[9]=225*256;player.stats[8]=225*256;
    player.levels[98]=5;player.levels[99]=10;
    assert(AuraBuffs::Cost(AuraBuffs::Index(98),1)==27&&AuraBuffs::Cost(AuraBuffs::Index(98),20)==112);
    const int tierSkills[]={98,103,108,113,118,123};
    for(int tier=1;tier<=6;++tier) {
        const int index=AuraBuffs::Index(tierSkills[tier-1]);
        assert(AuraBuffs::Cost(index,1)==27*tier);
        assert(AuraBuffs::Cost(index,20)==225*tier/2);
    }
    assert(AuraBuffs::Toggle(game,&player,98,5)==1);
    assert(GetUnit(&player,9,0)==180*256&&GetUnit(&player,8,0)==180*256);
    assert(AuraBuffs::Toggle(game,&player,99,10)==1);
    assert(GetUnit(&player,9,0)==113*256&&AuraBuffs::Reservation(game,&player)==112*256);
    const unsigned old=pulses;Frame(51);AuraBuffs::OnFrame(game);
    assert(pulses==old+2&&AuraBuffs::Total(AuraBuffs::Players[0])==112);
    assert(AuraBuffs::Toggle(game,&player,98,5)==1);
    assert(GetUnit(&player,9,0)==158*256&&AuraBuffs::Total(AuraBuffs::Players[0])==67);
    player.levels[99]=20;Frame(56);AuraBuffs::OnFrame(game);
    assert(GetUnit(&player,9,0)==113*256&&AuraBuffs::Total(AuraBuffs::Players[0])==112);
    player.levels[98]=100;assert(AuraBuffs::Toggle(game,&player,98,100)==0);
    player.stats[9]=40*256;Frame(61);AuraBuffs::OnFrame(game);
    assert(AuraBuffs::Total(AuraBuffs::Players[0])==0&&GetUnit(&player,9,0)==40*256);
    player.stats[9]=225*256;assert(AuraBuffs::Toggle(game,&player,99,20)==1);
    player.levels[118]=5;
    assert(AuraBuffs::Toggle(game,&player,118,5)==0); // 225 reserved, only 113 usable.
    assert(AuraBuffs::Toggle(game,&player,99,20)==1);
    assert(AuraBuffs::Toggle(game,&player,118,5)==1);
    assert(GetUnit(&player,9,0)==0&&AuraBuffs::Total(AuraBuffs::Players[0])==225);
    assert(AuraBuffs::Toggle(game,&player,118,5)==1);
    assert(GetUnit(&player,9,0)==225*256);
    assert(AuraBuffs::Toggle(game,&player,99,20)==1);
    player.stats[6]=0;Frame(66);AuraBuffs::OnFrame(game);
    assert(AuraBuffs::Total(AuraBuffs::Players[0])==0&&GetUnit(&player,9,0)==225*256);
    assert(AuraBuffs::Toggle(game,&player,97,1)==123);
    mercenary.type=1;mercenary.stats[6]=1000;mercenary.stats[9]=100*256;mercenary.stats[151]=12;
    const auto beforeItems=pulses,beforeSchedules=scheduled;
    AuraBuffs::ItemPeriodic(game,&mercenary,5,98,0);
    assert(itemCalls==1&&pulses==beforeItems+1&&scheduled==beforeSchedules+1);
    assert(GetUnit(&mercenary,9,0)==100*256&&!mercenary.lists.count(AuraBuffs::ReservationState));
    std::puts("PASS: six reservation tiers, higher-tier budget boundaries, multi-aura toggles, visible flat reservation, renewal, level changes, budget loss, death cleanup, native skill forwarding and free item aura renewal.");
    return 0;
}
