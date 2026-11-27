/* Telas Filuna em inglês: tradução por dicionário (i18n/en.json), feita na própria página.
 * - Só age quando o idioma do programa começa com "en". Em português não muda nada.
 * - O programa injeta este arquivo e o dicionário antes da página carregar (FilunaPanel.cpp), então a tela já nasce em
 *   inglês. Fora disso (ponte.js carrega este arquivo), pergunta o idioma ao programa e traduz o que já está na tela.
 * - Traduz nós de texto e os atributos placeholder, title, aria-label, alt e value de botões, e tudo que entrar depois
 *   (MutationObserver). Não mexe em campos de texto, código, nem em elementos com translate="no" ou class "sem-traducao".
 * - Dicionário: "frases" (texto inteiro), "padroes" (com {0}, {1}... para números, nomes e preços) e "trechos" (pedaços
 *   trocados dentro de um texto maior). Frases com duas ou mais palavras também valem como trecho.
 * FilunaI18n.t(texto) traduz um texto qualquer (alert, prompt, texto montado no código). */
(function () {
	if (window.FilunaI18n) return;
	var ATRIBUTOS = ["placeholder", "title", "aria-label", "alt"];
	var PULAR = { SCRIPT: 1, STYLE: 1, TEXTAREA: 1, INPUT: 1, CODE: 1, PRE: 1, NOSCRIPT: 1 };
	var LETRA = /[A-Za-zÀ-ÿ]/;
	var ativo = false, frases = {}, padroes = [], trechos = new Map(), cache = new Map(), feito = new WeakMap();
	var idioma = String(window.FILUNA_IDIOMA || "");

	function norm(s) { return String(s).replace(/\s+/g, " ").trim(); }
	function escRe(s) { return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"); }
	function letraOuNumero(c) { return !!c && /[\p{L}\p{N}]/u.test(c); }

	function preparar(d) {
		frases = (d && d.frases) || {};
		var lista = [];
		Object.keys((d && d.padroes) || {}).forEach(function (k) {
			var en = d.padroes[k];
			if (!en) return;
			var fixos = k.split(/\{\d+\}/), ordem = [], m, re = /\{(\d+)\}/g;
			while ((m = re.exec(k))) ordem.push(+m[1]);
			var fonte = "^" + fixos.map(escRe).join("(.*?)") + "$";
			var maior = fixos.reduce(function (a, b) { return b.trim().length > a.length ? b.trim() : a; }, "");
			lista.push({ re: new RegExp(fonte), en: en, ordem: ordem, fixo: maior, peso: fixos.join("").length });
		});
		// o padrão com mais texto fixo ganha ("Filamento {0} trocado" antes de "Filamento {0}")
		padroes = lista.sort(function (a, b) { return b.peso - a.peso; });
		var pedacos = {};
		Object.keys(frases).forEach(function (k) { if (frases[k] && k.indexOf(" ") > 0 && k.length >= 6) pedacos[k] = frases[k]; });
		Object.keys((d && d.trechos) || {}).forEach(function (k) { if (d.trechos[k]) pedacos[k] = d.trechos[k]; });
		trechos = new Map();
		Object.keys(pedacos).sort(function (a, b) { return b.length - a.length; }).forEach(function (k) {
			var w = (k.match(/[\p{L}\p{N}]+/u) || [""])[0];
			if (!w || k.indexOf(w) != 0) return;   // trecho que começa com pontuação não entra na busca por palavra
			if (!trechos.has(w)) trechos.set(w, []);
			trechos.get(w).push([k, pedacos[k]]);
		});
		cache = new Map();
	}

	// Sufixo de plural do português que vem numa variável ("peça{1}" com "s"/"es"): em inglês é "s".
	function captura(c, prof) {
		if (/^(s|es|is|ns|ões|ães)$/.test(c)) return "s";
		if (!c || !LETRA.test(c)) return c;
		var r = traduzir(c, prof + 1);
		return r == null ? c : r;
	}

	function porPedacos(s) {
		var out = "", i = 0, mudou = false, n = s.length;
		while (i < n) {
			var c = s[i];
			if (letraOuNumero(c) && !letraOuNumero(s[i - 1])) {
				var m = /^[\p{L}\p{N}]+/u.exec(s.slice(i, i + 60)), lista = m && trechos.get(m[0]), achou = null;
				if (lista)
					for (var k = 0; k < lista.length; k++)
						if (s.startsWith(lista[k][0], i) && !letraOuNumero(s[i + lista[k][0].length])) { achou = lista[k]; break; }
				if (achou) { out += achou[1]; i += achou[0].length; mudou = true; continue; }
				if (m) { out += m[0]; i += m[0].length; continue; }
			}
			out += c; i++;
		}
		return mudou ? out : null;
	}

	function traduzir(texto, prof) {
		var s = norm(texto);
		if (!s || !LETRA.test(s)) return null;
		if (cache.has(s)) return cache.get(s);
		var r = frases[s] || null;
		if (!r) {
			// símbolos e pontuação nas pontas ("✦ Minha IA", "(opcional)", "Arquivo:"): traduz o miolo
			var m = /^([^\p{L}\p{N}]*)(.*?)([^\p{L}\p{N}]*)$/u.exec(s);
			if (m && (m[1] || m[3]) && m[2] && frases[m[2]]) r = m[1] + frases[m[2]] + m[3];
		}
		if (!r && s.length > 3 && s == s.toUpperCase() && s != s.toLowerCase()) {
			// TÍTULO EM MAIÚSCULAS cujo texto está no dicionário com minúsculas
			var cap = s.charAt(0) + s.slice(1).toLowerCase();
			if (frases[cap]) r = frases[cap].toUpperCase();
		}
		if (!r && (prof || 0) < 3)
			for (var i = 0; i < padroes.length; i++) {
				var p = padroes[i];
				if (p.fixo && s.indexOf(p.fixo) < 0) continue;
				var g = p.re.exec(s);
				if (!g) continue;
				r = p.en.replace(/\{(\d+)\}/g, function (_, n) {
					var pos = p.ordem.indexOf(+n);
					return pos < 0 ? "" : captura(g[pos + 1], prof || 0);
				});
				break;
			}
		if (!r) r = porPedacos(s);
		if (cache.size > 20000) cache.clear();
		cache.set(s, r);
		return r;
	}

	// Mantém os espaços das pontas do texto original (o HTML usa para separar palavras).
	function t(texto) {
		if (!ativo || texto == null) return texto;
		var r = traduzir(texto, 0);
		if (r == null) return texto;
		var str = String(texto);
		return (str.match(/^\s*/)[0]) + r + (str.match(/\s*$/)[0]);
	}

	function pular(el) {
		for (var e = el; e && e.nodeType == 1; e = e.parentNode) {
			if (PULAR[e.tagName]) return true;
			if (e.isContentEditable || e.getAttribute("translate") == "no" || (e.classList && e.classList.contains("sem-traducao"))) return true;
		}
		return false;
	}

	function texto(no) {
		if (feito.get(no) === no.data) return;
		var pai = no.parentNode;
		if (!pai || pular(pai)) return;
		var novo = t(no.data);
		if (novo !== no.data) {
			// <option> sem value usa o texto como valor: guarda o original para o código continuar igual
			if (pai.tagName == "OPTION" && !pai.hasAttribute("value")) pai.setAttribute("value", norm(no.data));
			no.data = novo;
		}
		feito.set(no, no.data);
	}

	function atributos(el) {
		if (pular(el)) return;
		var marcas = feito.get(el) || {};
		var lista = ATRIBUTOS.slice();
		if (el.tagName == "INPUT" && /^(button|submit|reset)$/i.test(el.type)) lista.push("value");
		for (var i = 0; i < lista.length; i++) {
			var a = lista[i], v = el.getAttribute(a);
			if (!v || marcas[a] === v) continue;
			var novo = t(v);
			if (novo !== v) el.setAttribute(a, novo);
			marcas[a] = novo;
		}
		feito.set(el, marcas);
	}

	function arvore(raiz) {
		if (!raiz) return;
		if (raiz.nodeType == 3) return texto(raiz);
		if (raiz.nodeType != 1 && raiz.nodeType != 9 && raiz.nodeType != 11) return;
		if (raiz.nodeType == 1) {
			if (PULAR[raiz.tagName] && raiz.tagName != "INPUT") return;
			atributos(raiz);
		}
		var w = document.createTreeWalker(raiz, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, null), n;
		while ((n = w.nextNode())) {
			if (n.nodeType == 3) texto(n);
			else atributos(n);
		}
	}

	function observar() {
		new MutationObserver(function (lista) {
			for (var i = 0; i < lista.length; i++) {
				var m = lista[i];
				if (m.type == "characterData") texto(m.target);
				else if (m.type == "attributes") atributos(m.target);
				else for (var k = 0; k < m.addedNodes.length; k++) arvore(m.addedNodes[k]);
			}
		}).observe(document, { childList: true, subtree: true, characterData: true, attributes: true, attributeFilter: ATRIBUTOS.concat(["value"]) });
	}

	// Avisos do navegador (alert/confirm/prompt) com texto montado no código.
	function caixas() {
		["alert", "confirm", "prompt"].forEach(function (nome) {
			var orig = window[nome];
			if (typeof orig != "function") return;
			window[nome] = function (msg) {
				var a = Array.prototype.slice.call(arguments);
				if (typeof msg == "string") a[0] = String(msg).split("\n").map(function (l) { return t(l); }).join("\n");
				return orig.apply(window, a);
			};
		});
	}

	function iniciar(dic) {
		if (ativo || !dic) return;
		preparar(dic);
		ativo = true;
		idioma = "en";
		caixas();
		observar();
		if (document.documentElement) document.documentElement.lang = "en";
		else document.addEventListener("DOMContentLoaded", function () { document.documentElement.lang = "en"; });
		arvore(document);
	}

	window.FilunaI18n = {
		t: t,
		ativo: function () { return ativo; },
		idioma: function () { return idioma == "en" ? "en" : "pt"; },
		iniciar: iniciar
	};

	if (idioma == "en" && window.FILUNA_I18N) { iniciar(window.FILUNA_I18N); return; }
	if (idioma) return;   // o programa já disse o idioma (português): nada a fazer
	// Sem a injeção do programa: pergunta o idioma pela ponte, quando ela existir.
	setTimeout(function () {
		if (!window.Filuna || !Filuna.nativo) return;
		Filuna.nativo("app.idioma").then(function (r) {
			idioma = r && r.idioma == "en" ? "en" : "pt";
			if (idioma != "en") return;
			return Filuna.nativo("app.dicionario").then(function (d) { iniciar(d); });
		}).catch(function () {});
	}, 0);
})();
