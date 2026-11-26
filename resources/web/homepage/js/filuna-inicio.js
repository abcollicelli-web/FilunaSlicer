/* Filuna — tela Início (prévia aprovada pelo dono em 26/11/2026).
 * Impressoras ao vivo, estoque e histórico vêm deste computador pelo comando "filuna_nativo" (GUI_App.cpp, só três
 * leituras); Modelos 3D, Ferramentas e Paramétricos vêm do site. A bobina é desenhada por Filuna.bobinaSVG (ponte.js),
 * a mesma do Estoque, e a que está imprimindo gira. Depende de filuna-home.js (FlChamar, FlEsc, FlAbrirAba, FlToken).
 */
var FI = { pend: {}, seq: 0, estado: null, bobinas: [], giram: {}, recentes: [], fotos: {} };

function FlNativo(acao) {
	return new Promise(function (ok, falha) {
		var id = "fi" + (++FI.seq);
		FI.pend[id] = { ok: ok, falha: falha };
		try {
			window.wx.postMessage(JSON.stringify({ command: "filuna_nativo", id: id, acao: acao }));
		} catch (e) {
			delete FI.pend[id];
			falha(e);
			return;
		}
		setTimeout(function () { if (FI.pend[id]) { delete FI.pend[id]; falha(new Error("O Filuna não respondeu.")); } }, 15000);
	});
}
function FlNativoResposta(id, ok, dados) {
	var p = FI.pend[id];
	if (!p) return;
	delete FI.pend[id];
	if (ok) p.ok(dados || {}); else p.falha(dados || {});
}
function FiImportar() { try { window.wx.postMessage(JSON.stringify({ command: "filuna_importar" })); } catch (e) { } }

var FI_ESTADO = { imprimindo: ["Imprimindo", ""], pausada: ["Pausada", "erro"], livre: ["Pronta", "livre"], concluida: ["Concluída", "livre"],
	limpar_mesa: ["Limpar a mesa", "erro"], desligada: ["Desligada", "off"] };
function fiCor(c) {
	c = String(c || "");
	if (/^[0-9a-f]{6,8}$/i.test(c)) c = "#" + c;
	return /^#[0-9a-f]{6}/i.test(c) ? c.slice(0, 7) : "";
}
function fiGraus(v) { return v == null || isNaN(+v) ? "—" : Math.round(+v) + " °C"; }
function fiEntrada(sa) {
	if (!sa || sa.slot === undefined || sa.slot === "") return 0;
	var u = +sa.unidade || 0, s = +sa.slot;
	return isNaN(s) || s < 0 || s > 15 || u > 63 ? 0 : u * 4 + s + 1;
}

/* ---------- saudação e resumo ---------- */
function fiSaudacao() {
	var h = new Date().getHours(), nome = ($("#FlNome").text() || "").trim().split(" ")[0];
	$("#FiOla").text((h < 12 ? "Bom dia" : h < 18 ? "Boa tarde" : "Boa noite") + (nome ? ", " + nome : ""));
	var imps = ((FI.estado || {}).impressoras || []).filter(function (i) { return i.ligada; });
	var imp = imps.filter(function (i) { return i.estado == "imprimindo"; }).length;
	var ativas = FI.bobinas.filter(function (x) { return x.estado == "lacrada" || x.estado == "em_uso"; });
	var kg = ativas.reduce(function (a, x) { return a + (+x.restante_g || 0); }, 0);
	var partes = [];
	if (FI.estado) partes.push(imps.length ? imps.length + (imps.length == 1 ? " impressora conectada" : " impressoras conectadas") + (imp ? " · " + imp + " imprimindo" : "") : "Nenhuma impressora conectada");
	if (ativas.length) partes.push(ativas.length + (ativas.length == 1 ? " bobina" : " bobinas") + " no estoque (" + Filuna.gramas(kg) + ")");
	$("#FiResumo").text(partes.length ? partes.join(" · ") : "Abra um modelo, fatie e envie para a impressora.");
}

/* ---------- impressoras ao vivo ---------- */
var FI_IMP_SVG = '<svg viewBox="0 0 52 60" aria-hidden="true"><rect x="4" y="2" width="44" height="6" rx="2" fill="#5E5752"/><rect x="20" y="8" width="12" height="14" rx="2" fill="#C9C2BA"/>' +
	'<path d="M26 22v14" stroke="#F07FB0" stroke-width="2"/><rect x="6" y="50" width="40" height="5" rx="2" fill="#8A817A"/></svg>';
