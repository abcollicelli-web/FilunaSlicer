/* Filuna Farm no Slicer — etapa 2 em 3D: a mesa de cada impressora com as peças arrumadas como a Farm vai imprimir
 * (mesma conta de encaixe do farm-plano.js: folga e borda do PADRAO, peça girada quando cabem mais). Abas por
 * impressora e por mesa; arrastar gira, rolar aproxima. Usa o three.js da tela de Modelos 3D (m3d/three.js). */
(function (raiz) {
"use strict";
var FP = raiz.FarmPlano;

function corDa(p) { var c = (p.cores && p.cores.length ? p.cores[0] : p.cor) || ""; return /^#[0-9a-fA-F]{6}$/.test(c) ? c : "#8E8E8E"; }
function esc(s) { return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); }

// Mesas de uma impressora: peças iguais juntas (ou uma por mesa), em grade, na orientação em que cabem mais.
function mesasDa(imp, encaixe, umaPorMesa) {
	var P = FP.PADRAO, g = P.folga_mm, b = P.borda_mm, W = imp.mesa_w, D = imp.mesa_d, mesas = [];
	encaixe.forEach(function (e) {
		var p = e.peca, total = e.total;
		if (!total || p.z > imp.mesa_h) return;
		var w = W - 2 * b, d = D - 2 * b;
		function grade(a, c) { return a > w || c > d ? [0, 0] : [Math.floor((w + g) / (a + g)), Math.floor((d + g) / (c + g))]; }
		var g1 = grade(p.x, p.y), g2 = grade(p.y, p.x), girada = g2[0] * g2[1] > g1[0] * g1[1];
		var gr = girada ? g2 : g1, porMesa = umaPorMesa ? 1 : gr[0] * gr[1];
		if (!porMesa) return;
		var lx = girada ? p.y : p.x, ly = girada ? p.x : p.y;
		for (var feitas = 0; feitas < total; feitas += porMesa) {
			var n = Math.min(porMesa, total - feitas), col = Math.min(gr[0], n), lin = Math.ceil(n / col), pecas = [];
			var x0 = (W - (col * lx + (col - 1) * g)) / 2, y0 = (D - (lin * ly + (lin - 1) * g)) / 2;
			for (var k = 0; k < n; k++) pecas.push({ x: x0 + (k % col) * (lx + g), y: y0 + Math.floor(k / col) * (ly + g), w: lx, d: ly, h: p.z, cor: corDa(p) });
			mesas.push({ nome: p.nome, n: n, pecas: pecas });
		}
	});
	return mesas;
}

var cena = null;
function desenhar(caixa, imp, mesa) {
	var THREE = raiz.THREE;
	if (!THREE || !raiz.OrbitControls) { caixa.innerHTML = '<p class="fm-nota">A vista 3D não está disponível neste computador.</p>'; return; }
	if (cena) { cena.parar(); cena = null; }
	caixa.innerHTML = "";
	var larg = caixa.clientWidth || 640, alt = caixa.clientHeight || 380;
	var r = new THREE.WebGLRenderer({ antialias: true, alpha: true });
	r.setPixelRatio(Math.min(2, raiz.devicePixelRatio || 1)); r.setSize(larg, alt); caixa.appendChild(r.domElement);
	var s = new THREE.Scene(), W = imp.mesa_w, D = imp.mesa_d, maior = Math.max(W, D);
	var cam = new THREE.PerspectiveCamera(35, larg / alt, 1, maior * 20);
	cam.position.set(W * 0.15, maior * 1.05, maior * 1.25);
	s.add(new THREE.HemisphereLight(0xffffff, 0xd9d1c5, 1.15));
	var sol = new THREE.DirectionalLight(0xffffff, 1.4); sol.position.set(-maior, maior * 2, maior); s.add(sol);
	// mesa: placa escura com a grade de 10 mm e a borda útil marcada
	var mesaG = new THREE.Group(); s.add(mesaG);
	var placa = new THREE.Mesh(new THREE.BoxGeometry(W, 3, D), new THREE.MeshStandardMaterial({ color: 0x3a3634, roughness: .85 }));
	placa.position.set(0, -1.5, 0); mesaG.add(placa);
	var grade = new THREE.GridHelper(maior, Math.round(maior / 10), 0x6b6560, 0x55504c); grade.scale.set(W / maior, 1, D / maior); grade.position.y = 0.05; mesaG.add(grade);
	var P = FP.PADRAO, borda = new THREE.LineSegments(new THREE.EdgesGeometry(new THREE.PlaneGeometry(W - 2 * P.borda_mm, D - 2 * P.borda_mm)), new THREE.LineBasicMaterial({ color: 0xE0186B }));
	borda.rotation.x = -Math.PI / 2; borda.position.y = 0.1; mesaG.add(borda);
	(mesa ? mesa.pecas : []).forEach(function (p, i) {
		var cor = new THREE.Color(p.cor), m = new THREE.Mesh(new THREE.BoxGeometry(p.w, p.h, p.d), new THREE.MeshStandardMaterial({ color: cor, roughness: .55, metalness: .02 }));
		m.position.set(p.x + p.w / 2 - W / 2, p.h / 2, p.y + p.d / 2 - D / 2); m.scale.y = 0.001; m.userData.alvo = 1; m.userData.atraso = i * 60;
		var aresta = new THREE.LineSegments(new THREE.EdgesGeometry(m.geometry), new THREE.LineBasicMaterial({ color: cor.clone().multiplyScalar(.55) }));
		m.add(aresta); s.add(m);
	});
	var ctl = new raiz.OrbitControls(cam, r.domElement); ctl.target.set(0, 0, 0); ctl.enableDamping = true; ctl.maxPolarAngle = Math.PI * .48; ctl.update();
	var t0 = performance.now(), vivo = true, calmo = raiz.matchMedia && raiz.matchMedia("(prefers-reduced-motion: reduce)").matches;
	function quadro(t) {
		if (!vivo) return;
		s.children.forEach(function (o) { if (o.userData.alvo) { var k = calmo ? 1 : Math.max(0, Math.min(1, (t - t0 - o.userData.atraso) / 450)); o.scale.y = Math.max(.001, 1 - Math.pow(1 - k, 3)); } });
		ctl.update(); r.render(s, cam); requestAnimationFrame(quadro);
	}
	requestAnimationFrame(quadro);
	function redimensionar() { var l = caixa.clientWidth, a = caixa.clientHeight; if (!l || !a) return; r.setSize(l, a); cam.aspect = l / a; cam.updateProjectionMatrix(); }
	raiz.addEventListener("resize", redimensionar);
	cena = { parar: function () { vivo = false; raiz.removeEventListener("resize", redimensionar); ctl.dispose(); r.dispose(); } };
}

// el: elemento vazio onde a vista entra; encaixe: FarmWizard.encaixe(); imps: impressoras do plano; umaPorMesa: r.iguais
function montar(el, encaixe, imps, umaPorMesa) {
	if (!el) return;
	imps = (imps || []).filter(function (i) { return i.mesa_w && i.mesa_d; });
	if (!imps.length) { el.innerHTML = '<p class="fm-nota">Cadastre uma impressora para ver a montagem da mesa em 3D.</p>'; return; }
	var iImp = 0, iMesa = 0, mesas = [];
	function abas() {
		mesas = mesasDa(imps[iImp], encaixe, umaPorMesa);
		if (iMesa >= mesas.length) iMesa = 0;
		var h = '<div class="fw3d-abas" role="tablist" aria-label="Impressora">' + imps.map(function (i, k) { return '<button type="button" role="tab" class="fw-chip' + (k == iImp ? " on" : "") + '" data-3d-imp="' + k + '">' + esc(i.nome) + "</button>"; }).join("") + "</div>";
		h += '<div class="fw3d-mesas" role="tablist" aria-label="Mesa">' + (mesas.length ? mesas.slice(0, 40).map(function (m, k) { return '<button type="button" role="tab" class="fw3d-mesa' + (k == iMesa ? " on" : "") + '" data-3d-mesa="' + k + '"><b>Mesa ' + (k + 1) + "</b><small>" + esc(m.nome) + " × " + m.n + "</small></button>"; }).join("") + (mesas.length > 40 ? '<span class="fw3d-mais">+' + (mesas.length - 40) + " mesas</span>" : "")
			: '<span class="fw3d-vazio">Nenhuma peça cabe na mesa desta impressora.</span>') + "</div>";
		h += '<div class="fw3d-palco"><div class="fw3d-vista" id="fw3d-vista"></div><p class="fw3d-dica">' + esc(imps[iImp].nome) + " · mesa " + imps[iImp].mesa_w + " × " + imps[iImp].mesa_d + " mm · arraste para girar, role para aproximar</p></div>";
		el.innerHTML = h;
		el.querySelectorAll("[data-3d-imp]").forEach(function (b) { b.onclick = function () { iImp = +b.getAttribute("data-3d-imp"); iMesa = 0; abas(); }; });
		el.querySelectorAll("[data-3d-mesa]").forEach(function (b) { b.onclick = function () { iMesa = +b.getAttribute("data-3d-mesa"); el.querySelectorAll("[data-3d-mesa]").forEach(function (x) { x.classList.toggle("on", x === b); }); desenhar(document.getElementById("fw3d-vista"), imps[iImp], mesas[iMesa]); }; });
		desenhar(document.getElementById("fw3d-vista"), imps[iImp], mesas[iMesa]);
	}
	abas();
}

raiz.FarmMesa3D = { montar: montar, mesasDa: mesasDa };
})(window);
