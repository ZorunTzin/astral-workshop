"""Port the tested curse overhaul into Reimagined without importing FMMP balance.

Only ten curse skills, seven private helper skills, eight private missiles,
twenty-four private stats and two private states are transferred. Every private
numeric ID is allocated after the destination's records and rebound in C++.
"""
import csv
import io
import json
import re
from pathlib import Path
from shutil import copy2

HERE=Path(__file__).resolve().parent
CURSES={66,71,72,76,77,81,82,86,87,91}
SOURCE=HERE/'upstream/data/global/excel'

def read(path):
    with path.open(encoding='utf-8-sig',newline='') as f:
        reader=csv.DictReader(f,delimiter='\t')
        return reader.fieldnames,list(reader)

def write(path,header,rows):
    out=io.StringIO()
    writer=csv.DictWriter(out,fieldnames=header,delimiter='\t',lineterminator='\r\n')
    writer.writeheader();writer.writerows(rows)
    path.write_text(out.getvalue(),encoding='utf8',newline='')

def bind_sources(bindings):
    translation={k:v for group in bindings.values() for k,v in group.items()}
    source_root=HERE/'original'
    generated=HERE/'src';generated.mkdir(exist_ok=True)
    pattern=re.compile(r'\b('+ '|'.join(map(str,translation))+r')\b')
    for p in source_root.iterdir():
        if p.suffix not in ('.h','.cpp','.rc'):continue
        text=p.read_text(encoding='utf-8-sig')
        protected={}
        def protect(match):
            token=f'WORKSHOP_LITERAL_{len(protected)}'
            protected[token]=match[0]
            return token
        text=re.sub(r'\b(?:Frame|DecrepStrength)\(\d+\)',protect,text)
        text=pattern.sub(lambda m:str(translation[int(m[0])]),text)
        for token,value in sorted(protected.items(),key=lambda pair:len(pair[0]),reverse=True):text=text.replace(token,value)
        text=text.replace('FMMP-Experimental-R67','ReimaginedWorkshop')
        text=text.replace('0.5.22','0.19.0')
        text=text.replace('D2RL::PluginFlags::Server |','D2RL::PluginFlags::Shared |')
        text=text.replace('"fmmp-plague-trace"','"workshop-curses"').replace('"Frankenstein Curse Overhaul"','"Workshop Curses and Aura Buffs"')
        if p.name=='virus.cpp':
            text=text.replace("    if (skill == 70)\n", "    if (Sorceress::Event(game,event,source,target,skill,level)) return 1;\n    if (skill == 70)\n",1)
            text=text.replace('#include "soul_harvest.h"','#include "soul_harvest.h"\n#include "../aura_buffs.h"')
            text=text.replace('reinterpret_cast<void*>(&SoulHarvest::OnFrame)','reinterpret_cast<void*>(&AuraBuffs::Frame)')
            text=text.replace('    if (!NativeProbe::Register(c))','    if (!AuraBuffs::Install(c)) return false;\n    if (!NativeProbe::Register(c))')
        if p.name=='curses.h':
            text=text.replace('inline DamageFn OriginalHitLeech = nullptr;', 'inline DamageFn OriginalHitLeech = nullptr;\ninline DamageFn AdditionalHitEffects = nullptr;\ninline DamageFn AdditionalIncomingHit = nullptr;\ninline DamageFn AdditionalAfterDamage = nullptr;\ninline void (*AdditionalWarCry)(void*) = nullptr;\ninline bool (*AdditionalIgnoreDamage)(void*,void*,void*,void*) = nullptr;\ninline bool (*AdditionalMissileContext)(void*) noexcept = nullptr;\ninline bool (*AdditionalMissileFilter)(void*,void*) noexcept = nullptr;\ninline void (*AdditionalMissileCreated)(void*,void*,void*) noexcept = nullptr;\ninline int ExtraInstantMissile2 = -1;\ninline int ExtraInstantMissile = -1;\ninline int ExtraStackSkill = -1;')
            text=text.replace('inline void __fastcall OnDamage(void* game, void* attacker, void* defender, void* damage) noexcept {', 'inline void __fastcall OnDamage(void* game, void* attacker, void* defender, void* damage) noexcept {\n    if (AdditionalIgnoreDamage&&AdditionalIgnoreDamage(game,attacker,defender,damage))return;\n    if (AdditionalIncomingHit) AdditionalIncomingHit(game, attacker, defender, damage);')
            text=text.replace('    OriginalHitLeech(game, attacker, defender, damage);', '    if (AdditionalHitEffects) AdditionalHitEffects(game, attacker, defender, damage);\n    OriginalHitLeech(game, attacker, defender, damage);')
        if p.name=='curses.h':
            text=text.replace('    OriginalDamage(game, attacker, defender, damage);', '    OriginalDamage(game, attacker, defender, damage);\n    if (AdditionalAfterDamage) AdditionalAfterDamage(game, attacker, defender, damage);')
            text=text.replace('    auto* c = static_cast<Curse*>(raw);', '    auto* c = static_cast<Curse*>(raw);\n    if(c && c->skill==146 && AdditionalWarCry){void* result=OriginalApply(raw);AdditionalWarCry(raw);return result;}')
        if p.name=='curses.h':
            text=text.replace('inline int MissileTick(void* game,', 'inline void (*BeforeMissileTick)(void*,void*) noexcept = nullptr;\ninline void (*AfterMissileTick)(void*,unsigned,int) noexcept = nullptr;\ninline int MissileTick(void* game,')
            text=text.replace('    const int result = original(game, missile);', '      const unsigned trackedId = missile ? static_cast<unsigned*>(missile)[2] : 0xffffffffU;\n      if(BeforeMissileTick)BeforeMissileTick(game,missile);\n      const int result = original(game, missile);\n      if(AfterMissileTick)AfterMissileTick(game,trackedId,result);')
        if p.name=='curses.h':
            text=text.replace('inline int __fastcall OnMissileTick(void* game, void* missile) noexcept {','inline int __fastcall OnMissileTick(void* game, void* missile) noexcept {\n    if(missile && int(static_cast<unsigned*>(missile)[1])==ExtraInstantMissile || missile && int(static_cast<unsigned*>(missile)[1])==ExtraInstantMissile2)return 2;')
            text=text.replace('    if (!p)\n        return true;', '    if (!p)\n        return true;\n    if(p->skill==137){p->flags|=0x8000U;p->range=18+std::clamp(p->level,1,1000);return true;}')
        if p.name=='curses.h':
            text=text.replace('    auto* c = static_cast<Curse*>(raw);', '    auto* c = static_cast<Curse*>(raw);\n    if(c && c->skill==ExtraStackSkill && Monster(c->target)){Context context{c->target,c->state,true,Active};Active=&context;void* result=OriginalApply(raw);Active=context.previous;return result;}')
        if p.name=='curses.h':
            text=text.replace('    const bool own = ', '    const bool own = (AdditionalMissileContext&&AdditionalMissileContext(missile)) || ')
            text=text.replace('inline bool FilterFire(void* game, void* raw) noexcept {', 'inline bool FilterFire(void* game, void* raw) noexcept {\n    if(AdditionalMissileFilter&&!AdditionalMissileFilter(game,raw))return false;')
            text=text.replace('inline void TagFire(void*, void* raw, void* missile) noexcept {', 'inline void TagFire(void* game, void* raw, void* missile) noexcept {\n    if(AdditionalMissileCreated)AdditionalMissileCreated(game,raw,missile);')
        if p.name=='soul_harvest.h':
            text=text.replace('inline void ApplyStats(void* game,', 'inline int (*UnreservedMana)(void*,void*) = nullptr;\n\ninline void ApplyStats(void* game,')
            text=text.replace('const int base = std::max(0, Ramp::GetUnit(player, stat, 0));','const int base = std::max(0, stat==9 && UnreservedMana ? UnreservedMana(game,player) : Ramp::GetUnit(player,stat,0));')
        (generated/p.name).write_text(text,encoding='utf8')
