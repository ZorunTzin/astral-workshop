#include "model.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace MythicalModel;
 assert(!Bonded(0)&&!Bonded(1)&&!Bonded(64)&&Bonded(3)&&Bonded(65));
 assert(ElementExtra(1000,25,0,0,1000)==250);
 assert(ElementExtra(1000,25,50,0,1000)==125);
 assert(ElementExtra(1000,100,100,0,1000)==0);
 assert(ElementExtra(1000,100,-100,0,1000)==2000);
 assert(ElementExtra(0x3fffffff,100,-100,0,0x3fffffff)==0);
 unsigned triple=0,five=0;
 for(unsigned n=0;n<100;++n){triple+=Multiplier(true,false,false,n)==3;five+=Multiplier(false,false,true,n)==5;assert(Multiplier(false,false,false,n)==1);}
 assert(triple==10&&five==3);
 std::cout<<"Mythical powers: bond combinations, resistance, overflow bounds and proc probabilities passed.\n";
}
