// Comedouro Filuna — texto em contornos lisos (mm, y para cima, centrado em 0,0) a partir das fontes do computador.
import { tinta } from './texto.js';
import { vetorizarLiso, campoDistancia } from './contorno.js';

const cache = new Map();

// `altura` = altura da tinta (do topo da letra mais alta ao pé da mais baixa). `espacamento` em % do tamanho da fonte.
export function textoEmRegioes(texto, fonte, negrito, espacamento, altura) {
  const limpo = (texto || '').trim();
  if (!limpo) return null;
  const chave = [limpo, fonte, negrito, espacamento].join('|');
  if (!cache.has(chave)) cache.set(chave, vetorizar(limpo, fonte, negrito, espacamento));
  const base = cache.get(chave);
  if (!base) return null;
  const esc = altura / base.alt, m = (pts) => pts.map(([x, y]) => [x * esc, y * esc]);
  return { regioes: base.regioes.map(r => ({ externo: m(r.externo), buracos: r.buracos.map(m) })), larg: base.larg * esc, alt: altura, traco: base.traco * esc };
}

function vetorizar(texto, fonte, negrito, espacamento) {
  const fonteTela = tinta(texto, fonte, negrito, false, -espacamento);
  if (!fonteTela) return null;
  const altPx = 260, pad = 12, w = Math.max(8, Math.round(fonteTela.width * altPx / fonteTela.height)) + 2 * pad, h = altPx + 2 * pad;
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  const g = c.getContext('2d', { willReadFrequently: true });
  g.imageSmoothingQuality = 'high';
  g.drawImage(fonteTela, pad, pad, w - 2 * pad, altPx);
  const d = g.getImageData(0, 0, w, h).data, mask = new Uint8Array(w * h);
  for (let i = 0; i < mask.length; i++) mask[i] = d[i * 4 + 3] > 127 ? 1 : 0;
  const regioes = vetorizarLiso(mask, w, h, { suave: 2, areaMin: 0.3, minArea: 6 });
  const cx = w / 2, cy = h / 2, m = (pts) => pts.map(([x, y]) => [x - cx, cy - y]);
  return { regioes: regioes.map(r => ({ externo: m(r.externo), buracos: r.buracos.map(m) })), larg: w - 2 * pad, alt: altPx, traco: tracoMaisFino(mask, w, h) };
}

// Largura dos traços mais finos (px): o dobro da distância nas cristas do campo, no décimo percentil.
function tracoMaisFino(mask, w, h) {
  const d = campoDistancia(mask, w, h), cristas = [];
  for (let y = 1; y < h - 1; y++) for (let x = 1; x < w - 1; x++) {
    const i = y * w + x, v = d[i];
    if (v < 1) continue;
    if (v >= d[i - 1] && v >= d[i + 1] && v >= d[i - w] && v >= d[i + w]) cristas.push(v);
  }
  if (!cristas.length) return 0;
  cristas.sort((a, b) => a - b);
  return 2 * cristas[Math.floor(cristas.length * 0.1)];
}
