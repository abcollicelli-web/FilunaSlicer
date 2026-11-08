from pathlib import Path
import json,hashlib,shutil,collections,math,xml.etree.ElementTree as ET
from PIL import Image,ImageDraw,ImageFont
D=Path(__file__).resolve().parent.parent;E=json.loads((D/'ENTREGUES.json').read_text());emap={e['id']:e for e in E};old=json.loads((D/'producao/INVENTARIO-LOTE1.json').read_text());rows=old['inventory'];ns=[]
for r in rows:
 id=r['id'];e=None;variant='tela principal'
 if id=='PREP-01':e=emap['PREP-APROVADO'];variant='âncora original / Global, Avançado, Resistência, Girar'
 elif id=='DEV-11':e=emap['DEV-APROVADO'];variant='âncora original / impressão; aquecimento na prancha complementar DEV-11-AQUECIMENTO'
 elif id=='PAR-GAL':e=emap['PAR-GAL-1'];variant='três segmentos da grade / PAR-GAL-1, PAR-GAL-2, PAR-GAL-3';r['complementary_images']=['parametricos/PAR-GAL-2.png','parametricos/PAR-GAL-3.png']
 elif r['family']=='parametricos':
  base=id;state='entrada'
  for suffix in ['prévia','gerando','falha','gerado']:
   if id.endswith('-'+suffix):base=id[:-len(suffix)-1];state=suffix;break
  e=emap.get(base)
  if e:variant=e['variants'][state]
 else:e=emap.get(id)
 if not e:raise Exception('Sem imagem '+id)
 r.update(status=e['status'],png=e['png'],svg=e['svg'],visible_variant=variant,visual_coverage='DESENHADO_EM_C',pendencia=None)
 if 'CONTRATO_DETALHADO_PENDENTE' in e['status']:r['pendencia_documental']='Composição candidata; contrato detalhado não confirmado pelas fontes identificadas.'
 if r['family']=='parametricos' and id!='PAR-GAL':r['pendencia_documental']='Nomes de controles derivados de UX-P §4; valores e layout propostos, domínios/defaults finais exigem catálogo real. Subestados de feedback são propostas visuais.'
 if id in ['PREP-10','PREP-11']:r['complementary_images']=[f'preparacao/{id}-CONT.png'];r['visible_variant']+='; continuação dos campos na prancha complementar'
 if id=='DEV-11':r['complementary_images']=['dispositivo/DEV-11-AQUECIMENTO.png']
 ns.append(r)
# No invented inventory expansion: only three explicit continuations required to make current states visible.
for id,base in [('DEV-11-AQUECIMENTO','DEV-11'),('PREP-10-CONT','PREP-10'),('PREP-11-CONT','PREP-11')]:
 r=dict(next(r for r in rows if r['id']==base));e=emap[id];r.update(id=id,state=e['title'],requirement_id='VC-'+id,png=e['png'],svg=e['svg'],status=e['status'],visible_variant='prancha complementar integral',complementary_images=[]);ns.append(r)
mat=dict(status='COBERTURA_VISUAL_DO_INVENTARIO_CONSOLIDADO_DESENHADA_DERIVADAS_EM_REVISAO',current_source_head=old['current_source_head'],scope='205 linhas do inventário recebido + 3 continuações explícitas; não inventário universal do software',inventory=ns,board_count=len(E),state_count=len(ns),pending_images=0,pending_contracts='Detalhes normativos não confirmados continuam rotulados por linha; não são aprovação funcional.',remote_publication='NAO_EXECUTADA',consolidation='110 estados de 22 geradores em 22 pranchas com render e quatro variantes visíveis. Galeria em três segmentos. Nenhum estado contado só por menção em documento.')
(D/'MATRIZ-COBERTURA.json').write_text(json.dumps(mat,ensure_ascii=False,indent=2))
md=['# Matriz visual C — inventário consolidado','',f'{len(ns)} linhas mapeadas em {len(E)} pranchas. 205 linhas recebidas + 3 continuações. Zero linha sem imagem; isto não é homologação de produto. Derivadas em revisão; campos e contratos pendentes permanecem explícitos.','', '| ID | Família / estado | Imagem / fonte editável | Variante visível | Base | Status |','|---|---|---|---|---|---|']
for r in ns:md.append(f"| {r['id']} | {r['family']} / {r['state']} | [PNG]({r['png']}) · [SVG]({r['svg']}) | {r['visible_variant']} | [{r['source']} {r['section']}]({r['source_url']}) · `{r['source_ref'][:10]}` | {r['status']} |")
(D/'MATRIZ-COBERTURA.md').write_text('\n'.join(md)+'\n')
# Source snapshots are copied, never relabelled as new live reads.
fs=json.loads((D/'documentacao/FONTES.json').read_text())
for f in fs:
 sid=f['id'];p=D/'fontes/documentos'/(sid+'.captura.txt')
 if not p.exists():
  src=Path('/workspace/filuna-dispositivo-20261107/fontes')/(sid+'.captura.txt')
  if src.exists():shutil.copyfile(src,p)
 if p.exists():f['snapshot']=str(p.relative_to(D));f['snapshot_sha256']=hashlib.sha256(p.read_bytes()).hexdigest()
