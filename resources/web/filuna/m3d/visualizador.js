/* Gerado de site-v2/app/static (js/visualizador.js) para rodar no Filuna Slicer (file://, sem módulos JS). Não editar: gerar de novo. */
(function(){
// Visualizador Filuna de STL e 3MF: roda no navegador, o arquivo não sai do computador do cliente.
// Lê placas de projetos Bambu Studio / Orca / Filuna e mostra as peças sobre uma mesa de PEI texturizada.
const THREE = window.THREE;
const OrbitControls = window.OrbitControls;

// ---------- leitura de ZIP (3MF) sem bibliotecas: diretório central + DecompressionStream ----------
async function lerZip(buf) {
  const dv = new DataView(buf);
  let eocd = -1;
  for (let i = buf.byteLength - 22; i >= Math.max(0, buf.byteLength - 70000); i--) {
    if (dv.getUint32(i, true) === 0x06054b50) { eocd = i; break; }
  }
  if (eocd < 0) throw new Error('Esse 3MF não é um arquivo válido.');
  const total = dv.getUint16(eocd + 10, true);
  let p = dv.getUint32(eocd + 16, true);
  const entradas = {};
  const dec = new TextDecoder();
  for (let n = 0; n < total; n++) {
    if (dv.getUint32(p, true) !== 0x02014b50) break;
    const metodo = dv.getUint16(p + 10, true);
    const tamC = dv.getUint32(p + 20, true);
    const lenNome = dv.getUint16(p + 28, true), lenExtra = dv.getUint16(p + 30, true), lenCom = dv.getUint16(p + 32, true);
    const local = dv.getUint32(p + 42, true);
    const nome = dec.decode(new Uint8Array(buf, p + 46, lenNome));
    entradas[nome.replace(/^\//, '')] = { metodo, tamC, local };
    p += 46 + lenNome + lenExtra + lenCom;
  }
  return {
    nomes: Object.keys(entradas),
    async ler(nome) {
      const e = entradas[nome.replace(/^\//, '')];
      if (!e) return null;
      const ln = dv.getUint16(e.local + 26, true), le = dv.getUint16(e.local + 28, true);
      const dados = new Uint8Array(buf, e.local + 30 + ln + le, e.tamC);
      if (e.metodo === 0) return dados;
      if (e.metodo !== 8) throw new Error('Compressão do 3MF não suportada.');
      const s = new Blob([dados]).stream().pipeThrough(new DecompressionStream('deflate-raw'));
      return new Uint8Array(await new Response(s).arrayBuffer());
    },
    async texto(nome) { const d = await this.ler(nome); return d ? dec.decode(d) : null; },
  };
}

// ---------- STL ----------
function lerSTL(buf) {
  const dv = new DataView(buf);
  const n = buf.byteLength >= 84 ? dv.getUint32(80, true) : 0;
  if (n > 0 && 84 + n * 50 === buf.byteLength) {
    const pos = new Float32Array(n * 9);
    for (let i = 0; i < n; i++) {
      const o = 84 + i * 50 + 12;
      for (let j = 0; j < 9; j++) pos[i * 9 + j] = dv.getFloat32(o + j * 4, true);
    }
    return pos;
  }
  const txt = new TextDecoder().decode(buf);
  const nums = [];
  const re = /vertex\s+([-+\d.eE]+)\s+([-+\d.eE]+)\s+([-+\d.eE]+)/g;
  let m;
  while ((m = re.exec(txt))) nums.push(+m[1], +m[2], +m[3]);
  if (!nums.length) throw new Error('Esse STL não tem triângulos.');
  return new Float32Array(nums);
}

// ---------- 3MF ----------
const NS_CORE = 'http://schemas.microsoft.com/3dmanufacturing/core/2015/02';
function matriz(t) {
  const m = new THREE.Matrix4();
  if (!t) return m;
  const v = t.trim().split(/\s+/).map(Number);
  if (v.length !== 12) return m;
  m.set(v[0], v[3], v[6], v[9], v[1], v[4], v[7], v[10], v[2], v[5], v[8], v[11], 0, 0, 0, 1);
  return m;
}
function porTag(el, tag) { return Array.from(el.getElementsByTagNameNS(NS_CORE, tag)); }

// Pintura de cor por triângulo (Bambu Studio, OrcaSlicer e PrusaSlicer: atributo paint_color / mmu_segmentation).
// É uma árvore de subdivisões gravada em hexadecimal, lida de trás para frente, um "nibble" por caractere
// (mesmo formato do TriangleSelector do libslic3r). Cada folha guarda o número do filamento; 0 = cor da peça.
function pintarTriangulo(codigo, a, b, c, emitir) {
  const nib = [];
  for (let i = codigo.length - 1; i >= 0; i--) { const n = parseInt(codigo[i], 16); if (Number.isNaN(n)) return false; nib.push(n); }
  let k = 0;
  const prox = () => { if (k >= nib.length) throw 0; return nib[k++]; };
  const meio = (p, q) => [(p[0] + q[0]) / 2, (p[1] + q[1]) / 2, (p[2] + q[2]) / 2];
  function no(v0, v1, v2, prof) {
    if (prof > 24) throw 0;
    const cod = prox(), lados = cod & 3;
    if (!lados) {
      const estado = (cod & 12) === 12 ? (n => (n === 15 ? prox() + 18 : n + 3))(prox()) : cod >> 2;
      emitir(v0, v1, v2, estado);
      return;
    }
    const esp = lados === 3 ? 0 : cod >> 2;
    const t = [v0, v1, v2], vs = [t[esp % 3], t[(esp + 1) % 3], t[(esp + 2) % 3]];
    let filhos;
    if (lados === 1) {
      vs.splice(2, 0, meio(vs[2], vs[1]));
      filhos = [[0, 1, 2], [2, 3, 0]];
    } else if (lados === 2) {
      vs.splice(1, 0, meio(vs[1], vs[0]));
      vs.splice(4, 0, meio(vs[0], vs[3]));
      filhos = [[0, 1, 4], [1, 2, 4], [2, 3, 4]];
    } else {
      vs.splice(1, 0, meio(vs[1], vs[0]));
      vs.splice(3, 0, meio(vs[3], vs[2]));
      vs.splice(5, 0, meio(vs[0], vs[4]));
      filhos = [[0, 1, 5], [1, 2, 3], [3, 4, 5], [1, 3, 5]];
    }
    // os filhos vêm gravados do último para o primeiro
    for (let f = filhos.length - 1; f >= 0; f--) { const [i, j, l] = filhos[f]; no(vs[i], vs[j], vs[l], prof + 1); }
  }
  try { no(a, b, c, 0); return true; } catch (e) { return false; }
}

async function ler3MF(buf) {
  const zip = await lerZip(buf);
  const parser = new DOMParser();
  const modelos = {};
  async function modelo(caminho) {
    caminho = caminho.replace(/^\//, '');
    if (modelos[caminho]) return modelos[caminho];
    const txt = await zip.texto(caminho);
    if (!txt) return null;
    const doc = parser.parseFromString(txt, 'application/xml');
    const objetos = {};
    for (const o of porTag(doc, 'object')) objetos[o.getAttribute('id')] = o;
    return (modelos[caminho] = { doc, objetos });
  }
  const principal = '3D/3dmodel.model';
  const raiz = await modelo(principal);
  if (!raiz) throw new Error('Esse 3MF não tem modelo 3D.');

  // malha de um objeto (com componentes, recursivo), já transformada
  async function malha(caminho, id, mat, saida, ext, idObjeto, prof = 0) {
    if (prof > 8) return;
    const md = await modelo(caminho);
    const obj = md && md.objetos[id];
    if (!obj) return;
    const mesh = porTag(obj, 'mesh')[0];
    if (mesh) {
      const vs = porTag(mesh, 'vertex');
      const vx = new Float32Array(vs.length * 3);
      const tmp = new THREE.Vector3();
      vs.forEach((v, i) => {
        tmp.set(+v.getAttribute('x'), +v.getAttribute('y'), +v.getAttribute('z')).applyMatrix4(mat);
        vx[i * 3] = tmp.x; vx[i * 3 + 1] = tmp.y; vx[i * 3 + 2] = tmp.z;
      });
      const ts = porTag(mesh, 'triangle');
      const pintado = ts.some(t => t.hasAttribute('paint_color') || t.hasAttribute('slic3rpe:mmu_segmentation'));
      if (!pintado) {
        const pos = new Float32Array(ts.length * 9);
        ts.forEach((t, i) => {
          ['v1', 'v2', 'v3'].forEach((kk, j) => {
            const a = +t.getAttribute(kk) * 3;
            pos[i * 9 + j * 3] = vx[a]; pos[i * 9 + j * 3 + 1] = vx[a + 1]; pos[i * 9 + j * 3 + 2] = vx[a + 2];
          });
        });
        pos.extrusor = ext;
        saida.push(pos);
      } else {
        // peça única com várias cores: cada triângulo (ou pedaço dele) recebe a cor do seu filamento
        const pos = [], cor = [];
        const emitir = (p, q, r, estado) => {
          pos.push(p[0], p[1], p[2], q[0], q[1], q[2], r[0], r[1], r[2]);
          const c = corDoFilamento(estado || ext);
          cor.push(c.r, c.g, c.b, c.r, c.g, c.b, c.r, c.g, c.b);
        };
        const ponto = i => [vx[i * 3], vx[i * 3 + 1], vx[i * 3 + 2]];
        for (const t of ts) {
          const a = ponto(+t.getAttribute('v1')), b = ponto(+t.getAttribute('v2')), c = ponto(+t.getAttribute('v3'));
          const codigo = t.getAttribute('paint_color') || t.getAttribute('slic3rpe:mmu_segmentation');
          if (!codigo || !pintarTriangulo(codigo, a, b, c, emitir)) emitir(a, b, c, 0);
        }
        const arr = new Float32Array(pos);
        arr.cores = new Float32Array(cor);
        saida.push(arr);
      }
    }
    for (const c of porTag(obj, 'component')) {
      const alvo = c.getAttributeNS('http://schemas.microsoft.com/3dmanufacturing/production/2015/06', 'path') || c.getAttribute('p:path') || caminho;
      const idParte = c.getAttribute('objectid');
      const extParte = prof === 0 ? (extrusorParte[`${idObjeto}:${idParte}`] || ext) : ext;
      await malha(alvo, idParte, mat.clone().multiply(matriz(c.getAttribute('transform'))), saida, extParte, idObjeto, prof + 1);
    }
  }

  // configurações do projeto (mesa, cores e placas)
  let cfg = {};
  try { cfg = JSON.parse((await zip.texto('Metadata/project_settings.config')) || '{}'); } catch (e) { cfg = {}; }
  const cores = cfg.filament_colour || [];
  const cacheCor = {};
  const corDoFilamento = n => cacheCor[n] || (cacheCor[n] = new THREE.Color((cores[n - 1] || '#E0186B').slice(0, 7)));
  let mesa = null;
  if (Array.isArray(cfg.printable_area) && cfg.printable_area.length >= 3) {
    const pts = cfg.printable_area.map(s => s.split('x').map(Number));
    const xs = pts.map(p => p[0]), ys = pts.map(p => p[1]);
    mesa = { x0: Math.min(...xs), y0: Math.min(...ys), w: Math.max(...xs) - Math.min(...xs), d: Math.max(...ys) - Math.min(...ys) };
  }
  const extrusor = {}, placaDe = {}, nomesPlaca = [], nomeObj = {}, extrusorParte = {};
  const ms = await zip.texto('Metadata/model_settings.config');
  if (ms) {
    const doc = parser.parseFromString(ms, 'application/xml');
    for (const o of doc.getElementsByTagName('object')) {
      const e = Array.from(o.children).find(m => m.tagName === 'metadata' && m.getAttribute('key') === 'extruder');
      if (e) extrusor[o.getAttribute('id')] = +e.getAttribute('value');
      const n = Array.from(o.children).find(m => m.tagName === 'metadata' && m.getAttribute('key') === 'name');
      if (n) nomeObj[o.getAttribute('id')] = n.getAttribute('value');
      // peças com várias partes: cada parte pode usar um filamento diferente
      for (const pt of Array.from(o.children).filter(x => x.tagName === 'part')) {
        const pe = Array.from(pt.children).find(m => m.tagName === 'metadata' && m.getAttribute('key') === 'extruder');
        if (pe) extrusorParte[`${o.getAttribute('id')}:${pt.getAttribute('id')}`] = +pe.getAttribute('value');
      }
    }
    Array.from(doc.getElementsByTagName('plate')).forEach((pl, i) => {
      const meta = k => { const m = Array.from(pl.children).find(x => x.tagName === 'metadata' && x.getAttribute('key') === k); return m && m.getAttribute('value'); };
      nomesPlaca[i] = meta('plater_name') || '';
      for (const mi of pl.getElementsByTagName('model_instance')) {
        const m = Array.from(mi.getElementsByTagName('metadata')).find(x => x.getAttribute('key') === 'object_id');
        if (m) placaDe[m.getAttribute('value')] = i;
      }
    });
  }
  const nPlacas = Math.max(1, nomesPlaca.length);
  const colunas = Math.max(1, Math.ceil(Math.sqrt(nPlacas) - 1e-9));
  const itens = [];
  const build = porTag(raiz.doc, 'build')[0];
  for (const it of build ? porTag(build, 'item') : []) {
    const id = it.getAttribute('objectid');
    const partes = [];
    const ext = extrusor[id] || 1;
    await malha(principal, id, matriz(it.getAttribute('transform')), partes, ext, id);
    if (!partes.length) continue;
    const nome = nomeObj[id] || (raiz.objetos[id] && raiz.objetos[id].getAttribute('name')) || '';
    itens.push({ partes, placa: placaDe[id] ?? 0, cor: cores[ext - 1] || '#E0186B', nome });
  }
  if (!itens.length) throw new Error('Não encontrei peças nesse 3MF.');
  const corDe = n => cores[n - 1] || '#E0186B';
  return { itens, nPlacas, colunas, mesa, nomesPlaca, corDe, impressora: cfg.printer_settings_id || '' };
}

// ---------- mesa de PEI texturizada ----------
function texturaPEI() {
  const c = document.createElement('canvas');
  c.width = c.height = 512;
  const g = c.getContext('2d');
  g.fillStyle = '#2A2622'; g.fillRect(0, 0, 512, 512);
  const img = g.getImageData(0, 0, 512, 512);
  for (let i = 0; i < img.data.length; i += 4) {
    const n = (Math.random() - 0.5) * 34;
    img.data[i] = Math.max(0, Math.min(255, 58 + n)); img.data[i + 1] = Math.max(0, Math.min(255, 51 + n * 0.9)); img.data[i + 2] = Math.max(0, Math.min(255, 43 + n * 0.8));
  }
  g.putImageData(img, 0, 0);
  const t = new THREE.CanvasTexture(c);
  t.wrapS = t.wrapT = THREE.RepeatWrapping;
  t.colorSpace = THREE.SRGBColorSpace;
  return t;
}

function criarMesa(w, d) {
  const grupo = new THREE.Group();
  const tex = texturaPEI();
  tex.repeat.set(w / 60, d / 60);
  const forma = new THREE.Shape();
  const r = Math.min(10, w * 0.04);
  forma.moveTo(r, 0); forma.lineTo(w - r, 0); forma.quadraticCurveTo(w, 0, w, r); forma.lineTo(w, d - r);
  forma.quadraticCurveTo(w, d, w - r, d); forma.lineTo(r, d); forma.quadraticCurveTo(0, d, 0, d - r); forma.lineTo(0, r); forma.quadraticCurveTo(0, 0, r, 0);
  const geo = new THREE.ExtrudeGeometry(forma, { depth: 2.4, bevelEnabled: false });
  const uv = geo.attributes.uv;
  for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) / w, uv.getY(i) / d);
  const placa = new THREE.Mesh(geo, [
    new THREE.MeshStandardMaterial({ map: tex, roughness: 0.92, metalness: 0.15 }),
    new THREE.MeshStandardMaterial({ color: 0x1b1916, roughness: 0.6, metalness: 0.4 }),
  ]);
  placa.position.z = -2.4;
  grupo.add(placa);
  const linhas = new THREE.Group();
  const matL = new THREE.LineBasicMaterial({ color: 0x8c7d6c, transparent: true, opacity: 0.28 });
  const passo = w > 300 ? 50 : 25;
  for (let x = passo; x < w; x += passo) linhas.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(x, 0, 0.05), new THREE.Vector3(x, d, 0.05)]), matL));
  for (let y = passo; y < d; y += passo) linhas.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(0, y, 0.05), new THREE.Vector3(w, y, 0.05)]), matL));
  grupo.add(linhas);
  return grupo;
}

