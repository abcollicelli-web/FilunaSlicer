/* Filuna — QR code em SVG, sem dependências (modo byte, correção M, versões 1 a 10).
   Usado na etiqueta da bobina do Filuna Slicer e do site. O mesmo arquivo vive nos dois projetos.
   Algoritmo conforme ISO/IEC 18004; a estrutura segue o gerador de referência de Nayuki (MIT). */
(function (raiz) {
  'use strict';
  // [ec por bloco, blocos do grupo 1, dados por bloco do grupo 1, blocos do grupo 2, dados por bloco do grupo 2]
  var TAB_M = [null, [10, 1, 16, 0, 0], [16, 1, 28, 0, 0], [26, 1, 44, 0, 0], [18, 2, 32, 0, 0], [24, 2, 43, 0, 0],
    [16, 4, 27, 0, 0], [18, 4, 31, 0, 0], [22, 2, 38, 2, 39], [22, 3, 36, 2, 37], [26, 4, 43, 1, 44]];
  var ALINHA = [null, [], [6, 18], [6, 22], [6, 26], [6, 30], [6, 34], [6, 22, 38], [6, 24, 42], [6, 26, 46], [6, 28, 50]];

  function mul(x, y) {
    var z = 0;
    for (var i = 7; i >= 0; i--) { z = (z << 1) ^ ((z >>> 7) * 0x11D); z ^= ((y >>> i) & 1) * x; }
    return z;
  }
  function divisor(grau) {
    var r = [], raizG = 1, i, j;
    for (i = 0; i < grau - 1; i++) { r.push(0); }
    r.push(1);
    for (i = 0; i < grau; i++) {
      for (j = 0; j < r.length; j++) { r[j] = mul(r[j], raizG); if (j + 1 < r.length) { r[j] ^= r[j + 1]; } }
      raizG = mul(raizG, 2);
    }
    return r;
  }
  function resto(dados, div) {
    var r = div.map(function () { return 0; });
    dados.forEach(function (b) {
      var f = b ^ r.shift();
      r.push(0);
      div.forEach(function (c, i) { r[i] ^= mul(c, f); });
    });
    return r;
  }
  function utf8(texto) {
    if (typeof TextEncoder !== 'undefined') { return Array.prototype.slice.call(new TextEncoder().encode(texto)); }
    return unescape(encodeURIComponent(texto)).split('').map(function (c) { return c.charCodeAt(0); });
  }

  function gerar(texto) {
    var bytes = utf8(texto), ver, t;
    for (ver = 1; ver <= 10; ver++) {
      t = TAB_M[ver];
      var cap = t[1] * t[2] + t[3] * t[4];
      if (4 + (ver < 10 ? 8 : 16) + bytes.length * 8 <= cap * 8) { break; }
    }
    if (ver > 10) { throw new Error('Texto longo demais para o QR.'); }
    var capDados = t[1] * t[2] + t[3] * t[4];
    // bits: modo byte, tamanho, dados, terminador, completa o byte, bytes de preenchimento
    var bits = [];
    function poe(v, n) { for (var i = n - 1; i >= 0; i--) { bits.push((v >>> i) & 1); } }
    poe(4, 4); poe(bytes.length, ver < 10 ? 8 : 16);
    bytes.forEach(function (b) { poe(b, 8); });
    poe(0, Math.min(4, capDados * 8 - bits.length));
    poe(0, (8 - bits.length % 8) % 8);
    var dados = [], i, j;
    for (i = 0; i < bits.length; i += 8) {
      var b = 0;
      for (j = 0; j < 8; j++) { b = (b << 1) | bits[i + j]; }
      dados.push(b);
    }
    for (i = 0; dados.length < capDados; i++) { dados.push(i % 2 ? 0x11 : 0xEC); }
    // blocos e correção
    var div = divisor(t[0]), blocos = [], k = 0;
    for (i = 0; i < t[1] + t[3]; i++) {
      var n = i < t[1] ? t[2] : t[4];
      var d = dados.slice(k, k + n);
      k += n;
      blocos.push({ d: d, e: resto(d, div) });
    }
    var final = [], maior = Math.max(t[2], t[4]);
    for (i = 0; i < maior; i++) { blocos.forEach(function (bl) { if (i < bl.d.length) { final.push(bl.d[i]); } }); }
    for (i = 0; i < t[0]; i++) { blocos.forEach(function (bl) { final.push(bl.e[i]); }); }

    var tam = ver * 4 + 17, mod = [], fun = [];
    for (i = 0; i < tam; i++) { mod.push(new Array(tam).fill(false)); fun.push(new Array(tam).fill(false)); }
    function f(x, y, c) { mod[y][x] = c; fun[y][x] = true; }
    for (i = 0; i < tam; i++) { f(6, i, i % 2 === 0); f(i, 6, i % 2 === 0); }
    [[3, 3], [tam - 4, 3], [3, tam - 4]].forEach(function (p) {
      for (var dy = -4; dy <= 4; dy++) {
        for (var dx = -4; dx <= 4; dx++) {
          var x = p[0] + dx, y = p[1] + dy, dist = Math.max(Math.abs(dx), Math.abs(dy));
          if (x >= 0 && x < tam && y >= 0 && y < tam) { f(x, y, dist !== 2 && dist !== 4); }
        }
      }
    });
    var al = ALINHA[ver], u = al.length - 1;
    al.forEach(function (ay, a) {
      al.forEach(function (ax, c) {
        if ((a === 0 && c === 0) || (a === 0 && c === u) || (a === u && c === 0)) { return; }
        for (var dy = -2; dy <= 2; dy++) {
          for (var dx = -2; dx <= 2; dx++) { f(ax + dx, ay + dy, Math.max(Math.abs(dx), Math.abs(dy)) !== 1); }
        }
      });
    });
    function formato(mascara) {
      var dd = (0 << 3) | mascara, r = dd, q;  // correção M = 00
      for (q = 0; q < 10; q++) { r = (r << 1) ^ ((r >>> 9) * 0x537); }
      var bb = ((dd << 10) | r) ^ 0x5412;
      function bit(n) { return ((bb >>> n) & 1) !== 0; }
      for (q = 0; q <= 5; q++) { f(8, q, bit(q)); }
      f(8, 7, bit(6)); f(8, 8, bit(7)); f(7, 8, bit(8));
      for (q = 9; q < 15; q++) { f(14 - q, 8, bit(q)); }
      for (q = 0; q < 8; q++) { f(tam - 1 - q, 8, bit(q)); }
      for (q = 8; q < 15; q++) { f(8, tam - 15 + q, bit(q)); }
      f(8, tam - 8, true);
    }
    formato(0);
    if (ver >= 7) {
      var rv = ver, q;
      for (q = 0; q < 12; q++) { rv = (rv << 1) ^ ((rv >>> 11) * 0x1F25); }
      var bv = (ver << 12) | rv;
      for (q = 0; q < 18; q++) {
        var cv = ((bv >>> q) & 1) !== 0, a2 = tam - 11 + q % 3, b2 = Math.floor(q / 3);
        f(a2, b2, cv); f(b2, a2, cv);
      }
    }
    // dados em zigue-zague
    var pos = 0;
    for (var dir = tam - 1; dir >= 1; dir -= 2) {
      if (dir === 6) { dir = 5; }
      for (var v = 0; v < tam; v++) {
        for (j = 0; j < 2; j++) {
          var x = dir - j, sobe = ((dir + 1) & 2) === 0, y = sobe ? tam - 1 - v : v;
          if (!fun[y][x] && pos < final.length * 8) {
            mod[y][x] = ((final[pos >>> 3] >>> (7 - (pos & 7))) & 1) !== 0;
            pos++;
          }
        }
      }
    }
    function mascarar(m) {
      for (var y = 0; y < tam; y++) {
        for (var x = 0; x < tam; x++) {
          if (fun[y][x]) { continue; }
          var inv;
          switch (m) {
            case 0: inv = (x + y) % 2 === 0; break;
            case 1: inv = y % 2 === 0; break;
            case 2: inv = x % 3 === 0; break;
            case 3: inv = (x + y) % 3 === 0; break;
            case 4: inv = (Math.floor(x / 3) + Math.floor(y / 2)) % 2 === 0; break;
            case 5: inv = (x * y) % 2 + (x * y) % 3 === 0; break;
            case 6: inv = ((x * y) % 2 + (x * y) % 3) % 2 === 0; break;
            default: inv = ((x + y) % 2 + (x * y) % 3) % 2 === 0;
          }
          if (inv) { mod[y][x] = !mod[y][x]; }
        }
      }
    }
    function penalidade() {
      var p = 0, x, y, run, escuros = 0;
      function linha(get) {
        var s = 0;
        for (var a = 0; a < tam; a++) {
          run = 1;
          for (var b = 1; b <= tam; b++) {
            if (b < tam && get(a, b) === get(a, b - 1)) { run++; } else { if (run >= 5) { s += run - 2; } run = 1; }
          }
          var seq = [];
          for (b = 0; b < tam; b++) { seq.push(get(a, b) ? 1 : 0); }
          var txt = '0000' + seq.join('') + '0000';
          var r1 = txt.split('10111010000').length - 1, r2 = txt.split('00001011101').length - 1;
          s += 40 * (r1 + r2);
        }
        return s;
      }
      p += linha(function (a, b) { return mod[a][b]; });
      p += linha(function (a, b) { return mod[b][a]; });
      for (y = 0; y < tam - 1; y++) {
        for (x = 0; x < tam - 1; x++) {
          var c0 = mod[y][x];
          if (c0 === mod[y][x + 1] && c0 === mod[y + 1][x] && c0 === mod[y + 1][x + 1]) { p += 3; }
        }
      }
      for (y = 0; y < tam; y++) { for (x = 0; x < tam; x++) { if (mod[y][x]) { escuros++; } } }
      var total = tam * tam;
      p += Math.max(0, Math.ceil(Math.abs(escuros * 20 - total * 10) / total) - 1) * 10;
      return p;
    }
    var melhor = 0, menor = Infinity;
    for (var m = 0; m < 8; m++) {
      mascarar(m); formato(m);
      var pen = penalidade();
      if (pen < menor) { menor = pen; melhor = m; }
      mascarar(m);
    }
    mascarar(melhor); formato(melhor);
    return mod;
  }

  // SVG com margem de 4 módulos (zona de silêncio). px = lado em pixels.
  function svg(texto, px) {
    var mod = gerar(texto), tam = mod.length, lado = tam + 8, d = '';
    for (var y = 0; y < tam; y++) {
      for (var x = 0; x < tam; x++) { if (mod[y][x]) { d += 'M' + (x + 4) + ' ' + (y + 4) + 'h1v1h-1z'; } }
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="' + (px || lado * 4) + '" height="' + (px || lado * 4) +
      '" viewBox="0 0 ' + lado + ' ' + lado + '" shape-rendering="crispEdges" role="img" aria-label="QR ' +
      String(texto).replace(/[<>&"]/g, '') + '"><rect width="' + lado + '" height="' + lado + '" fill="#fff" stroke="none"/><path d="' + d +
      '" fill="#000" stroke="none"/></svg>';
  }

  raiz.FilunaQR = { gerar: gerar, svg: svg };
  if (typeof module !== 'undefined' && module.exports) { module.exports = raiz.FilunaQR; }
})(typeof window !== 'undefined' ? window : globalThis);
