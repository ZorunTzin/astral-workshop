// The native cube accepts only five modifier slots. This narrow dispatch hook
// expands our generated bundle before the cube commits/synchronizes its output.
// Every real bonus still uses D2R's own property implementation and random rolls.
#include "forge.inc"
#include "leagues/craft_bindings.h"
#include "affix_scaling.inc"
#include "universal_class_items.h"
#include "endgame/seal_engine.h"
#include "endgame/seal_bindings.h"

using ApplyPropertyFn = void(__fastcall*)(bool, int, void*, void*, void*, int, int, int,
                                         const Items::PropertySpec*, int, unsigned, void*);
ApplyPropertyFn originalApplyProperty{};
const PluginContext* forgeContext{};

// ItemService validates ordinary min/max ranges before native application.
// Procs instead use chance/skill-level, which may legitimately be descending.
// A scoped selector sends our own Sigil payload through the same native cube
// property handler already used by rune forging; no saved item keeps it.
constexpr int astralDispatchSelector=static_cast<int>(std::size(forgeSpans)+1);
thread_local const Items::PropertySpec* astralPayload=nullptr;
thread_local unsigned astralPayloadCount=0;
thread_local bool astralPayloadApplied=false;
struct AstralPayloadScope {
    AstralPayloadScope(const Items::PropertySpec* properties,unsigned count){
        astralPayload=properties;astralPayloadCount=count;astralPayloadApplied=false;
    }
    ~AstralPayloadScope(){astralPayload=nullptr;astralPayloadCount=0;}
};

