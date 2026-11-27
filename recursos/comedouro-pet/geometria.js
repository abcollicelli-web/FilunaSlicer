// Comedouro Filuna — geometria (mm, z para cima, a frente olha para -y). Duas peças:
//  • suporte para 1, 2 ou 3 tigelas de aço: casca com perfil (cone, anéis, barriga, cintura), tampo inclinado opcional
//    com os furos das tigelas, fundo aberto ou fechado, ressaltos com rebaixo para pés de borracha e encaixes para os dedos;
//  • tigela impressa em várias formas: parede no formato escolhido, cavidade redonda com canto arredondado no fundo e
//    comedouro lento opcional (aletas ou pinos).
// Nome, frase, texto de trás e ícones são desenhados planos e enrolados na parede (ou deitados no fundo).
// Tudo sai fechado (sólido) e imprime em pé, sem suporte.
import earcut from './earcut.js';
import { areaAssinada, pontoDentro } from './contorno.js';
import { contornoDe } from './formas.js';

const PI = Math.PI;
const limitar = (v, a, b) => Math.max(a, Math.min(b, v));
const suaveDegrau = (x) => { const t = limitar(x, 0, 1); return t * t * (3 - 2 * t); };

export class Malha {
  constructor() { this.pos = []; this.idx = []; }
  v(x, y, z) { this.pos.push(x, y, z); return this.pos.length / 3 - 1; }
  t(a, b, c) { this.idx.push(a, b, c); }
  juntar(o) {
    const base = this.pos.length / 3;
    for (let i = 0; i < o.pos.length; i++) this.pos.push(o.pos[i]);
    for (let i = 0; i < o.idx.length; i++) this.idx.push(o.idx[i] + base);
    return this;
  }
  volume() {
    let s = 0;
    const P = this.pos, I = this.idx;
    for (let i = 0; i < I.length; i += 3) {
      const a = I[i] * 3, b = I[i + 1] * 3, c = I[i + 2] * 3;
      s += P[a] * (P[b + 1] * P[c + 2] - P[b + 2] * P[c + 1]) - P[a + 1] * (P[b] * P[c + 2] - P[b + 2] * P[c]) + P[a + 2] * (P[b] * P[c + 1] - P[b + 1] * P[c]);
    }
    return s / 6;
  }
  caixa() {
    const c = [Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity];
    for (let i = 0; i < this.pos.length; i += 3) for (let k = 0; k < 3; k++) { c[k] = Math.min(c[k], this.pos[i + k]); c[k + 3] = Math.max(c[k + 3], this.pos[i + k]); }
    return c;
  }
}

function normalTri(P, a, b, c) {
  const ux = P[b * 3] - P[a * 3], uy = P[b * 3 + 1] - P[a * 3 + 1], uz = P[b * 3 + 2] - P[a * 3 + 2];
  const vx = P[c * 3] - P[a * 3], vy = P[c * 3 + 1] - P[a * 3 + 1], vz = P[c * 3 + 2] - P[a * 3 + 2];
  return [uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx];
}

// Liga anéis (listas de índices do mesmo tamanho, de baixo para cima) numa parede fechada. `ref(i)` dá para onde a face
// do primeiro anel deve olhar naquele ponto; a ordem dos triângulos é escolhida para obedecer.
function costurar(m, aneis, ref) {
  const n = aneis[0].length;
  let sinal = 1;
  const [a, b, c] = [aneis[0][0], aneis[0][1 % n], aneis[1][1 % n]], nn = normalTri(m.pos, a, b, c), r = ref(0);
  if (nn[0] * r[0] + nn[1] * r[1] + nn[2] * r[2] < 0) sinal = -1;
  for (let j = 0; j + 1 < aneis.length; j++) {
    const A = aneis[j], B = aneis[j + 1];
    for (let i = 0; i < n; i++) {
      const i2 = (i + 1) % n;
      if (sinal > 0) { m.t(A[i], A[i2], B[i2]); m.t(A[i], B[i2], B[i]); } else { m.t(A[i], B[i2], A[i2]); m.t(A[i], B[i], B[i2]); }
    }
  }
}

// Tampa plana: laço externo + furos (índices já na malha), projetados por `proj` em 2D. `antiHorario`: as faces ficam
// anti-horárias na projeção (olhando para o lado de onde a projeção é vista).
function tampa(m, externo, furos, proj, antiHorario) {
  const plano = [], buracos = [], ids = [];
  const somar = (laco) => { for (const i of laco) { const p = proj(i); plano.push(p[0], p[1]); ids.push(i); } };
  somar(externo);
  for (const f of furos) { buracos.push(ids.length); somar(f); }
  const tris = earcut(plano, buracos.length ? buracos : null);
  // o earcut devolve tudo no mesmo sentido (inclusive as lascas quase sem área): decide uma vez, pela soma
  const virar = (areaTotal(plano, tris) > 0) !== antiHorario;
  for (let k = 0; k < tris.length; k += 3) {
    if (virar) m.t(ids[tris[k]], ids[tris[k + 2]], ids[tris[k + 1]]); else m.t(ids[tris[k]], ids[tris[k + 1]], ids[tris[k + 2]]);
  }
}

