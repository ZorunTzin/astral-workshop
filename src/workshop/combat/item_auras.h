#pragma once
#include "src/ramp_native.h"
// New items use the ordinary oskill property. This native stat-write bridge
// handles old saved aura modifiers while the game reconstructs their item lists.
// It never changes character base skill points or writes a save file directly.
namespace ItemAuras {
inline Ramp::SetFn OriginalSet=nullptr;
inline unsigned Converted=0;
inline bool IsItem(void* list) {
    unsigned type=6;
    if(list)std::memcpy(&type,static_cast<unsigned char*>(list)+8,4);
    return type==4;
}
inline void __fastcall Set(unsigned char version,void* list,int stat,int value,unsigned layer) {
    if(stat!=151||value<=0||layer>=512||!IsItem(list)) {
        OriginalSet(version,list,stat,value,layer);return;
    }
    // Preserve other oskills at the same skill layer. Save deserialization and
    // old native property writers supply each positive aura contribution once.
    // A converted item contains no stat 151, so subsequent saves/loads do not
    // reapply conversion. Native item callbacks handle equip/unequip skill grants.
    const int display=Ramp::Get(version,list,387,layer),skill=Ramp::Get(version,list,97,layer);
    OriginalSet(version,list,151,0,layer);
    OriginalSet(version,list,387,display+value,layer);
    OriginalSet(version,list,97,skill+value,layer);
    if(Ramp::Log&&Converted++<20){char text[120]{};std::snprintf(text,sizeof(text),"Item aura converted to all-class skill: skill=%u levels=%d",layer,value);Ramp::Log(text);}
}
inline bool Install(const D2RL::PluginContext* context) {
    const auto hook=D2RL::MakeInlineHook(RampNative::SetStatRva,RampNative::SetStatBytes,sizeof(RampNative::SetStatBytes),reinterpret_cast<void*>(Set),reinterpret_cast<void**>(&OriginalSet));
    if(!context->api->installInlineHook(context,&hook))return false;
    context->LogInfo("Item aura properties grant all-class skills; old saved item aura stats convert when loaded");
    return true;
}
} // namespace ItemAuras