(D/'documentacao/FONTES.json').write_text(json.dumps(fs,ensure_ascii=False,indent=2))
# Revalidated source head (read only, no remote modifications).
(D/'documentacao/HEAD-VERIFICADO.json').write_text(json.dumps({'repo':'abcollicelli-web/FilunaSlicer','branch':'filuna/oem-portable-first-qa-20261106','sha':'83efc13ee2bfdb208859ceb6095fcbd69fe5bef9','verification':'cursor-github get_branch / 08-11-2026 Europe/Rome / somente leitura','published_design_commit':None},indent=2))
p=D/'documentacao/DESIGN-SYSTEM-C.md';s=p.read_text().split('\n\n## Cobertura consolidada desta revisão')[0].replace('futura tela de risco ainda PENDENTE','telas de risco desenhadas em VIS-06 (OEM/aceite por SHA) e VIS-07 (ENGINE_DIVERGENCE/sem override), ainda propostas não implementadas');s+='''

## Cobertura consolidada desta revisão
123 pranchas 2880×1920, em 11 famílias. A matriz rastreia 208 linhas (205 recebidas e três continuações explícitas). “Cobertura visual” significa desenho estático do inventário delimitado, não completude do software, validação de interação, aprovação individual das derivadas ou liberação de gates. Não há chamada de gerador, motor, impressora ou servidor.

### Entradas e variantes
Cada gerador tem sua própria composição e campos provenientes de UX-P §4; valores numéricos sem default no contrato são exemplos propostos, não especificação final. Itens com imagem têm variante de entrada sem peça, render ilustrativo após imagem, gerando sem arquivo válido, erro sem sucesso, e arquivo gerado com próxima ação. Os quatro subestados menores são desenhados em faixa inferior e localizados por coordenadas na matriz, não contados só por texto de checklist. Capas são arte autoral demonstrativa: não substituem as capas reais do catálogo na implementação.

### Continuidade funcional
Revisão de risco não modifica G-code. Risco do próprio OEM admite revisão de aceite consciente e auditável por usuário/SHA; uma alteração no hash invalida o aceite. ENGINE_DIVERGENCE bloqueia sem override e nunca recebe aceite de fabricante. Campos de G-code em presets não são convite para corrigir arquivos gerados. Fila e agenda só abrem revisão/aviso; não iniciam. Sessão, titularidade, capacidade e confirmação continuam necessárias.

### Lacunas documentais não disfarçadas
Visualização, Projeto, Ateliê, ferramentas auxiliares, conta/temas e telas web possuem detalhamento visual candidato em relação às fontes gerais; não se inventou confirmação contratual. Sincronização automática não foi confirmada e não aparece como ativa. O assistente usa uma ordem proposta, porque fontes antigas divergem; confirmar ordem antes de implementar. Os cenários não promovem recursos beta a estáveis. Guia de calibração não tem arquivo e termina pedindo sair do modo de calibração antes de peça comum.
''';p.write_text(s)
# Rewrite proposed append-only additions; no process/safety claims in public docs.
(D/'documentacao/ADENDOS-PARA-REPO.md').write_text('''# Adendos de consulta — texto pronto para aplicação, ainda não publicado

Acrescentar bloco equivalente a AGENTS.md, FILUNA/README.md, FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md e FILUNA/PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md, preservando integralmente história, gates e cronogramas:

## Direção visual C — aprovação t26u / 08-11-2026
Antes de criar ou alterar qualquer tela Filuna, consultar `FILUNA/design/visual-c/documentacao/DESIGN-SYSTEM-C.md`, `APROVACAO-VISUAL-C.json`, os dois modelos C aprovados, `MATRIZ-COBERTURA.md` e `README.md`. O proprietário aprovou a direção dos modelos C; derivadas e detalhamentos permanecem propostas em revisão. O inventário visual consolidado possui 123 pranchas e 208 linhas rastreadas. Isso não certifica software nem substitui requisitos funcionais mais recentes. Preparação não tem painel direito de informações de arquivo. Orca permanece o motor único. G-code é imutável: risco OEM e ENGINE_DIVERGENCE têm tratamentos distintos conforme regra mestre. G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED; não retomar código, CI, hardware, distribuição, servidor ou website com base nesta aprovação visual.

Commit documental sugerido: `docs(design): documenta direção C e telas [skip ci]`.
Branch documental proposta (não criada): `filuna/visual-c-20261108`.
''')
# Main gallery, no false remote links.
counts=collections.Counter(e['family'] for e in E)
readme=['# Filuna — direção visual C / inventário consolidado','', '**123 pranchas estáticas em 11 famílias; 208 linhas rastreadas.** Os dois modelos C aprovados foram preservados byte a byte. Todas as outras composições são propostas em revisão. Não há implementação nem homologação funcional.','', '## O que está pronto','- Seis abas principais; 26 cenários Dispositivo + aquecimento; 22 geradores com variantes visíveis; galeria em três segmentos; dez tutoriais e seis submenus de calibração; editores/menus da Preparação; ferramentas e auxiliares; web futuro separado e OFF.','- 205 linhas do lote recebido consolidadas, mais três continuações explícitas: nenhuma linha sem PNG/SVG. Detalhes normativos não confirmados permanecem marcados; não se afirma inventário universal do software.','- Capas autorais demonstrativas substituem placeholders de estudo; a implementação deverá usar as capas reais do catálogo.','- G2/G3 BLOCKED · G4 NOT RUN · G5/G6 BLOCKED. Nenhum código de aplicativo, CI, build, conta, hardware, servidor ou website alterado.','', '## Documentação','- [Sistema visual C](documentacao/DESIGN-SYSTEM-C.md)','- [Aprovação literal e hashes dos modelos](documentacao/APROVACAO-VISUAL-C.json)','- [Matriz navegável](MATRIZ-COBERTURA.md) · [JSON](MATRIZ-COBERTURA.json)','- [Fontes e referências pinadas](documentacao/FONTES.json)','- [Manifesto dos arquivos](MANIFESTO-SHA256.json)','- [Verificações visuais/documentais](documentacao/VERIFICACAO-VISUAL.json)','- [Adendos de consulta para o repositório](documentacao/ADENDOS-PARA-REPO.md)','- [Visão geral](overview-final.png)','', '## Publicação remota','**Ainda não executada.** O material está pronto localmente e empacotado para publicação documental. Head de origem confirmado em leitura remota: `83efc13ee2bfdb208859ceb6095fcbd69fe5bef9`. Nenhuma branch/commit de entrega foi criada: não há URL de publicação ou SHA remoto a atribuir a estas artes.','', '## Galeria por família']
for fam,count in sorted(counts.items()):
 readme+=['',f'### {fam} — {count} pranchas',f'[Mosaico](mosaicos/{fam}.png)','']
 for e in sorted((e for e in E if e['family']==fam),key=lambda e:e['id']):readme.append(f"- **{e['id']}** · {e['title']} — [PNG]({e['png']}) / [SVG]({e['svg']}) · {e['status']}")