function areaTotal(plano, tris) {
  let s = 0;
  for (let k = 0; k < tris.length; k += 3) {
    const [a, b, c] = [tris[k], tris[k + 1], tris[k + 2]];
    s += (plano[2 * b] - plano[2 * a]) * (plano[2 * c + 1] - plano[2 * a + 1]) - (plano[2 * c] - plano[2 * a]) * (plano[2 * b + 1] - plano[2 * a + 1]);
  }
  return s;
}

function anelDe(m, pontos) { return pontos.map(p => m.v(p[0], p[1], p[2])); }

// Cilindro (ou prisma de seção elíptica) fechado entre dois pontos, para recortes e ressaltos.
function tubo(centro0, centro1, eixoA, eixoB, ra, rb, n = 40) {
  const m = new Malha(), anel = (c) => {
    const pts = [];
    for (let i = 0; i < n; i++) {
      const t = i / n * 2 * PI, ca = Math.cos(t) * ra, sb = Math.sin(t) * rb;
      pts.push([c[0] + eixoA[0] * ca + eixoB[0] * sb, c[1] + eixoA[1] * ca + eixoB[1] * sb, c[2] + eixoA[2] * ca + eixoB[2] * sb]);
    }
    return anelDe(m, pts);
  };
  const A = anel(centro0), B = anel(centro1), eixo = [centro1[0] - centro0[0], centro1[1] - centro0[1], centro1[2] - centro0[2]];
  costurar(m, [A, B], (i) => [m.pos[A[i] * 3] - centro0[0], m.pos[A[i] * 3 + 1] - centro0[1], m.pos[A[i] * 3 + 2] - centro0[2]]);
  const cA = m.v(...centro0), cB = m.v(...centro1);
  for (let i = 0; i < n; i++) {
    const i2 = (i + 1) % n, nn = normalTri(m.pos, cA, A[i], A[i2]);
    if (nn[0] * eixo[0] + nn[1] * eixo[1] + nn[2] * eixo[2] > 0) { m.t(cA, A[i2], A[i]); m.t(cB, B[i], B[i2]); } else { m.t(cA, A[i], A[i2]); m.t(cB, B[i2], B[i]); }
  }
  return m;
}
const cilindroZ = (x, y, z0, z1, r, n = 40) => tubo([x, y, z0], [x, y, z1], [1, 0, 0], [0, 1, 0], r, r, n);

// ---------- contornos com parâmetro de comprimento (u = 0 na frente, crescendo para a direita de quem olha) ----------

// Estádio (círculo quando L = 0) com raio que muda com a altura: o suporte de 1, 2 ou 3 tigelas.
function contornoEstadio(L, raio, rRef, passo) {
  const am = [], nr = (v) => Math.max(0, Math.ceil(v / passo)), na = Math.max(24, Math.ceil(PI * rRef / passo));
  const reta = (k, x0, x1) => { const q = nr(Math.abs(x1 - x0)); for (let i = 0; i < q; i++) am.push({ k, a: x0 + (x1 - x0) * i / q }); };
  const arco = (k, f0) => { for (let i = 0; i < na; i++) am.push({ k, a: f0 + PI * i / na }); };
  reta('A', 0, L); arco('R', -PI / 2); reta('C', L, -L); arco('E', PI / 2); reta('F', -L, 0);
  const ponto = (s, r) => {
    switch (s.k) {
      case 'A': case 'F': return { x: s.a, y: -r, nx: 0, ny: -1 };
      case 'C': return { x: s.a, y: r, nx: 0, ny: 1 };
      case 'R': return { x: L + r * Math.cos(s.a), y: r * Math.sin(s.a), nx: Math.cos(s.a), ny: Math.sin(s.a) };
      default: return { x: -L + r * Math.cos(s.a), y: r * Math.sin(s.a), nx: Math.cos(s.a), ny: Math.sin(s.a) };
    }
  };
  const uDe = (s, r) => {
    switch (s.k) {
      case 'A': return s.a;
      case 'R': return L + r * (s.a + PI / 2);
      case 'C': return L + PI * r + (L - s.a);
      case 'E': return 3 * L + PI * r + r * (s.a - PI / 2);
      default: return 3 * L + 2 * PI * r + (s.a + L);
    }
  };
  return {
    n: am.length,
    perimetro: (z) => 4 * L + 2 * PI * raio(z),
    ponto(i, z, extra = 0) { const r = raio(z) + extra, s = am[i], p = ponto(s, r); p.u = uDe(s, raio(z)); return p; },
    noU(u, z) {
      const r = raio(z), P = 4 * L + 2 * PI * r, w = ((u % P) + P) % P;
      if (w < L) return ponto({ k: 'A', a: w }, r);
      if (w < L + PI * r) return ponto({ k: 'R', a: -PI / 2 + (w - L) / r }, r);
      if (w < 3 * L + PI * r) return ponto({ k: 'C', a: L - (w - L - PI * r) }, r);
      if (w < 3 * L + 2 * PI * r) return ponto({ k: 'E', a: PI / 2 + (w - 3 * L - PI * r) / r }, r);
      return ponto({ k: 'F', a: -L + (w - 3 * L - 2 * PI * r) }, r);
    },
  };
}

