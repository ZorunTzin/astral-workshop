#pragma once
#include "universal_class_items.inc"
inline constexpr bool universalClassItemsEnabled=false; // Native staffmod conversion still pending validation.
inline bool isUniversalClassItem(unsigned code) {
 return std::find(std::begin(universalClassItems),std::end(universalClassItems),code)!=std::end(universalClassItems);
}
inline Items::PropertySpec universalClassProperty(Items::PropertySpec effect,unsigned code) {
 if(universalClassItemsEnabled && isUniversalClassItem(code))for(const auto& mapping:universalClassProperties)
  if(effect.propertyId==mapping[0]){effect.propertyId=mapping[1];break;}
 return effect;
}
