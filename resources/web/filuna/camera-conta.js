/* Filuna — câmera das impressoras para a conta (site filuna3d.com e app do celular).
 *
 * O site e o app não alcançam a rede local da impressora: quem vê a câmera é este computador. Enquanto o
 * Filuna Slicer está aberto, este módulo tira uma foto da câmera de cada impressora conectada (pelo repasse
 * local, que já libera a leitura da imagem) e manda para a conta em
 * POST /api/slicer/v1/impressoras/<cadastro_id>/camera (JPEG até 600 KB). O site e o app mostram a última foto.
 *
 * - Impressora aberta na tela: usa o vídeo/imagem que já está tocando (a cada 5 s).
 * - As outras: abre a câmera por alguns segundos, tira a foto e fecha (a cada 20 s; 60 s se estiver parada).
 * - Só uma página do Slicer envia por vez (trava com batimento no armazenamento local).
 * - Sem conta conectada ou sem câmera: não faz nada.
 */
(function () {
	if (window.FilunaCameraConta || !window.Filuna) return;
	var API = "https://filuna3d.com/api/slicer/v1";
	var EU = Math.random().toString(36).slice(2), TRAVA = "fl.camconta.trava";
	var VISTA_MS = 5000, OUTRAS_MS = 20000, PARADA_MS = 60000, LARGURA = 1280, QUALIDADE = 0.72, MAX = 600 * 1024;
	var ultimo = {}, ocupado = {}, token = "";

	function travar() {
		try {
			var t = JSON.parse(localStorage.getItem(TRAVA) || "null"), agora = Date.now();
			// a tela de Dispositivos tem preferência (é a que já está com a câmera aberta)
			var prefiro = /dispositivo\.html/.test(location.pathname);
			if (t && t.dono != EU && agora - t.em < 15000 && !(prefiro && !t.disp)) return false;
			localStorage.setItem(TRAVA, JSON.stringify({ dono: EU, em: agora, disp: prefiro }));
			return true;
		} catch (e) { return true; }
	}

	function lerToken() {
		return Filuna.nativo("config.ler", { chave: "filuna_token" }).then(function (r) { return (r && r.valor) || ""; }, function () {
			try { return localStorage.getItem("filuna_token") || ""; } catch (e) { return ""; }
		});
	}

	function quadro(el) {
		var w = el.videoWidth || el.naturalWidth, h = el.videoHeight || el.naturalHeight;
		if (!w || !h) return Promise.resolve(null);
		var esc = Math.min(1, LARGURA / w), c = document.createElement("canvas");
		c.width = Math.round(w * esc); c.height = Math.round(h * esc);
		try { c.getContext("2d").drawImage(el, 0, 0, c.width, c.height); } catch (e) { return Promise.resolve(null); }
		return new Promise(function (ok) {
			var tentar = function (q) {
				try {
					c.toBlob(function (b) { if (b && b.size > MAX && q > 0.4) tentar(q - 0.15); else ok(b && b.size <= MAX ? b : null); }, "image/jpeg", q);
				} catch (e) { ok(null); } // imagem sem permissão de leitura (sem repasse)
			};
			tentar(QUALIDADE);
		});
	}

	// abre a câmera escondida só pelo tempo de pegar uma imagem
	function fotoAvulsa(v) {
		return new Promise(function (ok) {
			var fim = false, el, tocador = null, espera, caixa = document.createElement("div");
			function terminar(b) {
				if (fim) return; fim = true; clearTimeout(espera);
				try { if (tocador) tocador.destroy(); } catch (e) {}
				try { if (el) { el.removeAttribute("src"); if (el.load) el.load(); } caixa.remove(); } catch (e) {}
				ok(b || null);
			}
			function quandoTiver(tent) {
				if (fim) return;
				if ((el.videoWidth || el.naturalWidth) > 0) { setTimeout(function () { quadro(el).then(terminar); }, 400); return; }
				if (tent > 40) return terminar(null);
				setTimeout(function () { quandoTiver(tent + 1); }, 250);
			}
			espera = setTimeout(function () { terminar(null); }, 15000);
			caixa.style.cssText = "position:fixed;left:-10000px;top:0;width:2px;height:2px;overflow:hidden;opacity:0;pointer-events:none";
			if (v.camera_flv) {
				if (!window.mpegts || !mpegts.isSupported()) return terminar(null);
				el = document.createElement("video"); el.muted = true; el.playsInline = true; el.crossOrigin = "anonymous";
				caixa.appendChild(el); document.body.appendChild(caixa);
				try {
					tocador = mpegts.createPlayer({ type: "flv", isLive: true, url: v.camera_flv }, { enableStashBuffer: false, lazyLoad: false });
					tocador.on(mpegts.Events.ERROR, function () { terminar(null); });
					tocador.attachMediaElement(el); tocador.load(); var p = tocador.play(); if (p && p.catch) p.catch(function () {});
				} catch (e) { return terminar(null); }
			} else {
				el = new Image(); el.crossOrigin = "anonymous";
				caixa.appendChild(el); document.body.appendChild(caixa);
				el.onerror = function () { terminar(null); };
				el.src = v.camera_repasse;
			}
			quandoTiver(0);
		});
	}

	function enviar(id, blob) {
		return fetch(API + "/impressoras/" + encodeURIComponent(id) + "/camera", {
			method: "POST", headers: { "Authorization": "Bearer " + token, "Content-Type": "image/jpeg" }, body: blob
		}).then(function (r) { return r.ok; }, function () { return false; });
	}

	function naTela(v) {
		var el = document.querySelector("[data-cam]");
		if (!el || !window.estado || estado.vista != "unidade") return null;
		var src = el.getAttribute("data-src") || "";
		return src && (src == v.camera_flv || src == v.camera_repasse) ? el : null;
	}

	function volta() {
		if (!travar()) return;
		lerToken().then(function (t) {
			token = t; if (!token) return;
			return Filuna.nativo("dispositivos.estado").then(function (e) {
				(e && e.impressoras || []).forEach(function (v) {
					var id = v.cadastro_id || "";
					if (!id || !v.ligada || !(v.camera_flv || v.camera_repasse) || ocupado[id]) return;
					var el = naTela(v), agora = Date.now();
					var intervalo = el ? VISTA_MS : (v.estado == "imprimindo" || v.estado == "pausada" ? OUTRAS_MS : PARADA_MS);
					if (agora - (ultimo[id] || 0) < intervalo) return;
					ultimo[id] = agora; ocupado[id] = true;
					(el ? quadro(el) : fotoAvulsa(v)).then(function (b) { return b ? enviar(id, b) : false; })
						.then(function () { ocupado[id] = false; }, function () { ocupado[id] = false; });
				});
			});
		}).catch(function () {});
	}

	function comecar() {
		if (!window.mpegts) { // páginas sem o tocador de vídeo (Anycubic usa FLV)
			var s = document.createElement("script"); s.src = "mpegts.js"; document.head.appendChild(s);
		}
		setTimeout(volta, 3000);
		setInterval(volta, VISTA_MS);
	}
	// foto: o check-up do Raio X (diagnostico.js) confere se a câmera manda imagem
	window.FilunaCameraConta = { volta: volta, foto: fotoAvulsa };
	if (document.readyState == "loading") document.addEventListener("DOMContentLoaded", comecar); else comecar();
})();
