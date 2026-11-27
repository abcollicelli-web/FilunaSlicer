// Comedouro Filuna — tela: suporte para tigelas de aço ou tigela impressa, nome e enfeites, conferência e exportação.
import { textoEmRegioes } from './escrita.js';
import { fonteExiste } from './texto.js';
import { FORMAS, ICONES, regioesDe, miniatura } from './formas.js';
import { Malha, montarSuporte, montarTigela, prismaFino, dobrarNaParede, distanciaZona } from './geometria.js';
import { criarVista3D } from './vista3d.js';
import { projetoBambu, stl, zip } from './exportar.js';

const $ = (id) => document.getElementById(id);
function el(tag, props = {}, ...filhos) { const e = Object.assign(document.createElement(tag), props); e.append(...filhos); return e; }
const num = (v, c = 0) => v.toLocaleString('pt-BR', { minimumFractionDigits: c, maximumFractionDigits: c });
const FONTES = ['Arial Rounded MT Bold', 'Segoe UI Black', 'Arial Black', 'Cooper Black', 'Berlin Sans FB Demi', 'Bahnschrift', 'Impact', 'Comic Sans MS', 'Segoe Print', 'Ink Free', 'Georgia', 'Verdana', 'Arial'];
const DESLIZANTES = ['dTigela', 'altura', 'base', 'inclinacao', 'largura', 'alturaT', 'alturaLetras', 'posAltura', 'espacamento', 'giro'];
const PETS = {
  suporte: { gato: { altura: 75, dTigela: 130, base: 170 }, pequeno: { altura: 100, dTigela: 150, base: 200 }, medio: { altura: 130, dTigela: 160, base: 220 }, grande: { altura: 190, dTigela: 210, base: 280 } },
  tigela: { gato: { alturaT: 45, largura: 135 }, pequeno: { alturaT: 55, largura: 150 }, medio: { alturaT: 65, largura: 170 }, grande: { alturaT: 85, largura: 220 } },
};
const TEMAS = { azul: ['#3E6FD8', '#FFFFFF', '#F5C518'], rosa: ['#EC5D9A', '#FFFFFF', '#FFD1E3'], verde: ['#2FA36B', '#FFFFFF', '#F5C518'], preto: ['#26252B', '#F5C518', '#FFFFFF'], areia: ['#E8DCC8', '#8C6A4F', '#5C4433'], vermelho: ['#D7263D', '#FFFFFF', '#26252B'] };
const estado = { modo: 'suporte', qtd: 1, perfil: 'cone', fundo: 'aberto', lugar: 'parede', escrita: 'relevo', textura: 'liso', forma: 'redonda', pet: 'medio', aco: true, corte: false };
const vista = criarVista3D($('cena'));
let resultado = null, agendado = 0;

function ler() {
  const n = (id) => { const v = parseFloat($(id).value); return Number.isFinite(v) ? v : parseFloat($(id).defaultValue); };
  return {
    ...estado, dTigela: n('dTigela'), folga: n('folga'), profTigela: n('profTigela'), vao: n('vao'),
    perfilAmp: n('perfilAmp'), aneis: n('aneis'), altura: n('altura'), base: n('base'), inclinacao: n('inclinacao'),
    parede: estado.modo === 'suporte' ? n('parede') : n('paredeT'), aba: n('aba'), tampo: n('tampo'),
    espFundo: estado.modo === 'suporte' ? n('espFundoS') : n('espFundoT'), dedos: $('dedos').checked, dedosLarg: n('dedosLarg'), dedosAlt: n('dedosAlt'),
    largura: n('largura'), alturaT: n('alturaT'), baseMais: n('baseMais'), lento: $('lento').value, lentoQtd: n('lentoQtd'), lentoAlt: n('lentoAlt'),
    nome: $('nome').value, fonte: $('fonte').value, negrito: $('negrito').checked, alturaLetras: n('alturaLetras'), posAltura: n('posAltura'),
    espacamento: n('espacamento'), giro: n('giro'), relevo: n('relevo'), profGravado: n('profGravado'), patinhasLado: $('patinhasLado').checked,
    iconeLado: $('iconeLado').value, frase: $('frase').value, textoTras: $('textoTras').value,
    texturaP: { tipo: estado.textura, amp: n('texAmp'), qtd: n('texQtd') }, padrao: $('padrao').value, padraoTam: n('padraoTam'), padraoCor: $('padraoCor').value,
    pes: Number($('pes').value), corCorpo: $('corCorpo').value, corNome: $('corNome').value, corDetalhe: $('corDetalhe').value,
    mesa: $('impressora').value.split('x').map(Number),
  };
}

