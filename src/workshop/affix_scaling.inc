// Strength applies only while rolling ordinary prefixes/suffixes. Stored item
// stats are not multiplied on load or equip, so save/load cannot compound it.
#include "affix_rules.inc"
using AffixQualityFn = int(__fastcall*)(void*);
using AffixTablesFn = unsigned char*(__fastcall*)(bool);
AffixQualityFn affixQuality{};
AffixTablesFn affixTables{};
using AffixCodeFn=unsigned(__fastcall*)(void*);
AffixCodeFn affixCode{};

bool excludedAffixCode(unsigned id) {
    return std::find(std::begin(unscaledAffixCodes),std::end(unscaledAffixCodes),id)!=std::end(unscaledAffixCodes);
}

int scaleAffixNumber(int value,unsigned quality) {
    const long long numerator=static_cast<long long>(value)*(quality==4?200:135);
    // Mathematical floor also handles beneficial negative requirement modifiers.
    return static_cast<int>(numerator>=0?numerator/100:(numerator-99)/100);
}

Items::PropertySpec strengthenAffix(Items::PropertySpec property,unsigned quality,int itemLevel) {
    if(quality!=4&&quality!=6)return property;
    auto found=std::find_if(std::begin(affixRules),std::end(affixRules),
        [&](const auto& rule){return rule.property==property.propertyId;});
    if(found==std::end(affixRules)||!found->kind)return property;
    if(found->kind==2) {
        // Per-level rates and random-skill bonus amounts live in parameter.
        // For skill-rand, minimum/maximum are skill IDs and must stay intact.
        property.parameter=scaleAffixNumber(property.parameter,quality);
        return property;
    }
    if(found->kind==3&&property.parameter>0)
        property.parameter=scaleAffixNumber(property.parameter,quality);
    if(found->kind==5) {
        // Negative charged-skill fields are formulas, not negative skill levels.
        // Resolve their original level/count first, then strengthen each amount.
        auto skill=std::find_if(std::begin(affixSkills),std::end(affixSkills),
            [&](const auto& row){return row.id==property.parameter;});
        if(skill!=std::end(affixSkills)) {
            if(property.maximum==0)
                property.maximum=std::clamp((itemLevel-skill->required)/4+1,1,skill->maximum);
            else if(property.maximum<0) {
                const int divisor=std::max(1,-(std::max(1,99-skill->required)/property.maximum));
                property.maximum=std::max(1,(itemLevel-skill->required)/divisor);
            }
            if(property.minimum<0)
                property.minimum=property.maximum*(-property.minimum)/8-property.minimum;
            else if(property.minimum==0)property.minimum=5;
        }
    }
    property.minimum=scaleAffixNumber(property.minimum,quality);
    property.maximum=scaleAffixNumber(property.maximum,quality);
    if(found->cap) {
        property.minimum=std::min(property.minimum,static_cast<int>(found->cap));
        if(found->kind!=4)property.maximum=std::min(property.maximum,static_cast<int>(found->cap));
    }
    if(found->kind==5)property.minimum=std::min(property.minimum,255);
    return property;
}

bool ordinaryAffixRecord(bool expansion,void* mods,int index,const Items::PropertySpec* property) {
    if(!affixTables||!mods||!property||index<0||index>=3)return false;
    auto tables=affixTables(expansion);
    if(!tables)return false;
    const auto begin=*reinterpret_cast<uintptr_t*>(tables+0x15e8);
    const auto end=*reinterpret_cast<uintptr_t*>(tables+0x1610);
    const auto record=reinterpret_cast<uintptr_t>(mods);
    // Prefix/suffix records precede AutoMagic in the combined native table.
    // The 0x8c stride and 0x24 property offset are verified for build 93847.
    return begin&&end>begin&&record>=begin&&record<end&&
        (record-begin)%0x8c==0&&
        reinterpret_cast<uintptr_t>(property)==record+0x24+16*index;
}

bool initializeAffixScaling(const PluginContext* ctx) {
    auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
    const unsigned char qualityGuard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9};
    const unsigned char tablesGuard[]={0xff,0x15,0x07,0xef,0x2d,0x00};
    if(!exe||!core||std::memcmp(exe+0x36cf60,qualityGuard,sizeof(qualityGuard))||
        std::memcmp(core+0x422f63,tablesGuard,sizeof(tablesGuard)))return false;
    const unsigned char codeGuard[]={0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x85,0xc9};
    if(std::memcmp(exe+0x36ef50,codeGuard,sizeof(codeGuard)))return false;
    affixCode=reinterpret_cast<AffixCodeFn>(exe+0x36ef50);
    affixQuality=reinterpret_cast<AffixQualityFn>(exe+0x36cf60);
    affixTables=*reinterpret_cast<AffixTablesFn*>(core+0x701e70);
    if(!affixTables)return false;
    ctx->LogInfo("Affix scaling enabled: Magic x2, Rare x1.35; prefix/suffix only, charms and jewels excluded");
    return true;
}
