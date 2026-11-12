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
