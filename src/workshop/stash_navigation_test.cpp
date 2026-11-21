#include "stash_navigation.h"
#include <cassert>
#include <initializer_list>
int main(){
    assert(stashStride(false,false)==1);assert(stashStride(false,true)==5);
    assert(stashStride(true,false)==10);assert(stashStride(true,true)==100);
    for(int p=0;p<1000;++p)for(int delta:{-100,-10,-5,-1,1,5,10,100}){
        int moved=stashSteps(p,1000,delta);int target=p+(delta<0?-moved:moved);
        assert(target>=0&&target<1000);
        int expected=p+delta;if(expected<0)expected=0;if(expected>999)expected=999;
        assert(target==expected);
    }
    assert(stashSteps(0,0,100)==0);assert(stashSteps(1000,1000,-100)==0);
}
