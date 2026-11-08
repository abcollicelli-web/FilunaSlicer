from pathlib import Path
import json,zipfile,hashlib,collections,shutil
D=Path(__file__).resolve().parent.parent;E=json.loads((D/'ENTREGUES.json').read_text());P=D/'pacotes-finais';P.mkdir(exist_ok=True)
# Only final referenced files; no prior sketches, render caches, virtualenvs or previous ZIPs.
files={D/'README.md',D/'MATRIZ-COBERTURA.md',D/'MATRIZ-COBERTURA.json',D/'ENTREGUES.json',D/'overview-final.png'}
for e in E:files.update([D/e['png'],D/e['svg']])
for folder in ['assets','documentacao','mosaicos']:
 files.update(p for p in (D/folder).rglob('*') if p.is_file())
files.update(p for p in (D/'fontes/documentos').glob('*') if p.is_file())
for n in ['Inter-VariableFont.ttf','Inter-LICENSE.txt','_base_C.py','completar_C.py','refinar_C.py','ajustes_finais_C.py','renderizar_C.py','fechar_documentacao_C.py','LEIA-ME.md','estados-dispositivo.json','empacotar_final_C.py']:files.add(D/'fontes'/n)
# Preserve source inventory as provenance, no old operational README in publishable subset.
files.add(D/'producao/INVENTARIO-LOTE1.json')
qa=json.loads((D/'documentacao/VERIFICACAO-VISUAL.json').read_text());qa['visual_review']='Inspeção visual dos 11 mosaicos completos; seis folhas de recortes incluem todos os subestados dos 22 geradores; PNGs individuais significativos de cada família e ajustes de Dispositivo, Preparação, Calibração e riscos examinados. Sem certificação funcional.';qa['review_artifacts']=['mosaicos/','revisao-final/variantes-parametricos-1.jpg … variantes-parametricos-6.jpg'];(D/'documentacao/VERIFICACAO-VISUAL.json').write_text(json.dumps(qa,ensure_ascii=False,indent=2))
manifest={str(p.relative_to(D)):dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in sorted(files)}
(D/'MANIFESTO-SHA256.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2));files.add(D/'MANIFESTO-SHA256.json')
# Isolated publishable tree, not a repository checkout and no Git operations.
stage=Path('/workspace/filuna-publicacao-visual-c-20261108');target=stage/'FILUNA/design/visual-c';target.mkdir(parents=True,exist_ok=True)
for p in files:
 q=target/p.relative_to(D);q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
(stage/'LEIA-ME-PUBLICACAO.md').write_text('''# Material documental preparado / ainda não publicado

Esta pasta não é um checkout Git. Copiar FILUNA/design/visual-c/ para uma branch documental nova, partindo do dossiê atual. Consultar documentacao/ADENDOS-PARA-REPO.md para os quatro adendos mínimos, append-only, antes de aplicar. Não modificar código, workflows, gates ou cronogramas de engenharia. Não fazer merge nem executar CI. Commit sugerido: docs(design): documenta direção C e telas [skip ci]. Nenhuma branch, commit ou push foi efetuado nesta preparação.
''')
common={p for p in files if str(p.relative_to(D)).startswith(('assets/','fontes/Inter-'))}
packages=[]
def writezip(name,fs,readme=None):
 z=P/name
 with zipfile.ZipFile(z,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as zz:
  for p in sorted(fs):zz.write(p,str(p.relative_to(D)))
  if readme:zz.writestr('LEIA-ME-FAMILIA.md',readme)
 with zipfile.ZipFile(z) as zz:assert zz.testzip() is None
 assert z.stat().st_size<24_000_000,(name,z.stat().st_size)
 packages.append(dict(path=str(z),bytes=z.stat().st_size,MB=round(z.stat().st_size/1e6,2),sha256=hashlib.sha256(z.read_bytes()).hexdigest()))
for fam in sorted(set(e['family'] for e in E)):
 es=[e for e in E if e['family']==fam]
 groups=[es[:11],es[11:]] if fam=='preparacao' else [es]
 for gi,part in enumerate(groups,1):
  fs=set(common);fs.add(D/'mosaicos'/(fam+'.png'))
  for e in part:fs.update([D/e['png'],D/e['svg']])
  readme=f'# Filuna C / {fam} / conjunto {gi} de {len(groups)}\n\n{len(part)} pranchas PNG 2880×1920 e SVG editáveis. Direção C aprovada; derivadas em revisão. Manter assets e instalar Inter (OFL) para editar. Dados ilustrativos, não funcional; nenhum teste ou comando executado. G2/G3 BLOCKED · G4 NOT RUN · G5/G6 BLOCKED. Cada ZIP abre independentemente; não é binário dividido.\n\n'
  for e in sorted(part,key=lambda e:e['id']):readme+=f"- {e['id']}: {e['title']} · {e['png']} · {e['status']}\n"
  suffix=f'-{gi}' if len(groups)>1 else ''
  writezip('Filuna-C-'+fam+suffix+'.zip',fs,readme)
docfiles={p for p in files if not any(str(p.relative_to(D)).startswith(fam+'/') for fam in set(e['family'] for e in E))}
writezip('Filuna-C-documentacao-e-mosaicos.zip',docfiles)
(D/'PACOTES-FINAIS.json').write_text(json.dumps(packages,ensure_ascii=False,indent=2))
print('STAGING',stage,'FILES',len(files),'TOTAL_BYTES',sum(p.stat().st_size for p in files))
for p in packages:print(p['MB'],p['path'])
