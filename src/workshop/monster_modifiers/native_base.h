#pragma once
// Phase-one native-affix bridge. No engine table or nine-slot array grows.
namespace MonsterModifiersNative {
using InitFn=void(__fastcall*)(void*,int,int);
inline InitFn OriginalFast=nullptr,OriginalResistance=nullptr;
inline unsigned Rank(void* u){
 if(!Curses::Monster(u))return 0;void* data=nullptr;
 std::memcpy(&data,static_cast<unsigned char*>(u)+0x10,8);
 if(!data)return 0;unsigned short flags=0;
 std::memcpy(&flags,static_cast<unsigned char*>(data)+0x1a,2);return flags;
}
inline bool RawHas(void* u,unsigned char wanted){
 if(!Curses::Monster(u))return false;void* data=nullptr;
 std::memcpy(&data,static_cast<unsigned char*>(u)+0x10,8);if(!data)return false;
 const auto* mods=static_cast<unsigned char*>(data)+0x20;
 for(unsigned i=0;i<9&&mods[i];++i)if(mods[i]==wanted)return true;return false;
}
inline bool (*AdditionalHas)(void*,unsigned char)=nullptr;
inline bool Has(void* u,unsigned char wanted){return RawHas(u,wanted)||(AdditionalHas&&AdditionalHas(u,wanted));}
inline void __fastcall Fast(void* unit,int mod,int unique){
 if(mod!=6||!Curses::Monster(unit)||(Rank(unit)&4)){OriginalFast(unit,mod,unique);return;}
 // Preserve other movement contributions. +200% means 3x base movement,
 // not +200 on top of the old variable 10..100% Extra Fast contribution.
 const int before=Ramp::GetUnit(unit,67,0),attack=Ramp::GetUnit(unit,68,0);
 OriginalFast(unit,mod,unique);
 Ramp::SetUnit(unit,67,before+200,0);
 Ramp::SetUnit(unit,68,attack+200,0);
}
inline void __fastcall Resistance(void* unit,int mod,int unique){
 if(!Curses::Monster(unit)||(Rank(unit)&4)){OriginalResistance(unit,mod,unique);return;}
 if(mod==28)return; // Stone Skin is applied AFTER resistance, below.
 int before[3]{};constexpr int stats[]={43,39,41};
 if(mod==8)for(int i=0;i<3;++i)before[i]=Ramp::GetUnit(unit,stats[i],0);
 OriginalResistance(unit,mod,unique);
 if(mod==8)for(int i=0;i<3;++i){const int after=Ramp::GetUnit(unit,stats[i],0);
  if(after>before[i])Ramp::SetUnit(unit,stats[i],before[i]+(after-before[i])/2,0);
 }
}
inline void Damage(void* game,void*,void* defender,void* packet){
 if(!packet||!(Rank(defender)&8)||(Rank(defender)&4)||!Has(defender,28))return;
 if(!GuardianCombat::Hostile(game,defender))return;
 AuraFeatures::ScaleResolvedDamage(packet,1,4);
}
inline bool Install(const D2RL::PluginContext* context){
 const unsigned char fast[]={0x40,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x85,0xc9,0x74,0x0a};
 const unsigned char resist[]={0x40,0x53,0x57,0x48,0x83,0xec,0x38,0x8b,0xda,0x48,0x8b,0xf9,0x48,0x85,0xc9};
 if(!context->api->checkExpectedBytes(context,0x496300,fast,sizeof(fast))||
    !context->api->checkExpectedBytes(context,0x38e430,resist,sizeof(resist)))return false;
 auto a=D2RL::MakeInlineHook(0x496300,fast,sizeof(fast),reinterpret_cast<void*>(Fast),reinterpret_cast<void**>(&OriginalFast));
 auto b=D2RL::MakeInlineHook(0x38e430,resist,sizeof(resist),reinterpret_cast<void*>(Resistance),reinterpret_cast<void**>(&OriginalResistance));
 return context->api->installInlineHook(context,&a)&&context->api->installInlineHook(context,&b);
}
}
