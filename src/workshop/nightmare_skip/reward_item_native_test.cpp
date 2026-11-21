// Execute the installed loader's quantity validation branch against the actual
// reward request. No game process, character save or proprietary fixture needed.
#include "reward_item.h"
#include <windows.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <stdexcept>
using namespace D2RL;
namespace {
constexpr unsigned Entry=0x4235b3,End=0x42365a,Reject=0x423700;
int quantityWrites=0;
int __fastcall notGold(void*,int){return 0;}
int __fastcall noStackCapacity(void*){return 0;}
void __fastcall writeQuantity(void*,int,int,unsigned){++quantityWrites;}
std::vector<unsigned char> loadBranch(const char* path){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("Cannot read installed D2RCore.dll");
    auto size=file.tellg();std::vector<unsigned char> bytes(static_cast<size_t>(size));
    file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(bytes.data()+dos->e_lfanew);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||nt->Signature!=IMAGE_NT_SIGNATURE)throw std::runtime_error("Invalid loader PE");
    auto sections=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){
        auto& s=sections[i];
        if(Entry>=s.VirtualAddress && End<=s.VirtualAddress+s.SizeOfRawData){
            size_t start=s.PointerToRawData+Entry-s.VirtualAddress;
            return {bytes.begin()+start,bytes.begin()+start+(End-Entry)};
        }
    }
    throw std::runtime_error("Quantity branch not found");
}
}
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"Usage: reward_item_native_test path-to-D2RCore.dll\n";return 2;}
    try {
        auto branch=loadBranch(argv[1]);uint64_t hash=14695981039346656037ull;
        for(auto byte:branch){hash^=byte;hash*=1099511628211ull;}
        if(hash!=0x3d394ed3916fe9f3ull)throw std::runtime_error("Unsupported loader quantity branch; refusing to execute unknown bytes");
        constexpr size_t size=0x710000;
        auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        if(!memory)throw std::runtime_error("Cannot allocate test memory");
        std::memcpy(memory+Entry,branch.data(),branch.size());
        const unsigned char accept[]={0x48,0x83,0xc4,0x28,0x31,0xc0,0xc3};
        const unsigned char reject[]={0x48,0x83,0xc4,0x28,0xb8,1,0,0,0,0xc3};
        std::memcpy(memory+End,accept,sizeof(accept));std::memcpy(memory+Reject,reject,sizeof(reject));
        *reinterpret_cast<void**>(memory+0x6ffa78)=reinterpret_cast<void*>(notGold);
        *reinterpret_cast<void**>(memory+0x6ff8f8)=reinterpret_cast<void*>(noStackCapacity);
        *reinterpret_cast<void**>(memory+0x6ff0a0)=reinterpret_cast<void*>(writeQuantity);
        // Preserve the nonvolatile registers used by the captured branch, reserve
        // its frame and Windows call shadow space, and pass spec in RSI.
        unsigned char wrapper[]={
            0x55,0x56,0x53,                              // push rbp/rsi/rbx
            0x48,0x81,0xec,0x80,0x03,0,0,               // sub rsp,380h
            0x48,0x8d,0xac,0x24,0x80,0,0,0,            // lea rbp,[rsp+80h]
            0x48,0x89,0xce,                             // mov rsi,rcx
            0xe8,0,0,0,0,                              // call captured branch
            0x48,0x81,0xc4,0x80,0x03,0,0,               // add rsp,380h
            0x5b,0x5e,0x5d,0xc3};                      // pop rbx/rsi/rbp; ret
        constexpr unsigned Wrapper=0x1000,CallEnd=26;
        // The original branch starts inside a function with shadow space already
        // reserved. Supply that space before its nested native calls.
        constexpr unsigned Trampoline=0x1100;
        unsigned char trampoline[]={0x48,0x83,0xec,0x28,0xe9,0,0,0,0};
        const int32_t branchJump=Entry-(Trampoline+sizeof(trampoline));
        std::memcpy(trampoline+5,&branchJump,4);
        std::memcpy(memory+Trampoline,trampoline,sizeof(trampoline));
        const int32_t displacement=Trampoline-(Wrapper+CallEnd);
        std::memcpy(wrapper+22,&displacement,4);
        std::memcpy(memory+Wrapper,wrapper,sizeof(wrapper));
        DWORD old{};
        if(!VirtualProtect(memory,size,PAGE_EXECUTE_READ,&old))throw std::runtime_error("Cannot protect test code");
        FlushInstructionCache(GetCurrentProcess(),memory,size);
        auto validate=reinterpret_cast<Items::Result(__fastcall*)(const Items::ItemCreateSpec*)>(memory+Wrapper);
        const auto actual=NightmareSkip::rewardItemSpec(0x12345678);
        auto previous=actual;previous.quantity=1;
        auto rejected=validate(&previous),current=validate(&actual);
        std::cout<<"Installed loader, explicit quantity=1 on non-stackable reward: "<<unsigned(rejected)<<" (InvalidArgument expected)\n";
        std::cout<<"Installed loader, actual Nightmare Legacy request: "<<unsigned(current)<<" (Success expected)\n";
        VirtualFree(memory,0,MEM_RELEASE);
        if(rejected!=Items::Result::InvalidArgument||current!=Items::Result::Success||quantityWrites){std::cerr<<"FAIL: invalid non-stackable reward quantity\n";return 1;}
        std::cout<<"PASS: native quantity rejection reproduced; production request preserves default quantity\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
