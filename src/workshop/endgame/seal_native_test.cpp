#include "../passives.cpp"
#include <cassert>
static TrialCraft::Seal saved;
static unsigned __fastcall code(void*){return Items::MakeItemCode("Wa1");}
static int __fastcall stat(void*,int id,unsigned){return saved.fields.at(id-TrialCraft::FirstStat);}
static void __fastcall apply(bool,int,void*,void* item,void*,int,int,int,const Items::PropertySpec* p,int,unsigned,void*){
 auto data=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(item)+0x10);
 assert(!(*reinterpret_cast<unsigned*>(data+0x18)&(1u<<21)));
 for(int i=0;i<8;++i)if(p->propertyId==TrialCraft::Properties[i])saved.fields[i]+=p->minimum;
}
int main(){
 alignas(8) unsigned char item[64]{},data[64]{};
 *reinterpret_cast<unsigned*>(item)=4;
 *reinterpret_cast<unsigned char**>(item+0x10)=data;
 auto& flags=*reinterpret_cast<unsigned*>(data+0x18);
 flags=(1u<<21)|0x10|0x400;
 affixCode=code;readItemStat=stat;originalApplyProperty=apply;
 Items::PropertySpec p{TrialCraft::Dispatch,0,1,1};
 applyForgeProperty(true,7,nullptr,item,&p,0,0,0,&p,0,0x40,nullptr);
 assert(saved.valid()&&saved.grade()==1&&saved.count()>0);
 assert(flags==(0x10|0x400));
 auto text=TrialCraft::tooltip(saved,1);assert(text.find("Magic Trial Seal")!=std::string::npos);
 p.parameter=1;applyForgeProperty(true,7,nullptr,item,&p,0,0,0,&p,0,0x40,nullptr);
 assert(saved.valid()&&saved.grade()==2&&saved.count()>=3);
 p.parameter=4;applyForgeProperty(true,7,nullptr,item,&p,0,0,0,&p,0,0x40,nullptr);
 assert(saved.valid()&&saved.count()==0&&flags==(0x10|0x400));
 puts("PASS: native craft dispatch clears legacy compact flag before stat application, preserves other flags, updates and clears exact stats, renders modifiers.");
}
