#pragma once
#include <chrono>
namespace SetSelectors {
#include "set_selector_bindings.h"
inline std::uint64_t Random=0;
inline void* Session=nullptr;inline unsigned Last=0,NextFrame=0;
inline unsigned Draw(unsigned bound){
    if(!bound)return 0;
    if(!Random)Random=1; // A proc may precede the first selector frame.
    // Rejection sampling prevents modulo bias for three-, five- and six-piece sets.
    const unsigned threshold=(0u-bound)%bound;
    for(;;){Random^=Random>>12;Random^=Random<<25;Random^=Random>>27;
        unsigned value=static_cast<unsigned>((Random*2685821657736338717ULL)>>32);
        if(value>=threshold)return value%bound;}
}
inline void Tick(void* game){
    if(!game)return;unsigned now=Ramp::Frame(game);
    if(Session!=game||now<Last){Session=game;NextFrame=0;Random=static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())^reinterpret_cast<std::uintptr_t>(game);if(!Random)Random=1;}
    Last=now;if(now<NextFrame)return;NextFrame=now+1;
    SoulHarvest::CachePlayerIds(game);
    for(unsigned n=0;n<SoulHarvest::PlayerCount;++n){void* unit=Curses::Find(game,0,SoulHarvest::PlayerIds[n]);if(!unit)continue;
        for(auto& entry:Selectors)Ramp::SetUnit(unit,entry[1],static_cast<int>(Draw(entry[0])),0);
    }
}
}
