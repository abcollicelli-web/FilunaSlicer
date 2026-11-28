/* Filuna — "Seu dia" no alto da tela Início (pedido do dono, 27/11/2026): agenda de hoje, estoque baixo, vendas do
 * Mercado Livre e o que falta enviar. Os dados vêm do site numa chamada só (GET /inicio/resumo, contrato em
 * site-v2/docs/farm.md), com o dia do computador (tz). Consulta ao abrir, a cada 90 s com a Início à vista e a cada
 * 3 min com outra aba na frente; venda nova enquanto o programa está aberto vira aviso discreto e contador no menu.
 * Depende de filuna-home.js (FL_API, FlToken, FlEsc, FlAbrirAba) e de filuna-inicio.js (FlNativo); a bobina é a do
 * Estoque (Filuna.bobinaSVG, ponte.js).
 */
var FD = { api: FL_API, dados: null, vistas: null, novas: 0, ultimo: 0, buscando: false, itens: [] };
var FD_A_CADA = 90000, FD_FORA = 180000;
var FD_TIPOS = { impressao: "Impressão", pedido: "Entrega", envio: "Envio", evento: "Agenda" };
var FD_ENVIO = { ready_to_ship: "Pronto para enviar", handling: "Preparando", pending: "Aguardando", shipped: "Enviado", to_be_agreed: "A combinar" };
var FD_IC = {
	agenda: '<svg viewBox="0 0 24 24" aria-hidden="true"><rect x="3.5" y="5" width="17" height="15" rx="3"/><path d="M3.5 10h17M8 3v4M16 3v4"/></svg>',
	estoque: '<svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="8.5"/><circle cx="12" cy="12" r="3"/><path d="M12 3.5v5.5"/></svg>',
	vendas: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 5h2l2 10h10l2-7H7.2"/><circle cx="9.5" cy="19" r="1.4"/><circle cx="17" cy="19" r="1.4"/></svg>',
	enviar: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3.5 8 12 4l8.5 4v8L12 20l-8.5-4z"/><path d="M3.5 8 12 12l8.5-4M12 12v8"/></svg>'
};

function fdMoeda(v, moeda) {
	try { return new Intl.NumberFormat(FlLocal(), { style: "currency", currency: moeda || (FD.dados && FD.dados.moeda) || "BRL" }).format(+v || 0); }
	catch (e) { return "R$ " + (+v || 0).toFixed(2).replace(".", ","); }
}
function fdAgoraHM() { var d = new Date(); return ("0" + d.getHours()).slice(-2) + ":" + ("0" + d.getMinutes()).slice(-2); }
function fdPrazo(iso) {
	var d = iso ? new Date(/^\d{4}-\d\d-\d\d$/.test(iso) ? iso + "T23:59:00" : iso) : null;   // só a data: vale até o fim do dia
	if (!d || isNaN(d)) return { txt: "", atrasado: false };
	var hm = ("0" + d.getHours()).slice(-2) + ":" + ("0" + d.getMinutes()).slice(-2), hoje = new Date();
	var mesmoDia = d.toDateString() == hoje.toDateString();
	return { txt: (mesmoDia ? "hoje até " : d.toLocaleDateString(FlLocal(), { day: "2-digit", month: "2-digit" }) + " até ") + hm, atrasado: d < hoje };
}