// Polígono qualquer (anti-horário, começando na frente) que cresce por escala com a altura: a tigela em formato.
function contornoEscalado(base, escala) {
  const n = base.length, N = [], C = [0];
  for (let i = 0; i < n; i++) {
    const a = base[(i - 1 + n) % n], b = base[i], c = base[(i + 1) % n];
    let nx = (b[1] - a[1]) + (c[1] - b[1]), ny = -(b[0] - a[0]) - (c[0] - b[0]);
    const l = Math.hypot(nx, ny) || 1; nx /= l; ny /= l; N.push([nx, ny]);
    if (i > 0) C.push(C[i - 1] + Math.hypot(b[0] - a[0], b[1] - a[1]));
  }
  const P0 = C[n - 1] + Math.hypot(base[0][0] - base[n - 1][0], base[0][1] - base[n - 1][1]);
  return {
    n,
    perimetro: (z) => P0 * escala(z),
    ponto(i, z, extra = 0) { const s = escala(z); return { x: base[i][0] * s + N[i][0] * extra, y: base[i][1] * s + N[i][1] * extra, nx: N[i][0], ny: N[i][1], u: C[i] * s }; },
    noU(u, z) {
      const s = escala(z), w = (((u / s) % P0) + P0) % P0;
      let lo = 0, hi = n - 1;
      while (lo < hi) { const md = (lo + hi + 1) >> 1; if (C[md] <= w) lo = md; else hi = md - 1; }
      const i = lo, j = (i + 1) % n, seg = (j === 0 ? P0 : C[j]) - C[i], t = seg > 0 ? (w - C[i]) / seg : 0;
      let nx = N[i][0] * (1 - t) + N[j][0] * t, ny = N[i][1] * (1 - t) + N[j][1] * t;
      const l = Math.hypot(nx, ny) || 1; nx /= l; ny /= l;
      return { x: (base[i][0] * (1 - t) + base[j][0] * t) * s, y: (base[i][1] * (1 - t) + base[j][1] * t) * s, nx, ny };
    },
  };
}

// Reamostra um laço com passo fixo e começa no ponto da frente (menor y perto do meio).
function reamostrar(laco, passo) {
  const n = laco.length, C = [0];
  for (let i = 1; i <= n; i++) C.push(C[i - 1] + Math.hypot(laco[i % n][0] - laco[i - 1][0], laco[i % n][1] - laco[i - 1][1]));
  const P = C[n], q = Math.max(48, Math.round(P / passo)), saida = [];
  let k = 0;
  for (let s = 0; s < q; s++) {
    const w = P * s / q;
    while (C[k + 1] < w) k++;
    const t = (w - C[k]) / ((C[k + 1] - C[k]) || 1), a = laco[k], b = laco[(k + 1) % n];
    saida.push([a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t]);
  }
  let frente = 0, melhor = Infinity;
  const larg = Math.max(...saida.map(p => Math.abs(p[0])));
  saida.forEach((p, i) => { const nota = p[1] + Math.abs(p[0]) / larg * 4; if (nota < melhor) { melhor = nota; frente = i; } });
  return [...saida.slice(frente), ...saida.slice(0, frente)];
}

// ---------- texturas da parede (sempre para fora: a parede nunca fica mais fina) ----------

// zonas: retângulos {u0, u1, z0, z1} onde a parede fica lisa (nome, frase, texto de trás, ícones).
export function fazerTextura(t, perimetroDe, zonas, zMin, zMax) {
  if (!t || t.tipo === 'liso' || t.tipo === 'vazados' || !(t.amp > 0)) return () => 0;
  const k = Math.max(3, Math.round(t.qtd)), A = t.amp;
  const padrao = {
    retos: (s) => 0.5 + 0.5 * Math.cos(2 * PI * k * s),
    diagonais: (s, z, P) => 0.5 + 0.5 * Math.cos(2 * PI * (k * s + z / (P / k) * 0.8)),
    ondas: (s, z) => 0.5 + 0.5 * Math.cos(2 * PI * z / Math.max(6, A * 3.4) + 1.6 * Math.sin(2 * PI * Math.max(2, Math.round(k / 6)) * s)),
    trancado: (s, z) => { const a = Math.cos(2 * PI * k * s), b = Math.cos(2 * PI * z / Math.max(7, A * 4)); return 0.5 + 0.5 * a * b; },
  }[t.tipo] || (() => 0);
  return (u, z) => {
    const P = perimetroDe(z), s = (((u % P) + P) % P) / P;
    let peso = suaveDegrau((z - zMin) / 4) * suaveDegrau((zMax - z) / 4);
    for (const zn of zonas) peso *= suaveDegrau(distanciaZona(zn, u, z, P) / 3);
    return A * peso * padrao(s, z, P);
  };
}