function fiCores(i, ativa) {
	var cores = i.cores || [];
	if (!cores.length) return "";
	var cheias = cores.filter(function (c) { return !c.vazio; }).length;
	var bolas = cores.map(function (c) {
		var n = (+c.unidade || 0) * 4 + (+c.slot || 0) + 1, cor = fiCor(c.cor);
		return '<i class="' + (c.vazio || !cor ? "vazia" : "") + (ativa && n == ativa ? " ativa" : "") + '" style="background:' + (cor || "transparent") + '" title="Entrada ' + n + (c.material ? " · " + FlEsc(c.material) : "") + '"></i>';
	}).join("");
	return bolas + "<span>" + FlEsc(ativa ? "puxando a entrada " + ativa : cheias + " de " + cores.length + " entradas com filamento") + "</span>";
}
function fiCartaoImp(i) {
	var e = FI_ESTADO[i.estado] || [i.estado || "—", "off"], nome = i.nome || i.modelo_nome || "Impressora";
	var abre = "FlAbrirAba('filuna.dispositivo', 'unidade:" + FlEsc(nome).replace(/'/g, "&#39;") + "')";
	if (i.estado == "imprimindo" || i.estado == "pausada") {
		var p = Math.max(0, Math.min(100, Math.round(+i.progresso || 0))), resta = +i.restante_s || 0;
		var total = p > 0 && p < 100 && resta ? Math.round(resta / (1 - p / 100)) : 0, ativa = fiEntrada(i.slot_ativo);
		var linha = (i.peca || "Impressão em andamento") + (i.camadas ? " · camada " + (i.camada || 0) + " de " + i.camadas : "");
		return '<a class="FiCard FiImp ativa" onclick="' + abre + '"><span class="FiImpFoto">' + FI_IMP_SVG + '</span><span class="FiImpTxt">' +
			'<span class="FiImpCab"><b>' + FlEsc(nome) + '</b><span class="FiPil ' + e[1] + '">' + e[0] + "</span></span>" +
			'<span class="FiLin">' + FlEsc(linha) + "</span>" +
			'<span class="FiNums"><b>' + p + "%</b><span>" + (resta ? Filuna.tempo(resta) + " restantes" : "calculando o tempo") + "</span>" + (total ? "<small>" + Filuna.tempo(total) + " no total</small>" : "") + "</span>" +
			'<span class="FiBarra"><i style="width:' + p + '%"></i></span>' +
			'<span class="FiCores">' + fiCores(i, ativa) + (i.bico ? "<span>· bico " + fiGraus(i.bico.atual) + " · mesa " + fiGraus(i.mesa && i.mesa.atual) + "</span>" : "") + "</span></span></a>";
	}
	return '<a class="FiCard FiImp FiLivre" onclick="' + abre + '"><span class="FiImpCab"><b>' + FlEsc(nome) + '</b><span class="FiPil ' + e[1] + '">' + e[0] + "</span></span>" +
		'<span class="FiLin">' + (i.ligada ? (i.estado == "limpar_mesa" ? "Retire as peças e limpe a mesa" : "Livre") + " · bico " + fiGraus(i.bico && i.bico.atual) + " · mesa " + fiGraus(i.mesa && i.mesa.atual) : "Fora da rede ou desligada") + "</span>" +
		'<span class="FiCores">' + fiCores(i, 0) + "</span>" +
		'<span class="FiIr">' + (i.ligada ? "Mandar uma impressão →" : "Ver a impressora →") + "</span></a>";
}
function fiImpressoras() {
	var l = ((FI.estado || {}).impressoras || []).slice();
	var ordem = { imprimindo: 0, pausada: 1, limpar_mesa: 2, livre: 3, concluida: 3, desligada: 5 };
	l.sort(function (a, b) { return (ordem[a.estado] == null ? 4 : ordem[a.estado]) - (ordem[b.estado] == null ? 4 : ordem[b.estado]); });
	var el = $("#FiImps");
	if (!l.length) {
		el.addClass("uma").html('<a class="FiCard FiImp FiLivre" onclick="FlAbrirAba(\'filuna.dispositivo\')"><span class="FiImpCab"><b>Nenhuma impressora conectada</b></span>' +
			'<span class="FiLin">Conecte a sua impressora pela rede para mandar impressões e ver o andamento aqui.</span><span class="FiIr">Conectar impressora →</span></a>');
		return;
	}
	el.toggleClass("uma", l.length == 1).html(l.slice(0, 2).map(fiCartaoImp).join(""));
}

