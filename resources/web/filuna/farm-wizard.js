/* Filuna Farm no Slicer — funções do wizard. Porte fiel de site-v2/app/farm.py (branch site-v2/farm-wizard):
 * PASSOS, RESPOSTAS_PADRAO, _respostas_do_form, _aguardando, _cores_do_pedido, _linha_do_tempo, _tinta, _mensagens,
 * _explicacoes e o encaixe da etapa 2. Conferido contra o Python com os mesmos dados (teste-farm-plano). */
(function (raiz) {
"use strict";
var FP = raiz.FarmPlano;
var MATERIAIS = ["PLA", "PETG", "ABS", "TPU", "ASA"];
var HEX = /^#[0-9a-fA-F]{6}$/;
var PASSOS = [["Arquivo", "Leitura do arquivo"], ["Quantidade", "Montagem na mesa"], ["Cor e material", "Confirmação e filamento"],
	["Plano de impressão", "Impressoras, tempo e riscos"], ["Custo e envio", "Resumo e confirmação"], ["Acompanhar", "Impressão e avisos"]];
var RESPOSTAS_PADRAO = { material: "PLA", conjuntos: 1, modo_cores: "separadas", iguais: "juntas", prioridade: "prazo", wizard: 1 };

function num(v, padrao, minimo, maximo) {   // _num do site
	var n = parseFloat(String(v === undefined || v === null ? "" : v).replace(",", ".").trim());
	if (!isFinite(n)) return padrao;
	return Math.max(minimo, Math.min(maximo, n));
}

function respostas_do_form(f, r) {
	var base = {}; for (var k in RESPOSTAS_PADRAO) if (k !== "wizard") base[k] = RESPOSTAS_PADRAO[k];
	r = Object.assign(base, r || {});
	if (MATERIAIS.indexOf(f.material) >= 0) r.material = f.material;
	if (f.conjuntos) r.conjuntos = Math.trunc(num(f.conjuntos, 1, 1, 100000));
	if (f.modo_cores) r.modo_cores = f.modo_cores === "juntas" ? "juntas" : "separadas";
	if (f.iguais) r.iguais = f.iguais === "uma_por_mesa" ? "uma_por_mesa" : "juntas";
	if (["prazo", "filamento", "impressoras"].indexOf(f.prioridade) >= 0) r.prioridade = f.prioridade;
	var mats = {}, espera = (r.espera || []).slice(), troca = Object.assign({}, r.troca || {});
	Object.keys(f).forEach(function (k) {
		if (k.indexOf("troca_") === 0 && HEX.test("#" + k.slice(6))) {
			if (f[k]) troca["#" + k.slice(6).toUpperCase()] = f[k]; else delete troca["#" + k.slice(6).toUpperCase()];
		}
		if (k.indexOf("mat_") === 0 && MATERIAIS.indexOf(f[k]) >= 0 && HEX.test("#" + k.slice(4))) mats["#" + k.slice(4).toUpperCase()] = f[k];
		if (k.indexOf("espera_") === 0 && HEX.test("#" + k.slice(7))) {
			var c = "#" + k.slice(7).toUpperCase(), i = espera.indexOf(c);
			if (f[k] === "aguardar") { if (i < 0) espera.push(c); } else if (i >= 0) espera.splice(i, 1);
		}
	});
	if (Object.keys(mats).length) {
		r.materiais = Object.assign({}, r.materiais || {}, mats);
		var cont = {}, ordem = [];
		Object.keys(r.materiais).forEach(function (h) { var m = r.materiais[h]; if (!(m in cont)) { cont[m] = 0; ordem.push(m); } cont[m] += 1; });
		var melhor = ordem[0];
		ordem.forEach(function (m) { if (cont[m] > cont[melhor] || (cont[m] === cont[melhor] && m === "PLA" && melhor !== "PLA")) melhor = m; });
		r.material = melhor;
	}
	r.espera = espera.slice().sort();
	r.troca = troca;
	return r;
}

function aguardando(pl, r) {
	return pl.correcoes.filter(function (c) {
		return c.tipo === "falta_cor" && !c.resolvido && c.cor && (r.espera || []).some(function (e) { return FP.cor_perto(c.cor, e); });
	}).map(function (c) { return c.cor_nome || FP.nome_cor(c.cor); });   // falta_cor não traz cor_nome (mesma correção do site)
}

function cores_do_pedido(pcs, imps, bobs, r, pl) {
	var saida = [];
	pcs.forEach(function (p) {
		(p.cores && p.cores.length ? p.cores : (p.cor ? [p.cor] : [])).forEach(function (cor) {
			var item = saida.filter(function (x) { return FP.cor_perto(x.hex, cor); })[0];
			if (!item) { item = { hex: cor.toUpperCase(), chave: cor.toUpperCase().replace(/^#+/, ""), pecas: [], foto: p.id, nome: FP.nome_cor(cor) }; saida.push(item); }
			if (item.pecas.indexOf(p.nome) < 0) item.pecas.push(p.nome);
		});
	});
	saida.forEach(function (item) {
		var achou = [];
		imps.forEach(function (i) {
			if (i.estado === "offline") return;
			i.slots.forEach(function (s) {
				if (s.material && FP.cor_perto(s.cor_hex, item.hex)) {
					achou.push({ impressora: i.nome, material: s.material, restante_g: s.restante_g === undefined ? null : s.restante_g });
					item.nome = s.cor_nome || item.nome;
				}
			});
		});
		item.achou = achou;
		item.materiais = [];
		achou.forEach(function (a) { if (item.materiais.indexOf(a.material) < 0) item.materiais.push(a.material); });
		item.estoque = {};
		MATERIAIS.forEach(function (m) {
			item.estoque[m] = bobs.filter(function (b) { return b.material === m && FP.cor_perto(b.cor_hex, item.hex) && (b.restante_g || 0) > 0; }).length;
		});
		item.confirmado = null;
		var mats = r.materiais || {};
		for (var hx in mats) if (FP.cor_perto(hx, item.hex)) { item.confirmado = mats[hx]; break; }
		item.espera = (r.espera || []).some(function (e) { return FP.cor_perto(e, item.hex); });
		item.correcoes = ((pl || {}).correcoes || []).filter(function (c) { return c.cor && FP.cor_perto(c.cor, item.hex); });
	});
	return saida;
}

function tinta(hx) {
	var h = (hx || "#8E8E8E").replace(/^#+/, "");
	var r = parseInt(h.slice(0, 2), 16), g = parseInt(h.slice(2, 4), 16), b = parseInt(h.slice(4, 6), 16);
	if (isNaN(r) || isNaN(g) || isNaN(b)) r = g = b = 0;
	return r * .299 + g * .587 + b * .114 > 160 ? "#1D1A17" : "#fff";
}

function linha_do_tempo(pl, imps) {
	var extra = FP.PADRAO.min_mesa_extra * 60, por_imp = {}, ordem = [];
	imps.forEach(function (i) { if (!(i.id in por_imp)) ordem.push(i.id); por_imp[i.id] = { nome: i.nome, modelo: i.modelo || "", blocos: [], t: 0.0 }; });
	pl.mesas.forEach(function (m) {
		if (!m.impressora_id || !(m.impressora_id in por_imp)) return;
		var lane = por_imp[m.impressora_id];
		if (!lane.blocos.length) {
			var ii = imps.filter(function (i) { return i.id === m.impressora_id; })[0];
			var antes = (ii ? ii.carga_s : 0) || 0;
			if (antes) { lane.blocos.push({ tipo: "outro", ini: 0.0, dur: parseFloat(antes), rotulo: "outro pedido" }); lane.t = parseFloat(antes); }
		}
		if (lane.blocos.length) { lane.blocos.push({ tipo: "limpar", ini: lane.t, dur: extra, rotulo: "limpar mesa" }); lane.t += extra; }
		var dur = parseFloat(m.tempo_s || 0) || 1800.0;
		lane.blocos.push({ tipo: "mesa", ini: lane.t, dur: dur, cor: m.cor, peca_id: m.peca_id, tinta: tinta(m.cor),
			rotulo: m.quantidade > 1 ? m.peca + " ×" + m.quantidade : m.peca, tempo: m.tempo_s ? FP.duracao(m.tempo_s) : "a medir" });
		lane.t += dur;
	});
	var lanes = ordem.map(function (k) { return por_imp[k]; }).filter(function (x) { return x.blocos.some(function (b) { return b.tipo === "mesa"; }); });
	var total = lanes.length ? Math.max.apply(null, lanes.map(function (x) { return x.t; })) : 1.0;
	total = total || 1.0;
	var passo = total > 6 * 3600 ? 7200 : total > 2 * 3600 ? 3600 : 1800;
	lanes.forEach(function (x) { x.blocos.forEach(function (b) { b.esq = FP.pyRound(100 * b.ini / total, 2); b.larg = FP.pyRound(Math.max(0.6, 100 * b.dur / total), 2); }); });
	var marcas = [];
	for (var s = 0; s <= Math.trunc(total); s += passo) marcas.push({ esq: FP.pyRound(100 * s / total, 2), rotulo: s ? FP.duracao(s) : "agora" });
	return { lanes: lanes, total: total, marcas: marcas };
}

function mensagens(pl, n_cores, pendentes) {
	var d = FP.duracao, msgs = [], imps = [];
	pl.mesas.forEach(function (m) { if (m.impressora && imps.indexOf(m.impressora) < 0) imps.push(m.impressora); });
	if (imps.length > 1) msgs.push("Seu projeto tem " + n_cores + " cor" + (n_cores !== 1 ? "es" : "") + ". Vou usar " + imps.length +
		" impressoras ao mesmo tempo para acelerar: " + imps.join(", ") + ".");
	imps.forEach(function (nome) {
		var ms = pl.mesas.filter(function (m) { return m.impressora === nome; }), pecas = [];
		ms.forEach(function (m) { if (pecas.indexOf(m.peca) < 0) pecas.push(m.peca); });
		if (pecas.length > 1) {
			var a = ms[0], b = ms.filter(function (m) { return m.peca !== ms[0].peca; })[0];
			msgs.push("Primeiro vou imprimir " + a.peca + " em " + a.cor_nome + " na " + nome + ". Quando terminar, você recebe uma " +
				"mensagem para limpar a mesa e eu continuo com " + b.peca + " em " + b.cor_nome + (pecas.length > 2 ? " e depois o resto." : "."));
		} else if (ms.length > 1)
			msgs.push("A quantidade não cabe numa mesa só da " + nome + ": são " + ms.length + " mesas. Entre uma e outra eu peço para você " +
				"confirmar a mesa limpa.");
	});
	pendentes.forEach(function (cor) {
		msgs.push("As peças em " + cor + " ficam pendentes no projeto, sem perder nada: quando a cor estiver numa impressora, " +
			"elas entram na fila sozinhas.");
	});
	if (pl.resumo.prazo_s) msgs.push((pendentes.length ? "O resto fica" : "Fica") + " pronto em " + d(pl.resumo.prazo_s) + ".");
	return msgs;
}

function explicacoes(pl, r, n_imps) {
	var d = FP.duracao, g = FP.gramas, res = pl.resumo;
	var exp = {
		material: ["o material define quais impressoras e bobinas podem receber o pedido.",
			"o sistema procurou " + r.material + " nas suas " + n_imps + " impressora" + (n_imps !== 1 ? "s" : "") + "."],
		conjuntos: ["a quantidade decide quantas mesas e quanto filamento o pedido precisa.",
			res.pecas + " peça" + (res.pecas !== 1 ? "s" : "") + " em " + res.mesas + " mesa" + (res.mesas !== 1 ? "s" : "") +
			(res.gramas ? " e " + g(res.gramas) + " de filamento." : ".")]
	};
	var cc = pl.comparacoes.cores;
	if (cc) {
		var j = cc.juntas, s = cc.separadas, dif = j.prazo_s - s.prazo_s, fim;
		if (pl.modo === "separadas")
			fim = "separando, você deixa de jogar fora " + g(j.purga_g) + " (" + FP.reais(j.prejuizo) + ")" + (dif > 300 ? " e termina " + d(dif) + " antes" : "") +
				". As peças saem soltas para montar.";
		else
			fim = "juntando, a purga estimada é de " + g(j.purga_g) + " (" + FP.reais(j.prejuizo) + ", " + j.pct + "% do filamento)" +
				(dif > 300 ? " e o pedido leva " + d(dif) + " a mais" : "") + ". As peças já saem montadas.";
		exp.modo_cores = ["cada troca de cor na mesma mesa joga filamento fora (purga) e só funciona em impressora que troca de cor.", fim];
	}
	var ci = pl.comparacoes.iguais;
	if (r.iguais === "uma_por_mesa")
		exp.iguais = ["cada mesa a mais custa um aquecimento e uma limpeza.",
			ci.uma.mesas + " mesas em vez de " + ci.juntas.mesas + ": cerca de " + d(ci.uma.extra_s) + " a mais parados."];
	else
		exp.iguais = ["cada mesa a mais custa um aquecimento e uma limpeza.",
			ci.juntas.mesas + " mesa" + (ci.juntas.mesas !== 1 ? "s" : "") + " em vez de " + ci.uma.mesas +
			(ci.uma.extra_s ? ": separando o que pode ir junto você perderia cerca de " + d(ci.uma.extra_s) + " só esperando." : ".")];
	exp.prioridade = ["o sistema usa essa resposta para decidir quando dividir uma cor entre duas impressoras.",
		(res.prazo_s && res.impressoras > 1) ? "o pedido usa " + res.impressoras + " impressora" + (res.impressoras !== 1 ? "s" : "") + ": " +
			d(res.prazo_s) + " em vez de " + d(res.sequencial_s) + " numa só."
			: (res.sequencial_s ? "o pedido leva " + d(res.prazo_s || res.sequencial_s) + "." : "o prazo aparece quando o tempo for medido.")];
	return exp;
}

// Etapa 2: quantas peças de cada tipo cabem numa mesa de cada impressora (mesma conta da página do site)
function encaixe(pcs, imps, conjuntos) {
	return pcs.map(function (p) {
		var total = p.copias * conjuntos;
		return { peca: p, total: total, imps: imps.map(function (i) {
			var n = FP.cabem(p.x, p.y, p.z, { w: i.mesa_w, d: i.mesa_d, h: i.mesa_h }, FP.PADRAO)[0];
			return { nome: i.nome, cabem: n, mesas: n ? Math.ceil(total / n) : 0, mesa: fmtG(i.mesa_w) + " × " + fmtG(i.mesa_d) + " mm" };
		}) };
	});
}
function fmtG(v) { var n = Number(v); return String(Number.isInteger(n) ? n : n); }   // f"{x:g}" para os tamanhos de mesa

raiz.FarmWizard = { MATERIAIS: MATERIAIS, PASSOS: PASSOS, RESPOSTAS_PADRAO: RESPOSTAS_PADRAO, respostas_do_form: respostas_do_form,
	aguardando: aguardando, cores_do_pedido: cores_do_pedido, linha_do_tempo: linha_do_tempo, tinta: tinta, mensagens: mensagens,
	explicacoes: explicacoes, encaixe: encaixe };
})(typeof window !== "undefined" ? window : globalThis);
