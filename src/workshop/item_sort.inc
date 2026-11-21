// Sort existing items through the loader's authoritative transaction API.
namespace Sorting {
inline std::atomic<bool> busy{false};
inline std::array<std::atomic<bool>,10> nextColumnsFirst{};
struct Job {PlayerHandle player{};Items::ItemContainer container{};int page=-1;std::vector<Items::ItemInfo> before;std::vector<Items::ExistingItemOperation> operations;std::string status;bool columnsFirst=false;bool resync=false;ULONGLONG refreshAfter=0;};
inline const char* panel(Items::ItemContainer c){return c==Items::ItemContainer::Inventory?"PlayerInventoryExpansionLayout":c==Items::ItemContainer::Cube?"HoradricCubeLayout":"BankExpansionLayout";}
inline void status(const PluginContext* c,Items::ItemContainer container,const std::string& text){Organizer::nativeLabel(panel(container),"WorkshopSortStatus",text.c_str(),text.size());c->LogInfo(("[sort] "+text).c_str());}
inline void __cdecl syncClient(const PluginContext*,void*) noexcept;
inline void __cdecl finished(const PluginContext* c,void* raw) noexcept {
 auto job=static_cast<Job*>(raw);
 if(job->resync&&GetTickCount64()<job->refreshAfter){if(threads->runOnUiThread(c,finished,job)==Threads::Result::Success)return;job->resync=false;job->status="Sorted; reopen the page to refresh.";}
 if(job->resync){job->resync=false;if(threads->runOnGameThread(c,syncClient,job)==Threads::Result::Success)return;job->status="Sorted; reopen the page to refresh.";}
 status(c,job->container,job->status);
 {std::lock_guard lock(Organizer::mutex);Organizer::known={};for(auto& v:Organizer::cache)v.clear();Organizer::selectedResult={-1,L""};Organizer::settleUntil=GetTickCount64()+300;}
 delete job;busy=false;
}
inline Inventory::IterationAction __cdecl collect(const PluginContext*,const Items::ItemInfo* i,void* raw) noexcept {
 auto job=static_cast<Job*>(raw);if(i&&i->container==job->container&&(job->container!=Items::ItemContainer::SharedStash||i->sharedStashPage==unsigned(job->page)))job->before.push_back(*i);return Inventory::IterationAction::Continue;
}
// A second, position-preserving publication after the first packet batch has
// reached the UI fixes client cells hidden by overlapping old/new positions.
// This is not another sort: abort if any item no longer occupies its target.
inline void __cdecl syncClient(const PluginContext* c,void* raw) noexcept {
 auto job=static_cast<Job*>(raw);auto original=job->before;job->before.clear();
 Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,.containerMask=Items::ContainerBit(job->container)};
 bool valid=inventory->forEachInventoryItem(c,job->player,&filter,collect,job)==Inventory::Result::Success;
 if(job->before.size()!=original.size())valid=false;
 for(auto& op:job->operations){
  auto old=std::find_if(original.begin(),original.end(),[&](const auto& i){return i.handle==op.item;});
  auto fresh=old==original.end()?job->before.end():std::find_if(job->before.begin(),job->before.end(),[&](const auto& i){return i.runtimeId==old->runtimeId&&i.code==old->code;});
  if(fresh==job->before.end()||fresh->x!=int(op.move.destination.x)||fresh->y!=int(op.move.destination.y)){valid=false;break;}op.item=fresh->handle;
 }
 if(valid){
  Items::ExistingItemTransaction tx{.structSize=Items::ExistingItemTransactionSize,.player=job->player,.operationCount=uint32_t(job->operations.size()),.operations=job->operations.data()};
  Items::ExistingItemTransactionResult result{.structSize=Items::ExistingItemTransactionResultSize};auto rc=items->executeExistingItemTransaction(c,&tx,&result);
  char line[128];std::snprintf(line,sizeof(line),"[sort] client refresh result=%u committed=%u",unsigned(rc),result.committedOperationCount);c->LogInfo(line);
  if(rc!=Items::Result::Success)job->status="Sorted; reopen the page to refresh.";
 }else job->status="Items changed after sorting; refresh skipped.";
 if(threads->runOnUiThread(c,finished,job)!=Threads::Result::Success){delete job;busy=false;}
}
inline void __cdecl execute(const PluginContext* c,void* raw) noexcept {
 auto job=static_cast<Job*>(raw);auto container=job->container;
 auto complete=[&](){if(threads->runOnUiThread(c,finished,job)!=Threads::Result::Success){delete job;busy=false;}};
 auto fail=[&](const char* text){job->status=text;complete();};
 // Mint item handles from authoritative enumeration, not client UI snapshots.
 // Legacy handles validate the item's generation/item seeds on the calling side.
 job->before.clear();job->operations.clear();
 Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,.containerMask=Items::ContainerBit(container)};
 auto enumeration=inventory->forEachInventoryItem(c,job->player,&filter,collect,job);
 if(enumeration!=Inventory::Result::Success){job->status="Cannot read server items ("+std::to_string(unsigned(enumeration))+").";complete();return;}
 if(job->before.empty()){fail("No items in the selected container.");return;}
 std::vector<ItemSortModel::Item> plan;
 for(auto& i:job->before){const Organizer::CatalogEntry* base=nullptr;for(auto& b:Organizer::BaseNames)if(Organizer::normalizedCode(b.key)==Organizer::normalizedCode(i.code)){base=&b;break;}
  if(!base){fail("Unknown item dimensions. Sort cancelled.");return;}
  auto name=Organizer::itemName(i);plan.push_back({i.handle,base->width,base->height,base->sortGroup,int(i.quality),name});
 }
 int w=container==Items::ItemContainer::Inventory?10:container==Items::ItemContainer::Cube?12:16;
 int h=container==Items::ItemContainer::SharedStash?13:8;
 if(!ItemSortModel::plan(plan,w,h,job->columnsFirst)){fail("Sorted order does not fit. Items unchanged.");return;}
 for(auto& i:plan){Items::ExistingItemOperation op{};op.structSize=Items::ExistingItemOperationSize;op.kind=Items::ExistingItemOperationKind::Move;op.item=i.id;
  op.move.destination={.structSize=Items::ItemDestinationSize,.container=container,.placement=Items::Placement::Exact,.x=unsigned(i.x),.y=unsigned(i.y),.sharedStashPage=unsigned(std::max(0,job->page))};job->operations.push_back(op);
 }
 Items::ExistingItemTransaction tx{.structSize=Items::ExistingItemTransactionSize,.player=job->player,.operationCount=uint32_t(job->operations.size()),.operations=job->operations.data()};
 Items::ExistingItemTransactionResult result{.structSize=Items::ExistingItemTransactionResultSize};
 auto rc=items->executeExistingItemTransaction(c,&tx,&result);
 char log[180];std::snprintf(log,sizeof(log),"[sort] authoritative transaction container=%u operations=%u result=%u failed=%u committed=%u",unsigned(container),tx.operationCount,unsigned(rc),result.failureIndex,result.committedOperationCount);c->LogInfo(log);
 if(rc==Items::Result::Success)nextColumnsFirst[unsigned(container)]=!job->columnsFirst;
 job->resync=rc==Items::Result::Success;job->refreshAfter=GetTickCount64()+250;
 job->status=rc==Items::Result::Success?"Sorted.":"Sort rejected ("+std::to_string(unsigned(rc))+"). Items unchanged.";
 complete();
}
inline void __cdecl start(const PluginContext* c,void* raw) noexcept {
 auto container=static_cast<Items::ItemContainer>(reinterpret_cast<uintptr_t>(raw));if(busy.exchange(true))return;
 Job* job=new Job;job->container=container;job->columnsFirst=nextColumnsFirst[unsigned(container)].load();
 auto fail=[&](const char* text){status(c,container,text);delete job;busy=false;};
 if(!HasItemServiceField(items,ItemServiceRequiredSize)||!items->executeExistingItemTransaction){fail("Sort requires item move support.");return;}
 if(container==Items::ItemContainer::SharedStash){int pages,season;
  if(!visible(c)||!Organizer::state(job->page,pages,season)||season!=0){fail("Open a normal shared stash page.");return;}
  if(!HasItemServiceCapability(items,Items::ItemServiceCapability::SharedStashWrite)){fail("Loader does not support shared stash moves.");return;}
 }
 if(inventory->getLocalPlayer(c,&job->player)!=Inventory::Result::Success){fail("No active player.");return;}
 ItemHandle cursor{};auto cursorResult=inventory->getCursorItem(c,job->player,&cursor);
 if((cursorResult!=Inventory::Result::Success&&cursorResult!=Inventory::Result::NotFound)||cursor){fail("Put down the cursor item first.");return;}
 Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,.containerMask=Items::ContainerBit(container)};
 if(inventory->forEachInventoryItem(c,job->player,&filter,collect,job)!=Inventory::Result::Success){fail("Cannot read items.");return;}
 if(job->before.empty()){fail("Nothing to sort.");return;}
 status(c,container,"Sorting...");if(threads->runOnGameThread(c,execute,job)!=Threads::Result::Success)fail("Sort could not start.");
}
}