const mover = (regs, dx, dy) => regs.map(r => ({ externo: r.externo.map(([x, y]) => [x + dx, y + dy]), buracos: r.buracos.map(b => b.map(([x, y]) => [x + dx, y + dy])) }));

// Nome, frase, ícones dos lados e texto de trás no plano da parede (u ao redor, v na altura), mais as zonas lisas.
function arrumarTextos(p, zc, perimetro, u0) {
  const hL = p.alturaLetras, vao = hL * 0.24, itens = [], zonas = [];
  const nome = textoEmRegioes(p.nome, p.fonte, p.negrito, p.espacamento, hL);
  const frase = textoEmRegioes(p.frase, p.fonte, p.negrito, p.espacamento, hL * 0.42);
  const tras = textoEmRegioes(p.textoTras, p.fonte, p.negrito, p.espacamento, hL * 0.6);
  const alto = (nome ? hL : 0) + (frase ? (nome ? vao : 0) + frase.alt : 0);
  const vNome = alto / 2 - hL / 2, vFrase = -alto / 2 + (frase ? frase.alt / 2 : 0);
  let meia = 0;
  if (nome) { itens.push({ tipo: 'texto', regs: nome.regioes, u: u0, v: vNome }); meia = nome.larg / 2; }
  if (nome && p.patinhasLado) {
    const s = hL * 0.82, d = nome.larg / 2 + vao + s / 2;
    for (const lado of [-1, 1]) itens.push({ tipo: 'icone', regs: regioesDe(p.iconeLado, s), u: u0 + lado * d, v: vNome });
    meia = d + s / 2;
  }
  if (frase) { itens.push({ tipo: 'texto', regs: frase.regioes, u: u0, v: vFrase }); meia = Math.max(meia, frase.larg / 2); }
  if (alto > 0) zonas.push({ u0: u0 - meia - 3, u1: u0 + meia + 3, z0: zc - alto / 2 - 3, z1: zc + alto / 2 + 3 });
  if (tras) {
    itens.push({ tipo: 'texto', regs: tras.regioes, u: u0 + perimetro / 2, v: 0 });
    zonas.push({ u0: u0 + perimetro / 2 - tras.larg / 2 - 3, u1: u0 + perimetro / 2 + tras.larg / 2 + 3, z0: zc - tras.alt / 2 - 3, z1: zc + tras.alt / 2 + 3 });
  }
  // ícones em volta, onde a parede está livre
  if (p.padrao !== 'nenhum' && p.padraoTam > 0) {
    const s = p.padraoTam, k = Math.max(3, Math.floor(perimetro / (s * 1.9)));
    for (let i = 0; i < k; i++) {
      const u = u0 + perimetro * (i + 0.5) / k;
      if (zonas.some(zn => distanciaZona(zn, u, zc, perimetro) < s * 0.75 + 2)) continue;
      itens.push({ tipo: 'padrao', regs: regioesDe(p.padrao, s), u, v: 0 });
    }
    zonas.push(...itens.filter(i => i.tipo === 'padrao').map(i => ({ u0: i.u - s / 2 - 2, u1: i.u + s / 2 + 2, z0: zc - s / 2 - 2, z1: zc + s / 2 + 2 })));
  }
  return { itens, zonas, nome, frase, tras, alto, meia };
}

