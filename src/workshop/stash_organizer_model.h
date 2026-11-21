#pragma once
#include <string>
#include <vector>
#include <cwctype>
#include <algorithm>
namespace StashOrganizerModel {
inline std::wstring fold(std::wstring s){for(auto& c:s)c=static_cast<wchar_t>(std::towlower(c));return s;}
inline bool matches(const std::wstring& text,const std::wstring& query){
 auto hay=fold(text),q=fold(query);size_t at=0;
 while(at<q.size()){auto end=q.find(L' ',at);if(end==q.npos)end=q.size();
 if(end>at&&hay.find(q.substr(at,end-at))==hay.npos)return false;at=end+1;}return true;
}
struct ResultWheel {
 int remainder=0;
 int steps(int delta){remainder+=delta;int pages=remainder/120;remainder%=120;return pages;}
};
inline bool inResultArea(float x,float y,float screenWidth,float screenHeight){
 if(screenHeight<=0)return false;float scale=screenHeight/2160.0f;
 float left=screenWidth-1220*scale,top=screenHeight/2-580*scale;
 return x>=left+40*scale&&x<left+1160*scale&&y>=top+505*scale&&y<top+985*scale;
}
inline int pageNumber(const std::wstring& s,int total){
 if(s.empty()||s.size()>4)return -1;int n=0;
 for(auto c:s){if(c<L'0'||c>L'9')return -1;n=n*10+c-L'0';}
 return n>=1&&n<=total?n-1:-1;
}
// A user-selected page cancels scanning. There is deliberately no saved return page.
struct PageNavigation {
 bool scanning=false;int target=-1,scanPage=-1;
 void stop(){scanning=false;target=scanPage=-1;}
 void go(int page){stop();target=page;}
 void scan(){scanning=true;scanPage=target=0;}
};
struct Editor {
 std::wstring text;size_t cursor=0;bool selected=false;
 void eraseSelection(){if(selected){text.clear();cursor=0;selected=false;}}
 void insert(wchar_t c,size_t limit,bool digits=false){if(c<32||(digits&&(c<L'0'||c>L'9')))return;eraseSelection();if(text.size()<limit){text.insert(cursor,1,c);++cursor;}}
 void backspace(){if(selected){eraseSelection();return;}if(cursor){text.erase(--cursor,1);}}
 void remove(){if(selected){eraseSelection();return;}if(cursor<text.size())text.erase(cursor,1);}
 void move(int key){selected=false;if(key==0)cursor=0;else if(key==1)cursor=text.size();else if(key==2&&cursor)--cursor;else if(key==3&&cursor<text.size())++cursor;}
};
struct Tab {std::wstring name;unsigned color=0;bool favorite=false;};
struct Found {int page,x,y;std::wstring name;int width=1,height=1;std::wstring searchText;};
}