// Distância (mm) de um ponto da parede (u ao redor, z) até a zona, dando a volta na parede.
export function distanciaZona(zn, u, z, P) {
  const meio = (zn.u0 + zn.u1) / 2, metade = (zn.u1 - zn.u0) / 2;
  let d = (((u - meio) % P) + P) % P;
  if (d > P / 2) d -= P;
  return Math.hypot(Math.max(Math.abs(d) - metade, 0), Math.max(zn.z0 - z, z - zn.z1, 0));
}

// ---------- texto e ícones enrolados na parede ----------

// Regiões planas (u, v em mm) → prisma fino: bordas com pontos a cada `passo` e faces subdivididas até nenhum lado
// passar de 1,5 × `passo`, para a peça dobrar na parede curva sem afundar entre os pontos.
export function prismaFino(regioes, w0, w1, passo = 1.4) {
  const m = new Malha();
  for (const r of regioes) {
    const aneis = [enrolar(densificar(r.externo, passo), true), ...r.buracos.map(b => enrolar(densificar(b, passo), false))];
    const plano = [], buracos = [];
    aneis.forEach((a, i) => { if (i) buracos.push(plano.length / 2); for (const p of a) plano.push(p[0], p[1]); });
    const nBorda = plano.length / 2, brutos = earcut(plano, buracos.length ? buracos : null);
    if (!brutos.length) continue;
    // o earcut devolve tudo no mesmo sentido (inclusive as lascas quase sem área): decide uma vez, pela soma
    const tris = [], virar = areaTotal(plano, brutos) < 0;
    for (let k = 0; k < brutos.length; k += 3) tris.push(virar ? [brutos[k], brutos[k + 2], brutos[k + 1]] : [brutos[k], brutos[k + 1], brutos[k + 2]]);
    const finos = subdividir(plano, tris, passo * 1.5), n = plano.length / 2;
    const topo = m.pos.length / 3;
    for (let i = 0; i < n; i++) m.pos.push(plano[2 * i], plano[2 * i + 1], w1);
    const base = m.pos.length / 3;
    for (let i = 0; i < n; i++) m.pos.push(plano[2 * i], plano[2 * i + 1], w0);
    for (const [a, b, c] of finos) { m.t(topo + a, topo + b, topo + c); m.t(base + a, base + c, base + b); }
    let off = 0;
    for (const anel of aneis) {
      const q = anel.length;
      for (let i = 0; i < q; i++) { const a = off + i, b = off + (i + 1) % q; m.t(topo + a, base + a, base + b); m.t(topo + a, base + b, topo + b); }
      off += q;
    }
    if (off !== nBorda) throw new Error('borda do texto inconsistente');
  }
  return m;
}

// Divide pelo meio todo lado maior que `maximo` (o mesmo ponto novo serve aos dois triângulos do lado, então a malha
// continua fechada). As bordas já vêm curtas e não mudam. `plano` ganha os pontos novos.
function subdividir(plano, tris, maximo) {
  const lado = (a, b) => Math.hypot(plano[2 * a] - plano[2 * b], plano[2 * a + 1] - plano[2 * b + 1]);
  for (let volta = 0; volta < 8; volta++) {
    const meios = new Map(), chave = (a, b) => (a < b ? a * 1e7 + b : b * 1e7 + a);
    for (const t of tris) for (let k = 0; k < 3; k++) {
      const a = t[k], b = t[(k + 1) % 3];
      if (lado(a, b) > maximo && !meios.has(chave(a, b))) { meios.set(chave(a, b), plano.length / 2); plano.push((plano[2 * a] + plano[2 * b]) / 2, (plano[2 * a + 1] + plano[2 * b + 1]) / 2); }
    }
    if (!meios.size) break;
    const novos = [];
    for (const [a, b, c] of tris) {
      const ab = meios.get(chave(a, b)), bc = meios.get(chave(b, c)), ca = meios.get(chave(c, a));
      const q = (ab !== undefined) + (bc !== undefined) + (ca !== undefined);
      if (q === 0) novos.push([a, b, c]);
      else if (q === 3) novos.push([a, ab, ca], [ab, b, bc], [ca, bc, c], [ab, bc, ca]);
      else if (q === 1) {
        if (ab !== undefined) novos.push([a, ab, c], [ab, b, c]);
        else if (bc !== undefined) novos.push([a, b, bc], [a, bc, c]);
        else novos.push([a, b, ca], [ca, b, c]);
      } else if (ab === undefined) novos.push([a, b, bc], [a, bc, ca], [ca, bc, c]);
      else if (bc === undefined) novos.push([a, ab, ca], [ab, b, c], [ab, c, ca]);
      else novos.push([a, ab, c], [ab, b, bc], [ab, bc, c]);
    }
    tris = novos;
  }
  return tris;
}

