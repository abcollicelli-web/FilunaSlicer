/* Filuna — câmera ao vivo nos cartões da lista de impressoras (Dispositivo). Pedido do dono, 28/11/2026: ver em
 * miniatura todas as impressoras imprimindo.
 *
 * Cada cartão de impressora LIGADA (leitura ao vivo; cadastro não é presença) com câmera mostra um quadro recente,
 * pego do jeito mais leve que a marca oferece. Nenhum vídeo fica tocando:
 *   quadro — foto JPEG pedida de tempos em tempos (câmeras MJPEG com ?action=snapshot: Klipper/Moonraker e outras);
 *   mjpeg  — abre o vídeo MJPEG, guarda o primeiro quadro e fecha (Bambu A1/P1 pela porta 6000, demais MJPEG);
 *   flv    — abre o vídeo FLV (Anycubic, Bambu P2S/X1/H2), guarda o primeiro quadro decodificado e fecha.
 * Só pega quadro dos cartões visíveis (IntersectionObserver), com a aba aberta, no máximo dois ao mesmo tempo, a cada
 * 5 s (10 s no FLV) imprimindo e 20 s (30 s) parada. Sem quadro depois de duas tentativas, o cartão volta à foto.
 *
 *   CamCartoes.fonte(v)        -> { modo, url } ou null (sem câmera utilizável agora)
 *   CamCartoes.usaCamera(v)    -> o cartão deve abrir o espaço da câmera?
 *   CamCartoes.montar(raiz, l) -> liga as câmeras dos cartões [data-camslot] já desenhados; l = impressoras ao vivo
 *   CamCartoes.parar()         -> para tudo (saiu da lista) */