// ---------- visualizador ----------
function criarVisualizador(container) {
  const renderer = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true, alpha: true });
  renderer.setPixelRatio(Math.min(2, window.devicePixelRatio || 1));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  container.appendChild(renderer.domElement);
  const cena = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(35, 1, 1, 10000);
  camera.up.set(0, 0, 1);
  const controles = new OrbitControls(camera, renderer.domElement);
  controles.enableDamping = true;
  cena.add(new THREE.HemisphereLight(0xfff7ee, 0x2a2622, 1.25));
  const sol = new THREE.DirectionalLight(0xffffff, 1.6); sol.position.set(-200, -300, 500); cena.add(sol);
  const contra = new THREE.DirectionalLight(0xffe2cc, 0.55); contra.position.set(300, 400, 200); cena.add(contra);

  let dados = null, placaAtual = 0, grupoPlaca = null, mesaTam = { w: 256, d: 256 };

  function ajustarTamanho() {
    const w = container.clientWidth || 600, h = container.clientHeight || 420;
    renderer.setSize(w, h, false);
    camera.aspect = w / h; camera.updateProjectionMatrix();
  }
  new ResizeObserver(ajustarTamanho).observe(container);
  ajustarTamanho();

  (function loop() { controles.update(); renderer.render(cena, camera); requestAnimationFrame(loop); })();

  function enquadrar(w, d, altura) {
    const alvo = new THREE.Vector3(w / 2, d / 2, Math.min(altura, 120) / 3);
    // telas em pé (celular) precisam de mais distância para a mesa caber na largura
    const ajuste = camera.aspect < 1 ? Math.pow(1 / camera.aspect, 0.85) : 1;
    const dist = (Math.max(w, d) * 1.55 + altura * 0.6) * ajuste;
    camera.position.set(w / 2 - dist * 0.42, -dist * 0.78, dist * 0.62);
    controles.target.copy(alvo);
    controles.update();
  }

  function montarPlaca(i) {
    if (grupoPlaca) { cena.remove(grupoPlaca); grupoPlaca.traverse(o => { o.geometry && o.geometry.dispose(); }); }
    grupoPlaca = new THREE.Group();
    const { w, d } = mesaTam;
    grupoPlaca.add(criarMesa(w, d));
    const passoX = w * 1.2, passoY = d * 1.2;
    const col = i % dados.colunas, lin = Math.floor(i / dados.colunas);
    const ox = (dados.mesa ? dados.mesa.x0 : 0) + (dados.origemPlacas ? col * passoX : 0);
    const oy = (dados.mesa ? dados.mesa.y0 : 0) - (dados.origemPlacas ? lin * passoY : 0);
    let alturaMax = 10;
    for (const it of dados.itens.filter(x => x.placa === i)) {
      for (const pos of it.partes) {
        const geo = new THREE.BufferGeometry();
        geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
        geo.computeVertexNormals();
        geo.computeBoundingBox();
        alturaMax = Math.max(alturaMax, geo.boundingBox.max.z);
        let material;
        if (pos.cores) {
          geo.setAttribute('color', new THREE.BufferAttribute(pos.cores, 3));
          material = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.55, metalness: 0.05 });
        } else {
          const cor = pos.extrusor && dados.corDe ? dados.corDe(pos.extrusor) : it.cor;
          material = new THREE.MeshStandardMaterial({ color: new THREE.Color(cor.slice(0, 7)), roughness: 0.55, metalness: 0.05 });
        }
        const m = new THREE.Mesh(geo, material);
        m.position.set(-ox + (it.deslocar ? it.deslocar.x : 0), -oy + (it.deslocar ? it.deslocar.y : 0), 0);
        grupoPlaca.add(m);
      }
    }
    cena.add(grupoPlaca);
    return alturaMax;
  }

  function mostrarPlaca(i) {
    placaAtual = Math.max(0, Math.min(i, dados.nPlacas - 1));
    const h = montarPlaca(placaAtual);
    enquadrar(mesaTam.w, mesaTam.d, h);
  }

  function miniaturas(tam = 220) {
    const antes = placaAtual, w0 = renderer.domElement.width, h0 = renderer.domElement.height;
    const out = [];
    renderer.setSize(tam, tam * 0.78, false); camera.aspect = 1 / 0.78; camera.updateProjectionMatrix();
    for (let i = 0; i < dados.nPlacas; i++) {
      const h = montarPlaca(i); enquadrar(mesaTam.w, mesaTam.d, h); controles.update();
      renderer.render(cena, camera);
      out.push(renderer.domElement.toDataURL('image/png'));
    }
    renderer.setSize(w0 / renderer.getPixelRatio(), h0 / renderer.getPixelRatio(), false);
    ajustarTamanho();
    mostrarPlaca(antes);
    return out;
  }

  async function carregar(arquivo, opcoes = {}) {
    const buf = await arquivo.arrayBuffer();
    const nome = (arquivo.name || '').toLowerCase();
    if (nome.endsWith('.3mf')) {
      dados = await ler3MF(buf);
      mesaTam = dados.mesa ? { w: dados.mesa.w, d: dados.mesa.d } : (opcoes.mesa || { w: 256, d: 256 });
      // projetos com várias placas guardam as peças em posições de grade; conferimos se é o caso
      dados.origemPlacas = dados.nPlacas > 1;
      if (!dados.mesa) centralizar(dados, mesaTam);
    } else if (nome.endsWith('.stl')) {
      const pos = lerSTL(buf);
      mesaTam = opcoes.mesa || { w: 256, d: 256 };
      dados = { itens: [{ partes: [pos], placa: 0, cor: opcoes.cor || '#E0186B', nome: (arquivo.name || '').replace(/\.stl$/i, '') }], nPlacas: 1, colunas: 1, mesa: null, nomesPlaca: [''] };
      centralizar(dados, mesaTam);
    } else {
      throw new Error('Envie um arquivo .stl ou .3mf.');
    }
    mostrarPlaca(0);
    return { placas: dados.nPlacas, nomes: dados.nomesPlaca, impressora: dados.impressora || '', mesa: mesaTam };
  }

  function centralizar(d, mesa) {
    // apoia a peça na mesa e coloca no centro (STL e 3MF sem posição de mesa)
    const caixa = new THREE.Box3();
    for (const it of d.itens) for (const p of it.partes) for (let i = 0; i < p.length; i += 3) caixa.expandByPoint(new THREE.Vector3(p[i], p[i + 1], p[i + 2]));
    const dx = mesa.w / 2 - (caixa.min.x + caixa.max.x) / 2, dy = mesa.d / 2 - (caixa.min.y + caixa.max.y) / 2, dz = -caixa.min.z;
    for (const it of d.itens) for (const p of it.partes) for (let i = 0; i < p.length; i += 3) { p[i] += dx; p[i + 1] += dy; p[i + 2] += dz; }
  }

  // medidas reais de cada peça (caixa que envolve a peça), em milímetros como no arquivo
  function pecas() {
    return dados ? dados.itens.map((it, i) => {
      let x0 = Infinity, y0 = Infinity, z0 = Infinity, x1 = -Infinity, y1 = -Infinity, z1 = -Infinity;
      for (const p of it.partes) for (let k = 0; k < p.length; k += 3) {
        const x = p[k], y = p[k + 1], z = p[k + 2];
        if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; if (z < z0) z0 = z; if (z > z1) z1 = z;
      }
      return { nome: it.nome || `Peça ${i + 1}`, placa: it.placa + 1, x: +(x1 - x0).toFixed(2), y: +(y1 - y0).toFixed(2), z: +(z1 - z0).toFixed(2) };
    }) : [];
  }

  function trocarMesa(mesa) { mesaTam = mesa; if (dados && !dados.mesa) { centralizar(dados, mesa); mostrarPlaca(placaAtual); } }

  return { carregar, mostrarPlaca, miniaturas, trocarMesa, pecas, get placa() { return placaAtual; } };
}

window.criarVisualizador = criarVisualizador;
})();
