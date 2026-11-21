// Audit the real compiled table, not the editable *Id annotations. D2's linker
// removes Expansion separators and assigns consecutive numeric row IDs.
void __cdecl auditProperties(const PluginContext* ctx,const Lifecycle::DataTablesLoadedEvent*,void*) noexcept {
    const DataTableService* tables{};if(ctx->QueryService(&tables)!=ServiceQueryResult::Success)return;
    DataTables::TableView view{.structSize=DataTables::TableViewSize};
    if(tables->getTable(ctx,DataTables::Bank::Rotw,DataTables::TableId::Properties,&view)!=DataTables::Result::Success||view.rowCount<470)return;
    auto value=[&](unsigned row,unsigned offset){unsigned short stat=0;std::memcpy(&stat,static_cast<const unsigned char*>(view.rows)+row*view.rowSize+offset,2);return stat;};
    unsigned offset=0,candidates=0;
    for(unsigned i=0;i+2<=view.rowSize;i+=2)if(value(7,i)==0&&value(8,i)==2&&value(11,i)==9){offset=i;++candidates;}
    bool valid=candidates==1;
    for(auto& expected:propertyAudit)valid=valid&&expected.property<view.rowCount&&value(expected.property,offset)==expected.stat;
    if(valid)ctx->LogInfo("Compiled property audit PASS: every custom property resolves to its actual expected stat (including Mana and maximum summons).");
    else ctx->LogError("Compiled property audit FAILED: property/stat linker mismatch.");
}
void installPropertyAudit(const PluginContext* ctx){
    const LifecycleService* life{};if(ctx->QueryService(&life)!=ServiceQueryResult::Success)return;
    Lifecycle::DataTablesLoadedListener listener{.structSize=Lifecycle::DataTablesLoadedListenerSize,.callback=auditProperties};
    Lifecycle::ListenerHandle handle{};life->registerDataTablesLoadedListener(ctx,&listener,&handle);
}
