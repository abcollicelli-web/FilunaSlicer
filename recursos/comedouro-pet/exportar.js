// Comedouro Filuna — exportação: projeto 3MF do Bambu Studio / Orca / Filuna Slicer com objetos de várias partes (uma parte
// por cor, cada uma na sua extrusora), STL binário e ZIP comprimido (deflate).

const TAB = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c >>> 0;
  }
  return t;
})();

function crc32(dados) {
  let c = 0xFFFFFFFF;
  for (let i = 0; i < dados.length; i++) c = TAB[(c ^ dados[i]) & 0xFF] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}

const txt = (s) => new TextEncoder().encode(s);

async function comprimir(dados) {
  if (typeof CompressionStream === 'undefined' || dados.length < 512) return null;
  const fluxo = new Blob([dados]).stream().pipeThrough(new CompressionStream('deflate-raw'));
  const saida = new Uint8Array(await new Response(fluxo).arrayBuffer());
  return saida.length < dados.length ? saida : null;
}

// ZIP: `arquivos` = [[nome, Uint8Array], ...]. Devolve um Blob.
export async function zip(arquivos) {
  const partes = [], centro = [];
  let off = 0;
  for (const [nome, dados] of arquivos) {
    const n = txt(nome), crc = crc32(dados), z = await comprimir(dados), corpo = z || dados, metodo = z ? 8 : 0;
    const loc = new DataView(new ArrayBuffer(30));
    loc.setUint32(0, 0x04034b50, true); loc.setUint16(4, 20, true); loc.setUint16(8, metodo, true);
    loc.setUint32(14, crc, true); loc.setUint32(18, corpo.length, true); loc.setUint32(22, dados.length, true);
    loc.setUint16(26, n.length, true);
    partes.push(new Uint8Array(loc.buffer), n, corpo);
    const c = new DataView(new ArrayBuffer(46));
    c.setUint32(0, 0x02014b50, true); c.setUint16(4, 20, true); c.setUint16(6, 20, true); c.setUint16(10, metodo, true);
    c.setUint32(16, crc, true); c.setUint32(20, corpo.length, true); c.setUint32(24, dados.length, true);
    c.setUint16(28, n.length, true); c.setUint32(42, off, true);
    centro.push(new Uint8Array(c.buffer), n);
    off += 30 + n.length + corpo.length;
  }
  let tamCentro = 0;
  for (const p of centro) tamCentro += p.length;
  const fim = new DataView(new ArrayBuffer(22));
  fim.setUint32(0, 0x06054b50, true);
  fim.setUint16(8, arquivos.length, true); fim.setUint16(10, arquivos.length, true);
  fim.setUint32(12, tamCentro, true); fim.setUint32(16, off, true);
  return new Blob([...partes, ...centro, new Uint8Array(fim.buffer)]);
}

const esc = (s) => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/"/g, '&quot;');
const num = (v) => { const r = Math.round(v * 1000) / 1000; return Object.is(r, -0) ? '0' : String(r); };

// arquivo de objetos 3MF com várias malhas (uma por parte), montado em pedaços
function objetosModelo(partes, uuid) {
  const NS = 'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02" xmlns:p="http://schemas.microsoft.com/3dmanufacturing/production/2015/06" requiredextensions="p"';
  const pedacos = [`<?xml version="1.0" encoding="UTF-8"?>\n<model unit="millimeter" xml:lang="pt-BR" ${NS}>\n <resources>\n`];
  for (const pt of partes) {
    pedacos.push(`  <object id="${pt.id}" p:UUID="${uuid(pt.id, '0001')}" type="model">\n   <mesh>\n    <vertices>\n`);
    const linha = [];
    for (let j = 0; j < pt.pos.length; j += 3) linha.push(`<vertex x="${num(pt.pos[j])}" y="${num(pt.pos[j + 1])}" z="${num(pt.pos[j + 2])}"/>`);
    pedacos.push(linha.join('\n'), '\n    </vertices>\n    <triangles>\n');
    linha.length = 0;
    for (let j = 0; j < pt.idx.length; j += 3) linha.push(`<triangle v1="${pt.idx[j]}" v2="${pt.idx[j + 1]}" v3="${pt.idx[j + 2]}"/>`);
    pedacos.push(linha.join('\n'), '\n    </triangles>\n   </mesh>\n  </object>\n');
  }
  pedacos.push(' </resources>\n</model>\n');
  return txt(pedacos.join(''));
}

