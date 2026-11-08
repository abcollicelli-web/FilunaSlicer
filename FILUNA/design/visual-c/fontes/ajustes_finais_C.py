from pathlib import Path
exec((Path(__file__).resolve().parent/'completar_C.py').read_text().split('# DEVICE')[0])
entries=json.loads((D/'ENTREGUES.json').read_text())
boot('Dispositivo','Bobinas e slots','Estoque físico / vínculo não transfere material / dados ilustrativos')
text(72,365,'Seu material, com origem e saldo',33,C['ink'],600)
for i,(name,col) in enumerate([('PLA / Rosa mineral','#B94D72'),('PLA / Laranja','#E88759'),('PETG / Natural','#BEC6C0')]):
 y=422+i*200;circle(118,y+48,45,col);circle(118,y+48,16,C['paper']);text(199,y+26,name,24,C['ink'],600);text(199,y+66,'Peso e preço não registrados / sem saldo estimado',16,C['muted']);text(199,y+108,'Vínculo: Bancada 01 / selecionar slot',15,C['rose']);line(72,y+160,1134,y+160)
text(1210,380,'Associações separadas',29,C['ink'],600)
for i,(t,b) in enumerate([('Perfil','Preset define parâmetros de material. Não é estoque.'),('Bobina','Identifica recurso físico e seus valores informados.'),('Slot','Recurso da unidade. Associação confirmada pelo titular.')]):block(1210,451+i*173,t,b,638)
button(1210,1000,638,'Revisar vínculo sem comando físico',True,'settings');out('dispositivo','DEV-17','Bobinas e slots','UX-D §3; DEV §8; PROD')
boot('Início','Instalação e rollback','Cenários do instalador / conceito não funcional / nenhuma instalação executada')
for i,(t,st) in enumerate([('Verificar pacote','Origem / versão / integridade'),('Preparar instalação','Aplicativo sem trabalho ativo'),('Ativar versão','Somente após verificações'),('Conservar recuperação','Versão anterior íntegra')]):
 y=367+i*153;circle(100,y-7,17,C['rose'] if i==1 else '#E4E5E0');text(100,y-1,str(i+1),12,'white' if i==1 else C['muted'],500,'middle');text(142,y,t,26,C['ink'],600);text(142,y+45,st,16,C['muted']);line(142,y+95,1070,y+95)
text(1170,379,'42 %',85,C['ink'],500);text(1170,431,'Preparando / cenário ilustrativo',18,C['muted']);rect(1170,472,678,7,'#DEE1DD',3);rect(1170,472,285,7,C['rose'],3)
notice(1170,559,678,'Variante / falha de integridade','Instalação bloqueada. Não ativar pacote incompleto.');notice(1170,711,678,'Variante / rollback disponível','Revisar retorno à última versão íntegra sem apagar projetos.');button(1170,947,678,'Revisar recuperação da versão',True,'shield');button(1170,1015,678,'Manter versão atual',False,'check');out('auxiliares','AUX-4','Instalação e rollback','QUAL; OEM-UPD; UPD-W / detalhamento candidato',status='PROPOSTO_CONTRATO_DETALHADO_PENDENTE')
# File-level update of Fast tower illustration only, keeping existing navigation/texts.
p=D/'calibracao/CAL-INPUT-2.svg';svg=p.read_text();marker='<rect x="356" y="310"';a=svg.index(marker);b=svg.index('<text x="1050"',a)
s=[];rect(356,310,650,594,C['dark'],18);text(392,370,'Fast tower',33,'#EFDBE6',500)
for i in range(7):
 yy=777-i*54;ww=340-i*24;xx=680-ww/2;rect(xx,yy,ww,30,'#C78AA5',5);line(xx+21,yy-24,xx+21,yy+29,'#E8BCD0',6);line(xx+ww-21,yy-24,xx+ww-21,yy+29,'#E8BCD0',6)
text(390,855,'Arte ilustrativa / não é modelo de teste gerado',14,'#D3C1CC');svg=svg[:a]+'\n'.join(s)+'\n'+svg[b:];p.write_text(svg)
(D/'ENTREGUES.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2))
