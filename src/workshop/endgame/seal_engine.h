#pragma once
#include "engine.h"
#include <string>
namespace TrialCraft {
// These existing 12-bit Workshop fields are item-local. Seals cannot be equipped
// and never enter the Astral Sigil reader; no save schema or stat IDs change.
inline constexpr int FirstStat=444, FieldCount=8;
enum class Orb {Awakening,Ascension,Renewal,Augmentation,Purification,Fortune,Corruption};
inline constexpr int Mods[]={0,1,16,2,6,7,8,9,10,11,12,15,58,59};
inline constexpr const char* ExtraNames[]={"Champion population","Elite population"};
inline constexpr int RankValues[2][5]={{5,10,15,20,30},{2,4,6,8,12}};
struct Seal {
 std::array<int,FieldCount> fields{};
 int grade()const{return (fields[0]%100)/10;}
 bool corrupted()const{return fields[0]>=100;}
 int count()const{return fields[0]%10;}
 bool valid()const{
  if(fields[0]!=0&&fields[0]!=11&&fields[0]!=12&&fields[0]!=23&&fields[0]!=24&&fields[0]!=25&&fields[0]!=100&&fields[0]!=111&&fields[0]!=112&&fields[0]!=113&&fields[0]!=123&&fields[0]!=124&&fields[0]!=125&&fields[0]!=126&&fields[0]!=101)return false;
  if(fields[7]<0||fields[7]>1000)return false;
  for(int i=0;i<6;++i){int v=fields[i+1];if(i>=count()){if(v)return false;continue;}if(v<1||v>70)return false;for(int j=0;j<i;++j)if((fields[j+1]-1)/5==(v-1)/5)return false;}
  return true;
 }
 bool accepts(Orb o)const{
  if(!valid()||corrupted())return false;
  switch(o){case Orb::Awakening:return grade()==0;case Orb::Ascension:return grade()<2;
   case Orb::Augmentation:return grade()>0&&count()<(grade()==1?2:5);
   case Orb::Renewal:case Orb::Purification:case Orb::Fortune:return grade()>0;
   case Orb::Corruption:return true;}return false;
 }
 int mod(int i)const{return Mods[(fields[i+1]-1)/5];}
 int tier(int i)const{return (fields[i+1]-1)%5;}
 int value(int i,int stage)const{int m=mod(i),t=tier(i);return m>=58?RankValues[m-58][t]:Arena::TierValues[m][t]*(100+10*(stage-1))/100;}
 int reward()const{int n=fields[7];for(int i=0;i<count();++i)n+=10*(1<<tier(i));return n;}
};
// The shared tooltip bridge consumes UTF-8. Keep this contribution plain text;
// raw single-byte legacy color escapes are displayed literally by the HD UI.
inline std::string tooltip(const Seal& seal,int stage,bool ranges=false){
 if(!seal.valid()||stage<1||stage>5)return {};
 std::string text=seal.corrupted()?"CORRUPTED - cannot be crafted\n":"";
 text+=(seal.grade()==2?"Rare":seal.grade()==1?"Magic":"Normal");
 text+=" Trial Seal\n";
 if(!seal.count())text+="No starting modifiers\n";
 else {
  text+="Starting modifiers (entire run):\n";
  for(int i=0;i<seal.count();++i){int mod=seal.mod(i);
   text+=mod<58?Arena::EffectNames[mod]:ExtraNames[mod-58];
   text+=" +"+std::to_string(seal.value(i,stage))+"%";
   if(ranges){int lo=mod>=58?RankValues[mod-58][0]:Arena::TierValues[mod][0]*(100+10*(stage-1))/100;int hi=mod>=58?RankValues[mod-58][4]:Arena::TierValues[mod][4]*(100+10*(stage-1))/100;text+=" ["+std::to_string(lo)+"-"+std::to_string(hi)+"%]";}
   text+="\n";
   text+="("+std::string(Arena::Tiers[seal.tier(i)] )+")\n";
  }
 }
 if(seal.count())text+="All modifiers: monsters +"+std::to_string(10*seal.count())+"% life, +"+std::to_string(2*seal.count())+"% damage\n";
 text+="Starting loot bonus +"+std::to_string(seal.reward())+"%";
 return text;
}
inline unsigned random(unsigned& s){s=s*1664525u+1013904223u;return s>>8;}
inline int tier(unsigned& s){int r=random(s)%100,t=0;while(t<4&&r>=Arena::Weights[t])r-=Arena::Weights[t++];return t;}
inline void add(Seal& a,unsigned& s,int forced=-1){int used[14]={};for(int i=0;i<a.count();++i)used[(a.fields[i+1]-1)/5]=1;int pool[14],n=0;for(int i=0;i<14;++i)if(!used[i])pool[n++]=i;int m=pool[random(s)%n];a.fields[a.count()+1]=1+5*m+(forced<0?tier(s):forced);++a.fields[0];}
inline bool craft(Seal& a,Orb o,unsigned& s){
 if(!a.accepts(o))return false;
 switch(o){
 case Orb::Awakening:case Orb::Ascension:case Orb::Renewal:{int g=o==Orb::Awakening?1:o==Orb::Ascension?2:a.grade();a={};a.fields[0]=g*10;int n=g==1?1+random(s)%2:3+random(s)%3;while(n--)add(a,s);break;}
 case Orb::Augmentation:add(a,s);break;
 case Orb::Purification:a={};break;
 case Orb::Fortune:for(int i=0;i<a.count();++i)a.fields[i+1]=1+5*((a.fields[i+1]-1)/5)+tier(s);break;
 case Orb::Corruption:{int outcome=random(s)%3;if(outcome==0){add(a,s,4);}else if(outcome==1)a.fields[7]=160;else {for(int i=0;i<a.count();++i)a.fields[i+1]=1+5*((a.fields[i+1]-1)/5)+std::min(4,a.tier(i)+1);if(!a.count())add(a,s,4);}a.fields[0]+=100;break;}
 }return a.valid();
}
inline void apply(const Seal& seal,Arena::Run& run,int& champion,int& elite){
 champion=elite=0;if(!seal.valid())return;
 for(int i=0;i<seal.count();++i){run.addModifierBurden();int m=seal.mod(i),v=seal.value(i,run.stage);if(m==58){champion=std::min(100,champion+v);run.bonuses[m]=champion;}else if(m==59){elite=std::min(100,elite+v);run.bonuses[m]=elite;}else run.bonuses[m]=std::min(Arena::Caps[m],run.bonuses[m]+v);}
 run.rewardBonus=std::min(2000,run.rewardBonus+seal.reward());
}
}
