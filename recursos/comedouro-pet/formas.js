// Comedouro Filuna — formas desenhadas: o contorno das tigelas impressas e os ícones (patinha, osso, coração, estrela,
// peixe, coroa) que vão na parede. Tudo é desenhado numa tela e vira contorno liso em milímetros (y para cima).
import { vetorizarLiso, areaAssinada } from './contorno.js';

export const FORMAS = [
  { id: 'redonda', nome: 'Redonda' }, { id: 'coracao', nome: 'Coração' }, { id: 'gato', nome: 'Gato' }, { id: 'cao', nome: 'Cão' },
  { id: 'patinha', nome: 'Patinha' }, { id: 'osso', nome: 'Osso' }, { id: 'peixe', nome: 'Peixe' }, { id: 'coroa', nome: 'Coroa' },
  { id: 'estrela', nome: 'Estrela' }, { id: 'flor', nome: 'Flor' }, { id: 'hexagono', nome: 'Hexágono' },
];
export const ICONES = [
  { id: 'patinha', nome: 'Patinhas' }, { id: 'osso', nome: 'Ossos' }, { id: 'coracao', nome: 'Corações' },
  { id: 'estrela', nome: 'Estrelas' }, { id: 'peixe', nome: 'Peixes' }, { id: 'coroa', nome: 'Coroas' },
];

const elipse = (g, x, y, rx, ry, giro = 0) => { g.moveTo(x + rx * Math.cos(giro), y + rx * Math.sin(giro)); g.ellipse(x, y, rx, ry, giro, 0, Math.PI * 2); };
const circulo = (g, x, y, r) => elipse(g, x, y, r, r);
function poligono(g, pts) { g.moveTo(pts[0][0], pts[0][1]); for (const p of pts.slice(1)) g.lineTo(p[0], p[1]); g.closePath(); }
function estrela(g, pontas, r1, r2, giro = -Math.PI / 2) {
  const pts = [];
  for (let i = 0; i < pontas * 2; i++) { const a = giro + i * Math.PI / pontas, r = i % 2 ? r2 : r1; pts.push([r * Math.cos(a), r * Math.sin(a)]); }
  poligono(g, pts);
}

// Cada desenho ocupa o quadrado [-1, 1] (y para baixo, como na tela). `unida`: as partes soltas (dedos da patinha) encostam
// no corpo, para a tigela sair numa peça só; no ícone ficam soltas.
const DESENHOS = {
  redonda: (g) => circulo(g, 0, 0, 1),
  coracao: (g) => {
    g.moveTo(0, 0.95);
    g.bezierCurveTo(-0.35, 0.68, -1, 0.28, -1, -0.28); g.bezierCurveTo(-1, -0.78, -0.42, -1, 0, -0.55);
    g.bezierCurveTo(0.42, -1, 1, -0.78, 1, -0.28); g.bezierCurveTo(1, 0.28, 0.35, 0.68, 0, 0.95); g.closePath();
  },
  gato: (g) => {
    circulo(g, 0, 0.12, 0.84);
    poligono(g, [[-0.86, -0.12], [-0.78, -0.98], [-0.18, -0.6]]); poligono(g, [[0.86, -0.12], [0.78, -0.98], [0.18, -0.6]]);
  },
  cao: (g) => {
    circulo(g, 0, 0, 0.74);
    elipse(g, -0.72, 0.1, 0.27, 0.58, 0.35); elipse(g, 0.72, 0.1, 0.27, 0.58, -0.35); elipse(g, 0, 0.5, 0.42, 0.34);
  },
  patinha: (g, unida) => {
    elipse(g, 0, 0.36, 0.56, 0.48);
    const k = unida ? 0.82 : 1;
    for (const [x, y, r] of [[-0.66, -0.12, 0.24], [-0.26, -0.58, 0.27], [0.26, -0.58, 0.27], [0.66, -0.12, 0.24]]) circulo(g, x * k, y * k + (unida ? 0.04 : 0), r * (unida ? 1.12 : 1));
  },
  osso: (g) => {
    g.rect(-0.62, -0.2, 1.24, 0.4);
    for (const [x, y] of [[-0.72, -0.24], [-0.72, 0.24], [0.72, -0.24], [0.72, 0.24]]) circulo(g, x, y, 0.28);
  },
  peixe: (g) => { elipse(g, -0.14, 0, 0.72, 0.48); poligono(g, [[0.42, 0], [1, -0.46], [0.9, 0], [1, 0.46]]); },
  coroa: (g) => {
    poligono(g, [[-0.9, 0.62], [-0.9, -0.3], [-0.48, 0.08], [0, -0.52], [0.48, 0.08], [0.9, -0.3], [0.9, 0.62]]);
    for (const [x, y] of [[-0.9, -0.42], [0, -0.66], [0.9, -0.42]]) circulo(g, x, y, 0.15);
  },
  estrela: (g) => estrela(g, 5, 1, 0.5),
  flor: (g) => { for (let i = 0; i < 6; i++) { const a = i * Math.PI / 3; circulo(g, 0.55 * Math.cos(a), 0.55 * Math.sin(a), 0.42); } circulo(g, 0, 0, 0.5); },
  hexagono: (g) => { const pts = []; for (let i = 0; i < 6; i++) { const a = Math.PI / 6 + i * Math.PI / 3; pts.push([Math.cos(a), Math.sin(a)]); } poligono(g, pts); },
};

