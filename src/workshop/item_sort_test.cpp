#include "item_sort_model.h"
#include <cassert>
#include <iostream>
using namespace ItemSortModel;
int main(){
 std::vector<Item> v={{1,2,3,4,5,L"Set"},{2,1,1,4,7,L"Zebra"},{3,1,1,4,7,L"alpha"},{4,1,2,0,2,L"Tome"},{5,1,3,1,7,L"Charm"},{6,2,2,2,2,L"Cube"},{7,1,1,3,2,L"Rune"}};
 assert(plan(v,10,8));assert(v[0].id==4&&v[1].id==5&&v[2].id==6&&v[3].id==7&&v[4].id==3&&v[5].id==2&&v[6].id==1);
 std::vector<bool> cells(80);for(auto& i:v)for(int y=i.y;y<i.y+i.height;++y)for(int x=i.x;x<i.x+i.width;++x){assert(x<10&&y<8&&!cells[y*10+x]);cells[y*10+x]=true;}
 auto once=v;assert(plan(v,10,8));for(int i=0;i<int(v.size());++i)assert(v[i].id==once[i].id&&v[i].x==once[i].x&&v[i].y==once[i].y);
 std::vector<Item> full;for(int i=0;i<208;++i)full.push_back({unsigned(i+1),1,1,4,7,L"Item"});assert(plan(full,16,13));full.push_back({300,1,1,4,7,L"Item"});auto old=full;assert(!plan(full,16,13));for(int i=0;i<int(full.size());++i)assert(full[i].id==old[i].id&&full[i].x==old[i].x&&full[i].y==old[i].y);
 // First-fit strands the 2x2 despite ample space; revisit the 2x3 placement.
 std::vector<Item> fragmented={{1,1,1,0,2,L"A"},{2,2,3,1,2,L"B"},{3,2,2,2,2,L"C"}};
 assert(plan(fragmented,4,4));std::vector<bool> packed(16);for(auto& i:fragmented)for(int y=i.y;y<i.y+i.height;++y)for(int x=i.x;x<i.x+i.width;++x){assert(!packed[y*4+x]);packed[y*4+x]=true;}
 std::vector<Item> axis;for(int i=0;i<6;++i)axis.push_back({unsigned(i+1),1,1,4,7,std::to_wstring(i)});
 auto rows=axis;assert(plan(rows,3,2));assert(plan(axis,3,2,true));
 for(int i=0;i<6;++i){assert(rows[i].x==i%3&&rows[i].y==i/3);assert(axis[i].x==i/2&&axis[i].y==i%2);}
 assert(plan(axis,3,2,false));for(int i=0;i<6;++i)assert(axis[i].x==rows[i].x&&axis[i].y==rows[i].y);
 auto vertical=fragmented;assert(plan(vertical,4,4,true));
 std::vector<Item> bad={{1,17,1,0,2,L"Wide"}};assert(!plan(bad,16,13));
 std::cout<<"PASS: category priority, case-insensitive alphabet, no overlap, idempotence, full page and failure preservation\n";
}
