/* Filuna — conta, Farm e estoque na tela inicial.
 * As telas são do Filuna; os dados vêm do site (https://filuna3d.com/api/slicer/v1) e são os mesmos que o site
 * mostra, sincronizados: o que muda aqui muda lá, e vice-versa. O token do dispositivo fica guardado neste
 * computador e nunca a senha. Contrato da API: site-v2/docs/farm.md no repositório do site.
 */
var FL_API = "https://filuna3d.com/api/slicer/v1";
var FL_CHAVE = "filuna_token";
var FL_INTERVALO = 30000;
var m_FlTimer = null;

function FlToken() {
	try { return window.localStorage.getItem(FL_CHAVE) || ""; } catch (e) { return ""; }
}

function FlGuardaToken(t) {
	try {
		if (t) window.localStorage.setItem(FL_CHAVE, t);
		else window.localStorage.removeItem(FL_CHAVE);
	} catch (e) { }
	FlAvisaFiluna(t);
}

// As telas Filuna (Dispositivo, Calibração, Meu espaço, rodapé) leem o mesmo token pelo Filuna Slicer.
function FlAvisaFiluna(t) {
	try { window.wx.postMessage(JSON.stringify({ command: "filuna_token", token: t || "" })); } catch (e) { }
}

function FlAbrirAba(aba, secao) {
	try { window.wx.postMessage(JSON.stringify({ command: "filuna_aba", aba: aba, secao: secao || "" })); } catch (e) { }
}

// Entrou ou saiu em outra tela do Filuna: a conta é uma só, esta tela acompanha.
function FlTokenDeFora(t) {
	try {
		if (t) window.localStorage.setItem(FL_CHAVE, t);
		else window.localStorage.removeItem(FL_CHAVE);
	} catch (e) { }
	if (t) { FlMostrar(true); FlAtualizar(); } else FlMostrar(false);
}
setTimeout(function () { if (FlToken()) FlAvisaFiluna(FlToken()); }, 500);

function FlChamar(metodo, caminho, corpo) {
	var cab = { "Content-Type": "application/json" };
	var t = FlToken();
	if (t) cab["Authorization"] = "Bearer " + t;
	return fetch(FL_API + caminho, { method: metodo, headers: cab, body: corpo ? JSON.stringify(corpo) : undefined, cache: "no-store" })
		.then(function (r) {
			return r.json().catch(function () { return {}; }).then(function (d) {
				if (r.status === 401) { FlGuardaToken(""); FlMostrar(false); }
				if (!r.ok) throw new Error(d.erro || ("Falha na conexão com o site (" + r.status + ")."));
				return d;
			});
		});
}

function FlEsc(s) {
	return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) {
		return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c];
	});
}

function FlGramas(g) { return Math.round(g || 0).toLocaleString("pt-BR") + " g"; }

function FlErro(msg) { $("#FlErro").text(msg || ""); }

function FlMostrar(ligado) {
	$("#FlDesligado").prop("hidden", ligado);
	$("#FlLigado").prop("hidden", !ligado);
	$(".FlSoLigado").prop("hidden", !ligado);
	if (!ligado) {
		if (m_FlTimer) { clearInterval(m_FlTimer); m_FlTimer = null; }
		if ($(".BtnItemSelected").hasClass("FlSoLigado")) GotoMenu("recent");
	}
}

function FlStatus(ok, texto) {
	$("#FlPonto").toggleClass("FlOff", !ok);
	$("#FlSync").text(texto);
}