const cache = new Map();

// Regiões lisas do desenho, com a maior medida = `tamanho` mm, centradas em (0, 0) e y para cima.
export function regioesDe(id, tamanho, unida = false) {
  const chave = `${id}|${unida}`;
  if (!cache.has(chave)) cache.set(chave, desenharNormalizado(id, unida));
  const { regioes, larg, alt } = cache.get(chave), esc = tamanho / Math.max(larg, alt);
  const m = (pts) => pts.map(([x, y]) => [x * esc, y * esc]);
  return regioes.map(r => ({ externo: m(r.externo), buracos: r.buracos.map(m) }));
}

function desenharNormalizado(id, unida) {
  const S = 512, pad = 24, c = document.createElement('canvas');
  c.width = c.height = S + 2 * pad;
  const g = c.getContext('2d', { willReadFrequently: true });
  g.translate(pad + S / 2, pad + S / 2); g.scale(S / 2, S / 2);
  g.beginPath(); (DESENHOS[id] || DESENHOS.redonda)(g, unida); g.fillStyle = '#000'; g.fill('nonzero');
  const d = g.getImageData(0, 0, c.width, c.height).data, w = c.width, mask = new Uint8Array(w * w);
  for (let i = 0; i < mask.length; i++) mask[i] = d[i * 4 + 3] > 127 ? 1 : 0;
  const brutas = vetorizarLiso(mask, w, w, { suave: 2, areaMin: 0.6, minArea: 40 });
  let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
  for (const r of brutas) for (const [x, y] of r.externo) { x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
  const cx = (x0 + x1) / 2, cy = (y0 + y1) / 2, m = (pts) => pts.map(([x, y]) => [x - cx, cy - y]);
  return { regioes: brutas.map(r => ({ externo: m(r.externo), buracos: r.buracos.map(m) })), larg: x1 - x0, alt: y1 - y0 };
}

// Contorno de uma só peça (o maior laço), anti-horário, para a parede da tigela.
export function contornoDe(id, largura) {
  const regs = regioesDe(id, largura, true);
  let melhor = regs[0].externo;
  for (const r of regs) if (Math.abs(areaAssinada(r.externo)) > Math.abs(areaAssinada(melhor))) melhor = r.externo;
  return areaAssinada(melhor) > 0 ? melhor : [...melhor].reverse();
}

// Miniatura para os botões de escolha (SVG feito do mesmo contorno).
export function miniatura(id, icone = false) {
  const regs = regioesDe(id, 20, !icone);
  const d = regs.map(r => [r.externo, ...r.buracos].map(l => 'M' + l.map(([x, y]) => `${(x + 12).toFixed(1)} ${(12 - y).toFixed(1)}`).join('L') + 'Z').join('')).join('');
  return `<svg viewBox="0 0 24 24" width="26" height="26"><path d="${d}" fill="currentColor" fill-rule="evenodd"/></svg>`;
}