def install(root):
    root=Path(root)
    bindings={}
    assets=[]
    for directory in (root/'data/global/excel',root/'data/global/excel/base'):
        current={};original={};headers={}
        for table,key,ids in [('skills','skill',range(442,449)),('missiles','Missile',range(749,757)),
                ('itemstatcost','Stat',range(402,426)),('states','state',range(775,777))]:
            header,rows=read(directory/(table+'.txt'))
            _,source=read(SOURCE/(table+'.txt'))
            idcol=next(k for k in header if k.lower()=='*id')
            source_id=next(k for k in source[0] if k.lower()=='*id')
            start=max(int(r[idcol]) for r in rows if r[idcol].isdigit())+1
            mapping={i:start+n for n,i in enumerate(ids)}
            if table in bindings:assert bindings[table]==mapping
            bindings[table]=mapping
            original[table]=[r.copy() for r in rows]
            for r in source:
                if r[source_id].isdigit() and int(r[source_id]) in mapping:
                    new={k:r.get(k,'') for k in header}
                    new[idcol]=str(mapping[int(r[source_id])])
                    assert not any(x[key]==new[key] for x in rows)
                    rows.append(new)
            headers[table]=header;current[table]=rows
        skills=current['skills'];_,source_skills=read(SOURCE/'skills.txt')
        by_source={int(r['*Id']):r for r in source_skills if r['*Id'].isdigit()}
        desc_mapping={}
        for i,row in enumerate(skills):
            if row['*Id'].isdigit() and int(row['*Id']) in CURSES:
                replacement={k:by_source[int(row['*Id'])].get(k,'') for k in headers['skills']}
                desc_mapping[row['skilldesc']]=replacement['skilldesc']
                # Reimagined names skill 81 Corpse Field. Keep its canonical
                # name and description key so other Reimagined formulas resolve.
                replacement['skill']=row['skill']
                replacement['skilldesc']=row['skilldesc']
                skills[i]=replacement
        for row in skills:
            if row['*Id'].isdigit() and (int(row['*Id']) in CURSES or int(row['*Id']) in bindings['skills'].values()):
                for key,value in row.items():
                    row[key]=value.replace("skill('Confuse'","skill('Corpse Field'")
        # Keep Reimagined and Astral minion damage/life/count formulas. Transfer
        # only the corpse-free summoning route the user previously retained.
        skeleton=next(r for r in skills if r['skill']=='Raise Skeleton')
        src_skeleton=next(r for r in source_skills if r['skill']=='Raise Skeleton')
        for field in ('srvstfunc','srvdofunc','cltstfunc','cltdofunc','cltmissilea','SelectProc',
                      'TargetCorpse','TargetAlive','TgtPlaceCheck','calc2'):
            if field in skeleton:skeleton[field]=src_skeleton.get(field,'')
        _,source_states=read(SOURCE/'states.txt')
        overlays={'amplifydamage','weaken','confuse','attract','decrepify','lowerresist','ironmaiden','lifetap'}
        for row in current['states']:
            if row['state'] in overlays:
                src=next(r for r in source_states if r['state']==row['state'])
                row['overlay1']=src['overlay1']
        for table,rows in current.items():
            if table=='skills':
                changed=CURSES|{70}
                assert all(a==b for a,b in zip(original[table],rows) if int(a['*Id']) not in changed)
            elif table=='states':
                assert all(a==b for a,b in zip(original[table],rows) if a['state'] not in overlays)
            else:assert rows[:len(original[table])]==original[table]
            write(directory/(table+'.txt'),headers[table],rows)
        # Skill descriptions refer to named skills/strings, so they need no ID
        # translation and retain the target's table order and record count.
        header,rows=read(directory/'skilldesc.txt');_,source=read(SOURCE/'skilldesc.txt')
        descriptions={by_source[i]['skilldesc'] for i in CURSES}
        for i,row in enumerate(rows):
            if row['skilldesc'] in desc_mapping:
                src=next(r for r in source if r['skilldesc']==desc_mapping[row['skilldesc']])
                rows[i]={k:src.get(k,'') for k in header}
                rows[i]['skilldesc']=row['skilldesc']
                for key,value in rows[i].items():
                    rows[i][key]=value.replace("skill('Confuse'","skill('Corpse Field'")
        write(directory/'skilldesc.txt',header,rows)

    # Merge only the strings actually used by the transferred descriptions.
    _,descs=read(SOURCE/'skilldesc.txt')
    used={value for row in descs if row['skilldesc'] in descriptions
          for key,value in row.items() if value and (key.startswith('str ') or 'text' in key)}
    source_strings=json.loads((HERE/'upstream/data/local/lng/strings/skills.json').read_text(encoding='utf-8-sig'))
    destination=root/'data/local/lng/strings/skills.json'
    rows=json.loads(destination.read_text(encoding='utf-8-sig'))
    lookup={r['Key']:r for r in rows}
    last=max(r['id'] for p in destination.parent.glob('*.json')
             for r in json.loads(p.read_text(encoding='utf-8-sig')) if isinstance(r,dict) and 'id' in r)
    for src in source_strings:
        if src['Key'] not in used:continue
        if src['Key'] in lookup:
            row=lookup[src['Key']];new_id=row['id'];row.update(src);row['id']=new_id
        else:
            last+=1;rows.append({**src,'id':last})
    destination.write_text(json.dumps(rows,ensure_ascii=False,indent=4),encoding='utf8')
    assets.append(str(destination.relative_to(root)).replace('\\','/'))

    for p in (HERE/'upstream/data/hd').rglob('*'):
        if not p.is_file():continue
        relative=p.relative_to(HERE/'upstream')
        dest=root/relative;dest.parent.mkdir(parents=True,exist_ok=True)
        if '/spells/necromancer/' in str(relative).replace('\\','/') and dest.exists():
            # Patch only curse icon frames, preserving all other Reimagined art.
            import struct
            source=p.read_bytes();target=dest.read_bytes()
            def shape(raw):
                version,fw,w,h,zero,count=struct.unpack_from('<HH4I',raw,4)
                assert raw[:4] in (b'SPa1',b'SpA1') and version==31 and len(raw)==40+w*h*4
                return fw,w,h,count
            fw,w,h,count=shape(source);tw,twidth,th,tcount=shape(target)
            assert (fw,h)==(tw,th) and count>=52 and tcount>=52
            merged=bytearray(target)
            frames={int(r['IconCel'])+state for r in descs if r['skilldesc'] in descriptions for state in (0,1)}
            for y in range(h):
                for frame in frames:
                    a=40+(y*twidth+frame*tw)*4;b=40+(y*w+frame*fw)*4
                    merged[a:a+fw*4]=source[b:b+fw*4]
            dest.write_bytes(merged)
        else:copy2(p,dest)
        assets.append(str(relative).replace('\\','/'))

    # Bind the old implementation to fresh Reimagined IDs. Numeric substitutions
    # are token-bound and intentionally do not touch hexadecimal native RVAs.
    bind_sources(bindings)
    (HERE/'bindings.json').write_text(json.dumps(bindings,indent=2),encoding='utf8')
    (HERE/'assets.json').write_text(json.dumps(assets,indent=2),encoding='utf8')

if __name__=='__main__':
    import sys
    install(Path(sys.argv[1]))
