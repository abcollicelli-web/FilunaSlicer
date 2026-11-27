/* Filuna Farm no Slicer — planejador LOCAL. Porte fiel de site-v2/app/farm_plano.py (branch site-v2/farm-wizard):
 * mesmas regras, mesmos textos e mesmos números do site, conferidos com um teste que roda os dois lados com os
 * mesmos dados (lab: /srv/filuna-homolog/overlay/teste-farm-plano). Ao mudar o planejador do site, portar aqui.
 * Funções puras: recebem peças, respostas, impressoras, bobinas e o fatiamento; devolvem as mesas e as correções. */
(function (raiz) {
"use strict";
var PADRAO = { folga_mm: 5.0, borda_mm: 5.0, min_mesa_extra: 6.0, purga_g: { ams: 0.6, cabecas: 0.05 },
	troca_s: { ams: 75, cabecas: 12 }, ocupacao_mista: 0.72, altura_camada: 0.2 };
var CORES_NOME = [["Preto", "#1E1E1E"], ["Branco", "#FFFFFF"], ["Cinza", "#8E8E8E"], ["Vermelho", "#D62828"],
	["Laranja", "#F77F00"], ["Amarelo", "#F6C90E"], ["Verde", "#2A9D4B"], ["Azul", "#2563EB"],
	["Roxo", "#7B4BFF"], ["Rosa", "#FF5CA8"], ["Marrom", "#7A4A2A"], ["Bege", "#E3CDA4"]];

// ---- o que o Python faz de graça ----
function pyRound(x, n) {   // round() do Python: pelo valor binário exato; empate exato vai para o par
	n = n || 0;
	if (!isFinite(x)) return x;
	var neg = x < 0, t = Math.abs(x).toFixed(100), ponto = t.indexOf(".");
	var inteiro = t.slice(0, ponto), frac = t.slice(ponto + 1);
	var mant = inteiro + frac.slice(0, n), resto = frac.slice(n);
	var sobe;
	if (resto[0] > "5") sobe = true;
	else if (resto[0] < "5") sobe = false;
	else if (/[1-9]/.test(resto.slice(1))) sobe = true;
	else sobe = parseInt(mant[mant.length - 1], 10) % 2 === 1;   // empate exato: par
	var v = BigInt(mant) + (sobe ? 1n : 0n), sv = v.toString();
	if (n) { while (sv.length <= n) sv = "0" + sv; sv = sv.slice(0, sv.length - n) + "." + sv.slice(sv.length - n); }
	var r = parseFloat(sv);
	return neg && r !== 0 ? -r : r;
}
function soma(l, f) {   // sum() do Python 3.12+: soma compensada (Neumaier), como o CPython faz com números decimais
	var s = 0, c = 0;
	for (var i = 0; i < l.length; i++) {
		var x = f ? f(l[i]) : l[i], t = s + x;
		if (Math.abs(s) >= Math.abs(x)) c += (s - t) + x; else c += (x - t) + s;
		s = t;
	}
	return (c && isFinite(c)) ? s + c : s;
}
function maxPor(l, chave, padrao) {   // max(l, key=...): o primeiro maior
	if (!l.length) return padrao;
	var m = l[0], km = chave(l[0]);
	for (var i = 1; i < l.length; i++) { var k = chave(l[i]); if (cmp(k, km) > 0) { m = l[i]; km = k; } }
	return m;
}
function minPor(l, chave, padrao) {
	if (!l.length) return padrao;
	var m = l[0], km = chave(l[0]);
	for (var i = 1; i < l.length; i++) { var k = chave(l[i]); if (cmp(k, km) < 0) { m = l[i]; km = k; } }
	return m;
}
function cmp(a, b) {   // compara números, booleanos e tuplas como o Python
	if (Array.isArray(a)) {
		for (var i = 0; i < Math.min(a.length, b.length); i++) { var c = cmp(a[i], b[i]); if (c) return c; }
		return a.length - b.length;
	}
	a = +a; b = +b;
	return a < b ? -1 : a > b ? 1 : 0;
}
function ordenar(l, chave) {   // sorted(l, key=...), estável
	return l.map(function (x, i) { return [chave(x), i, x]; }).sort(function (p, q) { return cmp(p[0], q[0]) || p[1] - q[1]; }).map(function (p) { return p[2]; });
}
function ou0(v) { return v || 0; }

// ---- cores e textos ----
function _rgb(h) {
	h = (h || "#000000").replace(/^#+/, "");
	var r = parseInt(h.slice(0, 2), 16), g = parseInt(h.slice(2, 4), 16), b = parseInt(h.slice(4, 6), 16);
	if (isNaN(r) || isNaN(g) || isNaN(b)) return [0, 0, 0];
	return [r, g, b];
}
function distancia_cor(a, b) { var x = _rgb(a), y = _rgb(b); return Math.sqrt(Math.pow(x[0] - y[0], 2) + Math.pow(x[1] - y[1], 2) + Math.pow(x[2] - y[2], 2)); }
function cor_perto(a, b) { return !!(a && b) && distancia_cor(a, b) <= 60; }
function nome_cor(h) { if (!h) return "Sem cor"; return minPor(CORES_NOME, function (c) { return distancia_cor(c[1], h); })[0]; }
function duracao(seg) {
	seg = Math.trunc(seg || 0);
	var h = Math.floor(seg / 3600), m = pyRound((seg % 3600) / 60);
	if (m === 60) { h += 1; m = 0; }
	return h ? h + " h " + (m < 10 ? "0" : "") + m : m + " min";
}
function gramas(g) { return String(pyRound(g)).replace(/\B(?=(\d{3})+(?!\d))/g, " ") + " g"; }
// moeda da conta vinda da ponte (Filuna.moeda); sem ponte ou sem moeda = real, igual ao site
function emDolar() { return !!(raiz.Filuna && raiz.Filuna.moeda && raiz.Filuna.moeda() == "USD"); }
function reais(v) {
	var t = v.toFixed(2).split(".");
	if (emDolar()) return "$" + t[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",") + "." + t[1];
	var i = t[0].replace(/\B(?=(\d{3})+(?!\d))/g, ".");
	return "R$ " + i + "," + t[1];
}
function _cm(mm) {
	var v = (mm || 0) / 10, t = v < 1 ? v.toFixed(2) : v.toFixed(1);
	t = t.replace(/0+$/, "").replace(/\.$/, "").replace(".", ",");
	return t || "0";
}

function cabem(px, py, pz, mesa, p) {
	if (pz > mesa.h) return [0, 0];
	var g = p.folga_mm, w = mesa.w - 2 * p.borda_mm, d = mesa.d - 2 * p.borda_mm;
	function grade(a, b) { if (a > w || b > d) return 0; return Math.floor((w + g) / (a + g)) * Math.floor((d + g) / (b + g)); }
	var como_veio = grade(px, py);
	return [Math.max(como_veio, grade(py, px)), como_veio];
}
function _mesa(imp) { return { w: imp.mesa_w, d: imp.mesa_d, h: imp.mesa_h }; }

function _unidades(pecas, fatias) {
	var por_arquivo = {}, saida = {};
	pecas.forEach(function (pc) { por_arquivo[pc.arquivo] = (por_arquivo[pc.arquivo] || 0) + pc.volume_mm3 * pc.copias; });
	pecas.forEach(function (pc) {
		var f = fatias[pc.arquivo], total = por_arquivo[pc.arquivo];
		if (!f || total <= 0) { saida[pc.id] = null; return; }
		var parte = pc.volume_mm3 / total;
		saida[pc.id] = { tempo_s: f.tempo_s * parte, gramas: f.gramas * parte, altura_camada: f.altura_camada || 0 };
	});
	return saida;
}
function material_da_cor(r, cor) {
	var m = r.materiais || {};
	for (var hx in m) if (m[hx] && cor_perto(hx, cor)) return m[hx];
	return r.material || "PLA";
}
function _slots(impressoras, material, cor) {
	var saida = [];
	impressoras.forEach(function (imp) {
		for (var i = 0; i < imp.slots.length; i++) {
			var s = imp.slots[i];
			if (s.material === material && cor_perto(s.cor_hex, cor)) { saida.push([imp, s]); break; }
		}
	});
	return saida;
}
function _c(tipo, titulo, precisa_agir, porque, como, resultado, acao, resolvido, rotulo) {
	return { tipo: tipo, titulo: titulo, precisa_agir: precisa_agir, porque: porque, como: como, resultado: resultado,
		acao: acao === undefined ? null : acao, resolvido: !!resolvido,
		rotulo: rotulo || (precisa_agir ? "você precisa agir" : "já corrigido pelo sistema") };
}
function _mesas_iguais(pecas, conjuntos, impressoras, p, uma_por_mesa) {
	var total = 0;
	pecas.forEach(function (pc) {
		var n = impressoras.length ? Math.max.apply(null, impressoras.map(function (i) { return cabem(pc.x, pc.y, pc.z, _mesa(i), p)[0]; })) : 0;
		var qtd = pc.copias * conjuntos;
		total += (uma_por_mesa || n < 1) ? qtd : Math.ceil(qtd / n);
	});
	return total;
}

function planejar(pecas, r, impressoras, bobinas, fatias, params, precos_kg, densidades) {
	var p = Object.assign({}, PADRAO, params || {});
	var material = r.material;
	var conjuntos = Math.max(1, parseInt(r.conjuntos || 1, 10) || 1);
	var un = _unidades(pecas, fatias);
	var medido = pecas.every(function (pc) { return un[pc.id]; });
	var bob = {}; bobinas.forEach(function (b) { bob[b.id] = b; });
	var preco_kg = function (b_id, mat) {
		if (mat === undefined) mat = material;
		return (b_id in bob && bob[b_id].preco_kg) ? bob[b_id].preco_kg : ((precos_kg || {})[mat] !== undefined ? precos_kg[mat] : (emDolar() ? 25.0 : 100.0));
	};
	var dens = densidades || {}, base = r._base || material, fator = {};
	var mats = {}; for (var hx in (r.materiais || {})) mats[r.materiais[hx]] = 1; mats[material] = 1;
	for (var m in mats) fator[m] = (dens[m] && dens[base]) ? dens[m] / dens[base] : 1.0;
	r = Object.assign({}, r, { _fator: fator });
	var correcoes = [];
	var multicor = pecas.filter(function (pc) { return pc.cores.length > 1; });
	var simples = pecas.filter(function (pc) { return pc.cores.length <= 1; });
	var cores_pedido = [];
	pecas.forEach(function (pc) {
		(pc.cores.length ? pc.cores : [pc.cor]).forEach(function (c) {
			if (c && !cores_pedido.some(function (x) { return cor_perto(c, x); })) cores_pedido.push(c);
		});
	});
	var sep = _separadas(simples, multicor, material, conjuntos, r, impressoras, bob, un, p, correcoes, preco_kg);
	var jun = _juntas(pecas, cores_pedido, material, conjuntos, impressoras, bob, un, p, preco_kg);
	var usar_juntas = r.modo_cores === "juntas" && cores_pedido.length > 1;
	var escolhido = usar_juntas ? jun : sep;
	if (usar_juntas) correcoes = jun.correcoes.slice();
	if (!impressoras.length)
		correcoes.unshift(_c("sem_impressora", "Nenhuma impressora cadastrada", true,
			"a Farm precisa saber quais impressoras você tem e qual cor está em cada uma.",
			"cadastre suas impressoras, ou conecte o Filuna Slicer para ele informar sozinho.",
			"o pedido passa a ser dividido automaticamente entre elas.",
			{ tipo: "link", href: "/farm/impressoras", rotulo: "Cadastrar impressoras" }));
	if (!medido)
		correcoes.push(_c("sem_medida", "Tempo e filamento ainda não medidos", false,
			"o fatiamento deste arquivo não terminou ou não foi possível no servidor.",
			"o Filuna Slicer informa o tempo e o filamento reais ao fatiar cada mesa.",
			"a fila funciona, mas o prazo e a conferência de filamento só aparecem depois da medida."));
	var ids_simples = {}; simples.forEach(function (pc) { ids_simples[pc.id] = 1; });
	if (!usar_juntas && Object.keys(ids_simples).length > 1 && cores_pedido.length > 1) {
		var lista = pecas.map(function (pc) { return pc.copias + " " + pc.nome; }).join(" + ");
		correcoes.push(_c("montagem", "Montagem depois da impressão", false,
			"cada cor sai de uma impressora diferente, então as peças saem soltas.",
			"junte " + lista + " em cada conjunto.",
			conjuntos + " conjunto" + (conjuntos > 1 ? "s" : "") + " sem nenhuma purga de troca de cor.",
			null, true, "para você saber"));
	}
	var comparacoes = {};
	if (cores_pedido.length > 1)
		comparacoes.cores = {
			separadas: { purga_g: 0.0, prejuizo: 0.0, mesas: sep.mesas.length, prazo_s: sep.prazo_s, possivel: true },
			juntas: { purga_g: jun.purga_g, prejuizo: jun.prejuizo, mesas: jun.mesas.length, prazo_s: jun.prazo_s, possivel: jun.possivel,
				trocas: jun.trocas, pct: jun.gramas ? pyRound(100 * jun.purga_g / jun.gramas) : 0, impressoras: jun.n_impressoras }
		};
	var total_pecas = soma(pecas, function (pc) { return pc.copias; }) * conjuntos;
	var mesas_cheias = _mesas_iguais(simples, conjuntos, impressoras, p, false);
	var n_simples = soma(simples, function (pc) { return pc.copias; }) * conjuntos;
	comparacoes.iguais = { juntas: { mesas: mesas_cheias }, uma: { mesas: n_simples, extra_s: Math.max(0, n_simples - mesas_cheias) * p.min_mesa_extra * 60 } };
	var sugestoes = usar_juntas ? [] : _sugestoes(sep, simples, material, conjuntos, r, impressoras, bob, un, p);
	var custo = soma(escolhido.mesas, function (m) { return m.gramas / 1000 * preco_kg(m.bobina_id, m.material); });
	custo += escolhido.prejuizo || 0.0;
	return {
		modo: usar_juntas ? "juntas" : "separadas", medido: medido, mesas: escolhido.mesas, grupos: escolhido.grupos,
		correcoes: correcoes, comparacoes: comparacoes, sugestoes: sugestoes,
		resumo: { pecas: total_pecas, mesas: escolhido.mesas.length, gramas: escolhido.gramas, purga_g: escolhido.purga_g || 0.0,
			sequencial_s: escolhido.sequencial_s, prazo_s: escolhido.prazo_s, impressoras: escolhido.n_impressoras,
			custo: pyRound(custo, 2), pendentes: correcoes.filter(function (c) { return c.precisa_agir && !c.resolvido; }).length }
	};
}

function _separadas(simples, multicor, material, conjuntos, r, impressoras, bob, un, p, correcoes, preco_kg) {
	var uma = r.iguais === "uma_por_mesa", prioridade = r.prioridade || "prazo";
	var carga = {}, inicio = {}, reservado = {}, mesas = [], grupos = [];
	impressoras.forEach(function (i) { carga[i.id] = parseFloat(i.carga_s || 0) || 0; inicio[i.id] = carga[i.id]; });
	var por_cor = [];   // [[chave, [peças]]] na ordem do Python (dict)
	simples.forEach(function (pc) {
		var cor = (pc.cores.length ? pc.cores[0] : pc.cor) || "";
		var ach = null;
		for (var k = 0; k < por_cor.length; k++) if (cor_perto(por_cor[k][0], cor)) { ach = por_cor[k]; break; }
		if (ach) ach[1].push(pc); else {
			var mesma = por_cor.filter(function (e) { return e[0] === cor; })[0];
			if (mesma) mesma[1].push(pc); else por_cor.push([cor, [pc]]);
		}
	});
	var padrao = material;
	por_cor.forEach(function (ent) {
		var cor = ent[0], lista = ent[1];
		material = cor ? material_da_cor(r, cor) : padrao;
		var fator = (r._fator || {})[material]; if (fator === undefined) fator = 1.0;
		var cand = cor ? _slots(impressoras, material, cor) : [];
		var nome = null;
		for (var q0 = 0; q0 < cand.length; q0++) if (cand[q0][1].cor_nome) { nome = cand[q0][1].cor_nome; break; }
		nome = nome || nome_cor(cor);
		var g = { cor: cor, cor_nome: nome, material: material, pecas: lista.map(function (pc) { return pc.nome; }), mesas: [],
			impressoras: [], tempo_s: 0.0, gramas: 0.0 };
		lista.forEach(function (pc) {
			var total = pc.copias * conjuntos, u = un[pc.id], cabe = {};
			impressoras.forEach(function (imp) { cabe[imp.id] = cabem(pc.x, pc.y, pc.z, _mesa(imp), p); });
			var aptos = cand.filter(function (t) { return cabe[t[0].id][0] >= 1; });
			if (aptos.length && prioridade !== "prazo") {
				if (prioridade === "filamento") aptos = [maxPor(aptos, function (t) { return ou0((bob[t[1].bobina_id] || {}).restante_g); })];
				else {
					var usadas0 = {}; mesas.forEach(function (m) { usadas0[m.impressora_id] = 1; });
					aptos = [minPor(aptos, function (t) { return [!(t[0].id in usadas0), carga[t[0].id]]; })];
				}
			}
			var vals = impressoras.map(function (i) { return cabe[i.id][0]; });
			var maior = vals.length ? Math.max.apply(null, vals) : 0;
			if (maior < 1)
				correcoes.push(_c("nao_cabe", pc.nome + " não cabe em nenhuma mesa", true,
					"a peça mede " + _cm(pc.x) + " × " + _cm(pc.y) + " × " + _cm(pc.z) + " cm, maior que as suas mesas.",
					"divida a peça em partes no Filuna Slicer, ou cadastre uma impressora maior.",
					"a peça entra na fila assim que couber em alguma mesa."));
			var restante = total, numero = 0;
			while (restante > 0) {
				var imp = null, slot = null, n;
				if (aptos.length) {
					var esc = minPor(aptos, function (t) { return carga[t[0].id]; });
					imp = esc[0]; slot = esc[1]; n = uma ? 1 : cabe[imp.id][0];
				} else n = uma ? 1 : Math.max(maior, 1);
				var q = Math.min(n, restante);
				restante -= q; numero += 1;
				var t = u ? u.tempo_s * q : 0.0, gr = u ? u.gramas * q * fator : 0.0;
				var bloqueio = imp ? null : (maior < 1 ? "nao_cabe" : "falta_cor");
				var bid = slot ? slot.bobina_id : null;
				if (imp && u && bid in bob) {
					var livre = ou0(bob[bid].restante_g) - ou0(bob[bid].reservado_g) - ou0(reservado[bid]);
					if (gr > livre + 1e-6) bloqueio = "filamento";
					reservado[bid] = ou0(reservado[bid]) + gr;
				}
				var m = { peca_id: pc.id, peca: pc.nome, arquivo: pc.arquivo, quantidade: q, numero: numero,
					impressora_id: imp ? imp.id : null, impressora: imp ? imp.nome : null, posicao: slot ? slot.posicao : null,
					bobina_id: bid === undefined ? null : bid, material: material, cor: cor, cor_nome: nome,
					tempo_s: pyRound(t), gramas: pyRound(gr, 1), bloqueio: bloqueio, cabem: n };
				if (imp) {
					carga[imp.id] += t + p.min_mesa_extra * 60;
					if (g.impressoras.indexOf(imp.nome) < 0) g.impressoras.push(imp.nome);
				}
				mesas.push(m); g.mesas.push(m); g.tempo_s += t; g.gramas += gr;
			}
			g.mesas.forEach(function (m) { if (m.peca_id === pc.id) m.total = numero; });
			var cvs = impressoras.map(function (i) { return cabe[i.id]; });
			var mv = maxPor(cvs, function (c) { return c[0]; }, [0, 0]), melhor = mv[0], veio = mv[1];
			if (!uma && melhor > veio && veio >= 1 && Math.ceil(total / veio) > Math.ceil(total / melhor)) {
				var menos = Math.ceil(total / veio) - Math.ceil(total / melhor);
				correcoes.push(_c("reorganizado", pc.nome + ": peças giradas para caber mais", false,
					"do jeito que o arquivo veio cabiam " + veio + " por mesa.",
					"o sistema girou as peças; nada para você fazer.",
					melhor + " por mesa: " + menos + " mesa" + (menos > 1 ? "s" : "") + " a menos" +
					(menos ? " e cerca de " + duracao(menos * p.min_mesa_extra * 60) + " parados a menos." : "."), null, true));
			}
		});
		if (cor && !cand.length) {
			var evitar = {};
			por_cor.forEach(function (e2) {
				var c2 = e2[0];
				if (c2 && c2 !== cor) _slots(impressoras, material_da_cor(r, c2), c2).forEach(function (t) { evitar[t[0].id] = 1; });
			});
			var fc = _falta_cor(cor, nome, material, lista, impressoras, bob, g, evitar, (r.troca || {})[cor.toUpperCase()],
				por_cor.map(function (e) { return e[0]; }));
			fc.cor = cor;
			correcoes.push(fc);
		}
		_confere_filamento(g, bob, correcoes);
		grupos.push(g);
	});
	multicor.forEach(function (pc) {
		correcoes.push(_c("peca_multicor", pc.nome + " tem " + pc.cores.length + " cores na mesma peça", true,
			"peça pintada ou com partes coladas de cores diferentes não pode ser separada por impressora.",
			"responda “Todas as cores na mesma mesa” e use uma impressora que troca de cor.",
			"a peça sai pronta, com a purga de troca de cor mostrada antes de imprimir."));
	});
	var usadas = []; mesas.forEach(function (m) { if (m.impressora_id && usadas.indexOf(m.impressora_id) < 0) usadas.push(m.impressora_id); });
	return { mesas: mesas, grupos: grupos, gramas: pyRound(soma(mesas, function (m) { return m.gramas; }), 1),
		sequencial_s: pyRound(soma(mesas, function (m) { return m.tempo_s; })),
		prazo_s: pyRound(usadas.length ? Math.max.apply(null, usadas.map(function (i) { return carga[i] - inicio[i]; })) : 0),
		n_impressoras: usadas.length };
}

function posicao_livre(imp, cores_do_pedido) {
	cores_do_pedido = cores_do_pedido || [];
	for (var i = 0; i < imp.slots.length; i++) {
		var s = imp.slots[i];
		if (!s.cor_hex || !cores_do_pedido.some(function (c) { return c && cor_perto(s.cor_hex, c); })) return s;
	}
	return imp.slots[0];
}

function _falta_cor(cor, nome, material, lista, impressoras, bob, grupo, evitar, preferida, cores_do_pedido) {
	evitar = evitar || {};
	var carregadas = {};
	impressoras.forEach(function (i) { i.slots.forEach(function (s) { if (s.bobina_id) carregadas[s.bobina_id] = 1; }); });
	var em_estoque = ordenar(Object.keys(bob).map(function (k) { return bob[k]; }).filter(function (b) {
		return b.material === material && cor_perto(b.cor_hex, cor) && ou0(b.restante_g) > 0 && !(b.id in carregadas);
	}), function (b) { return -ou0(b.restante_g); });
	var pecas = lista.map(function (pc) { return pc.nome; }).join(", ");
	var porque = pecas + " precisa de " + material + " " + nome + " e nenhuma impressora está com essa cor. Sem a cor, não entra na fila.";
	var com_slot = impressoras.filter(function (i) { return i.slots.length && !(i.id in evitar); });
	if (!com_slot.length) com_slot = impressoras.filter(function (i) { return i.slots.length; });
	var livres = com_slot.filter(function (i) { return i.estado === "livre"; });
	if (!livres.length) livres = com_slot;
	var escolhida = impressoras.filter(function (i) { return i.id === preferida && i.slots.length; })[0];
	if (escolhida) livres = [escolhida];
	if (em_estoque.length && livres.length) {
		var b = em_estoque[0], imp = livres[0], pos = posicao_livre(imp, cores_do_pedido);
		return _c("falta_cor", "Falta cor na impressora: " + material + " " + nome, true, porque,
			"coloque a bobina " + material + " " + (b.cor_nome || nome) + " nº " + b.numero + " (" + gramas(b.restante_g) + ") na " + imp.nome +
			(imp.slots.length > 1 ? ", no lugar da posição " + pos.posicao + "." : "."),
			pecas + " entra na fila da " + imp.nome + (grupo.tempo_s ? " e leva " + duracao(grupo.tempo_s) + "." : "."),
			{ tipo: "carregar", impressora_id: imp.id, posicao: pos.posicao, bobina_id: b.id, rotulo: "Coloquei a bobina na " + imp.nome });
	}
	return _c("falta_cor", "Falta cor na impressora: " + material + " " + nome, true, porque,
		"não há bobina de " + material + " " + nome + " no estoque. Cadastre a bobina e coloque numa impressora.",
		pecas + " entra na fila assim que a cor estiver numa impressora.",
		{ tipo: "link", href: "/farm/estoque", rotulo: "Cadastrar bobina" });
}

function _confere_filamento(g, bob, correcoes) {
	var por_bobina = [];   // [[chave, mesas]] na ordem
	g.mesas.forEach(function (m) {
		if (m.bobina_id && m.impressora_id) {
			var k = m.impressora_id + "\u0000" + m.bobina_id, e = por_bobina.filter(function (x) { return x[0] === k; })[0];
			if (e) e[2].push(m); else por_bobina.push([k, m.bobina_id, [m]]);
		}
	});
	por_bobina.forEach(function (e) {
		var bid = e[1], ms = e[2], b = bob[bid];
		if (!b || !ms.some(function (m) { return m.bloqueio === "filamento"; })) return;
		var precisa = soma(ms, function (m) { return m.gramas; });
		var tem = Math.max(0.0, ou0(b.restante_g) - ou0(b.reservado_g));
		var primeira = ms.filter(function (m) { return m.bloqueio === "filamento"; })[0];
		var resto = soma(ms.filter(function (m) { return m.bloqueio === "filamento"; }), function (m) { return m.gramas; });
		var outras = ordenar(Object.keys(bob).map(function (k) { return bob[k]; }).filter(function (x) {
			return x.id !== bid && x.material === b.material && cor_perto(x.cor_hex, b.cor_hex) && ou0(x.restante_g) > 0;
		}), function (x) { return [ou0(x.restante_g) < resto, ou0(x.restante_g)]; });
		var antes = ms.filter(function (m) { return m.bloqueio !== "filamento"; }).length;
		var como, acao;
		if (outras.length) {
			var o = ou0(outras[0].restante_g) >= resto ? outras[0] : outras[outras.length - 1];
			como = "troque pela bobina nº " + o.numero + " (" + gramas(o.restante_g) + ") " + (antes ? "depois da mesa " + antes : "antes de começar") +
				". As mesas seguintes ficam aguardando a troca." +
				(ou0(o.restante_g) >= resto ? "" : " Mesmo assim ainda faltam " + gramas(resto - ou0(o.restante_g)) + ": cadastre mais uma bobina.");
			acao = { tipo: "carregar", impressora_id: primeira.impressora_id, posicao: primeira.posicao, bobina_id: o.id, rotulo: "Troquei a bobina" };
		} else {
			como = "faltam " + gramas(precisa - tem) + ". Cadastre outra bobina de " + b.material + " " + g.cor_nome + " e coloque na impressora.";
			acao = { tipo: "link", href: "/farm/estoque", rotulo: "Ver estoque" };
		}
		correcoes.push(Object.assign(_c("filamento", "Filamento insuficiente: " + b.material + " " + g.cor_nome + " na " + primeira.impressora, true,
			primeira.peca + " (" + primeira.arquivo + ") precisa de " + gramas(precisa) + " e a bobina nº " + b.numero + " tem " + gramas(tem) +
			". Acabaria na mesa " + primeira.numero + ".", como, "nenhuma peça perdida por acabar o filamento no meio.", acao),
			{ cor: g.cor, precisa_g: pyRound(precisa, 1), tem_g: pyRound(tem, 1) }));
	});
}

function _juntas(pecas, cores, material, conjuntos, impressoras, bob, un, p, preco_kg) {
	var correcoes = [];
	var area = soma(pecas, function (pc) { return (pc.x + p.folga_mm) * (pc.y + p.folga_mm) * pc.copias; });
	var alto = Math.max.apply(null, pecas.map(function (pc) { return pc.z; }));
	function serve(imp) {
		if ((imp.multicor !== "ams" && imp.multicor !== "cabecas") || alto > imp.mesa_h) return null;
		var achados = [];
		for (var i = 0; i < cores.length; i++) {
			var c = cores[i], s = imp.slots.filter(function (s) { return s.material === material && cor_perto(s.cor_hex, c); })[0];
			if (!s) return null;
			achados.push(s);
		}
		return achados;
	}
	function por_mesa(imp) {
		if (pecas.some(function (pc) { return cabem(pc.x, pc.y, pc.z, _mesa(imp), p)[0] < 1; })) return 0;
		var util = (imp.mesa_w - 2 * p.borda_mm) * (imp.mesa_d - 2 * p.borda_mm) * p.ocupacao_mista;
		return area ? Math.max(1, Math.floor(util / area)) : 0;
	}
	var aptas = impressoras.map(function (i) { return [i, serve(i)]; }).filter(function (t) { return t[1] && por_mesa(t[0]) >= 1; });
	var tem_multicor = impressoras.some(function (i) { return i.multicor === "ams" || i.multicor === "cabecas"; });
	var possivel = aptas.length > 0, tipo;
	if (aptas.length) tipo = aptas[0][0].multicor;
	else { var mi = impressoras.filter(function (i) { return i.multicor === "ams" || i.multicor === "cabecas"; })[0]; tipo = mi ? mi.multicor : "ams"; }
	var alt = 0;
	for (var k in un) if (un[k] && un[k].altura_camada) { alt = un[k].altura_camada; break; }
	alt = alt || p.altura_camada;
	var camadas = Math.max(1, Math.ceil(alto / alt)), trocas_mesa = 0;
	for (var i = 0; i < camadas; i++) {
		var z = i * alt, vistas = [];
		pecas.forEach(function (pc) {
			if (pc.z > z) (pc.cores.length ? pc.cores : [pc.cor]).forEach(function (x) {
				if (!x) return;
				var c = cores.filter(function (c) { return cor_perto(c, x); })[0]; c = c === undefined ? x : c;
				if (vistas.indexOf(c) < 0) vistas.push(c);
			});
		});
		trocas_mesa += Math.max(0, vistas.length - 1);
	}
	var t_conj = soma(pecas, function (pc) { return (un[pc.id] ? un[pc.id].tempo_s : 0) * pc.copias; });
	var g_conj = soma(pecas, function (pc) { return (un[pc.id] ? un[pc.id].gramas : 0) * pc.copias; });
	var referencia = aptas.length ? aptas[0][0] : maxPor(impressoras, function (i) { return i.mesa_w * i.mesa_d; }, null);
	var n = (referencia ? por_mesa(referencia) : 0) || 1;
	var carga = {}, inicio = {};
	aptas.forEach(function (t) { carga[t[0].id] = parseFloat(t[0].carga_s || 0) || 0; inicio[t[0].id] = carga[t[0].id]; });
	var mesas = [], restante = conjuntos, numero = 0, total_mesas = Math.ceil(conjuntos / n);
	while (restante > 0) {
		var q = Math.min(n, restante);
		restante -= q; numero += 1;
		var t = t_conj * q + trocas_mesa * p.troca_s[tipo], imp = null;
		if (aptas.length) {
			imp = minPor(aptas, function (t_) { return carga[t_[0].id]; })[0];
			carga[imp.id] += t + p.min_mesa_extra * 60;
		}
		mesas.push({ peca_id: null, peca: "Conjunto completo", arquivo: pecas[0].arquivo, quantidade: q, numero: numero, total: total_mesas,
			impressora_id: imp ? imp.id : null, impressora: imp ? imp.nome : null, posicao: null, bobina_id: null, material: material,
			cor: cores.length ? cores[0] : "", cor_nome: "Várias cores", tempo_s: pyRound(t),
			gramas: pyRound(g_conj * q + trocas_mesa * p.purga_g[tipo], 1), purga_g: pyRound(trocas_mesa * p.purga_g[tipo], 1),
			trocas: trocas_mesa, bloqueio: imp ? null : "sem_multicor", cabem: n });
	}
	var purga = pyRound(soma(mesas, function (m) { return m.purga_g; }), 1);
	var kg = aptas.length ? soma(aptas[0][1], function (s) { return preco_kg(s.bobina_id); }) / Math.max(1, aptas[0][1].length) : preco_kg(null);
	if (!possivel) {
		var como = !tem_multicor ? "cadastre uma impressora que troca de cor (com AMS ou várias cabeças), ou responda “Uma cor por impressora”."
			: "coloque as " + cores.length + " cores em " + material + " na mesma impressora que troca de cor, ou responda “Uma cor por impressora”.";
		correcoes.push(_c("sem_multicor", "Nenhuma impressora pronta para várias cores na mesma mesa", true,
			"imprimir todas as cores juntas exige uma impressora que troca de cor, com todas as cores carregadas.",
			como, "as mesas ficam aguardando até existir uma impressora assim."));
	}
	var usadas = []; mesas.forEach(function (m) { if (m.impressora_id && usadas.indexOf(m.impressora_id) < 0) usadas.push(m.impressora_id); });
	var seq = pyRound(soma(mesas, function (m) { return m.tempo_s; }));
	return { mesas: mesas, grupos: [], correcoes: correcoes, possivel: possivel, trocas: trocas_mesa * mesas.length,
		purga_g: purga, prejuizo: pyRound(purga / 1000 * kg, 2), gramas: pyRound(soma(mesas, function (m) { return m.gramas; }), 1),
		sequencial_s: seq, prazo_s: pyRound(usadas.length ? Math.max.apply(null, usadas.map(function (i) { return carga[i] - inicio[i]; })) : 0) || seq,
		n_impressoras: usadas.length || 1 };
}

function _sugestoes(sep, simples, material, conjuntos, r, impressoras, bob, un, p) {
	if (!sep.grupos.length || !sep.prazo_s || (r.prioridade || "prazo") !== "prazo") return [];
	var usadas = {}; sep.mesas.forEach(function (m) { usadas[m.impressora_id] = 1; });
	var livres = impressoras.filter(function (i) { return !(i.id in usadas) && i.estado === "livre" && i.slots.length; });
	var carregadas = {};
	impressoras.forEach(function (i) { i.slots.forEach(function (s) { if (s.bobina_id) carregadas[s.bobina_id] = 1; }); });
	var gargalo = maxPor(sep.grupos, function (g) { return g.tempo_s / Math.max(1, g.impressoras.length); });
	if (!livres.length || !gargalo.impressoras.length || !gargalo.cor) return [];
	material = gargalo.material || material;
	var b = ordenar(Object.keys(bob).map(function (k) { return bob[k]; }), function (x) { return -ou0(x.restante_g); }).filter(function (x) {
		return x.material === material && cor_perto(x.cor_hex, gargalo.cor) && !(x.id in carregadas) && ou0(x.restante_g) > 0;
	})[0];
	if (!b) return [];
	var alvo = livres[0];
	var teste = impressoras.map(function (i) {
		return Object.assign({}, i, { slots: i.id === alvo.id ? [{ posicao: i.slots[0].posicao, material: material, cor_hex: b.cor_hex, cor_nome: b.cor_nome, bobina_id: b.id }].concat(i.slots.slice(1)) : i.slots });
	});
	var novo = _separadas(simples, [], material, conjuntos, r, teste, bob, un, p, [], function () { return 0; });
	if (novo.prazo_s >= sep.prazo_s - 300) return [];
	return [{ texto: "a " + alvo.nome + " está livre. Colocando " + material + " " + gargalo.cor_nome + " (bobina nº " + b.numero + ") nela, " +
		"o prazo cai de " + duracao(sep.prazo_s) + " para " + duracao(novo.prazo_s) + ".",
		acao: { tipo: "carregar", impressora_id: alvo.id, posicao: alvo.slots[0].posicao, bobina_id: b.id, rotulo: "Coloquei a bobina na " + alvo.nome } }];
}

raiz.FarmPlano = { PADRAO: PADRAO, CORES_NOME: CORES_NOME, planejar: planejar, cabem: cabem, cor_perto: cor_perto, distancia_cor: distancia_cor,
	nome_cor: nome_cor, duracao: duracao, gramas: gramas, reais: reais, cm: _cm, material_da_cor: material_da_cor, posicao_livre: posicao_livre,
	pyRound: pyRound };
})(typeof window !== "undefined" ? window : globalThis);
