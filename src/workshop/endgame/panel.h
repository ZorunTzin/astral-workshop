#pragma once
#include "panel_layout.h"
inline const PanelService* ChoicePanels=nullptr;
inline const ThreadService* ChoiceThreads=nullptr;
inline Panels::RegistrationHandle ChoicePanel=0;
inline std::atomic<bool> ChoiceSyncPending{false};
inline unsigned ChoiceShownEpoch=0;
inline bool ChoiceHidden=false,ToggleWasDown=false;inline unsigned HiddenEpoch=0;
inline bool ShowActive=false;inline int ActivePage=0;
// Same guarded child traversal and visibility setter as Astral Atlas. Avoid
// findWidget: the installed loader faults while resolving native child paths.
inline int SetNativeOffers(const Run& run) noexcept {
 int stage=run.stage;const Offer* offers=run.offers.data();int active[ModCount]{},activeCount=0;
 for(int i=0;i<ModCount;++i)if(run.bonuses[i])active[activeCount++]=i;
 int page=ActivePage%std::max(1,(activeCount+7)/8);
 __try {
  auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
  auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
  const unsigned char guard[]={0xff,0x25,0xf2,0x51,0xb3,0x03};
  if(!core||!exe||std::memcmp(exe+0x2f5020,guard,sizeof(guard)))return 0;
  auto global=*reinterpret_cast<unsigned char***>(core+0x701bb0);if(!global||!*global)return 0;
  auto roots=*reinterpret_cast<unsigned char***>(*global+0x58);auto count=*reinterpret_cast<size_t*>(*global+0x60);if(count>2048)return 0;
  int totalShown=0;
  for(size_t j=0;j<count;++j){auto root=roots[j];if(!root)continue;auto name=*reinterpret_cast<const char**>(root+8);
   if(!name||std::strcmp(name,"workshop-curses/TrialChoices"))continue;
   auto children=*reinterpret_cast<unsigned char***>(root+0x58);auto n=*reinterpret_cast<size_t*>(root+0x60);if(n>16384)return 0;int shown=0;
   for(size_t k=0;k<n;++k){auto widget=children[k];if(!widget)continue;auto key=*reinterpret_cast<const char**>(widget+8);if(!key)continue;
    int slot,s,m,t,league;bool visible=false,isOffer=false;
    if(std::sscanf(key,"LeagueOffer_%d_%d_%d_%d_%d",&slot,&s,&m,&t,&league)==5&&slot>=0&&slot<3){isOffer=true;visible=!ShowActive&&s==stage&&m==int(offers[slot].mod)&&t==offers[slot].tier&&league==offers[slot].value;}
    else if(std::sscanf(key,"Offer_%d_%d_%d_%d",&slot,&s,&m,&t)==4&&slot>=0&&slot<3){isOffer=true;visible=!ShowActive&&s==stage&&m==int(offers[slot].mod)&&t==offers[slot].tier;}
    else if(std::sscanf(key,"Category_%d_%d",&slot,&t)==2&&slot>=0&&slot<3)visible=!ShowActive&&t==int(Positive[int(offers[slot].mod)]);
    else if(std::sscanf(key,"NextWave_%d_%d",&s,&t)==2){int powers[]={10000,1000,100,10,1};int next=run.wave+1;visible=s>=0&&s<5&&next/powers[s]%10==t&&(s==4||next>=powers[s]);}
    else if(std::sscanf(key,"RerollCount_%d",&t)==1)visible=!ShowActive&&t==run.rerolls;
    else if(std::sscanf(key,"BanishCount_%d",&t)==1)visible=!ShowActive&&t==run.banishes;
    else if(!std::strcmp(key,"Reroll")||!std::strcmp(key,"Banish"))visible=!ShowActive;
    else if(std::sscanf(key,"Rarity_%d_%d",&slot,&t)==2&&slot>=0&&slot<3)visible=!ShowActive&&t==offers[slot].tier;
    else if(std::sscanf(key,"Active_%d_%d",&slot,&m)==2)visible=ShowActive&&slot>=0&&slot<8&&page*8+slot<activeCount&&active[page*8+slot]==m;
    else if(std::sscanf(key,"Value_%d_%d_%d",&slot,&s,&t)==3){int index=page*8+slot;int powers[]={10000,1000,100,10,1};visible=ShowActive&&slot>=0&&slot<8&&s>=0&&s<5&&index<activeCount&&run.bonuses[active[index]]/powers[s]%10==t&&(s==4||run.bonuses[active[index]]>=powers[s]);}
    else if(!std::strcmp(key,"ActiveNext")||!std::strcmp(key,"ActiveNextLabel"))visible=ShowActive&&activeCount>8;
    else if(!std::strcmp(key,"ActiveHelp"))visible=ShowActive;
    else if(!std::strcmp(key,"ActiveEmpty"))visible=ShowActive&&!activeCount;
    else if(!std::strncmp(key,"Card",4)||!std::strcmp(key,"Finish")||!std::strcmp(key,"FinishLabel")||!std::strcmp(key,"Hint"))visible=!ShowActive;
    else continue;
    auto table=*reinterpret_cast<void***>(widget);auto show=reinterpret_cast<void(__fastcall*)(void*,bool)>(table[10]);
    const unsigned char setter[]={0x88,0x51,0x51,0xc3};
    if(!show||std::memcmp(reinterpret_cast<const void*>(show),setter,sizeof(setter)))return 0;
    show(widget,visible);if(isOffer&&visible&&widget[0x51])++shown;
   }totalShown=std::max(totalShown,shown);
  }return totalShown;
 }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
inline void __cdecl SyncChoicePanel(const PluginContext* c,void*) noexcept {
 try {
  View view;{std::lock_guard<std::mutex> lock(ViewMutex);view=Display;}
  Panels::PanelInfo info{.structSize=Panels::PanelInfoSize};
  bool open=ChoicePanels->getPanelInfo(c,ChoicePanel,&info)==Panels::Result::Success&&info.presentationState==Panels::PresentationState::Open;
  if(HiddenEpoch!=view.run.epoch){HiddenEpoch=view.run.epoch;ChoiceHidden=false;}
  DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
  bool down=pid==GetCurrentProcessId()&&(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState('J')&0x8000);
  if(down&&!ToggleWasDown&&view.visible)ChoiceHidden=!ChoiceHidden;ToggleWasDown=down;
  bool wanted=view.visible&&view.run.phase==Phase::Choice&&view.choiceReady&&!ChoiceHidden;
  if(!wanted){if(open)ChoicePanels->closePanel(c,ChoicePanel);ChoiceShownEpoch=0;ShowActive=false;ActivePage=0;}
  else {
   if(!open){auto result=ChoicePanels->openPanel(c,ChoicePanel);ChoiceShownEpoch=0;if(result!=Panels::Result::Success)c->LogWarn("Arena native choice panel could not open");}
   // Also retry after opening: the first native root can still be cached.
   if(SetNativeOffers(view.run)==3&&ChoiceShownEpoch!=view.run.epoch){ChoiceShownEpoch=view.run.epoch;c->LogInfo("Arena native choice panel: three offer texts visible");}
  }
 }catch(...){}
 ChoiceSyncPending=false;
}
inline void RequestPanelSync(){
 if(!ChoiceThreads||!ChoicePanels||!Context)return;
 if(!ChoiceSyncPending.exchange(true)&&ChoiceThreads->runOnUiThread(Context,SyncChoicePanel,nullptr)!=Threads::Result::Success)ChoiceSyncPending=false;
}
inline SharedEvents::UiMessageAction __cdecl ChoiceMessage(const PluginContext*,const SharedEvents::UiMessageEvent* e,void*) noexcept {
 if(!e||!e->text)return SharedEvents::UiMessageAction::Continue;
 if(!std::strcmp(e->text,"TrialActive")){ShowActive=!ShowActive;ActivePage=0;return SharedEvents::UiMessageAction::Consume;}
 if(!std::strcmp(e->text,"TrialActiveNext")){++ActivePage;return SharedEvents::UiMessageAction::Consume;}
 int choice=-1;
 if(!std::strcmp(e->text,"TrialClearLoot"))choice=6;
 if(!std::strcmp(e->text,"TrialReroll"))choice=4;
 if(!std::strcmp(e->text,"TrialBanish"))choice=5;
 if(!std::strcmp(e->text,"TrialFinish"))choice=3;
 for(int i=0;i<3;++i){char key[30];std::snprintf(key,sizeof(key),"TrialChoice%d",i);if(!std::strcmp(e->text,key))choice=i;}
 if(choice<0)return SharedEvents::UiMessageAction::Continue;
 std::lock_guard<std::mutex> lock(ViewMutex);
 if(!ShowActive&&Display.visible&&Display.run.phase==Phase::Choice&&Display.choiceReady)QueueChoice(Display.run.epoch,choice);
 return SharedEvents::UiMessageAction::Consume;
}
inline bool InstallChoicePanel(const PluginContext* c){
 const ResourceService* resources=nullptr;const SharedEventService* events=nullptr;
 if(c->QueryService(&ChoicePanels)!=ServiceQueryResult::Success||c->QueryService(&ChoiceThreads)!=ServiceQueryResult::Success||c->QueryService(&resources)!=ServiceQueryResult::Success||c->QueryService(&events)!=ServiceQueryResult::Success)return false;
 Resources::ResourceRegistration resource{.structSize=Resources::ResourceRegistrationSize,.path="data/global/ui/layouts/workshop-curses/TrialChoiceshd.json",.bytes=NativeChoiceLayout,.byteCount=sizeof(NativeChoiceLayout)-1};Resources::RegistrationHandle resourceHandle=0;
 Panels::PanelRegistration reg{.structSize=Panels::PanelRegistrationSize,.flags=Panels::PanelFlags::None,.localId="TrialChoices"};
 if(resources->registerResource(c,&resource,&resourceHandle)!=Resources::Result::Success||ChoicePanels->registerPanel(c,&reg,&ChoicePanel)!=Panels::Result::Success)return false;
 SharedEvents::UiMessageListener listener{.structSize=SharedEvents::UiMessageListenerSize,.callback=ChoiceMessage};SharedEvents::ListenerHandle handle=0;
 return events->registerUiMessageListener(c,&listener,&handle)==SharedEvents::Result::Success;
}
