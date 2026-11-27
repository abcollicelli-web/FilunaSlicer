/* Ponte das telas Filuna.
 * - Filuna.nativo(acao, dados): pede algo ao Filuna Slicer (C++). Mensagem {"filuna":1,"id","acao","dados"};
 *   o C++ responde chamando Filuna._resposta(id, ok, dados).
 * - Filuna.api(metodo, rota, corpo): dados da conta, com o token do dispositivo. O Filuna trabalha sem internet:
 *   cada leitura fica guardada no computador e é usada quando não há conexão; cadastros e ajustes ficam numa fila e
 *   são enviados quando a conexão volta. A conta vale 3 dias sem comprovar conexão (Filuna.plano()).
 * As telas são do Filuna Slicer; o site só sincroniza os dados. */
var Filuna = (function () {
	var API = "https://filuna3d.com/api/slicer/v1";
	// O login fica sempre no site de verdade; os dados (Filuna.api) podem ir a um site de teste (FILUNA_SITE_TESTE no programa).
	var API_LOGIN = API;
	var pendentes = {}, seq = 0, token = "", ouvintes = {};
	// Moeda da conta (versão americana): vem do site em /conta e /resumo ("moeda": "USD" ou "BRL"), fica guardada no
	// computador (funciona sem internet) e no app_config ("filuna_moeda", lido pelas Configurações em C++).
	// Sem moeda conhecida = real, como sempre.
	var moeda = "BRL";

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
		// Onde ficam os dados da conta: o site de sempre, ou o de teste que o programa indicar.
		var base = nativo("site.base").then(function (r) { if (r && r.base) API = r.base + "/api/slicer/v1"; }, function () {});
		base.then(function () { return pendentesLocais(); }).then(function (f) { conexao.pendentes = f.length; if (f.length) enviarFila(); });
		// O Filuna Slicer guarda a conta; o armazenamento da página só vale fora do programa. A moeda guardada vem antes.
		return base.then(function () { return lerLocal("conta.moeda"); }).then(function (m) { if (m == "USD" || m == "BRL") moeda = m; }, function () {})
			.then(function () { return nativo("config.ler", { chave: "filuna_token" }); }).then(function (r) {
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

	// Arquivo no corpo (envio do projeto para a Farm) e imagens que precisam da conta (foto da peça).
	function enviarArquivo(rota, dados) {
		if (!token) return Promise.reject({ status: 401, erro: "Entre na sua conta Filuna." });
		return fetch(API + rota, { method: "POST", headers: { "Authorization": "Bearer " + token, "Content-Type": "application/octet-stream" }, body: dados })
			.then(function (r) { return r.json().catch(function () { return {}; }).then(function (d) { if (!r.ok) throw { status: r.status, erro: d.erro || ("Erro " + r.status) }; return d; }); },
				function () { throw { status: 0, erro: "Sem conexão." }; });
	}
	function imagem(rota) {
		if (!token) return Promise.reject({ status: 401 });
		return fetch(API + rota, { headers: { "Authorization": "Bearer " + token } }).then(function (r) { if (!r.ok) throw { status: r.status }; return r.blob(); });
	}

	function marcarConexao(online) {
		if (conexao.online === online) return;
		conexao.online = online;
		mostrarConexao();
		_evento("conexao", { online: online });
		if (online) enviarFila();
	}

	// Guarda a moeda que o site mandou (só grava no programa quando muda).
	function lembrarMoeda(d) {
		var m = d && d.moeda;
		if ((m != "USD" && m != "BRL") || m == moeda) return;
		moeda = m;
		guardarLocal("conta.moeda", m);
		nativo("config.gravar", { chave: "filuna_moeda", valor: m }).catch(function () {});
		_evento("moeda", { moeda: m });
	}

	function api(metodo, rota, corpo) {
		if (!token) return Promise.reject({ status: 401, erro: "Entre na sua conta Filuna para ver seus dados." });
		if (metodo == "GET") {
			return rede(metodo, rota).then(function (d) {
				marcarConexao(true);
				lembrarMoeda(d);
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
		return rede(metodo, rota, corpo).then(function (d) { marcarConexao(true); lembrarMoeda(d); return d; }, function (e) {
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
		"calculo.energia_kwh": "num", "calculo.potencia_w": "num", "calculo.preco_kg": "num",
		"calculo.manutencao_hora": "num", "calculo.desgaste_hora": "num" };
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
		return fetch(API_LOGIN + "/entrar", {
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
	function bobinaSVG(cor, px, nivel) {
		// Vista 3/4 pela frente-direita: aba de trás (só a borda aparece à esquerda), o filamento enrolado
		// entre as abas e a aba da frente grande, perfurada, com três janelas que mostram o filamento.
		// nivel (opcional, 0 a 1): quanto filamento resta, desenhado em passos de 10% (0 = carretel vazio; sem nivel = cheia).
		// As janelas ficam no grupo .bobina-giro, que gira no plano da aba quando um elemento acima tem a classe "girando".
		var id = "fb" + (++nBobina), c = corRgb(cor), lum = (0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]) / 255;
		var claro = lum > 0.85 ? "#FFFFFF" : misturar(c, [255, 255, 255], 0.5);
		var meio = "rgb(" + c.join(",") + ")";
		var escuro = misturar(c, [0, 0, 0], lum > 0.85 ? 0.24 : 0.5), fundo = misturar(c, [0, 0, 0], lum > 0.85 ? 0.36 : 0.65);
		var fio = misturar(c, [0, 0, 0], lum > 0.85 ? 0.2 : 0.35);
		var A = { cx: 96, cy: 70, rx: 46, ry: 64 };   // aba da frente
		var B = { cx: 50, cy: 70, rx: 46, ry: 64 };   // aba de trás (mesmo tamanho, deslocada)
		// de 10 em 10%; o que ainda tem filamento nunca aparece vazio (45 g de 1 kg mostra 10%)
		var passo = nivel == null || isNaN(nivel) ? 1 : +nivel > 0 ? Math.max(0.1, Math.round(Math.min(1, +nivel) * 10) / 10) : 0;
		var CUBO = 0.3;                              // raio do cubo do carretel em relação à aba
		var R = passo > 0 ? 0.42 + 0.42 * passo : 0; // raio do filamento enrolado: 0,84 da aba com a bobina cheia
		var achata = A.rx / A.ry;                    // a aba é um círculo de raio A.ry visto de lado
		function noCirculo(f, graus) { var t = graus * Math.PI / 180; return [A.ry * f * Math.sin(t), -A.ry * f * Math.cos(t)]; }
		function janela(centro) {
			var p = [], g;
			for (g = centro - 30; g <= centro + 30; g += 5) p.push(noCirculo(0.82, g));
			for (g = centro + 20; g >= centro - 20; g -= 5) p.push(noCirculo(0.4, g));
			return "M" + p.map(function (q) { return q[0].toFixed(1) + " " + q[1].toFixed(1); }).join(" L") + " Z";
		}
		function cilindro(r, tinta) {
			return '<path d="M' + B.cx + " " + (A.cy - A.ry * r).toFixed(1) + " H" + A.cx + " V" + (A.cy + A.ry * r).toFixed(1) + " H" + B.cx + ' Z" fill="' + tinta + '"/>';
		}
		function ponta(r, tinta) {
			return '<ellipse cx="' + B.cx + '" cy="' + A.cy + '" rx="' + (B.rx * r).toFixed(1) + '" ry="' + (A.ry * r).toFixed(1) + '" ' + tinta + "/>";
		}
		var filamento = "";
		if (R) {
			var voltas = "", n = Math.round(22 * R / 0.84);
			for (var k = 1; k < n; k++) {
				var y = (A.cy - A.ry * R + k * (2 * A.ry * R) / n).toFixed(1);
				voltas += '<line x1="' + (B.cx - 6) + '" x2="' + A.cx + '" y1="' + y + '" y2="' + y + '" stroke="' + fio + '" stroke-opacity=".18" stroke-width=".7"/>';
			}
			filamento = cilindro(R, "url(#" + id + "f)") + voltas + ponta(R, 'fill="url(#' + id + 'f)"') + ponta(R, 'fill="none" stroke="' + fio + '" stroke-opacity=".25"');
		}
		// Imprimindo: o fio sai do alto do filamento enrolado e corre para a impressora (só aparece com "girando").
		function fioSaindo() {
			if (!R) return "";
			// sai do alto do rolo, no trecho entre a aba de trás e a da frente que aparece, sobe por trás da aba da frente
			// e segue para a direita, acima da bobina (o SVG deixa desenhar fora da caixa)
			var x0 = B.cx + 10, y0 = A.cy - A.ry * R + 2;
			var d = "M" + x0 + " " + y0.toFixed(1) + " C" + (x0 - 2) + " " + (y0 - 24).toFixed(1) + " " + (x0 + 30) + " -16 " + (x0 + 70) + " -14 S 160 -4 178 -16";
			return '<g class="bobina-fio"><path d="' + d + '" fill="none" stroke="' + escuro + '" stroke-width="5" stroke-linecap="round"/>' +
				'<path d="' + d + '" fill="none" stroke="' + meio + '" stroke-width="3.2" stroke-linecap="round"/>' +
				'<path class="bobina-fio-mov" d="' + d + '" fill="none" stroke="' + claro + '" stroke-width="1.4" stroke-linecap="round" stroke-dasharray="4 10" stroke-opacity=".9"/></g>';
		}
		var janelas = [0, 120, 240].map(function (g) {
			var d = janela(g);
			if (passo == 1) return '<path d="' + d + '" fill="url(#' + id + 'j)" stroke="#2B3050" stroke-width="3" stroke-linejoin="round"/>';
			return '<path d="' + d + '" fill="#232741" stroke="none"/>' +
				(R ? '<path d="' + d + '" fill="url(#' + id + 'j)" stroke="none" clip-path="url(#' + id + 'r)"/>' : "") +
				'<path d="' + d + '" fill="none" stroke="#2B3050" stroke-width="3" stroke-linejoin="round"/>';
		}).join("");
		return '<svg class="bobina-svg" viewBox="0 0 150 140" width="' + (px || 64) + '" height="' + (px || 64) + '" aria-hidden="true">' +
			'<defs>' +
			'<linearGradient id="' + id + 'f" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="' + escuro + '"/><stop offset=".18" stop-color="' + meio + '"/>' +
			'<stop offset=".32" stop-color="' + claro + '"/><stop offset=".5" stop-color="' + meio + '"/><stop offset="1" stop-color="' + fundo + '"/></linearGradient>' +
			'<linearGradient id="' + id + 'j" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="' + claro + '"/><stop offset=".45" stop-color="' + meio + '"/><stop offset="1" stop-color="' + escuro + '"/></linearGradient>' +
			'<linearGradient id="' + id + 'a" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#5A6390"/><stop offset=".55" stop-color="#3E4568"/><stop offset="1" stop-color="#2B3050"/></linearGradient>' +
			'<pattern id="' + id + 'p" width="5.2" height="9" patternUnits="userSpaceOnUse"><circle cx="1.3" cy="2.2" r="1.05" fill="#1E2136"/><circle cx="3.9" cy="6.7" r="1.05" fill="#1E2136"/></pattern>' +
			'<clipPath id="' + id + 'c"><ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.9) + '" ry="' + (A.ry * 0.9) + '"/></clipPath>' +
			(R && passo < 1 ? '<clipPath id="' + id + 'r"><circle r="' + (A.ry * R).toFixed(1) + '"/></clipPath>' : "") +
			'</defs>' +
			// aba de trás: só a borda visível à esquerda
			'<ellipse cx="' + B.cx + '" cy="' + B.cy + '" rx="' + B.rx + '" ry="' + B.ry + '" fill="#2B3050"/>' +
			'<ellipse cx="' + (B.cx + 2.5) + '" cy="' + B.cy + '" rx="' + (B.rx - 2) + '" ry="' + (B.ry - 2) + '" fill="#363C60"/>' +
			// cubo do carretel (aparece quando falta filamento) e o filamento: cilindros entre as abas
			(passo < 1 ? cilindro(CUBO, "#4B5380") + ponta(CUBO, 'fill="#3E4568"') : "") + filamento + fioSaindo() +
			// espessura e face da aba da frente
			'<ellipse cx="' + (A.cx - 3) + '" cy="' + A.cy + '" rx="' + A.rx + '" ry="' + A.ry + '" fill="#252946"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + A.rx + '" ry="' + A.ry + '" fill="url(#' + id + 'a)"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.95) + '" ry="' + (A.ry * 0.95) + '" fill="none" stroke="#7079A6" stroke-opacity=".55" stroke-width="1"/>' +
			'<g clip-path="url(#' + id + 'c)"><rect x="' + (A.cx - A.rx) + '" y="' + (A.cy - A.ry) + '" width="' + (2 * A.rx) + '" height="' + (2 * A.ry) + '" fill="url(#' + id + 'p)" opacity=".85"/></g>' +
			// janelas mostrando o filamento, no plano da aba (círculo achatado) para poderem girar
			'<g transform="translate(' + A.cx + " " + A.cy + ") scale(" + achata.toFixed(4) + ' 1)"><g class="bobina-giro">' + janelas + "</g></g>" +
			// miolo
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.3) + '" ry="' + (A.ry * 0.3) + '" fill="#4B5380"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.24) + '" ry="' + (A.ry * 0.24) + '" fill="none" stroke="#6C76A3" stroke-width="1"/>' +
			'<ellipse cx="' + A.cx + '" cy="' + A.cy + '" rx="' + (A.rx * 0.18) + '" ry="' + (A.ry * 0.18) + '" fill="none" stroke="#6C76A3" stroke-opacity=".7" stroke-width=".8"/>' +
			'<ellipse cx="' + (A.cx + 1) + '" cy="' + A.cy + '" rx="' + (A.rx * 0.12) + '" ry="' + (A.ry * 0.12) + '" fill="#171A2C"/>' +
			'</svg>';
	}
	function gramas(g) { g = Number(g || 0); return g >= 1000 ? (g / 1000).toFixed(2).replace(".", ",") + " kg" : Math.round(g) + " g"; }
	function tempo(s) { s = Math.max(0, Math.round(s || 0)); var h = Math.floor(s / 3600), m = Math.round((s % 3600) / 60); if (m == 60) { h++; m = 0; } return (h ? h + "h" : "") + (m < 10 && h ? "0" : "") + m + "min"; }
	// Dinheiro na moeda da conta: real "R$ 12,50" (como sempre) ou dólar "$1,234.56".
	function reais(v) {
		if (moeda == "USD") {
			var n = Number(v || 0);
			return (n < 0 ? "-$" : "$") + Math.abs(n).toLocaleString("en-US", { minimumFractionDigits: 2, maximumFractionDigits: 2 });
		}
		return "R$ " + Number(v || 0).toFixed(2).replace(".", ",");
	}
	function simbolo() { return moeda == "USD" ? "$" : "R$"; }
	// Valor que volta para um campo de texto: "12,50" no Brasil, "12.50" nos EUA.
	function valorCampo(v) { var t = Number(v || 0).toFixed(2); return moeda == "USD" ? t : t.replace(".", ","); }
	// Valor digitado -> número (null se vazio ou inválido). Real: vírgula é decimal; dólar: "1,234.56", "$12.50".
	function lerValor(texto) {
		var t = String(texto == null ? "" : texto);
		if (moeda == "USD") {
			t = t.replace(/[^0-9,.\-]/g, "");
			t = /^-?\d+,\d{1,2}$/.test(t) ? t.replace(",", ".") : t.replace(/,/g, "");
		} else t = t.replace(/[^0-9,\.]/g, "").replace(",", ".");
		if (t === "") return null;
		var n = parseFloat(t);
		return isFinite(n) ? n : null;
	}
	function num(v, casas) { return Number(v || 0).toFixed(casas == null ? 2 : casas).replace(".", ","); }
	// Seção "modelo:<slug>" (aba.abrir de outra tela, ex.: cartão da tela inicial): abre a página do modelo.
	function abrirModeloPedido(secao) {
		if (!secao || secao.indexOf("modelo:") != 0) return false;
		location.href = "modelo-3d.html?slug=" + encodeURIComponent(secao.slice(7));
		return true;
	}
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
	// Diagnóstico: erro de script em qualquer tela Filuna vai para o servidor (o dono vê em /admin/diagnostico).
	window.addEventListener("error", function (ev) {
		nativo("diagnostico.enviar", { etapa: location.pathname.split("/").pop(), mensagem: String(ev.message || "erro de script"),
			contexto: { arquivo: String(ev.filename || "").split("/").pop(), linha: ev.lineno || 0 } }).catch(function () {});
	});
	setInterval(function () { if (conexao.pendentes) enviarFila(); }, 60000);
	ouvir("preferencias.locais", function () { enviarPrefs().then(function () { _evento("preferencias", {}); }); });

	// ---- Suporte remoto (só com autorização do cliente, dada na tela inicial; prazo e registro ficam no servidor) ----
	// O suporte da Filuna enfileira ações de uma LISTA FECHADA; este programa executa só as que estão aqui e devolve
	// o resultado. Nada de comando livre, nada fora do Filuna. Sem autorização em vigor o servidor não entrega nada.
	var SUP_ACOES = ["coletar", "diagnostico.enviar", "atualizacao.verificar", "atualizacao.aplicar", "dispositivos.fabricantes",
		"dispositivos.modelos", "dispositivos.complemento", "dispositivos.complemento.baixar", "dispositivos.preparar",
		"dispositivos.descobertas", "dispositivos.estado", "recarregar", "dispositivos.esquecer", "dispositivos.reconectar", "farm.destravar",
		// assistência remota: rede do cliente, conexão, impressora, fatiador e impressão de teste
		"rede.diagnostico", "rede.procurar", "dispositivos.conectar_ip", "dispositivos.selecionar", "dispositivos.comando",
		"dispositivos.controle", "fatiador.estado", "fatiador.ajustar_impressora", "impressao.teste", "impressao.teste.estado",
		// fatiador completo (categoria "fatiador", só se o cliente marcou): as mesmas ações do Assistente de impressão
		"fatiador.pecas", "fatiador.processo", "fatiador.ajustar", "fatiador.perfis", "fatiador.perfil", "fatiador.mesa", "fatiador.orientar",
		"fatiador.arrumar", "fatiador.fatiar", "fatiador.fatiamento", "fatiador.avisos", "fatiador.imprimir",
		// Raio X do equipamento: leitura completa, catálogo, testes (os que aquecem/movem esperam o cliente confirmar
		// presença NESTA tela; "diagnostico.presenca" nunca vem de fora), andamento e parar
		"diagnostico.ler", "diagnostico.testes", "diagnostico.testar", "diagnostico.teste.estado", "diagnostico.parar"];
	// ação do suporte → ação do programa (as do fatiador completo); as outras têm o mesmo nome
	var SUP_NATIVO = { "fatiador.pecas": "peca.analise", "fatiador.processo": "processo.ler", "fatiador.perfis": "perfil.lista",
		"fatiador.perfil": "perfil.trocar", "fatiador.orientar": "peca.orientar", "fatiador.arrumar": "peca.arrumar", "fatiador.fatiar": "fatiar",
		"fatiador.avisos": "fatiamento.avisos", "fatiador.imprimir": "imprimir" };
	// mesma regra do servidor (modulos.py suporte_categoria): o programa confere de novo antes de executar
	function supCategoria(acao) {
		if (acao == "diagnostico.testar") return "impressora"; // teste físico: só com a categoria Impressora
		if (acao == "fatiador.estado" || acao == "coletar" || /^diagnostico\./.test(acao) || acao == "rede.diagnostico" ||
			["dispositivos.estado", "dispositivos.fabricantes", "dispositivos.modelos", "dispositivos.descobertas"].indexOf(acao) >= 0) return "diagnostico";
		if (acao == "fatiador.ajustar_impressora") return "reparo";
		if (/^fatiador\./.test(acao)) return "fatiador";
		if (/^impressao\./.test(acao) || acao == "dispositivos.comando" || acao == "dispositivos.controle") return "impressora";
		return "reparo";
	}
	// campos que o suporte pode mandar em cada ação (só texto, número e sim/não; nada de objeto ou lista)
	var SUP_CAMPOS = ["fabricante", "modelo", "dev_id", "ip", "codigo", "serie", "nome", "material", "posicao", "cmd", "tipo", "ligar", "valor",
		"eixo", "eixos", "distancia", "temperatura", "unidade", "slot", "chave", "valor_ajuste", "acao_mesa", "peca", "fator", "maior_lado_mm",
		"graus", "quantidade", "teste"];
	function supDados(d) {
		var r = {};
		SUP_CAMPOS.forEach(function (k) {
			var v = d[k];
			if (typeof v == "string" || typeof v == "number" || typeof v == "boolean") r[k] = v;
		});
		return r;
	}
	function supSeguro(acao, dados) { return nativo(acao, dados).catch(function (e) { return { erro: (e && e.message) || "falhou" }; }); }
	function supExecutar(c, categorias) {
		if (SUP_ACOES.indexOf(c.acao) < 0) return Promise.resolve({ erro: "ação fora da lista permitida" });
		// servidor antigo, sem categorias: vale o que valia antes (sem o fatiador completo)
		var cats = categorias || ["diagnostico", "reparo", "impressora"];
		if (cats.indexOf(supCategoria(c.acao)) < 0) return Promise.resolve({ erro: "categoria não autorizada pelo cliente" });
		var d = c.dados || {};
		if (c.acao == "fatiador.ajustar") {
			if (!d.chave) return Promise.resolve({ erro: "informe o ajuste (chave)" });
			var v = {}; v[String(d.chave)] = String(d.valor_ajuste == null ? "" : d.valor_ajuste);
			return supSeguro("processo.ajustar", { valores: v });
		}
		if (c.acao == "fatiador.mesa") {
			var m = supDados(d); m.acao = m.acao_mesa; delete m.acao_mesa;
			return supSeguro("mesa.comando", m);
		}
		if (c.acao == "fatiador.fatiamento")
			return Promise.all([supSeguro("fatiamento.estado"), supSeguro("fatiamento.resumo")]).then(function (r) { return { estado: r[0], resumo: r[1] }; });
		if (SUP_NATIVO[c.acao]) return supSeguro(SUP_NATIVO[c.acao], supDados(d));
		if (c.acao == "recarregar") { setTimeout(function () { location.reload(); }, 1500); return Promise.resolve({ recarregando: true }); }
		if (c.acao == "diagnostico.enviar") return supSeguro("diagnostico.enviar", { etapa: "suporte-remoto", mensagem: "Registro pedido pelo suporte", contexto: {} });
		if (c.acao == "coletar")
			return Promise.all([supSeguro("atualizacao.estado"), supSeguro("dispositivos.fabricantes"), supSeguro("dispositivos.estado"),
				supSeguro("dispositivos.descobertas"), supSeguro("preferencias.ler")]).then(function (r) {
				return { programa: r[0], marcas: r[1], impressoras: r[2], rede_local: r[3], preferencias: r[4],
					tela: location.pathname.split("/").pop(), sistema: navigator.userAgent, hora: new Date().toISOString() };
			});
		if (c.acao == "diagnostico.ler" || c.acao == "diagnostico.testar") {
			var dd = supDados(d); dd.origem = "suporte";
			return supSeguro(c.acao, dd);
		}
		return supSeguro(c.acao, supDados(d));
	}
	function supCiclo() {
		if (!token) { setTimeout(supCiclo, 120000); return; }
		rede("GET", "/suporte/fila").then(function (r) {
			var fila = (r && r.acoes) || [], espera = r && r.ativa ? 30000 : 300000;
			var cadeia = Promise.resolve();
			fila.forEach(function (c) {
				cadeia = cadeia.then(function () { return supExecutar(c, r.categorias); }).then(function (res) {
					return rede("POST", "/suporte/resultado", { id: c.id, ok: !(res && res.erro), resultado: res }).catch(function () {});
				});
			});
			cadeia.then(function () { setTimeout(supCiclo, fila.length ? 5000 : espera); });
		}, function () { setTimeout(supCiclo, 300000); });
	}
	// um só agente por programa: o rodapé está sempre carregado
	if (/rodape\.html$/.test(location.pathname)) setTimeout(supCiclo, 20000);
	ouvir("suporte.agora", function () { supCiclo(); });

	// ---- Peças geradas pelo assistente (Claude ou ChatGPT ligados à conta Filuna) ----
	// O cliente pede ao assistente, o site gera a peça (ex.: Chaveiro com logo) e guarda. Este programa pergunta se
	// há algo esperando e abre na Preparação pelo mesmo caminho dos modelos do catálogo: baixa com a chave do
	// aparelho e abre travado para salvar e exportar. O site entrega cada peça uma vez só.
	var iaAbrindo = "";
	function iaEstado(id, estado) { return rede("POST", "/modelos3d/envios/" + id, { estado: estado }).catch(function () {}); }
	function iaCiclo() {
		if (!token) { setTimeout(iaCiclo, 60000); return; }
		rede("GET", "/ia/envios").then(function (r) {
			var fila = (r && r.envios) || [], cadeia = Promise.resolve();
			fila.forEach(function (e) {
				cadeia = cadeia.then(function () {
					iaAbrindo = e.id;
					return nativo("modelo3d.abrir", { id: e.id, titulo: e.titulo || "Peça do assistente" }).catch(function () { iaAbrindo = ""; return iaEstado(e.id, "falhou"); });
				});
			});
			cadeia.then(function () { setTimeout(iaCiclo, 10000); });
		}, function (e) { setTimeout(iaCiclo, e && e.status == 404 ? 600000 : 60000); });   // 404: o site está com os assistentes desligados
	}
	ouvir("modelo3d.aberto", function () { iaAbrindo = ""; });
	ouvir("modelo3d.erro", function (d) {
		if (!iaAbrindo) return;
		iaEstado(iaAbrindo, d && d.erro == "abertura cancelada" ? "cancelado" : "falhou");
		iaAbrindo = "";
	});
	if (/rodape\.html$/.test(location.pathname)) setTimeout(iaCiclo, 12000);

	return { nativo: nativo, api: api, enviarArquivo: enviarArquivo, imagem: imagem, iniciar: iniciar, plano: plano, prefs: prefs, enviarPrefs: enviarPrefs, lerLocal: lerLocal, guardarLocal: guardarLocal,
		pendentesLocais: pendentesLocais, conexao: function () { return conexao; }, entrar: entrar, sair: sair, ouvir: ouvir, _resposta: _resposta, _evento: _evento,
		temToken: function () { return !!token; }, esc: esc, bobinaSVG: bobinaSVG, corRgb: corRgb, gramas: gramas, tempo: tempo, reais: reais, num: num,
		dinheiro: reais, simbolo: simbolo, valorCampo: valorCampo, lerValor: lerValor, moeda: function () { return moeda; }, abrirLink: abrirLink, abrirModeloPedido: abrirModeloPedido, telaLogin: telaLogin };
})();