/* ---------- recentes (a lista do home.js, os três últimos) ---------- */
function fiRecentes() {
	var l = FI.recentes.slice(0, 3);
	if (!l.length) { $("#FiRecLista").html('<p class="FiVazio">Nenhum projeto aberto ainda.</p>'); return; }
	$("#FiRecLista").html(l.map(function (f, k) {
		var nome = f.project_name || String(f.path || "").split(/[\\/]/).pop().replace(/\.3mf$/i, "");
		var q = String(f.time || "").replace(/^(\d{4})[\/-](\d{1,2})[\/-](\d{1,2}).*$/, function (_, a, m, d) { return ("0" + d).slice(-2) + "/" + ("0" + m).slice(-2); });
		var ic = f.image ? '<img src="' + FlEsc(f.image) + '" alt="">' : '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 3l8 4.5v9L12 21l-8-4.5v-9z"/></svg>';
		return '<a class="FiRec" data-k="' + k + '" title="' + FlEsc(f.path || "") + '"><span class="ic">' + ic + '</span><span class="n">' + FlEsc(nome) + '</span><span class="q">' + FlEsc(q) + "</span></a>";
	}).join(""));
	$("#FiRecLista .FiRec").on("click", function () { var f = l[+$(this).attr("data-k")]; if (f && window.OnOpenRecentFile) OnOpenRecentFile(f.path); });
}
(function () {
	var antes = window.ShowRecentFileList;
	if (typeof antes != "function") return;
	window.ShowRecentFileList = function (lista) {
		FI.recentes = lista || [];
		fiRecentes();
		return antes.apply(this, arguments);
	};
})();

