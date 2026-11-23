#include <map>
#include "passives.cpp"
#include <cassert>
namespace {
bool hasSigil=false;
unsigned level=83, minted=0, forged=0, planSeed=0;
std::vector<Items::PropertySpec> planEffects;
std::vector<Items::PropertySpec> dispatched;
Inventory::Result local(const PluginContext*,PlayerHandle* p) noexcept {*p=1;return Inventory::Result::Success;}
Inventory::Result list(const PluginContext* c,PlayerHandle,const Inventory::ItemFilter* filter,Inventory::ItemCallback cb,void* u) noexcept {
    Items::ItemInfo item{};
    if(filter->containerMask==Items::ContainerBit(Items::ItemContainer::Inventory)){
        if(hasSigil){item.code=Items::MakeItemCode("Wsig");item.quality=Items::Quality::Unique;item.qualityRecordId=sigilRow;item.handle=3;cb(c,&item,u);}return Inventory::Result::Success;
    }
    item.code=Items::MakeItemCode("Wpln");item.handle=5;cb(c,&item,u);
    item.code=Items::MakeItemCode("Wsig");item.handle=3;item.quality=Items::Quality::Unique;item.qualityRecordId=sigilRow;cb(c,&item,u);
    return Inventory::Result::Success;
}
Items::Result inspectItem(const PluginContext*,ItemHandle,Items::ItemInfo* info) noexcept {
    info->itemSeed=planSeed;info->runtimeId=40;return Items::Result::Success;
}
Items::Result create(const PluginContext*,PlayerHandle,const Items::ItemCreateSpec* out,ItemHandle* handle) noexcept {
    assert(out->code==Items::MakeItemCode("Wsig") && out->quality==Items::Quality::Unique);
    assert(out->propertyCount==1 && out->properties[0].propertyId==forgeDispatchProperty);
    for(unsigned i=0;i<out->propertyCount;i++)assert(out->properties[i].minimum<=out->properties[i].maximum);
    unsigned nativeItem[]={4,Items::MakeItemCode("Wsig"),0,0};
    dispatched.clear();
    applyForgeProperty(true,7,nullptr,nativeItem,const_cast<Items::PropertySpec*>(out->properties),0,0,0,out->properties,0,0x40,nullptr);
    assert(astralPayloadApplied);planEffects=dispatched;
    planSeed=out->itemSeed;*handle=5;++minted;return Items::Result::Success;
}
Items::Result commit(const PluginContext* c,const Items::Transaction* t,Items::TransactionResult*) noexcept {
    assert(t->inputCount==1 && t->inputs[0].item==3 && t->outputCount==1 && t->outputs->qualityRecordId==sigilRow);
    ++forged;return create(c,t->player,t->outputs,t->outputItems);
}
}

