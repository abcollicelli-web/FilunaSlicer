from pathlib import Path
import json,math,textwrap,base64,re
from xml.sax.saxutils import escape as esc
D=Path(__file__).resolve().parent.parent
exec((Path(__file__).resolve().parent/'_base_C.py').read_text().split('# PREPARAÇÃO C')[0])
D=Path(__file__).resolve().parent.parent
entries=json.loads((D/'ENTREGUES.json').read_text())
def para(x,y,t,w=600,size=16,c=None,gap=26):
 for i,l in enumerate(textwrap.wrap(t,width=max(15,int(w/(size*.53))))):text(x,y+i*gap,l,size,c or C['muted'])
def art(x,y,w,h,name):s.append(f'<image x="{x}" y="{y}" width="{w}" height="{h}" href="../assets/{name}" preserveAspectRatio="xMidYMid slice"/>')
def boot(active,title,sub=''):
 start(active,'neutral');rect(1270,81,490,74,'#FDFCFA');text(72,215,title,34,C['ink'],600);text(72,254,sub,15,C['muted']);line(72,283,1848,283)
def out(fam,id,title,source='',variants=None,status='PROPOSTO_EM_REVISAO'):
 s.append('</g>');text(42,1202,id+' / '+title,15,C['ink'],500);text(42,1235,'Estudo estático · dados ilustrativos · não funcional · derivada C em revisão',14,C['muted']);s.append('</svg>')
 f=D/fam;f.mkdir(exist_ok=True);p=f/(id+'.svg');p.write_text('\n'.join(s))
 global entries
 entries=[e for e in entries if e['id']!=id];entries.append(dict(id=id,family=fam,title=title,source=source,status=status,png=f'{fam}/{id}.png',svg=f'{fam}/{id}.svg',variants=variants or {}))
def block(x,y,title,body,w=550,ic=None):
 if ic:icon(x,y-22,ic,C['rose'],28);x+=44;w-=44
 text(x,y,title,21,C['ink'],600);para(x,y+36,body,w,15);line(x,y+105,x+w,y+105)
def notice(x,y,w,title,body,color=None):
 rect(x,y,w,110,'#F0EAE7',14);icon(x+20,y+21,'shield',color or C['rose'],26);text(x+63,y+34,title,18,color or C['ink'],600);para(x+63,y+65,body,w-88,14,gap=22)
def listrows(x,y,w,rows,rowh=81):
 for i,row in enumerate(rows):
  yy=y+i*rowh;text(x,yy+25,row[0],17,C['ink'],500)
  if len(row)>1:para(x,yy+53,row[1],w-175,14,gap=20)
  if len(row)>2:text(x+w,yy+26,row[2],14,C['rose'],500,'end')
  line(x,yy+rowh-1,x+w,yy+rowh-1)
def checks(x,y,labels,w=600):
 for i,t in enumerate(labels):
  yy=y+i*54;rect(x,yy,24,24,'white',6,C['line']);para(x+40,yy+19,t,w-50,15)
def metric(x,y,label,val,unit=''):
 text(x,y,label,12,C['muted'],600);text(x,y+54,val,44,C['ink'],500);text(x+max(70,len(val)*28),y+52,unit,16,C['muted'])
def strip(items,y=945,x=72,w=1776):
 cw=(w-18*(len(items)-1))/len(items)
 for i,(title,body,kind) in enumerate(items):
  xx=x+i*(cw+18);rect(xx,y,cw,170,'white',13,C['line']);icon(xx+18,y+18,kind,C['rose'],26);text(xx+55,y+37,title,16,C['ink'],600);para(xx+18,y+78,body,cw-36,14,gap=23)
  if kind=='girar':rect(xx+18,y+139,cw-36,5,'#E4E4E0',2);rect(xx+18,y+139,(cw-36)*.42,5,C['rose'],2)
  if kind=='check':rect(xx+18,y+123,cw-36,31,C['soft'],8);text(xx+cw/2,y+144,'Revisar resultado',12,C['rose'],500,'middle')
def printer(x,y,w=460,h=450):
 rect(x+12,y+24,w-24,h-24,'#151B20',24);rect(x,y,w,h-30,'#333C44',22,'#64727E');rect(x+25,y+30,w-50,h-105,'#10171D',12);line(x+55,y+60,x+w-55,y+60,'#71808C',4);line(x+w*.4,y+60,x+w*.4,y+h*.37,'#9FAAB0',5);rect(x+w*.4-28,y+h*.34,56,45,'#9CA7B0',8);path(f'M{x+50} {y+h*.69}L{x+w*.74} {y+h*.58}L{x+w-45} {y+h*.74}L{x+w*.3} {y+h*.86}Z','#48535D','#7C8993',2);icon(x+w*.44,y+h*.51,'cube','#D88CA6',72);rect(x+32,y+h-67,w-64,14,'#727A7E',4);circle(x+w-30,y+h-48,4,C['green'])
def devhead(title,sub='Unidade local / sessão do titular'):
 boot('Dispositivo',title,sub)
def camera(x=72,y=326,w=1140,h=638):
 art(x,y,w,h,'camera-generica.png');rect(x+20,y+20,270,33,'#252C33',10,'#57616B');icon(x+30,y+27,'camera','#D6DCE1',20);text(x+61,y+42,'Ilustração 3D · não é feed real',12,'#E2E7EB',500)
def motif(x,y,w,h,k,col='#CB7795'):
 # Autoral vetorial dimensional. Não é malha nem resultado de geração.
 s.append(f'<g transform="translate({x},{y}) scale({w/500},{h/400})">')
 s.append('<ellipse cx="250" cy="355" rx="175" ry="19" fill="#050A0F" opacity=".4"/>')
 if k in ['lua-litofania','lua']:
  circle(250,186,138,'#F2DDC7','#B8AA9F',2)
  for cx,cy,r in [(185,111,23),(290,224,41),(278,110,16),(172,227,18),(318,155,26),(220,286,16)]:circle(cx,cy,r,'#DDC5B0');circle(cx-4,cy-4,r*.65,'#EAD3BD')
  path('M163 321L338 321L363 353H138Z','#9A6475');rect(164,317,173,16,'#DB9CB4',7)
 elif k in ['pote-doces-imagem','pote']:
  path('M110 137L390 137L365 326Q250 370 135 326Z',col,'#E6A8BD',2);s.append('<ellipse cx="250" cy="139" rx="140" ry="38" fill="#E9AFBF"/>');s.append('<ellipse cx="250" cy="118" rx="152" ry="40" fill="#EFCCD4"/>');circle(250,101,24,'#B96787');icon(220,190,'spark','#F4D4DC',70)
 elif k in ['chaveiro-botton','chaveiro-tracos','chaveiro-alto-relevo']:
  circle(250,75,30,'none','#E7B3C4',15);rect(151,108,202,229,'#8B3F5C',55);rect(140,98,202,229,col,55,'#F4C0D2');circle(241,205,65,'#F0D8D2');path('M202 241L214 190L227 175L240 195L265 185L278 235Z','#A35572');circle(223,208,3,'#F8E4EB');circle(254,208,3,'#F8E4EB')
 elif k=='envelope-renda':
  rect(90,118,320,208,'#8B4B65',13);rect(81,107,320,208,'#DFB8C6',13);path('M81 107L241 247L401 107','#E9CFD8','#B27D95',2)
  for i in range(12):circle(96+i*26,298,8,'none','#AD7890',2)
  circle(241,225,27,'#AE6D88');text(241,232,'A&B',13,'#F8E8ED',600,'middle')
 elif k=='criador-trofeu':
  rect(126,307,255,39,'#9C6C7A',10);path('M207 294L292 294L307 310H193Z','#D4A2B2');rect(232,204,34,90,'#E7B6A8',8);path('M139 61H361L339 157Q324 215 250 228Q178 215 160 157Z','#D69A73','#F0C6A9',2);path('M143 85H96Q88 192 179 188M358 85H404Q411 192 324 188',stroke='#C59172',w=16);icon(220,109,'spark','#F9DBC4',62)
 elif k=='caixa-sombra':
  rect(96,55,323,299,'#795568',17);rect(80,42,323,299,'#E7C9CF',17);rect(106,67,271,248,'#56394B',7)
  for i,c in enumerate(['#966E81','#B78B9E','#D3A8B9','#F0CEDB']):path(f'M{116+i*8} {304-i*8}L{116+i*8} {220-i*23}Q235 {80+i*10} {367-i*8} {215-i*15}L{367-i*8} {304-i*8}Z',c)
 elif k=='flexi-articulado':
  for i in range(9):
   xx=80+i*39;yy=240+35*math.sin(i*.8);rect(xx,yy,43,62,col,18,'#F0B6CB');line(xx+12,yy+8,xx+12,yy+54,'#7F435D',2)
  circle(420,224,46,col,'#EFB9CE',2);circle(438,216,7,'#242A30');circle(440,213,2,'white')
 elif k=='ejetor-docinhos':
  for xx,c in [(135,'#D9A7BB'),(330,'#B77793')]:
   path(f'M{xx-60} 185Q{xx-85} 106 {xx} 126Q{xx+85} 105 {xx+61} 185L{xx+80} 285Q{xx} 340 {xx-80} 285Z','none',c,20)
  rect(287,207,83,28,'#EAC8D6',10);rect(319,155,20,72,'#EAC8D6',4)
 elif k in ['letra-grande','letra-grande-nome']:
  text(257,316,'A',320,'#784454',600,'middle');text(243,301,'A',320,col,600,'middle');text(253,266,'Ana',66,'#F6E6DE',600,'middle')
 elif k in ['quadro-relevo','placa-pintura','foto-linear-3d','imagem-multipartes']:
  rect(83,65,337,285,'#865365',12);rect(70,52,337,285,'#E7CCCA',12);rect(89,70,298,246,'#BC9CA6',4)
  if k=='foto-linear-3d':
   for i in range(30):line(105+i*9,90,105+i*9,300,'#F5E5DD',4+(i%3))
  elif k=='imagem-multipartes':
   path('M110 284L206 104L253 227Z','#DF9CAE');path('M264 230L220 91L352 148Z','#D6B89B');path('M214 295L362 172L368 298Z','#8DA99B')
  else:path('M111 283L161 203L213 246L276 134L369 283Z','#E8D3C5');circle(151,131,25,'#F4E6D3')
 elif k=='topo-bolo':
  rect(172,233,10,113,'#B36F89',3);rect(331,233,10,113,'#B36F89',3);text(250,227,'Ana',140,col,600,'middle');path('M92 251Q250 278 411 251',stroke='#EBC0D2',w=9)
 elif k=='arvore-natal':
  rect(231,278,35,71,'#A88772',4)
  for i in range(4):path(f'M250 {64+i*50}L{380-i*22} {287-i*37}H{120+i*22}Z', ['#789C89','#8BAC98','#AAC7B4','#C8DDC9'][i])
  for xx,yy in [(210,201),(286,251),(250,154)]:circle(xx,yy,12,'#E9A5B5')
 elif k=='luminaria-placa':
  rect(90,88,322,218,'#705667',43);rect(78,74,322,218,'#DAB0C3',43,'#F8DBB5');text(240,209,'AMOR',75,'#FFF2D8',600,'middle');rect(130,315,226,26,'#997484',8)
 elif k=='porta-chaveiros-rotatorio':
  rect(241,80,17,263,'#A0A8AD',6)
  for yy in [113,204]:
   s.append(f'<ellipse cx="250" cy="{yy}" rx="143" ry="35" fill="#DAA7BC"/>')
   for xx in [125,185,310,370]:line(xx,yy+8,xx,yy+50,'#EDCCD9',7)
  s.append('<ellipse cx="250" cy="342" rx="114" ry="20" fill="#C98AA5"/>')
 elif k in ['termoformagem','presente-termoformado']:
  for xx,yy in [(75,150),(260,90)]:
   path(f'M{xx} {yy+35}L{xx+126} {yy}L{xx+181} {yy+66}L{xx+49} {yy+100}Z','#E7BECE');path(f'M{xx+49} {yy+100}L{xx+181} {yy+66}V{yy+174}L{xx+49} {yy+208}Z','#AB6F8D');path(f'M{xx} {yy+35}L{xx+49} {yy+100}V{yy+208}L{xx} {yy+145}Z',col)
   if k=='presente-termoformado':line(xx+23,yy+63,xx+153,yy+29,'#F8E3D2',14)
 else:icon(170,115,'cube',col,160)
 s.append('</g>')
