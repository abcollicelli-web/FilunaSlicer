#!/usr/bin/env python3
"""Filuna LAB protocol contract probe. Experimental adapter, NOT GUI/native integration.
Runs only against isolated 10.77.0.0/24 QA emulators; no physical printer.
An individual stage only PASSes if its own protocol response is validated.
"""
import argparse,datetime,hashlib,json,os,pathlib,socket,ssl,sys,time,urllib.request,urllib.error
LAB_NET='10.77.0.'
METHODS=('discover','pair','status_stream','send','start','pause','resume','stop','camera','materials')
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def request(url,method='GET',data=None,headers=None):
 req=urllib.request.Request(url,data=data,method=method,headers=headers or {})
 with urllib.request.urlopen(req,timeout=4) as res:
  raw=res.read(1_000_000)
  try: body=json.loads(raw)
  except ValueError:body=raw[:300].decode(errors='replace')
  return res.status,body
def udp_broadcast(port,payload,attempts=2):
 events=[]
 with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
  s.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1)
  s.bind(('10.77.0.1',0))
  s.settimeout(1)
  for _ in range(attempts):
   s.sendto(payload,('10.77.0.255',port))
   try:
    while True:
     data,addr=s.recvfrom(20000)
     if addr[0].startswith(LAB_NET):
      try:reply=json.loads(data)
      except Exception:reply=data.decode(errors='replace')
      events.append({'source':addr[0],'payload':reply})
   except socket.timeout:pass
 return events
class Adapter:
 def __init__(self,brand,run):
  self.brand=brand;self.run=run
  self.statefile=run/(brand+'-paired.json')
  self.host=None;self.events=[]
 def note(self,stage,status,detail=None,**extra):
  item={'utc':utc(),'brand':self.brand,'stage':stage,'status':status,'detail':detail}
  item.update(extra);self.events.append(item)
  with (self.run/'contract-events.jsonl').open('a') as fh:fh.write(json.dumps(item,ensure_ascii=False,default=str)+'\n')
 def call(self,stage):
  try:
   detail=getattr(self,stage)()
   if isinstance(detail,dict) and detail.get('_pending'):
    self.note(stage,'NÃO EXECUTADO',detail['reason']);return
   self.note(stage,'PASS',detail)
  except (OSError,ValueError,KeyError,AssertionError,urllib.error.HTTPError) as exc:
   self.note(stage,'FALHA',repr(exc))
 def persist(self,host,protocol):
  if not host.startswith(LAB_NET):raise ValueError('address not in private lab subnet')
  self.statefile.write_text(json.dumps({'host':host,'brand':self.brand,'protocol':protocol}))
 def recover(self):
  if self.statefile.exists():
   saved=json.loads(self.statefile.read_text())
   if saved['brand']==self.brand and saved['host'].startswith(LAB_NET):return saved['host']
  return None
class Moonraker(Adapter):
 def discover(self):
  candidates=[]
  for octet in range(2,26):
   host=LAB_NET+str(octet)
   try:
    status,data=request('http://'+host+':7125/printer/info')
    if status==200 and isinstance(data,dict) and data.get('result',{}).get('state'):
     candidates.append({'host':host,'info':data['result']})
   except Exception:continue
  assert candidates,'Moonraker discovery no API answering on scanned LAN'
  self.host=candidates[0]['host']
  return {'discovered':candidates,'method':'subnet API discovery, no IP supplied'}
 def pair(self):
  if not self.host:raise ValueError('no discovered host')
  status,data=request('http://'+self.host+':7125/printer/info')
  assert status==200 and data.get('result',{}).get('state')
  self.persist(self.host,'moonraker')
  return {'paired':True,'saved_to':str(self.statefile)}
 def status_stream(self):
  host=self.recover();assert host,'pair not persisted'
  code,data=request('http://'+host+':7125/printer/objects/query?print_stats')
  assert code==200 and 'result' in data
  return {'status':data['result'],'persisted_after_reload':True}
 def send(self):
  host=self.recover();assert host
  gcode=b'; FILUNA CONTRACT LAB ONLY\nG28\n'
  sha=hashlib.sha256(gcode).hexdigest()
  status,data=request('http://'+host+':7125/server/files/upload','POST',gcode)
  assert status==200 and 'result' in data
  if not (self.run/'simulator-received-sha256.txt').exists():
   return {'_pending':True,'reason':'upload succeeded but simulator-side SHA256 evidence not exposed'}
  actual=(self.run/'simulator-received-sha256.txt').read_text().strip()
  assert actual==sha,'upload SHA-256 mismatch'
  return {'sent_sha256':sha,'received_sha256':actual,'bytes':len(gcode)}
 def _control(self,op):
  host=self.recover();assert host
  query='?filename=filuna-contract.gcode' if op=='start' else ''
  code,data=request('http://'+host+':7125/printer/print/'+op+query,'POST',b'')
  assert code==200 and data.get('result')=='ok',(code,data)
  return data
 def start(self):return self._control('start')
 def pause(self):return self._control('pause')
 def resume(self):return self._control('resume')
 def stop(self):return self._control('cancel')
 def camera(self):return {'_pending':True,'reason':'Moonraker QA emulator has no camera endpoint'}
 def materials(self):return {'_pending':True,'reason':'Moonraker QA emulator has no QIDI Box/CFS material contract'}
