// Reuse the existing native test doubles, then exercise the new feature layer.
#define main BaseAuraTests
#include "aura_test.cpp"
#undef main
Unit ally,pet,enemy;
bool redeemed=false;
int __fastcall RedeemMock(void*,void*,void*,int,int,int){return redeemed?1:0;}
int casts=0,chargeAtCast=0,levelAtCast=0,groundCreates=0;bool failCast=false;
void* __fastcall CreateGround(void*,void* raw) {
    auto* p=static_cast<::Pulse::Params*>(raw);
    assert(p->missile==AuraBuffs::GroundVisualMissile&&p->range==100);
    int init=1,arg=1;
    std::memcpy(&init,static_cast<unsigned char*>(raw)+0x70,4);
    std::memcpy(&arg,static_cast<unsigned char*>(raw)+0x74,4);
    assert(init==0&&arg==0);++groundCreates;return nullptr;
}
int __fastcall CoordX(void* p){return static_cast<Unit*>(p)->x;}
int __fastcall CoordY(void* p){return static_cast<Unit*>(p)->y;}
void* __fastcall RoomOf(void* p){return reinterpret_cast<void*>(static_cast<std::uintptr_t>(static_cast<Unit*>(p)->room));}
void* __fastcall MinionOwner(void* p){return p==&pet?&player:nullptr;}
void* __fastcall NoOwner(void*,void*){return nullptr;}
int __fastcall OwnerType(void*){return 0;}
int __fastcall OwnerId(void* p){return static_cast<Unit*>(static_cast<List*>(p)->owner)->id;}
int __fastcall Cast(void* u,int skill,int level,void*,int){assert(skill==AuraBuffs::NovaSkill);++casts;chargeAtCast=GetUnit(u,AuraBuffs::ChargeStat,0);levelAtCast=level;return failCast?0:1;}
void Place(Unit& u,int type,unsigned id){u.type=type;u.id=id;u.stats[6]=1000*256;u.stats[7]=1000*256;u.stats[9]=1000*256;u.stats[8]=1000*256;void* p=&u;std::memcpy(game+(type?0x2630:0x2230)+8*(id&127),&p,8);}
int main(){
 BaseAuraTests();
 Frame(100);player={};ally={};pet={};enemy={};AuraBuffs::Players={};AuraFeatures::Sources={};
 Place(player,0,10);Place(ally,0,11);Place(pet,1,12);Place(enemy,1,13);
 Curses::X=CoordX;Curses::Y=CoordY;SoulHarvest::Room=RoomOf;
 Curses::MinionOwner=MinionOwner;Curses::Owner=NoOwner;
 Curses::ListOwnerType=OwnerType;Curses::ListOwnerId=OwnerId;Curses::Cast=Cast;
 AuraBuffs::FeaturePulse=AuraFeatures::Pulse;AuraBuffs::FeatureDisable=AuraFeatures::Disable;
 // Difficulty scaling applies to hostile attacks and elemental totals only.
 alignas(8) unsigned char monsterHit[0x180]{};
 for(unsigned diff=0;diff<3;++diff){
   game[0x104]=diff;int value=100*256;
   std::memcpy(monsterHit+0x18,&value,4);std::memcpy(monsterHit+0x20,&value,4);std::memcpy(monsterHit+0x134,&value,4);
   AuraFeatures::MonsterDamage(game,&enemy,monsterHit);
   int result=0;std::memcpy(&result,monsterHit+0x134,4);assert(result==value*(200-25*diff)/100);
 }
 game[0x104]=0;
 const unsigned petClass=pet.cls;pet.cls=GuardianCombat::MonsterClass;
 int value=1000*256;std::memcpy(monsterHit+0x18,&value,4);std::memcpy(monsterHit+0x30,&value,4);std::memcpy(monsterHit+0x134,&value,4);
 AuraFeatures::MonsterDamage(game,&pet,monsterHit);
 for(unsigned offset:{0x18u,0x30u,0x134u}){int result=0;std::memcpy(&result,monsterHit+offset,4);assert(result==100*256);}
 pet.cls=petClass;
 // Efficiency uses division, never subtracts a percentage into negative costs.
 assert(AuraBuffs::Cost(AuraBuffs::Index(98),20,80)==62);
 assert(AuraBuffs::Cost(AuraBuffs::Index(123),20,80)==375);
 player.levels[99]=20;assert(AuraBuffs::Toggle(game,&player,99,20)==1);
 player.stats[AuraBuffs::EfficiencyStat]=80;Frame(105);AuraBuffs::OnFrame(game);
 assert(AuraBuffs::Total(AuraBuffs::Players[0])==62);
 assert(AuraBuffs::Toggle(game,&player,99,20)==1);
 player.stats[AuraBuffs::EfficiencyStat]=0;
 assert(AuraBuffs::Toggle(game,&player,100,20)==0);
 assert(AuraBuffs::Toggle(game,&player,105,20)==0);
 assert(AuraBuffs::Toggle(game,&player,110,20)==0);
 ::Pulse::OriginalCreate=CreateGround;
 // A one-second stationary threshold; players and player-owned pets only.
 player.levels[109]=20;assert(AuraBuffs::Toggle(game,&player,109,20)==1);
 AuraFeatures::Tick(game);Frame(129);AuraFeatures::Tick(game);
 assert(!player.lists.count(AuraBuffs::GroundState));
 Frame(130);AuraFeatures::Tick(game);
 assert(groundCreates==1);
 assert(GetList(&player,AuraBuffs::GroundState)&&GetList(&ally,AuraBuffs::GroundState)&&GetList(&pet,AuraBuffs::GroundState));
 assert(!enemy.lists.count(AuraBuffs::GroundState));
 assert(GetUnit(&player,AuraBuffs::GroundPhysicalStat,0)==60);
 assert(GetUnit(&player,329,0)==60&&GetUnit(&player,357,0)==60);
 assert(GetUnit(&player,74,0)==358);
 assert(player.lists[AuraBuffs::GroundState].expiry==330);
 // Move out. The ground remains at its original coordinates, and the player's
 // existing buff lasts eight seconds without self-stacking or following them.
 player.x=100;ally.room=2;Frame(140);AuraFeatures::Tick(game);
 assert(player.lists[AuraBuffs::GroundState].expiry==330);
 assert(ally.lists[AuraBuffs::GroundState].expiry==330);
 assert(pet.lists[AuraBuffs::GroundState].expiry==340);
 assert(GetUnit(&pet,329,0)==60);
 // Stopping the source leaves its existing recipient buff to expire normally.
 assert(AuraBuffs::Toggle(game,&player,109,20)==1);
 Frame(331);assert(!GetList(&player,AuraBuffs::GroundState));
 // Aura Mastery increases strength, not either passive or reservation.
 player.stats[AuraBuffs::EffectStat]=60;
 assert(AuraFeatures::DamageBonus(20,&player)==96);
 assert(AuraFeatures::RegenBasisPoints(20,&player)==560);
 assert(AuraBuffs::Cost(AuraBuffs::Index(109),20)==337);
 // Retribution counts damaging hits, independent of incoming hit magnitude.
 player.stats[AuraBuffs::EffectStat]=0;player.levels[103]=20;
 assert(AuraBuffs::Toggle(game,&player,103,20)==1);
 alignas(8) unsigned char damage[0x180]{};unsigned short flags=1;int total=100*256;
 auto hit=[&](int value=100*256,unsigned short result=1){
   std::memcpy(damage+4,&result,2);std::memcpy(damage+0x134,&value,4);
   AuraFeatures::IncomingHit(game,&enemy,&player,damage);
 };
 auto* source=AuraFeatures::Track(game,&player);
 assert(AuraFeatures::NovaDamage(1,&player)==75&&AuraFeatures::NovaDamage(20,&player)==550);
 assert(AuraFeatures::NovaRadiusPercent(1,&player)==100&&AuraFeatures::NovaRadiusPercent(20,&player)==157);
 hit(1);hit(1000*256);Frame(405);AuraFeatures::Tick(game);
 assert(casts==0&&source->hits==2);
 hit();hit();assert(source->hits==4&&source->pendingNovas==0);
 hit();assert(source->hits==0&&source->pendingNovas==1);
 Frame(406);AuraFeatures::Tick(game);
 assert(casts==1&&chargeAtCast==550&&levelAtCast==20&&source->pendingNovas==0&&GetUnit(&player,AuraBuffs::ChargeStat,0)==0);
 Frame(481);AuraFeatures::Tick(game);assert(casts==1); // No timed releases.
 hit(100*256,0x11);hit(0);hit(100*256,0);assert(source->hits==3); // Block, zero and armor miss all count.
 source->hits=0;
 AuraFeatures::EmittingNova=true;hit();AuraFeatures::EmittingNova=false;assert(source->hits==0);
 player.stats[AuraBuffs::EffectStat]=60;
 assert(AuraFeatures::NovaDamage(20,&player)==880&&AuraFeatures::NovaRadiusPercent(20,&player)==251);
 for(int i=0;i<10;++i)hit();assert(source->pendingNovas==2&&source->hits==0);
 Frame(482);AuraFeatures::Tick(game);assert(casts==2&&chargeAtCast==880&&source->pendingNovas==1);
 Frame(483);AuraFeatures::Tick(game);assert(casts==3&&source->pendingNovas==0);
 for(int i=0;i<5;++i)hit();failCast=true;Frame(484);AuraFeatures::Tick(game);
 assert(source->pendingNovas==1&&GetUnit(&player,AuraBuffs::ChargeStat,0)==0);
 failCast=false;Frame(485);AuraFeatures::Tick(game);assert(source->pendingNovas==0);
 hit();assert(AuraBuffs::Toggle(game,&player,103,20)==1&&source->hits==0&&source->pendingNovas==0);
 player.stats[AuraBuffs::EffectStat]=0;
 // Toggling back on never retains hits from the previous activation.
 assert(AuraBuffs::Toggle(game,&player,103,20)==1);
 for(int i=0;i<5;++i)hit();player.stats[6]=0;Frame(486);AuraFeatures::Tick(game);
 assert(source->hits==0&&source->pendingNovas==0);player.stats[6]=1000*256;
 assert(AuraBuffs::Toggle(game,&player,103,20)==1);
 flags=1;std::memcpy(damage+4,&flags,2);
 // Physical damage is increased once after resistance; elemental components
 // are already handled by native mastery and are not multiplied a second time.
 AuraFeatures::ApplyGround(game,&player,&ally,20);
 total=150*256;int physical=100*256;std::memcpy(damage+0x134,&total,4);std::memcpy(damage+0x18,&physical,4);
 AuraFeatures::Hit(game,&ally,&enemy,damage);std::memcpy(&total,damage+0x134,4);assert(total==210*256);
 // Legacy equipment auras use the hidden original record, without reservation.
 mercenary.stats[151]=8;const auto before=pulses;
 AuraBuffs::ItemPeriodic(game,&mercenary,5,100,0);
 assert(pulses==before+1&&!mercenary.lists.count(AuraBuffs::ReservationState));
 // Only a native successful corpse redemption earns a stack, with independent expiry.
 Frame(1000);player.stats[AuraBuffs::EffectStat]=0;player.stats[6]=1000*256;
 player.levels[124]=20;AuraBuffs::Players[0].costs[AuraBuffs::Index(124)]=1;
 RedemptionLife::Original=RedeemMock;
 redeemed=false;assert(RedemptionLife::Redeem(game,&player,&enemy,124,20,0)==0);
 assert(!GetList(&player,RedemptionLife::RedemptionLifeState));
 redeemed=true;RedemptionLife::Redeem(game,&player,&enemy,124,20,0);
 assert(player.lists[RedemptionLife::RedemptionLifeState].stats[76]==3);
 Frame(1025);RedemptionLife::Redeem(game,&player,&enemy,124,20,0);
 assert(player.lists[RedemptionLife::RedemptionLifeState].stats[76]==6);
 Frame(1375);RedemptionLife::Tick(game);
 assert(player.lists[RedemptionLife::RedemptionLifeState].stats[76]==3);
 Frame(1400);RedemptionLife::Tick(game);
 assert(!GetList(&player,RedemptionLife::RedemptionLifeState)&&RedemptionLife::Players[0].total==0);
 // Level-one bonuses are nonzero, and Salvation also supplies poison maximum.
 Frame(1401);RedemptionLife::Redeem(game,&player,&enemy,124,1,0);
 assert(player.lists[RedemptionLife::RedemptionLifeState].stats[76]==1);
 const auto salvation=AuraBuffs::Index(125);
 Curses::Curse salvationBuff{&player,&player,125,1,100,-1,0,AuraBuffs::Definitions[salvation].state,nullptr};
 Apply(&salvationBuff);AuraFeatures::AfterPulse(game,&player,salvation,1);
 assert(player.lists[AuraBuffs::Definitions[salvation].state].stats[46]==1);
 std::puts("PASS: ground visual uses complete zeroed native missile parameters, efficiency recalculation, passives/summon exclusion, stationary threshold, allied players/pets, eight-second expiry, no stacking, mastery, physical hit bonus, exact fifth-hit Retribution, level/effect damage and radius, queued releases, cast retry, death/toggle/reentrancy and blocked-hit guards and original equipment auras.");
}