/* ---------- conversa com o site ---------- */
function fdApi(metodo, rota, corpo) {
	return fetch(FD.api + rota, { method: metodo, cache: "no-store", headers: { "Content-Type": "application/json", "Authorization": "Bearer " + FlToken() },
		body: corpo ? JSON.stringify(corpo) : undefined }).then(function (r) {
		return r.json().catch(function () { return {}; }).then(function (d) { if (!r.ok) throw new Error(d.erro || "Falha na conexão com o site (" + r.status + ")."); return d; });
	});
}
function fdExterno(url) { try { window.wx.postMessage(JSON.stringify({ command: "common_openurl", url: url })); } catch (e) { } }
// Página do site já com a conta: o site devolve um link de uso único (2 minutos) e ela abre no navegador.
function fdAbrirUrl(url) {
	if (/^https:\/\//i.test(url)) return fdExterno(url);
	if (url.charAt(0) != "/") return;
	fdApi("POST", "/entrada-web", { volta: url }).then(function (r) { if (r.url) fdExterno(r.url); }).catch(function () { fdExterno(FD.api.replace(/\/api\/slicer\/v1$/, "") + url); });
}
function fdIr(item) {
	if (!item) return;
	if (item.link_aba) FlAbrirAba(item.link_aba, item.secao || "");
	else if (item.url) fdAbrirUrl(item.url);
	else if (item.link) fdAbrirUrl(item.link);
}
function fdEtiqueta(item, botao) {
	var url = String(item.etiqueta_url || "");
	if (!/^\/api\/slicer\/v1\//.test(url)) { if (url) fdAbrirUrl(url); return; }
	$(botao).prop("disabled", true).text("Baixando…");
	fetch(FD.api + url.slice("/api/slicer/v1".length), { headers: { "Authorization": "Bearer " + FlToken() } }).then(function (r) {
		if (!r.ok) return r.json().catch(function () { return {}; }).then(function (d) { throw new Error(d.erro || "A etiqueta ainda não está disponível."); });
		return r.blob().then(function (b) {
			var a = document.createElement("a");
			a.href = URL.createObjectURL(b); a.download = "etiqueta-" + String(item.pedido || "envio").replace(/[^\w-]/g, "") + ".pdf";
			document.body.appendChild(a); a.click(); a.remove();
			$(botao).prop("disabled", false).text("Etiqueta");
		});
	}).catch(function (e) { $(botao).prop("disabled", false).text("Etiqueta").attr("title", e.message); });
}

/* ---------- desenho ---------- */
function fdItem(obj) { FD.itens.push(obj); return FD.itens.length - 1; }
function fdBloco(id, ic, cor, titulo, num, ver, corpo, extra) {
	return '<div class="FiCard FdBloco ' + cor + '" id="' + id + '"><div class="FdCab"><span class="FdIc">' + FD_IC[ic] + "</span><b>" + titulo + "</b>" +
		(num ? '<em class="FdNum">' + num + "</em>" : "") + (extra || "") + (ver ? '<a class="FdVer" data-i="' + fdItem(ver) + '">' + FlEsc(ver.rotulo) + " →</a>" : "") +
		'</div><div class="FdCorpo">' + corpo + "</div></div>";
}
function fdVazio(t) { return '<p class="FdVazio">' + FlEsc(t) + "</p>"; }

function fdAgenda(d) {
	var agora = fdAgoraHM(), l = (d.agenda || []).slice();
	// o que ainda vai acontecer primeiro (o do dia todo junto), o que já passou no fim e mais apagado
	var vem = l.filter(function (a) { return !a.hora || a.hora >= agora; }), foi = l.filter(function (a) { return a.hora && a.hora < agora; });
	var prox = vem.filter(function (a) { return a.hora; })[0];
	var h = vem.concat(foi).slice(0, 3).map(function (a) {
		return '<a class="FdLin t-' + FlEsc(a.tipo) + (a.hora && a.hora < agora ? " passou" : "") + (a === prox ? " prox" : "") + '" data-i="' + fdItem(a) + '" title="' + FlEsc(a.titulo + " · " + (a.detalhe || "")) + '">' +
			'<span class="FdHora">' + FlEsc(a.hora || "hoje") + '</span><span class="FdTx"><b>' + FlEsc(a.titulo) + "</b><small><span>" + FlEsc(FD_TIPOS[a.tipo] || "") + "</span>" + FlEsc(a.detalhe ? " · " + a.detalhe : "") + "</small></span></a>";
	}).join("");
	var total = d.agenda_total || l.length;
	return fdBloco("FdAgenda", "agenda", "violeta", "Agenda de hoje", total ? String(total) : "", { rotulo: "Farm", link_aba: "filuna.farm" },
		h || fdVazio("Nada marcado para hoje. Impressões da Farm, entregas e envios aparecem aqui."));
}

function fdEstoque(d) {
	if (!d.estoque_alerta) return "";
	var l = d.estoque_baixo || [], lim = d.estoque_limite_g || 0;
	var h = l.slice(0, 3).map(function (b) {
		var frac = b.peso_inicial_g ? Math.max(0, Math.min(1, b.restante_g / b.peso_inicial_g)) : null;
		var bob = window.Filuna && Filuna.bobinaSVG ? Filuna.bobinaSVG(b.cor || "#C2185B", 30, frac) : '<i class="FdCor" style="background:' + FlEsc(b.cor) + '"></i>';
		return '<a class="FdLin FdBob' + (b.restante_g < lim / 2 ? " critico" : "") + '" data-i="' + fdItem({ link_aba: "filuna.espaco", secao: "estoque" }) + '" title="' + FlEsc("Limite de estoque baixo: " + lim + " g") + '">' +
			'<span class="FdBobImg">' + bob + '</span><span class="FdTx"><b>' + FlEsc((b.material || "") + " " + (b.cor_nome || "")) + "</b><small>" +
			FlEsc((b.marca ? b.marca + " · " : "") + (b.bobina || "") + " · ") + "<span>" + FlEsc(b.na_impressora ? "na " + b.na_impressora : "guardada") + "</span>" + '</small></span><span class="FdG">' + FlEsc(FlGramas(b.restante_g)) + "</span></a>";
	}).join("");
	return fdBloco("FdEstoque", "estoque", "laranja", "Estoque baixo", l.length ? String(d.estoque_baixo_total || l.length) : "", { rotulo: "Estoque", link_aba: "filuna.espaco", secao: "estoque" },
		h || fdVazio("Todas as bobinas acima de " + lim + " g."));
}

function fdVendas(d) {
	var v = d.vendas || {}, ver = { rotulo: "Vendas", link_aba: "filuna.loja", secao: "loja:vendas" };
	if (!v.conectado)
		return fdBloco("FdVendas", "vendas", "magenta", "Mercado Livre", "", null,
			'<div class="FdConvite"><p>Veja aqui as vendas do dia, as perguntas e o que falta enviar.</p><a class="FdBtn" data-i="' + fdItem({ link_aba: "filuna.loja", secao: "loja:lojas" }) + '">Conecte seu Mercado Livre</a></div>');
	if (v.indisponivel && !(v.vendas_hoje || []).length)
		return fdBloco("FdVendas", "vendas", "magenta", "Mercado Livre", "", ver, fdVazio("O Mercado Livre não respondeu agora. Tentamos de novo em instantes."));
	var l = v.vendas_hoje || [], soma = l.reduce(function (s, x) { return s + (+x.valor || 0); }, 0);
	var nova = FD.novas ? '<span class="FdNova">' + FD.novas + (FD.novas == 1 ? " nova" : " novas") + "</span>" : "";
	var h = l.length ? '<div class="FdSoma"><b>' + FlEsc(fdMoeda(soma, l[0].moeda)) + "</b><span>" + l.length + (l.length == 1 ? " venda hoje" : " vendas hoje") + "</span></div>" : "";
	var chips = "";
	if (v.perguntas_n) chips += '<a class="FdChip" data-i="' + fdItem({ link_aba: "filuna.loja", secao: "loja:perguntas" }) + '">' + v.perguntas_n + (v.perguntas_n == 1 ? " pergunta sem resposta" : " perguntas sem resposta") + "</a>";
	if (v.reclamacoes_n) chips += '<a class="FdChip alerta" data-i="' + fdItem(ver) + '">' + v.reclamacoes_n + (v.reclamacoes_n == 1 ? " reclamação" : " reclamações") + "</a>";
	// com perguntas ou reclamações à mostra cabe uma venda; sem elas, duas
	h += l.slice(0, chips ? 1 : 2).map(function (x) {
		return '<a class="FdLin FdVenda" data-i="' + fdItem(ver) + '" title="' + FlEsc(x.titulo) + '"><span class="FdFoto">' + (x.foto ? '<img src="' + FlEsc(x.foto) + '" alt="" loading="lazy">' : FD_IC.vendas) +
			'</span><span class="FdTx"><b>' + FlEsc(x.titulo || "Venda") + "</b><small>" + FlEsc(x.comprador || "") + '</small></span><span class="FdG">' + FlEsc(fdMoeda(x.valor, x.moeda)) + "</span></a>";
	}).join("");
	if (!l.length) h = fdVazio("Nenhuma venda ainda hoje.");
	return fdBloco("FdVendas", "vendas", "magenta", "Mercado Livre", "", ver, h + (chips ? '<div class="FdChips">' + chips + "</div>" : ""), nova);
}

function fdEnviar(d) {
	var l = d.a_enviar || [];
	var h = l.slice(0, 3).map(function (e) {
		var p = fdPrazo(e.prazo), k = fdItem(e);
		var sub = (e.origem == "mercadolivre" ? "Mercado Livre" : "Minha loja") + (e.pedido ? " · " + e.pedido : "") + " · " +
			(p.txt ? (p.atrasado ? "atrasado, era " : "") + p.txt : (FD_ENVIO[e.status_envio] || e.status_envio || ""));
		return '<a class="FdLin FdEnvio' + (p.atrasado ? " atrasado" : "") + '" data-i="' + k + '" title="' + FlEsc(e.titulo) + '"><span class="FdFoto">' +
			(e.foto ? '<img src="' + FlEsc(e.foto) + '" alt="" loading="lazy">' : FD_IC.enviar) + '</span><span class="FdTx"><b>' + FlEsc(e.titulo) + "</b><small>" + FlEsc(sub) + "</small></span>" +
			(e.etiqueta_url ? '<button type="button" class="FdEtq" data-etq="' + k + '">Etiqueta</button>' : '<span class="FdSeta">›</span>') + "</a>";
	}).join("");
	var total = d.a_enviar_total || l.length;
	return fdBloco("FdEnviar", "enviar", "violeta", "O que falta enviar", total ? String(total) : "", (d.vendas || {}).conectado ? { rotulo: "Pedidos", link_aba: "filuna.loja", secao: "loja:vendas" } : null,
		h || fdVazio("Nada esperando envio. Pedidos pagos aparecem aqui com o prazo."));
}

function fdDesenhar() {
	var d = FD.dados, el = $("#FdDia");
	if (!FlToken()) { el.prop("hidden", true); $("#FlInicio").removeClass("comDia"); return; }
	el.prop("hidden", false); $("#FlInicio").addClass("comDia");
	if (!d) { el.find(".FdGrade").html('<p class="FdVazio">Carregando o seu dia…</p>'); return; }
	FD.itens = [];
	var dt = new Date(d.hoje + "T12:00:00");
	$("#FdData").text(isNaN(dt) ? "" : dt.toLocaleDateString(FlLocal(), { weekday: "long", day: "numeric", month: "long" }));
	$("#FdHora").text("atualizado às " + fdAgoraHM());
	el.find(".FdGrade").html(fdAgenda(d) + fdEstoque(d) + fdVendas(d) + fdEnviar(d));
	el.find("img").on("error", function () { $(this).replaceWith(FD_IC.vendas); });
}

/* ---------- venda nova enquanto o programa está aberto ---------- */
function fdConferirVendas(d) {
	var l = ((d.vendas || {}).vendas_hoje || []).map(function (v) { return String(v.pedido || ""); }).filter(Boolean);
	if (FD.vistas === null) { FD.vistas = {}; l.forEach(function (p) { FD.vistas[p] = true; }); return; }   // ao abrir: o que já havia não é novidade
	var novas = ((d.vendas || {}).vendas_hoje || []).filter(function (v) { return v.pedido && !FD.vistas[String(v.pedido)]; });
	novas.forEach(function (v) { FD.vistas[String(v.pedido)] = true; });
	if (!novas.length) return;
	FD.novas += novas.length;
	$("#FlMenuLoja").text(FD.novas).prop("hidden", false);
	var v = novas[0];
	$("#FdAviso").remove();
	$("body").append('<div id="FdAviso" role="status"><span class="FdFoto">' + (v.foto ? '<img src="' + FlEsc(v.foto) + '" alt="">' : FD_IC.vendas) + '</span><span class="FdTx"><b>' +
		(novas.length > 1 ? novas.length + " vendas novas no Mercado Livre" : "Venda nova no Mercado Livre") + "</b><small>" + FlEsc((v.titulo || "") + " · " + fdMoeda(v.valor, v.moeda)) +
		'</small></span><button type="button" aria-label="Fechar">×</button></div>');
	var some = setTimeout(function () { $("#FdAviso").fadeOut(400, function () { $(this).remove(); }); }, 12000);
	$("#FdAviso").on("click", function (ev) {
		clearTimeout(some); $(this).remove();
		if (!$(ev.target).is("button")) { fdLimparNovas(); FlAbrirAba("filuna.loja", "loja:vendas"); }
	});
}
function fdLimparNovas() { FD.novas = 0; $("#FlMenuLoja").prop("hidden", true); $("#FdVendas .FdNova").remove(); }

/* ---------- ciclo ---------- */
function fdVisivel() { return !document.hidden && $("#ContentBoard").is(":visible"); }
function fdCarregar(forcar) {
	if (!FlToken()) { FD.dados = null; FD.vistas = null; fdDesenhar(); return; }
	if (FD.buscando || (!forcar && Date.now() - FD.ultimo < (fdVisivel() ? FD_A_CADA : FD_FORA))) return;
	FD.buscando = true;
	if (!FD.dados) fdDesenhar();
	fdApi("GET", "/inicio/resumo?tz=" + new Date().getTimezoneOffset()).then(function (d) {
		FD.dados = d; FD.ultimo = Date.now();
		fdConferirVendas(d);
		fdDesenhar();
	}).catch(function () {
		FD.ultimo = Date.now() - FD_A_CADA + 30000;   // sem internet: tenta de novo em 30 s, sem apagar o que já está na tela
		if (!FD.dados) $("#FdDia").prop("hidden", true).parent().removeClass("comDia");
	}).then(function () { FD.buscando = false; });
}

$(function () {
	$(document).on("click", "#FdDia [data-i]", function (ev) {
		var b = $(ev.target).closest("[data-etq]");
		if (b.length) { ev.stopPropagation(); fdEtiqueta(FD.itens[+b.attr("data-etq")], b[0]); return; }
		if ($(this).closest("#FdVendas").length) fdLimparNovas();
		fdIr(FD.itens[+$(this).attr("data-i")]);
	});
	$("#FlMenuLoja").parent().on("click", fdLimparNovas);
	// a tela de teste do programa (FILUNA_SITE_TESTE) troca o site dos dados; programa antigo não conhece a ação e fica no de verdade
	if (typeof FlNativo == "function")
		FlNativo("site.base").then(function (r) { if (r && r.base && r.base + "/api/slicer/v1" != FD.api) { FD.api = r.base + "/api/slicer/v1"; fdCarregar(true); } }).catch(function () { });
	fdCarregar(true);
	setInterval(function () { fdCarregar(false); }, 15000);
	document.addEventListener("visibilitychange", function () { if (!document.hidden && Date.now() - FD.ultimo > 30000) fdCarregar(true); });
	// entrou ou saiu da conta: o dia é outro
	var antes = window.FlMostrar;
	if (typeof antes == "function") window.FlMostrar = function (ligado) {
		var r = antes.apply(this, arguments);
		if (!ligado) { FD.dados = null; FD.vistas = null; fdLimparNovas(); fdDesenhar(); } else if (!FD.dados) setTimeout(function () { fdCarregar(true); }, 300);
		return r;
	};
});
