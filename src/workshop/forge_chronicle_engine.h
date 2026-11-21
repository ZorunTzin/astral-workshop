#pragma once
#include <cstdint>
#include <cstddef>
namespace ForgeChronicle {
constexpr int Count=220;
constexpr int identity(int marker,int value){
 if(marker<2||marker>3||value<0||value>127)return -1;
 int index=(marker-2)*128+value;return index<Count?index:-1;
}
#pragma pack(push,1)
struct Packet {std::uint16_t command=0x87;std::uint32_t player=0,item=0;std::uint16_t recipe=0;std::uint8_t type=2;std::uint32_t minutes=0;std::uint16_t dropper=0;std::uint8_t reserved=0,flags=0;};
#pragma pack(pop)
static_assert(sizeof(Packet)==21);
static_assert(offsetof(Packet,recipe)==10 && offsetof(Packet,type)==12 && offsetof(Packet,minutes)==13 && offsetof(Packet,dropper)==17);
}
