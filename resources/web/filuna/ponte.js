/* Ponte das telas Filuna.
 * - Filuna.nativo(acao, dados): pede algo ao Filuna Slicer (C++). Mensagem {"filuna":1,"id","acao","dados"};
 *   o C++ responde chamando Filuna._resposta(id, ok, dados).
 * - Filuna.api(metodo, rota, corpo): dados da conta, com o token do dispositivo. O Filuna trabalha sem internet:
 *   cada leitura fica guardada no computador e é usada quando não há conexão; cadastros e ajustes ficam numa fila e
 *   são enviados quando a conexão volta. A conta vale 3 dias sem comprovar conexão (Filuna.plano()).
 * As telas são do Filuna Slicer; o site só sincroniza os dados. */
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
		// Mesma coluna da esquerda da Preparação em todas as telas.
		nativo("app.metricas").then(function (m) {
			if (m.lateral) document.documentElement.style.setProperty("--lateral", m.lateral + "px");
		}).catch(function () {});
		pendentesLocais().then(function (f) { conexao.pendentes = f.length; if (f.length) enviarFila(); });
		// O Filuna Slicer guarda a conta; o armazenamento da página só vale fora do programa.
		return nativo("config.ler", { chave: "filuna_token" }).then(function (r) {
			token = r.valor || "";
			return token;
		}, function () {
			try { token = localStorage.getItem("filuna_token") || ""; } catch (e) {}
			return token;
		});
	}

	// ---- Dados no computador (funciona sem internet) ----
	var TOLERANCIA_MS = 3 * 24 * 3600 * 1000;           // a conta vale 3 dias sem comprovar conexão
	var NA_FILA = ["/app/estoque/nova", "/app/estoque/ajustar", "/preferencias"]; // só o que é seguro mandar depois
	var conexao = { online: true, pendentes: 0 };

	function lerLocal(chave) {
		return nativo("armazem.ler", { chave: chave }).then(function (r) { return r.valor; }, function () {
			try { return JSON.parse(localStorage.getItem("fl." + chave)); } catch (e) { return null; }
		});
	}
	function guardarLocal(chave, valor) {
		return nativo("armazem.gravar", { chave: chave, valor: valor }).catch(function () {
			try { localStorage.setItem("fl." + chave, JSON.stringify(valor)); } catch (e) {}
		});
	}
	function chaveCache(rota) { return "api." + rota.replace(/[^A-Za-z0-9]/g, "_").slice(0, 100); }

	function rede(metodo, rota, corpo) {
		return fetch(API + rota, {
			method: metodo,
			headers: { "Authorization": "Bearer " + token, "Content-Type": "application/json" },
			body: corpo ? JSON.stringify(corpo) : undefined
		}).then(function (r) {
			return r.json().catch(function () { return {}; }).then(function (d) {
				if (!r.ok) throw { status: r.status, erro: d.erro || ("Erro " + r.status) };
				return d;
			});
		}, function () { throw { status: 0, erro: "Sem conexão." }; });
	}

	function marcarConexao(online) {
		if (conexao.online === online) return;
		conexao.online = online;
		mostrarConexao();
		_evento("conexao", { online: online });
		if (online) enviarFila();
	}

	function api(metodo, rota, corpo) {
		if (!token) return Promise.reject({ status: 401, erro: "Entre na sua conta Filuna para ver seus dados." });
		if (metodo == "GET") {
			return rede(metodo, rota).then(function (d) {
				marcarConexao(true);
				guardarLocal(chaveCache(rota), d);
				if (rota == "/resumo" || rota == "/conta") guardarLocal("conta.verificada", Date.now());
				return d;
			}, function (e) {
				if (e.status !== 0) throw e;
				marcarConexao(false);
				return lerLocal(chaveCache(rota)).then(function (d) {
					if (d == null) throw { status: 0, erro: "Sem conexão. Estes dados aparecem depois da primeira conexão do Filuna." };
					d._offline = true;
					return d;
				});
			});
		}
		return rede(metodo, rota, corpo).then(function (d) { marcarConexao(true); return d; }, function (e) {
			if (e.status !== 0) throw e;
			marcarConexao(false);
			if (NA_FILA.indexOf(rota) < 0) throw { status: 0, erro: "Sem conexão. Esta ação precisa da impressora ou da conta conectadas." };
			return enfileirar(metodo, rota, corpo).then(function () { return { _pendente: true }; });
		});
	}

	// Fila do que foi feito sem internet; enviada na ordem quando a conexão volta.
	function enfileirar(metodo, rota, corpo) {
		return lerLocal("fila").then(function (f) {
			f = Array.isArray(f) ? f : [];
			f.push({ metodo: metodo, rota: rota, corpo: corpo || {}, em: new Date().toISOString() });
			conexao.pendentes = f.length; mostrarConexao();
			return guardarLocal("fila", f);
		});
	}
	var enviando = false;
	function enviarFila() {
		if (enviando || !token) return Promise.resolve();
		enviando = true;
		return lerLocal("fila").then(function (f) {
			f = Array.isArray(f) ? f : [];
			var i = 0;
			function proximo() {
				if (i >= f.length) return null;
				var it = f[i];
				return rede(it.metodo, it.rota, it.corpo).then(function () { i++; return proximo(); }, function (e) {
					if (e.status === 0) { marcarConexao(false); return null; }   // ainda sem conexão: tenta depois
					i++; return proximo();                                         // recusado pelo servidor: não repete
				});
			}
			return Promise.resolve(proximo()).then(function () {
				var resto = f.slice(i);
				conexao.pendentes = resto.length; mostrarConexao(true);
				if (i > 0) _evento("sincronizado", {});
				return guardarLocal("fila", resto);
			});
		}).then(function () { enviando = false; }, function () { enviando = false; });
	}
	function pendentesLocais() { return lerLocal("fila").then(function (f) { return Array.isArray(f) ? f : []; }); }

	// A conta (plano) vale 3 dias sem comprovar conexão; o motor Orca funciona sempre, com ou sem conta.
	function plano() {
		return lerLocal("conta.verificada").then(function (t) {
			var desde = t ? Date.now() - Number(t) : Infinity;
			return { online: conexao.online, valido: conexao.online || desde < TOLERANCIA_MS,
				horasRestantes: conexao.online ? 72 : Math.max(0, Math.round((TOLERANCIA_MS - desde) / 3600000)) };
		});
	}

	var avisoTimer = null;
	function mostrarConexao(acabouDeEnviar) {
		var el = document.getElementById("flConexao");
		if (!el) { el = document.createElement("div"); el.id = "flConexao"; document.body.appendChild(el); }
		clearTimeout(avisoTimer);
		if (!conexao.online) {
			el.className = ""; el.style.display = "flex";
			el.textContent = conexao.pendentes ? "Sem conexão — " + conexao.pendentes + (conexao.pendentes == 1 ? " alteração será enviada depois" : " alterações serão enviadas depois") : "Sem conexão — trabalhando no computador";
		} else if (acabouDeEnviar && !conexao.pendentes) {
			el.className = "ok"; el.style.display = "flex"; el.textContent = "Salvo no servidor";
			avisoTimer = setTimeout(function () { el.style.display = "none"; }, 3000);
		} else el.style.display = "none";
	}

	// Preferências Filuna: ficam nas Configurações do programa (engrenagem), no computador; a conta recebe uma cópia.
	var PREFS = { "interface.autoajuda": "bool", "estoque.alerta_baixo": "bool", "estoque.limite_baixo_g": "num",
		"farm.avisar_limpar_mesa": "bool", "farm.avisar_filamento": "bool", "inicio.tendencias": "bool", "ofertas.ativas": "bool",
		"ofertas.material": "texto", "ofertas.preco_max_kg": "num", "calculo.margem": "num", "calculo.mao_obra_hora": "num",
		"calculo.energia_kwh": "num", "calculo.potencia_w": "num", "calculo.preco_kg": "num" };
	function prefs() {
		var chaves = Object.keys(PREFS), valores = {};
		return Promise.all(chaves.map(function (k) {
			return nativo("config.ler", { chave: "filuna_pref_" + k.replace(/\./g, "_") }).then(function (r) {
				var v = r.valor;
				if (v === undefined || v === null || v === "") return;
				valores[k] = PREFS[k] == "bool" ? (v == "true" || v == "1") : PREFS[k] == "num" ? Number(String(v).replace(",", ".")) : v;
			}).catch(function () {});
		})).then(function () {
			if (!token) return valores;
			// O que não foi ajustado neste computador vem da conta (última cópia guardada, se estiver sem internet).
			return api("GET", "/preferencias").then(function (r) {
				var s = r.valores || {};
				Object.keys(s).forEach(function (k) { if (!(k in valores)) valores[k] = s[k]; });
				return valores;
			}, function () { return valores; });
		});
	}
	function enviarPrefs() {
		if (!token) return Promise.resolve();
		var locais = {};
		return Promise.all(Object.keys(PREFS).map(function (k) {
			return nativo("config.ler", { chave: "filuna_pref_" + k.replace(/\./g, "_") }).then(function (r) {
				if (r.valor === undefined || r.valor === null) return;
				locais[k] = PREFS[k] == "bool" ? (r.valor == "true" || r.valor == "1") : PREFS[k] == "num" ? (r.valor === "" ? null : Number(String(r.valor).replace(",", "."))) : r.valor;
			}).catch(function () {});
		})).then(function () { return Object.keys(locais).length ? api("POST", "/preferencias", { valores: locais }).catch(function () {}) : null; });
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

	// Desenho da bobina com a cor do filamento (hex "#RRGGBB" ou "rgb(r,g,b)"): abas perfuradas e o
	// filamento com luz e sombra calculadas da cor, para qualquer cor (inclusive branco e preto).
	var nBobina = 0;
	function corRgb(c) {
		c = String(c || "").trim();
		var m = /^#?([0-9a-f]{6})$/i.exec(c);
		if (m) return [parseInt(m[1].slice(0, 2), 16), parseInt(m[1].slice(2, 4), 16), parseInt(m[1].slice(4, 6), 16)];
		m = /^#?([0-9a-f]{3})$/i.exec(c);
		if (m) return [0, 1, 2].map(function (i) { return parseInt(m[1][i] + m[1][i], 16); });
		m = /rgba?\(\s*(\d+)[ ,]+(\d+)[ ,]+(\d+)/i.exec(c);
		if (m) return [+m[1], +m[2], +m[3]].map(function (v) { return Math.max(0, Math.min(255, v)); });
		return [194, 24, 91];
	}
	function misturar(rgb, alvo, t) { return "rgb(" + rgb.map(function (v, i) { return Math.round(v + (alvo[i] - v) * t); }).join(",") + ")"; }
	function bobinaSVG(cor, px) {
		var id = "fb" + (++nBobina), c = corRgb(cor), lum = (0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]) / 255;
		var claro = misturar(c, [255, 255, 255], lum > 0.85 ? 0.7 : 0.5), escuro = misturar(c, [0, 0, 0], lum > 0.85 ? 0.2 : 0.45);
		var base = "rgb(" + c.join(",") + ")", fio = misturar(c, [0, 0, 0], lum > 0.85 ? 0.18 : 0.3);
		var aba = "#363B59", abaLuz = "#59628C", furo = "#23263B";
		var furos = "";
		for (var y = 10, lin = 0; y <= 110; y += 5.6, lin++)
			for (var x = 74 + (lin % 2 ? 2.8 : 0); x <= 118; x += 5.6) {
				var dx = (x - 96) / 22, dy = (y - 60) / 55, d = dx * dx + dy * dy;
				if (d < 0.82 && d > 0.16) furos += '<circle cx="' + x.toFixed(1) + '" cy="' + y.toFixed(1) + '" r="1.15" fill="' + furo + '"/>';
			}
		var voltas = "";
		for (var k = 0; k < 17; k++) { var yy = (14 + k * 5.7).toFixed(1); voltas += '<line x1="30" x2="96" y1="' + yy + '" y2="' + yy + '" stroke="' + fio + '" stroke-opacity=".22" stroke-width=".6"/>'; }
		return '<svg class="bobina-svg" viewBox="0 0 124 120" width="' + (px || 64) + '" height="' + (px || 64) + '" aria-hidden="true">' +
			'<defs><linearGradient id="' + id + 'f" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="' + escuro + '"/><stop offset=".25" stop-color="' + claro + '"/>' +
			'<stop offset=".55" stop-color="' + base + '"/><stop offset="1" stop-color="' + escuro + '"/></linearGradient>' +
			'<linearGradient id="' + id + 'a" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="' + abaLuz + '"/><stop offset="1" stop-color="' + aba + '"/></linearGradient></defs>' +
			'<ellipse cx="30" cy="60" rx="19" ry="55" fill="' + aba + '"/>' +
			'<path d="M30 8 H96 V112 H30 Z" fill="url(#' + id + 'f)"/>' + voltas +
			'<path d="M30 8 H96" stroke="' + claro + '" stroke-opacity=".7" stroke-width="1"/>' +
			'<ellipse cx="96" cy="60" rx="22" ry="55" fill="url(#' + id + 'a)"/>' + furos +
			'<path d="M96 60 L87 11 A22 55 0 0 1 105 11 Z" fill="url(#' + id + 'f)"/>' +
			'<path d="M96 60 L79 95 A22 55 0 0 0 88 113 Z" fill="url(#' + id + 'f)"/>' +
			'<path d="M96 60 L113 95 A22 55 0 0 1 104 113 Z" fill="url(#' + id + 'f)"/>' +
			'<ellipse cx="96" cy="60" rx="22" ry="55" fill="none" stroke="' + furo + '" stroke-opacity=".55" stroke-width="1.2"/>' +
			'<ellipse cx="96" cy="60" rx="9" ry="21" fill="' + abaLuz + '"/><ellipse cx="96.6" cy="60" rx="5.5" ry="13" fill="' + furo + '"/>' +
			'<ellipse cx="96.6" cy="60" rx="2.8" ry="7" fill="#12131F"/></svg>';
	}
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

	// Telas de programa: sem menu de navegador nem arrastar imagens/textos.
	document.addEventListener("contextmenu", function (e) { if (!/^(INPUT|TEXTAREA)$/.test(e.target.tagName)) e.preventDefault(); });
	document.addEventListener("dragstart", function (e) { if (e.target.tagName == "IMG" || e.target.tagName == "A") e.preventDefault(); });
	window.addEventListener("online", function () { enviarFila(); });
	setInterval(function () { if (conexao.pendentes) enviarFila(); }, 60000);
	ouvir("preferencias.locais", function () { enviarPrefs().then(function () { _evento("preferencias", {}); }); });

	return { nativo: nativo, api: api, iniciar: iniciar, plano: plano, prefs: prefs, enviarPrefs: enviarPrefs, lerLocal: lerLocal, guardarLocal: guardarLocal,
		pendentesLocais: pendentesLocais, conexao: function () { return conexao; }, entrar: entrar, sair: sair, ouvir: ouvir, _resposta: _resposta, _evento: _evento,
		temToken: function () { return !!token; }, esc: esc, bobinaSVG: bobinaSVG, corRgb: corRgb, gramas: gramas, tempo: tempo, reais: reais, num: num, abrirLink: abrirLink, telaLogin: telaLogin };
})();
