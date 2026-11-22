#include "item_sort_model.h"
#include <D2RLPlugin/api.h>
#define NOMINMAX
#include <windows.h>
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#include <filesystem>
#include <fstream>
#include <string>
#include <cmath>
#include <array>
#include <algorithm>
#include <mutex>
#include <vector>
#include <atomic>
#include <bit>
#include <random>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include "stash_organizer_model.h"
using namespace D2RL;

namespace {
constexpr char id[]="astral-workshop";
#include "stars.inc"
constexpr char layout[]=
#include "unified.inc"
;
constexpr PluginInfo info { .infoSize=PluginInfoSize, .abiVersion=D2RL_PLUGIN_ABI_VERSION,
    .id=id, .name="Astral Passives", .version="0.20.2", .author="Reimagined Workshop",
    .description="Plan a constellation and inscribe it for the Horadric Cube.", .flags=PluginFlags::Shared | PluginFlags::NativeHooks };
const PanelService* panels{};
const WidgetService* widgets{};
const ThreadService* threads{};
const ItemService* items{};
const InventoryService* inventory{};
static_assert(std::size(stars)==9);
struct Build : std::array<unsigned,9> { std::array<unsigned char,112> depth{}; };
unsigned number(unsigned p,unsigned i){return i<12?12*p+i:108+i-12;}

using RequiredLevelFn=int(__fastcall*)(void*,void*);
using ReadStatFn=int(__fastcall*)(void*,int,unsigned);
RequiredLevelFn originalRequiredLevel{};
ReadStatFn readItemStat{};
#include "stash_navigation.h"
void __cdecl resolveStashProfile(const PluginContext*,void*) noexcept;
#include "stash_navigation.inc"
#include "forge_native.inc"
#include "table_audit.inc"
#include "forge_tooltip.inc"
#include "forge_chronicle_native.inc"
#include "endgame/seal_tooltip.inc"
// Override only items carrying this Workshop-owned requirement stat.
// Native skill grants otherwise raise the base requirement before levelreq
// modifiers are added, and oskills can differ with the receiving class.
int __fastcall sigilRequiredLevel(void* item,void* player){
    if(item && *static_cast<unsigned*>(item)==4){
        int level=readItemStat(item,pointRequirementStat,0);
        if(level>0&&level<=99)return level;
    }
    int required=originalRequiredLevel(item,player);
    forgeTooltipSnapshot={};
    if(item && *static_cast<unsigned*>(item)==4){
        const int marker=readItemStat(item,forgeMarkerStat,0),value=readItemStat(item,forgeLevelStat,0);
        required=std::max(required,forgeRequiredLevel(marker,value))+std::max(0,readItemStat(item,LeagueCraft::LevelStat,0));
        forgeTooltipSnapshot={static_cast<unsigned*>(item)[2],marker,value,GetTickCount64()};
    }
    return required;
}
#include "leagues/craft_native.inc"
Panels::RegistrationHandle panel{};
Build selected{};
unsigned budget=0;
// The installed loader predates a public player-stat service. Resolve the same
// native local-player accessors used by its InventoryService, with byte guards.
void* localPlayerUnit(){
    auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
    auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const unsigned char guard[]={0xff,0x15,0xfe,0x39,0x2e,0x00};
    const unsigned char statGuard[]={0xff,0x25,0xf2,0x51,0xb3,0x03};
    if(!core||!exe||std::memcmp(core+0x41caa4,guard,sizeof(guard))||
       std::memcmp(exe+0x2f5020,statGuard,sizeof(statGuard)))return nullptr;
    auto index=*reinterpret_cast<unsigned(__fastcall**)()>(core+0x7004a8);
    auto lookup=*reinterpret_cast<void*(__fastcall**)(unsigned)>(core+0x7004d8);
    if(!index||!lookup)return nullptr;
    return lookup(index());
}
unsigned characterLevel(){
    void* player=localPlayerUnit();if(!player)return 0;
    auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto get=reinterpret_cast<int(__fastcall*)(void*,int,unsigned)>(exe+0x2f5020);
    int value=get(player,12,0);
    return value>0&&value<=100?static_cast<unsigned>(value):0;
}

unsigned (*readLevel)()=characterLevel;
PlayerHandle editingPlayer{};
// Keep allocation IDs stable when importing an older carried Sigil. Removing
// the retired cluster refunds its points; Apply rebuilds the current bonuses.
void migrateRetired(Build& build){
    build[6]&=~(1u<<12);
    build.depth[retiredRoot]=0;
}
unsigned points(const Build& build){
    unsigned result=0;
    for(auto mask:build)result+=std::popcount(mask);
    for(auto extra:build.depth) result+=extra;
    return result;
}
unsigned keystones(const Build& build){unsigned n=0;for(auto mask:build)n+=std::popcount((mask>>9)&7u);return n;}
unsigned keystoneSlots(unsigned level){return std::min(level/30,3u);}
unsigned requiredLevel(const Build& build){return std::max({1u,points(build),30u*keystones(build)});}
bool reachable(const Build& build,unsigned p,unsigned node){
    if(p>=9 || node>=(p==6?16u:12u))return false;
    unsigned parent=parentNodes[number(p,node)];
    if(parent==65535)return true;
    if(parent>=16)return false;
    return bool(build[p]&(1u<<parent));
}
void prune(Build& build){
    migrateRetired(build);
    // A removed stem returns all now-unreachable descendants. Iterate until
    // stable because numeric IDs preserve the old saved ordering, not depth.
    bool changed;
    do {
        changed=false;
        for(unsigned p=0;p<9;p++)for(unsigned i=0;i<(p==6?16u:12u);i++){
            if((build[p]&(1u<<i))&&!reachable(build,p,i)){
                build[p]&=~(1u<<i);changed=true;
            }
        }
    }while(changed);
    for(unsigned p=0;p<9;p++)for(unsigned i=0;i<(p==6?16u:12u);i++)
        if(!(build[p]&(1u<<i)))build.depth[number(p,i)]=0;
}

bool valid(const Build& build,bool enforceKeystones=true){
    if((build[6]&(1u<<12))||build.depth[retiredRoot])return false;
    int keys=0;
    for(unsigned p=0;p<9;p++){
        if(build[p]>=(p==6?65536u:4096u))return false;
        keys+=std::popcount((build[p]>>9)&7u);
        for(unsigned i=0;i<(p==6?16u:12u);i++)if((build[p]&(1u<<i)) && !reachable(build,p,i))return false;
    }
    for(unsigned p=0;p<9;p++)for(unsigned i=0;i<(p==6?16u:12u);i++){
        unsigned n=number(p,i),extra=build.depth[n];
        if(extra>4 || (extra && (!(build[p]&(1u<<i)) || (i>=9 && i<12))))return false;
    }
    return (!enforceKeystones||keys<=3) && points(build)<=99;
}
struct InventoryState { ItemHandle sigil{}; unsigned level{},row{},count{}; };
Inventory::IterationAction inspect(const PluginContext*,const Items::ItemInfo* item,void* data) noexcept {
    auto& state=*static_cast<InventoryState*>(data);
    if(item->code==Items::MakeItemCode("Wcap")) state.level=std::max(state.level,item->itemLevel);
    if(item->code==Items::MakeItemCode("Wsig") && item->quality==Items::Quality::Unique){
        state.sigil=item->handle;state.row=item->qualityRecordId;++state.count;
    }
    return Inventory::IterationAction::Continue;
}
bool scan(const PluginContext* c,PlayerHandle p,InventoryState& state){
    Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,
        .containerMask=Items::ContainerBit(Items::ItemContainer::Inventory)};
    return inventory->forEachInventoryItem(c,p,&filter,inspect,&state)==Inventory::Result::Success;
}
// ABI 4 loaders predating getItemDetails still expose saved item seeds. Keep a
// local receipt so these loaders can transfer the exact roll shown on the plan.
struct Receipt {
    unsigned magic=0x34504c41;
    Build allocation{};
    unsigned count{};
    std::array<Items::PropertySpec,Items::MaxProperties> effects{};
};
std::filesystem::path ledger;
unsigned fingerprint(const Receipt& r){
    unsigned h=2166136261u;
    for(auto b:std::bit_cast<std::array<unsigned char,sizeof(Receipt)>>(r))h=(h^b)*16777619u;
    return h?h:1;
}
bool saveRecord(unsigned seed,const Receipt& r){
    if(!seed)return false;
    auto f=ledger/std::to_wstring(seed);
    if(std::filesystem::exists(f)){
        Receipt old{};std::ifstream in(f,std::ios::binary);
        return bool(in.read(reinterpret_cast<char*>(&old),sizeof(old)))&&!std::memcmp(&old,&r,sizeof(r));
    }
    auto temp=f;temp+=L".tmp";
    {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(&r),sizeof(r));out.close();if(!out)return false;}
    return MoveFileExW(temp.c_str(),f.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
}
bool recorded(const PluginContext* c,ItemHandle item,Receipt& r){
    Items::ItemInfo info{.structSize=Items::ItemInfoSize};
    if(items->getItemInfo(c,item,&info)!=Items::Result::Success)return false;
    for(auto key:{info.itemSeed,info.generationSeed}){
        if(!key)continue;
        std::ifstream in(ledger/std::to_wstring(key),std::ios::binary);
        unsigned tag{};in.read(reinterpret_cast<char*>(&tag),sizeof(tag));in.seekg(0);
        if(tag==0x33504c41){
            Receipt migrated{};
            in.seekg(sizeof(tag));in.read(reinterpret_cast<char*>(migrated.allocation.data()),9*sizeof(unsigned));
            prune(migrated.allocation);
            if(in && valid(migrated.allocation,false)){r=migrated;return true;}
        }
        if(in.read(reinterpret_cast<char*>(&r),sizeof(r)) && r.magic==0x34504c41 && r.count<=Items::MaxProperties){
            prune(r.allocation);
            if(valid(r.allocation,false))return true;
        }
    }
    return false;
}
bool readItem(const PluginContext* c,ItemHandle handle,Build& result){
    Receipt r{};if(recorded(c,handle,r)){result=r.allocation;return true;}
    if(!HasItemServiceField(items,ItemServiceDetailsFieldEnd))return false;
    Items::ItemDetails details{.structSize=Items::ItemDetailsSize};
    std::array<Items::ItemStatValue,256> stats{};
    if(items->getItemDetails(c,handle,&details,stats.data(),stats.size())!=Items::Result::Success)return false;
    Build saved{};
    for(unsigned i=0;i<details.statCount;i++){
        auto index=stats[i].statId-firstMaskStat;
        if(index<saved.size())saved[index]|=stats[i].value;
        if(stats[i].statId==petMaskStat)saved[6]|=unsigned(stats[i].value)<<12;
    }
    prune(saved);
    if(!valid(saved,false))return false;
    result=saved;return true;
}
bool readBuild(const PluginContext* c,PlayerHandle player,Build& result){
    InventoryState state{};
    return scan(c,player,state)&&state.count==1&&readItem(c,state.sigil,result);
}
#include "tree_persistence.inc"
const OverlayService* overlay{};
#include "visual.inc"
std::atomic<bool> treeVisible=false;
bool visualReady=false;
unsigned initialRefreshes=0;
std::atomic<bool> tickPending=false;
std::atomic<int> pendingNode=-1;
bool choose(const PluginContext*,const char*);
// The installed loader resolves roots but faults while resolving child paths.
// Resolve this panel's direct children through the native layout verified for
// build 93847. UI mutation stays on the UI thread and uses native virtual calls.
void updateNative(const PluginContext* ctx) noexcept {
    __try {
        auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
        auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
        const unsigned char guard[]={0xff,0x25,0xf2,0x51,0xb3,0x03};
        if(!core||!exe||std::memcmp(exe+0x2f5020,guard,sizeof(guard)))return;
        auto global=*reinterpret_cast<unsigned char***>(core+0x701bb0);
        if(!global||!*global)return;
        auto manager=*global;
        auto roots=*reinterpret_cast<unsigned char***>(manager+0x58);
        auto count=*reinterpret_cast<size_t*>(manager+0x60);
        if(count>2048)return;
        unsigned char* root=nullptr;
        for(size_t j=0;j<count;j++){
            auto candidate=roots[j];if(!candidate)continue;
            auto name=*reinterpret_cast<const char**>(candidate+8);
            if(name&&!std::strcmp(name,"astral-workshop/Atlas")){root=candidate;break;}
        }
        if(!root)return;
        auto children=*reinterpret_cast<unsigned char***>(root+0x58);
        count=*reinterpret_cast<size_t*>(root+0x60);
        if(count>2048)return;
        constexpr unsigned segments[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
        unsigned spent=points(selected),remaining=budget>spent?budget-spent:0;
        remaining=std::min(remaining,99u);
        unsigned marks=0,digits=0;
        for(size_t j=0;j<count;j++){
            auto widget=children[j];if(!widget)continue;
            auto name=*reinterpret_cast<const char**>(widget+8);if(!name)continue;
            bool visible=false,change=false;
            if(!std::strncmp(name,"Ring",4)){
                unsigned uid=std::strtoul(name+4,nullptr,10);
                if(uid>=std::size(visualNodes))continue;
                unsigned p,i,rank=0;
                if(uid<112){p=uid<108?uid/12:6;i=uid<108?uid%12:12+uid-108;}
                else{auto e=extensions[uid-112];p=e.region;i=e.node;rank=e.rank;}
                visible=rank?selected.depth[number(p,i)]>=rank:bool(selected[p]&(1u<<i));
                change=true;marks++;
            }
            if(!std::strncmp(name,"Digit",5)){
                unsigned digit{},bar{};
                if(std::sscanf(name,"Digit%uBar%u",&digit,&bar)==2&&digit<2&&bar<7){
                    visible=bool(segments[digit?remaining%10:remaining/10]&(1u<<bar));change=true;digits++;
                }
            }
            if(change){
                auto table=*reinterpret_cast<void***>(widget);
                // In build 93847 the draw traversal checks byte 0x51.
                // Slot 9 changes 0x50, which controls update/input activity.
                // Verify the small native setter before invoking slot 10.
                auto show=reinterpret_cast<void(__fastcall*)(void*,bool)>(table[10]);
                const unsigned char setter[]={0x88,0x51,0x51,0xc3};
                if(show&&!std::memcmp(reinterpret_cast<const void*>(show),setter,sizeof(setter))){
                    show(widget,visible);
                    if(bool(widget[0x51])!=visible)ctx->LogError("Native visibility readback mismatch");
                }else ctx->LogError("Unsupported native visibility setter");
            }
        }
        visualReady=marks==activeNodeCount&&digits==14;
        char message[100];std::snprintf(message,sizeof(message),"Native update marks=%u digits=%u remaining=%u",marks,digits,remaining);ctx->LogInfo(message);
    }__except(EXCEPTION_EXECUTE_HANDLER){ctx->LogError("Native Astral UI update failed");}
}
void refresh(const PluginContext* ctx,void*) noexcept {
    PlayerHandle local{};
    budget=inventory->getLocalPlayer(ctx,&local)==Inventory::Result::Success?readLevel():0;
    persistDraft(ctx);
    if(treeVisible)updateNative(ctx);
}
void tickTree(const PluginContext* ctx,void*) noexcept {
    Panels::PanelInfo state{.structSize=Panels::PanelInfoSize};
    bool shown=panels->getPanelInfo(ctx,panel,&state)==Panels::Result::Success &&
        state.presentationState==Panels::PresentationState::Open;
    treeVisible=shown;
    if(shown&&(!visualReady||initialRefreshes)){
        refresh(ctx,nullptr);
        if(visualReady&&initialRefreshes)--initialRefreshes;
    }
    int node=pendingNode.exchange(-1);
    if(shown && node>=0){
        char action[32];std::snprintf(action,sizeof(action),"Node%d",node);
        choose(ctx,action);
    }
    tickPending=false;
}
void open(const PluginContext*,void*) noexcept;
void drawTree(const PluginContext* ctx,const Overlay::Frame* frame,void*) noexcept {

    if(!treeVisible)return;
    if(!tickPending.exchange(true) &&
       threads->runOnUiThread(ctx,tickTree,nullptr)!=Threads::Result::Success)tickPending=false;
    float scale=frame->screenHeight/2160.f;
    float left=(frame->screenWidth-3620*scale)/2,top=(frame->screenHeight-2020*scale)/2;
    float sx=3620.f/1470*scale,sy=2020.f/870*scale;
    POINT mouse{};GetCursorPos(&mouse);
    HWND window=GetForegroundWindow();DWORD pid{};GetWindowThreadProcessId(window,&pid);
    bool foreground=pid==GetCurrentProcessId();ScreenToClient(window,&mouse);
    RECT client{};GetClientRect(window,&client);
    float mx=client.right?mouse.x*frame->screenWidth/client.right:0;
    float my=client.bottom?mouse.y*frame->screenHeight/client.bottom:0;
    int hover=-1;float nearest=1e9;
    for(unsigned uid=0;uid<std::size(visualNodes);uid++){
        auto v=visualNodes[uid];if(v.r<=0)continue;float x=left+v.x*sx,y=top+v.y*sy;
        float distance=(mx-x)*(mx-x)+(my-y)*(my-y);
        if(foreground && distance<v.r*v.r*sx*sy && distance<nearest){hover=uid;nearest=distance;}
    }
    static bool held=false;
    bool down=foreground && (GetAsyncKeyState(VK_LBUTTON)&0x8000);
    if(down&&!held&&hover>=0)pendingNode=hover;
    held=down;
}

void open(const PluginContext* ctx,void*) noexcept {
    PlayerHandle player{};
    if(inventory->getLocalPlayer(ctx,&player)==Inventory::Result::Success){
        const auto key=characterKey();
        if(!key.empty() && key!=editingKey){
            editingKey=key;editingPlayer=player;selected={};
            if(!loadDraft(key,selected))readBuild(ctx,player,selected);
            lastDraftValid=false;
        }
    }
    if(treeVisible){panels->closePanel(ctx,panel);treeVisible=false;return;}
    visualReady=false;
    if(panels->openPanel(ctx,panel)==Panels::Result::Success){
        treeVisible=true;refresh(ctx,nullptr);
        // openPanel can return before the live children replace a cached root.
        // Refresh the first four UI frames as well, then stop polling stats.
        visualReady=false;initialRefreshes=4;
    }
}
struct Request { Build build; PlayerHandle player; };
std::mutex requestMutex;
Request request{};
bool queued=false;
Panels::RegistrationHandle successPanel{}, failurePanel{};
void showSuccess(const PluginContext* c,void*) noexcept {treeVisible=false;panels->closePanel(c,panel);panels->openPanel(c,successPanel);}
void showFailure(const PluginContext* c,void*) noexcept {panels->openPanel(c,failurePanel);}
void apply(const PluginContext* ctx,void*) noexcept {
    Request job;
    { std::lock_guard lock(requestMutex);job=request;queued=false; }
    PlayerHandle player{};
    if(inventory->getLocalPlayer(ctx,&player)!=Inventory::Result::Success ||(job.player&&player!=job.player)){ctx->LogWarn("No matching game-thread player");return;}
    Receipt receipt{};
    InventoryState bag{};
    if(!scan(ctx,player,bag)||bag.count>1){threads->runOnUiThread(ctx,showFailure,nullptr);return;}
    if(!valid(job.build) || points(job.build)>budget || keystones(job.build)>keystoneSlots(budget)){ctx->LogWarn("Invalid selected allocation");threads->runOnUiThread(ctx,showFailure,nullptr);return;}
    std::vector<Items::PropertySpec> effects;
    auto add=[&](unsigned prop,int n,int param=0){
        auto it=std::find_if(effects.begin(),effects.end(),[&](auto& e){return e.propertyId==prop && e.parameter==param;});
        if(it==effects.end())effects.push_back(Items::MakeExactProperty(prop,n,param));
        else {it->minimum+=n;it->maximum+=n;}
    };
    for(unsigned p=0;p<job.build.size();p++){
        unsigned mask=job.build[p];
        if(mask&4095u)add(firstMaskProperty+p,mask&4095u);
        if(p==6 && mask>>12)add(petMaskProperty,mask>>12);
        for(unsigned i=0;i<(p==6?16u:9u);i++){
            if((i>=9&&i<12)||!(mask&(1u<<i)))continue;
            for(unsigned attribute:attributes)add(attribute,3);
            unsigned n=number(p,i);
            for(unsigned rank=0;rank<job.build.depth[n];rank++){
                const auto& specialized=innerRanks[n][rank];
                if(specialized.count){
                    for(unsigned j=0;j<specialized.count;j++){
                        const auto& term=specialized.effects[j];
                        if(term.propertyId==strikeProperty||term.propertyId==killProperty||term.propertyId==attackProperty){
                            // Proc properties use minimum=chance and maximum=skill
                            // level. Identical procs add chance, capped at 100%.
                            auto found=std::find_if(effects.begin(),effects.end(),[&](const auto& e){
                                return e.propertyId==term.propertyId&&e.parameter==term.parameter&&e.maximum==term.maximum;
                            });
                            if(found==effects.end())effects.push_back(term);
                            else found->minimum=std::min(100,found->minimum+term.minimum);
                        }else add(term.propertyId,term.minimum,term.parameter);
                    }
                    continue;
                }
                auto& modifier=ranks[n][rank];
                if(modifier.value)add(stars[p][i].property,modifier.value);
                if(modifier.extra)add(modifier.bonus,modifier.extra);
            }
        }
        for(unsigned i=9;i<12;i++)if(mask&(1u<<i)){
            const auto& n=stars[p][i];
            if(n.property==65535){add(summonCountProperty,n.lo);}
            else effects.push_back({n.property,n.param,n.lo,n.hi});
            const auto& extra=keystoneExtras[number(p,i)];
            for(unsigned j=0;j<extra.count;j++){
                const auto& term=extra.effects[j];
                add(term.propertyId,term.minimum,term.parameter);
            }
        }
    }
    for(auto& term:effects)if(term.propertyId==lightningDamageProperty){
        auto maximum=Items::MakeExactProperty(lightningMaximumProperty,term.maximum);
        term=Items::MakeExactProperty(lightningMinimumWideProperty,term.minimum);
        effects.push_back(maximum);
        break; // All ordinary Lightning ranges were aggregated before this step.
    }
    for(auto& term:effects)if(term.propertyId==poisonDamageProperty && term.minimum==term.maximum){
        auto duration=Items::MakeExactProperty(poisonDurationProperty,term.parameter);
        term=Items::MakeExactProperty(poisonRateWideProperty,term.minimum);
        effects.push_back(duration);
        break;
    }
    bool hasOverflow=false;
    for(const auto& mapping:wideStats){
        auto slot=std::find_if(effects.begin(),effects.end(),[&](auto& e){return e.propertyId==mapping.wide;});
        if(slot==effects.end())continue;
        const int total=slot->minimum;
        if(total<=mapping.limit){
            *slot=Items::MakeExactProperty(mapping.native,total);
        }else{
            *slot=Items::MakeExactProperty(mapping.wide,total);
            hasOverflow=true;
        }
    }
    add(pointRequirement,requiredLevel(job.build));
    if(hasOverflow)add(flatUnit,1);
    if(effects.size()>Items::MaxProperties){ctx->LogError("Sigil exceeds loader property limit");threads->runOnUiThread(ctx,showFailure,nullptr);return;}
    receipt.allocation=job.build;receipt.count=static_cast<unsigned>(effects.size());
    std::copy(effects.begin(),effects.end(),receipt.effects.begin());
    const unsigned seed=fingerprint(receipt);
    if(!saveRecord(seed,receipt)){ctx->LogError("Plan receipt write failed");threads->runOnUiThread(ctx,showFailure,nullptr);ctx->WriteConsoleMessage("Unable to save the plan receipt.");return;}
    const Items::PropertySpec selector{forgeDispatchProperty,astralDispatchSelector,1,1};
    AstralPayloadScope nativePayload(effects.data(),static_cast<unsigned>(effects.size()));
    Items::ItemCreateSpec output{.structSize=Items::ItemCreateSpecSize,
        .flags=Items::ItemCreateFlagBit(Items::ItemCreateFlag::AllowDuplicateUnique),
        .code=Items::MakeItemCode("Wsig"),.quality=Items::Quality::Unique,
        .qualityRecordId=bag.count?bag.row:Items::RandomQualityRecord,
        .itemLevel=1,.seedMode=Items::SeedMode::Deterministic,.generationSeed=seed,.itemSeed=seed,.quantity=Items::DefaultValue,.durability=Items::DefaultValue,
        .stateFlags=Items::ItemStateIdentified,.propertyCount=1,.properties=&selector,
        .destination={.structSize=Items::ItemDestinationSize,.container=Items::ItemContainer::Inventory,.placement=Items::Placement::Automatic}};
    ItemHandle handle{};
    Items::Result status;
    if(!bag.count){
        status=items->createItem(ctx,player,&output,&handle);
    }else{
        Items::TransactionInput input{bag.sigil,1,Items::SocketedItemPolicy::RejectIfNotEmpty};
        Items::Transaction tx{.structSize=Items::TransactionSize,.player=player,.inputCount=1,.outputCount=1,
            .inputs=&input,.outputs=&output,.outputItems=&handle,.outputCapacity=1};
        Items::TransactionResult report{.structSize=Items::TransactionResultSize};
        status=items->executeTransaction(ctx,&tx,&report);
    }
    if(status==Items::Result::Success && !astralPayloadApplied){
        ctx->LogError("Native Sigil payload was not applied; unsupported item-creation route");
        status=Items::Result::Unsupported;
    }
    if(status!=Items::Result::Success){
        for(const auto& effect:effects){
            char detail[160];std::snprintf(detail,sizeof(detail),"Sigil modifier property=%u param=%d min=%d max=%d",effect.propertyId,effect.parameter,effect.minimum,effect.maximum);
            ctx->LogWarn(detail);
        }
    }
    if(status==Items::Result::Success){
        Items::ItemInfo minted{.structSize=Items::ItemInfoSize};
        if(items->getItemInfo(ctx,handle,&minted)==Items::Result::Success){
            if(minted.itemSeed)saveRecord(minted.itemSeed,receipt);
            if(minted.generationSeed)saveRecord(minted.generationSeed,receipt);
        }
    }
    char msg[128];if(status==Items::Result::Success)std::snprintf(msg,sizeof(msg),"Astral Sigil imprinted: %u stars.",points(job.build));
    else std::snprintf(msg,sizeof(msg),"Sigil creation failed (loader result %u). Details logged.",static_cast<unsigned>(status));
    ctx->WriteConsoleMessage(msg);ctx->LogInfo(msg);
    threads->runOnUiThread(ctx,status==Items::Result::Success?showSuccess:showFailure,nullptr);
}
bool choose(const PluginContext* ctx,const char* key){
    if(!std::strcmp(key,"Reset")){selected={};refresh(ctx,nullptr);return true;}
    if(!std::strcmp(key,"Load")){
        PlayerHandle p{};
        if(inventory->getLocalPlayer(ctx,&p)==Inventory::Result::Success && readBuild(ctx,p,selected))refresh(ctx,nullptr);
        else ctx->WriteConsoleMessage("Could not read a saved constellation from the inventory.");
        return true;
    }
    if(!std::strcmp(key,"inscribe")||!std::strcmp(key,"Inscribe")){
        ctx->LogInfo("Apply clicked");
        std::lock_guard lock(requestMutex);
        if(queued){ctx->LogWarn("Apply already queued");return false;}
        request={selected,0};queued=threads->runOnGameThread(ctx,apply,nullptr)==Threads::Result::Success;
        if(!queued){ctx->LogWarn("Could not queue plan creation");threads->runOnUiThread(ctx,showFailure,nullptr);}
        return queued;
    }
    if(!std::strncmp(key,"Node",4)){
        char* end=nullptr;auto uid=std::strtoul(key+4,&end,10);
        if(end==key+4||*end||uid>=112+std::size(extensions))return false;
        if(uid<112)return choose(ctx,(uid<108?stars[uid/12][uid%12]:stars[6][12+uid-108]).key);
        auto e=extensions[uid-112];if(number(e.region,e.node)==retiredRoot)return false;auto& depth=selected.depth[number(e.region,e.node)];
        if(!(selected[e.region]&(1u<<e.node))||e.rank>depth+1)return false;
        if(e.rank>depth && points(selected)>=budget)return false;
        depth=e.rank<=depth?e.rank-1:e.rank;
        refresh(ctx,nullptr);return true;
    }
    for(unsigned page=0;page<9;page++)for(unsigned i=0;i<(page==6?16u:12u);i++)if(!std::strcmp(key,stars[page][i].key)){
        auto& m=selected[page];unsigned bit=1u<<i;
        if(!reachable(selected,page,i))return false;
        const bool active=(m&bit)!=0;
        if(!active && points(selected)>=budget)return false;
        if(!active && i>=9&&i<12 && keystones(selected)>=keystoneSlots(budget))return false;
        
        if(active)m&=~bit;else m|=bit;
        prune(selected);
        refresh(ctx,nullptr);return true;
    }
    return false;
}
SharedEvents::UiMessageAction onMessage(const PluginContext* ctx,const SharedEvents::UiMessageEvent* event,void*) noexcept {
    if(event && event->target && event->command && !_stricmp(event->target,id)){
        choose(ctx,event->command);
        return SharedEvents::UiMessageAction::Consume;
    }
    if(event && event->target && event->command && event->text &&
       !_stricmp(event->target,"PanelManager") && !_stricmp(event->command,"ClosePanel")){
        for(const char* nav: {"Reset","Load"}){
            char key[32];std::snprintf(key,sizeof(key),"Workshop%s",nav);
            if(!std::strcmp(event->text,key)){choose(ctx,nav);return SharedEvents::UiMessageAction::Consume;}
        }
        if(!_stricmp(event->text,"WorkshopInscribe")){choose(ctx,"inscribe");return SharedEvents::UiMessageAction::Consume;}
        if(!std::strncmp(event->text,"WorkshopN",9)){
            char action[32];std::snprintf(action,sizeof(action),"Node%s",event->text+9);
            choose(ctx,action);return SharedEvents::UiMessageAction::Consume;
        }
        if(!std::strncmp(event->text,"WorkshopAction",14)){
            char action[32];std::snprintf(action,sizeof(action),"Node%s",event->text+14);
            choose(ctx,action);return SharedEvents::UiMessageAction::Consume;
        }
    }
    return SharedEvents::UiMessageAction::Continue;
}
ConsoleCommandResult command(D2R::Game::Client*,const ConsoleCommandContext* cmd,void*) noexcept {
    if(!cmd||!cmd->plugin)return ConsoleCommandResult::Failed;
    if(cmd->argsLength==0){threads->runOnUiThread(cmd->plugin,open,nullptr);return ConsoleCommandResult::Handled;}
    char key[32]{};if(cmd->argsLength>=sizeof(key))return ConsoleCommandResult::InvalidArguments;
    std::memcpy(key,cmd->args,cmd->argsLength);
    return choose(cmd->plugin,key)?ConsoleCommandResult::Handled:ConsoleCommandResult::InvalidArguments;
}
Input::ActionResult hotkey(const PluginContext* ctx,const Input::ActionEvent* e,void*) noexcept {
    {std::lock_guard lock(WorkshopStash::Organizer::mutex);if(WorkshopStash::Organizer::opened)return Input::ActionResult::Handled;}
    if(e && e->kind==Input::ActionEventKind::Pressed){threads->runOnUiThread(ctx,open,nullptr);return Input::ActionResult::Handled;}
    return Input::ActionResult::Ignored;
}
}
namespace {
#include "nightmare_skip.inc"
void __cdecl resolveStashProfile(const PluginContext*,void*) noexcept {
 __try {
  WorkshopNightmare::Native player{};
  if(WorkshopNightmare::inspect(WorkshopNightmare::PlayerId.load(),player))
   WorkshopStash::Organizer::detectedProfile=( *reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(player.client)+14)&4)?1:0;
 }__except(EXCEPTION_EXECUTE_HANDLER){}
 WorkshopStash::Organizer::profileRequestPending=false;
}

}
D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const PluginInfo* {return &info;}
D2RL_PLUGIN_EXPORT bool D2RLoaderLoadPlugin(const PluginContext* ctx) noexcept {
    const ResourceService* resources{};const SharedEventService* events{};const InputService* input{};
    if(!ctx)return false;
    wchar_t profile[MAX_PATH];
    auto length=GetEnvironmentVariableW(L"LOCALAPPDATA",profile,MAX_PATH);
    if(!length||length>=MAX_PATH)return false;
    ledger=std::filesystem::path(profile)/L"ReimaginedWorkshop"/L"PlanReceipts";
    std::error_code ec;std::filesystem::create_directories(ledger,ec);
    if(ec)return false;
    auto service=[&](auto* ptr,const char* name){
        auto result=ctx->QueryService(ptr);
        if(result!=ServiceQueryResult::Success){ctx->LogError(name);return false;}return true;
    };
    if(!service(&resources,"ResourceService")||!service(&panels,"PanelService")||!service(&widgets,"WidgetService")||
       !service(&threads,"ThreadService")||!service(&items,"ItemService")||!service(&inventory,"InventoryService")||
       !service(&overlay,"OverlayService")||!service(&events,"SharedEventService")||!service(&input,"InputService"))return false;
    char diagnostic[180];
    auto module=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll"));
    std::snprintf(diagnostic,sizeof(diagnostic),"Widget API size=%u find=%llx child=%llx visible=%llx enabled=%llx",widgets->serviceSize,
        reinterpret_cast<uintptr_t>(widgets->findPanel)-module,reinterpret_cast<uintptr_t>(widgets->findWidget)-module,
        reinterpret_cast<uintptr_t>(widgets->setWidgetVisible)-module,reinterpret_cast<uintptr_t>(widgets->setWidgetEnabled)-module);
    ctx->LogInfo(diagnostic);
    std::snprintf(diagnostic,sizeof(diagnostic),"Overlay size=%u rect=%llx text=%llx",overlay->serviceSize,
        reinterpret_cast<uintptr_t>(overlay->drawRectangle)-module,reinterpret_cast<uintptr_t>(overlay->drawText)-module);
    ctx->LogInfo(diagnostic);
    if(!HasWidgetServiceField(widgets,offsetof(WidgetService,setWidgetEnabled)+sizeof(widgets->setWidgetEnabled))){ctx->LogError("Widget state API missing");return false;}
    if(!HasItemServiceField(items,offsetof(ItemService,executeTransaction)+sizeof(items->executeTransaction)))return false;
    Resources::ResourceRegistration resource{.structSize=Resources::ResourceRegistrationSize,
        .path="data/global/ui/layouts/astral-workshop/Atlashd.json",.bytes=layout,.byteCount=sizeof(layout)-1};
    Resources::RegistrationHandle rh{};
    Panels::PanelRegistration reg{.structSize=Panels::PanelRegistrationSize,.flags=Panels::PanelFlags::CloseOnEscape,.localId="Atlas"};
    if(resources->registerResource(ctx,&resource,&rh)!=Resources::Result::Success||panels->registerPanel(ctx,&reg,&panel)!=Panels::Result::Success)return false;
    auto notice=[&](const char* name,const char* message,Panels::RegistrationHandle& handle){
        char path[160],json[1600];std::snprintf(path,sizeof(path),"data/global/ui/layouts/astral-workshop/%shd.json",name);
        std::snprintf(json,sizeof(json),R"({"type":"Panel","name":"astral-workshop/%s","fields":{"anchor":{"x":0.5,"y":0.5},"priority":8600},"children":[{"type":"ImageWidget","name":"Frame","fields":{"rect":{"x":-627,"y":-270},"filename":"Panel\\Modals\\Modal_No_Title_BG"}},{"type":"TextBoxWidget","name":"Message","fields":{"rect":{"x":-510,"y":-80,"width":1020,"height":200},"text":"%s","style":"$StyleModalDialogDescription"}}]})",name,message);
        Resources::ResourceRegistration res{.structSize=Resources::ResourceRegistrationSize,.path=path,.bytes=json,.byteCount=static_cast<unsigned>(std::strlen(json))};
        Resources::RegistrationHandle h{};
        Panels::PanelRegistration dlg{.structSize=Panels::PanelRegistrationSize,.flags=Panels::PanelFlags::CloseOnEscape,.localId=name};
        return resources->registerResource(ctx,&res,&h)==Resources::Result::Success && panels->registerPanel(ctx,&dlg,&handle)==Panels::Result::Success;
    };
    if(!notice("PlanReady","Astral Sigil ready in your inventory. Press Esc to return.",successPanel)||
       !notice("PlanFailed","Sigil creation failed. Carry at most one Sigil and leave inventory space. Modifier details are in astral-workshop.log. Press Esc to return.",failurePanel))return false;
    Overlay::CallbackRegistration drawing{.structSize=Overlay::CallbackRegistrationSize,.phase=Overlay::Phase::AfterGameUi,.callback=drawTree};
    Overlay::CallbackHandle drawingHandle{};
    if(overlay->registerFrameCallback(ctx,&drawing,&drawingHandle)!=Overlay::Result::Success)return false;
    ctx->LogInfo(HasItemServiceField(items,ItemServiceDetailsFieldEnd)?"Plan details available":"Plan details unavailable");
    SharedEvents::UiMessageListener listener{.structSize=SharedEvents::UiMessageListenerSize,.callback=onMessage};
    SharedEvents::ListenerHandle lh{};
    if(events->registerUiMessageListener(ctx,&listener,&lh)!=SharedEvents::Result::Success){ctx->LogError("UI message registration failed");return false;}
    Input::ActionRegistration action{.structSize=Input::ActionRegistrationSize,.logicalId="astral-tree",.displayName="Astral Passive Tree",.category="Reimagined Workshop",.defaultPrimary={.key=Input::Key::F9},.callback=hotkey};
    Input::ActionHandle ah{};input->registerAction(ctx,&action,&ah);
    const unsigned char expected[]={0xff,0x25,0xda,0x35,0xab,0x03};
    const unsigned char statExpected[]={0xff,0x25,0xf2,0x51,0xb3,0x03};
    auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    if(std::memcmp(exe+0x2f5020,statExpected,sizeof(statExpected)))return false;
    readItemStat=reinterpret_cast<ReadStatFn>(exe+0x2f5020);
    WorkshopStash::install(ctx,events);
    if(!WorkshopNightmare::install(ctx,events))ctx->LogError("[nightmare-skip] Unsupported native bindings or services; skip disabled.");
    if(!installForgeChronicle(ctx))ctx->LogError("Runeforged Chronicle unavailable; other Workshop features remain active");
    if(!installForgeHook(ctx))return false;
    installPropertyAudit(ctx);
    SharedEvents::ItemTooltipListener forgedLabel{.structSize=SharedEvents::ItemTooltipListenerSize,
        .region=SharedEvents::ItemTooltipRegion::Attributes,.position=SharedEvents::ItemTooltipPosition::Top,
        .callback=forgeTooltip};
    SharedEvents::ListenerHandle forgedLabelHandle{};
    if(events->registerItemTooltipListener(ctx,&forgedLabel,&forgedLabelHandle)!=SharedEvents::Result::Success){
        ctx->LogError("Runeforged tooltip registration failed");return false;
    }
    auto sealLabel=forgedLabel;sealLabel.callback=trialSealTooltip;
    SharedEvents::ListenerHandle sealHandle{};
    if(events->registerItemTooltipListener(ctx,&sealLabel,&sealHandle)!=SharedEvents::Result::Success)return false;
    ctx->LogInfo("Runeforged and Trial seal saved tooltip labels enabled");
    auto hook=MakeInlineHook(0x376de0,expected,sizeof(expected),
        reinterpret_cast<void*>(sigilRequiredLevel),reinterpret_cast<void**>(&originalRequiredLevel));
    if(!ctx->api->installInlineHook(ctx,&hook))return false;
    if(!LeagueCraft::Install(ctx))return false;
    return ctx->RegisterConsoleCommand("astral",command,"Open tree, toggle a node by name, or inscribe the plan.");
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {WorkshopStash::Organizer::shutdown();}