# DEVICE 01-26 — compositions differentiated by phase
states=json.loads((D/'fontes/estados-dispositivo.json').read_text())
for d in states:
 n=d['id'];id=f'DEV-{n:02d}'
 if n==11:continue
 devhead(d['title'],d['sources']+' / proposta de composição C')
 if n==1:
  rect(72,322,1050,768,C['dark'],22);printer(366,430,430,450);text(597,993,'Seu espaço de impressão começa aqui',26,'#EBE9E6',500,'middle')
  for i,(t,b) in enumerate([('Identificar','Descobrir serviços locais ou informar endereço autorizado.'),('Cadastrar','Associar unidade, modelo e perfil sem confundir com online.'),('Parear','Acesso local do titular; autorização separada.'),('Verificar','Capacidades observadas antes de liberar comandos.')]):block(1180,367+i*155,t,b,668)
  button(1180,1043,668,'Adicionar impressora',True,'plus')
 elif n==2:
  field(72,318,700,'Buscar unidade','Nome, modelo ou identificador');tag(820,336,155,'Todas');tag(997,336,155,'Disponíveis',C['muted'],'#E9EAE6');tag(1174,336,155,'Sem conexão',C['muted'],'#E9EAE6')
  for i,(name,status) in enumerate([('Bancada 01','Pronta · leitura atual'),('Bancada 02','Imprimindo · 62 %'),('Oficina','Offline · última leitura expirada')]):
   x=72+i*602;rect(x,416,574,604,'#EFEDE8',20);printer(x+98,455,374,310);text(x+28,815,name,28,C['ink'],600);text(x+28,860,status,15,C['green'] if i<2 else C['muted']);button(x+28,927,518,'Abrir unidade',i==0,'chevron-right')
  text(72,1085,'Cadastro não é presença. Estado desconhecido não é apresentado como online.',16,C['muted'])
 elif n in [3,4,5,6,9,26]:
  # Ordered assistive column, broad task body, no permanent dashboard boxes.
  steps=['Descoberta','Identidade','Cadastro e acesso','Perfis e recursos','Revisão']
  for i,t in enumerate(steps):
   yy=369+i*78;circle(96,yy-5,15,C['rose'] if i==min(4,{3:0,4:1,5:2,6:3,9:4,26:0}[n]) else '#E4E5E1');text(96,yy,str(i+1),12,'white' if i=={3:0,4:1,5:2,6:3,9:4,26:0}[n] else C['muted'],500,'middle');text(130,yy,t,15,C['ink'],500)
  line(370,322,370,1113)
  if n==3:
   text(420,361,'Buscar apenas na rede autorizada',29,C['ink'],600);field(420,409,620,'Interface local','Selecionar interface');button(420,499,620,'Autorizar descoberta',True,'wifi');notice(420,587,620,'Nenhuma busca neste estudo','Candidato descoberto não é cadastrado nem pareado.')
   listrows(1100,351,738,[('Serviços permitidos','Priorizar protocolo e modelo, não varredura cega.','01'),('Candidato A','Identificador anunciado · revisar evidências.','Revisar'),('Candidato B','Modelo ainda não confirmado.','Não pareado')],145)
   strip([('Buscando','Aguardando serviços na interface escolhida.','girar'),('Nada encontrado','Ver rede local e endereço manual autorizado.','wifi'),('Sem permissão','Pedir autorização; não tentar outra interface.','lock')],905,420,1428)
  elif n==4:
   printer(451,427,430,430);text(952,368,'Confirmar a identidade',29,C['ink'],600);listrows(952,418,896,[('Fabricante / modelo','Origem do anúncio e serviços compatíveis.','Não pareada'),('Identificador persistente','Comparar evidências; IP não é identidade.','Revisar'),('Conflito de informações','Nome ou variante não coincidem.','Bloqueado')],131);notice(952,844,896,'Não decidir pelo nome ou IP','Resolver a ambiguidade antes de cadastrar.');button(952,1009,420,'Revisar evidências',True,'shield')
  elif n==5:
   text(420,365,'Perfil de impressão',27,C['ink'],600)
   for i,(l,v) in enumerate([('Fabricante','Elegoo'),('Modelo / variante','Centauri Carbon 2'),('Bico / revisão','0,4 mm · confirmar'),('Nome local','Bancada 01')]):field(420,410+i*115,620,l,v)
   text(1100,365,'Acesso local separado',27,C['ink'],600);field(1100,410,748,'Endereço manual','IP privado ou hostname');field(1100,525,748,'Método autorizado','Resolver por modelo / protocolo');notice(1100,642,748,'Credenciais ficam locais','Inserção protegida no produto; nenhum código real neste desenho.');checks(1100,813,['Confirmo a unidade e a titularidade','Autorizo o pareamento desta sessão'],748);button(1100,1000,748,'Salvar cadastro e revisar acesso',True,'lock')
  elif n==6:
   text(420,365,'Somente os perfis desta unidade',29,C['ink'],600);listrows(420,414,1428,[('Impressora','Fabricante, modelo e revisão do módulo OEM.','Selecionado'),('Materiais e processo','Bico, mesa e presets compatíveis.','Seletivo'),('Cache verificado','Sem rede: só usar pacote íntegro e compatível.','Condicional')],128);text(420,891,'42 %',61,C['ink'],500);rect(420,927,1428,7,'#DEE1DD',3);rect(420,927,600,7,C['rose'],3);strip([('Carregando','Download seletivo; pode retomar com identidade válida.','girar'),('Falha de integridade','Preservar versão anterior. Sem ativar pacote incompleto.','shield'),('Concluído','Instalado não prova impressão física.','check')],965,420,1428)
  elif n==9:
   text(420,365,'Revisão da prontidão',29,C['ink'],600);listrows(420,406,1428,[('Identificação e cadastro','Confirmados no cenário; não equivalem a homologação.','Revisado'),('Acesso e perfis','Exigem verificação real de sessão e módulo.','Pendente'),('Câmera e recursos','Sem frame real não há aprovação.','NOT RUN'),('Impressão física','Três provas independentes por modelo.','NOT RUN')],127);notice(420,984,1428,'Liberação condicional','Nenhuma unidade liberada por este desenho. G2/G3 BLOCKED · G4 NOT RUN · G5/G6 BLOCKED.')
  else:
   text(420,365,'Protocolos primeiro. Código local depois.',29,C['ink'],600)
   for i,(t,b) in enumerate([('1. Descobrir e identificar','Consultar os serviços permitidos e confirmar o modelo da unidade.'),('2. Abrir o tutorial do modelo','Habilitar LAN na própria impressora conforme manual OEM.'),('3. Parear e verificar','Código permanece no cliente. Sem usar conta cloud como atalho.')]):block(420,432+i*190,t,b,1428,'wifi' if i==0 else 'lock')
   button(420,1040,680,'Ver orientação específica do modelo',True,'help')
 elif n in [7,10,12,13,15,20,25]:
  if n==25:
   rect(72,326,1058,702,C['dark'],22);icon(490,496,'lock','#D9A1B6',116);text(601,694,'Nenhum acesso remoto ativo',33,'#F2F0ED',500,'middle');text(601,742,'FUTURO · DESLIGADO POR PADRÃO',14,'#B6C0C6',500,'middle')
   for i,(t,b) in enumerate([('Só o titular','Autorização granular, revogável e com duração definida.'),('Administrador observa','Servidor nunca comanda impressoras dos clientes.'),('Sem controles remotos agora','Fila e agenda não autorizam iniciar; câmera não é enviada.')]):block(1180,366+i*206,t,b,668)
   button(1180,1050,668,'Ver escopo futuro',False,'lock')
  else:
   camera(72,326,1140,638)
   if n==20:
    rect(180,571,352,224,'none',12,'#E8A5BE',extra='stroke-width="3"');tag(202,591,195,'Objeto 02 selecionado')
   if n==7:
    title='Câmera local e privada';body='Ilustração não prova frame. Verificar origem, capacidade e consentimento antes de habilitar.';actions=['Autorizar prévia local','Revogar acesso'];rows=[('Origem','Rede local / sessão do titular'),('Sem frame','Não aprovar o teste'),('Privacidade','Sem envio de imagem ao servidor')]
   elif n==10:
    title='Bancada 01 pronta';body='Pronta descreve o cenário de interface. Nenhuma impressora real está conectada.';actions=['Revisar envio','Capacidades e saúde'];rows=[('Estado','Pronta / leitura atual'),('Bico / mesa','25 °C / 24 °C'),('Trabalho','Nenhum trabalho em execução')]
   elif n==12:
    title='Trabalho pausado';body='Retomar exige suporte OEM, identidade reconciliada, sessão e confirmação local.';actions=['Revisar retomada','Cancelar trabalho'];rows=[('Progresso','62 % / preservado'),('Motivo','Pausa confirmada no cenário'),('Retomada','Indisponível sem capacidade')]
   elif n==13:
    title='Revisar antes de enviar';body='Arquivo próprio preserva direitos normais. Catálogo Filuna print-only não libera geometria exportável.';actions=['Confirmar envio local','Voltar para Visualização'];rows=[('Arquivo','suporte-orbital.gcode'),('Destino registrado','Bancada 01 / confirmar identidade'),('Materiais / perfil','PLA / 0,20 mm')]
   elif n==15:
    title='Iniciar este trabalho?';body='Autorização específica para arquivo e unidade. Sucesso só após ACK e reconciliação do início.';actions=['Confirmar início local','Ainda não iniciar'];rows=[('Arquivo recebido','Integridade antes do comando'),('Destinatário','Bancada 01 / titular'),('Antes de iniciar','Mesa limpa, material e capacidade')]
   else:
    title='Pular objeto 02?';body='Ação irreversível, somente se o OEM fornecer lista e capacidade. Não permitir pular o último objeto.';actions=['Revisar exclusão do objeto','Manter todos os objetos'];rows=[('Objeto selecionado','02 / identificador OEM'),('Objetos restantes','Pelo menos um objeto imprimível'),('Permissão','Titular / sessão / confirmação')]
   text(1264,366,title,26,C['ink'],600);para(1264,408,body,584,15);listrows(1264,530,584,[(l,v) for l,v in rows],115)
   button(1264,948,584,actions[0],True,'shield');button(1264,1008,584,actions[1],False,'chevron-right');text(72,1034,'LAN LOCAL',11,C['muted'],600);text(72,1075,'Registro ≠ online · Envio ≠ ACK ≠ início · nenhuma ação executada',16,C['muted'])
 elif n in [8,17]:
  for i,col in enumerate(['#B94D72','#E88759','#BDC4CD','#7E686F']):
   x=72+i*450;rect(x,326,426,429,'#EFEDE8',18);circle(x+210,470,96,col);circle(x+210,470,36,'#F8F7F3');text(x+24,620,'Slot '+str(i+1),26,C['ink'],600);text(x+24,658,'PLA / associação a confirmar',15,C['muted']);text(x+24,701,'Restante não informado',14,C['muted'])
  listrows(72,801,1112,[('Bobina não é perfil','Estoque, preset e recurso físico têm identidades separadas.'),('Associação humana','Confirmar material e slot; não inventar peso ou saldo.')],120);notice(1240,820,608,'Mesma família de material','Vínculo não autoriza carregar ou descarregar na máquina.');button(1240,1005,608,'Revisar associação',True,'settings')
 elif n==14:
  text(72,386,'Transferir com integridade',37,C['ink'],600);text(72,430,'suporte-orbital.gcode → Bancada 01',20,C['muted']);text(72,595,'62',126,C['ink'],500);text(233,588,'%',42,C['muted']);rect(72,646,1080,10,'#DEE1DD',5);rect(72,646,670,10,C['rose'],5);text(72,706,'Envio ainda não concluído',19,C['rose'],500)
  for i,(t,b) in enumerate([('01 · Arquivo','G-code preservado byte a byte. Verificação por SHA.'),('02 · Recebimento','ACK confirma recebimento, não início físico.'),('03 · Autorização','Revisar e confirmar localmente antes de iniciar.')]):block(1240,383+i*183,t,b,608)
  strip([('Falha de transferência','Não reenviar início. Conferir arquivo e destino.','shield'),('Recibo recebido','Upload finalizado; ainda sem impressão.','check'),('Reconciliação','Resultado incerto: consultar estado antes de repetir.','girar')],931)
 elif n in [16,18]:
  text(72,356,'Trabalhos locais' if n==16 else 'Resultado registrado não é peça aprovada',27,C['ink'],600)
  rows=[('Suporte orbital','Bancada 01 / quantidade 1','Aguardando'),('Caixa divisória','Bancada 02 / quantidade 2','Agendado'),('Vaso torcido','Oficina / quantidade 1','Em revisão')] if n==16 else [('Suporte orbital','Recebimento, início e conclusão separados','Concluído'),('Caixa divisória','Interrompido pelo titular','Cancelado'),('Vaso torcido','Aguardando reconciliação','Resultado incerto')]
  listrows(72,409,1090,rows,154)
  if n==16:
   field(1230,370,618,'Prioridade / quantidade','Normal / 1');field(1230,500,618,'Agenda do trabalho','Selecionar lembrete');notice(1230,629,618,'Agenda somente avisa','Produzir abre revisão. Nunca inicia impressora automaticamente.');button(1230,835,618,'Abrir revisão de produção',True,'shield');button(1230,905,618,'Salvar lembrete',False,'clock')
  else:
   checks(1230,375,['Avaliei a peça física','Adesão e acabamento aceitáveis','Material e modelo identificados'],618);field(1230,600,618,'Observação do titular','Registrar evidência / sem prova anexada');notice(1230,730,618,'Três provas independentes','Não contar três eventos de rede como três impressões.');button(1230,956,618,'Revisar avaliação física',True,'shield')
  text(72,1080,'Nenhum evento ilustrado altera os gates do projeto.',16,C['muted'])
 elif n==19:
  text(72,360,'Instrumentos locais',28,C['ink'],600)
  metric(72,423,'BICO','—','°C');metric(430,423,'MESA','—','°C');metric(788,423,'VENTILAÇÃO','—','%')
  for i,t in enumerate(['Definir alvo do bico','Definir alvo da mesa','Movimento / extrusão']):field(72,600+i*103,1020,t,'Indisponível até capacidade e sessão verificadas')
  for i,(t,b) in enumerate([('Saúde e alertas','Mostrar apenas sensores suportados e leitura atual.'),('Manutenção','Orientar inspeção. Não disparar procedimento automaticamente.'),('Confirmação local','Titular, sessão válida, capacidade e aceite específico.')]):block(1180,371+i*224,t,b,668,'shield')
 elif n==21:
  for i,(t,b,k) in enumerate([('Offline','Última leitura expirou. Comandos suspensos.','wifi'),('Sessão expirada','Revalidar localmente; não reutilizar autorização.','lock'),('Identidade mudou','Rever unidade antes de qualquer envio.','shield'),('Telemetria antiga','Mostrar horário de leitura, nunca estado online falso.','clock'),('Resultado incerto','Reconciliar antes de reenviar comando.','girar'),('Capacidade ausente','Explicar motivo, manter ação indisponível.','settings')]):
   x=72+(i%3)*602;y=326+(i//3)*382;icon(x,y,k,C['rose'],43);text(x,y+94,t,26,C['ink'],600);para(x,y+140,b,554,17);button(x,y+249,554,'Ver recuperação segura',False,'chevron-right')
 elif n==22:
  text(72,365,'Fatos antes de hipóteses',32,C['ink'],600);listrows(72,422,1040,[('Fato observado','Serviço, identidade, horário e erro retornado.'),('Hipótese de causa','Separar da evidência. Sem diagnóstico inventado.'),('Pacote de diagnóstico','Pré-visualizar e sanitizar antes de compartilhar.')],163)
  for i,(t,b) in enumerate([('Telemetria voluntária','Desligada por padrão. Opt-in específico e revogável.'),('Dados minimizados','Sem senhas, tokens, câmera ou geometrias privadas.'),('Servidor observador','Nunca inicia nem controla impressoras clientes.')]):block(1180,370+i*200,t,b,668)
  button(72,1029,1040,'Pré-visualizar diagnóstico sanitizado',True,'eye')
 elif n==23:
  printer(100,367,510,500);text(72,963,'Bancada 01',30,C['ink'],600);text(72,1014,'Identidade física ≠ preset ≠ versão do módulo',16,C['muted'])
  listrows(740,345,1108,[('Módulo OEM','Origem, versão e licença verificáveis.','Consultar'),('Motor Orca único','Pin upstream preservado; sem multiengine.','Fixo'),('Atualização seletiva','Pacote desta unidade; não atualizar em impressão.','Revisar'),('Integridade','Falha preserva versão anterior íntegra.','Fail closed'),('Rollback','Voltar à versão válida sem elevar gates.','Condicional')],130);button(740,1051,1108,'Revisar pacote e alterações',True,'download')
 elif n==24:
  for i,(t,sub) in enumerate([('Prova 01','Impressão física identificada'),('Prova 02','Impressão independente e evidência'),('Prova 03','Repetibilidade confirmada pelo titular')]):
   x=72+i*602;text(x,420,'0'+str(i+1),110,C['soft'],600);text(x,503,t,29,C['ink'],600);para(x,552,sub,554,19);tag(x,645,180,'NOT RUN',C['muted'],'#E6E8E4');line(x,728,x+554,728)
  notice(72,841,1776,'Beta não é estável','Condição da unidade não homologa o modelo. Sem promoção automática; G4 físico permanece NOT RUN.');text(72,1055,'G2 / G3 BLOCKED      G4 NOT RUN      G5 / G6 BLOCKED',25,C['ink'],500)
 out('dispositivo',id,d['title'],d['sources'])
# Preserve approved device art as DEV11 coverage; separate warm-up substate.
devhead('Aquecimento antes de imprimir','Fase anterior ao início confirmado / cenário de interface')
camera();text(1264,377,'Preparando a unidade',29,C['ink'],600);metric(1264,454,'BICO','180','/ 215 °C');metric(1264,592,'MESA','48','/ 60 °C');notice(1264,745,584,'Aguardar estado confirmado','Comando aceito não prova início de deposição.');button(1264,967,584,'Revisar cancelamento',False,'stop');out('dispositivo','DEV-11-AQUECIMENTO','Aquecimento','DEV §6')
# Preparation derivatives preserve exact anchor composition, no right info panel.
anchor=(D/'preparacao/filuna-preparacao-C.svg').read_text()
def prepbase(title):
 global s
 # retain viewport, toolbar and profiles, replace only content panel below mode.
 p=anchor.split('</svg>')[0];p=p[:p.rfind('</g>')];s=[p]
 rect(41,828,358,257,C['paper']);rect(41,610,358,216,C['paper'])
 text(64,642,'Processo',19,C['ink'],600);tag(223,620,72,'Global');text(312,641,'Objetos',12,C['muted'],500)
 text(64,684,'Simples',12,C['rose'] if title=='Simples' else C['muted']);tag(133,663,100,'Avançado',C['muted'] if title in ['Simples','Especialista'] else C['rose'], '#E9EAE6' if title in ['Simples','Especialista'] else C['soft']);text(249,684,'Especialista',12,C['rose'] if title=='Especialista' else C['muted']);field(64,711,312,'Perfil','0,20 mm Padrão @ CC2')
 for xx,tt in [(64,'Qualidade'),(165,'Resistência'),(278,'Velocidade')]:text(xx,790,tt,11,C['rose'] if title==tt else C['muted'],600 if title==tt else 400)
 for xx,tt in [(64,'Suporte'),(151,'Multimaterial'),(285,'Outros')]:text(xx,818,tt,11,C['rose'] if title==tt else C['muted'],600 if title==tt else 400)
 line(64,833,376,833)
def preppanel(fields):
 for i,(l,v) in enumerate(fields):
  yy=872+i*58;text(64,yy,l,13,C['ink']);rect(289,yy-24,87,34,'white',8,C['line']);text(332,yy-2,v,12,C['ink'],500,'middle')
prepfields={
 'PREP-03':('Qualidade',[('Altura da camada','0,20 mm'),('Primeira camada','0,20 mm'),('Largura da linha','0,42 mm'),('Onde fica a emenda','Alinhada')]),
 'PREP-04':('Velocidade',[('Parede de fora','50 mm/s'),('Preenchimento','150 mm/s'),('Primeira camada','30 mm/s'),('Deslocamento','300 mm/s')]),
 'PREP-05':('Suporte',[('Usar suporte','Sim'),('Tipo','Árvore'),('Ângulo','45°'),('Folga em cima','0,2 mm')]),
 'PREP-06':('Multimaterial',[('Torre de limpeza','Sim'),('Largura da torre','35 mm'),('Posição X / Y','10 / 10'),('Matriz de purga','Abrir')]),
 'PREP-07':('Outros',[('Aba na base','Automática'),('Saia','Sim'),('Modo vaso','Não'),('Largura da aba','5 mm')]),
 'PREP-08':('Simples',[('Altura da camada','0,20 mm'),('Voltas da parede','2'),('Densidade','15 %'),('Usar suporte','Sim')]),
 'PREP-09':('Especialista',[('Modo das paredes','Arachne'),('Resolução','0,012 mm'),('Fechar vãos','0,049 mm'),('Usar arcos','Sim')])}
for id,(title,fields) in prepfields.items():prepbase(title);preppanel(fields);out('preparacao',id,title+' / processo','UX-PREP §4')
prepbase('Objetos');rect(64,661,312,102,C['paper']);field(64,681,312,'Buscar por nome','Suporte');listrows(64,784,312,[('Suporte orbital','Placa 01 / slot 1','1×'),('Cubo','Fora da mesa / slot 3','1×')],106);rect(440,520,345,466,'#F8F7F3',15,C['line']);listrows(460,534,305,[('Cor do filamento','Slot 1 / PLA'),('Cópias','Mais uma cópia / encher mesa'),('Arrumar','Centralizar / deitar / espelhar'),('Imprimir','Pode / não imprimir / voltar'),('Excluir','Remover apenas o objeto')],84);out('preparacao','PREP-02','Objetos e menu contextual','UX-PREP §4 Objetos')
# Editors use modal sheets over same viewport, grouped by level.
for id,title,groups in [
 ('PREP-10','Impressora',[('A máquina',[('Nome','Bancada 01'),('Diâmetro do bico','0,4 mm'),('Mesa','PEI texturizada'),('Área útil','256 × 256 × 256 mm')]),('Movimento',[('Mecanismo','CoreXY'),('Firmware','Klipper'),('Velocidade máxima','500 mm/s'),('Aceleração máxima','20000 mm/s²')]),('Especialista',[('Extrusoras','1'),('G-code de início','Origem do perfil OEM'),('G-code de fim','Origem do perfil OEM'),('Malha só onde imprime','Sim')])]),
 ('PREP-11','Filamento',[('O filamento',[('Nome','PLA Rosa mineral'),('Material','PLA'),('Diâmetro','1,75 mm'),('Temperatura do bico','220 °C')]),('Fluxo',[('Temperatura da mesa','60 °C'),('Quanto sai de plástico','0,98'),('Fluxo máximo','15 mm³/s'),('Avanço de pressão','0,02')]),('Avançado / especialista',[('Retração','0,8 mm'),('Ventoinha','30–100 %'),('Preço do quilo','R$ 90'),('Temperatura em que amolece','45 °C')])])]:
 prepbase(title);rect(40,156,1840,1004,'#10151A',extra='opacity=".45"');rect(260,265,1400,808,C['paper'],22);text(302,322,title,31,C['ink'],600);text(1618,320,'×',28,C['muted'],400,'end');tag(1365,296,200,'Níveis sincronizados')
 for i,(g,fs) in enumerate(groups):
  x=302+i*453;text(x,388,g,20,C['ink'],600)
  for j,(l,v) in enumerate(fs):field(x,438+j*111,410,l,v)
 text(302,959,'Valores de referência do contrato · não gravados · não alteram G-code já gerado',15,C['muted']);button(1295,995,323,'Pronto',True,'check');out('preparacao',id,'Editor '+title,'UX-PREP §§2–4')
prepbase('Purga na troca');rect(40,156,1840,1004,'#10151A',extra='opacity=".45"');rect(452,273,1020,806,C['paper'],22);text(494,335,'Purga na troca de cor',31,C['ink'],600);text(494,376,'Volume em mm³ / diagonal 0 / intervalo 26–900',15,C['muted'])
for i in range(5):
 for j in range(5):
  x=534+j*170;y=423+i*88
  if i==0 or j==0:
   if i+j>0:text(x+60,y+33,'Slot '+str(max(i,j)),15,C['ink'],500,'middle')
  else:rect(x,y,128,58,'#E8E7E3' if i==j else 'white',10,C['line']);text(x+64,y+35,'0' if i==j else str(96+((i+j)*23)%160),17,C['muted'] if i==j else C['ink'],500,'middle')
field(494,913,425,'Multiplicador / 0–3','0,3');button(950,925,460,'Usar estes volumes',True,'check');out('preparacao','PREP-12','Matriz de purga','UX-PREP §3')
# Context overlays differentiated
for id,title in [('PREP-13','Criar impressora'),('PREP-14','Filamento / ações'),('PREP-15','Gestão de mesas'),('PREP-16','IA Filuna'),('PREP-17','Autoajuda'),('PREP-18','Fatiar mesa ou tudo'),('PREP-19','Exportar e imprimir'),('PREP-20','Custos pós-fatiamento')]:
 prepbase(title)
 if id=='PREP-13':
  rect(460,340,620,315,C['paper'],18);text(490,391,'Criar perfil de impressora',25,C['ink'],600);field(490,434,560,'Nome obrigatório','');text(490,522,'Informe um nome antes de criar.',14,C['rose']);button(490,556,560,'Criar',False,'plus')
 elif id=='PREP-14':
  rect(440,451,500,360,C['paper'],18);listrows(464,468,452,[('Editar','Abrir editor do filamento ativo.'),('Mesclar','Revisar o destino antes de unir perfis.'),('Excluir','Indisponível para o último filamento.')],101)
 elif id=='PREP-15':
  rect(1450,340,350,316,C['paper'],18);listrows(1474,359,302,[('Adicionar mesa','Nova placa vazia.'),('Duplicar mesa','Copiar placa selecionada.'),('Remover mesa','Não excluir a última.')],90)
 elif id=='PREP-16':
  rect(440,544,650,501,C['paper'],18);text(468,589,'IA Filuna / orientações',25,C['ink'],600);listrows(468,619,592,[('Orientar na mesa','Revisar maior face para reduzir altura.'),('Suportes','Revisar vãos e capacidade de ponte.'),('Perfil','Equilíbrio entre detalhe e prazo.'),('Venda','Peso e preço só após fatiamento válido.')],90)
 elif id=='PREP-17':
  rect(442,522,630,493,C['paper'],18);motif(474,576,230,190,'imagem-multipartes');text(744,585,'Densidade',25,C['ink'],600);tag(744,611,125,'Avançado');para(744,688,'Quanto material ocupa o interior. Não muda o contorno.',280,15);text(474,849,'Padrão: 15 %',19,C['ink'],500);text(474,890,'sparse_infill_density',16,C['rose']);text(474,958,'Quadro estático de animação proposta.',14,C['muted'])
 elif id=='PREP-18':
  rect(1435,164,400,237,C['paper'],16);listrows(1460,181,350,[('Fatiar mesa','Somente a placa ativa.'),('Fatiar tudo','Todas as placas válidas.')],93)
 elif id=='PREP-19':
  rect(1276,164,455,274,C['paper'],16);listrows(1300,182,407,[('Imprimir','Abrir revisão de envio local.'),('Exportar G-code','Arquivo próprio / resultado imutável.')],102);text(1300,418,'Enviar não inicia a máquina.',13,C['rose'])
 else:
  rect(440,830,1070,208,C['paper'],18)
  for x,l,v in [(472,'Peso','58,6 g'),(733,'Custo','R$ 7,38'),(994,'Tempo','03h24'),(1255,'Venda sugerida','R$ 24,90')]:text(x,882,l,13,C['muted']);text(x,937,v,27,C['ink'],600)
  text(472,997,'Exemplo após fatiar · valores dependem dos custos definidos por você.',15,C['muted'])
 out('preparacao',id,title,'UX-PREP §§1–9; t23u/t26u')
# Visualization stages U1, each risk has dedicated composition
for id,title in [('VIS-02','Antes de fatiar'),('VIS-03','Fatiando com Orca'),('VIS-04','Falha de fatiamento'),('VIS-05','Arquivo inválido'),('VIS-06','Risco no G-code oficial OEM'),('VIS-07','ENGINE_DIVERGENCE')]:
 boot('Visualização',title,'Análise somente leitura / G-code imutável / nenhuma execução real')
 if id in ['VIS-02','VIS-03']:
  art(72,324,1190,745,'mesa-suporte-orbital.png');text(1314,383,'Prévia ainda não disponível',27,C['ink'],600)
  if id=='VIS-02':para(1314,441,'Fatie uma placa válida para visualizar camadas e estimativas.',534,18);button(1314,958,534,'Voltar à Preparação',True,'camadas')
  else:text(1314,582,'42 %',77,C['ink'],500);rect(1314,638,534,7,'#DDE0DC',3);rect(1314,638,224,7,C['rose'],3);text(1314,694,'Aguardar término sem apresentar sucesso.',15,C['muted']);button(1314,958,534,'Revisar cancelamento',False,'stop')
 elif id in ['VIS-04','VIS-05']:
  icon(100,370,'shield',C['rose'],80);text(100,530,'Não foi possível continuar',41,C['ink'],600);para(100,594,'Erro retornado deve ser exibido com causa e próximo passo. Não apresentar arquivo incompleto como válido.',1000,20);notice(100,766,1010,'Nenhum envio disponível','Preservar o projeto. Voltar à preparação para revisar a entrada.');button(100,958,1010,'Revisar arquivo e parâmetros',True,'folder');listrows(1200,379,648,[('Diagnóstico','Origem do arquivo e detalhe sanitizado.'),('Estado','Sem resultado de fatiamento válido.'),('Recuperação','Corrigir entrada, gerar novamente.')],158)
 else:
  rect(72,326,1030,732,C['dark'],20);text(110,382,'G-CODE / LEITURA SOMENTE',13,'#C5CBD0',600)
  for i,l in enumerate((['; Origem OEM preservada','G1 X63.569 Y147.185 Z3.05','; linha 38911 / camada 14','; Nenhuma correção inserida','; SHA do arquivo identifica o aceite'] if id=='VIS-06' else ['; Filuna ≠ baseline OEM','; Comparação de resultados divergiu','; ENGINE_DIVERGENCE','; Nenhuma correção inserida','; Bloqueado / sem aceite e sem override'])):text(110,471+i*72,l,23,'#E7C7D2' if i==1 else '#D0D5D8',400)
  text(1164,369,'Risco OEM' if id=='VIS-06' else 'Divergência do motor',30,C['ink'],600)
  if id=='VIS-06':
   para(1164,425,'O risco está no resultado oficial do fabricante. O arquivo permanece imutável; aceite é específico por usuário e SHA.',684,18);checks(1164,597,['Li o risco geométrico e sua origem OEM','Entendo o risco para esta unidade e arquivo','Confirmo aceite auditável deste SHA'],684);notice(1164,800,684,'Mudou o arquivo? Novo aceite','Não reutilizar autorização em outro hash.');button(1164,984,684,'Revisar aceite por SHA',True,'shield')
  else:para(1164,425,'Filuna divergiu do resultado oficial. Não existe aceite de fabricante nem override para prosseguir.',684,21);notice(1164,666,684,'Envio bloqueado','Investigar paridade e gerar novamente. Não reescrever movimentos.');button(1164,984,684,'Voltar à Preparação',True,'shield')
 out('visualizacao',id,title,'PLANO/CONT/QUAL @83efc13; regra U1')
# Calibration: tutorial-specific diagrams, four mandatory sections
cal=[('CAL-02','Fluxo máximo','A vazão que o material mantém sem perda.','Trechos de extrusão crescente e falhas de adesão.','Aumenta a vazão por faixas.','Filamento / fluxo máximo'),('CAL-03','Avanço de pressão','Compensar a resposta do fluxo nas mudanças de velocidade.','Linhas e quinas com excesso ou falta de material.','Varia o parâmetro de pressão no teste.','Filamento / avanço de pressão'),('CAL-04','Taxa de fluxo','Ajustar a quantidade efetiva de plástico.','Superfícies com lacunas ou excesso no topo.','Deposita regiões com fatores de fluxo distintos.','Filamento / quanto sai de plástico'),('CAL-05','Retração','Reduzir fios entre regiões separadas.','Fios, falhas e marcas na retomada.','Varia a retração conforme o modelo do teste.','Filamento / retração'),('CAL-06','Quinas','Comparar o comportamento nas mudanças de direção.','Cantos, ondulações e regularidade da parede.','Percorre quinas nas condições do teste.','Processo / velocidade e aceleração'),('CAL-07','Modelagem de entrada','Entender ressonâncias e compensação de movimento.','Ondulações depois de arestas e padrões repetidos.','Executa o teste selecionado do submenu.','Firmware / input shaper compatível'),('CAL-08','VFA','Investigar artefatos verticais finos.','Faixas verticais e mudanças por velocidade.','Varia a velocidade ao longo da peça.','Processo / faixa de velocidade'),('CAL-09','Tolerância','Comparar folgas e encaixes reais.','Encaixe livre, firme ou bloqueado.','Imprime pares com folgas graduais.','Projeto / tolerância do encaixe'),('CAL-10','Guia de calibração','Organizar a sequência e registrar evidência física.','Resultados de temperatura, fluxo e movimento.','Não inicia teste por esta tela de tutorial.','Perfil validado após peça física')]
for id,title,a,b,c,d in cal:
 boot('Calibração',title,'Tutorial / modelo de referência / resultado depende da avaliação física')
 rect(72,326,760,655,C['dark'],20)
 if id=='CAL-09':
  for i in range(5):rect(140+i*123,535,88,110,'#CC91A9',9);circle(184+i*123,585,22+i*2,C['dark']);text(184+i*123,702,f'{.1+i*.05:.2f}',16,'#EACAD8',500,'middle')
 elif id=='CAL-04':
  for i in range(3):rect(140+i*213,530,178,178,'#BC7997',8);[line(149+i*213,545+j*14,307+i*213,545+j*14,'#EDBED0',2+i) for j in range(11)]
 else:
  for i in range(6):
   xx=153+(i%3)*217;yy=444+(i//3)*235;path(f'M{xx} {yy+117}V{yy+25}L{xx+55} {yy}L{xx+124} {yy+40}V{yy+153}L{xx+55} {yy+117}Z','#C48BA4','#F0BFD3',2)
   for j in range(4):line(xx+3,yy+44+j*15,xx+54,yy+19+j*15,'#ECD1DB',1)
 text(452,936,'Arte vetorial ilustrativa / não é peça de teste',14,'#C8CFD4',500,'middle')
 for i,(t,body) in enumerate([('Para que serve',a),('O que você vai ver na peça',b),('O que a impressora faz',c),('Onde gravar o resultado',d)]):block(894,364+i*163,t,body,954)
 button(72,1020,760,'Baixar modelo de referência',False,'download');text(894,1055,'Comparar a peça física antes de gravar no perfil.',17,C['muted'])
 out('calibracao',id,title,'SHELL §4')
for i,t in enumerate(['Ringing tower','Fast tower','ZV','MZV','ZVD','EI'],1):
 boot('Calibração','Modelagem de entrada / '+t,'Submenu contratual / apresentação derivada C / não configura firmware automaticamente')
 rect(72,326,1020,733,C['dark'],20)
 for q in range(9):
  xx=124+q*102;points=[]
  for z in range(80):points.append((xx+z*1.15,690+math.sin(z*.34)*90*math.exp(-z/(20+i*8))))
  path('M'+' L'.join(f'{x:.1f} {y:.1f}' for x,y in points),stroke='#D8A2BA',w=2)
 text(125,390,'RESSONÂNCIA / DIAGRAMA ILUSTRATIVO',13,'#CBD1D6',600);text(125,982,t,48,'#E8D2DC',500)
 for q,(title,body) in enumerate([('Para que serve','Comparar ressonâncias usando o teste ou filtro '+t+'.'),('O que observar','Ondulação, frequência e qualidade após as arestas.'),('Ação da impressora','Depende do teste e suporte do firmware.'),('Registrar resultado','No input shaper do firmware compatível; revisão humana.')]):block(1150,363+q*166,title,body,698)
 out('calibracao',f'CAL-INPUT-{i}',t,'SHELL §4 submenu input')
# Parametric tools — specific §4 controls, input/processing/error/output variants visible
catalog=[
 ('lua-litofania','Lua Litofania LED',True,[('Diâmetro','120 mm'),('Abertura','60 mm'),('Espessura / relevo','2 / 1 mm'),('Folga LED USB 60 × 8','0,2 mm'),('Base','Incluída')],'3MF / duas mesas ou STL'),
 ('pote-doces-imagem','Pote para Doces com Imagem',True,[('Tamanho','100 mm'),('Altura','80 mm'),('Paredes','2 mm'),('Folga','0,2 mm'),('Tampa / cores','Até 8')],'3MF ou STL'),
 ('chaveiro-botton','Chaveiro Botton',True,[('Formato','Silhueta'),('Recortar o fundo','Ligado'),('Tamanho / altura','70 / 4 mm'),('Medida do botton','32 mm'),('Cor / imagem','Branco / cor real')],'3MF ou STL'),
 ('envelope-renda','Envelope de Renda em Tule',True,[('Textura de renda','Selecionar'),('Nomes','Ana e Bruno'),('Data','Selecionar'),('Lacre','Selecionar'),('Pausa para tule','Revisar')],'3MF com pausa ou STL'),
 ('criador-trofeu','Criador de Troféu',False,[('Emblema','Selecionar'),('Formato','Taça'),('Base / placa','Com encaixe'),('Nomes','Filuna'),('Cores','Duas cores')],'3MF ou STL'),
 ('caixa-sombra','Caixa de Sombra',True,[('Camadas / até 10','5'),('Vãos','2 mm'),('Moldura','10 mm'),('Cor de cada peça','Paleta de cinco cores')],'3MF ou STL'),
 ('flexi-articulado','Brinquedo Flexível',True,[('Dobras desenhadas','Revisar traçado'),('Folga do pino','0,2 mm'),('Cores','Duas cores'),('Montagem','Sai montado')],'3MF ou STL'),
 ('ejetor-docinhos','Ejetor de Docinhos',True,[('Tamanho','70 mm'),('Relevo','2 mm'),('Folga','0,2 mm'),('Parede','1,2 mm')],'3MF ou STL / não certificado alimentar'),
 ('letra-grande','Letra Grande com Textura',False,[('Letra / textura','A / linhas'),('Nome','Ana'),('Espessuras','Revisar'),('Rebaixo','1 mm'),('Tolerância','0,2 mm')],'3MF ou STL'),
 ('quadro-relevo','Quadro em Relevo com IA',True,[('Estilo','Baixo-relevo'),('Tamanho','150 mm'),('Altura','4 mm'),('Detalhe','Médio'),('Moldura','Incluída')],'3MF ou STL / 1 crédito; falha não cobra'),
 ('topo-bolo','Topo de Bolo com Nome',True,[('Nome','Ana'),('Fonte manuscrita','Selecionar'),('Cores','Duas cores'),('Tamanho','150 mm'),('Palitos','Dois')],'3MF ou STL / peça única'),
 ('chaveiro-tracos','Chaveiro com Traços',True,[('Cores','Duas cores'),('Tamanho','70 mm'),('Furo','4 mm'),('Posição da argola','Superior')],'3MF ou STL'),
 ('placa-pintura','Placa para Pintura',True,[('Limiar','128'),('Suavização','Média'),('Espessura','3 mm'),('Camadas','12 brancas / 3 pretas')],'3MF ou STL'),
 ('arvore-natal','Árvore de Natal com Enfeites',False,[('Altura','180 mm'),('Camadas','5'),('Ondas','Selecionar'),('Enfeites','Incluídos')],'3MF ou STL / sem suporte'),
 ('luminaria-placa','Luminária Placa com Palavras',False,[('Forma','Retângulo arredondado'),('Palavras','AMOR'),('Cores','Rosa / creme'),('LED','59 mm')],'3MF ou STL'),
 ('porta-chaveiros-rotatorio','Porta-Chaveiros Rotatório',False,[('Níveis','2'),('Braços','4 por nível'),('Rolamentos 608','Dois'),('Eixo','M8')],'ZIP / beta sem prova física'),
 ('chaveiro-alto-relevo','Chaveiro em Alto-Relevo',True,[('Medida','70 mm'),('Profundidade','3 mm'),('Cores','Uma cor'),('Verso','Plano')],'3MF ou STL'),
 ('termoformagem','Gerador de Termoformagem',True,[('Molde fêmea / macho','PETG'),('Placas','PLA'),('Aro','Incluído'),('Fechamento','Revisar')],'3MF ou STL'),
 ('presente-termoformado','Presente Quadrado Termoformado',False,[('Caixa quadrada','100 mm'),('Fitas','Duas'),('Laço','Incluído'),('Nome','Ana')],'3MF ou STL'),
 ('foto-linear-3d','Foto Linear 3D',True,[('Linhas vazadas','Revisar'),('Arte','Imagem anexada'),('Base com encaixe','Incluída')],'3MF ou STL'),
 ('letra-grande-nome','Letra Grande com Nome',False,[('Letra / nome','A / Ana'),('Parede / fundo','2 / 2 mm'),('Material','PLA'),('Tolerância','0,2 mm'),('Gabarito','Incluído')],'3MF, STL e SVG'),
 ('imagem-multipartes','Imagem Multipartes',True,[('Tamanho','150 mm'),('Espessuras','3 mm'),('Tolerância','0,2 mm'),('Separação','Áreas em peças')],'3MF ou STL')]
for slug,title,inp,fs,fmt in catalog:
 boot('Início',title,'Paramétricos / Voltar para galeria / parâmetros próprios do gerador Filuna')
 rect(40,284,360,876,'#F2F0EB');text(64,326,'‹ Voltar para galeria',15,C['rose'],500)
 if inp:
  button(64,359,312,'Trocar imagem',True,'upload');rect(64,426,312,75,'#DDC7CF',9);icon(194,441,'cube',C['rose'],40);text(64,524,'Miniatura / imagem anexada',12,C['muted'])
 else:text(64,384,'Ajustes específicos',20,C['ink'],600);text(64,421,'Sem imagem obrigatória.',13,C['muted'])
 yy=560 if inp else 471
 for j,(l,v) in enumerate(fs):field(64,yy+j*86,312,l,v)
 text(64,1082,'Valores: proposta ilustrativa.',12,C['muted']);text(64,1110,'Domínios: UX-P §4.',12,C['muted'])
 rect(424,308,1424,596,C['dark'],20);motif(835,329,636,510,slug)
 text(456,350,'PRÉVIA VÁLIDA / COMPOSIÇÃO ILUSTRATIVA',12,'#CAD1D7',600);text(456,866,fmt,14,'#E1C8D3',500);button(1580,833,238,'Gerar',True,'cube')
 # Four genuinely drawn component variants: no preview in entry tile.
 y=942;cw=342
 for i,(t,k) in enumerate([('Entrada','upload' if inp else 'cube'),('Gerando','girar'),('Falha','shield'),('Gerado','check')]):
  x=424+i*(cw+18);rect(x,y,cw,174,'white',13,C['line']);icon(x+17,y+17,k,C['rose'],24);text(x+54,y+35,t,16,C['ink'],600)
  if i==0:
   rect(x+17,y+54,cw-34,60,C['dark'],8);text(x+cw/2,y+89,'Anexar imagem / prévia vazia' if inp else title,11,'#D8D8DD',500,'middle');text(x+17,y+143,'Gerar indisponível' if inp else 'Nome até existir malha real',12,C['muted'])
  elif i==1:
   text(x+17,y+83,'Processando · 42 %',14,C['muted']);rect(x+17,y+109,cw-34,5,'#E1E4DF',2);rect(x+17,y+109,130,5,C['rose'],2);text(x+17,y+148,'Sem arquivo concluído.',12,C['muted'])
  elif i==2:
   text(x+17,y+82,'Falha de geração',15,C['rose'],500);text(x+17,y+113,'Revisar entrada e tentar de novo.',12,C['muted']);text(x+17,y+147,'Nenhum arquivo válido.',12,C['muted'])
  else:
   text(x+17,y+81,'Arquivo válido no cenário',14,C['muted']);rect(x+17,y+104,cw-34,43,C['soft'],10);text(x+cw/2,y+132,('Exportar ZIP / beta' if slug=='porta-chaveiros-rotatorio' else 'Preparar / exportar 3MF'),12,C['rose'],500,'middle')
 out('parametricos','PAR-'+slug,title,'UX-P §§2–4',{'entrada':'faixa inferior / 1ª variante (x424,y942)','prévia':'render principal (x424,y308)','gerando':'faixa inferior / 2ª variante (x784,y942)','falha':'faixa inferior / 3ª variante (x1144,y942)','gerado':'faixa inferior / 4ª variante (x1504,y942)'})
# Full parametric gallery in three viewport pages, authored representative cover art
for pg in range(3):
 boot('Início','Paramétricos',f'Escolha um. O ajuste fica na ferramenta, não nesta galeria. / segmento {pg+1} de 3')
 for q,row in enumerate(catalog[pg*8:pg*8+8]):
  slug,title,inp,fs,fmt=row;x=72+(q%4)*450;y=316+(q//4)*393;rect(x,y,426,276,C['dark'],18);motif(x+38,y+12,350,254,slug);para(x,y+310,title,426,17,C['ink'],24);text(x,y+363,'Abrir ajustes',13,C['rose'],500)
 out('parametricos',f'PAR-GAL-{pg+1}','Galeria / segmento '+str(pg+1),'UX-P §1; capas autorais ilustrativas não substituem catálogo real')
# Home replace provisional thumbnails using actual family-specific vector forms
boot('Início','Seu ateliê de impressão','Recentes, catálogo e ferramentas / estudo estático C')
rect(40,284,226,876,'#F0EEE9')
for i,t in enumerate(['Recentes','Modelos 3D','Paramétricos','Ferramentas','Estoque','Ateliê']):text(72,342+i*69,t,16,C['rose'] if i==0 else C['muted'],500)
art(290,308,1042,397,'gato-vazado-ilustrativo.png');text(1366,353,'Preparação de impressão',24,C['ink'],600)
for yy,l,v in [(409,'Unidade registrada','Bancada 01'),(514,'Material','PLA / Rosa mineral')]:field(1366,yy,482,l,v)
button(1366,640,482,'Imprimir / revisar',True,'upload')
for row,labels in enumerate([['Lua Litofania LED','Caixa de Sombra','Troféu','Luminária'],['Imagem para 3MF','Imagem para SVG','Preço simples','Preço completo']]):
 yy=746+row*202;text(290,yy,'Recentes' if row==0 else 'Ferramentas',23,C['ink'],600)
 for j,t in enumerate(labels):
  x=290+j*394
  if row==0:rect(x,yy+20,366,104,C['dark'],12);motif(x+111,yy+24,145,96,['lua-litofania','caixa-sombra','criador-trofeu','luminaria-placa'][j])
  else:icon(x+15,yy+44,['cube','cor','clock','settings'][j],C['rose'],44)
  text(x,yy+158,t,16,C['ink'],500)
out('inicio','HOME-01','Início / capas ilustrativas autorais','SHELL §2; HOME-18; t26u')
# Auxiliary desktop screens — differing layouts
for id,title in [('HOME-02','Recentes'),('HOME-03','Catálogo Filuna / print-only'),('HOME-04','Ferramentas'),('HOME-05','Estoque'),('HOME-06','Ateliê')]:
 boot('Início',title,'Entrada pela Home / derivada C em revisão')
 if id in ['HOME-02','HOME-03']:
  for i,k in enumerate(['lua-litofania','caixa-sombra','luminaria-placa']):
   x=72+i*602;rect(x,326,574,447,C['dark'],18);motif(x+35,346,504,403,k);text(x,820,['Lua Litofania LED','Caixa de Sombra','Luminária'][i],25,C['ink'],600);text(x,861,'Arquivo próprio / abrir projeto' if id=='HOME-02' else 'Licença de impressão / sem exportar geometria',14,C['muted']);button(x,913,574,'Abrir projeto' if id=='HOME-02' else 'Revisar impressão',i==0,'folder' if id=='HOME-02' else 'shield')
  if id=='HOME-02':notice(72,1010,1776,'Variante / sem recentes','Lista vazia: abra um arquivo local para começar. Nenhum projeto fictício é salvo.')
  else:notice(72,1010,1776,'Proteção tem escopo','Print-only não promete DRM absoluto. Arquivos próprios preservam direitos normais.')
 elif id=='HOME-04':
  for i,(t,b) in enumerate([('Imagem para 3MF','Converter imagem em geometria / revisão de parâmetros.'),('Imagem para SVG','Preparar contornos vetoriais a partir de imagem.'),('Preço simples','Custo básico e valor sugerido.'),('Preço completo','Detalhamento de custo e margem.')]):
   x=72+(i%2)*900;y=342+(i//2)*365;icon(x,y,['cube','cor','clock','settings'][i],C['rose'],61);text(x+99,y+41,t,31,C['ink'],600);para(x,y+116,b,820,19);button(x,y+218,820,'Abrir ferramenta',i==0,'chevron-right')
 elif id=='HOME-05':
  listrows(72,342,1080,[('PLA / Rosa mineral','Bobina sem peso cadastrado.','— g'),('PLA / Laranja','Preço e saldo não informados.','—'),('PETG / Natural','Sem vínculo com slot.','Não associado')],179);text(1230,381,'Bobina não é preset',31,C['ink'],600);para(1230,442,'Registre dados reais para calcular custo. A associação não transfere material à máquina.',618,20);field(1230,648,618,'Peso disponível','Não informado');field(1230,771,618,'Preço do quilo','Não informado');button(1230,1001,618,'Revisar cadastro da bobina',True,'settings')
 else:
  rect(72,326,1110,745,C['dark'],20);motif(325,400,620,590,'criador-trofeu');text(1240,382,'Espaço de trabalho',31,C['ink'],600);para(1240,445,'Organização do ateliê ligada aos projetos e recursos do cliente. O contrato detalhado desta área permanece em revisão.',608,19);notice(1240,713,608,'Sem funções inventadas','Não inclui automação de produção, cobrança ou gestão não especificada.');button(1240,1010,608,'Voltar aos projetos',False,'folder')
 out('inicio',id,title,'SHELL §2',status='PROPOSTO_CONTRATO_DETALHADO_PENDENTE' if id=='HOME-06' else 'PROPOSTO_EM_REVISAO')
for i,title in enumerate(['Imagem para 3MF','Imagem para SVG','Preço simples','Preço completo'],1):
 boot('Início',title,'Ferramentas / campos detalhados candidatos, não implementação')
 if i<3:
  field(72,352,430,'Entrada','Anexar imagem');button(72,447,430,'Anexar imagem',True,'upload');field(72,566,430,'Tamanho / proposta','100 mm');field(72,680,430,'Detalhe / proposta','Médio');rect(550,326,1298,732,C['dark'],18);motif(870,400,620,550,'imagem-multipartes' if i==1 else 'placa-pintura');text(590,1017,'Prévia ilustrativa / só após a entrada válida',16,'#D5C8D0')
 else:
  fields=[('Material / peso','PLA / 58,6 g'),('Custo do material','R$ 5,27'),('Energia / máquina','R$ 2,11')]+([('Mão de obra','A informar'),('Perdas / margem','Definidas pelo cliente')] if i==4 else [])
  for q,(l,v) in enumerate(fields):field(72,352+q*135,840,l,v)
  text(1050,384,'Sugestão de venda',28,C['ink'],600);text(1050,521,'R$ 24,90',81,C['ink'],500);para(1050,600,'Exemplo editorial, não cálculo real. Valores só após fatiamento válido e custos informados.',798,20);notice(1050,810,798,'Preço não entra no G-code','Nenhum valor ilustrativo é gravado em projeto ou orçamento.')
 out('ferramentas',f'TOOL-{i}',title,'SHELL §2 / detalhamento proposto',status='PROPOSTO_CONTRATO_DETALHADO_PENDENTE')
# Project aux mapped to visible variants in dedicated sheet
boot('Projeto','Projeto / estados locais','Compatibilidade 3MF / proposta de detalhes, sem sincronização automática')
art(72,326,1000,744,'mesa-suporte-orbital.png');text(1130,371,'Salvar alterações?',34,C['ink'],600);para(1130,435,'Há alterações locais no projeto. Escolha salvar antes de fechar, descartar ou continuar editando.',718,21);button(1130,631,718,'Salvar projeto',True,'download');button(1130,704,718,'Continuar editando',False,'folder');text(1130,819,'Descartar alterações / confirmação separada',16,C['rose']);out('projeto','PROJ-AUX-1','Salvar alterações e fechar','QUAL compatibilidade / composição proposta',status='PROPOSTO_CONTRATO_DETALHADO_PENDENTE')
for n,t,b in [(2,'Arquivo inválido','O arquivo local não pôde ser aberto. Preservar o atual; rever formato e integridade.'),(3,'Projeto vazio','Abra um arquivo compatível ou importe uma peça para iniciar a preparação.'),(4,'Recuperação local','Verifique a cópia de recuperação antes de substituir o projeto. Sincronização não confirmada no contrato; não ativar.')]:
 boot('Projeto',t,'Estado auxiliar proposto / não implica função implementada');icon(116,388,'folder' if n==3 else 'shield',C['rose'],88);text(116,580,t,48,C['ink'],600);para(116,652,b,1250,25);button(116,943,1250,'Abrir arquivo local' if n==3 else 'Revisar sem sobrescrever',True,'folder');out('projeto',f'PROJ-AUX-{n}',t,'QUAL compatibilidade / composição proposta',status='PROPOSTO_CONTRATO_DETALHADO_PENDENTE')
for n,t in enumerate(['Conta e sessão','Configurações e temas','Atualização disponível','Instalação e rollback','Sem internet / cache'],1):
 boot('Início',t,'Auxiliares / estado estático / não altera conta nem instalação')
 if n==1:
  icon(130,366,'lock',C['rose'],85);text(130,558,'Sua sessão, seu dispositivo',43,C['ink'],600);para(130,625,'Nenhuma conta conectada neste estudo. Dados e comandos locais dependem da sessão válida do titular.',880,22);field(1170,388,678,'Estado da sessão','Não autenticada');button(1170,535,678,'Entrar com segurança',True,'lock');notice(1170,706,678,'Sessão expirada / variante','Comandos suspensos até revalidar. Cadastro não equivale a acesso.');button(1170,1000,678,'Rever privacidade',False,'shield')
 elif n==2:
  for i,(name,bg,fg) in enumerate([('Porcelana',C['paper'],C['ink']),('Grafite',C['dark'],'#F2EEEE')]):
   x=72+i*900;rect(x,326,848,440,bg,22,C['line']);rect(x+24,350,800,65,'#ECEAE5' if i==0 else '#343C42',12);rect(x+24,443,240,298,'#E2DFD9' if i==0 else '#2D3439',12);rect(x+290,443,534,298,C['dark'],12);text(x+37,392,'Filuna',20,fg,600);text(x,827,name,28,C['ink'],600)
  notice(72,930,1776,'Porcelana C é a referência aprovada','Grafite aqui é apenas variante candidata. Não substituir a âncora aprovada nem o logo original.')
 elif n in [3,4]:
  text(72,381,'Pacote verificável, decisão explícita',39,C['ink'],600);listrows(72,440,1050,[('Origem e versão','Exibir fabricante, licença e versão do pacote.','Revisar'),('Integridade','Verificar antes de ativar; rejeitar falhas.','SHA'),('Estado de impressão','Não atualizar durante trabalho ativo.','Bloquear'),('Rollback','Preservar versão anterior válida.','Disponível')],138);notice(1210,378,638,'Nenhuma instalação executada','Tela documental. Não cria build, pacote, atualização ou release.');button(1210,669,638,'Revisar alterações',True,'eye');button(1210,742,638,'Adiar',False,'clock');strip([('Falha de verificação','Preservar versão anterior íntegra.','shield'),('Instalando / proposta','Progresso não equivale a sucesso.','girar')],943,1210,638)
 else:
  rect(72,326,1090,742,C['dark'],22);icon(533,470,'wifi','#D1A1B6',100);text(617,697,'Você está sem internet',37,'#EAE5E8',500,'middle');text(617,754,'Trabalho local não significa acesso remoto.',17,'#BFC7CD',400,'middle');listrows(1220,371,628,[('Projeto local','Abrir arquivos locais disponíveis.'),('Cache de perfis','Somente pacote íntegro e compatível.'),('Catálogo / conta','Indicar dados desatualizados ou indisponíveis.'),('Impressora LAN','Estado depende da leitura local, não da internet.')],158)
 out('auxiliares',f'AUX-{n}',t,'QUAL; OEM-UPD; UPD-W / detalhamento candidato',status='PROPOSTO_CONTRATO_DETALHADO_PENDENTE')
# Planned web surfaces clearly separated and no remote controls
for n,t in enumerate(['Consentimento e dados','Observabilidade sanitizada','Remoto do titular / OFF','Revogação e exclusão'],1):
 boot('Início',t,'WEB FUTURO / conceito não funcional / servidor observador, nunca controlador')
 if n==1:
  text(72,387,'Compartilhar somente o necessário',39,C['ink'],600);para(72,460,'Consentimento específico por categoria. Desligado por padrão; nenhuma câmera, geometria privada ou credencial.',1030,24)
  for i,(a,b) in enumerate([('Eventos sanitizados','Sem tokens, senhas ou identificadores secretos.'),('Diagnóstico voluntário','Pré-visualização antes do envio.'),('Retenção e escopo','Informar prazo e finalidade antes de consentir.')]):block(72,649+i*139,a,b,1030)
  icon(1430,443,'shield',C['rose'],127);text(1510,704,'OFF',69,C['ink'],600,'middle');text(1510,762,'Nenhum consentimento ativo',17,C['muted'],400,'middle')
 elif n==2:
  text(72,368,'Administração observadora',31,C['ink'],600)
  for x,l,v in [(72,'EVENTOS SANITIZADOS','—'),(672,'PACOTES / VERSÕES','—'),(1272,'FALHAS REPORTADAS','—')]:metric(x,440,l,v)
  listrows(72,603,1776,[('Informação agregada','Somente dados consentidos e minimizados.','Sem dados reais'),('Estado de software','Não certifica hardware nem impressão.','Observação'),('Sem comandos','Não há imprimir, pausar, retomar ou controle de câmera.','Nunca controlar')],141)
 elif n==3:
  rect(72,326,1080,735,C['dark'],22);icon(516,477,'lock','#D6A3B8',106);text(612,720,'Acesso remoto desligado',38,'#F3ECEF',500,'middle');text(612,777,'FUTURO / NÃO DISPONÍVEL',16,'#BCC6CE',500,'middle');listrows(1210,369,638,[('Titular','Só as próprias impressoras.'),('Autorização granular','Por unidade, ação e duração.'),('Revogação','Cancelar acesso imediatamente.'),('Administrador','Sem controle de clientes.')],161)
 else:
  text(72,387,'Revogar acesso não apaga projeto local',37,C['ink'],600);checks(72,495,['Revogar consentimento de dados','Revogar acessos remotos futuros','Solicitar exclusão dos dados elegíveis'],1050);notice(72,737,1050,'Revisão antes de confirmar','Explicar o escopo e a retenção legal aplicável; não prometer apagar o que não pode.');button(72,986,1050,'Revisar revogação / não executado',True,'shield');icon(1390,460,'lock',C['rose'],130);para(1230,731,'Conceito futuro. Nenhum pedido enviado, dado excluído ou conta alterada.',618,23)
 out('web-futuro',f'WEB-FUT-{n}',t,'SRV §§5–7 / futuro / proposta em revisão')
(D/'ENTREGUES.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2));print('Total',len(entries),'pranchas')