/* ---------- estoque: a bobina que imprime em destaque, girando ---------- */
function fiBobina(x) {
	var t = x.tipo || {}, imp = /^impressora:/.test(x.local || "") ? x.local.split(":")[1] : "", ent = /:slot:(\d+)$/.exec(x.local || "");
	return { id: x.id, material: t.material || "", cor: t.cor_hex || "#C2185B", cor_nome: t.cor_nome || "", marca: t.marca || "", estado: x.estado,
		restante_g: +x.restante_g || 0, peso_inicial_g: +x.capacidade_g || 0, na_impressora: imp, entrada: ent ? +ent[1] : 0 };
}
function fiNomeImp(id) {
	var i = ((FI.estado || {}).impressoras || []).filter(function (i) { return i.cadastro_id == id || i.dev_id == id; })[0];
	return i ? i.nome || i.modelo_nome || "Impressora" : "";
}
// Mesma regra do Estoque (meu-espaco.html): gira a bobina da entrada que a impressora está puxando; sem essa
// informação, as que o trabalho aberto confirmou; senão todas as carregadas na impressora que imprime.
function fiGiram(trabalhos) {
	var imprimindo = {}, entrada = {}, confirmadas = {}, por = {}, giram = {};
	((FI.estado || {}).impressoras || []).forEach(function (i) {
		if (i.estado != "imprimindo") return;
		var e = fiEntrada(i.slot_ativo);
		[i.cadastro_id, i.dev_id].forEach(function (k) { if (k) { imprimindo[k] = true; if (e) entrada[k] = e; } });
	});
	(trabalhos || []).forEach(function (t) { if (t.estado == "imprimindo") (t.bobinas || []).forEach(function (id) { if (id) confirmadas[id] = true; }); });
	FI.bobinas.forEach(function (x) {
		if ((x.estado == "lacrada" || x.estado == "em_uso") && x.na_impressora && imprimindo[x.na_impressora]) (por[x.na_impressora] = por[x.na_impressora] || []).push(x);
	});
	Object.keys(por).forEach(function (k) {
		var l = por[k], so = entrada[k] ? l.filter(function (x) { return x.entrada == entrada[k]; }) : l.filter(function (x) { return confirmadas[x.id]; });
		(so.length ? so : l).forEach(function (x) { giram[x.id] = true; });
	});
	return giram;
}
function fiPct(x) { return x.peso_inicial_g ? Math.max(0, Math.min(100, x.restante_g / x.peso_inicial_g * 100)) : 0; }
function fiEstoque() {
	var ativas = FI.bobinas.filter(function (x) { return x.estado == "lacrada" || x.estado == "em_uso"; });
	var total = ativas.reduce(function (a, x) { return a + x.restante_g; }, 0), baixas = ativas.filter(function (x) { return x.restante_g < 150; }).length;
	$("#FiEstResumo").text(ativas.length ? ativas.length + (ativas.length == 1 ? " bobina · " : " bobinas · ") + Filuna.gramas(total) + " · " +
		(baixas ? baixas + " com estoque baixo" : "nenhuma com estoque baixo") : "");
	var uso = ativas.filter(function (x) { return FI.giram[x.id]; })[0];
	if (uso) {
		var p = Math.round(fiPct(uso));
		$("#FiEstUso").html('<a class="FiUso" onclick="FlAbrirAba(\'filuna.espaco\', \'estoque\')"><span class="bob girando">' + Filuna.bobinaSVG(uso.cor, 84, uso.peso_inicial_g ? p / 100 : null) +
			'</span><span class="t"><span class="k">IMPRIMINDO AGORA</span><b>' + FlEsc((uso.marca ? uso.marca + " " : "") + uso.material + " " + uso.cor_nome) + "</b><small>" +
			FlEsc((fiNomeImp(uso.na_impressora) || "Impressora") + (uso.entrada ? " · entrada " + uso.entrada : "")) + '</small><span class="FiBarra"><i style="width:' + p + "%;background:" + FlEsc(uso.cor) + '"></i></span><small>' +
			p + "% · " + Filuna.gramas(uso.restante_g) + " restantes</small></span></a>");
	} else {
		$("#FiEstUso").html(ativas.length ? '<a class="FiUso parada" onclick="FlAbrirAba(\'filuna.espaco\', \'estoque\')"><span class="bob">' + Filuna.bobinaSVG(ativas[0].cor, 84, null) +
			'</span><span class="t"><span class="k" style="color:#5E5752">NADA IMPRIMINDO</span><b>' + Filuna.gramas(total) + "</b><small>" + ativas.length + (ativas.length == 1 ? " bobina ativa" : " bobinas ativas") + "</small></span></a>"
			: '<a class="FiUso parada" onclick="FlAbrirAba(\'filuna.espaco\', \'estoque\')"><span class="t"><b>Estoque vazio</b><small>Cadastre as bobinas para a baixa automática.</small></span></a>');
	}
	// as outras: as que estão nas impressoras primeiro, depois as guardadas
	var resto = ativas.filter(function (x) { return x !== uso; }).sort(function (a, b) { return (b.na_impressora ? 1 : 0) - (a.na_impressora ? 1 : 0) || a.restante_g - b.restante_g; });
	$("#FiEstLista").html(resto.slice(0, 12).map(function (x) {
		var onde = x.na_impressora ? (fiNomeImp(x.na_impressora) || "Impressora").split(" ")[0] + (x.entrada ? " · entrada " + x.entrada : "") : "guardada";
		return '<a class="FiBob' + (x.restante_g < 150 ? " baixa" : "") + '" onclick="FlAbrirAba(\'filuna.espaco\', \'estoque\')" title="' + FlEsc(x.material + " " + x.cor_nome + " · " + Math.round(fiPct(x)) + "% · " + Filuna.gramas(x.restante_g)) + '">' +
			Filuna.bobinaSVG(x.cor, 46, x.peso_inicial_g ? fiPct(x) / 100 : null) + "<b>" + FlEsc(x.material) + "</b><span>" + FlEsc(x.cor_nome || "") + "</span><span>" + FlEsc(onde) + "</span></a>";
	}).join(""));
}