readme+=['','## Como interpretar','PNG e SVG medem 2880×1920. SVGs usam assets locais relativos, sem dependência remota. Manter as pastas assets e fontes junto às famílias. Inter é distribuída com licença OFL. Peças, render de câmera e capas são ilustrações autorais, nunca frames de OEM ou dados medidos.','A matriz informa coordenadas no viewBox 1920×1280, não nos pixels finais 1,5×. Em geradores, o render principal demonstra prévia; a faixa inferior mostra entrada vazia/nome, geração, falha e arquivo válido. Estados propostos não são comportamento executado.','Contratos detalhados ainda não confirmados estão anotados por linha; campos numéricos sem domínio normativo são amostras, não padrões oficiais. Home/galeria não alteram o catálogo publicado.','Histórico do lote parcial está preservado em producao/; ele não é a cobertura desta revisão.']
(D/'README.md').write_text('\n'.join(readme)+'\n')
# Mosaics and inspection sheets, every layout + parametric strip at readable crop size.
(D/'mosaicos').mkdir(exist_ok=True);(D/'revisao-final').mkdir(exist_ok=True)
font=ImageFont.truetype(str(D/'fontes/Inter-VariableFont.ttf'),21)
for fam in sorted(counts):
 es=sorted([e for e in E if e['family']==fam],key=lambda e:e['id']);cols=3;tw=640;th=470
 im=Image.new('RGB',(cols*tw,80+math.ceil(len(es)/cols)*th),'#E8E7E3');dr=ImageDraw.Draw(im);dr.text((24,24),'FILUNA C / '+fam.upper()+' / '+str(len(es))+' PRANCHAS',font=font,fill='#252A2E')
 for i,e in enumerate(es):
  src=Image.open(D/e['png']).convert('RGB');src.thumbnail((tw,427));x=(i%cols)*tw;y=80+(i//cols)*th;im.paste(src,(x,y));dr.text((x+14,y+430),e['id'],font=font,fill='#252A2E')
 im.save(D/'mosaicos'/(fam+'.png'),optimize=True)
 # six boards per inspection sheet, larger than overview
 for group in range(math.ceil(len(es)/6)):
  ims=es[group*6:group*6+6];sheet=Image.new('RGB',(1920,3*678),'#E8E7E3');dd=ImageDraw.Draw(sheet)
  for j,e in enumerate(ims):
   src=Image.open(D/e['png']).convert('RGB').resize((960,640),Image.Resampling.LANCZOS);x=(j%2)*960;y=(j//2)*678;sheet.paste(src,(x,y));dd.text((x+16,y+643),e['id'],font=font,fill='#252A2E')
  sheet.save(D/'revisao-final'/f'{fam}-{group+1}.jpg',quality=94)
# all generator lower strips in groups of four; no state only recorded in words
pars=[e for e in E if e['family']=='parametricos' and e.get('variants')]
for group in range(math.ceil(len(pars)/4)):
 sheet=Image.new('RGB',(2136,4*320),'#E8E7E3');dd=ImageDraw.Draw(sheet)
 for i,e in enumerate(pars[group*4:group*4+4]):
  im=Image.open(D/e['png']).convert('RGB');cr=im.crop((636,1395,2772,1695));sheet.paste(cr,(0,i*320));dd.text((9,i*320+298),e['id'],font=font,fill='#252A2E')
 sheet.save(D/'revisao-final'/f'variantes-parametricos-{group+1}.jpg',quality=95)
# overview 12 representative screens
ids=['HOME-01','PREP-APROVADO','VIS-01','DEV-APROVADO','PROJ-01','CAL-01','DEV-03','DEV-16','PAR-lua-litofania','PAR-GAL-2','VIS-06','WEB-FUT-2']
im=Image.new('RGB',(2400,4*570+95),'#E8E7E3');dr=ImageDraw.Draw(im);dr.text((25,28),'FILUNA / DIREÇÃO C — 123 PRANCHAS / 11 FAMÍLIAS / DERIVADAS EM REVISÃO',font=font,fill='#252A2E')
for i,id in enumerate(ids):
 e=emap[id];src=Image.open(D/e['png']).convert('RGB').resize((800,533),Image.Resampling.LANCZOS);x=(i%3)*800;y=95+(i//3)*570;im.paste(src,(x,y));dr.text((x+15,y+536),id+' / '+e['family'],font=font,fill='#252A2E')
im.save(D/'overview-final.png',optimize=True)
# Static checks only; no app tests.
checks=[]
for e in E:
 p=D/e['png'];im=Image.open(p);assert im.size==(2880,1920),(e['id'],im.size)
 svg=(D/e['svg']).read_text();ET.fromstring(svg);assert not __import__('re').search(r'(?:href|src)="https?://',svg)
 checks.append({'id':e['id'],'png_size':list(im.size),'svg_xml':'VALIDO','remote_asset_refs':0})
for f in json.loads((D/'documentacao/APROVACAO-VISUAL-C.json').read_text())['files']:assert hashlib.sha256((D/f['preserved_copy']).read_bytes()).hexdigest()==f['sha256']
qa={'scope':'Somente arquivos visuais/documentais; não testes de software','boards':len(E),'matrix_states':len(ns),'missing_image_rows':0,'anchor_hashes':'4/4 IGUAIS','xml_dimensions_assets':checks,'visual_review':'Folhas revisao-final e mosaicos geradas para inspeção humana; não é certificação de interação ou acessibilidade.','gates':{'G2':'BLOCKED','G3':'BLOCKED','G4':'NOT RUN','G5':'BLOCKED','G6':'BLOCKED'},'remote_publication':'NAO_EXECUTADA'}
(D/'documentacao/VERIFICACAO-VISUAL.json').write_text(json.dumps(qa,ensure_ascii=False,indent=2))
print('Documentos, matriz e mosaicos prontos:',len(E),len(ns));print(dict(counts))