// Projeto 3MF: `pecas` = [{nome, mesa, mesaNome, ajustes, partes: [{nome, extrusora, pos, idx}]}], já na posição
// final no espaço das mesas. Cada peça vira um objeto; cada parte, uma malha com a sua extrusora.
export async function projetoBambu(pecas, mesas, titulo) {
  const uuid = (k, tipo) => `${String(k).padStart(8, '0')}-${tipo}-4f11-9a3d-f11a03d00000`;
  const NS = 'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02" xmlns:p="http://schemas.microsoft.com/3dmanufacturing/production/2015/06" requiredextensions="p"';
  let id = 1;
  const objs = pecas.map((pc) => ({ ...pc, partes: pc.partes.map(pt => ({ ...pt, id: id++ })) }));
  objs.forEach(o => { o.id = id++; });
  const arquivos = [], rels = [];
  objs.forEach((o, i) => {
    arquivos.push([`3D/Objects/object_${i + 1}.model`, objetosModelo(o.partes, uuid)]);
    rels.push(`<Relationship Target="/3D/Objects/object_${i + 1}.model" Id="rel-${i + 1}" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/>`);
  });
  const principal = ['<?xml version="1.0" encoding="UTF-8"?>', `<model unit="millimeter" xml:lang="pt-BR" xmlns:BambuStudio="http://schemas.bambulab.com/package/2021" ${NS}>`,
    ' <metadata name="Application">Filuna 3D</metadata>', ' <metadata name="BambuStudio:3mfVersion">1</metadata>', ` <metadata name="Title">${esc(titulo)}</metadata>`, ' <resources>'];
  objs.forEach((o, i) => {
    principal.push(`  <object id="${o.id}" p:UUID="${uuid(o.id, '0002')}" type="model">`, '   <components>');
    for (const pt of o.partes) principal.push(`    <component p:path="/3D/Objects/object_${i + 1}.model" objectid="${pt.id}" p:UUID="${uuid(pt.id, '0003')}" transform="1 0 0 0 1 0 0 0 1 0 0 0"/>`);
    principal.push('   </components>', '  </object>');
  });
  principal.push(' </resources>', ' <build p:UUID="00000000-0004-4f11-9a3d-f11a03d00000">');
  objs.forEach(o => principal.push(`  <item objectid="${o.id}" p:UUID="${uuid(o.id, '0005')}" transform="1 0 0 0 1 0 0 0 1 0 0 0" printable="1"/>`));
  principal.push(' </build>', '</model>', '');
  const config = ['<?xml version="1.0" encoding="UTF-8"?>', '<config>'];
  objs.forEach(o => {
    const faces = o.partes.reduce((s, pt) => s + pt.idx.length / 3, 0);
    config.push(`  <object id="${o.id}">`, `    <metadata key="name" value="${esc(o.nome)}"/>`, `    <metadata key="extruder" value="${o.partes[0].extrusora}"/>`);
    for (const [k, v] of Object.entries(o.ajustes || {})) config.push(`    <metadata key="${esc(k)}" value="${esc(v)}"/>`);
    config.push(`    <metadata face_count="${faces}"/>`);
    for (const pt of o.partes) {
      config.push(`    <part id="${pt.id}" subtype="${pt.negativa ? 'negative_part' : 'normal_part'}">`, `      <metadata key="name" value="${esc(pt.nome)}"/>`, '      <metadata key="matrix" value="1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"/>',
        `      <metadata key="extruder" value="${pt.extrusora}"/>`,
        `      <mesh_stat face_count="${pt.idx.length / 3}" edges_fixed="0" degenerate_facets="0" facets_removed="0" facets_reversed="0" backwards_edges="0"/>`, '    </part>');
    }
    config.push('  </object>');
  });
  for (let m = 0; m < mesas; m++) {
    const nome = (objs.find(o => o.mesa === m) || {}).mesaNome || '';
    config.push('  <plate>', `    <metadata key="plater_id" value="${m + 1}"/>`, `    <metadata key="plater_name" value="${esc(nome)}"/>`, '    <metadata key="locked" value="false"/>');
    objs.forEach((o, i) => { if (o.mesa === m) config.push('    <model_instance>', `      <metadata key="object_id" value="${o.id}"/>`, '      <metadata key="instance_id" value="0"/>', `      <metadata key="identify_id" value="${i + 1}"/>`, '    </model_instance>'); });
    config.push('  </plate>');
  }
  config.push('</config>', '');
  const filamentos = new Map();
  for (const o of objs) for (const pt of o.partes) if (!pt.negativa && /^#[0-9a-f]{6}$/i.test(pt.cor || '') && !filamentos.has(pt.extrusora)) filamentos.set(pt.extrusora, pt.cor.toUpperCase());
  const extras = filamentos.size ? [['Metadata/filuna_filamentos.json', txt(JSON.stringify({ versao: 1, filamentos: [...filamentos].sort((a, b) => a[0] - b[0]).map(([extrusora, cor]) => ({ extrusora, cor })) }))]] : [];
  const tipos = '<?xml version="1.0" encoding="UTF-8"?>\n<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/><Default Extension="config" ContentType="text/xml"/><Default Extension="json" ContentType="application/json"/></Types>';
  const relRaiz = '<?xml version="1.0" encoding="UTF-8"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Target="/3D/3dmodel.model" Id="rel-0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>';
  const relModelo = `<?xml version="1.0" encoding="UTF-8"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">${rels.join('')}</Relationships>`;
  return zip([['[Content_Types].xml', txt(tipos)], ['_rels/.rels', txt(relRaiz)], ['3D/3dmodel.model', txt(principal.join('\n'))], ['3D/_rels/3dmodel.model.rels', txt(relModelo)], ...arquivos, ['Metadata/model_settings.config', txt(config.join('\n'))], ...extras]);
}

// STL binário de uma peça.
export function stl(peca, cabecalho) {
  const nt = peca.idx.length / 3;
  const buf = new ArrayBuffer(84 + nt * 50), dv = new DataView(buf);
  const cab = (cabecalho || 'Filuna 3D').slice(0, 79);
  for (let i = 0; i < 80; i++) dv.setUint8(i, i < cab.length ? cab.charCodeAt(i) & 0x7f : 32);
  dv.setUint32(80, nt, true);
  let o = 84;
  const P = peca.pos;
  for (let i = 0; i < peca.idx.length; i += 3) {
    const a = peca.idx[i] * 3, b = peca.idx[i + 1] * 3, c = peca.idx[i + 2] * 3;
    const ux = P[b] - P[a], uy = P[b + 1] - P[a + 1], uz = P[b + 2] - P[a + 2];
    const vx = P[c] - P[a], vy = P[c + 1] - P[a + 1], vz = P[c + 2] - P[a + 2];
    const nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx, l = Math.hypot(nx, ny, nz) || 1;
    for (const x of [nx / l, ny / l, nz / l, P[a], P[a + 1], P[a + 2], P[b], P[b + 1], P[b + 2], P[c], P[c + 1], P[c + 2]]) {
      dv.setFloat32(o, x, true); o += 4;
    }
    dv.setUint16(o, 0, true); o += 2;
  }
  return new Uint8Array(buf);
}