function gerar() {
  const p = ler(), avisos = [];
  const comCor = (m, cor, extra = {}) => ({ pos: m.pos, idx: m.idx, corVista: cor, ...extra });
  const nomeM = new Malha(), detalheM = new Malha(), gravadoM = new Malha(), vistaGravado = new Malha();
  const gravado = p.escrita === 'gravado', fundo = gravado ? -Math.min(p.profGravado, p.parede - 1) : -0.25;
  let corpo, zc, layout, peca;

  if (p.modo === 'suporte') {
    const alturaFrente = () => peca.frenteAltura - peca.tampo;
    peca = montarSuporte({ ...p, textura: p.texturaP, qtd: estado.qtd }, (cont) => {
      const altFrente = p.altura - (cont.noU(0, 0).y * -1) * Math.tan(p.inclinacao * Math.PI / 180);
      const zMin = estado.fundo === 'fechado' ? p.espFundo : 0, zMax = altFrente - Math.max(p.tampo, 2.4);
      zc = zMin + (zMax - zMin) * p.posAltura / 100;
      layout = arrumarTextos(p, zc, cont.perimetro(zc), 0);
      return layout.zonas;
    });
    corpo = peca.corpo;
    const zMax = alturaFrente(), zMin = peca.fundoAltura;
    if (layout.alto && (zc + layout.alto / 2 > zMax - 1 || zc - layout.alto / 2 < zMin + 1)) avisos.push(['aviso', 'O nome passa da parede na altura escolhida: diminua as letras ou mude a "Altura na parede".']);
    if (p.altura - p.profTigela < peca.fundoAltura + 3) avisos.push(['aviso', `A tigela de aço (${num(p.profTigela)} mm) chega no chão: aumente a altura do suporte para pelo menos ${num(p.profTigela + peca.fundoAltura + 6)} mm.`]);
    else avisos.push(['ok', `A tigela de aço fica a ${num(p.altura - p.profTigela - peca.fundoAltura)} mm do chão.`]);
    if (peca.R0 < peca.rT + 4) avisos.push(['aviso', 'Base quase do tamanho do topo: com o pet empurrando, o suporte pode tombar. Aumente a base.']);
    if (peca.frenteAltura < 30) avisos.push(['aviso', 'Com essa inclinação a frente fica muito baixa: diminua a inclinação ou aumente a altura.']);
  } else {
    peca = montarTigela({ ...p, altura: p.alturaT, textura: p.texturaP, zonasDe: (cont) => {
      zc = p.alturaT * p.posAltura / 100;
      const P = cont.perimetro(zc);
      // com o nome no fundo, a parede ainda leva o texto de trás e os ícones em volta
      layout = arrumarTextos(estado.lugar === 'fundo' ? { ...p, nome: '', frase: '' } : p, zc, P, P * p.giro / 360);
      return layout.zonas;
    } });
    corpo = peca.corpo;
    if (layout.alto && (zc + layout.alto / 2 > p.alturaT - 2 || zc - layout.alto / 2 < 2)) avisos.push(['aviso', 'O nome passa da parede: diminua as letras ou mude a "Altura na parede".']);
    if (peca.cavidade.rTop < 25) avisos.push(['aviso', 'O espaço da comida ficou pequeno para esse formato: aumente a largura ou afine a parede.']);
    avisos.push(['ok', `Cabem cerca de ${num(peca.ml)} ml (enchendo até 90%).`]);
    if (estado.lugar === 'fundo') {
      const c = peca.cavidade, nome = textoEmRegioes(p.nome, p.fonte, p.negrito, p.espacamento, p.alturaLetras);
      if (nome && p.lento !== 'nenhum') avisos.push(['aviso', 'Com comedouro lento o fundo fica ocupado: o nome vai melhor na parede.']);
      else if (nome) {
        const maxL = (c.rBot - 6) * 2, esc = Math.min(1, maxL / nome.larg);
        if (esc < 1) avisos.push(['aviso', `O nome não cabia no fundo e foi diminuído para ${num(p.alturaLetras * esc)} mm de altura.`]);
        const regs = nome.regioes.map(r => ({ externo: r.externo.map(([x, y]) => [c.cx + x * esc, c.cy + y * esc]), buracos: r.buracos.map(b => b.map(([x, y]) => [c.cx + x * esc, c.cy + y * esc])) }));
        if (gravado) { gravadoM.juntar(prismaFino(regs, c.tf + fundo, c.tf + 1, 2)); vistaGravado.juntar(prismaFino(regs, c.tf - 0.2, c.tf + 0.06, 2)); } else nomeM.juntar(prismaFino(regs, c.tf - 0.2, c.tf + p.relevo, 2));
        layout = { ...layout, nome };
      }
    }
  }

  // nome, frase, ícones e texto de trás dobrados na parede
  for (const it of layout.itens) {
    const regs = mover(it.regs, 0, it.v), destino = it.tipo === 'padrao' && p.padraoCor === 'detalhe' ? detalheM : nomeM;
    if (gravado) {
      gravadoM.juntar(dobrarNaParede(prismaFino(regs, fundo, 1.2), peca.contorno, it.u, zc));
      vistaGravado.juntar(dobrarNaParede(prismaFino(regs, -0.3, 0.06), peca.contorno, it.u, zc));
    } else destino.juntar(dobrarNaParede(prismaFino(regs, -0.25, p.relevo), peca.contorno, it.u, zc));
  }

  // conferência
  const traco = Math.min(...[layout.nome, layout.frase, layout.tras].filter(Boolean).map(t => t.traco));
  if (Number.isFinite(traco)) {
    if (traco < 0.9) avisos.push(['aviso', `Traço mais fino das letras: ${num(traco, 1)} mm. Para o bico de 0,4 mm use negrito, letras maiores ou outra fonte (ideal ≥ 1 mm).`]);
    else avisos.push(['ok', `Letras com traço de ${num(traco, 1)} mm: o bico de 0,4 mm desenha bem.`]);
  }
  if (layout.meia && peca.contorno && layout.meia * 2 > peca.contorno.perimetro(zc) * 0.46) avisos.push(['aviso', 'O nome dá mais de meia volta: diminua as letras ou o espaço entre elas.']);
  if (p.parede < 1.2) avisos.push(['erro', 'Parede fina demais para imprimir: use pelo menos 1,2 mm.']);
  if (gravado && p.profGravado > p.parede - 1) avisos.push(['aviso', `O gravado foi limitado a ${num(p.parede - 1, 1)} mm para não furar a parede.`]);

  const partesVista = [comCor(corpo, p.corCorpo), ...peca.positivos.map(m => comCor(m, p.corCorpo)), comCor(nomeM, p.corNome), comCor(detalheM, p.corDetalhe), comCor(vistaGravado, escurecer(p.corCorpo))];
  for (const ng of peca.negativos) {
    if (ng.nome === 'Encaixe para os dedos') partesVista.push(comCor(ng.vista || ng.malha, '#0E0D10', { transparente: true }));
    else if (ng.nome === 'Vazado') partesVista.push(comCor(ng.malha, '#0E0D10'));
  }
  const caixa = juntarCaixa([corpo, ...peca.positivos, nomeM, detalheM]);
  const [W, D, Z] = p.mesa, larg = caixa[3] - caixa[0], prof = caixa[4] - caixa[1], alt = caixa[5] - caixa[2];
  if (larg > W - 6 || prof > D - 6 || alt > Z) {
    let dica = '';
    if (p.modo === 'suporte' && estado.qtd > 1 && larg > W - 6) {
      const sobra = larg - p.dTigela * estado.qtd, cabe = Math.floor((W - 6 - sobra) / estado.qtd);
      dica = cabe >= 80 ? ` Nessa mesa cabem ${estado.qtd} tigelas de até ${cabe} mm, ou escolha uma mesa maior.` : ' Escolha uma mesa maior ou menos tigelas.';
    }
    avisos.push(['erro', `Não cabe na mesa escolhida (${W} × ${D} × ${Z} mm): a peça tem ${num(larg)} × ${num(prof)} × ${num(alt)} mm.${dica}`]);
  }
  else avisos.unshift(['ok', `Cabe na mesa: ${num(larg)} × ${num(prof)} × ${num(alt)} mm.`]);

  const vol = corpo.volume() + peca.positivos.reduce((s, m) => s + m.volume(), 0) + nomeM.volume() + detalheM.volume() - gravadoM.volume() * 0.7;
  resultado = { p, peca, corpo, nomeM, detalheM, gravadoM, caixa, gramas: vol / 1000 * 1.27 };
  const chave = [p.modo, estado.qtd, Math.round(larg / 10), Math.round(alt / 10)].join('|');
  vista.mostrar(partesVista, p.modo === 'suporte' && estado.aco ? peca.fantasmas : [], chave);
  mostrarConferencia(avisos);
  const capacidade = p.modo === 'tigela' ? [`${num(peca.ml)} ml`, 'de comida'] : [`${num(peca.rb * 2, 1)} mm`, estado.qtd > 1 ? `${estado.qtd} furos` : 'furo da tigela'];
  $('numeros').replaceChildren(...[[`${num(larg)}×${num(prof)}×${num(alt)}`, 'mm'], capacidade, [`≈ ${num(resultado.gramas)} g`, 'de PETG']].map(([b, s]) => el('div', {}, el('b', { textContent: b }), el('span', { textContent: s }))));
  $('medidas').innerHTML = `${num(larg)} × ${num(prof)} × ${num(alt)} mm<span>${p.modo === 'suporte' ? `Furo ${num(peca.rb * 2, 1)} mm · tampo ${num(peca.tampo, 1)} mm` : `Comporta cerca de ${num(peca.ml)} ml`}</span>`;
}

