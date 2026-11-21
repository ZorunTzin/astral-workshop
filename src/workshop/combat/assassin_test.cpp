#define main OriginalAuraTest
#include "aura_test.cpp"
#undef main
static int phase=0;
static void* selected=nullptr;
static int attackObject,targetObject,previousObject,damageObject;
int main(){
 OriginalAuraTest();
 // Runtime must consume the same evaluated total percentage as the sheet,
 // once, for both physical and elemental channels (including splash hits).
 auto previousEvaluate=Curses::Evaluate;
 Curses::Evaluate=[](unsigned char,void*,int formula,int skill,int level)->int{assert(formula==275&&skill==275&&level==20);return 1120;};
 Unit missileUnit;::Pulse::Params shot{};shot.owner=&player;shot.missile=Assassin::HitMissile;shot.level=20;
 AssassinPreview::Hit hit{};hit.skill=AssassinPreview::Skill::Omnislash;hit.weaponPercent=700;
 Assassin::ActiveHit=&hit;
 missileUnit.stats[21]=100*256;missileUnit.stats[22]=200*256;missileUnit.stats[48]=10*256;
 Assassin::Projectile(game,&shot,&missileUnit);
 assert(missileUnit.stats[21]==1120*256&&missileUnit.stats[22]==2240*256&&missileUnit.stats[48]==112*256);
 Assassin::ActiveHit=nullptr;Curses::Evaluate=previousEvaluate;
 // Stationary sentries may have zero walk-target coordinates, while their
 // actual position remains valid. The host must use the fixed-point position.
 alignas(8) unsigned char rawUnit[0x110]{},rawPath[32]{};
 void* rawPathPointer=rawPath;std::memcpy(rawUnit+0x38,&rawPathPointer,8);
 unsigned x=1234u<<16,y=5678u<<16;std::memcpy(rawPath,&x,4);std::memcpy(rawPath+4,&y,4);
 Assassin::NativePositions=true;
 assert(Assassin::Coordinate(rawUnit,false)==1234&&Assassin::Coordinate(rawUnit,true)==5678);
 Assassin::NativePositions=false;
 // Omnislash does not depend on a used-skill animation node. Resolve the
 // native packet exactly once and restore the player's previous target.
 selected=&previousObject;phase=0;
 Assassin::GetTarget=[](void*,void*)->void*{return selected;};
 Assassin::SetTarget=[](void*,void* target){selected=target;};
 Assassin::Allocate=[](void*,void*,void* target,int skill,int level){assert(phase++==0&&selected==target&&skill==Assassin::Finisher&&level==20);};
 Assassin::DamageFor=[](void*,void*)->void*{assert(phase++==1);return &damageObject;};
 Assassin::Release=[](void*,void*,int skill,void* packet){assert(phase++==2&&skill==Assassin::Finisher&&packet==&damageObject);};
 Assassin::ResolveMelee=[](void*,void*,void*,int range,unsigned char durability)->int{assert(phase++==3&&range==6&&durability==1);return 1;};
 assert(Assassin::Finish(game,&attackObject,&targetObject,20)==1&&phase==4&&selected==&previousObject);
 assert(Assassin::HastedDeadline(100,120,100)==110);
 assert(Assassin::HastedDeadline(100,115,65)==109);
 assert(Assassin::HastedDeadline(100,120,0)==120);
 assert(Assassin::HastedDeadline(100,101,500)==101);
 assert(Assassin::HastedDeadline(100,99,65)==99);
 std::puts("PASS: Omnislash native allocation, charge release, one resolved hit and target restoration; bounded Overclock AI deadlines.");
 Curses::X=[](void* u)->int{return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u)->int{return static_cast<Unit*>(u)->y;};
 // Every Assassin buff shares the existing budget, renews, toggles off and
 // releases reservation on death. This exercises the real aura integration.
 AuraBuffs::Players={};player.lists.clear();player.stats[6]=1000;player.stats[9]=500*256;player.stats[8]=500*256;
 AuraBuffs::FeaturePulse=Assassin::BuffPulse;AuraBuffs::FeatureDisable=Assassin::Disable;
 for(int sid:{263,278,276,257,277}){
  player.levels[sid]=20;
  assert(AuraBuffs::Cost(AuraBuffs::Index(sid),20)==75);
  assert(AuraBuffs::Cost(AuraBuffs::Index(sid),20,100)==37);
  assert(AuraBuffs::Toggle(game,&player,sid,20)==1);
 }
 assert(AuraBuffs::Reservation(game,&player)==375*256);
 Frame(200);AuraBuffs::OnFrame(game);
 for(int sid:{263,278,276,257,277})assert(GetList(&player,AuraBuffs::Definitions[AuraBuffs::Index(sid)].state));
 assert(AuraBuffs::Toggle(game,&player,263,20)==1);
 assert(AuraBuffs::Reservation(game,&player)==300*256);
 player.stats[6]=0;Frame(205);AuraBuffs::OnFrame(game);
 assert(AuraBuffs::Reservation(game,&player)==0);
 std::puts("PASS: five Assassin reservations coexist, use efficiency, survive renewal and clean up on toggle/death.");
}
