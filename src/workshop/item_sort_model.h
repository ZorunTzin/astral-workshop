#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <cwctype>
namespace ItemSortModel {
struct Item {unsigned long long id;int width,height,group,quality;std::wstring name;int x=0,y=0;};
inline int rank(const Item& i){if(i.group<4)return i.group;switch(i.quality){case 7:return 4;case 5:return 5;case 6:return 6;case 8:return 7;case 4:return 8;case 3:return 9;case 2:return 10;default:return 11;}}
inline std::wstring key(std::wstring s){for(auto& c:s)c=std::towlower(c);return s;}
// Work on a copy: callers never receive a partial placement on failure.
inline bool plan(std::vector<Item>& items,int width,int height,bool columnsFirst=false){
 auto sorted=items;std::stable_sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){auto ar=rank(a),br=rank(b);if(ar!=br)return ar<br;auto ak=key(a.name),bk=key(b.name);return ak==bk?a.id<b.id:ak<bk;});
 int area=0;for(auto& i:sorted){if(i.width<1||i.height<1||i.width>width||i.height>height)return false;area+=i.width*i.height;}if(area>width*height)return false;
 std::vector<bool> occupied(width*height);
 auto free=[&](const Item& i,int x,int y){for(int dy=0;dy<i.height;++dy)for(int dx=0;dx<i.width;++dx)if(occupied[(y+dy)*width+x+dx])return false;return true;};
 auto mark=[&](const Item& i,bool value){for(int dy=0;dy<i.height;++dy)for(int dx=0;dx<i.width;++dx)occupied[(i.y+dy)*width+i.x+dx]=value;};
 // Bounded backtracking keeps category/alphabet order but revisits an earlier
 // placement when that placement strands a larger item. Never rotate items.
 unsigned budget=20000;
 auto place=[&](auto&& self,size_t n)->bool {
  if(n==sorted.size())return true;if(!budget)return false;--budget;
  auto& i=sorted[n];
  const int across=width-i.width+1,down=height-i.height+1;
  for(int cell=0;cell<across*down;++cell){
   int x=columnsFirst?cell/down:cell%across,y=columnsFirst?cell%down:cell/across;
   if(!budget)return false;if(!free(i,x,y))continue;
   i.x=x;i.y=y;mark(i,true);
   if(self(self,n+1))return true;mark(i,false);
  }return false;
 };
 if(!place(place,0))return false;items=std::move(sorted);return true;
}
}