const escurecer = (hex) => { const v = parseInt(hex.slice(1), 16), f = (s) => Math.round(((v >> s) & 255) * 0.45); return `#${[16, 8, 0].map(s => f(s).toString(16).padStart(2, '0')).join('')}`; };
function juntarCaixa(malhas) {
  const c = [Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity];
  for (const m of malhas) if (m.pos.length) { const b = m.caixa(); for (let k = 0; k < 3; k++) { c[k] = Math.min(c[k], b[k]); c[k + 3] = Math.max(c[k + 3], b[k + 3]); } }
  return c;
}
function mostrarConferencia(avisos) {
  const ordem = { erro: 0, aviso: 1, ok: 2 };
  avisos.sort((a, b) => ordem[a[0]] - ordem[b[0]]);
  $('verificacao').replaceChildren(...avisos.map(([tipo, t]) => el('div', { className: tipo, textContent: (tipo === 'ok' ? '✓ ' : tipo === 'erro' ? '✕ ' : '! ') + t })));
  $('b3mf').disabled = avisos.some(a => a[0] === 'erro');
}

function agendar() {
  clearTimeout(agendado); $('ocupado').hidden = false;
  agendado = setTimeout(() => { try { gerar(); } catch (e) { mostrarConferencia([['erro', 'Não consegui montar a peça com essas medidas: ' + ((e && e.message) || e)]]); console.error(e); } $('ocupado').hidden = true; }, 140);
}