function densificar(laco, passo) {
  const saida = [];
  for (let i = 0; i < laco.length; i++) {
    const a = laco[i], b = laco[(i + 1) % laco.length], q = Math.max(1, Math.ceil(Math.hypot(b[0] - a[0], b[1] - a[1]) / passo));
    for (let k = 0; k < q; k++) saida.push([a[0] + (b[0] - a[0]) * k / q, a[1] + (b[1] - a[1]) * k / q]);
  }
  return saida;
}
function enrolar(pts, antihorario) { return (areaAssinada(pts) > 0) === antihorario ? pts : [...pts].reverse(); }

// Dobra o prisma (u, v, w) na parede: u ao redor a partir de `u0`, v na altura a partir de `zc`, w para fora.
export function dobrarNaParede(prisma, contorno, u0, zc) {
  const P = prisma.pos;
  for (let i = 0; i < P.length; i += 3) {
    const z = zc + P[i + 1], q = contorno.noU(u0 + P[i], z), w = P[i + 2];
    P[i] = q.x + q.nx * w; P[i + 1] = q.y + q.ny * w; P[i + 2] = z;
  }
  return prisma;
}

// ---------- suporte para tigelas de aço ----------

export function perfilRaio(p) {
  const H = p.altura, A = p.perfilAmp || 0;
  const extra = {
    cone: () => 0, reto: () => 0,
    aneis: (z) => A * Math.abs(Math.sin(PI * Math.max(1, Math.round(p.aneis || 3)) * limitar(z / H, 0, 1))),
    barriga: (z) => A * Math.sin(PI * limitar(z / H, 0, 1)),
    cintura: (z) => -A * Math.sin(PI * limitar(z / H, 0, 1)),
  }[p.perfil] || (() => 0);
  return (R0, rT) => (z) => R0 + (rT - R0) * limitar(z / H, 0, 1.2) + extra(z);
}

export function medidasSuporte(p) {
  const rb = p.dTigela / 2 + p.folga, aba = Math.max(p.aba, p.parede + 2), rT = rb + aba;
  const n = p.qtd, passo = 2 * rb + p.vao, L = (n - 1) / 2 * passo;
  const R0 = Math.max(rT + (p.perfil === 'reto' ? 0 : 0.01), p.perfil === 'reto' ? rT : p.base / 2);
  const centros = Array.from({ length: n }, (_, k) => (k - (n - 1) / 2) * passo);
  return { rb, rT, R0, L, centros, aba };
}

