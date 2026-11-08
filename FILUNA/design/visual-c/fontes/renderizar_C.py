from pathlib import Path
import concurrent.futures,json
import cairosvg
from PIL import Image,ImageDraw,ImageFont
D=Path(__file__).resolve().parent.parent
entries=json.loads((D/'ENTREGUES.json').read_text())
def render(e):
 if e['status']=='MODELO_VISUAL_APROVADO':return e['id']
 p=D/e['svg'];cairosvg.svg2png(url=str(p),write_to=str(D/e['png']),unsafe=True)
 im=Image.open(D/e['png']);im.resize((960,640),Image.Resampling.LANCZOS).save(p.with_name(p.stem+'-preview.png'))
 return e['id']
if __name__=='__main__':
 with concurrent.futures.ProcessPoolExecutor(max_workers=4) as ex:
  for i,id in enumerate(ex.map(render,entries),1):print(i,id,flush=True)