// ---------- exportar ----------
function partesExportar() {
  const r = resultado, p = r.p, [W, D] = p.mesa, dx = W / 2 - (r.caixa[0] + r.caixa[3]) / 2, dy = D / 2 - (r.caixa[1] + r.caixa[4]) / 2;
  const levar = (m) => { const pos = m.pos.slice(); for (let i = 0; i < pos.length; i += 3) { pos[i] += dx; pos[i + 1] += dy; } return { pos, idx: m.idx }; };
  const partes = [{ nome: 'Corpo', extrusora: 1, cor: p.corCorpo, ...levar(r.corpo) }];
  if (r.peca.positivos.length) { const m = new Malha(); r.peca.positivos.forEach(x => m.juntar(x)); partes.push({ nome: p.modo === 'tigela' && p.lento !== 'nenhum' ? 'Comedouro lento' : 'Apoio dos pés', extrusora: 1, cor: p.corCorpo, ...levar(m) }); }
  if (r.nomeM.idx.length) partes.push({ nome: 'Nome', extrusora: 2, cor: p.corNome, ...levar(r.nomeM) });
  if (r.detalheM.idx.length) partes.push({ nome: 'Detalhes', extrusora: 3, cor: p.corDetalhe, ...levar(r.detalheM) });
  if (r.gravadoM.idx.length) partes.push({ nome: 'Gravação', extrusora: 1, negativa: true, ...levar(r.gravadoM) });
  const porTipo = new Map();
  for (const ng of r.peca.negativos) { if (!porTipo.has(ng.nome)) porTipo.set(ng.nome, new Malha()); porTipo.get(ng.nome).juntar(ng.malha); }
  for (const [nome, m] of porTipo) partes.push({ nome, extrusora: 1, negativa: true, ...levar(m) });
  return partes;
}
const semAcento = (s) => (s || '').normalize('NFD').replace(/[̀-ͯ]/g, '').replace(/[^\w-]+/g, '-').replace(/^-|-$/g, '').toLowerCase();
function baixar(nome, blob) {
  const url = URL.createObjectURL(blob), a = el('a', { href: url, download: nome });
  document.body.appendChild(a); a.click(); a.remove(); setTimeout(() => URL.revokeObjectURL(url), 4000);
}
async function exportar(tipo) {
  if (!resultado) return;
  const p = resultado.p, base = `comedouro-${semAcento(p.nome) || 'pet'}`, partes = partesExportar();
  if (tipo === '3mf') {
    const ajustes = { wall_loops: '3', top_shell_layers: '5', bottom_shell_layers: '4', sparse_infill_density: '15%', enable_support: '0' };
    baixar(`${base}.3mf`, await projetoBambu([{ nome: `Comedouro ${p.nome}`.trim(), mesa: 0, mesaNome: p.nome, ajustes, partes }], 1, `Comedouro ${p.nome}`));
    $('info').textContent = 'Pronto: o comedouro foi para a Preparação com cada cor na sua extrusora. Imprima em pé, sem suporte.';
  } else {
    const positivas = partes.filter(x => !x.negativa);
    const arquivos = positivas.map(pt => [`${base}-${semAcento(pt.nome)}-extrusora-${pt.extrusora}.stl`, stl(pt, `${pt.nome} - Filuna`)]);
    arquivos.push(['LEIA-ME.txt', new TextEncoder().encode('Comedouro Filuna\n\nAbra todos os STL juntos, como uma peça só de várias partes, e ponha cada parte na sua extrusora.\n' +
      (partes.some(x => x.negativa) ? 'Gravação, encaixes, vazados e rebaixos dos pés só saem no 3MF (lá eles recortam o corpo na hora de fatiar).\n' : ''))]);
    baixar(`${base}-stl.zip`, await zip(arquivos));
  }
}

