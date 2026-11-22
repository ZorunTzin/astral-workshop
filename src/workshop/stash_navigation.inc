// Native page fingerprints and descriptor lookup adapted from d2r-access (MIT).
// See d2rloader/licenses/d2r-access-stash-MIT.txt. No stash save data is edited.
namespace WorkshopStash {
using PageFn=uint64_t(*)(uintptr_t,bool,bool);
using CountFn=uint64_t(*)(uintptr_t);
using SeasonFn=uint8_t(*)(uintptr_t);
using FindFn=void*(*)(void*,const char*);
PageFn page{}; CountFn count{}; SeasonFn season{}; FindFn find{},child{};
struct TextView {const char* data;size_t size;};
using SetTextFn=void(*)(void*,const TextView*);
SetTextFn setText{};
void** manager{};
bool ready=false;
uint64_t hash(const unsigned char* bytes,size_t size) {
    uint64_t h=14695981039346656037ULL;
    for(size_t i=0;i<size;++i){
        unsigned char b=bytes[i]; h=(h^b)*1099511628211ULL;
        if(b==0xe8||b==0xe9){size_t n=std::min(size_t(4),size-i-1);
            for(size_t j=0;j<n;++j)h*=1099511628211ULL;i+=n;}
    }return h;
}
uintptr_t relative(uintptr_t p,int length,int operand){return p+length+*reinterpret_cast<int32_t*>(p+operand);}
uintptr_t indirect(uintptr_t p){return *reinterpret_cast<uint16_t*>(p)==0x15ff?*reinterpret_cast<uintptr_t*>(relative(p,6,2)):0;}
bool resolve(const PluginContext* ctx) noexcept {
    __try {
        const auto exe=ctx->exeBase;
        if(hash(reinterpret_cast<unsigned char*>(exe+0x23bcf0),1505)!=0x0606B958AEEBD4E8ULL)return false;
        const auto call=exe+0x23c2a8;
        if(*reinterpret_cast<unsigned char*>(call)!=0xe8)return false;
        auto target=relative(call,5,1);
        if(*reinterpret_cast<uint16_t*>(target)==0x25ff)target=*reinterpret_cast<uintptr_t*>(relative(target,6,2));
        if(hash(reinterpret_cast<unsigned char*>(target),317)!=0x11116ABEB33A3AAAULL)return false;
        auto c=indirect(target+0x2b),s=indirect(target+0x3d);
        if(!c||!s)return false;
        const auto core=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll"));
        if(!core)return false;
        auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(core);
        auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(core+dos->e_lfanew);
        auto section=IMAGE_FIRST_SECTION(nt);
        uintptr_t values[4]{};unsigned hits[4]{};
        const uint64_t anchors[4]={0x3440170,0x89f760,0x855c90,0x86dd50};
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i,++section){
            if((section->Characteristics&(IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE|IMAGE_SCN_MEM_EXECUTE))!=(IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE))continue;
            auto begin=core+section->VirtualAddress,end=begin+section->Misc.VirtualSize;
            for(auto at=begin+8;at+8<=end;at+=8)for(unsigned k=0;k<4;++k)
                if(*reinterpret_cast<uint64_t*>(at)==anchors[k]){++hits[k];values[k]=*reinterpret_cast<uintptr_t*>(at-8);}
        }
        if(hits[0]!=1||hits[1]!=1||hits[2]!=1||!values[0]||!values[1]||!values[2])return false;
        if(hits[3]!=1||!values[3])return false;
        setText=reinterpret_cast<SetTextFn>(values[3]);
        child=reinterpret_cast<FindFn>(values[2]);
        manager=reinterpret_cast<void**>(values[0]);find=reinterpret_cast<FindFn>(values[1]);
        page=reinterpret_cast<PageFn>(target);count=reinterpret_cast<CountFn>(c);season=reinterpret_cast<SeasonFn>(s);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// Called only on the UI thread, with the current panel resolved on every use.
int move(int delta) noexcept {
    __try {
        auto panel=reinterpret_cast<uintptr_t>(find(*manager,"BankExpansionLayout"));
        if(!panel)return -1;
        const auto total=count(panel);const auto active=season(panel);
        if(active>=2||total==0||total>1000)return -1;
        auto current=reinterpret_cast<uint64_t*>(panel+0x168+active*8);
        const int steps=stashSteps(*current,total,delta);
        for(int i=0;i<steps;++i){
            auto expected=delta<0?*current-1:*current+1;
            page(panel,delta>0,true); // singleStep bypasses loader Ctrl/Shift multipliers
            if(*current!=expected)return -1;
        }return steps;
    } __except(EXCEPTION_EXECUTE_HANDLER){return -1;}
}
bool visible(const PluginContext*) noexcept {
    __try {
        auto node=reinterpret_cast<uintptr_t>(find(*manager,"BankExpansionLayout"));
        if(!node||*reinterpret_cast<unsigned char*>(node+0x51)!=1)return false;
        const char* chain[]={"basicstash_container","SharedStashTabContainer","StashNavigation"};
        for(int i=0;i<3;++i){
            node=reinterpret_cast<uintptr_t>(child(reinterpret_cast<void*>(node),chain[i]));
            if(!node||*reinterpret_cast<unsigned char*>(node+0x51)!=1)return false;
        }return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
#include "leagues/stash.inc"
#include "stash_organizer.inc"
#include "item_sort.inc"
void navigate(const PluginContext* ctx,void* data) noexcept {
    if(!ready)return;
    if(!visible(ctx)){ctx->LogInfo("[stash] Ignored navigation: stash is not visible.");return;}
    const int delta=static_cast<int>(reinterpret_cast<intptr_t>(data));
    int moved=move(delta);
    char line[100];std::snprintf(line,sizeof(line),"[stash] requested=%d moved=%d",delta,moved);ctx->LogInfo(line);
    if(moved<0){ready=false;ctx->LogError("[stash] Navigation stopped: unexpected native page state.");}
}
SharedEvents::UiMessageAction message(const PluginContext* ctx,const SharedEvents::UiMessageEvent* e,void*) noexcept {
    if(!e||!e->target||!e->command||!e->text||
       _stricmp(e->target,"PanelManager")||_stricmp(e->command,"ClosePanel")||
       std::strncmp(e->text,"WorkshopStash",13))return SharedEvents::UiMessageAction::Continue;
    const char* command=e->text+13;
    if(!std::strcmp(command,"League")||!std::strcmp(command,"LeagueBack")){threads->runOnUiThread(ctx,LeagueStash::select,reinterpret_cast<void*>(uintptr_t(!std::strcmp(command,"LeagueBack"))));return SharedEvents::UiMessageAction::Consume;}
    if(!std::strncmp(command,"UI",2)&&Organizer::command(ctx,command+2))return SharedEvents::UiMessageAction::Consume;
    if(!std::strcmp(command,"Organizer")){
        ctx->LogInfo("[stash] Organizer button clicked.");
        threads->runOnUiThread(ctx,Organizer::open,nullptr);
        return SharedEvents::UiMessageAction::Consume;
    }
    if(!std::strncmp(command,"Sort",4)){
        auto container=!std::strcmp(command,"SortInventory")?Items::ItemContainer::Inventory:!std::strcmp(command,"SortCube")?Items::ItemContainer::Cube:Items::ItemContainer::SharedStash;
        threads->runOnUiThread(ctx,Sorting::start,reinterpret_cast<void*>(uintptr_t(container)));
        return SharedEvents::UiMessageAction::Consume;
    }
    int direction=0;bool fast=false;
    if(!std::strcmp(command,"Previous"))direction=-1;
    else if(!std::strcmp(command,"Next"))direction=1;
    else if(!std::strcmp(command,"PreviousTen")){direction=-1;fast=true;}
    else if(!std::strcmp(command,"NextTen")){direction=1;fast=true;}
    if(!direction)return SharedEvents::UiMessageAction::Continue;
    const int delta=direction*stashStride(fast,(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0);
    threads->runOnUiThread(ctx,navigate,reinterpret_cast<void*>(static_cast<intptr_t>(delta)));
    return SharedEvents::UiMessageAction::Consume;
}
void install(const PluginContext* ctx,const SharedEventService* events){
    ready=resolve(ctx);
    if(!ready){ctx->LogError("[stash] Unsupported native page layout; Workshop navigation disabled.");return;}
    SharedEvents::UiMessageListener listener{.structSize=SharedEvents::UiMessageListenerSize,.callback=message};
    SharedEvents::ListenerHandle handle{};
    ready=events->registerUiMessageListener(ctx,&listener,&handle)==SharedEvents::Result::Success;
    if(ready)Organizer::install(ctx);
    ctx->LogInfo(ready?"[stash] Navigation ready: 1/5 and 10/100 pages.":"[stash] UI message registration failed.");
}
}
