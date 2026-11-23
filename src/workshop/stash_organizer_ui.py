"""Native stash organizer panel. All actions use the existing UI message route."""
def layout():
 children=[]
 def add(typ,name,x,y,w,h,**fields):
  children.append(dict(type=typ,name=name,fields=dict(rect=dict(x=x-1220,y=y-580,width=w,height=h),**fields)))
 def label(name,x,y,text,w=1100):
  add('TextBoxWidget',name,x,y,w,40,text=text,enabled=False,style=dict(pointSize=24,alignment=dict(h='left',v='center')))
 def button(name,x,y,w,labeltext=''):
  add('ButtonWidget',name,x,y,w,48,filename='Workshop\\organizer'+str(80 if w==25 else w),normalFrame=0,hoveredFrame=1,pressedFrame=2,disabledFrame=3,onClickMessage='PanelManager:ClosePanel:WorkshopStashUI'+name)
  if labeltext:label(name+'Label',x+12,y+4,labeltext,w-24)
 def field(name,x,y,w,n):
  button(name,x,y,w)
  label(name+'Value',x+10,y+4,'',w-20)
 add('ClickCatcherWidget','Catch',0,0,1200,1160)
 add('ButtonWidget','Background',0,0,1200,1160,filename='Workshop\\organizerPanel',normalFrame=0,hoveredFrame=0,pressedFrame=0,disabledFrame=0,onClickMessage='PanelManager:ClosePanel:WorkshopStashUIBlur')
 label('Heading',40,25,'SHARED STASH - PAGE OPTIONS & SEARCH')
 button('Close',1080,20,80,'X')
 label('SearchLabel',40,125,'Search:',270)
 field('Search',325,120,350,80)
 button('Clear',690,120,230,'Search')
 button('ResetSearch',930,120,230,'Clear')
 label('NameLabel',40,205,'Change Page Name:',350)
 field('PageNameInput',400,200,500,40)
 button('Save',930,200,230,'Save name')
 button('Favorite',40,275,350,'Make Favorite')
 button('Favorites',410,275,590,'Show Only Favorites')
 label('ColorLabel',40,345,'Color:',170)
 for i in range(6):
  button('Color'+str(i),220+i*90,340,80)
  children[-1]['fields']['filename']='Workshop\\organizerColor'+str(i)
 label('PageLabel',40,420,'Page:',170)
 field('Page',220,415,130,4)
 button('Go',370,415,80,'Go')
 for i in range(10):
  button('Row'+str(i),40,505+i*48,1120)
  label('Result'+str(i),140,509+i*48,'',970)
  button('Scroll'+str(i),1135,505+i*48,25)
  for c in range(6):
   add('ImageWidget','ResultColor'+str(i)+'_'+str(c),44,505+i*48,80,48,filename='Workshop\\organizerColor'+str(c),visible=False,enabled=False)
 button('Prev',40,1005,230,'Previous')
 button('Next',930,1005,230,'Next')
 label('CurrentPage',140,75,'',1000)
 for c in range(6):
  add('ImageWidget','CurrentColor'+str(c),44,71,80,48,filename='Workshop\\organizerColor'+str(c),visible=False,enabled=False)
 label('ResultCount',285,1010,'',630)
 label('Status',40,1060,'',1100)
 label('Hint',40,1100,'Click a result to reveal it in the stash.')
 return dict(type='Panel',name='astral-workshop/StashOrganizer',fields=dict(anchor=dict(x=1.0,y=0.5),priority=8600),children=children)