// ---------- ligações ----------
function marcar(seletor, chave, valor) { document.querySelectorAll(seletor).forEach(b => b.classList.toggle('on', b.dataset[chave] === String(valor))); }
function trocarModo(m) {
  estado.modo = m; marcar('[data-modo]', 'modo', m);
  document.querySelectorAll('.so-suporte').forEach(e => { e.hidden = m !== 'suporte'; });
  document.querySelectorAll('.so-tigela').forEach(e => { e.hidden = m !== 'tigela'; });
  aplicarPet(estado.pet); atualizarVisiveis();
}
function aplicarPet(pet) {
  estado.pet = pet; marcar('[data-pet]', 'pet', pet);
  for (const [k, v] of Object.entries(PETS[estado.modo][pet])) { $(k).value = v; if ($(k + 'N')) $(k + 'N').value = v; }
  $('alturaLetras').value = $('alturaLetrasN').value = { gato: 20, pequeno: 24, medio: 30, grande: 38 }[pet];
  agendar();
}
function atualizarVisiveis() {
  const mostrar = (sel, sim) => document.querySelectorAll(sel).forEach(e => { e.hidden = !sim; });
  mostrar('.perfil-amp', ['aneis', 'barriga', 'cintura'].includes(estado.perfil)); mostrar('.so-aneis', estado.perfil === 'aneis');
  mostrar('.so-fechado', estado.fundo === 'fechado' && estado.modo === 'suporte'); mostrar('.so-dedos', $('dedos').checked && estado.modo === 'suporte');
  mostrar('.multi', estado.qtd > 1); mostrar('.so-lento', $('lento').value !== 'nenhum' && estado.modo === 'tigela');
  mostrar('.so-relevo', estado.escrita === 'relevo'); mostrar('.so-gravado', estado.escrita === 'gravado');
  mostrar('.so-textura', estado.textura !== 'liso'); mostrar('.so-padrao', $('padrao').value !== 'nenhum');
  if (estado.modo !== 'suporte') mostrar('.so-suporte', false);
  if (estado.modo !== 'tigela') mostrar('.so-tigela', false);
}
async function enviarFonte(arq) {
  if (!arq) return;
  try {
    const nome = arq.name.replace(/\.[^.]+$/, '').slice(0, 40), face = new FontFace(nome, await arq.arrayBuffer());
    await face.load(); document.fonts.add(face);
    $('fonte').append(el('option', { value: nome, textContent: nome })); $('fonte').value = nome; agendar();
  } catch { $('info').textContent = 'Não consegui ler essa fonte. Use um arquivo .ttf ou .otf.'; }
}