// `zonasDe(contorno)` devolve onde a parede fica lisa (nome, textos, ícones); o texto em si é dobrado fora daqui.
export function montarSuporte(p, zonasDe) {
  const { rb, rT, R0, L, centros } = medidasSuporte(p);
  const H = p.altura, t = p.parede, tp = Math.max(p.tampo, 2.4), alfa = p.inclinacao * PI / 180, ta = Math.tan(alfa), ca = Math.cos(alfa), sa = Math.sin(alfa);
  const raioExt = perfilRaio(p)(R0, rT), raioInt = (z) => raioExt(z) - t;
  const ampMax = Math.max(0, p.perfilAmp || 0) + (p.textura && p.textura.tipo !== 'liso' ? p.textura.amp || 0 : 0);
  const texturado = p.textura && !['liso', 'vazados'].includes(p.textura.tipo) && p.textura.amp > 0;
  const passo = texturado ? 0.9 : 1.8;
  // mesma amostragem por fora e por dentro: o fundo aberto liga ponto a ponto
  const rRef = Math.max(R0, rT) + ampMax, ext = contornoEstadio(L, raioExt, rRef, passo), int = contornoEstadio(L, raioInt, rRef, passo);
  const zTopo = (y) => H + y * ta, zSob = (y) => H - tp / ca + y * ta;
  const zFundo = p.fundo === 'fechado' ? Math.max(1.2, p.espFundo) : 0;
  const zonas = zonasDe ? zonasDe(ext) : [];
  const tex = fazerTextura(p.textura, ext.perimetro, zonas, 0, H - tp);
  const m = new Malha(), n = ext.n;

  // altura do topo de cada coluna (o raio muda com a altura e o topo é inclinado)
  const topoCol = (cont, plano, i, extraDe) => { let z = H; for (let k = 0; k < 6; k++) z = plano(cont.ponto(i, z, extraDe ? extraDe(i, z) : 0).y); return z; };
  const linhas = (texturado || ['aneis', 'barriga', 'cintura'].includes(p.perfil)) ? Math.max(8, Math.ceil(H / (texturado ? 1.5 : 2.0))) : 2;
  const texCol = (i, z) => tex(ext.ponto(i, z).u, z);

  // parede externa
  const zTop = Array.from({ length: n }, (_, i) => topoCol(ext, zTopo, i, texCol));
  const aneisExt = [];
  for (let j = 0; j <= linhas; j++) {
    const pts = [];
    for (let i = 0; i < n; i++) { const z = zTop[i] * j / linhas, q = ext.ponto(i, z, texCol(i, z)); pts.push([q.x, q.y, z]); }
    aneisExt.push(anelDe(m, pts));
  }
  costurar(m, aneisExt, () => { const q = ext.ponto(0, 0); return [q.nx, q.ny, 0]; });

  // casca por dentro (até a face de baixo do tampo)
  const zSobCol = Array.from({ length: n }, (_, i) => topoCol(int, zSob, i));
  const linhasInt = ['aneis', 'barriga', 'cintura'].includes(p.perfil) ? Math.max(8, Math.ceil(H / 2)) : 2;
  const aneisInt = [];
  for (let j = 0; j <= linhasInt; j++) {
    const pts = [];
    for (let i = 0; i < n; i++) { const z = zFundo + (zSobCol[i] - zFundo) * j / linhasInt, q = int.ponto(i, z); pts.push([q.x, q.y, z]); }
    aneisInt.push(anelDe(m, pts));
  }
  costurar(m, aneisInt, () => { const q = int.ponto(0, zFundo); return [-q.nx, -q.ny, 0]; });

  // furos das tigelas no tampo (eixo perpendicular ao tampo)
  const e2 = [0, ca, sa], nrm = [0, -sa, ca], nf = Math.max(64, Math.ceil(2 * PI * rb / 1.2)), furosTopo = [], furosBaixo = [];
  for (const cx of centros) {
    const topo = [], baixo = [];
    for (let i = 0; i < nf; i++) {
      const a = i / nf * 2 * PI, x = cx + rb * Math.cos(a), y = rb * Math.sin(a) * e2[1], z = H + rb * Math.sin(a) * e2[2];
      topo.push([x, y, z]); baixo.push([x - tp * nrm[0], y - tp * nrm[1], z - tp * nrm[2]]);
    }
    const A = anelDe(m, baixo), B = anelDe(m, topo);
    costurar(m, [A, B], (i) => { const q = m.pos.slice(A[i] * 3, A[i] * 3 + 2); return [cx - q[0], -q[1], 0]; });
    furosTopo.push(B); furosBaixo.push(A);
  }
  const proj = (i) => { const x = m.pos[i * 3], y = m.pos[i * 3 + 1], z = m.pos[i * 3 + 2]; return [x, y * e2[1] + (z - H) * e2[2]]; };
  tampa(m, aneisExt[linhas], furosTopo, proj, true);
  tampa(m, aneisInt[linhasInt], furosBaixo, proj, false);

  // fundo
  const projZ = (i) => [m.pos[i * 3], m.pos[i * 3 + 1]];
  if (zFundo > 0) { tampa(m, aneisExt[0], [], projZ, false); tampa(m, aneisInt[0], [], projZ, true); } else {
    const A = aneisExt[0], B = aneisInt[0];
    for (let i = 0; i < n; i++) { const i2 = (i + 1) % n; m.t(A[i], B[i2], A[i2]); m.t(A[i], B[i], B[i2]); }
  }

  // pés de borracha: ressalto por dentro da casca (fundo aberto) e rebaixo para o pé
  const negativos = [], positivos = [];
  if (p.pes > 0) {
    const rp = p.pes / 2, rr = rp + 2.4, hb = zFundo > 0 ? 0 : 4, recuo = rr + 1.6 + (R0 - rT) * 5 / H;
    const lugares = [];
    for (const [cx, f] of [[L, -PI / 4], [L, PI / 4], [-L, 3 * PI / 4], [-L, 5 * PI / 4]]) lugares.push([cx + (R0 - recuo) * Math.cos(f), (R0 - recuo) * Math.sin(f)]);
    if (p.qtd === 3) lugares.push([0, -(R0 - recuo)], [0, R0 - recuo]);
    for (const [x, y] of lugares) {
      if (hb > 0) positivos.push(cilindroZ(x, y, 0, hb, rr, 48));
      negativos.push({ nome: 'Rebaixo do pé', malha: cilindroZ(x, y, -1, 1.6, rp + 0.25, 48) });
    }
  }

  // encaixes para os dedos (tirar a tigela): de lado com 1 tigela, na frente e atrás com 2 ou 3
  if (p.dedos) {
    const w = p.dedosLarg / 2, h = p.dedosAlt;
    centros.forEach((cx) => {
      const dirs = p.qtd === 1 ? [[1, 0], [-1, 0]] : [[0, -1], [0, 1]];
      for (const [dx, dy] of dirs) {
        // do furo até um pouco além da parede de fora (na fundura do encaixe a parede já abriu um pouco)
        const alcance = (p.qtd === 1 ? rT : rT) + (R0 - rT) * h / H + 4 - rb;
        const z = zTopo(dy * rb), c0 = [cx + dx * (rb - 3), dy * (rb - 3) * ca, z], c1 = [cx + dx * (rb + alcance), dy * (rb + alcance), z + dy * alcance * ta];
        // na prévia só a metade de baixo (o que some da peça), um pouco para dentro da parede
        const baixo = (c) => [c[0], c[1], c[2] - h / 2];
        negativos.push({ nome: 'Encaixe para os dedos', malha: tubo(c0, c1, [-dy, dx, 0], [0, 0, 1], w, h, 48), vista: tubo(baixo(c0), baixo(c1), [-dy, dx, 0], [0, 0, 1], w * 0.96, h / 2, 48) });
      }
    });
  }

  // vazados: janelas atravessando a casca, fora das zonas do texto
  if (p.textura && p.textura.tipo === 'vazados') {
    const k = Math.max(6, Math.round(p.textura.qtd)), zA = Math.max(zFundo + 8, H * 0.22), zB = H - tp - 8;
    if (zB - zA > 12) {
      const zc = (zA + zB) / 2, P = ext.perimetro(zc), larg = P / k * 0.42, alt = zB - zA;
      for (let i = 0; i < k; i++) {
        const u = P * (i + 0.5) / k;
        const folgaJanela = Math.hypot(larg / 2, alt / 2) + 2;
        if (zonas.some(zn => distanciaZona(zn, u, zc, P) < folgaJanela)) continue;
        const janela = [], r = larg / 2;
        for (let s = 0; s < 40; s++) { const a = s / 40 * 2 * PI, x = r * Math.cos(a), y = (alt / 2 - r) * Math.sign(Math.sin(a)) + r * Math.sin(a); janela.push([x, y]); }
        negativos.push({ nome: 'Vazado', malha: dobrarNaParede(prismaFino([{ externo: janela, buracos: [] }], -(t + 4), 4, 2), ext, u, zc) });
      }
    }
  }

  // tigelas de aço só para a prévia (borda apoiada no tampo, eixo perpendicular a ele)
  const fantasmas = centros.map(cx => ({ cx, alfa, H, rb, prof: p.profTigela }));
  return { corpo: m, positivos, negativos, contorno: ext, fantasmas, rb, rT, R0, L, H, frenteAltura: zTopo(-R0), fundoAltura: zFundo, zSob: (y) => zSob(y), tampo: tp };
}