void __fastcall applyForgeProperty(bool expansion, int source, void* unit, void* item,
        void* mods, int index, int propSet, int applyType,
        const Items::PropertySpec* property, int state, unsigned flags, void* other) {
    if(property && property->propertyId==TrialCraft::Dispatch){
        if(source!=7||!item||*static_cast<unsigned*>(item)!=4||!affixCode||!readItemStat||property->parameter<0||property->parameter>6)return;
        unsigned code=affixCode(item);bool seal=false;for(auto c:{"Wa1","Wa2","Wa3","Wa4","Wa5"})if(code==Items::MakeItemCode(c[0],c[1],c[2]))seal=true;
        if(!seal)return;
        TrialCraft::Seal before;for(int i=0;i<TrialCraft::FieldCount;++i)before.fields[i]=readItemStat(item,TrialCraft::FirstStat+i,0);
        auto after=before;static unsigned entropy=unsigned(GetTickCount());entropy^=unsigned(GetTickCount64())+static_cast<unsigned*>(item)[2];
        if(!TrialCraft::craft(after,TrialCraft::Orb(property->parameter),entropy)){if(forgeContext)forgeContext->LogError("Trial craft rejected invalid seal state");return;}
        // Legacy seals retain IFLAG_COMPACTSAVE even after misc.compactsave=0.
        // Native serializer 0x37FFD0 only sets this flag, never clears it.
        // Normalize the cube output before its native packet/save serialization.
        auto itemData=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
        if(!itemData){if(forgeContext)forgeContext->LogError("Trial seal missing ItemData");return;}
        auto& itemFlags=*reinterpret_cast<unsigned*>(itemData+0x18);
        const bool wasCompact=(itemFlags&(1u<<21))!=0;
        itemFlags&=~(1u<<21);
        if(wasCompact&&forgeContext)forgeContext->LogInfo("Trial seal migrated from legacy compact format");
        // Ordinary additive saved properties allow exact replacement, including
        // clearing old fields (native set=1 intentionally ignores zero values).
        for(int i=0;i<TrialCraft::FieldCount;++i){int delta=after.fields[i]-before.fields[i];if(!delta)continue;
            Items::PropertySpec effect{unsigned(TrialCraft::Properties[i]),0,delta,delta};
            originalApplyProperty(expansion,7,unit,item,&effect,0,propSet,applyType,&effect,state,flags,other);
        }
        bool correct=true;for(int i=0;i<TrialCraft::FieldCount;++i)correct&=readItemStat(item,TrialCraft::FirstStat+i,0)==after.fields[i];
        if(forgeContext){if(correct)forgeContext->LogInfo("Trial seal crafted; saved state verified");else forgeContext->LogError("Trial seal native stat verification failed");}
        return;
    }
    if (!property || property->propertyId != forgeDispatchProperty) {
        if(source==0&&item&&*static_cast<unsigned*>(item)==4&&
            ordinaryAffixRecord(expansion,mods,index,property)) {
            const int quality=affixQuality(item);
            if((quality==4||quality==6)&&!excludedAffixCode(affixCode?affixCode(item):0)) {
                auto data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
                const int itemLevel=data?*reinterpret_cast<int*>(data+0x10):1;
                auto enhanced=universalClassProperty(strengthenAffix(*property,quality,itemLevel),affixCode?affixCode(item):0);
                originalApplyProperty(expansion,source,unit,item,mods,index,propSet,
                    applyType,&enhanced,state,flags,other);
                return;
            }
        }
        auto converted=property?universalClassProperty(*property,item&&affixCode?affixCode(item):0):Items::PropertySpec{};
        originalApplyProperty(expansion, source, unit, item, mods, index, propSet,
                              applyType, property?&converted:property, state, flags, other);
        return;
    }
    if(property->parameter==astralDispatchSelector){
        if(astralPayload && astralPayloadCount && item && *static_cast<unsigned*>(item)==4 &&
           affixCode && affixCode(item)==Items::MakeItemCode("Wsig")){
            auto payload=astralPayload;unsigned count=astralPayloadCount;
            astralPayload=nullptr; // No recursive selector can replay this bundle.
            for(unsigned i=0;i<count;i++){
                const auto& effect=payload[i];
                originalApplyProperty(expansion,7,unit,item,const_cast<Items::PropertySpec*>(&effect),
                    0,propSet,applyType,&effect,state,flags,other);
            }
            astralPayloadApplied=true;
            if(forgeContext){
                char message[100];std::snprintf(message,sizeof(message),"Applied %u native Astral Sigil properties",count);
                forgeContext->LogInfo(message);
            }
        }else if(forgeContext)forgeContext->LogError("Rejected unscoped Astral Sigil selector");
        return;
    }
    // Only native cube-style application to an item may execute a forge bundle.
    // The dispatch is not saved. The final ordinary property marks the result,
    // and op 18 rejects it on every future attempt to forge the same item.
    if (source != 7 || !item || *static_cast<unsigned*>(item) != 4 ||
        property->parameter < 1 || static_cast<unsigned>(property->parameter) > std::size(forgeSpans)) {
        if (forgeContext) forgeContext->LogError("Rejected invalid forge dispatch");
        return;
    }
    const auto& span = forgeSpans[property->parameter - 1];
    const int oldSkills=readItemStat?readItemStat(item,127,0):0;
    for (unsigned i = 0; i < span.count; ++i) {
        const auto effect = universalClassProperty(forgeEffects[span.start + i],affixCode?affixCode(item):0);
        originalApplyProperty(expansion, source, unit, item, const_cast<Items::PropertySpec*>(&effect),
                              0, propSet, applyType, &effect, state, flags, other);
    }
    if (forgeContext) {
        char line[100];
        std::snprintf(line,sizeof(line),"Rune forge bundle %d applied: %u properties; all-skills %d -> %d",
                      property->parameter,span.count,oldSkills,readItemStat?readItemStat(item,127,0):0);
        forgeContext->LogInfo(line);
    }
}

bool installForgeHook(const PluginContext* ctx) {
    if(!initializeAffixScaling(ctx))return false;
    // D2R 3.3.0 / build 93847. Fail closed when this native signature changes.
    const unsigned char expected[]={0x48,0x89,0x6c,0x24,0x20,0x56,0x41,0x56,
                                    0x41,0x57,0x48,0x83,0xec,0x60};
    auto hook=MakeInlineHook(0x3d5060,expected,sizeof(expected),
        reinterpret_cast<void*>(applyForgeProperty),reinterpret_cast<void**>(&originalApplyProperty));
    if(!ctx->api->installInlineHook(ctx,&hook))return false;
    forgeContext=ctx;
    ctx->LogInfo("Rune-forging native bundle dispatcher enabled");
    return true;
}