function FlAtualizar() {
	if (!FlToken()) return;
	FlChamar("GET", "/resumo").then(function (r) {
		FlMostrar(true);
		$("#FlNome").text(r.conta.nome || r.conta.email);
		$("#FlEmailLigado").text(r.conta.email);
		$("#FlInicial").text((r.conta.nome || r.conta.email || "F").trim().charAt(0).toUpperCase());
		$("#FlPlano").text("Plano " + r.conta.plano);
		var f = r.farm;
		$("#FlFarmSub").text(f.liberada ? (f.impressoras + " de " + f.limite + " impressoras no plano " + r.conta.plano)
			: "A Farm faz parte dos planos Maker e Negócio.");
		$("#FlKpiImprimindo").text(f.imprimindo);
		$("#FlKpiMesas").text(f.mesas_na_fila);
		$("#FlKpiPecas").text(f.pecas_na_fila);
		$("#FlKpiLimpar").text(f.limpar_mesa);
		$("#FlMenuAvisos").text(r.avisos.length).prop("hidden", !r.avisos.length);
		var h = "";
		r.avisos.forEach(function (a) {
			h += '<div class="FlAviso ' + (a.tipo === "limpar" ? "FlVioleta" : "FlLaranja") + '"><b>' + FlEsc(a.titulo) + "</b><p>" + FlEsc(a.texto) + "</p>";
			if (a.tipo === "limpar" && a.impressora_id)
				h += '<button type="button" class="FlBotao" onclick="FlMesaLimpa(\'' + FlEsc(a.impressora_id) + '\', this)">Mesa limpa, liberar a próxima</button>';
			h += "</div>";
		});
		$("#FlAvisos").html(h || '<p class="FlVazio">Nada precisa de você agora.</p>');
		FlStatus(true, "Sincronizado");
		$(".FlStatus").attr("title", "Última sincronização com o site: " + new Date().toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" }));
		if ($(".BtnItemSelected").attr("menu") === "filuna_farm") FlImpressoras();
		if ($(".BtnItemSelected").attr("menu") === "filuna_estoque") FlEstoque();
	}).catch(function (e) {
		if (FlToken()) FlStatus(false, "Sem conexão");
	});
}

var FL_ESTADOS = { livre: "Livre", imprimindo: "Imprimindo", pausada: "Pausada", limpar_mesa: "Limpar a mesa", offline: "Desligada" };

function FlImpressoras() {
	FlChamar("GET", "/impressoras").then(function (r) {
		var h = "";
		r.impressoras.forEach(function (i) {
			h += '<div class="FlImp"><div class="FlImpCab"><b>' + FlEsc(i.nome) + '</b><span class="FlPilula FlEstado-' + FlEsc(i.estado) + '">' +
				FlEsc(FL_ESTADOS[i.estado] || i.estado) + "</span></div><small>Mesa " + (i.mesa.w / 10).toLocaleString("pt-BR") + " × " +
				(i.mesa.d / 10).toLocaleString("pt-BR") + " cm</small>";
			i.slots.forEach(function (s) {
				h += '<div class="FlSlot"><i style="background:' + FlEsc(s.cor || "#EFE9DF") + '"></i><span>' +
					(s.material ? FlEsc(s.material + " " + s.cor_nome) : "Posição " + s.posicao + " vazia") + "</span><em>" +
					(s.bobina_restante_g != null ? FlGramas(s.bobina_restante_g) : "") + "</em></div>";
			});
			if (i.estado === "limpar_mesa")
				h += '<button type="button" class="FlBotao" onclick="event.stopPropagation(); FlMesaLimpa(\'' + FlEsc(i.id) + '\', this)">Mesa limpa</button>';
			h += "</div>";
		});
		$("#FlImpressoras").html(h || '<p class="FlVazio">Nenhuma impressora na Farm ainda.</p>');
		// Clicar na impressora abre a unidade na tela Dispositivo.
		$("#FlImpressoras .FlImp").each(function (k) {
			var nome = (r.impressoras || [])[k] ? r.impressoras[k].nome : "";
			$(this).off("click").on("click", function () { FlAbrirAba("filuna.dispositivo", "unidade:" + nome); });
		});
	}).catch(function () { });
}

function FlEstoque() {
	FlChamar("GET", "/estoque").then(function (r) {
		var h = '<div class="FlLinha FlLinhaCab"><span>Bobina</span><span>Onde está</span><span>Restante</span><span>Na fila</span></div>';
		r.bobinas.forEach(function (b) {
			h += '<div class="FlLinha"><span class="FlCorNome"><i style="background:' + FlEsc(b.cor) + '"></i><b>' + FlEsc(b.material + " " + b.cor_nome) +
				" · nº " + b.numero + "</b><small>" + (b.nova ? "nova, fechada" : "já usada") + (b.marca ? " · " + FlEsc(b.marca) : "") + "</small></span><span>" +
				FlEsc(b.na_impressora || "Guardada") + "</span><span><b>" + FlGramas(b.restante_g) + "</b></span><span" +
				(b.insuficiente ? ' class="FlAlerta">' + FlGramas(b.na_fila_g) + ": insuficiente" : ">" + FlGramas(b.na_fila_g)) + "</span></div>";
		});
		$("#FlBobinas").html(r.bobinas.length ? h : '<p class="FlVazio">Nenhuma bobina cadastrada.</p>');
	}).catch(function () { });
}

function FlMesaLimpa(id, btn) {
	$(btn).prop("disabled", true).text("Liberando…");
	FlChamar("POST", "/impressoras/" + encodeURIComponent(id) + "/mesa-limpa", {}).then(FlAtualizar)
		.catch(function (e) { $(btn).prop("disabled", false).text(e.message); });
}

function FlLigou(r) {
	FlGuardaToken(r.token);
	FlErro("");
	$("#FlSenha").val("");
	FlAtualizar();
	if (!m_FlTimer) m_FlTimer = setInterval(function () { if (!document.hidden) FlAtualizar(); }, FL_INTERVALO);
}

$(function () {
	$("#FlFormEntrar").on("submit", function (ev) {
		ev.preventDefault();
		var b = $("#FlBtnEntrar").prop("disabled", true).text("Entrando…");
		FlChamar("POST", "/entrar", { email: $("#FlEmail").val(), senha: $("#FlSenha").val(), dispositivo: "Filuna Slicer" })
			.then(FlLigou).catch(function (e) { FlErro(e.message); })
			.then(function () { b.prop("disabled", false).text("Entrar"); });
	});
	$("#FlFormCodigo").on("submit", function (ev) {
		ev.preventDefault();
		FlChamar("POST", "/parear", { codigo: $("#FlCodigo").val(), dispositivo: "Filuna Slicer" })
			.then(FlLigou).catch(function (e) { FlErro(e.message); });
	});
	$("#FlBtnSair").on("click", function () {
		FlChamar("POST", "/sair", {}).catch(function () { }).then(function () { FlGuardaToken(""); FlMostrar(false); });
	});
	if (FlToken()) {
		FlMostrar(true);
		FlAtualizar();
		m_FlTimer = setInterval(function () { if (!document.hidden) FlAtualizar(); }, FL_INTERVALO);
	} else {
		FlMostrar(false);
	}
});

/* ---- Manutenção: atualização do programa, primeira instalação e diagnóstico ----
 * O Filuna Slicer (FilunaManutencao.cpp) consulta o servidor, baixa só o que mudou e, ao aplicar, fecha e abre
 * de novo sozinho. Aqui fica o aviso no canto inferior esquerdo, a janela com as novidades e as três perguntas
 * da primeira instalação. Coisas internas aparecem só pelo rótulo ("update de segurança" etc.). */
var m_FlUpd = { situacao: "ocioso" }, m_FlPrefs = null;
var m_FlBusca = false, m_FlBuscaT = 0, m_FlBuscaEm = 0, m_FlViuVerif = false;

function FlManut(acao, dados) {
	try { window.wx.postMessage(JSON.stringify({ command: "filuna_manutencao", corpo: JSON.stringify({ acao: acao, dados: dados || {} }) })); } catch (e) { }
}

function FlMB(b) { return (Math.max(0, b || 0) / 1048576).toFixed(1).replace(".", ",") + " MB"; }

function FlUpdDesenhar() {
	var e = m_FlUpd, s = e.situacao;
	var mostra = s == "pronto" || s == "disponivel" || s == "baixando" || s == "aplicando" || s == "erro";
	$("#FlUpdAviso").toggle(mostra);
	$("#FlUpdAviso .t").text(s == "baixando" ? "Baixando atualização… " + (e.feito || 0) + "%" : s == "aplicando" ? "Aplicando atualização…" : s == "erro" ? "Atualização não concluída" : s == "pronto" ? "Atualização pronta · clique para aplicar" : "Atualização disponível");
	$("#FlVersaoAtual .v").text(e.versao ? "Versão " + e.versao + " · " + (e.build || "") : "");
	// busca manual (botão "Procurar atualização"): mostra o resultado em vez de esperar a busca automática
	var $b = $("#FlUpdBuscar"), $r = $("#FlVersaoAtual .r");
	// instalado pela Microsoft Store: quem atualiza o programa é a Store, então aqui não há o que procurar
	if (s == "loja") { $b.hide(); $r.text("atualizado pela Microsoft Store"); FlLojaConferir(e.build, e.versao_loja); return; }
	$b.text(s == "verificando" ? "Procurando…" : "Procurar atualização").toggleClass("off", s == "verificando" || s == "baixando" || s == "aplicando");
	if (m_FlBusca && s == "verificando") m_FlViuVerif = true;
	// a resposta imediata ao clique ainda traz o estado de antes: só conclui depois de ver "procurando" ou após 3 s
	if (m_FlBusca && s != "verificando" && (m_FlViuVerif || Date.now() - m_FlBuscaEm > 3000)) {
		var res = s == "atual" ? "Você já está na versão mais nova." : s == "ocioso" ? "Não deu para procurar agora: confira a internet e a conta Filuna." : "";
		if (s == "disponivel" || s == "pronto" || s == "baixando") { m_FlBusca = false; $("#FlUpdJanela").show(); }
		else if (res) { m_FlBusca = false; $r.text(res); clearTimeout(m_FlBuscaT); m_FlBuscaT = setTimeout(function () { $r.text(""); }, 10000); }
	}
	if (!$("#FlUpdJanela").is(":visible")) return;
	$("#FlUpdTitulo").text("Filuna Slicer " + (e.nova_versao || ""));
	var h = "";
	(e.notas || []).forEach(function (n) { h += "<li>" + FlEsc(n) + "</li>"; });
	$("#FlUpdNotas").html(h || "<li>Melhorias e correções.</li>");
	h = "";
	(e.internos || []).forEach(function (n) { h += "<span>" + FlEsc(n) + "</span>"; });
	$("#FlUpdInternos").html(h).toggle(!!h);
	$("#FlUpdTam").text(s == "pronto" ? "Download concluído" : e.tamanho ? "Download: " + FlMB(e.tamanho) + (e.tipo == "delta" || e.tipo == "delta-zstd" ? " (só o que mudou)" : "") : "");
	var ocupado = s == "baixando" || s == "aplicando";
	$("#FlUpdBarra").toggle(ocupado); $("#FlUpdBarra i").css("width", (s == "aplicando" ? 100 : (e.feito || 0)) + "%");
	$("#FlUpdMsg").text(s == "baixando" ? "Baixando… " + (e.feito || 0) + "%" : s == "aplicando" ? "Pronto. O Filuna vai fechar e abrir de novo sozinho." : s == "erro" ? (e.erro || "Não deu certo.") + " O erro já foi enviado para a Filuna." : s == "pronto" ? "Já está baixada. O Filuna fecha uma vez, aplica e abre de novo sozinho." : "O Filuna fecha, atualiza e abre de novo sozinho.").toggleClass("erro", s == "erro");
	$("#FlUpdAgora").prop("disabled", ocupado).text(s == "erro" ? "Tentar de novo" : "Atualizar agora");
	$("#FlUpdDepois").prop("disabled", ocupado);
}

// Instalado pela Microsoft Store: a Store atualiza sozinha, mas no horário dela e sem avisar. O site diz se este build
// é mais velho que o publicado na loja; aí aparece "Nova versão na Microsoft Store", que abre o app na Store para
// atualizar na hora (pedido do dono, 21/11/2026). Confere ao abrir e a cada 30 min.
var m_FlLoja = { em: 0, dados: null };
function FlLojaConferir(build, versao) {
	if (!build || Date.now() - m_FlLoja.em < 1800000) return FlLojaDesenhar();
	m_FlLoja.em = Date.now();
	fetch(FL_API + "/loja/versao?build=" + encodeURIComponent(build) + (versao ? "&versao=" + encodeURIComponent(versao) : ""), { cache: "no-store" }).then(function (r) { return r.json(); })
		.then(function (d) { m_FlLoja.dados = d; FlLojaDesenhar(); }).catch(function () { m_FlLoja.em = 0; });
}
function FlLojaDesenhar() {
	var d = m_FlLoja.dados;
	if (!d || !d.atualizar) return;
	$("#FlUpdAviso").show().addClass("loja");
	$("#FlUpdAviso .t").text("Nova versão " + String(d.publicada || "").replace(/\.0$/, "") + " na Microsoft Store · Atualizar")
		.attr("title", "Abre o Filuna na Microsoft Store. A Store instala a versão nova quando o Filuna estiver fechado.");
	$("#FlVersaoAtual .r").text("");
}

function FlPrimeiraDesenhar() {
	var p = m_FlPrefs || {};
	if (p.primeira_ok == "1" || !FlToken()) { $("#FlPrimeira").hide(); return; }
	$("#FlPrimeira").show();
}

// Chamado pelo Filuna Slicer (C++): {tipo: "estado", estado: {...}} ou {tipo: "preferencias", preferencias: {...}}
function FlManutencao(m) {
	if (!m) return;
	if (m.tipo == "estado" && m.estado) { m_FlUpd = m.estado; FlUpdDesenhar(); }
	if (m.tipo == "preferencias" && m.preferencias) {
		m_FlPrefs = m.preferencias;
		// "manter a conta logada" desligado: a conta sai quando o programa abre de novo
		try {
			if (m_FlPrefs.manter_login == "0" && !window.sessionStorage.getItem("fl_sessao")) { FlGuardaToken(""); FlMostrar(false); }
			window.sessionStorage.setItem("fl_sessao", "1");
		} catch (e) { }
		FlPrimeiraDesenhar();
	}
}

$(function () {
	$("body").append(
		'<div id="FlRodape"><div id="FlUpdAviso" style="display:none"><b></b><span class="t">Atualização disponível</span></div>' +
		'<div id="FlVersaoAtual"><span class="v"></span><a href="#" id="FlUpdBuscar">Procurar atualização</a><span class="r"></span></div></div>' +
		'<div id="FlUpdJanela" class="FlJan" style="display:none"><div class="cx">' +
		'<small>Atualização do Filuna Slicer</small><h2 id="FlUpdTitulo"></h2>' +
		'<h4>Novidades</h4><ul id="FlUpdNotas"></ul><div id="FlUpdInternos"></div>' +
		'<div id="FlUpdBarra" style="display:none"><i></i></div><p id="FlUpdMsg"></p><p id="FlUpdTam"></p>' +
		'<div class="bt"><button id="FlUpdDepois" class="sec">Depois</button><button id="FlUpdAgora">Atualizar agora</button></div></div></div>' +
		'<div id="FlPrimeira" class="FlJan" style="display:none"><div class="cx">' +
		'<small>Primeira vez neste computador</small><h2>Como você quer usar o Filuna?</h2>' +
		'<label><input type="checkbox" id="FlPrefWin"> Abrir o Filuna junto com o Windows</label>' +
		'<label><input type="checkbox" id="FlPrefLogin" checked> Manter minha conta conectada neste computador</label>' +
		'<label><input type="checkbox" id="FlPrefDisp" checked> Atualizar sozinho os dispositivos e o suporte às minhas impressoras</label>' +
		'<p>Você muda isso depois, quando quiser.</p>' +
		'<div class="bt"><button id="FlPrefSalvar">Salvar</button></div></div></div>');
	$("#FlUpdAviso").on("click", function () {
		// na versão da Store o aviso abre o Filuna na Microsoft Store, onde fica o botão Atualizar
		if (m_FlUpd.situacao == "loja" && m_FlLoja.dados && m_FlLoja.dados.atualizar) {
			try { window.wx.postMessage(JSON.stringify({ command: "common_openurl", url: m_FlLoja.dados.abrir })); } catch (e) { }
			return;
		}
		$("#FlUpdJanela").show(); FlUpdDesenhar();
	});
	$("#FlUpdDepois").on("click", function () { $("#FlUpdJanela").hide(); });
	$(document).on("click", "#FlUpdBuscar", function (ev) {
		ev.preventDefault();
		if ($(this).hasClass("off")) return;
		m_FlBusca = true; m_FlViuVerif = false; m_FlBuscaEm = Date.now(); $("#FlVersaoAtual .r").text("");
		$(this).text("Procurando…").addClass("off");
		FlManut("atualizacao.verificar");
		// a resposta chega pelo estado; se não chegar (sem conta), confere de novo
		setTimeout(function () { FlManut("atualizacao.estado"); }, 4000);
		setTimeout(function () { FlManut("atualizacao.estado"); }, 15000);
	});
	$("#FlUpdAgora").on("click", function () { FlManut(m_FlUpd.situacao == "erro" ? "atualizacao.verificar" : "atualizacao.aplicar"); });
	$("#FlPrefSalvar").on("click", function () {
		FlManut("preferencias.gravar", { inicio_windows: $("#FlPrefWin").prop("checked"), manter_login: $("#FlPrefLogin").prop("checked"), auto_dispositivos: $("#FlPrefDisp").prop("checked") });
		$("#FlPrimeira").hide();
	});
	setTimeout(function () { FlManut("preferencias.ler"); FlManut("atualizacao.estado"); }, 800);
	setInterval(function () { if (!document.hidden) FlManut("atualizacao.estado"); }, 60000);
	// erro de script nesta tela também vai para o diagnóstico
	window.addEventListener("error", function (ev) {
		FlManut("diagnostico.enviar", { etapa: "inicio", mensagem: String(ev.message || "erro de script"), contexto: { arquivo: String(ev.filename || ""), linha: ev.lineno || 0 } });
	});
});

/* ---- Suporte remoto: o cliente autoriza aqui, por prazo, e encerra quando quiser ----
 * Com a autorização em vigor, o suporte da Filuna pode ler os dados do Filuna Slicer deste computador e pedir
 * ações de uma lista fechada (ver ponte.js), só nas categorias que o cliente marcou (diagnóstico sempre; conexão e
 * reparos, impressoras e o fatiador completo só se marcar). Tudo o que for feito aparece no histórico abaixo. */
function FlSupData(iso) { try { return new Date(iso).toLocaleString("pt-BR", { day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit" }); } catch (e) { return iso || ""; } }

function FlSupDesenhar(r) {
	var h = "";
	if (r.ativa) {
		var nomes = (r.opcoes || []).filter(function (o) { return (r.categorias || []).indexOf(o.id) >= 0; }).map(function (o) { return o.nome; });
		h += '<p class="ok">Suporte remoto <b>autorizado</b> ' + (r.permanente ? "até você desligar" : "até " + FlEsc(FlSupData(r.expira_em))) + '.</p>' +
			(nomes.length ? "<p>Liberado: <b>" + FlEsc(nomes.join(", ")) + "</b>.</p>" : "") +
			'<div class="bt"><button id="FlSupEncerrar" class="sec">Encerrar agora</button></div>';
	} else {
		h += "<p>Marque o que o suporte da Filuna pode fazer:</p>" + (r.opcoes || []).map(function (o) {
			return '<label class="FlSupCat"><input type="checkbox" value="' + FlEsc(o.id) + '"' + (o.obrigatoria ? " checked disabled" : (o.id == "fatiador" ? "" : " checked")) + "> <b>" +
				FlEsc(o.nome) + "</b><span>" + FlEsc(o.texto) + "</span></label>";
		}).join("") +
			'<p>E por quanto tempo:</p><div class="FlSupOp">' +
			'<button data-h="1">1 hora</button><button data-h="24">24 horas</button><button data-h="168">7 dias</button>' +
			(r.pode_permanente ? '<button data-p="1" class="sec">Até eu desligar</button>' : "") + '</div>';
	}
	var hist = (r.historico || []).slice(0, 12);
	if (hist.length) {
		h += "<h4>O que o suporte fez</h4><ul>";
		hist.forEach(function (i) { h += "<li>" + FlEsc(FlSupData(i.criado_em)) + " · " + FlEsc(i.acao) + (i.concluido_em ? (i.ok ? " · feito" : " · não concluiu") : " · aguardando") + "</li>"; });
		h += "</ul>";
	}
	$("#FlSupCorpo").html(h);
	$("#FlSupLink").text(r.ativa ? "Suporte remoto: autorizado" : "Suporte remoto").toggleClass("on", !!r.ativa);
	$("#FlSupCorpo [data-h], #FlSupCorpo [data-p]").on("click", function () {
		var b = $(this), corpo = b.data("p") ? { permanente: true } : { horas: parseInt(b.data("h"), 10) };
		corpo.categorias = $("#FlSupCorpo .FlSupCat input:checked").map(function () { return this.value; }).get();
		FlChamar("POST", "/suporte/autorizar", corpo).then(FlSupCarregar).catch(function (e) { $("#FlSupCorpo").prepend('<p class="erro">' + FlEsc(e.message) + "</p>"); });
	});
	$("#FlSupEncerrar").on("click", function () { FlChamar("POST", "/suporte/encerrar", {}).then(FlSupCarregar).catch(function () { }); });
}

function FlSupCarregar() {
	if (!FlToken()) { $("#FlSupLink").hide(); return Promise.resolve(); }
	$("#FlSupLink").show();
	return FlChamar("GET", "/suporte/estado").then(FlSupDesenhar).catch(function () { });
}

$(function () {
	$("body").append('<div id="FlSupLink" style="display:none">Suporte remoto</div>' +
		'<div id="FlSup" class="FlJan" style="display:none"><div class="cx"><small>Suporte remoto da Filuna</small><h2>Você decide quando e por quanto tempo</h2>' +
		'<p>Com a sua autorização, o suporte da Filuna pode <b>ler os dados do Filuna Slicer</b> deste computador (versão, impressoras, módulos, registro de erros) e <b>fazer só o que você marcar</b>, de uma lista fechada: conexão e reparos, controle das impressoras e o fatiador completo (perfis, ajustes, peças, fatiar e mandar imprimir). O suporte <b>não vê a sua tela, não acessa seus arquivos e não roda comandos</b> no seu computador. Você pode encerrar a qualquer momento.</p>' +
		'<div id="FlSupCorpo"></div><div class="bt"><button id="FlSupFechar" class="sec">Fechar</button></div></div></div>');
	$("#FlSupLink").on("click", function () { $("#FlSup").show(); FlSupCarregar(); });
	$("#FlSupFechar").on("click", function () { $("#FlSup").hide(); });
	setTimeout(FlSupCarregar, 1500);
	setInterval(function () { if (!document.hidden) FlSupCarregar(); }, 120000);
});


// ---------- Modelos 3D do site na tela inicial ----------
function FlM3dCarregar() {
	fetch("https://filuna3d.com/api/slicer/v1/modelos3d").then(function (r) { return r.json(); }).then(function (d) {
		var ms = (d && d.modelos) || [];
		if (!ms.length) return;
		var tempo = function (s) { s = +s || 0; return s >= 3600 ? Math.floor(s / 3600) + "h" + ("0" + Math.floor(s % 3600 / 60)).slice(-2) : Math.max(1, Math.round(s / 60)) + " min"; };
		var h = "";
		ms.forEach(function (m, k) {
			h += '<a class="m3d-cartao" data-k="' + k + '"><span class="foto"><img src="' + FlEsc(m.capa) + '" alt="" loading="lazy"></span><span class="txt">' +
				'<span class="cat">' + FlEsc(m.categoria || "") + "</span><b>" + FlEsc(m.titulo) + '</b><span class="pontos">' +
				(m.cores || []).map(function (c) { return '<i style="--c:' + FlEsc(c.hex) + '" title="' + FlEsc(c.nome || "") + '"></i>'; }).join("") + "</span>" +
				(m.tempo_s ? '<span class="dados"><span>' + tempo(m.tempo_s) + "</span><span>" + m.gramas + ' g</span><span class="preco">vende por R$ ' + Number(m.preco).toFixed(2).replace(".", ",") + "</span></span>" : "") +
				"</span></a>";
		});
		$("#FlM3dGrade").html(h);
		$("#FlM3d").show();
		// O cartão abre a aba Modelos 3D do Filuna já na página do modelo ("modelo:<slug>").
		$("#FlM3dGrade .m3d-cartao").on("click", function () { var m = ms[+$(this).attr("data-k")]; if (m) FlAbrirAba("filuna.modelos", m.slug ? "modelo:" + m.slug : ""); });
	}).catch(function () { /* sem internet: a seção simplesmente não aparece */ });
}
$(function () { FlM3dCarregar(); });

/* ---- Rodapé da coluna da esquerda: suporte remoto, aviso de atualização e versão num bloco só ----
   O link do suporte entra no rodapé (antes ficava solto numa altura fixa e encostava nos outros) e a coluna
   reserva embaixo a altura real do rodapé, que muda quando o aviso ou o resultado da busca aparecem. */
function FlRodapeArrumar() {
	var r = document.getElementById("FlRodape"), s = document.getElementById("FlSupLink");
	if (!r) return;
	if (s && s.parentNode !== r) r.insertBefore(s, r.firstChild);
	document.documentElement.style.setProperty("--fl-rodape-h", r.offsetHeight + "px");
}
$(function () {
	FlRodapeArrumar();
	var r = document.getElementById("FlRodape");
	if (r && window.ResizeObserver) new ResizeObserver(FlRodapeArrumar).observe(r);
});