// ---------- tigela impressa ----------

export function montarTigela(p) {
  const H = p.altura, t = p.parede, tf = Math.max(1.6, p.espFundo), flare = p.baseMais / 100;
  const largura = p.largura;
  const base = p.forma === 'redonda'
    ? reamostrar(Array.from({ length: 360 }, (_, i) => { const a = -PI / 2 + i / 360 * 2 * PI; return [largura / 2 * Math.cos(a), largura / 2 * Math.sin(a)]; }), 1.2)
    : reamostrar(contornoDe(p.forma, largura), 1.0);
  const escala = (z) => 1 + flare * (1 - limitar(z / H, 0, 1));
  const cont = contornoEscalado(base, escala);
  const texturado = p.textura && !['liso', 'vazados'].includes(p.textura.tipo) && p.textura.amp > 0;
  const tex = fazerTextura(p.textura, cont.perimetro, p.zonasDe ? p.zonasDe(cont) : [], 0, H);
  const m = new Malha(), n = cont.n;
  const linhas = texturado ? Math.max(8, Math.ceil(H / 1.5)) : 2;
  const aneis = [];
  for (let j = 0; j <= linhas; j++) {
    const z = H * j / linhas, pts = [];
    for (let i = 0; i < n; i++) { const q = cont.ponto(i, z, tex(cont.ponto(i, z).u, z)); pts.push([q.x, q.y, z]); }
    aneis.push(anelDe(m, pts));
  }
  costurar(m, aneis, () => { const q = cont.ponto(0, 0); return [q.nx, q.ny, 0]; });

  // cavidade: o maior círculo que cabe com a parede pedida; afina um pouco para baixo e faz curva no fundo
  const { cx, cy, raio } = circuloInscrito(base);
  const rTop = Math.max(8, raio - t), prof = H - tf, rBot = Math.max(6, rTop - prof * Math.tan(6 * PI / 180));
  const f = limitar(Math.min(14, prof * 0.35, rBot * 0.6), 1, 14), nc = Math.max(72, Math.ceil(2 * PI * rTop / 1.2));
  const perfilCav = [[rTop, H]];
  const passosParede = Math.max(2, Math.ceil((prof - f) / 6));
  for (let k = 1; k <= passosParede; k++) { const s = k / passosParede; perfilCav.push([rTop + (rBot - rTop) * s, H + (tf + f - H) * s]); }
  for (let k = 1; k <= 10; k++) { const a = k / 10 * PI / 2; perfilCav.push([rBot - f + f * Math.cos(a), tf + f - f * Math.sin(a)]); }
  const aneisCav = perfilCav.map(([r, z]) => anelDe(m, Array.from({ length: nc }, (_, i) => { const a = i / nc * 2 * PI; return [cx + r * Math.cos(a), cy + r * Math.sin(a), z]; })));
  const deBaixo = [...aneisCav].reverse();
  costurar(m, deBaixo, (i) => { const k = deBaixo[1][i] * 3; return [cx - m.pos[k], cy - m.pos[k + 1], 0.2]; });
  const meio = m.v(cx, cy, tf), fundoCav = aneisCav[aneisCav.length - 1];
  for (let i = 0; i < nc; i++) m.t(meio, fundoCav[i], fundoCav[(i + 1) % nc]);
  const projZ = (i) => [m.pos[i * 3], m.pos[i * 3 + 1]];
  tampa(m, aneis[linhas], [aneisCav[0]], projZ, true);
  tampa(m, aneis[0], [], projZ, false);

  const positivos = [], negativos = [];
  // comedouro lento
  const hLento = prof * limitar(p.lentoAlt / 100, 0.2, 0.85);
  if (p.lento === 'aletas') {
    const k = limitar(Math.round(p.lentoQtd), 3, 10), esp = 7, ri = Math.max(6, rBot * 0.2), re = rBot - 0.5;
    positivos.push(cilindroZ(cx, cy, tf - 0.5, tf + hLento, ri + 2, 48));
    for (let i = 0; i < k; i++) {
      const a = i / k * 2 * PI + PI / 2, ux = Math.cos(a), uy = Math.sin(a), vx = -uy, vy = ux, seg = [];
      for (let s = 0; s <= 12; s++) { const b = -PI / 2 + s / 12 * PI; seg.push([re + esp / 2 * Math.cos(b) - esp / 2, esp / 2 * Math.sin(b)]); }
      const forma = [[ri, -esp / 2], ...seg.map(([r, w]) => [r, w]), [ri, esp / 2]].map(([r, w]) => [cx + ux * r + vx * w, cy + uy * r + vy * w]);
      positivos.push(prismaFino([{ externo: forma, buracos: [] }], tf - 0.5, tf + hLento, 3));
    }
  } else if (p.lento === 'pinos') {
    const d = 13, passoP = d * 2.1, lim = rBot - d / 2 - 4;
    for (let y = -lim; y <= lim; y += passoP * 0.866) for (let x = -lim + ((Math.round((y + lim) / (passoP * 0.866)) % 2) * passoP / 2); x <= lim; x += passoP) {
      if (Math.hypot(x, y) <= lim) positivos.push(cilindroZ(cx + x, cy + y, tf - 0.5, tf + hLento, d / 2, 36));
    }
  }

  // pés de borracha: rebaixos por baixo, perto da beira
  if (p.pes > 0) {
    const rp = p.pes / 2, P = cont.perimetro(0);
    for (let k = 0; k < 4; k++) {
      const q = cont.noU(P * (k + 0.5) / 4, 0), rec = rp + 4.5;
      negativos.push({ nome: 'Rebaixo do pé', malha: cilindroZ(q.x - q.nx * rec, q.y - q.ny * rec, -1, 1.6, rp + 0.25, 48) });
    }
  }
  const volumeCav = volumeCavidade(rTop, rBot, f, tf, H) - (p.lento === 'aletas' ? 0.55 : p.lento === 'pinos' ? 0.25 : 0) * PI * rBot * rBot * hLento * 0.35;
  return { corpo: m, positivos, negativos, contorno: cont, cavidade: { cx, cy, rTop, rBot, tf, prof }, H, ml: volumeCav / 1000 };
}

