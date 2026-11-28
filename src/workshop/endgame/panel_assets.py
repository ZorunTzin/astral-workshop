"""Native trial-choice panel using the same sprite/layout path as Astral Atlas."""
import json,struct,math,random
from PIL import Image,ImageDraw,ImageColor
from pathlib import Path

def install(root):
 root=Path(root);out=root/'data/hd/global/ui/Workshop';out.mkdir(parents=True,exist_ok=True);assets=[]
 def sprite(name,w,h,frames,color='#d8b773',border_only=False):
  image=Image.new('RGBA',(w*frames,h));draw=ImageDraw.Draw(image)
  for i in range(frames):
   x=i*w;draw.rounded_rectangle((x+2,2,x+w-3,h-3),22,fill=None if border_only else ('#251238' if i!=3 else '#40234f'),outline=color,width=4)
   draw.rounded_rectangle((x+14,14,x+w-15,h-15),16,outline=color if border_only else '#715184',width=2)
  if border_only:
   rgb=ImageColor.getrgb(color);random.seed(71)
   layer=Image.new('RGBA',(w,h));d=ImageDraw.Draw(layer)
   for y in range(h):
    glow=.17+.18*(1-y/h)+.10*math.exp(-((y-h*.2)/(h*.24))**2)
    d.line((0,y,w,y),fill=tuple(int(c*glow) for c in rgb)+(255,))
   for _ in range(65):
    xx=random.randrange(24,w-24);yy=random.randrange(24,h-24)
    d.ellipse((xx,yy,xx+1,yy+1),fill=tuple(int(c*.5) for c in rgb)+(255,))
   d.rounded_rectangle((3,3,w-4,h-4),22,outline=color,width=4)
   d.rounded_rectangle((15,15,w-16,h-16),16,outline=tuple(int(c*.5) for c in rgb)+(255,),width=1)
   for yy in (115,):
    d.line((65,yy,w-65,yy),fill=color,width=2)
    d.polygon(((w//2,yy-7),(w//2+7,yy),(w//2,yy+7),(w//2-7,yy)),fill=color)
   for xx in (35,w-35):
    d.line((xx,35,xx,80),fill=color,width=2);d.line((xx,35,xx+(35 if xx<100 else -35),35),fill=color,width=2)
   mask=Image.new('L',(w,h));ImageDraw.Draw(mask).rounded_rectangle((2,2,w-3,h-3),22,fill=255)
   layer.putalpha(mask);image=layer
  path=out/(name+'.sprite');path.write_bytes(struct.pack('<4sHH8I',b'SpA1',31,w,w*frames,h,0,frames,0,0,0,0)+image.tobytes());assets.append(str(path.relative_to(root)).replace('\\','/'))
 sprite('trial-panel',2300,1250,1);sprite('trial-card',650,740,4);sprite('trial-finish',800,100,4);sprite('trial-active',420,76,4)
 for tier,color in enumerate(('#ffffff','#528fff','#ffff42','#bb66ff','#e8b443')):sprite('trial-rarity-'+str(tier),650,740,1,color,True)
 def widget(t,n,**f):return dict(type=t,name=n,fields=f)
 def text(n,x,y,w,h,value,visible=True):return widget('TextBoxWidget',n,rect=dict(x=x,y=y,width=w,height=h),text=value,style='$StyleModalDialogDescription',visible=visible,enabled=False)
 children=[widget('ImageWidget','Background',rect=dict(x=-1150,y=-625),filename='Workshop\\trial-panel'),text('Title',-1010,-560,2020,90,'ASTRAL TRIALS - CHOOSE YOUR NEXT MODIFIER')]
 catalog=json.loads((root/'workshop/endgame/modifiers.json').read_text());names=[m['name'] for m in catalog];effects=[m['effect'] for m in catalog];base=[m['base'] for m in catalog];tiers=['WHITE','MAGIC','RARE','UNIQUE','LEGENDARY'];strength=[1,2,3,4,6]
 for slot in range(3):
  x=-1030+705*slot
  children.append(widget('ButtonWidget','Card'+str(slot),rect=dict(x=x,y=-400),filename='Workshop\\trial-card',pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage='PanelManager:ClosePanel:TrialChoice'+str(slot)))
  for tier in range(5):children.append(widget('ImageWidget',f'Rarity_{slot}_{tier}',rect=dict(x=x,y=-400),filename='Workshop\\trial-rarity-'+str(tier),visible=False,enabled=False))
  for positive,label in enumerate(('MONSTER MODIFIER','CHARACTER BONUS')):children.append(text(f'Category_{slot}_{positive}',x+32,-372,586,65,label,False))
  for stage in range(1,6):
   for mod in range(len(catalog)):
    for tier in range(5):
     value=catalog[mod]["tiers"][tier]*(100+10*(stage-1))//100;reward=(5 if catalog[mod]['positive'] else 10)*(1<<tier)//2
     valueText=f'{tiers[tier]}\n\n{names[mod]}\n{effects[mod]}\nUp to +{value}{catalog[mod]['unit']}\nRun cap: {str(catalog[mod]['cap'])+catalog[mod]['unit'] if not catalog[mod].get('uncapped') else 'None'}\nMonsters: +10% life / +2% damage\n\nLoot bonus +{reward}%\n\nCLICK TO CHOOSE'
     if catalog[mod]['kind']=='wave':valueText=f'{tiers[tier]}\n\n{names[mod]}\n{effects[mod]}\nNEXT WAVE ONLY\nMonsters: +10% life / +2% damage\n\nLoot bonus +{reward}%\n\nCLICK TO CHOOSE'
     if names[mod]=='League invasion':
      for league,title in enumerate(('Night of the Dead','Breach','Beyond'),1):
       children.append(text(f'LeagueOffer_{slot}_{stage}_{mod}_{tier}_{league}',x+32,-275,586,600,valueText.replace(names[mod],title+' invasion').replace(effects[mod],'Next wave: '+title+' monsters'),False))
     else:children.append(text(f'Offer_{slot}_{stage}_{mod}_{tier}',x+32,-275,586,600,valueText,False))
 children += [widget('ButtonWidget','Finish',rect=dict(x=-400,y=410),filename='Workshop\\trial-finish',pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage='PanelManager:ClosePanel:TrialFinish'),text('FinishLabel',-370,430,740,70,'FINISH RUN'),text('Hint',-1020,550,2040,60,'Choose one card to begin the next wave. Ctrl+J: hide/show this menu.')]
 children += [widget('ButtonWidget','ClearLoot',rect=dict(x=-1100,y=-585),filename='Workshop\\trial-active',pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage='PanelManager:ClosePanel:TrialClearLoot'),text('ClearLootLabel',-1090,-580,400,65,'DELETE GROUND LOOT')]
 children += [widget('ButtonWidget','ActiveToggle',rect=dict(x=680,y=-585),filename='Workshop\\trial-active',pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage='PanelManager:ClosePanel:TrialActive'),text('ActiveLabel',690,-580,400,65,'ACTIVE MODIFIERS')]
 children.append(text('NextWaveLabel',-1090,-503,270,70,'NEXT WAVE:'))
 for pos in range(5):
  for digit in range(10):children.append(text(f'NextWave_{pos}_{digit}',-820+pos*30,-503,30,70,str(digit),False))
 for name,x,event in [('Reroll',-1030,'TrialReroll'),('Banish',610,'TrialBanish')]:
  children.append(widget('ButtonWidget',name,rect=dict(x=x,y=420),filename='Workshop\\trial-active',pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage='PanelManager:ClosePanel:'+event))
  for count in range(4):children.append(text(f'{name}Count_{count}',x+10,425,400,65,f'{name.upper()}S: {count}'.replace('BANISHS','BANISHES'),False))
 for row in range(8):
  for mod,m in enumerate(catalog):children.append(text(f'Active_{row}_{mod}',-980,-390+row*95,1450,90,m['name']+' | '+m['effect']+' ('+('percent' if m['unit']=='%' else 'flat')+')',False))
  for pos in range(5):
   for digit in range(10):children.append(text(f'Value_{row}_{pos}_{digit}',600+pos*55,-390+row*95,55,90,str(digit),False))
 children += [widget('ButtonWidget','ActiveNext',rect=dict(x=-210,y=410),filename='Workshop\\trial-active',pressedFrame=1,disabledFrame=2,hoveredFrame=3,onClickMessage='PanelManager:ClosePanel:TrialActiveNext',visible=False),text('ActiveNextLabel',-200,415,400,65,'NEXT PAGE',False),text('ActiveHelp',-1020,550,2040,60,'Total selected bonuses. Click ACTIVE MODIFIERS again to return.',False),text('ActiveEmpty',-900,-100,1800,150,'No selected modifiers yet.',False)]
 panel=widget('Panel','workshop-curses/TrialChoices',anchor=dict(x=.5,y=.5),priority=8600);panel['children']=children
 literal=json.dumps(panel);(root/'workshop/endgame/panel_layout.h').write_text('inline constexpr char NativeChoiceLayout[]=\n'+'\n'.join('R"trial('+literal[j:j+6000]+')trial"' for j in range(0,len(literal),6000))+';\n')
 return assets