/* ---------- modelos 3D (catálogo do site) ---------- */
function fiModelos() {
	fetch("https://filuna3d.com/api/slicer/v1/modelos3d").then(function (r) { return r.json(); }).then(function (d) {
		var ms = ((d && d.modelos) || []).slice(0, 4);
		var tempo = function (s) { s = +s || 0; return s >= 3600 ? Math.floor(s / 3600) + "h" + ("0" + Math.floor(s % 3600 / 60)).slice(-2) : Math.max(1, Math.round(s / 60)) + " min"; };
		$("#FiM3d").html(ms.map(function (m, k) {
			return '<a class="FiItem" data-k="' + k + '"><span class="foto"><img src="' + FlEsc(m.capa) + '" alt="' + FlEsc(m.titulo) + '" loading="lazy"></span><span class="txt"><div class="n">' + FlEsc(m.titulo) +
				'</div><div class="s">' + (m.tempo_s ? "<span>" + tempo(m.tempo_s) + "</span><span>" + m.gramas + " g</span><b>R$ " + Number(m.preco).toFixed(2).replace(".", ",") + "</b>" : FlEsc(m.categoria || "")) + "</div></span></a>";
		}).join(""));
		$("#FiM3d .FiItem").on("click", function () { var m = ms[+$(this).attr("data-k")]; if (m) FlAbrirAba("filuna.modelos", m.slug ? "modelo:" + m.slug : ""); });
	}).catch(function () { $("#FiM3d").html('<p class="FiVazio">Sem internet agora: os modelos aparecem quando a conexão voltar.</p>'); });
}

/* ---------- ferramentas e paramétricos (recursos da conta) ---------- */
function fiFoto(rota, img) {
	if (!rota || !FlToken()) return;
	if (!FI.fotos[rota]) FI.fotos[rota] = fetch(FL_API + rota, { headers: { "Authorization": "Bearer " + FlToken() } })
		.then(function (r) { if (!r.ok) throw r.status; return r.blob(); }).then(function (b) { return URL.createObjectURL(b); });
	FI.fotos[rota].then(function (u) { img.src = u; }, function () { delete FI.fotos[rota]; });
}
function fiRecursos() {
	if (!FlToken()) {
		var entrar = '<p class="FiVazio">Entre na sua conta Filuna para ver.</p>';
		$("#FiFerr").html(entrar); $("#FiParam").html(entrar);
		return;
	}
	FlChamar("GET", "/recursos").then(function (r) {
		var itens = (r && r.itens) || [];
		[["ferramenta", "#FiFerr", "ferramentas"], ["parametrico", "#FiParam", "parametricos"]].forEach(function (q) {
			var l = itens.filter(function (i) { return i.tipo == q[0] && i.liberado; }).slice(0, 2);
			if (!l.length) l = itens.filter(function (i) { return i.tipo == q[0]; }).slice(0, 2);
			$(q[1]).html(l.length ? l.map(function (i) {
				var d = String(i.descricao || "").split(/(?<=[.!?])\s/)[0];
				return '<a class="FiItem" onclick="FlAbrirAba(\'filuna.parametricos\', \'' + q[2] + '\')"><span class="foto"><img alt="" data-foto="' + FlEsc(i.miniatura || "") + '"></span><span class="txt"><div class="n">' +
					FlEsc(i.nome) + '</div><div class="s"><span>' + FlEsc(d.length > 60 ? d.slice(0, 57) + "…" : d || i.categoria || "") + "</span></div></span></a>";
			}).join("") : '<p class="FiVazio">Nada liberado para a sua conta ainda.</p>');
			$(q[1] + " img[data-foto]").each(function () { fiFoto(this.getAttribute("data-foto"), this); });
		});
	}).catch(function () { });
}

/* ---------- ciclo: impressoras e estoque a cada 10 s enquanto a Início está à vista ---------- */
function fiVisivel() { return !document.hidden && $("#ContentBoard").is(":visible"); }
function fiAoVivo() {
	if (!fiVisivel()) return;
	Promise.all([FlNativo("dispositivos.estado").catch(function () { return null; }), FlNativo("estoque.listar").catch(function () { return null; }),
		FlNativo("historico.listar").catch(function () { return {}; })]).then(function (r) {
		if (r[0]) FI.estado = r[0];
		if (r[1]) FI.bobinas = (r[1].bobinas || []).map(fiBobina);
		FI.giram = fiGiram((r[2] || {}).trabalhos);
		fiImpressoras();
		fiEstoque();
		fiSaudacao();
	});
}
$(function () {
	fiSaudacao();
	fiModelos();
	fiRecursos();
	fiAoVivo();
	setInterval(fiAoVivo, 10000);
	setInterval(fiSaudacao, 60000);
	// entrou ou saiu da conta: os recursos mudam
	var antes = window.FlMostrar;
	if (typeof antes == "function") window.FlMostrar = function (ligado) { var r = antes.apply(this, arguments); setTimeout(function () { fiRecursos(); fiSaudacao(); }, 300); return r; };
});
