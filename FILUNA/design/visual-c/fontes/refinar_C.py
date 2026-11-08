from pathlib import Path
exec((Path(__file__).resolve().parent/'completar_C.py').read_text().split('# DEVICE')[0])
entries=json.loads((D/'ENTREGUES.json').read_text())
# Home structural contract: original logo, hero and three bands.
start('Início','neutral');rect(1270,81,490,74,'#FDFCFA');rect(40,156,224,1004,'#F3F1ED');line(264,156,264,1160)
for i,t in enumerate(['Recentes','Modelos 3D','Paramétricos','Ferramentas','Estoque','Ateliê']):
 y=207+i*58
 if i==0:rect(56,y-28,192,42,C['soft'],10)
 text(76,y,t,16,C['rose'] if i==0 else C['muted'],600 if i==0 else 500)
art(288,180,1048,420,'gato-vazado-ilustrativo.png');text(317,218,'DESTAQUE',11,'#CFD7DE',600);text(317,567,'Gato vazado / peça ilustrativa já fatiada no cenário',16,'#F6F7F8',500)
text(1368,213,'Preparação de impressão',22,C['ink'],600)
for yy,l,v in [(257,'Unidade registrada','Bancada 01'),(341,'Material','PLA / Rosa mineral'),(425,'Perfil','Qualidade / 0,20 mm')]:field(1368,yy,480,l,v)
button(1368,513,480,'Imprimir',True,'upload');text(1368,584,'Revisão e confirmação antes de iniciar.',13,C['muted'])
for yy,title,keys,names in [(641,'Recentes',['lua-litofania','caixa-sombra','criador-trofeu','luminaria-placa','arvore-natal'],['Lua Litofania LED','Caixa de Sombra','Troféu','Luminária','Árvore de Natal']),(820,'Modelos',['chaveiro-botton','pote-doces-imagem','flexi-articulado','envelope-renda','foto-linear-3d'],['Chaveiro Botton','Pote para Doces','Brinquedo Flexível','Envelope de Renda','Foto Linear 3D'])]:
 text(288,yy,title,22,C['ink'],600);text(1848,yy,'Ver todos',13,C['rose'],500,'end')
 for i,(k,t) in enumerate(zip(keys,names)):
  x=288+i*316;rect(x,yy+18,296,86,C['dark'],10);motif(x+91,yy+20,116,83,k);text(x,yy+133,t,14,C['ink'],500)
text(288,1016,'Ferramentas',22,C['ink'],600)
for i,t in enumerate(['Imagem para 3MF','Imagem para SVG','Preço simples','Preço completo']):
 x=288+i*395;icon(x,1047,['cube','cor','clock','settings'][i],C['rose'],26);text(x+40,1067,t,15,C['ink'],500)
out('inicio','HOME-01','Início / três faixas e capas autorais ilustrativas','SHELL §2; HOME-18; t26u')
# Calibration tutorials retain contract navigation and destinations exactly.
cal=[('CAL-01','Temperatura','Filamento / temperatura do bico','Comparar acabamento e adesão por faixa.','Pontes, fios e superfícies em temperaturas diferentes.','Varia a temperatura por faixas do modelo.'),('CAL-02','Fluxo máximo','Filamento / mm³/s','Identificar vazão sustentável do material.','Regiões com falhas quando o fluxo excede o limite.','Varia o fluxo volumétrico do teste.'),('CAL-03','Avanço de pressão','Filamento / pressure advance','Ajustar a resposta de fluxo nas acelerações.','Quinas com excesso e falta de material.','Varia o parâmetro de pressão no teste.'),('CAL-04','Taxa de fluxo','Filamento / razão de fluxo','Ajustar a quantidade efetiva de plástico.','Topo com lacunas ou excesso.','Compara regiões com fatores de fluxo distintos.'),('CAL-05','Retração','Impressora / distância e velocidade','Reduzir fios entre partes separadas.','Fios e marcas de retomada.','Varia a retração conforme o teste.'),('CAL-06','Quinas','Impressora / jerk ou quina','Comparar mudanças de direção.','Ondulações e regularidade nos cantos.','Percorre quinas nas condições selecionadas.'),('CAL-07','Modelagem de entrada','Firmware ou G-code inicial','Entender ressonância e compensação.','Ondulações após arestas.','Executa teste compatível selecionado no submenu.'),('CAL-08','VFA','Impressora / velocidades a evitar','Investigar artefatos verticais finos.','Faixas verticais em velocidades diferentes.','Varia a velocidade na peça.'),('CAL-09','Tolerância','Processo / furo e contorno','Comparar folgas e encaixes reais.','Encaixe livre, firme ou bloqueado.','Imprime pares com folgas graduais.'),('CAL-10','Guia de calibração','Só a ordem / não grava parâmetro','Organizar sequência e evidências físicas.','Resultados dos testes em ordem.','Não inicia impressão nesta tela.')]
def calshell(id,title,sub='Tutorial para o cliente / revisão humana da peça física'):
 boot('Calibração',title,sub);rect(40,284,284,876,'#F0EEE9')
 for i,(cid,t,*_) in enumerate(cal):
  y=330+i*64
  if cid==id:rect(56,y-27,252,42,C['soft'],9)
  text(72,y,t+(' ›' if cid=='CAL-07' else ''),14,C['rose'] if cid==id else C['muted'],600 if cid==id else 400)