function iniciar() {
  $('fonte').append(...FONTES.filter(fonteExiste).map(f => el('option', { value: f, textContent: f })));
  $('iconeLado').append(...ICONES.map(i => el('option', { value: i.id, textContent: i.nome.replace(/s$/, '').replace(/õe$/, 'ão').replace(/çõe$/, 'ção') })));
  $('padrao').append(el('option', { value: 'nenhum', textContent: 'Nenhum' }), ...ICONES.map(i => el('option', { value: i.id, textContent: i.nome })));
  $('formas').append(...FORMAS.map(f => { const b = el('button', { type: 'button', className: f.id === estado.forma ? 'on' : '' }); b.dataset.forma = f.id; b.innerHTML = miniatura(f.id) + `<span>${f.nome}</span>`; return b; }));
  const clique = (sel, chave, depois) => document.querySelectorAll(sel).forEach(b => b.addEventListener('click', () => { estado[chave] = b.dataset[chave] === undefined ? estado[chave] : (isNaN(+b.dataset[chave]) ? b.dataset[chave] : +b.dataset[chave]); marcar(sel, chave, b.dataset[chave]); if (depois) depois(); atualizarVisiveis(); agendar(); }));
  clique('[data-qtd]', 'qtd', () => { if (estado.qtd > 1 && +$('base').value < 160) $('base').value = $('baseN').value = 180; });
  clique('[data-perfil]', 'perfil'); clique('[data-fundo]', 'fundo'); clique('[data-lugar]', 'lugar'); clique('[data-escrita]', 'escrita');
  clique('[data-textura]', 'textura', () => { if (estado.textura === 'vazados') $('texQtd').value = 14; else if (+$('texQtd').value < 20) $('texQtd').value = 36; });
  clique('#formas [data-forma]', 'forma', () => { if (estado.forma !== 'redonda' && estado.lugar === 'parede' && ['coracao', 'estrela', 'coroa', 'flor'].includes(estado.forma)) { estado.lugar = 'fundo'; marcar('[data-lugar]', 'lugar', 'fundo'); } });
  document.querySelectorAll('[data-modo]').forEach(b => b.addEventListener('click', () => trocarModo(b.dataset.modo)));
  document.querySelectorAll('[data-pet]').forEach(b => b.addEventListener('click', () => aplicarPet(b.dataset.pet)));
  document.querySelectorAll('[data-tema]').forEach(b => b.addEventListener('click', () => { const [a, n, d] = TEMAS[b.dataset.tema]; $('corCorpo').value = a; $('corNome').value = n; $('corDetalhe').value = d; marcar('[data-tema]', 'tema', b.dataset.tema); agendar(); }));
  for (const id of DESLIZANTES) {
    $(id).addEventListener('input', () => { $(id + 'N').value = $(id).value; agendar(); });
    $(id + 'N').addEventListener('change', () => { $(id).value = $(id + 'N').value; agendar(); });
  }
  document.querySelectorAll('.lp input:not([type=range]):not([type=file]), .lp select').forEach(e => { if (!DESLIZANTES.includes(e.id.replace(/N$/, ''))) e.addEventListener(e.type === 'text' ? 'input' : 'change', () => { atualizarVisiveis(); agendar(); }); });
  $('focinho').addEventListener('change', () => { $('inclinacao').value = $('inclinacaoN').value = $('focinho').checked ? 15 : 0; agendar(); });
  $('impressora').addEventListener('change', agendar);
  $('arquivoFonte').addEventListener('change', (e) => enviarFonte(e.target.files[0]));
  $('verAco').addEventListener('click', () => { estado.aco = !estado.aco; $('verAco').classList.toggle('on', estado.aco); agendar(); });
  $('verCorte').addEventListener('click', () => { estado.corte = !estado.corte; $('verCorte').classList.toggle('on', estado.corte); vista.cortar(estado.corte); });
  $('b3mf').addEventListener('click', () => exportar('3mf'));
  $('bzip').addEventListener('click', () => exportar('zip'));
  if (!$('info')) $('verificacao').before(el('p', { className: 'info', id: 'info' }));
  atualizarVisiveis();
  document.fonts.ready.then(agendar);
}
iniciar();
