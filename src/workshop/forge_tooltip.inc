// Rune-word identity shares the existing marker/floor save fields. The widths
// and IDs never change, so older characters and their forged items still load.
const ForgeWord* forgeWord(int marker,int value){
    if(marker<2||marker>3||value<0||value>127)return nullptr;
    const auto index=static_cast<unsigned>((marker-2)*128+value);
    return index<std::size(forgeWords)?&forgeWords[index]:nullptr;
}
int forgeRequiredLevel(int marker,int value){
    if(const auto* word=forgeWord(marker,value))return word->required;
    return marker==1?std::clamp(value,0,127):0;
}
// Native requirement calculation already receives a live item on the tooltip's
// UI thread. Copy values there; never retain or later dereference that pointer.
// The listener checks the public runtime ID, a short lifetime and one-shot use.
struct ForgeTooltipSnapshot { unsigned runtimeId=0;int marker=0,value=0;ULONGLONG time=0; };
thread_local ForgeTooltipSnapshot forgeTooltipSnapshot;
// The deployed ABI-1 ItemService ends before getItemDetails. Its own getItemInfo
// uses this client unit lookup. Reuse it only after public handle validation,
// on the tooltip thread, with a byte guard and full identity checks.
bool readLegacyForge(const Items::ItemInfo& info,int& marker,int& value){
    auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));
    if(!core||!readItemStat)return false;
    const unsigned char guard[]={0x44,0x89,0xf9,0x31,0xd2,0xff,0x15,0x94,0x34,0x2e,0x00};
    if(std::memcmp(core+0x41d309,guard,sizeof(guard)))return false;
    auto lookup=*reinterpret_cast<void*(__fastcall**)(unsigned,unsigned)>(core+0x7007a8);
    if(!lookup)return false;
    auto* unit=static_cast<unsigned*>(lookup(info.runtimeId,4));
    if(!unit||unit[0]!=4||unit[1]!=info.classId||unit[2]!=info.runtimeId)return false;
    marker=readItemStat(unit,forgeMarkerStat,0);value=readItemStat(unit,forgeLevelStat,0);
    return true;
}
void __cdecl forgeTooltip(const PluginContext* ctx,SharedEvents::ItemTooltipEvent* event,void*) noexcept {
    if(!event||!event->text||!event->capacity||!items)return;
    Items::ItemInfo info{.structSize=Items::ItemInfoSize};
    const auto snapshot=forgeTooltipSnapshot;forgeTooltipSnapshot={};
    if(items->getItemInfo(ctx,event->item,&info)!=Items::Result::Success)return;
    int marker=0,value=0;
    // Equipped-item tooltips may skip the native requirement calculation.
    // Read the hovered item's own saved fields instead of relying on its order.
    if(HasItemServiceField(items,ItemServiceDetailsFieldEnd)&&items->getItemDetails){
        Items::ItemDetails details{.structSize=Items::ItemDetailsSize};
        std::array<Items::ItemStatValue,1024> stats{};
        if(items->getItemDetails(ctx,event->item,&details,stats.data(),stats.size())==Items::Result::Success){
            for(unsigned i=0;i<std::min<unsigned>(details.statCount,stats.size());i++){
                if(stats[i].layer)continue;
                if(stats[i].statId==forgeMarkerStat)marker=stats[i].value;
                if(stats[i].statId==forgeLevelStat)value=stats[i].value;
            }
        }
    }
    if(!marker)readLegacyForge(info,marker,value);
    if(!marker&&snapshot.marker&&GetTickCount64()-snapshot.time<=100&&info.runtimeId==snapshot.runtimeId){marker=snapshot.marker;value=snapshot.value;}
    const auto* word=forgeWord(marker,value);
    if(!word&&marker!=1)return;
    char label[160];
    if(word)std::snprintf(label,sizeof(label),"\xee\x81\xbe" "4Runeforged (%s)\xee\x81\xbe" "0",word->name);
    else std::snprintf(label,sizeof(label),"\xee\x81\xbe" "4Runeforged\xee\x81\xbe" "0"); // Old items had no saved word identity.
    const auto length=std::strlen(label);
    if(length>=event->capacity)return;
    std::memcpy(event->text,label,length+1);event->length=static_cast<unsigned>(length);
}
