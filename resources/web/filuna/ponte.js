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
		// Vista 3/4 pela frente-direita: aba de trás (só a borda aparece à esquerda), o filamento enrolado
		// entre as abas e a aba da frente grande, perfurada, com três janelas que mostram o filamento.
		var id = "fb" + (++nBobina), c = corRgb(cor), lum = (0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]) / 255;
		var claro = lum > 0.85 ? "#FFFFFF" : misturar(c, [255, 255, 255], 0.5);
		var meio = "rgb(" + c.join(",") + ")";
		var escuro = misturar(c, [0, 0, 0], lum > 0.85 ? 0.24 : 0.5), fundo = misturar(c, [0, 0, 0], lum > 0.85 ? 0.36 : 0.65);
		var fio = misturar(c, [0, 0, 0], lum > 0.85 ? 0.2 : 0.35);
		var A = { cx: 96, cy: 70, rx: 46, ry: 64 };   // aba da frente
		var B = { cx: 50, cy: 70, rx: 46, ry: 64 };   // aba de trás (mesmo tamanho, deslocada)
		var R = 0.84;                                // raio do filamento enrolado em relação à aba
		function pt(e, f, graus) { var t = graus * Math.PI / 180; return [e.cx + e.rx * f * Math.sin(t), e.cy - e.ry * f * Math.cos(t)]; }
		function janela(centro) {
			var p = [], g;
			for (g = centro - 30; g <= centro + 30; g += 5) p.push(pt(A, 0.82, g));
			for (g = centro + 20; g >= centro - 20; g -= 5) p.push(pt(A, 0.4, g));
			return "M" + p.map(function (q) { return q[0].toFixed(1) + " " + q[1].toFixed(1); }).join(" L") + " Z";
		}
		var voltas = "";
		for (var k = 1; k < 22; k++) {
			var y = (A.cy - A.ry * R + k * (2 * A.ry * R) / 22).toFixed(1);
			voltas += '<line x1="' + (B.cx - 6) + '" x2="' + A.cx + '" y1="' + y + '" y2="' + y + '" stroke="' + fio + '" stroke-opacity=".18" stroke-width=".7"/>';
		}
		var topo = A.cy - A.ry * R, base = A.cy + A.ry * R;
		return '<svg class="bobina-svg" viewBox="0 0 150 140" width="' + (px || 64) + '" height="' + (px || 64) + '" aria-hidden="true">' +
			'<defs>' +
			'<linearGradient id="' + id + 'f" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="' + escuro + '"/><stop offset=".18" stop-color="' + meio + '"/>' +
			'<stop offset=".32" stop-color="' + claro + '"/><stop offset=".5" stop-color="' + meio + '"/><stop offset="1" stop-color="' + fundo + '"/></linearGradient>' +
			'<linearGradient id="' + id + 'j" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="' + claro + '"/><stop offset=".45" stop-color="' + meio + '"/><stop offset="1" stop-color="' + escuro + '"/></linearGradient>' +
			'<linearGradient id="' + id + 'a" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#5A6390"/><stop offset=".55" stop-color="#3E4568"/><stop offset="1" stop-color="#2B3050"/></linearGradient>' +
			'<pattern id="' + id + 'p" width="5.2" height="9" patternUnits="userSpaceOnUse"><circle cx="1.3" cy="2.2" r="1.05" fill="#1E2136"/><circle cx="3.9" cy="6.7" r="1.05" fill="#1E2136"/></pattern>' +
			'<clipPath id="' + id + 'c"><ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.9) + '" ry="' + (A.ry * 0.9) + '"/></clipPath>' +
			'</defs>' +
			// aba de trás: só a borda visível à esquerda
			'<ellipse cx="' + B.cx + '" cy="' + B.cy + '" rx="' + B.rx + '" ry="' + B.ry + '" fill="#2B3050"/>' +
			'<ellipse cx="' + (B.cx + 2.5) + '" cy="' + B.cy + '" rx="' + (B.rx - 2) + '" ry="' + (B.ry - 2) + '" fill="#363C60"/>' +
			// filamento: cilindro entre as abas
			'<path d="M' + B.cx + ' ' + topo.toFixed(1) + ' H' + A.cx + ' V' + base.toFixed(1) + ' H' + B.cx + ' Z" fill="url(#' + id + 'f)"/>' + voltas +
			'<ellipse cx="' + B.cx + '" cy="' + A.cy + '" rx="' + (B.rx * R).toFixed(1) + '" ry="' + (A.ry * R).toFixed(1) + '" fill="url(#' + id + 'f)"/>' +
			'<ellipse cx="' + B.cx + '" cy="' + A.cy + '" rx="' + (B.rx * R).toFixed(1) + '" ry="' + (A.ry * R).toFixed(1) + '" fill="none" stroke="' + fio + '" stroke-opacity=".25"/>' +
			// espessura e face da aba da frente
			'<ellipse cx="' + (A.cx - 3) + '" cy="' + A.cy + '" rx="' + A.rx + '" ry="' + A.ry + '" fill="#252946"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + A.rx + '" ry="' + A.ry + '" fill="url(#' + id + 'a)"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.95) + '" ry="' + (A.ry * 0.95) + '" fill="none" stroke="#7079A6" stroke-opacity=".55" stroke-width="1"/>' +
			'<g clip-path="url(#' + id + 'c)"><rect x="' + (A.cx - A.rx) + '" y="' + (A.cy - A.ry) + '" width="' + (2 * A.rx) + '" height="' + (2 * A.ry) + '" fill="url(#' + id + 'p)" opacity=".85"/></g>' +
			// janelas mostrando o filamento
			[0, 120, 240].map(function (g) {
				return '<path d="' + janela(g) + '" fill="url(#' + id + 'j)" stroke="#2B3050" stroke-width="3" stroke-linejoin="round"/>';
			}).join("") +
			// miolo
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.3) + '" ry="' + (A.ry * 0.3) + '" fill="#4B5380"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.24) + '" ry="' + (A.ry * 0.24) + '" fill="none" stroke="#6C76A3" stroke-width="1"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.18) + '" ry="' + (A.ry * 0.18) + '" fill="none" stroke="#6C76A3" stroke-opacity=".7" stroke-width=".8"/>' +
			'<ellipse cx="' + (A.cx + 1) + '" cy="' + A.cy + '" rx="' + (A.rx * 0.12) + '" ry="' + (A.ry * 0.12) + '" fill="#171A2C"/>' +
			'</svg>';
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