(function () {
	var ESPERA = 12000, MAX_JUNTAS = 2, MAX_LARGURA = 640;
	var VAZIO = "data:image/gif;base64,R0lGODlhAQABAAAAACw=";
	var cams = {}, fila = [], rodando = 0, observador = null, mostrando = true, escondida = false;

	function fonte(v) {
		if (!v || !v.dev_id || !v.ligada) return null;
		var url = String(v.camera_url || ""), rep = String(v.camera_repasse || "");
		if (v.camera_flv) return window.mpegts && mpegts.isSupported && mpegts.isSupported() ? { modo: "flv", url: v.camera_flv } : null;
		if (/\/flv(\?|$)/.test(url)) return null; // FLV sem o repasse local: a tela não lê direto
		if (/^https?:\/\//.test(url) && /action=stream/.test(url)) {
			var foto = url.replace("action=stream", "action=snapshot"), i = rep.indexOf("/cam?u=");
			return { modo: "quadro", url: i > 0 ? rep.slice(0, i) + "/cam?u=" + encodeURIComponent(foto) : foto };
		}
		if (rep) return { modo: "mjpeg", url: rep };
		if (/^https?:\/\//.test(url)) return { modo: "mjpeg", url: url };
		return null;
	}
	function imprimindo(v) { return !!v && (v.estado == "imprimindo" || v.estado == "pausada"); }
	function intervalo(c) {
		var flv = c.modo == "flv";
		return imprimindo(c.v) ? (flv ? 10000 : 5000) : flv ? 30000 : 20000;
	}
	function usaCamera(v) {
		var f = fonte(v);
		if (!f) return false;
		var c = cams[v.dev_id];
		return !c || c.url != f.url || mostra(c);
	}
	// depois de um quadro, aguenta mais falhas antes de voltar à foto (rede oscilando); sem nenhum, duas bastam
	function mostra(c) { return c.falhas < (c.teve ? 4 : 2); }
	function ativo() { return mostrando && !escondida && !document.hidden; }

	function nova(dev, f) {
		var cv = document.createElement("canvas");
		cv.className = "camquadro";
		return { dev: dev, modo: f.modo, url: f.url, canvas: cv, timer: 0, ocupado: false, falhas: 0, teve: false, visivel: false, slot: null, abortar: null, v: null };
	}
	function desenharQuadro(c, fonteImg, w, h) {
		if (!w || !h) return false;
		var esc = Math.min(1, MAX_LARGURA / w);
		c.canvas.width = Math.round(w * esc);
		c.canvas.height = Math.round(h * esc);
		try { c.canvas.getContext("2d").drawImage(fonteImg, 0, 0, c.canvas.width, c.canvas.height); } catch (e) { return false; }
		return true;
	}

	function pegarImagem(c, pronto) {
		var im = new Image(), fim = false, t0 = Date.now(), iv = 0;
		var acabar = function (ok) {
			if (fim) return;
			fim = true;
			clearInterval(iv);
			var feito = ok && desenharQuadro(c, im, im.naturalWidth, im.naturalHeight);
			im.onload = im.onerror = null;
			im.src = VAZIO; // fecha a conexão do vídeo MJPEG
			pronto(feito);
		};
		c.abortar = function () { c.cancelado = true; acabar(false); };
		im.onload = function () { acabar(im.naturalWidth > 0); };
		im.onerror = function () { acabar(false); };
		iv = setInterval(function () { if (im.naturalWidth > 0) acabar(true); else if (Date.now() - t0 > ESPERA) acabar(false); }, 200);
		im.src = c.modo == "quadro" ? c.url + (c.url.indexOf("?") >= 0 ? "&" : "?") + "_=" + Date.now() : c.url;
	}
	function pegarVideo(c, pronto) {
		var vd = document.createElement("video"), tc = null, fim = false, t0 = Date.now(), iv = 0;
		vd.muted = true;
		vd.playsInline = true;
		var acabar = function (ok) {
			if (fim) return;
			fim = true;
			clearInterval(iv);
			var feito = ok && desenharQuadro(c, vd, vd.videoWidth, vd.videoHeight);
			try { tc.pause(); tc.unload(); tc.detachMediaElement(); tc.destroy(); } catch (e) {}
			pronto(feito);
		};
		c.abortar = function () { c.cancelado = true; acabar(false); };
		try {
			tc = mpegts.createPlayer({ type: "flv", isLive: true, url: c.url }, { enableStashBuffer: false, lazyLoad: false, enableWorker: false });
			tc.on(mpegts.Events.ERROR, function () { acabar(false); });
			tc.attachMediaElement(vd);
			tc.load();
			var p = tc.play();
			if (p && p.catch) p.catch(function () {});
		} catch (e) { acabar(false); return; }
		iv = setInterval(function () {
			if (vd.readyState >= 2 && vd.videoWidth > 0 && vd.currentTime > 0) acabar(true);
			else if (Date.now() - t0 > ESPERA) acabar(false);
		}, 200);
	}

	function agendar(c, ms) {
		clearTimeout(c.timer);
		c.timer = setTimeout(function () { c.timer = 0; pedir(c); }, ms);
	}
	function pedir(c) {
		if (c.ocupado || c.na_fila || !cams[c.dev]) return;
		if (!ativo() || !c.visivel || !c.slot || !c.slot.isConnected) return; // volta quando aparecer de novo
		c.na_fila = true;
		fila.push(c);
		andar();
	}
	function andar() {
		while (rodando < MAX_JUNTAS && fila.length) {
			var c = fila.shift();
			c.na_fila = false;
			if (!cams[c.dev] || !ativo() || !c.visivel) continue;
			rodando++;
			c.ocupado = true;
			(c.modo == "flv" ? pegarVideo : pegarImagem)(c, terminou.bind(null, c));
		}
	}
	function terminou(c, ok) {
		rodando--;
		c.ocupado = false;
		c.abortar = null;
		if (c.cancelado) { c.cancelado = false; andar(); return; } // pausa (aba escondida, saiu da lista): não conta como falha
		if (ok) { c.falhas = 0; c.teve = true; c.em = Date.now(); }
		else c.falhas++;
		aplicar(c);
		if (cams[c.dev]) agendar(c, ok ? intervalo(c) : Math.min(60000, 5000 * Math.pow(2, c.falhas)));
		andar();
	}

	// estado visual do cartão: quadro na tela, conectando, ou de volta à foto
	function aplicar(c) {
		var s = c.slot, un = s && s.closest(".un");
		if (!s || !un) return;
		s.classList.toggle("tem", c.teve);
		un.classList.toggle("comcam", mostra(c));
	}

	function verVisibilidade(lista) {
		lista.forEach(function (e) {
			var c = cams[e.target.getAttribute("data-camslot")];
			if (!c || c.slot != e.target) return;
			var antes = c.visivel;
			c.visivel = e.isIntersecting;
			if (c.visivel && !antes && !c.timer && !c.ocupado) pedir(c);
		});
	}

	function montar(raiz, vivas) {
		var porDev = {};
		(vivas || []).forEach(function (v) { if (v && v.dev_id) porDev[v.dev_id] = v; });
		if (!observador && window.IntersectionObserver) observador = new IntersectionObserver(verVisibilidade, { rootMargin: "80px" });
		if (observador) observador.disconnect();
		var vistos = {};
		(raiz ? raiz.querySelectorAll("[data-camslot]") : []).forEach(function (s) {
			var dev = s.getAttribute("data-camslot"), v = porDev[dev], f = fonte(v);
			if (!f) return;
			var c = cams[dev];
			if (c && c.url != f.url) { soltar(c); c = null; }
			if (!c) c = cams[dev] = nova(dev, f);
			vistos[dev] = true;
			c.v = v;
			c.slot = s;
			s.insertBefore(c.canvas, s.firstChild);
			aplicar(c);
			if (observador) observador.observe(s);
			else c.visivel = true;
			if (!c.timer && !c.ocupado) pedir(c);
		});
		Object.keys(cams).forEach(function (d) { if (!vistos[d]) soltar(cams[d]); });
	}
	function soltar(c) {
		clearTimeout(c.timer);
		if (c.abortar) c.abortar();
		delete cams[c.dev];
	}
	// aba escondida: nada de quadro novo; o que estava sendo pego é cancelado
	function pausar() {
		fila = [];
		Object.keys(cams).forEach(function (d) {
			var c = cams[d];
			clearTimeout(c.timer);
			c.timer = 0;
			c.na_fila = false;
			if (c.abortar) c.abortar();
		});
	}
	// saiu da lista (unidade aberta, outra categoria): também para de observar os cartões
	function parar() {
		if (observador) observador.disconnect();
		pausar();
		Object.keys(cams).forEach(function (d) { cams[d].visivel = false; });
	}
	function retomar() {
		Object.keys(cams).forEach(function (d) { var c = cams[d]; if (!c.timer && !c.ocupado) pedir(c); });
	}

	document.addEventListener("visibilitychange", function () { if (document.hidden) pausar(); else retomar(); });
	if (window.Filuna && Filuna.ouvir) {
		// a aba Dispositivo saiu da frente (o programa avisa) ou voltou
		Filuna.ouvir("esconder", function () { escondida = true; pausar(); });
		Filuna.ouvir("mostrar", function () { escondida = false; retomar(); });
		// Reserva para versões do programa que não avisam "esconder": com a aba aberta o estado ao vivo chega a cada
		// 3 s; parado há mais de 10 s (depois de ter chegado alguma vez), a aba saiu da frente.
		var ultimoEstado = 0;
		Filuna.ouvir("dispositivos.estado", function () {
			ultimoEstado = Date.now();
			if (!mostrando) { mostrando = true; retomar(); }
		});
		setInterval(function () { if (mostrando && ultimoEstado && Date.now() - ultimoEstado > 10000) { mostrando = false; pausar(); } }, 5000);
	}

	window.CamCartoes = { fonte: fonte, usaCamera: usaCamera, montar: montar, parar: parar };
})();
