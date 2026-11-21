// Optional progression after a fresh Normal Baal completion. No save-format
// extension: native Nightmare reward flags and Anya's consumed flag guard the
// single character-bound token. All mutations run on the authoritative thread.
#include "nightmare_skip/engine.h"
#include "nightmare_skip/quests.h"
#include "nightmare_skip/reward_item.h"
#include "combat/src/curses_native.h"
namespace WorkshopNightmare {
constexpr unsigned TokenCode=Items::MakeItemCode("Wnl");
inline bool isToken(unsigned code){return code==TokenCode||code==Items::MakeItemCode("Wnsk");}
using ApplyResist=void(__fastcall*)(void*,void*);
inline ApplyResist ApplyResistance{};
using Pointer=void*(__fastcall*)(void*);
using Progress=void(__fastcall*)(void*,unsigned short,unsigned short);
using AddStat=void(__fastcall*)(void*,int,int,unsigned);
using Lookup=void*(__fastcall*)(void*,int,unsigned);
inline Lookup ServerUnit{};
inline Pointer Client{},Game{};inline Progress Unlock{};inline AddStat Add{};
inline const PluginContext* Context{};
inline Panels::RegistrationHandle ChoicePanel{},DonePanel{},SpacePanel{};
inline std::atomic<unsigned> PlayerId{~0u};
inline std::atomic<bool> Enabled{false},Busy{false},OfferVisible{false};
inline std::atomic<bool> Eligible{false},Baselined{false};
inline std::atomic<unsigned> Epoch{0};
inline ULONGLONG LastPoll{};
inline Items::ItemInfo ConsumeIdentity{},HoveredIdentity{};
inline PlayerHandle HoveredOwner{},ConsumeOwner{};
inline ULONGLONG HoveredAt{};
inline POINT HoveredPoint{};
inline bool RightWasDown{};
inline const char* InspectStage="not started";
inline ULONGLONG LastInspectWarning{};
struct Native {
    void* unit{};void* game{};void* client{};
    unsigned short* quests[3]{};unsigned difficulty{},progress{},key{};
};
// Resolve the current server unit by id each time; never mutate client units.
inline bool inspect(unsigned id,Native& out) noexcept {
    __try {
        // Combat already exports the game observed by its existing server hook.
        // Read it only here on the authoritative game thread; do not retain it.
        InspectStage="observed server game";
        auto combat=GetModuleHandleW(L"d2rl-workshop-combat.dll");
        auto exported=combat?reinterpret_cast<void***>(GetProcAddress(combat,"FMMP_Curse_ObservedGame")):nullptr;
        if(!exported||!*exported||!**exported)return false;
        void* currentGame=**exported;
        InspectStage="game-scoped server unit lookup";
        auto u=static_cast<unsigned char*>(ServerUnit(currentGame,0,id));
        if(u&&*reinterpret_cast<unsigned*>(u)==0&&*reinterpret_cast<unsigned*>(u+8)==id){
                InspectStage="player data and native game/client";
                auto data=*reinterpret_cast<unsigned char**>(u+0x10);
                auto game=Game(u);auto client=Client(u);
                if(!data||!game||game!=currentGame||!client)return false;
                InspectStage="difficulty and progression";
                out.unit=u;out.game=game;out.client=client;
                out.difficulty=*(static_cast<unsigned char*>(game)+0x104);
                out.progress=(*reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(client)+14)>>8)&31;
                if(out.difficulty>2)return false;
                InspectStage="character name";
                unsigned key=2166136261u;unsigned n=0;
                for(;n<16&&data[n];++n){key^=data[n];key*=16777619u;}
                if(n==0||n==16)return false;
                key^=*reinterpret_cast<unsigned*>(u+4);key*=16777619u;out.key=key?key:1;
                InspectStage="quest records";
                for(unsigned d=0;d<3;++d){
                    auto record=*reinterpret_cast<void**>(data+0x40+8*d);
                    if(!record)return false;
                    out.quests[d]=*reinterpret_cast<unsigned short**>(record);
                    if(!out.quests[d])return false;
                    volatile auto last=out.quests[d][47];(void)last;
                }
                InspectStage="ready";
                return true;
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){}
    return false;
}
inline bool untouched(const Native& n){
    return NightmareSkip::untouchedNightmare(n.quests[1]);
}
inline bool pending(const Native& n){
    if(!(n.quests[0][40]&1)||n.progress<10||(n.quests[1][37]&(1u<<7)))return false;
    return NightmareSkip::rewardPending(n.quests[1]);
}
inline void __cdecl show(const PluginContext* c,void* arg) noexcept {
    auto payload=reinterpret_cast<uintptr_t>(arg);
    if(unsigned(payload>>32)!=Epoch.load()||PlayerId==~0u)return;
    auto which=unsigned(payload);
    if(which==1&&OfferVisible){panels->closePanel(c,SpacePanel);panels->openPanel(c,ChoicePanel);}
    if(which==2){panels->closePanel(c,ChoicePanel);panels->openPanel(c,DonePanel);}
    if(which==3){panels->closePanel(c,ChoicePanel);panels->openPanel(c,SpacePanel);}
    if(which==4)panels->closePanel(c,ChoicePanel);
}
inline void notify(const PluginContext* c,unsigned which){threads->runOnUiThread(c,show,reinterpret_cast<void*>((uintptr_t(Epoch.load())<<32)|which));}
inline void __cdecl execute(const PluginContext* c,void* arg) noexcept {
    auto payload=reinterpret_cast<uintptr_t>(arg);
    if(unsigned(payload>>32)!=Epoch.load())return;
    const unsigned action=unsigned(payload);
    Native n;const unsigned id=PlayerId.load();
    if(!Enabled||!inspect(id,n)){
        auto now=GetTickCount64();
        if(!LastInspectWarning||now-LastInspectWarning>=30000){
            LastInspectWarning=now;
            char line[200];std::snprintf(line,sizeof(line),"[nightmare-skip] Cannot inspect player=%u at %s; retrying.",id,InspectStage);c->LogWarn(line);
        }
        Busy=false;return;
    }
    if(action==0){
        if(!Baselined){
            Eligible=NightmareSkip::eligibleBaseline(n.difficulty,n.progress,n.quests[0][40],untouched(n));
            Baselined=true;
            char line[240];std::snprintf(line,sizeof(line),"[nightmare-skip] Baseline: player=%u difficulty=%u progression=%u Baal=%04x untouchedNM=%u eligible=%u",id,n.difficulty,n.progress,n.quests[0][40],unsigned(untouched(n)),unsigned(Eligible.load()));c->LogInfo(line);
        }
        Busy=false;return;
    }
    if(action==1){
        if(NightmareSkip::completionReady(Eligible,n.difficulty,n.progress,n.quests[0][40],untouched(n))){
            Eligible=false;OfferVisible=true;notify(c,1);c->LogInfo("[nightmare-skip] Fresh Normal Baal completion: choice available.");
        }
        // A quest event may precede the progression update. Keep waiting.
        Busy=false;return;
    }
    if(action==2){OfferVisible=false;notify(c,4);Busy=false;return;}
    if(action==3){
        if(!OfferVisible||n.difficulty!=0||n.progress!=5||!(n.quests[0][40]&1)||!untouched(n)){Busy=false;return;}
        PlayerHandle player{};
        if(inventory->getLocalPlayer(c,&player)!=Inventory::Result::Success){Busy=false;return;}
        const auto spec=NightmareSkip::rewardItemSpec(n.key);
        ItemHandle item{};
        const auto result=items->createItem(c,player,&spec,&item);
        if(result!=Items::Result::Success){
            char line[200];std::snprintf(line,sizeof(line),"[nightmare-skip] Nightmare Legacy creation failed: Items::Result=%u player=%u; quests and unlock unchanged.",unsigned(result),id);c->LogWarn(line);
            notify(c,3);Busy=false;return;
        }
        // No save can interleave these synchronous game-thread mutations.
        // Clear all reward-pending/custom bits, including Radament's book and
        // Alkor's life potion entitlement. Anya's bit 7 remains unset until use.
        NightmareSkip::completeNightmare(n.quests[1]);
        Unlock(n.client,5,1);
        OfferVisible=false;notify(c,2);
        c->LogInfo("[nightmare-skip] Hell unlocked; Nightmare quests rewarded; bound token created.");
        Busy=false;return;
    }
    if(action==4){
        Items::ItemInfo info{.structSize=Items::ItemInfoSize};
        PlayerHandle owner=ConsumeOwner;
        auto infoResult=Items::Result::NotFound;
        if(owner){
            Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,
                .containerMask=Items::ContainerBit(Items::ItemContainer::Inventory)};
            const auto scan=inventory->forEachInventoryItem(c,owner,&filter,
                [](const PluginContext*,const Items::ItemInfo* candidate,void* found) noexcept {
                    if(candidate&&NightmareSkip::sameRewardItem(ConsumeIdentity,*candidate)){
                        *static_cast<Items::ItemInfo*>(found)=*candidate;
                        return Inventory::IterationAction::Stop;
                    }
                    return Inventory::IterationAction::Continue;
                },&info);
            if(scan==Inventory::Result::Success&&info.handle)infoResult=Items::Result::Success;
        }
        const ItemHandle item=info.handle;
        const bool questPending=pending(n); // diagnostic only; consuming has no quest gate
        if(infoResult!=Items::Result::Success||
           !isToken(info.code)||info.container!=Items::ItemContainer::Inventory){
            char line[320];std::snprintf(line,sizeof(line),"[nightmare-skip] Reward rejected: pending=%u infoResult=%u code=%08x container=%u owner=%08x itemSeed=%08x generationSeed=%08x progression=%u Anya=%04x",unsigned(questPending),unsigned(infoResult),info.code,unsigned(info.container),n.key,info.itemSeed,info.generationSeed,n.progress,n.quests[1][37]);c->LogWarn(line);Busy=false;return;
        }
        const auto destroyed=items->destroyItem(c,owner,item,Items::SocketedItemPolicy::RejectIfNotEmpty);
        if(destroyed!=Items::Result::Success){
            char line[160];std::snprintf(line,sizeof(line),"[nightmare-skip] Reward removal failed: result=%u; bonuses unchanged.",unsigned(destroyed));c->LogWarn(line);Busy=false;return;
        }
        // Base resistance stats are not saved. Apply only the +10 delta now;
        // native Anya reconstruction restores the total from quest flags on load.
        // Calling native ApplyResistanceReward here would add Normal's bonus again.
        n.quests[1][37]|=1u<<7;
        Add(n.unit,5,4,0);Add(n.unit,4,5,0);Add(n.unit,7,20*256,0);
        // Native Anya builds and synchronizes a resistance stat list. Isolate
        // the Nightmare contribution during this synchronous call so existing
        // Normal/Hell rewards are not added again. Restore their flags at once.
        const auto normalAnya=n.quests[0][37],hellAnya=n.quests[2][37];
        n.quests[0][37]&=~(1u<<7);n.quests[2][37]&=~(1u<<7);
        ApplyResistance(n.game,n.unit);
        n.quests[0][37]=normalAnya;n.quests[2][37]=hellAnya;
        c->LogInfo("[nightmare-skip] Consumed reward: 4 skills, 5 attributes, 20 life, 10 all resistances.");
    }
    Busy=false;
}
inline bool queue(const PluginContext* c,unsigned action){
    if(Busy.exchange(true))return false;
    if(threads->runOnGameThread(c,execute,reinterpret_cast<void*>((uintptr_t(Epoch.load())<<32)|action))!=Threads::Result::Success){Busy=false;return false;}
    return true;
}
// Tooltip capture identifies the exact item under the mouse without blocking
// inventory pickup. UI Activate is not a reliable right-click notification.
inline void __cdecl hover(const PluginContext* c,SharedEvents::ItemTooltipEvent* e,void*) noexcept {
    HoveredAt=0;
    if(!Enabled||!e)return;
    Items::ItemInfo info{.structSize=Items::ItemInfoSize};
    if(items->getItemInfo(c,e->item,&info)!=Items::Result::Success||!isToken(info.code)||
       info.container!=Items::ItemContainer::Inventory)return;
    if(inventory->getLocalPlayer(c,&HoveredOwner)!=Inventory::Result::Success)return;
    HoveredIdentity=info;GetCursorPos(&HoveredPoint);HoveredAt=GetTickCount64();
}
inline void __cdecl poll(const PluginContext* c,const Overlay::Frame*,void*) noexcept {
    DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
    const bool right=process==GetCurrentProcessId()&&(GetAsyncKeyState(VK_RBUTTON)&0x8000);
    const bool pressed=right&&!RightWasDown;RightWasDown=right;
    auto now=GetTickCount64();
    if(!Enabled||PlayerId==~0u)return;
    POINT cursor{};GetCursorPos(&cursor);
    if(pressed&&HoveredAt&&now-HoveredAt<100&&
       cursor.x==HoveredPoint.x&&cursor.y==HoveredPoint.y&&!Busy.exchange(true)){
        ConsumeIdentity=HoveredIdentity;ConsumeOwner=HoveredOwner;
        c->LogInfo("[nightmare-skip] Right-clicked inventory reward; resolving server item.");
        if(threads->runOnGameThread(c,execute,reinterpret_cast<void*>((uintptr_t(Epoch.load())<<32)|4))!=Threads::Result::Success)Busy=false;
    }
    if(Busy||(Baselined&&!Eligible)||now-LastPoll<250)return;
    LastPoll=now;queue(c,Baselined?1:0);
}
inline void __cdecl event(const PluginContext* c,const Lifecycle::GameplayEvent* e,void*) noexcept {
    if(!Enabled||!e||e->structSize<Lifecycle::GameplayEventRequiredSize)return;
    if(e->kind==Lifecycle::GameplayEventKind::GameLeft){++Epoch;HoveredAt=0;PlayerId=~0u;Eligible=false;Baselined=false;OfferVisible=false;Busy=false;return;}
    if(e->kind==Lifecycle::GameplayEventKind::LocalPlayerReady){++Epoch;PlayerId=e->playerId;Eligible=false;Baselined=false;OfferVisible=false;Busy=false;queue(c,0);return;}
    if(e->kind==Lifecycle::GameplayEventKind::QuestCompleted&&e->difficulty==0&&e->questRecordId==40&&e->playerId==PlayerId.load())queue(c,1);
}
inline SharedEvents::UiMessageAction __cdecl message(const PluginContext* c,const SharedEvents::UiMessageEvent* e,void*) noexcept {
    if(!Enabled||!e||!e->target||!e->command||!e->text||_stricmp(e->target,"PanelManager")||_stricmp(e->command,"ClosePanel"))return SharedEvents::UiMessageAction::Continue;
    if(!std::strcmp(e->text,"WorkshopContinueNightmare")){queue(c,2);return SharedEvents::UiMessageAction::Consume;}
    if(!std::strcmp(e->text,"WorkshopSkipNightmare")){queue(c,3);return SharedEvents::UiMessageAction::Consume;}
    return SharedEvents::UiMessageAction::Continue;
}
inline ItemInteractions::Decision __cdecl use(const PluginContext* c,const ItemInteractions::ItemInteractionEvent* e,void*) noexcept {
    if(!Enabled||!e||e->structSize<ItemInteractions::ItemInteractionEventRequiredSize)return ItemInteractions::Decision::Continue;
    // Mouse use is handled by tooltip + right-click polling; leave pickup free.
    if(e->inputSource!=ItemInteractions::InputSource::Controller)return ItemInteractions::Decision::Continue;
    Items::ItemInfo info{.structSize=Items::ItemInfoSize};
    if(items->getItemInfo(c,e->item,&info)!=Items::Result::Success||!isToken(info.code))return ItemInteractions::Decision::Continue;
    if(e->container==Items::ItemContainer::Inventory&&!Busy.exchange(true)){
        ConsumeIdentity=info;ConsumeOwner=e->player;
        if(threads->runOnGameThread(c,execute,reinterpret_cast<void*>((uintptr_t(Epoch.load())<<32)|4))!=Threads::Result::Success)Busy=false;
    }
    return ItemInteractions::Decision::Consume;
}
#include "nightmare_skip/layout.h"
inline Input::ActionResult __cdecl reopen(const PluginContext* c,const Input::ActionEvent* e,void*) noexcept {
    if(Enabled&&OfferVisible&&e&&e->kind==Input::ActionEventKind::Pressed){notify(c,1);return Input::ActionResult::Handled;}
    return Input::ActionResult::Ignored;
}
inline bool install(const PluginContext* c,const SharedEventService* events){
    Context=c;
    auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const unsigned char addGuard[]={0xff,0x25,0x5a,0x6d,0xb3,0x03};
    const unsigned char clientGuard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x85,0xc9,0x74,0x1e};
    const unsigned char findGuard[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
    // Share the proven game accessor with Combat. UnitTimerRva returns the
    // event list at +0x148; GameRva returns the actual game at +0xd8.
    // Full leaf progression function guard includes save-flags offset and mask.
    const auto hash=[](const unsigned char* p,size_t n){uint64_t h=14695981039346656037ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;};
    if(std::memcmp(base+0x2f34f0,addGuard,sizeof(addGuard))||std::memcmp(base+0x48fde0,clientGuard,sizeof(clientGuard))||
       std::memcmp(base+CurseNative::GameRva,CurseNative::GameBytes,sizeof(CurseNative::GameBytes))||hash(base+0x486490,0x67)!=ProgressionHash||hash(base+0x5472a0,0x90)!=QuestLayoutHash||std::memcmp(base+0x48fe80,findGuard,sizeof(findGuard)))return false;
    ServerUnit=reinterpret_cast<Lookup>(base+0x48fe80);
    Client=reinterpret_cast<Pointer>(base+0x48fde0);Game=reinterpret_cast<Pointer>(base+CurseNative::GameRva);
    ApplyResistance=reinterpret_cast<ApplyResist>(base+0x5472a0);
    Unlock=reinterpret_cast<Progress>(base+0x486490);Add=reinterpret_cast<AddStat>(base+0x2f34f0);
    const LifecycleService* lifecycle{};const ItemInteractionService* interactions{};const ResourceService* resources{};
    if(c->QueryService(&lifecycle)!=ServiceQueryResult::Success||!HasLifecycleServiceField(lifecycle,40)||
       c->QueryService(&interactions)!=ServiceQueryResult::Success||!HasItemInteractionServiceField(interactions,24)||
       c->QueryService(&resources)!=ServiceQueryResult::Success)return false;
    auto panel=[&](const char* name,const char* bytes,Panels::RegistrationHandle& handle){
        std::string path="data/global/ui/layouts/astral-workshop/"+std::string(name)+"hd.json";
        Resources::ResourceRegistration res{.structSize=Resources::ResourceRegistrationSize,.path=path.c_str(),.bytes=bytes,.byteCount=unsigned(std::strlen(bytes))};
        Resources::RegistrationHandle rh{};
        Panels::PanelRegistration reg{.structSize=Panels::PanelRegistrationSize,.flags=Panels::PanelFlags::CloseOnEscape,.localId=name};
        return resources->registerResource(c,&res,&rh)==Resources::Result::Success&&panels->registerPanel(c,&reg,&handle)==Panels::Result::Success;
    };
    if(!panel("NightmareChoice",ChoiceLayout,ChoicePanel)||!panel("NightmareDone",DoneLayout,DonePanel)||!panel("NightmareSpace",SpaceLayout,SpacePanel))return false;
    SharedEvents::UiMessageListener msg{.structSize=SharedEvents::UiMessageListenerSize,.callback=message};SharedEvents::ListenerHandle mh{};
    if(events->registerUiMessageListener(c,&msg,&mh)!=SharedEvents::Result::Success)return false;
    SharedEvents::ItemTooltipListener tooltip{.structSize=SharedEvents::ItemTooltipListenerSize,
        .region=SharedEvents::ItemTooltipRegion::Description,.position=SharedEvents::ItemTooltipPosition::Bottom,.callback=hover};
    SharedEvents::ListenerHandle tooltipHandle{};
    if(events->registerItemTooltipListener(c,&tooltip,&tooltipHandle)!=SharedEvents::Result::Success)return false;
    ItemInteractions::ItemInteractionListener activation{.structSize=ItemInteractions::ItemInteractionListenerRequiredSize,.callback=use};ItemInteractions::ListenerHandle ih{};
    if(interactions->registerListener(c,&activation,&ih)!=ItemInteractions::Result::Success)return false;
    for(auto kind:{Lifecycle::GameplayEventKind::GameLeft,Lifecycle::GameplayEventKind::LocalPlayerReady,Lifecycle::GameplayEventKind::QuestCompleted}){
        Lifecycle::GameplayEventListener listener{.structSize=Lifecycle::GameplayEventListenerSize,.kind=kind,.callback=event};Lifecycle::ListenerHandle h{};
        if(lifecycle->registerGameplayEventListener(c,&listener,&h)!=Lifecycle::Result::Success)return false;
    }
    const InputService* input{};
    if(c->QueryService(&input)!=ServiceQueryResult::Success)return false;
    Input::ActionRegistration key{.structSize=Input::ActionRegistrationSize,.logicalId="nightmare-choice",.displayName="Reopen difficulty choice",.category="Reimagined Workshop",.defaultPrimary={.key=Input::Key::F10},.callback=reopen};
    Input::ActionHandle kh{};if(input->registerAction(c,&key,&kh)!=Input::Result::Success)return false;
    Overlay::CallbackRegistration polling{.structSize=Overlay::CallbackRegistrationSize,.phase=Overlay::Phase::AfterGameUi,.callback=poll};
    Overlay::CallbackHandle ph{};
    if(overlay->registerFrameCallback(c,&polling,&ph)!=Overlay::Result::Success)return false;
    Enabled=true;c->LogInfo("[nightmare-skip] Optional fresh Normal Baal choice enabled.");return true;
}
}
