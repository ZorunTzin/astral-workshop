#define main PreviousAuraTests
#include "../combat/aura_test.cpp"
#undef main
#include "caption.h"
extern "C" void* CaptionTestInvoke(void*,void*);
extern "C" void CaptionTestResume();
int main(){
 using namespace MonsterModifiers;
 unsigned unit[16]={1,58,9001};alignas(8) unsigned char panel[64]{};
 const char* stock="Life 123 / Resistances / Fire Enchanted";
 std::memcpy(panel+0x28,&stock,8);
 MonsterCaption::Publish(unit,Bit(Poison)|Bit(Gravity)|Bit(Totems));
 WorkshopMonsterCaptionResume=reinterpret_cast<void*>(CaptionTestResume);
 assert(CaptionTestInvoke(unit,panel)==panel);
 const char* result=nullptr;std::memcpy(&result,panel+0x28,8);
 assert(std::strcmp(result,"Life 123 / Resistances / Fire Enchanted\nPlaguebearer / Gravity Core\nTotemic")==0);
 // Rendering must never borrow another class's cached ID or an old session.
 unit[1]=59;assert(MonsterCaption::Read(unit)==0);unit[1]=58;
 MonsterCaption::Clear();assert(MonsterCaption::Read(unit)==0);
 std::memcpy(panel+0x28,&stock,8);CaptionTestInvoke(unit,panel);std::memcpy(&result,panel+0x28,8);assert(result==stock);
 puts("PASS: real x64 continuation, stack alignment, panel return register, native text preserved, correct appended names, class and session isolation.");
}