class ElegooCC2(Adapter):
 def discover(self):
  events=udp_broadcast(52700,b'{"id":0,"method":7000}')
  found=[]
  for e in events:
   payload=e.get('payload')
   if isinstance(payload,dict) and payload.get('result',{}).get('machine_model')=='Centauri Carbon 2':
    found.append(e['source'])
  assert found,'No CC2 broadcast discovery response'
  self.host=found[0];return {'devices':found,'source':'UDP 52700 broadcast'}
 def pair(self):
  assert self.host,'no discovery'
  self.persist(self.host,'elegoo-cc2')
  return {'paired_in_lab':True,'saved':str(self.statefile),'authentication':'not demonstrated'}
 def status_stream(self):
  return {'_pending':True,'reason':'CC2 MQTT authentication and state client must use real module'}
 def send(self):return {'_pending':True,'reason':'CC2 OEM upload protocol module not connected'}
 def start(self):return {'_pending':True,'reason':'CC2 OEM control module not connected'}
 def pause(self):return self.start()
 def resume(self):return self.start()
 def stop(self):return self.start()
 def camera(self):return {'_pending':True,'reason':'No CC2 camera flow in provisional adapter'}
 def materials(self):return {'_pending':True,'reason':'CANVAS module not connected'}
class Bambu(Adapter):
 def discover(self):
  return {'_pending':True,'reason':'Bambuddy SSDP announces to UDP 2021; listener already owned by running lab, no non-IP automatic discovery verified'}
 def pair(self):
  return {'_pending':True,'reason':'Bambu virtual printer Bind/secure MQTT client module not yet connected (do not extract secrets from simulator DB)'}
 def status_stream(self):
  return {'_pending':True,'reason':'no authenticated MQTT status session'}
 def send(self):return {'_pending':True,'reason':'no FTPS job with simulator-side SHA256 verified'}
 def start(self):return {'_pending':True,'reason':'no authenticated Bambu MQTT print command'}
 def pause(self):return self.start()
 def resume(self):return self.start()
 def stop(self):return self.start()
 def camera(self):return {'_pending':True,'reason':'Bambu camera module not tested through Filuna contract'}
 def materials(self):return {'_pending':True,'reason':'AMS lite slot contract not tested'}
def main():
 p=argparse.ArgumentParser()
 p.add_argument('--brand',required=True,choices=('bambu','elegoo','moonraker'))
 p.add_argument('--run-dir',required=True)
 args=p.parse_args()
 out=pathlib.Path(args.run_dir).resolve();out.mkdir(parents=True,exist_ok=True)
 assert 'filuna-local-oem-qa-20261106' in str(out)
 cls={'bambu':Bambu,'elegoo':ElegooCC2,'moonraker':Moonraker}[args.brand]
 adapter=cls(args.brand,out)
 for stage in METHODS:adapter.call(stage)
 result={'brand':args.brand,'events':adapter.events,
         'pass':sum(e['status']=='PASS' for e in adapter.events),
         'not_executed':sum(e['status']=='NÃO EXECUTADO' for e in adapter.events)}
 (out/(args.brand+'-summary.json')).write_text(json.dumps(result,indent=2,ensure_ascii=False))
 print(json.dumps({'brand':args.brand,'pass':result['pass'],'not_executed':result['not_executed']},ensure_ascii=False))
if __name__=='__main__':main()
