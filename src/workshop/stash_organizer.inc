// Native in-game organizer. UI calls are queued on the UI thread; the overlay only draws snapshots.
// Metadata is separate from character and shared stash save files. No items are moved.
namespace Organizer {
#include "stash_organizer_catalog.h"
using namespace StashOrganizerModel;
inline const PluginContext* context{};
inline const OverlayService* draw{};
inline Panels::RegistrationHandle panelHandle{};
inline std::filesystem::path metadata;
inline std::array<Tab,1000> tabs{};
inline std::array<std::vector<Found>,1000> cache{};
inline std::array<bool,1000> known{};
struct Row {int page;std::wstring label;int x=-1,y=-1;std::wstring itemName;};
inline Row selectedResult{-1,L""};
inline std::vector<Row> results;
inline std::mutex mutex;
inline std::atomic<bool> pending{false};
inline bool available=false,opened=false,stashOpen=false,loaded=false,favoritesOnly=false;
inline std::atomic<int> detectedProfile{-1};
inline std::atomic<bool> profileRequestPending{false};
inline int current=-1,total=0,activeSeason=-1,profile=0,resultOffset=0;
inline PageNavigation navigation;
inline int& target=navigation.target;inline int& scanPage=navigation.scanPage;inline bool& scanning=navigation.scanning;
inline ULONGLONG settleUntil=0,lastCapture=0,lastQueued=0;
inline std::wstring query,message;
inline bool filterApplied=false;
inline Editor editors[3];
inline std::atomic<int> activeEdit{-1};
inline bool traceFrame=false,traceText=false;
inline Overlay::Color colors[]={{.82f,.82f,.82f,1},{.92f,.73f,.30f,1},{.40f,.75f,.49f,1},{.39f,.65f,.90f,1},{.76f,.53f,.90f,1},{.93f,.51f,.51f,1}};
inline void report(const std::wstring& s){message=s;}
inline unsigned normalizedCode(unsigned code){unsigned n=0;for(unsigned i=0;i<4;++i){unsigned c=(code>>(8*i))&255;if(c&&c!=32)n|=c<<(8*i);}return n;}
inline bool state(int& cur,int& pages,int& seas) noexcept {
 __try {
  auto panel=reinterpret_cast<uintptr_t>(find(*manager,"BankExpansionLayout"));if(!panel)return false;
  auto n=count(panel);auto a=season(panel);if(a>=2||n<1||n>1000)return false;
  auto i=*reinterpret_cast<uint64_t*>(panel+0x168+a*8);if(i>=n)return false;
  cur=int(i);pages=int(n);seas=a;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline std::wstring wide(const char* s){
 if(!s||!*s)return {};int n=MultiByteToWideChar(CP_UTF8,0,s,-1,nullptr,0);if(n<1)return {};
 std::wstring out(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s,-1,out.data(),n);out.pop_back();return out;
}
// Legacy read-only adapter already used by forge_tooltip.inc. Resolve a live
// client unit on the UI thread; copy stats and never retain native pointers.
inline bool searchStats(const Items::ItemInfo& info,int* values) noexcept {
 __try {
  auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
  const unsigned char guard[]={0x44,0x89,0xf9,0x31,0xd2,0xff,0x15,0x94,0x34,0x2e,0x00};
  if(!core||!readItemStat||std::memcmp(core+0x41d309,guard,sizeof(guard)))return false;
  auto lookup=*reinterpret_cast<void*(__fastcall**)(unsigned,unsigned)>(core+0x7007a8);
  if(!lookup)return false;auto unit=static_cast<unsigned*>(lookup(info.runtimeId,4));
  if(!unit||unit[0]!=4||unit[1]!=info.classId||unit[2]!=info.runtimeId)return false;
  for(size_t i=0;i<std::size(SearchStats);++i)values[i]=readItemStat(unit,SearchStats[i].id,0);
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline std::wstring affixText(const Items::ItemInfo& info){
 if(!(info.stateFlags&Items::ItemStateIdentified))return {};
 std::array<int,std::size(SearchStats)> values{};std::wstring text;
 if(!searchStats(info,values.data()))return text;
 int resistanceCount=0;
 for(size_t i=0;i<values.size();++i){if(!values[i])continue;
  text+=L" ";text+=wide(values[i]>0?SearchStats[i].positive:SearchStats[i].negative);
  int id=SearchStats[i].id;if((id==39||id==41||id==43||id==45)&&values[i]>0)++resistanceCount;
  if(id==17||id==18)text+=L" enhanced weapon damage";
 }
 if(resistanceCount==4)text+=L" all resistances all resist";
 return text;
}
inline std::wstring section(int page){return L"profile"+std::to_wstring(profile)+L"-season"+std::to_wstring(activeSeason)+L"-page"+std::to_wstring(page+1);}
inline void readMetadata(){
 tabs={};known={};for(auto& c:cache)c.clear();
 for(int i=0;i<total;++i){wchar_t b[160]{};auto sec=section(i);
  GetPrivateProfileStringW(sec.c_str(),L"name",L"",b,160,metadata.c_str());tabs[i].name=b;
  tabs[i].favorite=GetPrivateProfileIntW(sec.c_str(),L"favorite",0,metadata.c_str())!=0;
  tabs[i].color=std::min(5u,GetPrivateProfileIntW(sec.c_str(),L"color",0,metadata.c_str()));
 }loaded=true;
}
inline bool saveTab(int i){
 auto sec=section(i);auto color=std::to_wstring(tabs[i].color);
 return WritePrivateProfileStringW(sec.c_str(),L"name",tabs[i].name.c_str(),metadata.c_str())&&
 WritePrivateProfileStringW(sec.c_str(),L"favorite",tabs[i].favorite?L"1":L"0",metadata.c_str())&&
 WritePrivateProfileStringW(sec.c_str(),L"color",color.c_str(),metadata.c_str());
}
inline std::wstring itemName(const Items::ItemInfo& info){
 std::wstring base;for(auto& e:BaseNames)if(normalizedCode(e.key)==normalizedCode(info.code)){base=wide(e.name);break;}
 if(info.quality==Items::Quality::Unique&&info.qualityRecordId>=0&&unsigned(info.qualityRecordId)<std::size(UniqueNames))base=wide(UniqueNames[info.qualityRecordId])+L" / "+base;
 if(info.quality==Items::Quality::Set&&info.qualityRecordId>=0&&unsigned(info.qualityRecordId)<std::size(SetNames))base=wide(SetNames[info.qualityRecordId])+L" / "+base;
 for(auto& e:RunewordNames)if(e.id==info.prefixIds[0]){base=wide(e.name)+L" / "+base+L" runeforged runeword";break;}
 const wchar_t* quality[]={L"",L"inferior",L"normal",L"superior",L"magic",L"set",L"rare",L"unique",L"crafted"};
 if(unsigned(info.quality)<std::size(quality))base+=L" "+std::wstring(quality[unsigned(info.quality)]);
 char code[5]{};std::memcpy(code,&info.code,4);base+=L" ["+wide(code)+L"]";return base;
}
inline std::string utf8(const std::wstring& s){
 if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);
 std::string out(n,'\0');WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;
}
inline bool readInput(const char* name,std::wstring& out){int i=!std::strcmp(name,"Search")?0:!std::strcmp(name,"PageNameInput")?1:2;out=editors[i].text;return true;}
inline std::wstring input(const char* name){std::wstring out;readInput(name,out);return out;}
inline std::wstring title(int i){return std::wstring(tabs[i].favorite?L"* ":L"")+L"Page "+std::to_wstring(i+1)+L"  "+(tabs[i].name.empty()?L"Unnamed":tabs[i].name);}
// Native labels use the same engine setter as stock TextBoxWidget controls.
// Descriptor resolution is shared with navigation; never treat SDK handles as pointers.
inline bool nativeLabel(const char* panelName,const char* name,const char* text,size_t length) noexcept {
 __try {
  if(!manager||!*manager||!setText)return false;
  auto panel=find(*manager,panelName);if(!panel)return false;
  auto widget=static_cast<unsigned char*>(child(panel,name));if(!widget)return false;
  auto data=*reinterpret_cast<const char**>(widget+0x88);
  auto size=*reinterpret_cast<size_t*>(widget+0x90);
  if(size==length&&(!length||(data&&!std::memcmp(data,text,length))))return true;
  TextView view{text,length};setText(widget,&view);return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void nativeLabel(const char* name,std::wstring text,unsigned color=0,size_t limit=48){
 if(text.size()>limit)text=text.substr(0,limit-3)+L"...";
 auto encoded=utf8(text);
 if(!nativeLabel("astral-workshop/StashOrganizer",name,encoded.data(),encoded.size())){
  static bool logged=false;if(!logged){context->LogWarn("[stash] Native result label unavailable.");logged=true;}
 }
}
inline void nativeImage(const char* panelName,const char* name,bool show,int frame=-1) noexcept {
 __try {
  auto p=find(*manager,panelName);if(!p)return;
  auto w=static_cast<unsigned char*>(child(p,name));if(!w)return;
  auto v=*reinterpret_cast<void***>(w);const unsigned char expected[]={0x88,0x51,0x51,0xc3};
  if(std::memcmp(v[10],expected,sizeof(expected)))return;
  if(!show){reinterpret_cast<void(*)(void*,bool)>(v[10])(w,false);return;}
  if(frame>=0){const unsigned char setter[]={0x89,0x91,0x90,0,0,0,0xc3};auto fn=context->exeBase+0x859700;
   if(std::memcmp(reinterpret_cast<void*>(fn),setter,sizeof(setter)))return;
   reinterpret_cast<void(*)(void*,unsigned)>(fn)(w,unsigned(frame));
  }
  reinterpret_cast<void(*)(void*,bool)>(v[10])(w,show);
 }__except(EXCEPTION_EXECUTE_HANDLER){}
}
inline void syncMarks(){
 int cells[13][16]{};
 if(filterApplied&&opened&&current>=0&&current<total&&!query.empty()&&known[current]&&GetTickCount64()>=settleUntil){
  for(auto& item:cache[current]){
   if(item.x<0||item.y<0||item.x+item.width>16||item.y+item.height>13)continue;
   bool hit=selectedResult.page==current&&selectedResult.x>=0
    ? item.x==selectedResult.x&&item.y==selectedResult.y&&item.name==selectedResult.itemName
    : matches(item.name+L" "+item.searchText,query);
   for(int y=0;y<item.height;++y)for(int x=0;x<item.width;++x)
    cells[item.y+y][item.x+x]=hit?((x==0?1:0)|(x==item.width-1?2:0)|(y==0?4:0)|(y==item.height-1?8:0)):16;
  }
 }
 for(int y=0;y<13;++y)for(int x=0;x<16;++x){char name[40];std::snprintf(name,sizeof(name),"WorkshopMatch%d_%d",x,y);
  nativeImage("BankExpansionLayout",name,cells[y][x]!=0,cells[y][x]);}
}
inline void syncNative(){
 if(current<0||current>=total||!stashOpen)return;
 auto caption=utf8(title(current).substr(0,22));
 nativeLabel("BankExpansionLayout","WorkshopStashPageTitle",caption.data(),caption.size());
 syncMarks();
 for(int c=0;c<6;++c){char name[40];std::snprintf(name,sizeof(name),"WorkshopPageColor%d",c);nativeImage("BankExpansionLayout",name,tabs[current].color==c);}
 if(!opened)return;
 for(int i=0;i<3;++i){const char* names[]={"SearchValue","PageNameInputValue","PageValue"};auto value=editors[i].text;
  size_t cursor=editors[i].cursor;size_t width=i==0?15:24;size_t start=cursor>width?cursor-width:0;value=value.substr(start,width+2);
  if(activeEdit==i)value.insert(std::min(cursor-start,value.size()),L"|");nativeLabel(names[i],value,0,i==2?5:28);}
 for(int c=0;c<6;++c){char name[40];std::snprintf(name,sizeof(name),"CurrentColor%d",c);nativeImage("astral-workshop/StashOrganizer",name,tabs[current].color==c);}
 nativeLabel("CurrentPage",title(current)+ (profile?L" [HC]":L" [SC]"),tabs[current].color);
 nativeLabel("FavoritesLabel",favoritesOnly?L"Show All Pages":L"Show Only Favorites",0,24);
 nativeLabel("FavoriteLabel",tabs[current].favorite?L"Remove Favorite":L"Make Favorite",0,20);
 for(int i=0;i<10;++i){char name[24];std::snprintf(name,sizeof(name),"Result%d",i);int row=resultOffset+i;
  for(int c=0;c<6;++c){char image[40];std::snprintf(image,sizeof(image),"ResultColor%d_%d",i,c);nativeImage("astral-workshop/StashOrganizer",image,row<int(results.size())&&tabs[results[row].page].color==c);}
  nativeLabel(name,row<int(results.size())?results[row].label:(i==0?L"No matches. Try another search.":L""),row<int(results.size())?tabs[results[row].page].color:0);
 }
 int indexed=0;for(int i=0;i<total;++i)if(known[i])++indexed;
 nativeLabel("ResultCount",std::to_wstring(results.size())+L" hits | "+std::to_wstring(indexed)+L"/"+std::to_wstring(total)+L" indexed",0,32);
 const wchar_t* colorNames[]={L"Default",L"Gold",L"Green",L"Blue",L"Purple",L"Red"};
 nativeLabel("ColorLabel",colorNames[tabs[current].color],tabs[current].color,10);
 nativeLabel("Status",message,1);
}
inline HHOOK textKeyHook{};
inline bool editorVisible() noexcept {
 __try {auto p=static_cast<unsigned char*>(find(*manager,"astral-workshop/StashOrganizer"));return p&&p[0x51]==1;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void __cdecl action(const PluginContext*,void*) noexcept;
inline void __cdecl editKey(const PluginContext*,void* data) noexcept {
 std::lock_guard lock(mutex);unsigned packed=unsigned(reinterpret_cast<uintptr_t>(data));int field=(packed>>24)&3;unsigned kind=(packed>>16)&255,value=packed&65535;
 if(!opened||activeEdit!=field)return;auto& e=editors[field];
 if(kind==0)e.insert(wchar_t(value),field==0?80:field==1?40:4,field==2);
 if(kind==1)e.backspace();if(kind==2)e.remove();if(kind==3)e.move(value);
 if(kind==4){e.selected=true;e.cursor=e.text.size();}
 if(kind==5)activeEdit=(field+1)%3;
 if(kind==6){activeEdit=-1;if(field==0||field==2)threads->runOnUiThread(context,action,reinterpret_cast<void*>(uintptr_t(field==0?11:6)));}
 if(kind==7&&OpenClipboard(nullptr)){auto h=GetClipboardData(CF_UNICODETEXT);if(h){auto text=static_cast<const wchar_t*>(GlobalLock(h));if(text){for(size_t i=0;text[i]&&i<4096;++i)e.insert(text[i],field==0?80:field==1?40:4,field==2);GlobalUnlock(h);}}CloseClipboard();}
 syncNative();
}
inline void queueEdit(unsigned kind,unsigned value=0){int field=activeEdit;if(field<0)return;unsigned packed=(unsigned(field)<<24)|(kind<<16)|value;threads->runOnUiThread(context,editKey,reinterpret_cast<void*>(uintptr_t(packed)));}
inline ResultWheel resultWheel;
inline bool wheelOverResults(MSG* msg){
 POINT pt{short(LOWORD(msg->lParam)),short(HIWORD(msg->lParam))};RECT rect{};
 if(!GetClientRect(msg->hwnd,&rect)||!ScreenToClient(msg->hwnd,&pt))return false;
 return inResultArea(float(pt.x),float(pt.y),float(rect.right-rect.left),float(rect.bottom-rect.top));
}
inline LRESULT CALLBACK textKeys(int code,WPARAM removed,LPARAM data){
 if(code==HC_ACTION&&removed==PM_REMOVE){
  auto msg=reinterpret_cast<MSG*>(data);
  if(msg->message==WM_MOUSEWHEEL&&editorVisible()&&wheelOverResults(msg)){
   int steps=resultWheel.steps(short(HIWORD(msg->wParam)));
   for(int i=0;i<std::abs(steps);++i)threads->runOnUiThread(context,action,reinterpret_cast<void*>(uintptr_t(steps>0?9:10)));
   msg->message=WM_NULL;return CallNextHookEx(textKeyHook,code,removed,data);
  }
 }

 if(code==HC_ACTION&&removed==PM_REMOVE&&activeEdit>=0&&editorVisible()){
  auto msg=reinterpret_cast<MSG*>(data);auto kind=msg->message;
  if(kind==WM_CHAR){if(msg->wParam>=32)queueEdit(0,unsigned(msg->wParam));msg->message=WM_NULL;}
  if(kind==WM_KEYDOWN||kind==WM_KEYUP||kind==WM_SYSKEYDOWN||kind==WM_SYSKEYUP){
   if(msg->wParam==VK_ESCAPE){activeEdit=-1;return CallNextHookEx(textKeyHook,code,removed,data);}
   if(kind==WM_KEYDOWN){
    switch(msg->wParam){
     case VK_BACK:queueEdit(1);break;case VK_DELETE:queueEdit(2);break;
     case VK_HOME:queueEdit(3,0);break;case VK_END:queueEdit(3,1);break;
     case VK_LEFT:queueEdit(3,2);break;case VK_RIGHT:queueEdit(3,3);break;
     case VK_TAB:queueEdit(5);break;case VK_RETURN:queueEdit(6);break;
     default:if(GetKeyState(VK_CONTROL)&0x8000){if(msg->wParam=='A')queueEdit(4);if(msg->wParam=='V')queueEdit(7);}
      else TranslateMessage(msg);break;
    }
   }
   msg->message=WM_NULL;
  }
 }
 return CallNextHookEx(textKeyHook,code,removed,data);
}
inline void installTextKeys(){
 if(textKeyHook)return;
 DWORD process=0;auto thread=GetWindowThreadProcessId(GetForegroundWindow(),&process);
 if(process!=GetCurrentProcessId()||!thread)return;
 textKeyHook=SetWindowsHookExW(WH_GETMESSAGE,textKeys,nullptr,thread);
 if(!textKeyHook)context->LogWarn("[stash] Text input key guard could not attach.");
 else context->LogInfo("[stash] Native text-field key guard attached.");
}
inline void rebuild(){
 results.clear();
 for(int i=0;i<total;++i){if(favoritesOnly&&!tabs[i].favorite)continue;
  if(query.empty()||(!tabs[i].name.empty()&&matches(tabs[i].name,query)))results.push_back({i,title(i)});
  if(!query.empty())for(auto& item:cache[i])if(matches(item.name+L" "+item.searchText,query))results.push_back({i,L"Page "+std::to_wstring(i+1)+L" | "+item.name,item.x,item.y,item.name});
 }
 resultOffset=std::clamp(resultOffset,0,std::max(0,(int(results.size())-1)/10*10));
}
struct Capture {int page;unsigned unmapped=0;unsigned seen=0;std::vector<Found> items;};
inline Inventory::IterationAction __cdecl collect(const PluginContext*,const Items::ItemInfo* item,void* data) noexcept {
 auto& c=*static_cast<Capture*>(data);
 if(!item||item->container!=Items::ItemContainer::SharedStash)return Inventory::IterationAction::Continue;
 ++c.seen;
 if(item->sharedStashPage==UINT32_MAX){++c.unmapped;return Inventory::IterationAction::Continue;}
 if(item->sharedStashPage>=unsigned(total))return Inventory::IterationAction::Continue;
 int w=1,h=1;for(auto& e:BaseNames)if(normalizedCode(e.key)==normalizedCode(item->code)){w=e.width;h=e.height;break;}
 c.items.push_back({int(item->sharedStashPage),item->x,item->y,itemName(*item),w,h,affixText(*item)});return Inventory::IterationAction::Continue;
}
inline bool capture(){
 if(current<0||activeSeason!=0)return false;
 PlayerHandle player{};auto pr=inventory->getLocalPlayer(context,&player);
 if(pr!=Inventory::Result::Success){report(L"Cannot read local player ("+std::to_wstring(unsigned(pr))+L").");return false;}
 Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,.containerMask=Items::ContainerBit(Items::ItemContainer::SharedStash)};
 Capture c{current};auto result=inventory->forEachInventoryItem(context,player,&filter,collect,&c);
 int after, pages, seas;
 if(result!=Inventory::Result::Success||!state(after,pages,seas)||after!=current||seas!=activeSeason){
  report(L"Cannot read shared page ("+std::to_wstring(unsigned(result))+L").");
  static bool logged=false;if(!logged){auto line=utf8(L"[stash] Index failed: "+message);context->LogWarn(line.c_str());logged=true;}return false;
 }
 // The legacy loader enumerates special/previous-season proxy inventories too.
 // Those deliberately have UINT32_MAX page identities; they must not invalidate
 // normal numbered pages. Keep only explicit page identities, never guess one.
 bool first=!known[current];std::array<bool,1000> replaced{};
 cache[current].clear();known[current]=true;replaced[current]=true;
 for(auto& item:c.items){int p=item.page;if(!replaced[p]){cache[p].clear();replaced[p]=true;known[p]=true;}cache[p].push_back(std::move(item));}
 rebuild();
 if(first){char log[160];std::snprintf(log,sizeof(log),"[stash] Indexed page=%d items=%zu sharedItemsSeen=%u ignoredSpecial=%u",current+1,cache[current].size(),c.seen,c.unmapped);context->LogInfo(log);}
 return true;
}
inline void stopScan(){navigation.stop();}
inline void requestPage(int i){if(i<0||i>=total)return;navigation.go(i);report(L"Opening page "+std::to_wstring(i+1)+L".");}
inline void close(){activeEdit=-1;opened=false;selectedResult={-1,L""};syncMarks();panels->closePanel(context,panelHandle);}
inline void __cdecl open(const PluginContext* c,void*) noexcept {
 std::lock_guard lock(mutex);
 if(!available){c->LogError("[stash] Organizer click received, but initialization failed.");return;}
 if(detectedProfile<0){c->LogInfo("[stash] Waiting for character save-pool detection.");return;}
 if(profile!=detectedProfile){profile=detectedProfile;loaded=false;}
 if(!ready||!visible(c)){c->LogInfo("[stash] Organizer click ignored: shared stash is not visible.");return;}
 int oldSeason=activeSeason;if(!state(current,total,activeSeason))return;
 if(!loaded||oldSeason!=activeSeason)readMetadata();
 if(opened){close();return;}
 auto result=panels->openPanel(c,panelHandle);
 if(result!=Panels::Result::Success){char line[120];std::snprintf(line,sizeof(line),"[stash] Native organizer could not open: %u",unsigned(result));c->LogError(line);return;}
 editors[1].text=tabs[current].name;editors[1].cursor=editors[1].text.size();editors[1].selected=false;
 editors[2].text=std::to_wstring(current+1);editors[2].cursor=editors[2].text.size();editors[2].selected=false;
 resultWheel.remainder=0;installTextKeys();opened=true;stashOpen=true;traceFrame=traceText=true;settleUntil=GetTickCount64()+300;rebuild();
 report(L"Enter text, then click Search to apply it.");
 syncNative();c->LogInfo("[stash] Native organizer opened.");
}
// Called on the UI thread from the shared message dispatcher.
inline void __cdecl action(const PluginContext*,void* data) noexcept {
 int id=int(reinterpret_cast<intptr_t>(data));std::lock_guard lock(mutex);
 {char line[100];std::snprintf(line,sizeof(line),"[stash] Menu action=%d page=%d opened=%d",id,current+1,int(opened));context->LogInfo(line);}
 if(!opened||current<0)return;
 if(id>=30&&id<=32){activeEdit=id-30;editors[id-30].cursor=editors[id-30].text.size();syncNative();return;}
 activeEdit=-1;
 if(id==1){close();return;}
 if(id==2){std::wstring name;if(!readInput("PageNameInput",name)){report(L"Name field is not ready; please reopen the menu.");return;}tabs[current].name=name;report(saveTab(current)?L"Page name saved.":L"Could not save page name.");}
 if(id==3){tabs[current].favorite=!tabs[current].favorite;report(saveTab(current)?L"Favorite updated.":L"Could not save favorite.");}
 if(id==4){favoritesOnly=!favoritesOnly;resultOffset=0;}
 if(id==6){int n=pageNumber(input("Page"),total);if(n<0)report(L"Enter a page from 1 to "+std::to_wstring(total)+L".");else requestPage(n);}
 if(id==8){settleUntil=0;capture();lastCapture=GetTickCount64();}
 if(id==9)resultOffset=std::max(0,resultOffset-10);
 if(id==10)resultOffset+=10;
 if(id==11){selectedResult={-1,L""};query=input("Search");if(query.find_first_not_of(L" \t\r\n")==std::wstring::npos)query.clear();filterApplied=!query.empty();resultOffset=0;report(query.empty()?L"Filter cleared.":L"Filter applied. Click an item to reveal it in the stash.");}
 if(id==34){selectedResult={-1,L""};editors[0]=Editor{};query.clear();filterApplied=false;resultOffset=0;report(L"Search cleared. All item markings removed.");syncMarks();}
 if(id>=20&&id<26){tabs[current].color=id-20;report(saveTab(current)?L"Page color saved.":L"Could not save page color.");}
 if(id>=100&&id<110){int row=resultOffset+id-100;if(row<int(results.size())){selectedResult=results[row];requestPage(results[row].page);syncMarks();}}
 rebuild();
}
inline bool command(const PluginContext* c,const char* name){
 int id=0;
 if(!std::strcmp(name,"ResetSearch"))id=34;
 if(!std::strcmp(name,"Blur"))id=33;
 if(!std::strcmp(name,"Search"))id=30;
 if(!std::strcmp(name,"PageNameInput"))id=31;
 if(!std::strcmp(name,"Page"))id=32;
 const char* names[]={"Close","Save","Favorite","Favorites","Profile","Go","Scan","Refresh","Prev","Next","Clear"};
 for(int i=0;i<11;++i)if(!std::strcmp(name,names[i]))id=i+1;
 if(std::strlen(name)==6&&!std::strncmp(name,"Color",5)&&name[5]>='0'&&name[5]<='5')id=20+name[5]-'0';
 if(std::strlen(name)==4&&!std::strncmp(name,"Row",3)&&name[3]>='0'&&name[3]<='9')id=100+name[3]-'0';
 if(!id)return false;
 threads->runOnUiThread(c,action,reinterpret_cast<void*>(static_cast<intptr_t>(id)));return true;
}
inline void __cdecl update(const PluginContext* c,void*) noexcept {
 LeagueStash::tick();
 pending=false;std::lock_guard lock(mutex);
 struct SyncOnExit {~SyncOnExit(){syncNative();}} sync;
 if(!available||!ready)return;
 if(detectedProfile<0){if(!profileRequestPending.exchange(true)&&threads->runOnGameThread(c,resolveStashProfile,nullptr)!=Threads::Result::Success)profileRequestPending=false;return;}
 if(profile!=detectedProfile){profile=detectedProfile;loaded=false;}
 stashOpen=visible(c);
 if(!stashOpen){stopScan();if(opened)close();return;}
 int previous=current,oldSeason=activeSeason;if(!state(current,total,activeSeason))return;
 if(!loaded||oldSeason!=activeSeason){stopScan();readMetadata();rebuild();}
 auto now=GetTickCount64();
 if(opened){Panels::PanelInfo state{.structSize=Panels::PanelInfoSize};
  opened=panels->getPanelInfo(c,panelHandle,&state)==Panels::Result::Success&&state.presentationState==Panels::PresentationState::Open;

 }
 if(target>=0){int distance=target-current;if(distance){
   if(move(std::clamp(distance,-25,25))<0){stopScan();report(L"Page navigation failed.");return;}
   // Re-read immediately so the next tick does not confuse our own move with a manual click.
   state(current,total,activeSeason);
   editors[1].text=tabs[current].name;editors[1].cursor=editors[1].text.size();editors[1].selected=false;
   editors[2].text=std::to_wstring(current+1);editors[2].cursor=editors[2].text.size();editors[2].selected=false;
   settleUntil=now+500;return;
  }
  target=-1;settleUntil=now+500;
 }
 if(previous!=current){
  editors[1].text=tabs[current].name;editors[1].cursor=editors[1].text.size();editors[1].selected=false;
  editors[2].text=std::to_wstring(current+1);editors[2].cursor=editors[2].text.size();editors[2].selected=false;
  if(scanning&&target<0&&current!=scanPage){stopScan();report(L"Scan stopped after manual navigation.");}
  settleUntil=now+500;
 }
 if(now<settleUntil||now-lastCapture<500)return;
 lastCapture=now;
 bool ok=capture();
 if(scanning){if(!ok){stopScan();return;}
  report(L"Scanned "+std::to_wstring(scanPage+1)+L" / "+std::to_wstring(total)+L" pages.");
  ++scanPage;if(scanPage>=total){stopScan();report(L"Scan complete. Select a result to open its page.");}else target=scanPage;
 }
}
inline void fill(const Overlay::Frame* f,float x,float y,float w,float h,Overlay::Color color){
 Overlay::FilledRectangleRequest r{.structSize=Overlay::FilledRectangleRequestSize,.canvas=f->canvas,.rect={x,y,x+w,y+h},.color=color};draw->drawFilledRectangle(context,&r);
}
inline void outline(const Overlay::Frame* f,float x,float y,float w,float h,Overlay::Color color,float thickness){
 Overlay::RectangleRequest r{.structSize=Overlay::RectangleRequestSize,.canvas=f->canvas,.rect={x,y,x+w,y+h},.color=color,.thickness=thickness};draw->drawRectangle(context,&r);
}
inline void label(const Overlay::Frame* f,float x,float y,const std::wstring& text,float size,Overlay::Color color){
 auto s=utf8(text);Overlay::TextRequest r{.structSize=Overlay::TextRequestSize,.canvas=f->canvas,.position={x,y},.color=color,.textSize=size,.text=s.data(),.textLength=uint32_t(s.size())};
 auto result=draw->drawText(context,&r);if(traceText){char line[160];std::snprintf(line,sizeof(line),"[stash] First text draw: result=%u pos=%.0f,%.0f size=%.1f bytes=%u",unsigned(result),x,y,size,r.textLength);context->LogInfo(line);traceText=false;}static bool logged=false;if(result!=Overlay::Result::Success&&!logged){char line[100];std::snprintf(line,sizeof(line),"[stash] Overlay text failed: %u",unsigned(result));context->LogError(line);logged=true;}
}
inline void __cdecl frame(const PluginContext* c,const Overlay::Frame* f,void*) noexcept {
 if(!f||!available)return;
 auto now=GetTickCount64();if(now-lastQueued>=100&&!pending.exchange(true)){
  lastQueued=now;if(threads->runOnUiThread(c,update,nullptr)!=Threads::Result::Success)pending=false;
 }
 std::lock_guard lock(mutex);
 if(traceFrame){char line[200];std::snprintf(line,sizeof(line),"[stash] Render frame: size=%u viewport=%.0fx%.0f stash=%d menu=%d page=%d",f->structSize,f->screenWidth,f->screenHeight,int(stashOpen),int(opened),current+1);c->LogInfo(line);traceFrame=false;}
 if(!stashOpen||current<0||current>=total)return;

}
inline void __cdecl reset(const PluginContext*,void*) noexcept {
 std::lock_guard lock(mutex);stopScan();detectedProfile=-1;loaded=false;stashOpen=false;known={};for(auto& items:cache)items.clear();query.clear();selectedResult={-1,L""};filterApplied=false;for(auto& e:editors)e=Editor{};if(opened)close();pending=false;
}
inline void __cdecl lifecycle(const PluginContext* c,const Lifecycle::GameplayEvent*,void*) noexcept {threads->runOnUiThread(c,reset,nullptr);}
inline void install(const PluginContext* c){
 context=c;wchar_t path[32768]{};auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);
 if(!n||n>=32768)return;
 std::error_code ec;auto dir=std::filesystem::path(path)/L"AstralWorkshop";std::filesystem::create_directories(dir,ec);if(ec)return;metadata=dir/L"stash-pages.ini";
 const ResourceService* resources{};const LifecycleService* life{};
 if(c->QueryService(&resources)!=ServiceQueryResult::Success||c->QueryService(&draw)!=ServiceQueryResult::Success||c->QueryService(&life)!=ServiceQueryResult::Success)return;
 Resources::ResourceRegistration res{.structSize=Resources::ResourceRegistrationSize,.path="data/global/ui/layouts/astral-workshop/StashOrganizerhd.json",.bytes=OrganizerLayout,.byteCount=sizeof(OrganizerLayout)-1};Resources::RegistrationHandle rh{};
 Panels::PanelRegistration reg{.structSize=Panels::PanelRegistrationSize,.flags=Panels::PanelFlags::CloseOnEscape|Panels::PanelFlags::TextEntry,.localId="StashOrganizer"};
 auto resourceResult=resources->registerResource(c,&res,&rh);
 if(resourceResult!=Resources::Result::Success){char line[120];std::snprintf(line,sizeof(line),"[stash] Organizer resource registration failed: %u",unsigned(resourceResult));c->LogError(line);return;}
 auto panelResult=panels->registerPanel(c,&reg,&panelHandle);
 // The installed legacy PanelService rejects flag bits above 0x03. Retry only
 // Unsupported, keeping the same native panel and native focused text fields.
 if(panelResult==Panels::Result::Unsupported){
  reg.flags=Panels::PanelFlags::CloseOnEscape;
  panelResult=panels->registerPanel(c,&reg,&panelHandle);
  if(panelResult==Panels::Result::Success)c->LogInfo("[stash] Organizer registered using legacy panel flags (TextEntry unavailable).");
 }
 if(panelResult!=Panels::Result::Success){char line[120];std::snprintf(line,sizeof(line),"[stash] Organizer panel registration failed: %u",unsigned(panelResult));c->LogError(line);return;}
 Overlay::CallbackRegistration drawing{.structSize=Overlay::CallbackRegistrationSize,.phase=Overlay::Phase::AfterGameUi,.callback=frame};Overlay::CallbackHandle dh{};
 if(draw->registerFrameCallback(c,&drawing,&dh)!=Overlay::Result::Success)return;
 for(auto kind:{Lifecycle::GameplayEventKind::GameLeft,Lifecycle::GameplayEventKind::LocalPlayerReady}){Lifecycle::GameplayEventListener l{.structSize=Lifecycle::GameplayEventListenerSize,.kind=kind,.callback=lifecycle};Lifecycle::ListenerHandle h{};life->registerGameplayEventListener(c,&l,&h);}
 available=true;c->LogInfo("[stash] Native organizer ready: click Organizer. Visited pages are indexed automatically.");
}
inline void shutdown(){if(textKeyHook){UnhookWindowsHookEx(textKeyHook);textKeyHook=nullptr;}} // Loader owns native panels/resources and discards registered callbacks on unload.
}