for idx,(id,title,dest,a,b,c) in enumerate(cal):
 calshell(id,title);rect(356,310,650,594,C['dark'],18)
 if id=='CAL-01':art(356,310,650,594,'torre-temperatura-ilustrativa.png')
 elif id=='CAL-02':
  for i in range(8):
   yy=388+i*52;path(f'M407 {yy}L945 {yy+17}M945 {yy+17}L407 {yy+35}',stroke='#E6A3BF',w=2+i*.65)
 elif id=='CAL-03':
  for i in range(6):
   xx=413+i*94;path(f'M{xx} 769V467L{xx+42} 424L{xx+77} 467V769',stroke='#DB9AB4',w=3+i*.6);text(xx+38,823,f'{.01+i*.01:.2f}',13,'#E5D4DC',400,'middle')
 elif id=='CAL-04':
  for i in range(3):
   xx=391+i*201;rect(xx,461,178,208,'#BA7895',9)
   for j in range(12):line(xx+11,475+j*15,xx+167,475+j*15,'#EBC5D5',1.5+i)
   text(xx+89,726,['−5 %','0 %','+5 %'][i],20,'#E8D6DE',500,'middle')
 elif id=='CAL-05':
  for xx in [445,813]:
   path(f'M{xx} 758V456L{xx+48} 425L{xx+91} 456V758Z','#C68AA3','#EDBBD0',2)
  for j in range(6):path(f'M536 {474+j*43}Q682 {440+j*38} 813 {474+j*43}',stroke='#E2B9CC',w=max(.5,2-j*.24))
 elif id=='CAL-06':
  for i in range(4):
   yy=388+i*105;path(f'M407 {yy+70}H566L659 {yy}L758 {yy+70}H945',stroke='#DEA5BF',w=4)
 elif id=='CAL-07':
  for i,t in enumerate(['Ringing tower','Fast tower','ZV','MZV','ZVD','EI']):
   x=381+(i%2)*303;y=345+(i//2)*171;rect(x,y,278,144,'#303A42',13);icon(x+16,y+18,'camadas','#D7B1C3',30);text(x+16,y+113,t,22,'#EAE0E4',500)
 elif id=='CAL-08':
  path('M450 753V384L799 426V796Z','#B97C98')
  for i in range(35):line(457+i*9,389+i*1.1,457+i*9,752+i*1.1,'#E7B5C9',1+(i%5)*.4)
 elif id=='CAL-09':
  for i in range(5):
   x=396+i*115;rect(x,496,85,114,'#C78BA5',9);circle(x+42,552,19+i*2,C['dark']);text(x+42,677,f'{.1+i*.05:.2f}',15,'#E2C5D2',500,'middle')
 else:
  for i,(cid,t,*_) in enumerate(cal[:-1]):text(391,366+i*53,str(i+1).zfill(2)+'  '+t,19,'#E8D8E1',500)
  text(391,867,'Sair do modo de calibração antes de peça comum.',13,'#D2B1C0')
 for i,(t,body) in enumerate([('Para que serve',a),('O que você vai ver na peça',b),('O que a impressora faz',c),('Onde gravar o resultado',dest)]):block(1050,354+i*151,t,body,798)
 if id=='CAL-10':button(356,944,650,'Sem arquivo',False,'help')
 else:button(356,944,650,'Baixar modelo de referência',False,'download')
 text(356,1035,'1. Preparar o perfil   2. Revisar o teste   3. Comparar a peça',14,C['ink'],500);para(1050,1035,'Como saber se deu certo: compare acabamento e medidas; registre somente após validação física.',798,15)
 out('calibracao',id,title+' / tutorial','SHELL §4',variants={'navegacao':'lateral fixa x40–324','tutorial':'quatro seções x1050','modelo':'x356–1006'})
for i,t in enumerate(['Ringing tower','Fast tower','ZV','MZV','ZVD','EI'],1):
 calshell('CAL-07','Modelagem de entrada / '+t);rect(356,310,650,594,C['dark'],18)
 text(392,370,t,33,'#EFDBE6',500)
 if i<=2:
  for k in range(8):
   yy=400+k*48;path(f'M450 {yy+26}V{yy}H640V{yy+26}H856V{yy}',stroke='#D8A2B8',w=3)
 else:
  pts=[(385+j*3,600+math.sin(j*.22)*137*math.exp(-j/(18+i*7))) for j in range(190)];path('M'+' L'.join(f'{x:.2f} {y:.2f}' for x,y in pts),stroke='#E2A6C2',w=3);line(385,600,970,600,'#6D7E89',1)
 text(390,855,'Diagrama ilustrativo / não define frequência real',14,'#D3C1CC')
 for j,(title,body) in enumerate([('Para que serve','Comparar a ressonância no teste '+t+'.'),('O que observar','Ondulação e resposta após arestas.'),('O que a impressora faz','Depende de suporte do firmware e do teste.'),('Onde gravar','Firmware ou G-code inicial, antes de gerar arquivo.')]):block(1050,354+j*151,title,body,798)
 for j,q in enumerate(['Ringing tower','Fast tower','ZV','MZV','ZVD','EI']):
  x=356+j*249;rect(x,963,230,53,C['soft'] if q==t else 'white',10,C['line']);text(x+115,997,q,15,C['rose'] if q==t else C['muted'],500,'middle')
 text(356,1095,'Sem alteração de arquivo gerado. Nenhum teste ou comando executado.',16,C['muted']);out('calibracao',f'CAL-INPUT-{i}',t,'SHELL §4 submenu input')
# Additional editor pages retain all normative fields instead of hiding them off canvas.
for id,title,fields in [('PREP-10-CONT','Impressora / opções especialistas',[('Ventoinha auxiliar','Não'),('Malha só onde imprime','Sim'),('Extrusoras','1'),('G-code de início','Origem do preset OEM'),('G-code de fim','Origem do preset OEM')]),('PREP-11-CONT','Filamento / opções especialistas',[('Sem vento no começo','3 camadas'),('Avanço que muda com a velocidade','Não'),('Temperatura em que amolece','45 °C'),('Compactar na troca','10 mm³'),('G-code ao carregar','Vazio')])]:
 boot('Preparação',title,'Continuação do editor / campos reais UX-PREP / preset não altera G-code já gerado')
 for i,(l,v) in enumerate(fields):field(72,352+i*139,850,l,v)
 art(990,326,858,735,'mesa-suporte-orbital.png');out('preparacao',id,title,'UX-PREP §§2–3')
# Main preview legend needs all detailed contracts still labelled proposal; no changes to anchor.
# Remove obsolete gallery board from current deliverable (preserve history in producao)
entries=[e for e in entries if e['id']!='PAR-01']
(D/'ENTREGUES.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2));print(len(entries),'pranchas refinadas')
