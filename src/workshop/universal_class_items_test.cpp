#include <algorithm>
#include <iterator>
#include <cassert>
#include <cstdio>
namespace Items {struct PropertySpec {unsigned propertyId;int parameter,minimum,maximum;};}
#include "universal_class_items.h"
int main(){
 static_assert(!universalClassItemsEnabled,"Do not enable incomplete staffmod conversion");
 for(unsigned code:universalClassItems){
  assert(isUniversalClassItem(code));
  for(const auto& p:universalClassProperties){
   Items::PropertySpec in{p[0],123,1,3};auto out=universalClassProperty(in,code);
   assert(out.propertyId==in.propertyId&&out.parameter==123&&out.minimum==1&&out.maximum==3);
  }
 }
 assert(!isUniversalClassItem(0));
 puts("PASS: incomplete class conversion remains disabled; item properties unchanged.");
}
