void __cdecl trialSealTooltip(const PluginContext* ctx,SharedEvents::ItemTooltipEvent* e,void*) noexcept {
 if(!e||!e->text||!e->capacity||!items||!readItemStat)return;
 Items::ItemInfo info{.structSize=Items::ItemInfoSize};if(items->getItemInfo(ctx,e->item,&info)!=Items::Result::Success)return;
 int stage=0;for(int i=1;i<=5;++i){char code[4]={'W','a',char('0'+i),0};if(info.code==Items::MakeItemCode(code))stage=i;}if(!stage)return;
 auto core=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"D2RCore.dll"));const unsigned char guard[]={0x44,0x89,0xf9,0x31,0xd2,0xff,0x15,0x94,0x34,0x2e,0x00};
 if(!core||std::memcmp(core+0x41d309,guard,sizeof(guard)))return;
 auto lookup=*reinterpret_cast<void*(__fastcall**)(unsigned,unsigned)>(core+0x7007a8);if(!lookup)return;
 auto* u=static_cast<unsigned*>(lookup(info.runtimeId,4));if(!u||u[0]!=4||u[1]!=info.classId||u[2]!=info.runtimeId)return;
 TrialCraft::Seal seal;for(int i=0;i<TrialCraft::FieldCount;++i)seal.fields[i]=readItemStat(u,TrialCraft::FirstStat+i,0);if(!seal.valid())return;
 std::string text=TrialCraft::tooltip(seal,stage,(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0);
 if(text.size()>=e->capacity)return;std::memcpy(e->text,text.c_str(),text.size()+1);e->length=unsigned(text.size());
}
