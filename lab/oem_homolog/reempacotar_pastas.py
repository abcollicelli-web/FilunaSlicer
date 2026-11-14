#!/usr/bin/env python3
"""Reempacota os zips dos módulos com a entrada de cada pasta ANTES dos arquivos dela.
O extrator do Filuna (extract_archive_confined) só cria pastas a partir de entradas de pasta;
zips do Compress-Archive/zipfile sem essas entradas falham em 'não extraiu o pacote'."""
import zipfile, hashlib, json, os, shutil, sys, glob
BASE = '/srv/filuna-homolog/modulos'; BAK = '/srv/filuna-homolog/modulos-bak-ordem'
for z in sorted(glob.glob(BASE + '/*/*.zip')):
    mid = os.path.basename(os.path.dirname(z)); nome = os.path.basename(z)
    os.makedirs(f'{BAK}/{mid}', exist_ok=True)
    if not os.path.exists(f'{BAK}/{mid}/{nome}'):
        shutil.copy2(z, f'{BAK}/{mid}/{nome}')
    src = zipfile.ZipFile(z)
    arquivos = [i for i in src.infolist() if not i.filename.endswith('/')]
    pastas = set()
    for i in src.infolist():
        partes = i.filename.rstrip('/').split('/')
        fim = len(partes) if i.filename.endswith('/') else len(partes) - 1
        for k in range(1, fim + 1):
            pastas.add('/'.join(partes[:k]) + '/')
    tmp = z + '.novo'
    with zipfile.ZipFile(tmp, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as out:
        for p in sorted(pastas):
            zi = zipfile.ZipInfo(p, date_time=(2026, 11, 14, 0, 0, 0)); zi.external_attr = 0x10
            out.writestr(zi, b'')
        for i in arquivos:
            zi = zipfile.ZipInfo(i.filename, date_time=i.date_time); zi.external_attr = i.external_attr
            zi.compress_type = zipfile.ZIP_DEFLATED
            with src.open(i) as f, out.open(zi, 'w') as g:
                shutil.copyfileobj(f, g, 1 << 20)
    src.close()
    # confere: mesmo conjunto de arquivos e toda pasta antes do arquivo
    n = zipfile.ZipFile(tmp); vistos = {''}; ruim = 0
    for i in n.infolist():
        if i.filename.endswith('/'): vistos.add(i.filename.rstrip('/')); continue
        if (i.filename.rsplit('/', 1)[0] if '/' in i.filename else '') not in vistos: ruim += 1
    assert ruim == 0 and n.testzip() is None
    assert sorted(x.filename for x in n.infolist() if not x.filename.endswith('/')) == sorted(x.filename for x in arquivos)
    n.close()
    os.replace(tmp, z)
    h = hashlib.sha256()
    with open(z, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
    man = z[:-4] + '.manifest.json'
    d = json.load(open(man)); d['pacote_sha256'] = h.hexdigest(); d['pacote_bytes'] = os.path.getsize(z)
    json.dump(d, open(man, 'w'), ensure_ascii=False, indent=2)
    print('OK', mid, nome, len(pastas), 'pastas', len(arquivos), 'arquivos', d['pacote_bytes'], h.hexdigest()[:12], flush=True)
print('FIM', flush=True)
