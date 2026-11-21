#pragma once
#include "caption_text.h"
#include <mutex>
#include <unordered_map>
namespace MonsterCaption {
struct Snapshot {unsigned cls=0;MonsterModifiers::Mask mask=0;};
inline std::mutex Mutex;
inline std::unordered_map<unsigned,Snapshot> Snapshots;
inline void Clear(){std::lock_guard<std::mutex> lock(Mutex);Snapshots.clear();}
inline void Publish(void* unit,MonsterModifiers::Mask mask){
 const auto* words=static_cast<unsigned*>(unit);if(!words||words[0]!=1)return;
 std::lock_guard<std::mutex> lock(Mutex);Snapshots[words[2]]={words[1],mask};
}
inline MonsterModifiers::Mask Read(void* unit){
 const auto* words=static_cast<unsigned*>(unit);if(!words||words[0]!=1)return 0;
 std::lock_guard<std::mutex> lock(Mutex);auto it=Snapshots.find(words[2]);return it!=Snapshots.end()&&it->second.cls==words[1]?it->second.mask:0;
}
}
extern "C" {
 inline void* WorkshopMonsterCaptionResume=nullptr;
 void WorkshopMonsterCaptionHook();
 // Executed after the native panel constructor AND Monster Display's existing
 // extension. Do not hook its entry or change its return-address discriminator.
 // Rendering reads a copied ID/class/mask only, never server units or actors.
 void WorkshopMonsterCaptionAppend(void* unit,void* panel) noexcept {
  try {
   if(!panel)return;auto mask=MonsterCaption::Read(unit);if(!(mask&(MonsterModifiers::All&~((1u<<13)-1))))return;
   static thread_local std::array<std::array<char,4096>,8> buffers{};
   static thread_local unsigned cursor=0;auto& buffer=buffers[cursor++%buffers.size()];
   const char* original=nullptr;std::memcpy(&original,static_cast<unsigned char*>(panel)+0x28,8);
   if(MonsterCaption::Append(buffer.data(),buffer.size(),original,mask)){
    const char* text=buffer.data();std::memcpy(static_cast<unsigned char*>(panel)+0x28,&text,8);
   }
  } catch(...) { /* A display allocation failure must not interrupt gameplay. */ }
 }
}
namespace MonsterCaption {
inline bool Install(const D2RL::PluginContext* context){
 // Complete native instructions immediately after the monster text constructor.
 const unsigned char expected[]={0x0f,0x57,0xc0,0x48,0x8d,0x3d,0xfb,0x30,0xbf,0x01,0x33,0xc0,0x0f,0x29,0x45,0x30};
 if(!context->api->checkExpectedBytes(context,0xcfcab,expected,sizeof(expected)))return false;
 auto hook=D2RL::MakeInlineHook(0xcfcab,expected,sizeof(expected),reinterpret_cast<void*>(WorkshopMonsterCaptionHook),&WorkshopMonsterCaptionResume);
 if(!context->api->installInlineHook(context,&hook))return false;
 context->LogInfo("Monster modifier captions enabled: ten custom names; original health/resistance text retained");return true;
}
}
