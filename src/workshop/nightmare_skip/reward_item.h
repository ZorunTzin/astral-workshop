#pragma once
#include <D2RLPlugin/item.h>
namespace NightmareSkip {
// Temporary local Akara test stock. Consumption has no character or quest gate.
inline constexpr bool TestVendorEnabled=false;
// The item identity seed is distinct from the RNG used while generating it.
// Accept either preserved seed for compatibility with existing issued tokens.
constexpr bool rewardOwnedBy(unsigned ownerKey,unsigned itemSeed,unsigned generationSeed) {
    return ownerKey!=0 && (itemSeed==ownerKey || generationSeed==ownerKey);
}
// UI handles cannot be reused by the authoritative item service. Match the
// same live item after enumerating the server inventory, never just its code.
constexpr bool sameRewardItem(const D2RL::Items::ItemInfo& requested,const D2RL::Items::ItemInfo& current) {
    using namespace D2RL;
    return (current.code==Items::MakeItemCode("Wnsk")||current.code==Items::MakeItemCode("Wnl")) && requested.code==current.code &&
        current.container==Items::ItemContainer::Inventory &&
        requested.runtimeId==current.runtimeId;
}
// ItemService uses its own state bits, not native D2 item flags (0x10).
// Preserve native quantity: an explicit amount is rejected for non-stackables.
constexpr D2RL::Items::ItemCreateSpec rewardItemSpec(unsigned ownerKey) {
    using namespace D2RL;
    return {.structSize=Items::ItemCreateSpecSize,.code=Items::MakeItemCode("Wnl"),
        .quality=Items::Quality::Normal,.qualityRecordId=Items::RandomQualityRecord,.itemLevel=1,
        .seedMode=Items::SeedMode::Deterministic,.generationSeed=ownerKey,.itemSeed=ownerKey,
        .quantity=Items::DefaultValue,.durability=Items::DefaultValue,.stateFlags=Items::ItemStateIdentified,
        .destination={.structSize=Items::ItemDestinationRequiredSize,.container=Items::ItemContainer::Inventory,
            .placement=Items::Placement::Automatic}};
}
static_assert((rewardItemSpec(1).stateFlags & ~D2RL::Items::AllItemStates)==0);
}
