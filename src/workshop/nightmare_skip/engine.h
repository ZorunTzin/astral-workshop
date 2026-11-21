#pragma once
#include <cstdint>

namespace NightmareSkip {
enum class State : std::uint8_t { Unoffered, Offered, ContinueNightmare, RewardPending, RewardConsumed };
enum class Choice : std::uint8_t { Nightmare, Hell };
struct Reward { int skillPoints, allResistances, attributePoints, maximumLife; };
inline constexpr Reward NightmareReward{4, 10, 5, 20};
inline constexpr unsigned Normal = 0, Nightmare = 1, Hell = 2, BaalQuest = 40;

// This record belongs to the character's save transaction, never the account or
// a transferable item. The native adapter must persist it with quest flags and
// rewards; a sidecar written independently of the save is not sufficient.
struct Record {
    State state = State::Unoffered;
    std::uint64_t token = 0;
};
struct Completion {
    unsigned difficulty = Normal, quest = BaalQuest;
    bool normalBaalCompleteBefore = false;
    bool normalBaalCompleteAfter = false;
    bool higherDifficultyAlreadyReached = false;
};
constexpr bool offer(Record& record, const Completion& event) {
    if (record.state != State::Unoffered || event.difficulty != Normal ||
        event.quest != BaalQuest || event.normalBaalCompleteBefore ||
        !event.normalBaalCompleteAfter || event.higherDifficultyAlreadyReached)
        return false;
    record.state = State::Offered;
    return true;
}
// A full inventory leaves the offer pending: do not unlock Hell, mark quests,
// close the choice, or discard the reward until the transaction can commit.
constexpr bool choose(Record& record, Choice choice, bool inventorySpace,
                      std::uint64_t uniqueToken) {
    if (record.state != State::Offered) return false;
    if (choice == Choice::Nightmare) {
        record.state = State::ContinueNightmare;
        return true;
    }
    if (choice != Choice::Hell || !inventorySpace || uniqueToken == 0) return false;
    record.token = uniqueToken;
    record.state = State::RewardPending;
    return true;
}
// Call on a transaction copy, and commit only together with item removal,
// character stat changes and Anya's consumed-resistance flag. Replayed requests
// and tokens belonging to other characters never grant anything.
constexpr bool consume(Record& record, std::uint64_t itemToken) {
    if (record.state != State::RewardPending || !itemToken || itemToken != record.token)
        return false;
    record.state = State::RewardConsumed;
    return true;
}
}