Threads::Callback pendingWork=nullptr;
Threads::Result enqueueGame(const PluginContext*,Threads::Callback fn,void*) noexcept {pendingWork=fn;return Threads::Result::Success;}
Threads::Result enqueueUi(const PluginContext*,Threads::Callback fn,void*) noexcept {assert(fn==showSuccess);return Threads::Result::Success;}
Items::Result forgeDetails(const PluginContext*,ItemHandle,Items::ItemDetails* d,Items::ItemStatValue* out,unsigned capacity) noexcept {
    assert(capacity>=2);unsigned lore=0;while(std::strcmp(forgeWords[lore].name,"Lore"))++lore;
    d->statCount=2;out[0]={forgeMarkerStat,0,2+int(lore>>7),0x40};out[1]={forgeLevelStat,0,int(lore&127),0x40};return Items::Result::Success;
}
Widgets::Result noPanel(const PluginContext*,const char*,Widgets::WidgetHandle*) noexcept {return Widgets::Result::NotFound;}
int nativeRequirement(void*,void*){return 67;}
int simulatedRequirement=32,simulatedOrbRequirement=0;
int savedRequirement(void*,int stat,unsigned){if(stat==forgeLevelStat||stat==forgeMarkerStat)return 0;if(stat==LeagueCraft::LevelStat)return simulatedOrbRequirement;assert(stat==pointRequirementStat);return simulatedRequirement;}
namespace {
void __fastcall captureForge(bool, int source, void*, void* item, void*, int, int, int,
        const Items::PropertySpec* property, int, unsigned, void*) {
    assert((source==7||source==0) && item && property);
    dispatched.push_back(*property);
}
alignas(8) unsigned char affixTableMock[0x1700]{};
alignas(8) unsigned char affixRecordsMock[3*0x8c]{};
unsigned char* __fastcall fakeAffixTables(bool){return affixTableMock;}
int __fastcall fakeAffixQuality(void*){return 4;}
unsigned __fastcall fakeAffixCode(void* p){return static_cast<unsigned*>(p)[1];}
void testAffixStrength() {
    auto test=[](Items::PropertySpec input,unsigned quality,int param,int lo,int hi){
        auto result=strengthenAffix(input,quality,80);
        if(result.parameter!=param||result.minimum!=lo||result.maximum!=hi){std::fprintf(stderr,"Mismatch prop %u: actual %d/%d/%d expected %d/%d/%d\n",input.propertyId,result.parameter,result.minimum,result.maximum,param,lo,hi);}
        assert(result.propertyId==input.propertyId&&result.parameter==param&&result.minimum==lo&&result.maximum==hi);
    };
    test({13,0,30,40},4,0,60,80); // Life, Magic x2.
    test({13,0,30,40},6,0,40,54); // Rare x1.35, floored.
    test({104,0,1,3},6,0,1,4);  // +Skills floor.
    test({124,16,1,2},4,16,2,4); // Skill-tab ID stays fixed.
    test({261,2,66,70},4,4,66,70); // Random-skill ID range stays fixed.
    test({145,16,0,0},6,21,0,0); // Per-level numeric parameter.
    test({115,0,1,1},4,0,1,1); // Cannot Be Frozen.
    test({127,53,30,20},4,53,60,40); // Proc chance AND level.
    test({127,53,75,48},4,53,100,96); // Chance capped, level not wrapped.
    test({243,53,20,10},4,53,40,20); // Charges and level.
    test({243,53,-60,-10},4,53,224,14); // Resolve dynamic charges first.
    test({90,0,128,128},4,0,255,255); // Native flee scale cap.
    test({275,0,50,50},4,0,100,100); // Mosaic is a numeric chance.
    test({117,5,3,5},4,5,6,10); // Reanimate monster ID unchanged.
    test({133,4,0,0},4,8,0,0); // Socket amount lives in parameter.
    test({75,0,-15,-15},6,0,-21,-21); // Requirement reduction.
    for(unsigned quality:{1,2,3,5,7,8})test({13,0,30,40},quality,0,30,40);
    for(const auto& rule:affixRules)if(!rule.kind) {
        Items::PropertySpec before{rule.property,53,1,1};
        auto after=strengthenAffix(before,4,80);
        assert(std::memcmp(&before,&after,sizeof(before))==0);
    }
    *reinterpret_cast<uintptr_t*>(affixTableMock+0x15e8)=reinterpret_cast<uintptr_t>(affixRecordsMock);
    *reinterpret_cast<uintptr_t*>(affixTableMock+0x1610)=reinterpret_cast<uintptr_t>(affixRecordsMock+2*0x8c);
    affixTables=fakeAffixTables;affixQuality=fakeAffixQuality;affixCode=fakeAffixCode;
    auto property=reinterpret_cast<Items::PropertySpec*>(affixRecordsMock+0x24);
    *property={13,0,30,40};
    assert(ordinaryAffixRecord(true,affixRecordsMock,0,property));
    assert(!ordinaryAffixRecord(true,affixRecordsMock+1,0,property));
    auto automatic=reinterpret_cast<Items::PropertySpec*>(affixRecordsMock+2*0x8c+0x24);
    *automatic={13,0,30,40};
    assert(!ordinaryAffixRecord(true,affixRecordsMock+2*0x8c,0,automatic));
    alignas(8) unsigned char unit[64]{},data[32]{};
    *reinterpret_cast<unsigned*>(unit)=4;
    *reinterpret_cast<void**>(unit+0x10)=data;
    *reinterpret_cast<int*>(data+0x10)=80;
    originalApplyProperty=captureForge;dispatched.clear();
    applyForgeProperty(true,0,nullptr,unit,affixRecordsMock,0,0,0,property,0,0x40,nullptr);
    assert(dispatched.size()==1&&dispatched[0].minimum==60&&dispatched[0].maximum==80);
    assert(property->minimum==30&&property->maximum==40); // Shared table unmodified.
    for(unsigned base:unscaledAffixCodes){
        reinterpret_cast<unsigned*>(unit)[1]=base;dispatched.clear();
        applyForgeProperty(true,0,nullptr,unit,affixRecordsMock,0,0,0,property,0,0x40,nullptr);
        assert(dispatched.size()==1&&dispatched[0].minimum==30&&dispatched[0].maximum==40);
    }
    reinterpret_cast<unsigned*>(unit)[1]=0;

    dispatched.clear();
    applyForgeProperty(true,0,nullptr,unit,affixRecordsMock+2*0x8c,0,0,0,automatic,0,0x40,nullptr);
    assert(dispatched.size()==1&&dispatched[0].minimum==30); // AutoMagic unmodified.
    std::puts("PASS: quality factors, skills and IDs, fixed traits, both proc fields, dynamic charges, random-affix boundaries, no shared-table mutation.");
}
void testForgeDispatch() {
    originalApplyProperty=captureForge;
    unsigned nativeItem[4]={4};
    for(unsigned i=0;i<std::size(forgeSpans);++i) {
        dispatched.clear();
        Items::PropertySpec selector{forgeDispatchProperty,static_cast<int>(i+1),1,1};
        applyForgeProperty(true,7,nullptr,nativeItem,&selector,0,0,0,&selector,0,0x40,nullptr);
        const auto& span=forgeSpans[i];
        assert(dispatched.size()==span.count);
        for(unsigned j=0;j<span.count;++j)
            assert(std::memcmp(&dispatched[j],&forgeEffects[span.start+j],sizeof(Items::PropertySpec))==0);
    }
    dispatched.clear();
    Items::PropertySpec invalid{forgeDispatchProperty,0,1,1};
    applyForgeProperty(true,7,nullptr,nativeItem,&invalid,0,0,0,&invalid,0,0x40,nullptr);
    assert(dispatched.empty());
    // The private selector cannot change another item, run without its
    // synchronous Sigil scope, or replay a previously consumed payload.
    affixCode=fakeAffixCode;
    Items::PropertySpec privateSelector{forgeDispatchProperty,astralDispatchSelector,1,1};
    dispatched.clear();applyForgeProperty(true,7,nullptr,nativeItem,&privateSelector,0,0,0,&privateSelector,0,0x40,nullptr);
    assert(dispatched.empty());
    Items::PropertySpec nativeEffects[]={{strikeProperty,82,25,10},{attributes[0],0,3,3}};
    {
        AstralPayloadScope payload(nativeEffects,2);
        nativeItem[1]=Items::MakeItemCode("rin");
        applyForgeProperty(true,7,nullptr,nativeItem,&privateSelector,0,0,0,&privateSelector,0,0x40,nullptr);
        assert(dispatched.empty()&&!astralPayloadApplied);
        nativeItem[1]=Items::MakeItemCode("Wsig");
        applyForgeProperty(true,7,nullptr,nativeItem,&privateSelector,0,0,0,&privateSelector,0,0x40,nullptr);
        assert(astralPayloadApplied&&dispatched.size()==2&&dispatched[0].minimum==25&&dispatched[0].maximum==10);
        dispatched.clear();applyForgeProperty(true,7,nullptr,nativeItem,&privateSelector,0,0,0,&privateSelector,0,0x40,nullptr);
        assert(dispatched.empty());
    }
    assert(!astralPayload&&astralPayloadCount==0);
    std::puts("PASS: private Sigil selector rejects unscoped calls, other item codes and replay, preserving descending proc fields.");
    Items::PropertySpec ordinary{1,0,15,30};
    applyForgeProperty(true,7,nullptr,nativeItem,&ordinary,0,0,0,&ordinary,0,0x40,nullptr);
    assert(dispatched.size()==1 && dispatched[0].minimum==15 && dispatched[0].maximum==30);
    std::printf("PASS: %zu native forge bundles, full random ranges, forwarding and invalid-selector rejection.\n",std::size(forgeSpans));
}
}
unsigned routeMask(unsigned p,unsigned node){
    unsigned result=0;
    for(unsigned guard=0;guard<6;guard++){
        result|=1u<<node;
        unsigned parent=parentNodes[number(p,node)];
        if(parent==65535)return result;
        assert(parent<16);node=parent;
    }
    assert(false);return 0;
}
int main(){
    { // Only the stack's own unit may receive the extra crafting description.
      using namespace WorkshopStash;
      static unsigned char bank[0x80]{},slot[0x620]{};
      static unsigned unit[]={4,123,40};
      static void* owner=bank;
      auto oldFind=find,oldChild=child;auto oldManager=manager;
      manager=&owner;
      find=+[](void*,const char*)->void*{return bank;};
      child=+[](void*,const char* name)->void*{
        return (!std::strcmp(name,"advancedstash_materials")||!std::strcmp(name,"LeagueOrb02"))?slot:nullptr;
      };
      *reinterpret_cast<unsigned**>(slot+0x608)=unit;bank[0x51]=1;
      LeagueStash::active=true;
      Items::ItemInfo info{.classId=123,.runtimeId=40};
      assert(LeagueStash::tooltipIndex(info)==2);
      info.runtimeId=41;assert(LeagueStash::tooltipIndex(info)==-1);
      info.runtimeId=40;info.classId=124;assert(LeagueStash::tooltipIndex(info)==-1);
      info.classId=123;bank[0x51]=0;assert(LeagueStash::tooltipIndex(info)==-1);
      bank[0x51]=1;LeagueStash::active=false;assert(LeagueStash::tooltipIndex(info)==-1);
      for(auto text:LeagueStash::OrbStashDescriptions){
        assert(std::strstr(text,"Weapons:")&&std::strstr(text,"Armor / Shields:")&&std::strstr(text,"Rings / Amulets:"));
        assert(!std::strstr(text,"%%"));
      }
      find=oldFind;child=oldChild;manager=oldManager;
    }

    testForgeDispatch();
    for(unsigned i=0;i<std::size(forgeWords);++i){
        const int marker=2+(i>>7),value=i&127;
        assert(forgeWord(marker,value)==&forgeWords[i]);
        assert(forgeRequiredLevel(marker,value)==forgeWords[i].required);
    }
    assert(!forgeWord(0,0)&&!forgeWord(1,50)&&!forgeWord(3,127));
    assert(forgeRequiredLevel(1,55)==55&&forgeRequiredLevel(0,80)==0);
    testAffixStrength();
    unsigned itemMemory[4]={4};auto& itemType=itemMemory[0];readItemStat=savedRequirement;originalRequiredLevel=nativeRequirement;
    assert(sigilRequiredLevel(&itemType,nullptr)==32);
    simulatedRequirement=3;assert(sigilRequiredLevel(&itemType,nullptr)==3);
    simulatedRequirement=0;assert(sigilRequiredLevel(&itemType,nullptr)==67);
    simulatedOrbRequirement=10;assert(sigilRequiredLevel(&itemType,nullptr)==77);simulatedOrbRequirement=0;
    simulatedRequirement=32;itemType=0;assert(sigilRequiredLevel(&itemType,nullptr)==67);

    readLevel=[](){return budget;};
    budget=99;
    ledger=std::filesystem::current_path()/"test-receipts";std::filesystem::create_directories(ledger);
    // A draft survives a fresh editor instance, including unsent ranks/reset.
    {editingKey="test-character-A";selected={};selected[0]=1;selected.depth[0]=2;
    persistDraft(nullptr);Build restored{};assert(loadDraft(editingKey,restored));
    assert(restored[0]==1&&restored.depth[0]==2);
    Build other{};assert(!loadDraft("test-character-B",other));
    selected={};persistDraft(nullptr);restored[0]=1;assert(loadDraft(editingKey,restored)&&points(restored)==0);
    editingKey.clear();lastDraftValid=false;}
    InventoryService inv{};inv.getLocalPlayer=local;inv.forEachInventoryItem=list;inventory=&inv;
    ItemService it{};it.createItem=create;it.getItemInfo=inspectItem;it.executeTransaction=commit;items=&it;
    {char text[160]{};SharedEvents::ItemTooltipEvent event{.text=text,.capacity=sizeof(text)};
     unsigned stealth=0;while(std::strcmp(forgeWords[stealth].name,"Stealth"))++stealth;
     forgeTooltipSnapshot={40,2+int(stealth>>7),int(stealth&127),GetTickCount64()};
     forgeTooltip(nullptr,&event,nullptr);assert(std::string(text)=="Runeforged (Stealth)");
     event.length=0;text[0]=0;forgeTooltip(nullptr,&event,nullptr);assert(!event.length);
     forgeTooltipSnapshot={41,2,0,GetTickCount64()};forgeTooltip(nullptr,&event,nullptr);assert(!event.length);
     forgeTooltipSnapshot={40,1,55,GetTickCount64()};forgeTooltip(nullptr,&event,nullptr);assert(std::string(text)=="Runeforged");
    }
    {char text[160]{};SharedEvents::ItemTooltipEvent event{.text=text,.capacity=sizeof(text)};
     it.serviceVersion=1;it.serviceSize=ItemServiceSize;it.getItemDetails=forgeDetails;
     forgeTooltipSnapshot={};forgeTooltip(nullptr,&event,nullptr);assert(std::string(text)=="Runeforged (Lore)");
     it.serviceSize=0;it.getItemDetails=nullptr;
    }
    PluginContext ctx{}; ThreadService svc{};svc.runOnGameThread=enqueueGame;svc.runOnUiThread=enqueueUi;threads=&svc;
    WidgetService widgetMock{};widgetMock.findPanel=noPanel;widgets=&widgetMock;
    Receipt legacy{};legacy.allocation[6]=(15u<<12)|511u;
    legacy.allocation.depth[retiredRoot]=4;
    unsigned oldPoints=points(legacy.allocation);
    unsigned oldSeed=fingerprint(legacy);assert(saveRecord(oldSeed,legacy));planSeed=oldSeed;
    Build migrated{};assert(readItem(&ctx,5,migrated));
    assert(points(migrated)==oldPoints-5 && valid(migrated));
    assert(!(migrated[6]&(1u<<12)) && migrated.depth[retiredRoot]==0);
    selected=migrated;assert(!choose(&ctx,"Node108"));
    for(unsigned uid=112;uid<112+std::size(extensions);uid++){
        auto e=extensions[uid-112];
        if(number(e.region,e.node)==retiredRoot){
            std::string key="Node"+std::to_string(uid);assert(!choose(&ctx,key.c_str()));
        }
    }
    std::puts("PASS: retired Broodheart refunds five allocated points, old Sigils remain readable, retired root/ranks reject allocation.");
    Receipt oldFork{};oldFork.allocation[0]=3u|(1u<<9);oldFork.allocation.depth[1]=4;
    unsigned forkSeed=fingerprint(oldFork);assert(saveRecord(forkSeed,oldFork));planSeed=forkSeed;
    Build straight{};assert(readItem(&ctx,5,straight)&&straight[0]==1u&&points(straight)==1);
    assert(straight.depth[1]==0); // Missing new Side prerequisite returns Outer + Keystone.
    std::puts("PASS: older fork allocations are pruned safely and every unreachable rank/Keystone point is returned.");
    budget=2;selected={};
    assert(choose(&ctx,"Node0") && points(selected)==1);
    assert(choose(&ctx,"Node112") && points(selected)==2);
    assert(!choose(&ctx,"Node113") && points(selected)==2);
    assert(choose(&ctx,"Node112") && points(selected)==1);
    budget=29;selected={};
    assert(choose(&ctx,"Node0")&&choose(&ctx,"Node2")&&choose(&ctx,"Node1"));
    assert(!choose(&ctx,"Node9")&&points(selected)==3);
    budget=30;assert(choose(&ctx,"Node9")&&points(selected)==4&&requiredLevel(selected)==30);
    assert(choose(&ctx,"Node3")&&choose(&ctx,"Node5")&&choose(&ctx,"Node4"));
    assert(!choose(&ctx,"Node10"));
    budget=60;assert(choose(&ctx,"Node10")&&keystones(selected)==2);
    assert(choose(&ctx,"Node6")&&choose(&ctx,"Node8")&&choose(&ctx,"Node7"));
    assert(!choose(&ctx,"Node11"));
    budget=90;assert(choose(&ctx,"Node11")&&keystones(selected)==3&&requiredLevel(selected)==90);
    assert(choose(&ctx,"Node12")&&choose(&ctx,"Node14")&&choose(&ctx,"Node13"));
    assert(!choose(&ctx,"Node21"));
    assert(choose(&ctx,"Node9")&&keystones(selected)==2);
    assert(choose(&ctx,"Node21")&&keystones(selected)==3);
    budget=99;
    auto clickApply=[&](Build build){
        selected=build;
        SharedEvents::UiMessageEvent click{.structSize=SharedEvents::UiMessageEventSize,
            .target="PanelManager",.command="ClosePanel",.text="WorkshopInscribe"};
        assert(onMessage(&ctx,&click,nullptr)==SharedEvents::UiMessageAction::Consume && pendingWork);
        pendingWork(&ctx,nullptr);pendingWork=nullptr;
    };
    for(unsigned e=0;e<std::size(extensions);e++){
        const auto step=extensions[e];
        unsigned p=step.region,i=step.node,n=number(p,i);
        if(n==retiredRoot)continue;
        Build path{};path[p]=routeMask(p,i);
        path.depth[n]=step.rank;
        assert(valid(path));
        unsigned previous=minted;
        selected=path;selected.depth[n]=step.rank-1;
        std::string action="Node"+std::to_string(112+e);
        std::string route="WorkshopN"+std::to_string(112+e);
        SharedEvents::UiMessageEvent event{.structSize=SharedEvents::UiMessageEventSize,.target="PanelManager",.command="ClosePanel",.text=route.c_str()};
        assert(onMessage(&ctx,&event,nullptr)==SharedEvents::UiMessageAction::Consume && selected.depth[n]==step.rank);
        clickApply(selected);
        assert(minted==previous+1);
        Build restored{};assert(readItem(&ctx,5,restored) && restored==path && restored.depth==path.depth);
        // Check actual modifier rolls, including parameterized skill tabs,
        // chance/level proc fields, and wider saved attribute representations.
        std::map<std::pair<unsigned,int>,std::pair<int,int>> expected;
        auto expect=[&](unsigned prop,int param,int lo,int hi){
            if(prop==lightningDamageProperty){
                auto& minimum=expected[{lightningMinimumWideProperty,0}];minimum.first+=lo;minimum.second+=lo;
                auto& maximum=expected[{lightningMaximumProperty,0}];maximum.first+=hi;maximum.second+=hi;
                return;
            }
            if(prop==poisonDamageProperty){
                auto& rate=expected[{poisonRateWideProperty,0}];rate.first+=lo;rate.second+=lo;
                auto& duration=expected[{poisonDurationProperty,0}];duration.first=param;duration.second=param;
                return;
            }
            auto& amount=expected[{prop,param}];amount.first+=lo;
            if(prop==strikeProperty||prop==killProperty||prop==attackProperty)amount.second=hi;
            else amount.second+=hi;
        };
        for(auto prop:attributes)expect(prop,0,3*std::popcount(path[p]),3*std::popcount(path[p]));
        for(unsigned r=0;r<step.rank;r++){
            const auto& specialized=innerRanks[n][r];
            if(specialized.count){
                for(unsigned j=0;j<specialized.count;j++){
                    auto v=specialized.effects[j];expect(v.propertyId,v.parameter,v.minimum,v.maximum);
                }
            }else{
                auto entry=ranks[n][r];
                if(entry.value)expect(stars[p][i].property,0,entry.value,entry.value);
                if(entry.extra)expect(entry.bonus,0,entry.extra,entry.extra);
            }
        }
        for(auto [key,value]:expected){
            unsigned id=key.first;auto [lo,hi]=value;
            for(auto w:wideStats)if(w.wide==id){
                if(lo<=w.limit)id=w.native;else{}
                break;
            }
            assert(std::any_of(planEffects.begin(),planEffects.end(),[&](auto x){
                return x.propertyId==id&&x.parameter==key.second&&x.minimum==lo&&x.maximum==hi;
            }));
        }
        hasSigil=true;
    }
    Build limit{};limit.fill(511);limit[6]|=14u<<12;
    for(unsigned i=0;i<3;i++)limit.depth[i]=4;
    limit.depth[3]=3;assert(points(limit)==99 && valid(limit));
    limit.depth[3]++;assert(!valid(limit));
    for(unsigned p=0;p<9;p++)for(unsigned k=9;k<12;k++){
        Build build;build.fill(511);build[6]|=14u<<12;build[p]|=1u<<k;
        assert(valid(build) && points(build)==85);
        selected=build;level=0;unsigned n=minted;
        SharedEvents::UiMessageEvent click{.structSize=SharedEvents::UiMessageEventSize,
            .target="PanelManager",.command="ClosePanel",.text="WorkshopInscribe"};
        assert(onMessage(&ctx,&click,nullptr)==SharedEvents::UiMessageAction::Consume && pendingWork);
        pendingWork(&ctx,nullptr);pendingWork=nullptr;
        assert(minted==n+1 && forged==minted-1);
        assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==pointRequirement && e.minimum==85;}));
        Build read{};assert(readItem(&ctx,5,read) && read==build && read.depth==build.depth);
        if(p==6 && k==10)assert(std::any_of(planEffects.begin(),planEffects.end(),[&](auto e){return e.propertyId==summonCountProperty && e.minimum==2 && e.maximum==2;}));

    }
    
    Build corpseBuild{};
    corpseBuild[1]=1;corpseBuild.depth[number(1,0)]=4;
    corpseBuild[8]=1u<<3;corpseBuild.depth[number(8,3)]=4;
    clickApply(corpseBuild);
    assert(std::count_if(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==killProperty;})==1);
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){
        return e.propertyId==killProperty&&e.minimum==5&&e.maximum==1;
    }));
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){
        return e.propertyId==attackProperty&&e.parameter==126&&e.minimum==25&&e.maximum==1;
    }));
    Build classBuild{};
    unsigned tabProperty=0,classClusters=0;
    for(unsigned p=0;p<9;p++)for(unsigned i=0;i<9;i+=3){
        unsigned n=number(p,i);
        const auto& first=innerRanks[n][0];
        if(first.count==1&&first.effects[0].minimum==1&&first.effects[0].parameter<24&&innerRanks[n][3].count==1){
            // Class clusters contain three distinct skill-tab parameters.
            auto a=first.effects[0],b=innerRanks[n][1].effects[0],c=innerRanks[n][2].effects[0];
            if(a.propertyId==b.propertyId&&a.propertyId==c.propertyId&&b.parameter==a.parameter+1&&c.parameter==a.parameter+2){
                tabProperty=a.propertyId;classBuild[p]|=1u<<i;classBuild.depth[n]=4;classClusters++;
            }
        }
    }
    assert(classClusters==16&&points(classBuild)==80&&valid(classBuild));
    clickApply(classBuild);
    for(int tab=0;tab<24;tab++)assert(std::any_of(planEffects.begin(),planEffects.end(),[&](auto e){
        return e.propertyId==tabProperty&&e.parameter==tab&&e.minimum==2&&e.maximum==2;
    }));
    // Class clusters must remain effective with Keystones on their paths.
    Build classKeys=classBuild;
    for(unsigned key=9;key<12;key++)classKeys[0]|=routeMask(0,key);
    clickApply(classKeys);
    for(int tab=0;tab<24;tab++)assert(std::any_of(planEffects.begin(),planEffects.end(),[&](auto e){
        return e.propertyId==tabProperty&&e.parameter==tab&&e.minimum==2&&e.maximum==2;
    }));
    std::puts("PASS: all 24 class skill tabs survive Keystone allocation; API receives one valid selector and native handlers receive complete modifiers.");
    Build multi{};multi[0]=routeMask(0,9);multi[1]=routeMask(1,9);
    assert(valid(multi));
    clickApply(multi);
    Build restored{};assert(readItem(&ctx,5,restored) && restored==multi);
    // Reproduce the mixed Mana allocation that crosses the native 223-Mana
    // item field: the property receives logical points, not 256ths twice.
    Build mixedMana{};mixedMana[0]=routeMask(0,4);mixedMana.depth[number(0,4)]=4;
    mixedMana[7]=(1u<<3)|(1u<<5);mixedMana.depth[number(7,5)]=2;
    mixedMana[8]=(1u<<6)|(1u<<8);mixedMana.depth[number(8,8)]=4;
    clickApply(mixedMana);
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==wideStats[0].wide&&e.minimum==446&&e.maximum==446;}));
    assert(std::none_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==wideStats[0].native;}));
    // Swarm grants life/damage in its three middle nodes, then one summon.
    Build swarm{};swarm[6]=511u|(14u<<12);
    swarm.depth[number(6,15)]=4;
    clickApply(swarm);
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==stars[6][12].property&&e.minimum==55;}));
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==stars[6][13].property&&e.minimum==30;}));
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==summonCountProperty&&e.minimum==1;}));
    // A count Keystone must not write a zero Energy property over the shared
    // attribute bundle. Grave Lore keeps its flat Energy plus percentage bonus.
    Build grave{};grave[6]=routeMask(6,10);
    unsigned gp=0,gi=0;
    for(unsigned p=0;p<9;p++)for(unsigned i=0;i<9;i++)if(std::strcmp(stars[p][i].key,"GraveLore")==0){gp=p;gi=i;}
    grave[gp]|=(1u<<(gi-2))|(1u<<gi);grave.depth[number(gp,gi)]=4;
    clickApply(grave);
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==summonCountProperty&&e.minimum==2;}));
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==10&&e.minimum==68;}));
    assert(std::any_of(planEffects.begin(),planEffects.end(),[](auto e){return e.propertyId==287&&e.minimum==10;}));
    // Excel balance import: test actual minted Sigil payloads, including a
    // Keystone above native FHR capacity and the newly selected native effects.
    auto named=[](const char* key){
        for(unsigned p=0;p<9;p++)for(unsigned i=0;i<(p==6?16u:12u);i++)
            if(!std::strcmp(stars[p][i].key,key))return std::pair{p,i};
        assert(false);return std::pair{0u,0u};
    };
    auto allocate=[&](const char* key,unsigned depth=0){
        auto [p,i]=named(key);Build b{};
        b[p]=routeMask(p,i);b.depth[number(p,i)]=depth;
        return b;
    };
    auto exact=[&](unsigned property,int value,int parameter=0,int maximum=-1){
        return std::any_of(planEffects.begin(),planEffects.end(),[&](auto e){
            return e.propertyId==property&&e.parameter==parameter&&e.minimum==value&&e.maximum==(maximum<0?value:maximum);
        });
    };
    clickApply(allocate("Rimeheart"));
    assert(exact(wideStats[11].wide,200)&&exact(flatUnit,1));
    assert(!exact(wideStats[11].native,200));
    clickApply(allocate("BloodTithe"));assert(exact(strikeProperty,25,82,10));
    clickApply(allocate("Ravenlord"));assert(exact(summonCountProperty,2));
    clickApply(allocate("SoulFont"));assert(exact(stars[named("SoulFont").first][named("SoulFont").second].property,3,58));
    auto [rp,ri]=named("Reaping");clickApply(allocate("Reaping",4));
    assert(exact(innerRanks[number(rp,ri)][0].effects[0].propertyId,11));
    auto [np,ni]=named("NightSight");clickApply(allocate("NightSight",4));
    assert(exact(innerRanks[number(np,ni)][0].effects[0].propertyId,20));
    // Regression for Treasure: 50+50+50+100 Gold Find remains on the
    // actual minted Sigil when the Alchemist Keystone is selected.
    auto [tp,ti]=named("Treasure");
    unsigned gold=stars[tp][ti].property;
    Build treasure=allocate("Treasure",4);
    clickApply(treasure);assert(exact(gold,250));
    treasure[tp]|=1u<<9;
    clickApply(treasure);assert(exact(gold,250));
    std::puts("PASS: Treasure grants 250% extra gold with and without Alchemist.");
    auto findProperty=[](unsigned property,int parameter=0){
        int value=0;for(auto e:planEffects)if(e.propertyId==property&&e.parameter==parameter)value+=e.minimum;
        return value;
    };
    {auto [p,i]=named("BattleCry");Build b{};b[p]=routeMask(p,i);clickApply(b);
     const unsigned oskill=stars[p][i].property;
     assert(findProperty(oskill,149)==1&&findProperty(oskill,155)==1&&findProperty(oskill,138)==0);}
    std::puts("PASS: Battle Cry Keystone grants +1 Battle Orders and +1 Battle Command, without Shout.");
    // Every 5/5 elemental specialization retains its actual flat/mastery
    // payload when the branch Keystone is learned.
    for(const char* key:{"Tempest","Inferno","Glacier","Virulence","Fang"}){
        auto [p,i]=named(key);Build b=allocate(key,4);b[p]|=1u<<(9+i/3);
        clickApply(b);
        assert(findProperty(stars[p][i].property)==(std::strcmp(key,"Fang")==0?75:36));
    }
    {auto [p,i]=named("LongReach");Build b=allocate("LongReach",4);clickApply(b);
     assert(findProperty(stars[p][i].property)==75);
     auto [fp,fi]=named("Fang");assert(findProperty(stars[fp][fi].property)==30);}
    for(const char* key:{"Spark","Flow"}){
        auto [p,i]=named(key);Build b=allocate(key,4);b[p]|=routeMask(p,9+i/3);
        clickApply(b);
        assert(findProperty(lightningMinimumWideProperty)==150&&findProperty(lightningMaximumProperty)==150&&findProperty(flatUnit)==1);
    }
    Build poison=allocate("Venom",4);clickApply(poison);
    assert(findProperty(poisonRateWideProperty)==1535&&findProperty(poisonDurationProperty)==25&&findProperty(flatUnit)==1);
    std::puts("PASS: both Lightning clusters retain 150 minimum/maximum damage; Poison stores its full 1535-rate over 25 frames without narrowing.");
    Build speed=allocate("Pursuit",4);auto [wp,wi]=named("Wind");speed.depth[number(wp,wi)]=4;
    clickApply(speed);assert(findProperty(wideStats[6].native)==76&&findProperty(wideStats[11].native)==35);
    speed[wp]|=1u<<10;clickApply(speed);
    assert(findProperty(wideStats[6].native)==76&&findProperty(wideStats[11].native)==35);
    std::puts("PASS: all four spell-mastery clusters grant 36%, Fang retains 75 minimum damage; Pursuit+Wind grant 76 FRW and 35 FHR together, including their Keystone.");
    {auto [p,i]=named("Bloodthirster");Build b{};b[p]=routeMask(p,i);clickApply(b);
     assert(findProperty(stars[p][i].property)==50);}
    {auto [p,i]=named("Berserker");Build b{};b[p]=routeMask(p,i);clickApply(b);
     assert(findProperty(stars[p][i].property,128)==1);}
    // Visible stem prerequisites: Inner -> Side -> Outer -> Keystone.
    selected={};assert(choose(&ctx,"Node0"));
    assert(!choose(&ctx,"Node1")&&!choose(&ctx,"Node9"));
    assert(choose(&ctx,"Node2")&&choose(&ctx,"Node1")&&choose(&ctx,"Node9"));
    assert(choose(&ctx,"Node2"));
    assert(selected[0]==1); // Unlearning Side returns Outer and its Keystone.
    std::puts("PASS: straight stem prerequisites and descendant point refunds match visible edges.");
    std::puts("PASS: imported Keystone procs, +2 summons, Energy Shield, 11 Life on hit, 20% physical pierce and saved 200-FHR overflow.");
    std::puts("PASS: all 27 keystones, Apply click without chart, level requirement, persisted receipts, 336 active cluster rank allocations and their effective properties, 99-point cap, one-sigil replacement.");
    return 0;
}
