#define NOMINMAX
#include <Windows.h>
#include <D2RLPlugin/context.h>
#include <D2RLPlugin/inventory.h>
#include <D2RLPlugin/threads.h>
#include <atomic>
#include <mutex>
#include <array>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "item_sort_model.h"
using namespace D2RL;
const InventoryService* inventory;const ItemService* items;const ThreadService* threads;
namespace Organizer {
struct CatalogEntry {unsigned key;const char* name;int width,height,sortGroup;};
const CatalogEntry BaseNames[]={{1,"Zebra",1,1,4},{2,"Alpha",1,1,4}};
unsigned normalizedCode(unsigned n){return n;}
std::wstring itemName(const Items::ItemInfo& i){return i.code==1?L"Zebra":L"Alpha";}
std::mutex mutex;std::array<bool,1000> known;std::array<std::vector<int>,1000> cache;
struct Row{int page;std::wstring name;};Row selectedResult;ULONGLONG settleUntil;
std::string last;
void nativeLabel(const char*,const char*,const char* s,size_t n){last.assign(s,n);}
bool state(int& page,int& pages,int& season){page=0;pages=1000;season=0;return true;}
}
bool visible(const PluginContext*){return true;}
ULONGLONG testTime=0;
ULONGLONG fakeTime(){return testTime;}
#define GetTickCount64 fakeTime
#include "item_sort.inc"
#undef GetTickCount64
bool game=false;int commits=0;bool reject=false;int positions[3]={0,5,8};bool allVisible=true;int ys[3]={};bool expectColumns=false;
Threads::Callback callback{};void* payload{};
Inventory::Result local(const PluginContext*,PlayerHandle* p)noexcept{*p=1;return Inventory::Result::Success;}
Inventory::Result cursor(const PluginContext*,PlayerHandle,ItemHandle* p)noexcept{*p=0;return Inventory::Result::NotFound;}
Inventory::Result enumerate(const PluginContext* c,PlayerHandle,const Inventory::ItemFilter*,Inventory::ItemCallback cb,void* u)noexcept{
 for(int n=1;n<=2;++n){Items::ItemInfo i{};i.handle=(game?100:10)+n;i.runtimeId=n;i.code=n;i.x=positions[n];i.y=ys[n];i.container=Items::ItemContainer::SharedStash;i.sharedStashPage=0;i.quality=Items::Quality::Unique;cb(c,&i,u);}return Inventory::Result::Success;
}
Threads::Result queue(const PluginContext*,Threads::Callback cb,void* p)noexcept{callback=cb;payload=p;return Threads::Result::Success;}
Items::Result commit(const PluginContext*,const Items::ExistingItemTransaction* tx,Items::ExistingItemTransactionResult* result)noexcept{
 assert(game);assert(tx->operationCount==2);assert(tx->operations[0].item==102&&tx->operations[1].item==101);assert(tx->operations[0].move.destination.x==0&&tx->operations[0].move.destination.y==0);assert(tx->operations[1].move.destination.x==(expectColumns?0u:1u)&&tx->operations[1].move.destination.y==(expectColumns?1u:0u));
 if(!reject){bool unchanged=true;for(unsigned k=0;k<tx->operationCount;++k){auto& op=tx->operations[k];int code=int(op.item-100);unchanged&=positions[code]==int(op.move.destination.x)&&ys[code]==int(op.move.destination.y);positions[code]=int(op.move.destination.x);ys[code]=int(op.move.destination.y);}allVisible=unchanged;}
 ++commits;result->committedOperationCount=reject?0:2;return reject?Items::Result::Conflict:Items::Result::Success;
}
int main(){
 InventoryService inv{};inv.getLocalPlayer=local;inv.getCursorItem=cursor;inv.forEachInventoryItem=enumerate;inventory=&inv;
 ItemService item{};item.serviceSize=sizeof(item);item.serviceVersion=1;item.capabilities=Items::ItemServiceCapabilityBit(Items::ItemServiceCapability::SharedStashWrite);item.executeExistingItemTransaction=commit;items=&item;
 ThreadService thread{};thread.runOnGameThread=queue;thread.runOnUiThread=queue;threads=&thread;PluginContext context{};
 for(int n=0;n<2;++n){reject=n;expectColumns=n!=0;game=false;Sorting::start(&context,reinterpret_cast<void*>(uintptr_t(Items::ItemContainer::SharedStash)));assert(Sorting::busy);
 game=true;auto cb=callback;cb(&context,payload);if(!reject)assert(!allVisible);
 game=false;testTime+=300;cb=callback;cb(&context,payload);
 if(!reject){assert(Sorting::busy);game=true;cb=callback;cb(&context,payload);assert(allVisible);game=false;cb=callback;cb(&context,payload);}
 assert(!Sorting::busy);assert(Organizer::last==(reject?"Sort rejected (4). Items unchanged.":"Sorted."));}
 assert(commits==3);assert(Sorting::nextColumnsFirst[8]);assert(!Sorting::nextColumnsFirst[4]&&!Sorting::nextColumnsFirst[5]);
 // User movement during the publication delay must not be undone.
 reject=false;positions[1]=5;positions[2]=8;game=false;Sorting::start(&context,reinterpret_cast<void*>(uintptr_t(Items::ItemContainer::SharedStash)));
 game=true;auto next=callback;next(&context,payload);positions[1]=7;game=false;testTime+=300;next=callback;next(&context,payload);
 game=true;next=callback;next(&context,payload);game=false;next=callback;next(&context,payload);
 assert(commits==4&&positions[1]==7&&!Sorting::busy);assert(!Sorting::nextColumnsFirst[8]);assert(Organizer::last=="Items changed after sorting; refresh skipped.");
std::cout<<"PASS: server handles, delayed position-preserving refresh, rejection and busy reset\n";
}
