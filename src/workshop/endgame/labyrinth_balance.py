"""Replace blanket Labyrinth immunities with named Hell counterparts +15 points."""
from pathlib import Path
from combat.port import read,write
SOURCES = {'labhawk': 'foulcrow2', 'labhawknest': 'crownest2', 'labquilrat': 'quillrat4', 'labthornhulk': 'thornhulk1', 'labmauler': 'deathmauler5', 'labtaint': 'bighead3', 'labmoonlord': 'bloodlord1', 'labarach': 'arach1', 'labexploder': 'suicideminion1', 'labcat': 'pantherwoman4', 'labslinger': 'slinger4', 'labraider': 'sandraider3', 'laboblivionknight': 'doomknight3', 'labwraith': 'wraith2', 'labreanimatedhorde': 'reanimatedhorde1', 'labmummy': 'mummy2', 'labzombie': 'zombie2', 'labunraveler': 'unraveler4', 'labstormcaster': 'fingermage3', 'labskelearcher': 'sk_archer3', 'labskeletonwarrior': 'skeleton3', 'labbonefetish': 'bonefetish2', 'labvampire': 'vampire1', 'labskelemagepois': 'skmage_pois2', 'labskelemagecold': 'skmage_cold2', 'labskelemagefire': 'skmage_fire2', 'labskelemageltng': 'skmage_ltng2', 'labcorruptrogue': 'corruptrogue5', 'labcorruptlancer': 'cr_lancer5', 'labcorruptarcher': 'cr_archer5', 'labgoat': 'goatman4', 'labbalrog': 'megademon1', 'labimp': 'imp1', 'labfrozenterror': 'frozenhorror2', 'labfallenshaman': 'fallenshaman1', 'labfallen': 'fallen1', 'labcouncilmember': 'councilmember1', 'labminionofdestruction': 'baalminion1', 'labflayer': 'fetish3', 'labstygiandoll': 'bonefetish5', 'labsuccubus': 'succubus4', 'labsnowyeti': 'snowyeti1', 'labgloam': 'willowisp1', 'labburningsoul': 'willowisp3', 'labviper': 'clawviper3', 'labquillbear': 'quillbear4', 'labsummoner': 'summoner', 'labspidermagus': 'arach5', 'labheirophant': 'cantor3', 'labzealot': 'zealot3', 'labblunderbore': 'blunderbore4', 'labcow': 'hellbovine', 'labfiretrap1': 'gargoyletrap', 'labdiablo': 'diablo', 'labdemonspawn': 'megademon1', 'labskeleswarriorsub': 'skeleton3', 'labskelemagesub': 'skmage_pois2'}
RESISTS = ('ResDm(H)','ResMa(H)','ResFi(H)','ResLi(H)','ResCo(H)','ResPo(H)')
def install(root):
 for folder in ('data/global/excel','data/global/excel/base'):
  p=Path(root)/folder/'monstats.txt';h,rows=read(p);by={r['Id']:r for r in rows}
  for target,source in SOURCES.items():
   for field in RESISTS:by[target][field]=str(int(by[source][field] or 0)+15)
  write(p,h,rows)
 print(f'Labyrinth: {len(SOURCES)} monster variants use native Hell resistances +15 points.')
