/* Ponte das telas Filuna.
 * - Filuna.nativo(acao, dados): pede algo ao Filuna Slicer (C++). Mensagem {"filuna":1,"id","acao","dados"};
 *   o C++ responde chamando Filuna._resposta(id, ok, dados).
 * - Filuna.api(metodo, rota, corpo): dados da conta no site (os mesmos do filuna3d.com), com o token do dispositivo.
 * As telas são nativas do Filuna; do site vêm só os dados. */
var Filuna = (function () {
	var API = "https://filuna3d.com/api/slicer/v1";
	var pendentes = {}, seq = 0, token = "", ouvintes = {};

	function nativo(acao, dados) {
		return new Promise(function (ok, falha) {
			var id = "p" + (++seq);
			pendentes[id] = { ok: ok, falha: falha };
			try {
				window.wx.postMessage(JSON.stringify({ filuna: 1, id: id, acao: acao, dados: dados || {} }));
			} catch (e) {
				delete pendentes[id];
				falha(new Error("Fora do Filuna Slicer"));
				return;
			}
			setTimeout(function () {
				if (pendentes[id]) { delete pendentes[id]; falha(new Error("O Filuna não respondeu.")); }
			}, 30000);
		});
	}

	function _resposta(id, ok, dados) {
		var p = pendentes[id];
		if (!p) return;
		delete pendentes[id];
		if (ok) p.ok(dados || {}); else p.falha(new Error((dados && dados.erro) || "Não deu certo."));
	}

	function ouvir(nome, f) { (ouvintes[nome] = ouvintes[nome] || []).push(f); }
	function _evento(nome, dados) { (ouvintes[nome] || []).forEach(function (f) { try { f(dados || {}); } catch (e) {} }); }

	function guardarToken(t) {
		token = t || "";
		try { if (t) localStorage.setItem("filuna_token", t); else localStorage.removeItem("filuna_token"); } catch (e) {}
		return nativo("config.gravar", { chave: "filuna_token", valor: token }).catch(function () {});
	}

	function iniciar() {
		// O Filuna Slicer guarda a conta; o armazenamento da página só vale fora do programa.
		return nativo("config.ler", { chave: "filuna_token" }).then(function (r) {
			token = r.valor || "";
			return token;
		}, function () {
			try { token = localStorage.getItem("filuna_token") || ""; } catch (e) {}
			return token;
		});
	}

	function api(metodo, rota, corpo) {
		if (!token) return Promise.reject({ status: 401, erro: "Entre na sua conta Filuna para ver seus dados." });
		return fetch(API + rota, {
			method: metodo,
			headers: { "Authorization": "Bearer " + token, "Content-Type": "application/json" },
			body: corpo ? JSON.stringify(corpo) : undefined
		}).then(function (r) {
			return r.json().catch(function () { return {}; }).then(function (d) {
				if (!r.ok) throw { status: r.status, erro: d.erro || ("Erro " + r.status) };
				return d;
			});
		}, function () { throw { status: 0, erro: "Sem conexão com o filuna3d.com." }; });
	}

	function entrar(email, senha) {
		return fetch(API + "/entrar", {
			method: "POST", headers: { "Content-Type": "application/json" },
			body: JSON.stringify({ email: email, senha: senha, dispositivo: "Filuna Slicer" })
		}).then(function (r) {
			return r.json().then(function (d) { if (!r.ok) throw { erro: d.erro || "E-mail ou senha incorretos." }; return guardarToken(d.token); });
		});
	}

	function sair() { var t = api("POST", "/sair", {}).catch(function () {}); return t.then(function () { return guardarToken(""); }); }

	// utilidades de tela
	function esc(s) { return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); }
	function gramas(g) { g = Number(g || 0); return g >= 1000 ? (g / 1000).toFixed(2).replace(".", ",") + " kg" : Math.round(g) + " g"; }
	function tempo(s) { s = Math.max(0, Math.round(s || 0)); var h = Math.floor(s / 3600), m = Math.round((s % 3600) / 60); if (m == 60) { h++; m = 0; } return (h ? h + "h" : "") + (m < 10 && h ? "0" : "") + m + "min"; }
	function reais(v) { return "R$ " + Number(v || 0).toFixed(2).replace(".", ","); }
	function num(v, casas) { return Number(v || 0).toFixed(casas == null ? 2 : casas).replace(".", ","); }
	function abrirLink(url) { return nativo("url.abrir", { url: url }).catch(function () { window.open(url, "_blank"); }); }

	function telaLogin(el, aoEntrar, texto) {
		el.innerHTML = '<div class="login card"><div class="lbl">Conta Filuna</div><div class="h1" style="font-size:22px;margin:6px 0 4px">Entrar</div>' +
			'<div class="sub" style="margin-bottom:14px">' + esc(texto || "Os mesmos dados do filuna3d.com, aqui no Filuna.") + '</div>' +
			'<div class="campo"><label>E-mail</label><input type="email" id="flEmail"></div>' +
			'<div class="campo"><label>Senha</label><input type="password" id="flSenha"></div>' +
			'<div id="flErro"></div><button class="btn primario" id="flEntrar" style="width:100%">Entrar</button>' +
			'<div class="ajuda" style="margin-top:10px">Sem conta? <a id="flCriar">Crie em filuna3d.com</a></div></div>';
		el.querySelector("#flCriar").onclick = function () { abrirLink("https://filuna3d.com/entrar"); };
		el.querySelector("#flEntrar").onclick = function () {
			var b = this; b.disabled = true;
			entrar(el.querySelector("#flEmail").value, el.querySelector("#flSenha").value).then(aoEntrar, function (e) {
				b.disabled = false; el.querySelector("#flErro").innerHTML = '<div class="erro">' + esc(e.erro || e.message) + "</div>";
			});
		};
	}

	// Uma conta só para o programa inteiro: entrar ou sair em qualquer tela (Início incluído) recarrega as outras.
	ouvir("conta", function (d) {
		var t = (d && d.token) || "";
		if (t === token) return;
		token = t;
		try { if (t) localStorage.setItem("filuna_token", t); else localStorage.removeItem("filuna_token"); } catch (e) {}
		location.reload();
	});
	// Ao voltar para a tela, confere se a conta mudou enquanto ela estava escondida.
	ouvir("mostrar", function () {
		nativo("config.ler", { chave: "filuna_token" }).then(function (r) {
			if ((r.valor || "") !== token) _evento("conta", { token: r.valor || "" });
		}).catch(function () {});
	});

	return { nativo: nativo, api: api, iniciar: iniciar, entrar: entrar, sair: sair, ouvir: ouvir, _resposta: _resposta, _evento: _evento,
		temToken: function () { return !!token; }, esc: esc, gramas: gramas, tempo: tempo, reais: reais, num: num, abrirLink: abrirLink, telaLogin: telaLogin };
})();
