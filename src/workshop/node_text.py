"""Item-style English descriptions from the actual property payloads."""
import json
from combat.port import read

FLAT = {'hp':'Life','mana':'Mana','str':'Strength','dex':'Dexterity','vit':'Vitality','enr':'Energy',
        'ac':'Defense','ac-miss':'Defense vs. missiles','att':'Attack Rating','dmg-min':'Minimum damage',
        'dmg-max':'Maximum damage','light':'Light Radius','red-dmg':'Damage reduced','red-mag':'Magic damage reduced',
        'healperhit':'Life on hit','heal-kill':'Life after each kill','mana-kill':'Mana after each kill','regen':'Replenish Life',
        'extra_traps':'Maximum traps','ws-pet-count':'Maximum summons','ws-minion-limit':'Maximum summons',
        'all-stats':'All attributes','allskills':'All skills','magicskill':'Magic skills'}
PERCENT = {'move2':'Faster run/walk','balance2':'Faster hit recovery','swing2':'Increased attack speed',
           'cast2':'Faster cast rate','block2':'Faster block rate','block':'Increased chance of blocking',
           'str%':'Strength','dex%':'Dexterity','vit%':'Vitality','enr%':'Energy',
           'att%':'Bonus to Attack Rating','dmg%':'Enhanced damage','dmg-undead':'Damage to undead',
           'mana%':'Maximum mana','hp%':'Maximum life','mag%':'Better chance of getting magic items','gold%':'Extra gold from monsters',
           'lifesteal':'Life stolen per hit','manasteal':'Mana stolen per hit','crush':'Chance of crushing blow',
           'deadly':'Deadly strike','openwounds':'Chance of open wounds','pierce':'Piercing attack',
           'regen-mana':'Regenerate Mana','res-curse-len':'Reduced curse duration','red-dmg%':'Damage reduced',
           'ws-pet-life':'Minion maximum life','ws-pet-damage':'Minion damage',
           'ws-pet-speed':'Minion attack and movement speed','dmg-to-mana':'Damage taken goes to Mana'}
CLASSES = dict(zip(('ama','sor','nec','pal','bar','dru','ass','war'),
                   ('Amazon','Sorceress','Necromancer','Paladin','Barbarian','Druid','Assassin','Warlock')))
TABS = ('Bow and Crossbow Skills','Passive and Magic Skills','Javelin and Spear Skills',
        'Fire Skills','Lightning Skills','Cold Skills','Summoning Skills','Poison and Bone Skills','Curses',
        'Combat Skills','Offensive Auras','Defensive Auras','Combat Skills','Combat Masteries','Warcries',
        'Summoning Skills','Shape Shifting Skills','Elemental Skills','Martial Arts','Shadow Disciplines','Traps',
        'Demon Skills','Eldritch Skills','Chaos Skills')

class Formatter:
    def __init__(self, root):
        _, skills = read(root/'data/global/excel/skills.txt')
        _, descriptions = read(root/'data/global/excel/skilldesc.txt')
        strings = {r['Key']:r['enUS'] for r in json.loads((root/'data/local/lng/strings/skills.json').read_text(encoding='utf8'))}
        descriptions = {r['skilldesc']:r for r in descriptions}
        self.names = {r['skill']:strings.get(descriptions.get(r['skilldesc'],{}).get('str name'),r['skill']) for r in skills}
        # The artwork/tree is generated before the curse/aura data port.
        self.names.update({'Amplify Damage':'Plague','Terror':'Infection','Dim Vision':'Curse Mastery',
                           'Weaken':'Plague Lord','Corpse Field':'Confuse','Iron Maiden':'Soul Harvest',
                           'Life Tap':'Elemental Marks','Resist Fire':'Reservation Efficiency',
                           'Resist Cold':'Aura Mastery','Resist Lightning':'Astral Guardian',
                           'Thorns':'Retribution','Cleansing':'Sanctified Ground'})

    def effect(self, code, parameter, minimum, maximum=None):
        n = int(minimum); hi = n if maximum is None else int(maximum)
        value = str(n) if hi == n else f'{n}-{hi}'
        if not n and not hi: return ''
        if code in ('hit-skill','kill-skill','att-skill'):
            event = {'hit-skill':'on striking','kill-skill':'on kill','att-skill':'on attack'}[code]
            return f'{n}% chance to cast level {hi} {self.names.get(parameter,parameter)} {event}'
        if code in ('oskill','aura'):
            return f'+{value} to {self.names.get(parameter,parameter)} (All Classes)'
        if code == 'skilltab':
            tab=int(parameter); return f'+{value} to {TABS[tab]} ({list(CLASSES.values())[tab//3]})'
        if code in CLASSES: return f'+{value} to {CLASSES[code]} skill levels'
        if code == 'ignore-ac': return "Ignore target's defense"
        if code == 'nofreeze': return 'Cannot be frozen'
        if code.startswith('dmg-') and code[4:] in ('fire','cold','ltng','pois','mag'):
            element={'ltng':'lightning','pois':'poison','mag':'magic'}.get(code[4:],code[4:])
            if code == 'dmg-pois':
                seconds=int(parameter)/25
                amount=round(n*int(parameter)/256)
                return f'Adds {amount} poison damage over {seconds:g} seconds to attacks'
            return f'Adds {value} {element} damage to attacks'
        if code.startswith('pierce-'):
            element={'dmg':'Physical','ltng':'Lightning','pois':'Poison','mag':'Magic'}.get(code[7:],code[7:].title())
            return f'-{value}% to enemy {element} Resistance'
        if code.startswith('extra-'):
            element={'ltng':'Lightning','pois':'Poison','mag':'Magic'}.get(code[6:],code[6:].title())
            return f'+{value}% to {element} skill damage'
        if code in PERCENT: return f'{value}% {PERCENT[code]}'
        if code.startswith('res-'):
            part=code[4:]; maximum=part.endswith('-max'); part=part.removesuffix('-max')
            element={'all':'All','ltng':'Lightning','pois':'Poison','mag':'Magic'}.get(part,part.title())
            return f'+{value}% to '+('maximum ' if maximum else '')+element+' Resistances'
        if code in PERCENT: return f'{value}% {PERCENT[code]}'
        if code in FLAT: return f'+{value} {FLAT[code]}'
        raise ValueError('Missing item-style description: '+code)

    def many(self, effects):
        return '\n'.join(t for e in effects if (t := self.effect(e['code'],e['parameter'],e['minimum'],e['maximum'])))