function volumeCavidade(rTop, rBot, f, tf, H) {
  let v = 0;
  const zCheio = tf + (H - tf) * 0.9, dz = 0.25;
  for (let z = tf; z < zCheio; z += dz) {
    let r;
    if (z < tf + f) { const a = Math.asin(limitar((tf + f - z) / f, 0, 1)); r = rBot - f + f * Math.cos(a); } else r = rBot + (rTop - rBot) * (z - tf - f) / Math.max(1e-6, H - tf - f);
    v += PI * r * r * dz;
  }
  return v;
}

// Centro e raio do maior círculo dentro do polígono (busca em grade e refinamento).
export function circuloInscrito(poli) {
  const dist = (x, y) => {
    let d = Infinity;
    for (let i = 0; i < poli.length; i++) {
      const a = poli[i], b = poli[(i + 1) % poli.length], dx = b[0] - a[0], dy = b[1] - a[1], l2 = dx * dx + dy * dy || 1;
      const t = limitar(((x - a[0]) * dx + (y - a[1]) * dy) / l2, 0, 1);
      d = Math.min(d, Math.hypot(x - a[0] - dx * t, y - a[1] - dy * t));
    }
    return pontoDentro([x, y], poli) ? d : -d;
  };
  let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
  for (const [x, y] of poli) { x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
  let melhor = { cx: 0, cy: 0, raio: -Infinity }, passo = Math.max(x1 - x0, y1 - y0) / 24;
  for (let y = y0; y <= y1; y += passo) for (let x = x0; x <= x1; x += passo) { const d = dist(x, y); if (d > melhor.raio) melhor = { cx: x, cy: y, raio: d }; }
  for (let k = 0; k < 6; k++) {
    passo /= 3;
    const { cx, cy } = melhor;
    for (let dy = -3; dy <= 3; dy++) for (let dx = -3; dx <= 3; dx++) { const d = dist(cx + dx * passo, cy + dy * passo); if (d > melhor.raio) melhor = { cx: cx + dx * passo, cy: cy + dy * passo, raio: d }; }
  }
  return melhor;
}
