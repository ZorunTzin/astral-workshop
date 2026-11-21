#include "stash_organizer_model.h"
#include "stash_navigation.h"
#include <cassert>
#include <iostream>
int main(){using namespace StashOrganizerModel;
 assert(pageNumber(L"1",1000)==0);assert(pageNumber(L"1000",1000)==999);
 for(auto s:{L"",L"0",L"1001",L"-1",L"2x",L" 3",L"99999"})assert(pageNumber(s,1000)==-1);
 assert(matches(L"Tal Rasha set armor",L"TAL armor"));assert(!matches(L"Tal Rasha set armor",L"Tal rune"));
 assert(matches(L"Shako [uap] unique",L"UNIQUE shako"));assert(matches(L"any",L""));
 for(int start:{0,1,499,999})for(int end:{0,1,499,999}){int pos=start,iterations=0;
 while(pos!=end){int delta=end-pos;if(delta>25)delta=25;if(delta< -25)delta=-25;
 int step=stashSteps(pos,1000,delta);assert(step>0);pos+=delta<0?-step:step;assert(++iterations<=40);}
 assert(pos==end);}
 assert(matches(L"Secret of Steel / Heraldic Shield unique runeforged",L"secret steel"));
 Found affixed{0,2,3,L"Windrunner / Ashwood Bow",2,4,L"151% Enhanced Weapon Damage 17% All Resistances"};
 assert(matches(affixed.name+L" "+affixed.searchText,L"enhanced weapon damage"));
 assert(matches(affixed.name+L" "+affixed.searchText,L"all resistances"));
 assert(matches(affixed.name+L" "+affixed.searchText,L"windrunner resistances"));
 assert(!matches(affixed.name+L" "+affixed.searchText,L"faster cast rate"));
 assert(!matches(L"Unrelated sword",L"all resistances"));
 ResultWheel wheel;assert(wheel.steps(60)==0);assert(wheel.steps(60)==1);assert(wheel.steps(-240)==-2);
 assert(inResultArea(2700,1200,3840,2160));assert(!inResultArea(1000,1200,3840,2160));assert(!inResultArea(2700,700,3840,2160));
 assert(inResultArea(1350,600,1920,1080));assert(!inResultArea(1350,350,1920,1080));
 PageNavigation nav;nav.scan();assert(nav.scanning&&nav.target==0);
 nav.go(47);assert(!nav.scanning&&nav.target==47&&nav.scanPage==-1);
 nav.scan();nav.stop();assert(!nav.scanning&&nav.target==-1&&nav.scanPage==-1);
 nav.go(999);assert(nav.target==999&&!nav.scanning);
 Editor e;
 for(auto c:std::wstring(L"Secret of Steel"))e.insert(c,80);
 assert(e.text==L"Secret of Steel");e.backspace();assert(e.text==L"Secret of Stee");e.insert(L'l',80);
 e.move(0);e.remove();assert(e.text==L"ecret of Steel");e.insert(L'S',80);
 e.selected=true;e.insert(L'ä',80);assert(e.text==L"ä");e.backspace();assert(e.text.empty());
 Editor number;number.insert(L'x',4,true);number.insert(L'1',4,true);number.insert(L'0',4,true);assert(number.text==L"10");
 number.selected=true;number.backspace();assert(number.text.empty());
 std::wstring applied=L"unique";Editor draft;draft.insert(L's',80);assert(applied==L"unique");applied=draft.text;assert(applied==L"s");
 std::cout<<"PASS: page boundaries, invalid input, multiword/case-independent search, batched jumps in both directions\n";
}