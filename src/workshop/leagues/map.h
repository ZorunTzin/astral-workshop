#pragma once
#include <D2RLPlugin/overlay.h>
namespace LeagueMap {
struct Point {int x,y;};struct Marker {int x,y,kind;};
using ProjectFn=Point*(__fastcall*)(void*,Point*,uint64_t);
using CoordFn=int(__fastcall*)(void*);using DimensionFn=int(__fastcall*)();
inline CoordFn ClientX=nullptr,ClientY=nullptr;inline DimensionFn Width=nullptr,Height=nullptr;
inline ProjectFn Original=nullptr;inline const D2RL::OverlayService* Api=nullptr;
inline SRWLOCK Lock=SRWLOCK_INIT;inline unsigned char Transform[0x38]{};inline ULONGLONG Observed=0;
inline std::vector<Marker> Markers;
inline Point* __fastcall project(void* context,Point* out,uint64_t coords){
 auto result=Original(context,out,coords);
 AcquireSRWLockExclusive(&Lock);std::memcpy(Transform,context,sizeof(Transform));Observed=GetTickCount64();ReleaseSRWLockExclusive(&Lock);return result;
}
inline void publish(std::vector<Marker> markers){AcquireSRWLockExclusive(&Lock);Markers=std::move(markers);ReleaseSRWLockExclusive(&Lock);}
inline void __cdecl draw(const D2RL::PluginContext* c,const D2RL::Overlay::Frame* f,void*) noexcept {
 if(!f||!Original||!Api)return;
 unsigned char transform[0x38];std::vector<Marker> markers;ULONGLONG observed;
 AcquireSRWLockShared(&Lock);std::memcpy(transform,Transform,sizeof(transform));markers=Markers;observed=Observed;ReleaseSRWLockShared(&Lock);
 if(GetTickCount64()-observed>100)return; // Native projection stops when automap is hidden.
 const D2RL::Overlay::Color colors[]={{.65f,.68f,.78f,1},{.1f,1,.25f,1},{1,.1f,.12f,1}};
 for(auto m:markers){
  if(m.kind<0||m.kind>2)continue;
  // Native static-path client coordinates; convert native viewport to overlay pixels.
  Point iso{m.x,m.y},p{};uint64_t packed=0;std::memcpy(&packed,&iso,8);Original(transform,&p,packed);
  if(p.x<0||p.y<0||p.x>Width()||p.y>Height())continue;
  float x=float(p.x)*f->screenWidth/std::max(1,Width()),y=float(p.y)*f->screenHeight/std::max(1,Height()),s=8;
  D2RL::Overlay::Point points[]={{x,y-s},{x+s,y},{x,y+s},{x-s,y}};
  for(int i=0;i<4;++i){D2RL::Overlay::LineRequest line{sizeof(line),0,f->canvas,points[i],points[(i+1)%4],colors[m.kind],2.5f,0};Api->drawLine(c,&line);}
  const char* names[]={"Night","Breach","Beyond"};D2RL::Overlay::TextRequest text{sizeof(text),0,f->canvas,{x+12,y-9},colors[m.kind],18,0,names[m.kind],unsigned(std::strlen(names[m.kind])),0};Api->drawText(c,&text);
 }
}
inline bool install(const D2RL::PluginContext* c){
 const unsigned char xy[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x85,0xc9,0x75,0x13};
 const unsigned char w[]={0x48,0x83,0xec,0x28,0xe8,0xf7,0x6c,0x7c,0x00};
 const unsigned char h[]={0x48,0x83,0xec,0x28,0xe8,0x67,0x6d,0x7c,0x00};
 if(!c->api->checkExpectedBytes(c,0x34af60,xy,sizeof(xy))||!c->api->checkExpectedBytes(c,0x34afb0,xy,sizeof(xy))||!c->api->checkExpectedBytes(c,0x7f510,w,sizeof(w))||!c->api->checkExpectedBytes(c,0x7f4a0,h,sizeof(h)))return false;
 ClientX=reinterpret_cast<CoordFn>(c->exeBase+0x34af60);ClientY=reinterpret_cast<CoordFn>(c->exeBase+0x34afb0);Width=reinterpret_cast<DimensionFn>(c->exeBase+0x7f510);Height=reinterpret_cast<DimensionFn>(c->exeBase+0x7f4a0);
 if(c->QueryService(&Api)!=D2RL::ServiceQueryResult::Success)return false;
 const unsigned char guard[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20};
 if(!c->api->checkExpectedBytes(c,0xd4910,guard,sizeof(guard)))return false;
 auto hook=D2RL::MakeInlineHook(0xd4910,guard,sizeof(guard),reinterpret_cast<void*>(project),reinterpret_cast<void**>(&Original));
 if(!c->api->installInlineHook(c,&hook))return false;
 D2RL::Overlay::CallbackRegistration reg{sizeof(reg),0,D2RL::Overlay::Phase::AfterGameUi,0,draw,nullptr};D2RL::Overlay::CallbackHandle handle{};
 return Api->registerFrameCallback(c,&reg,&handle)==D2RL::Overlay::Result::Success;
}
}
