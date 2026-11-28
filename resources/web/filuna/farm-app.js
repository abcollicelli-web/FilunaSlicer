/* Filuna Farm no Slicer — telas iguais às do site (templates farm/*.html do branch site-v2/farm-wizard, portados), rodando aqui.
 * Projeto feito no Slicer: tudo local (arquivo aberto e analisado com o farm_arquivos.py do site no Python do programa,
 * medido fatiando com os perfis da impressora, plano com o farm_plano portado e conferido) e só no fim salvo no site
 * para acompanhar pelo site e pelo app. Pedido feito no site/app: o site planeja; aqui se vê e se age sobre os dados. */
(function () {
"use strict";
var FP = window.FarmPlano, W = window.FarmWizard, E = function (t) { return Filuna.esc(t == null ? "" : String(t)); };
var tela = document.getElementById("tela");
var st = { rota: "/farm", dados: null, local: null, fotos: {}, recarga: 0 };

// ---------------- filtros do site ----------------
var horas = function (s) { return FP.duracao(s); }, g = function (x) { return FP.gramas(x || 0); }, brl = function (v) { return FP.reais(v || 0); };
var cm = function (mm) { return FP.cm(mm); };
function quando(s) { return s && s.length >= 16 ? s.slice(8, 10) + "/" + s.slice(5, 7) + " " + s.slice(11, 16) : ""; }
function pl_(n, um, varios) { return n == 1 ? um : varios; }
function ou(lista) { return lista.length < 2 ? (lista[0] || "") : lista.slice(0, -1).join(", ") + " ou " + lista[lista.length - 1]; }
function unicos(l) { var s = []; l.forEach(function (x) { if (s.indexOf(x) < 0) s.push(x); }); return s; }

// ---------------- macros (farm/_m.html e _w.html) ----------------
function cor(hex, nome) { return '<span class="fm-cor" style="background:' + E(hex || "#EFE9DF") + '"' + (nome ? ' title="' + E(nome) + '"' : "") + "></span>"; }
function foto(pid, hex, gr) {
	var src = pid && st.fotos[pid];
	return '<span class="fm-foto' + (gr ? " g" : "") + '">' + (pid ? '<img data-foto="' + E(pid) + '"' + (src ? ' src="' + src + '"' : "") + ' alt="Foto da peça">'
		: '<span class="fm-foto-vazia" style="background:' + E(hex || "#D9D1C5") + '"></span>') + "</span>";
}
function botao(acao, rotulo, campos, classe, volta) {
	var h = '<form method="post" class="fm-inline"><input type="hidden" name="acao" value="' + E(acao) + '">' + (volta ? '<input type="hidden" name="volta" value="' + E(volta) + '">' : "");
	for (var k in (campos || {})) h += '<input type="hidden" name="' + E(k) + '" value="' + E(campos[k]) + '">';
	return h + '<button class="' + E(classe || "btn btn-line btn-sm") + '">' + E(rotulo) + "</button></form>";
}
function estado_imp(i, estados) {
	var c = { imprimindo: "mag", limpar_mesa: "viol", pausada: "warn", offline: "" }[i.estado]; if (c === undefined) c = "ok";
	return '<span class="pilula ' + c + '">' + E((estados || {})[i.estado] || i.estado) + "</span>";
}
function estado_trab(t, bloq) {
	if (t.estado == "bloqueado") return '<span class="pilula warn">' + E((bloq || {})[t.bloqueio] || "Aguardando correção") + "</span>";
	if (t.estado == "imprimindo") return '<span class="pilula mag">Imprimindo ' + Math.round(t.progresso || 0) + "%</span>";
	return { concluido: '<span class="pilula ok">Concluída</span>', falhou: '<span class="pilula warn">Falhou</span>', cancelado: '<span class="pilula">Cancelada</span>',
		enviado: '<span class="pilula mag">Enviada ao Slicer</span>' }[t.estado] || '<span class="pilula">Na fila</span>';
}
function acao_correcao(a, volta) {
	if (!a) return "";
	if (a.tipo == "link") return '<a class="btn btn-ink btn-sm" href="' + E(a.href) + '">' + E(a.rotulo) + "</a>";
	if (a.tipo == "carregar") return botao("carregar", a.rotulo, { impressora_id: a.impressora_id, posicao: a.posicao, bobina_id: a.bobina_id }, "btn btn-ink btn-sm", volta);
	return "";
}
function sem_plano(lim) {
	return '<section class="bloco"><h2>A Filuna Farm faz parte dos planos Maker e Negócio</h2><p class="fm-sub">Seu plano atual é ' + E(lim.plano) +
		'. No Maker você organiza até 3 impressoras; no Negócio, até 50.</p><p style="margin-top:16px"><a class="btn btn-mag btn-sm" href="/conta/plano">Ver planos</a></p></section>';
}
function etapas(passos, passo, alcancado, pid, enviado) {
	var h = '<nav class="fw-etapas" aria-label="Etapas do pedido">';
	passos.forEach(function (t, i) {
		var n = i + 1, feito = n < passo || (enviado && n < 6), pode = pid && n >= 2 && n <= alcancado && !(enviado && n < 6) && n != passo;
		var dentro = '<span class="fw-n">' + (feito ? "✓" : n) + "</span><span><b>" + E(t[0]) + "</b><small>" + E(t[1]) + "</small></span>";
		h += pode ? '<a class="fw-etapa' + (feito ? " feito" : "") + '" href="/farm/pedidos/' + E(pid) + "?passo=" + n + '">' + dentro + "</a>"
			: '<span class="fw-etapa' + (feito ? " feito" : "") + (n == passo ? " atual" : "") + (n > alcancado && n != passo ? " bloq" : "") + '">' + dentro + "</span>";
	});
	return h + "</nav>";
}
function linha(lt, agora) {
	var h = '<div class="fw-gantt"><div class="fw-g-in"><div class="fw-g-eixo"><span>Impressora</span><div class="fw-g-marcas">';
	lt.marcas.forEach(function (mk) { h += '<span style="left:' + mk.esq + '%">' + E(mk.rotulo) + "</span>"; });
	h += '</div><span class="fw-g-fim">fim</span></div>';
	lt.lanes.forEach(function (l) {
		h += '<div class="fw-g-lin"><div class="fw-g-nome"><b>' + E(l.nome) + "</b><small>" + E(l.modelo) + '</small></div><div class="fw-g-trilha">';
		l.blocos.forEach(function (b) {
			if (b.tipo == "mesa") h += '<div class="fw-g-b' + (b.tinta != "#fff" ? " claro" : "") + '" style="left:' + b.esq + "%;width:" + b.larg + "%;--c:" + E(b.cor || "#D9D1C5") + '" title="' + E(b.rotulo + " · " + b.tempo) + '"><span>' + E(b.rotulo + " · " + b.tempo) + "</span></div>";
			else if (b.tipo == "limpar") h += '<div class="fw-g-b limpar" style="left:' + b.esq + "%;width:" + b.larg + '%" title="Limpar a mesa e confirmar"></div>';
			else h += '<div class="fw-g-b outro" style="left:' + b.esq + "%;width:" + b.larg + '%" title="Terminando outro pedido"><span>outro pedido</span></div>';
		});
		h += (agora ? '<i class="fw-g-agora"></i>' : "") + '</div><div class="fw-g-fim">' + horas(l.t) + "</div></div>";
	});
	return h + '</div></div><div class="fw-legenda"><span><i style="background:#8E8E8E"></i>mesa, na cor da peça</span><span><i class="limpar"></i>limpar a mesa (espera sua confirmação)</span><span><i class="outro"></i>impressora terminando outro pedido</span></div>';
}
var ICO = { erro: '<svg viewBox="0 0 24 24"><path d="M12 3 2 20h20L12 3Z" fill="currentColor"/><path d="M12 9v5M12 17.2v.3" stroke="#fff" stroke-width="2.2" stroke-linecap="round"/></svg>',
	filamento: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" stroke-width="2.2"/><circle cx="12" cy="12" r="3.2" fill="currentColor"/><path d="M12 3v3M21 12h-3" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/></svg>',
	ausente: '<svg viewBox="0 0 24 24"><rect x="3.5" y="3.5" width="17" height="17" rx="4" fill="none" stroke="currentColor" stroke-width="2.2" stroke-dasharray="3.5 2.5"/><path d="M9.6 9.4a2.5 2.5 0 1 1 3.4 2.3c-.7.3-1 .8-1 1.5v.4M12 16.6v.2" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"/></svg>',
	limpar: '<svg viewBox="0 0 24 24"><rect x="3" y="14" width="18" height="5" rx="1.5" fill="currentColor"/><path d="m8 9 3 3 6-7" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"/></svg>',
	atencao: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="10" fill="currentColor"/><path d="M12 7v6M12 16.5v.3" stroke="#fff" stroke-width="2.2" stroke-linecap="round"/></svg>' };
function icone(tipo) { var t = { falhou: "erro", filamento: "filamento", estoque_baixo: "filamento", falta_cor: "ausente", limpar: "limpar" }[tipo] || "atencao"; return '<span class="fd-ico ' + t + '" aria-hidden="true">' + ICO[t] + "</span>"; }
function situacao(s) { return { pronta: '<span class="pilula ok">Pronta</span>', imprimindo: '<span class="pilula mag">Imprimindo</span>', ausente: '<span class="pilula warn">Peça ausente</span>', falhou: '<span class="pilula fd-erro">Erro</span>', fila: '<span class="pilula">Na fila</span>' }[s] || ""; }
function pq(exp, k) { return exp && exp[k] ? '<div class="fm-pq"><p><b>Por quê:</b> ' + E(exp[k][0]) + '</p><p class="res"><b>Resultado:</b> ' + E(exp[k][1]) + "</p></div>" : ""; }

// ---------------- telas do site ----------------
function vPainel(d) {
	var lim = d.lim, kpi = d.kpi, h = '<div class="cab-pag"><div><h1>Farm</h1><p>Plano ' + E(lim.plano) + " · " + kpi.total + " de " + lim.farm_impressoras + " impressora" + pl_(lim.farm_impressoras, "", "s") +
		' · Filuna Slicer ligado</p></div>' + (lim.farm_impressoras ? '<a class="btn btn-mag" href="/farm/novo">+ Adicionar projeto</a>' : "") + "</div>";
	if (!lim.farm_impressoras) return h + sem_plano(lim);
	var pj = d.projetos || [], gasto = 0, previsto = 0, ausentes = 0, resta = 0;
	pj.forEach(function (p) { gasto += p.gasto_g || 0; previsto += p.previsto_g || 0; ausentes += p.ausentes || 0; resta = Math.max(resta, p.resta_s || 0); });
	var av = d.avisos || [], falhou = av.filter(function (a) { return a.tipo == "falhou"; });
	h += '<div class="fm-kpis fd-kpis"><div class="fm-kpi"><span>Imprimindo agora</span><b>' + kpi.imprimindo + "<small> de " + kpi.total + "</small></b><small>impressora" + pl_(kpi.total, "", "s") + "</small></div>" +
		'<div class="fm-kpi"><span>Tempo restante</span><b>' + (resta ? horas(resta) : "—") + "</b><small>" + pj.length + " projeto" + pl_(pj.length, "", "s") + " na fila</small></div>" +
		'<div class="fm-kpi"><span>Material gasto</span><b>' + (gasto ? g(gasto) : "0 g") + "</b><small>de " + (previsto ? g(previsto) : "0 g") + " previstos</small></div>" +
		'<div class="fm-kpi"><span>Peças ausentes</span><b class="' + (ausentes ? "fd-warn" : "") + '">' + ausentes + "</b><small>esperando cor ou filamento</small></div>" +
		'<div class="fm-kpi"><span>Alertas</span><b class="fd-kpi-ico">' + (av.length ? icone((falhou[0] || av[0]).tipo) : "") + av.length + "</b><small>" + (av.length ? "precisam de você" : "nada pendente") + "</small></div></div>";
	if (av.length) {
		h += '<section class="fd-alertas" aria-label="Alertas">';
		av.forEach(function (a) {
			h += '<div class="fd-alerta">' + icone(a.tipo) + '<div class="fd-alerta-txt"><b>' + E(a.titulo) + "</b><p>" + E(a.texto) + '</p><div class="fm-acoes">';
			if (a.tipo == "limpar") h += botao("mesa_limpa", "Mesa limpa, enviar a próxima", { impressora_id: a.impressora_id }, "btn btn-ink btn-sm");
			else if (a.tipo == "falhou") h += botao("repetir", "Repetir esta mesa", { trabalho_id: (a.trabalho || {}).id }, "btn btn-ink btn-sm");
			else if (a.tipo == "falta_cor" && a.pedido_id) h += '<a class="btn btn-ink btn-sm" href="/farm/pedidos/' + E(a.pedido_id) + "?passo=6&grupo=" + E(((a.trabalho || {}).cor_hex || "#000000").replace("#", "").toUpperCase()) + '">Imprimir agora</a>';
			else if (a.impressora_id) h += '<a class="btn btn-ink btn-sm" href="/farm/impressoras/' + E(a.impressora_id) + '">Trocar bobina</a>';
			if (a.pedido_id) h += '<a class="btn btn-line btn-sm" href="/farm/pedidos/' + E(a.pedido_id) + '">Ver pedido</a>';
			h += "</div></div></div>";
		});
		h += "</section>";
	}
	h += '<div class="fm-duas"><div class="fm-principal"><div class="fm-tit"><h2>Projetos em impressão</h2><a href="/farm/novo">Adicionar projeto</a></div>';
	if (!pj.length) h += '<section class="bloco fd-vazio"><b>Nenhum projeto imprimindo agora.</b><p>Adicione um projeto: o Filuna lê o arquivo, faz as perguntas e divide entre as impressoras.</p><a class="btn btn-mag btn-sm" href="/farm/novo">+ Adicionar projeto</a></section>';
	pj.forEach(function (p) {
		h += '<article class="bloco fd-proj"><div class="fd-proj-cab"><div><a href="/farm/pedidos/' + E(p.id) + '"><b>#' + E(p.numero) + " · " + E(p.nome) + "</b></a><small>" + E(p.arquivo) + " · enviado " + quando(p.criado_em) + '</small></div><div class="fd-pilulas">' +
			(p.erros ? '<span class="pilula fd-erro">' + icone("falhou") + p.erros + " erro" + pl_(p.erros, "", "s") + "</span>" : "") + (p.ausentes ? '<span class="pilula warn">' + p.ausentes + " ausente" + pl_(p.ausentes, "", "s") + "</span>" : "") +
			'<a class="btn btn-line btn-sm" href="/farm/pedidos/' + E(p.id) + '">Abrir</a></div></div><div class="fd-prog"><div class="fm-barra"><i style="width:' + p.pct + '%"></i></div><b>' + p.pct + "%</b></div>" +
			'<div class="fd-nums"><div><span>Decorrido</span><b>' + (p.decorrido_s ? horas(p.decorrido_s) : "ainda não começou") + "</b></div><div><span>Restante</span><b>" + (p.resta_s ? horas(p.resta_s) : "—") + "</b></div>" +
			"<div><span>Material gasto</span><b>" + g(p.gasto_g) + " <small>de " + g(p.previsto_g) + "</small></b></div><div><span>Peças prontas</span><b>" + p.prontas + " <small>de " + p.total + '</small></b></div></div><div class="fd-pecas">';
		(p.pecas || []).forEach(function (pc) {
			h += '<div class="fd-peca s-' + E(pc.situacao) + '">' + foto(pc.peca_id, pc.cor) + '<div class="fd-peca-txt"><div class="fd-peca-cab"><b>' + E(pc.nome) + " ×" + pc.total + "</b>" + situacao(pc.situacao) + "</div><small>" + cor(pc.cor) + " " + E(pc.material + " " + pc.cor_nome) + " · " + pc.prontas + " de " + pc.total + " pronta" + pl_(pc.total, "", "s") + "</small>" +
				(pc.situacao == "ausente" ? '<a class="fd-link fd-agora" href="/farm/pedidos/' + E(p.id) + "?passo=6&grupo=" + E((pc.cor || "#000000").replace("#", "").toUpperCase()) + '">Imprimir agora</a>' : "") + "<ul>";
			(pc.mesas || []).forEach(function (t) {
				h += "<li>mesa " + t.numero + "/" + t.total + " · " + E(t.imp_nome || "sem impressora") + " · " +
					(t.estado == "imprimindo" ? "<b>" + Math.round(t.progresso || 0) + "%</b>, resta " + horas(t.resta_s || 0) : t.estado == "concluido" ? "pronta" :
						t.estado == "bloqueado" ? '<b class="fd-warn">' + E((d.bloqueios || {})[t.bloqueio] || "aguardando correção") + "</b>" :
						t.estado == "falhou" ? '<b class="fd-erro-t">falhou</b> ' + botao("repetir", "Repetir", { trabalho_id: t.id }, "fd-link") : t.estado == "enviado" ? "enviada ao Slicer" : "na fila") + "</li>";
			});
			h += "</ul></div></div>";
		});
		h += "</div></article>";
	});
	h += '<div class="fm-tit" style="margin-top:8px"><h2>Impressoras</h2><a href="/farm/impressoras">Cadastrar ou editar</a></div>';
	if (!d.imps.length) h += '<section class="bloco"><p class="vazio">Nenhuma impressora ainda. <a href="/farm/impressoras">Cadastre a primeira</a> ou conecte a impressora no Dispositivo para ela entrar sozinha.</p></section>';
	h += '<div class="fm-grade">';
	d.imps.forEach(function (i) {
		var t = i.trabalho;
		h += '<article class="fm-imp' + (i.estado == "pausada" || (t && t.estado == "falhou") ? " fd-imp-erro" : "") + '"><a class="fm-imp-link" href="/farm/impressoras/' + E(i.id) + '"><div class="fm-imp-cab"><div><b>' + E(i.nome) + "</b><small>" + E(i.modelo_nome || "Impressora") + " · mesa " + cm(i.mesa_w) + " × " + cm(i.mesa_d) + " cm</small></div>" + estado_imp(i, d.estados) + "</div>" +
			'<div class="fm-job">' + foto(t ? t.peca_id : null, t ? t.cor_hex : "") + "<div>" +
			(t ? "<b>" + E(t.descricao) + "</b><small>mesa " + t.numero + " de " + t.total + " · " + t.quantidade + " peça" + pl_(t.quantidade, "", "s") + " · pedido #" + E(t.pedido_numero) + "</small>"
				: i.na_fila ? "<b>" + i.na_fila + " mesa" + pl_(i.na_fila, "", "s") + " esperando</b><small>" + horas(i.carga_s) + " de impressão</small>" : "<b>Sem trabalho</b><small>Nenhuma peça dessa cor na fila</small>") + "</div></div>";
		if (t && (i.estado == "imprimindo" || i.estado == "pausada")) h += '<div class="fm-barra"><i style="width:' + Math.round(t.progresso || 0) + '%"></i></div><div class="fm-prog"><span>' + Math.round(t.progresso || 0) + "% concluído</span><span>resta " + horas(t.resta_s || 0) + "</span></div>";
		h += '<div class="fm-slots">';
		(i.slots || []).forEach(function (s) { h += '<div class="fm-slot">' + cor(s.cor_hex) + "<div><b>" + E(s.material ? s.material + " " + (s.cor_nome || "") : "Sem bobina") + "</b><small>" + (s.bobina_id ? "bobina nº " + E(s.bobina_numero) + " · restam " + g(s.restante_g) : s.material ? "bobina não cadastrada no estoque" : "posição " + s.posicao + " vazia") + "</small></div></div>"; });
		h += '</div><span class="fm-abrir">Abrir impressora · bobinas e controle</span></a>' + (i.estado == "limpar_mesa" ? botao("mesa_limpa", "Mesa limpa", { impressora_id: i.id }, "btn btn-ink btn-sm btn-block") : "") + "</article>";
	});
	h += '</div><details class="bloco fd-fila"><summary><b>Fila separada por cor</b><span>' + kpi.mesas + " mesa" + pl_(kpi.mesas, "", "s") + " · " + kpi.pecas + " peça" + pl_(kpi.pecas, "", "s") + " · prazo " + (kpi.prazo_s ? horas(kpi.prazo_s) : "—") +
		'</span></summary><div class="fm-rola"><table class="tab"><tr><th>Cor e material</th><th>Peça · pedido</th><th>Peças</th><th>Mesas</th><th>Impressora</th><th>Filamento</th><th>Tempo</th></tr>';
	(d.fila || []).forEach(function (q) {
		h += '<tr><td><span class="fm-cornome">' + cor(q.cor_hex) + "<b>" + E(q.material + " " + (q.cor_nome || "")) + '</b></span></td><td><a class="fm-peca" href="/farm/pedidos/' + E(q.pedido_id) + '">' + foto(q.peca_id, q.cor_hex, true) + "<span>" + E(q.descricao) + "<small>Pedido #" + E(q.pedido_numero) + " · " + E(q.arquivo) + "</small></span></a></td><td>" + q.pecas + "</td><td>" + q.mesas + "</td><td>" + E(q.impressoras || "—") +
			"</td><td>" + (q.bloqueadas ? '<span class="pilula warn">' + E((d.bloqueios || {})[q.bloqueio] || "Aguardando") + "</span>" : q.gramas ? g(q.gramas) : "a medir") + "</td><td><b>" + (q.tempo_s ? horas(q.tempo_s) : "a medir") + "</b></td></tr>";
	});
	if (!(d.fila || []).length) h += '<tr><td colspan="7" class="vazio">Fila vazia.</td></tr>';
	h += '</table></div></details></div><aside class="fm-lado">';
	if ((d.pedidos || []).length) {
		h += '<section class="bloco"><h2 class="fw-h">Projetos esperando suas respostas</h2>';
		d.pedidos.forEach(function (p) { h += '<a class="fd-linha" href="/farm/pedidos/' + E(p.id) + '"><span><b>#' + E(p.numero) + " · " + E(p.nome) + "</b><small>" + E({ questionario: "responder as perguntas", calculando: "medindo tempo e filamento", plano: "conferir e enviar" }[p.estado] || "") + '</small></span><span class="fd-seta">→</span></a>'; });
		h += "</section>";
	}
	h += '<section class="bloco"><div class="fm-tit"><h2>Projetos finalizados</h2></div>';
	(d.finalizados || []).forEach(function (p) { h += '<div class="fd-fim">' + foto(p.peca_id, p.cor, true) + "<div><b>#" + E(p.numero) + " · " + E(p.nome) + "</b><small>" + quando(p.atualizado_em) + " · " + p.pecas + " peça" + pl_(p.pecas, "", "s") + " · " + (p.tempo_s ? horas(p.tempo_s) : "—") + " · " + (p.gasto_g ? g(p.gasto_g) : "—") + '</small></div><a class="btn btn-line btn-sm" href="/farm/novo?de=' + E(p.id) + '">Imprimir de novo</a></div>'; });
	if (!(d.finalizados || []).length) h += '<p class="vazio">Os projetos concluídos aparecem aqui para imprimir de novo.</p>';
	h += '</section><section class="bloco"><div class="fm-tit"><h2>Estoque de filamento</h2><a href="/farm/estoque">Ver bobinas</a></div>';
	(d.bobinas || []).forEach(function (b) { h += '<div class="fm-bob">' + cor(b.cor_hex) + "<div><b>" + E(b.material + " " + (b.cor_nome || "") + " · nº " + b.numero) + "</b><small>" + E(b.na_impressora ? "na " + b.na_impressora : "guardada") + (b.nova ? " · nova" : "") + '</small></div><div class="fm-bob-g"><b>' + g(b.restante_g) + "</b>" + (b.reservado_g > b.restante_g ? '<small class="warn">insuficiente</small>' : b.reservado_g ? "<small>" + g(b.reservado_g) + " na fila</small>" : "") + "</div></div>"; });
	if (!(d.bobinas || []).length) h += '<p class="vazio">Nenhuma bobina. <a href="/farm/estoque">Cadastre a primeira</a>.</p>';
	return h + '<p class="fm-nota">A baixa é automática: ao fim de cada impressão o sistema desconta da bobina o filamento usado e a purga.</p></section></aside></div>';
}

function vImpressoras(d) {
	var h = '<div class="cab-pag"><div><h1>Impressoras</h1><p>' + d.imps.length + " de " + d.lim.farm_impressoras + " no plano " + E(d.lim.plano) + '. O Filuna Slicer atualiza estado e cores sozinho.</p></div></div>';
	if (!d.lim.farm_impressoras) return h + sem_plano(d.lim);
	if (d.erro) h += '<div class="alerta erro">' + E(d.erro) + "</div>";
	h += '<section class="bloco" style="padding:8px 24px"><table class="tab"><tr><th>Impressora</th><th>Mesa</th><th>Cores carregadas</th><th>Estado</th><th>Fila</th></tr>';
	d.imps.forEach(function (i) {
		h += '<tr><td><a href="/farm/impressoras/' + E(i.id) + '"><b>' + E(i.nome) + '</b></a><small class="fm-bloco">' + E(i.modelo_nome || "") + (i.origem == "slicer" ? " · pelo Slicer" : "") + "</small></td><td>" + cm(i.mesa_w) + " × " + cm(i.mesa_d) + " × " + cm(i.mesa_h) + " cm</td><td>" +
			(i.slots || []).map(function (s) { return '<span class="fm-cornome">' + cor(s.cor_hex) + E(s.material ? s.material + " " + (s.cor_nome || "") : "vazia") + "</span>"; }).join(" ") + "</td><td>" + estado_imp(i, d.estados) + "</td><td>" + i.na_fila + " mesa" + pl_(i.na_fila, "", "s") + "</td></tr>";
	});
	if (!d.imps.length) h += '<tr><td colspan="5" class="vazio">Nenhuma impressora cadastrada.</td></tr>';
	h += '</table></section><section class="bloco"><h2>Cadastrar impressora</h2><form method="post" class="fm-form"><input type="hidden" name="acao" value="adicionar">' +
		'<label class="campo"><span>Nome (como você chama)</span><input name="nome" maxlength="40" placeholder="Ex.: A1-02" required></label><label class="campo"><span>Modelo</span><select name="modelo"><option value="">Outra (informar a mesa)</option>' +
		(d.catalogo || []).map(function (c) { return '<option value="' + E(c.id) + '">' + E(c.nome) + " · mesa " + cm(c.mesa.w) + " × " + cm(c.mesa.d) + " cm</option>"; }).join("") + "</select></label>" +
		'<div class="fm-outra"><label class="campo"><span>Nome do modelo</span><input name="modelo_nome" maxlength="60"></label><label class="campo"><span>Largura da mesa (cm)</span><input name="mesa_w" inputmode="decimal" placeholder="25,6"></label>' +
		'<label class="campo"><span>Profundidade (cm)</span><input name="mesa_d" inputmode="decimal" placeholder="25,6"></label><label class="campo"><span>Altura (cm)</span><input name="mesa_h" inputmode="decimal" placeholder="25,6"></label></div>' +
		'<label class="campo"><span>Cores</span><select name="multicor">' + Object.keys(d.multicor || {}).map(function (k) { return '<option value="' + E(k) + '">' + E(d.multicor[k]) + "</option>"; }).join("") + "</select></label>" +
		'<label class="campo"><span>Quantas cores ao mesmo tempo</span><input type="number" name="cores" min="2" max="16" value="4"></label><div class="fm-form-fim"><button class="btn btn-ink">Cadastrar</button></div></form>' +
		'<p class="fm-nota">Depois de cadastrar, abra a impressora e escolha qual bobina do estoque está nela.</p></section>';
	return h;
}

function vImpressora(d) {
	var imp = d.imp, t = imp.trabalho, h = '<div class="cab-pag"><div><h1>' + E(imp.nome) + " " + estado_imp(imp, d.estados) + "</h1><p>" + E(imp.modelo_nome || "Impressora") + " · mesa " + cm(imp.mesa_w) + " × " + cm(imp.mesa_d) + " × " + cm(imp.mesa_h) + " cm · " + E((d.multicor || {})[imp.multicor] || "") +
		(imp.visto_em ? " · vista pelo Slicer em " + quando(imp.visto_em) : "") + '</p></div></div><div class="fm-duas"><div class="fm-principal"><section class="bloco"><h2>Trabalho atual</h2>';
	if (t && ["imprimindo", "pausada", "limpar_mesa"].indexOf(imp.estado) >= 0) {
		h += '<div class="fm-job grande">' + foto(t.peca_id, t.cor_hex) + "<div><b>" + E(t.descricao) + "</b><small>mesa " + t.numero + " de " + t.total + " · " + t.quantidade + " peça" + pl_(t.quantidade, "", "s") + ' · <a href="/farm/pedidos/' + E(t.pedido_id) + '">pedido #' + E(t.pedido_numero) + "</a> · " + E(t.arquivo) + "</small><small>" +
			E(t.material + " " + (t.cor_nome || "")) + " · " + (t.gramas ? g(t.gramas) : "filamento a medir") + " · " + (t.tempo_s ? horas(t.tempo_s) : "tempo a medir") + "</small></div></div>";
		if (imp.estado != "limpar_mesa") h += '<div class="fm-barra"><i style="width:' + Math.round(t.progresso || 0) + '%"></i></div><div class="fm-prog"><span>' + Math.round(t.progresso || 0) + "% concluído</span><span>resta " + horas(t.resta_s || 0) + "</span></div>";
	} else h += '<p class="vazio">Nenhuma impressão em andamento.</p>';
	h += '<h3 class="fm-h3">Controle</h3>';
	if (imp.estado == "limpar_mesa") h += '<div class="fm-aviso violeta"><b>' + (t && t.estado == "falhou" ? "Impressão falhou" : "Impressão concluída") + " — limpar a mesa</b><p>A próxima mesa só é enviada depois que você confirmar.</p>" + botao("mesa_limpa", "Mesa limpa, enviar a próxima", { impressora_id: imp.id }, "btn btn-ink btn-sm") + "</div>";
	if ((imp.estado == "imprimindo" || imp.estado == "pausada") && t) {
		h += '<div class="fm-acoes">' + (imp.estado == "imprimindo" ? botao("pausar", "Pausar", {}, "btn btn-ink btn-sm") : botao("retomar", "Retomar", {}, "btn btn-ink btn-sm")) + botao("cancelar", "Cancelar impressão") +
			botao("concluir", "Marcar como concluída", { trabalho_id: t.id }) + botao("falhou", "Marcar como falhou", { trabalho_id: t.id }) + "</div>";
		if (d.comando) h += '<p class="fm-nota">Último comando: ' + E(d.comando.comando) + " — " + E({ pendente: "esperando o Filuna Slicer buscar", entregue: "entregue ao Filuna Slicer", feito: "feito", falhou: "não foi possível" }[d.comando.estado] || "") + " (" + quando(d.comando.atualizado_em) + ").</p>";
	}
	h += '</section><section class="bloco"><h2>Fila desta impressora</h2><div class="fm-rola"><table class="tab"><tr><th>Peça · pedido</th><th>Mesa</th><th>Peças</th><th>Filamento</th><th>Tempo</th><th>Estado</th><th></th></tr>';
	(d.fila || []).forEach(function (f, k) {
		h += '<tr><td><span class="fm-peca">' + foto(f.peca_id, f.cor_hex, true) + "<span>" + E(f.descricao) + '<small><a href="/farm/pedidos/' + E(f.pedido_id) + '">Pedido #' + E(f.pedido_numero) + "</a> · " + E(f.arquivo) + "</small></span></span></td><td>" + f.numero + " de " + f.total + "</td><td>" + f.quantidade + "</td><td>" + (f.gramas ? g(f.gramas) : "a medir") + "</td><td>" + (f.tempo_s ? horas(f.tempo_s) : "a medir") + "</td><td>" + estado_trab(f, d.bloqueios) + "</td><td>" +
			((f.estado == "aguardando" || f.estado == "enviado") && imp.estado == "livre" && k == 0 ? botao("iniciar", "Comecei a imprimir", { trabalho_id: f.id }) : f.estado == "falhou" ? botao("repetir", "Repetir", { trabalho_id: f.id }) : "") + "</td></tr>";
	});
	if (!(d.fila || []).length) h += '<tr><td colspan="7" class="vazio">Nenhuma mesa esperando nesta impressora.</td></tr>';
	h += '</table></div></section></div><aside class="fm-lado"><section class="bloco"><h2>Bobinas na impressora</h2>';
	(imp.slots || []).forEach(function (s) {
		h += '<form method="post" class="fm-slot-form"><input type="hidden" name="acao" value="carregar"><input type="hidden" name="impressora_id" value="' + E(imp.id) + '"><input type="hidden" name="posicao" value="' + s.posicao + '"><div class="fm-slot">' + cor(s.cor_hex) + "<div><b>Posição " + s.posicao + ": " + E(s.material ? s.material + " " + (s.cor_nome || "") : "vazia") + "</b><small>" +
			(s.bobina_id ? "bobina nº " + E(s.bobina_numero) + " · restam " + g(s.restante_g) : s.material ? "informada pelo Slicer; ligue a uma bobina do estoque para conferir o filamento" : "escolha a bobina") + '</small></div></div><label class="campo"><select name="bobina_id"><option value="">Sem bobina</option>' +
			(d.bobinas || []).map(function (b) { return '<option value="' + E(b.id) + '"' + (b.id == s.bobina_id ? " selected" : "") + ">" + E(b.material + " " + (b.cor_nome || "") + " nº " + b.numero + " · " + g(b.restante_g) + (b.na_impressora && b.id != s.bobina_id ? " · na " + b.na_impressora : "")) + "</option>"; }).join("") +
			'</select></label><button class="btn btn-line btn-sm">Salvar</button></form>';
	});
	h += '<p class="fm-nota">Trocar a bobina aqui libera na hora as mesas que esperavam essa cor. <a href="/farm/estoque">Cadastrar bobina</a></p></section>' + (["livre", "offline", "limpar_mesa"].indexOf(imp.estado) >= 0 ? botao("remover", "Remover esta impressora da Farm", {}, "btn btn-line btn-sm btn-block") : "") + "</aside></div>";
	return h;
}

function vEstoque(d) {
	var h = '<div class="cab-pag"><div><h1>Estoque de filamento</h1><p>Cada bobina tem seu número, mesmo quando são iguais. A baixa acontece sozinha ao fim de cada impressão.</p></div></div>';
	if (d.erro) h += '<div class="alerta erro">' + E(d.erro) + "</div>";
	h += '<section class="bloco" style="padding:8px 24px"><div class="fm-rola"><table class="tab"><tr><th>Bobina</th><th>Marca</th><th>Onde está</th><th>Restante</th><th>Na fila</th><th>Preço/kg</th><th>Ajustar</th></tr>';
	(d.bobinas || []).forEach(function (b) {
		h += '<tr><td><span class="fm-cornome">' + cor(b.cor_hex) + "<b>" + E(b.material + " " + (b.cor_nome || "") + " · nº " + b.numero) + '</b></span><small class="fm-bloco">' + (b.nova ? "nova, fechada" : "já usada") + " · " + g(b.peso_inicial_g) + " de fábrica</small></td><td>" + E(b.marca || "—") + "</td><td>" + E(b.na_impressora || "Guardada") + "</td><td><b>" + g(b.restante_g) + "</b></td><td>" +
			(b.reservado_g > b.restante_g ? '<span class="pilula warn">' + g(b.reservado_g) + ": insuficiente</span>" : g(b.reservado_g)) + "</td><td>" + (b.preco_kg ? brl(b.preco_kg) : "—") + '</td><td><form method="post" class="fm-ajuste"><input type="hidden" name="bobina_id" value="' + E(b.id) + '"><label><input name="restante" inputmode="decimal" value="' + Math.round(b.restante_g || 0) + '"></label><button class="btn btn-line btn-sm" name="acao" value="ajustar">Salvar</button><button class="btn btn-line btn-sm" name="acao" value="retirar">Acabou</button></form></td></tr>';
	});
	if (!(d.bobinas || []).length) h += '<tr><td colspan="7" class="vazio">Nenhuma bobina cadastrada.</td></tr>';
	h += '</table></div></section><section class="bloco"><h2>Cadastrar bobina</h2><form method="post" class="fm-form"><input type="hidden" name="acao" value="adicionar"><label class="campo"><span>Material</span><select name="material">' + (d.materiais || W.MATERIAIS).map(function (m) { return "<option>" + E(m) + "</option>"; }).join("") + "</select></label>" +
		'<label class="campo"><span>Cor</span><input type="color" name="cor_hex" value="#1E1E1E" class="fm-cor-campo"></label><label class="campo"><span>Nome da cor (opcional)</span><input name="cor_nome" maxlength="30" placeholder="Ex.: Preto fosco"></label><label class="campo"><span>Marca (opcional)</span><input name="marca" maxlength="40"></label>' +
		'<label class="campo"><span>Peso da bobina cheia (g)</span><input name="peso" inputmode="decimal" value="1000"></label><label class="campo"><span>Preço por kg (' + (window.Filuna && Filuna.simbolo ? Filuna.simbolo() : "R$") + ', opcional)</span><input name="preco_kg" inputmode="decimal" placeholder="' + (window.Filuna && Filuna.moeda && Filuna.moeda() == "USD" ? "25.00" : "120") + '"></label><label class="campo"><span>Quantas bobinas iguais</span><input type="number" name="quantidade" min="1" max="50" value="1"></label>' +
		'<label class="campo"><span>Já usada? Gramas que restam</span><input name="restante" inputmode="decimal" placeholder="Deixe vazio se for nova"></label><label class="check fm-check"><input type="checkbox" name="usada" value="1"> A bobina já foi usada (vale o restante informado)</label><div class="fm-form-fim"><button class="btn btn-ink">Cadastrar</button></div></form></section>';
	h += '<section class="bloco" style="padding:8px 24px"><table class="tab"><tr><th>Quando</th><th>Bobina</th><th>Motivo</th><th>Filamento</th><th>Purga</th></tr>';
	(d.movimentos || []).forEach(function (mv) { h += "<tr><td>" + quando(mv.em) + "</td><td>" + E(mv.material + " " + (mv.cor_nome || "") + " · nº " + mv.numero) + "</td><td>" + E(mv.descricao ? mv.descricao + " · " + mv.arquivo : "Ajuste manual") + "</td><td>" + g(mv.gramas) + "</td><td>" + g(mv.purga_g) + "</td></tr>"; });
	if (!(d.movimentos || []).length) h += '<tr><td colspan="5" class="vazio">Nenhuma baixa ainda.</td></tr>';
	return h + "</table></section>";
}
window.FarmTelas = { vPainel: vPainel, vImpressoras: vImpressoras, vImpressora: vImpressora, vEstoque: vEstoque, etapas: etapas, linha: linha, botao: botao, foto: foto, cor: cor, estado_imp: estado_imp,
	estado_trab: estado_trab, acao_correcao: acao_correcao, sem_plano: sem_plano, pq: pq, ou: ou, unicos: unicos, horas: horas, g: g, brl: brl, cm: cm, quando: quando, pl_: pl_, E: E, st: st, tela: tela };
})();
