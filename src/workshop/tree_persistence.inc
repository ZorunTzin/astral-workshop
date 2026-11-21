// Store drafts by character identity, not transient loader PlayerHandles.
// Apply remains explicit: reopening restores the plan without granting items.
std::string editingKey;
Build lastDraft{};
bool lastDraftValid=false;
std::string characterKey(){
    auto* unit=static_cast<unsigned char*>(localPlayerUnit());
    if(!unit||*reinterpret_cast<unsigned*>(unit)!=0)return {};
    auto exe=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    const unsigned char guard[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9};
    if(std::memcmp(exe+0x34b240,guard,sizeof(guard)))return {};
    auto data=reinterpret_cast<void*(__fastcall*)(void*)>(exe+0x34b240)(unit);
    if(!data)return {};
    // PlayerData starts with the fixed 16-byte character name. Hex encoding
    // prevents names from ever becoming paths outside the draft directory.
    const auto* name=static_cast<const unsigned char*>(data);
    std::string key=std::to_string(*reinterpret_cast<unsigned*>(unit+4))+"-";
    const char hex[]="0123456789abcdef";
    unsigned n=0;for(;n<16&&name[n];n++){key+=hex[name[n]>>4];key+=hex[name[n]&15];}
    return n&&n<16?key:std::string{};
}
bool loadDraft(const std::string& key,Build& result){
    Receipt saved{};
    std::ifstream input(ledger/("draft-"+key),std::ios::binary);
    if(!input.read(reinterpret_cast<char*>(&saved),sizeof(saved))||saved.magic!=0x34504c41)return false;
    prune(saved.allocation);
    if(!valid(saved.allocation,false))return false;
    result=saved.allocation;return true;
}
void persistDraft(const PluginContext* ctx){
    if(editingKey.empty()||!valid(selected,false))return;
    if(lastDraftValid&&std::memcmp(&lastDraft,&selected,sizeof(Build))==0)return;
    const auto path=ledger/("draft-"+editingKey);auto temp=path;temp+=L".tmp";
    Receipt saved{};saved.allocation=selected;
    std::ofstream output(temp,std::ios::binary|std::ios::trunc);
    output.write(reinterpret_cast<const char*>(&saved),sizeof(saved));output.close();
    if(!output||!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        if(ctx)ctx->LogError("Could not persist Astral draft");return;
    }
    lastDraft=selected;lastDraftValid=true;
}
