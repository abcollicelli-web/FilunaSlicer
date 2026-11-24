// Meu espaço › Minha marca e Posts (Post nas redes sociais, telas aprovadas pelo dono em 22/11/2026).
// As seções só entram no menu quando o servidor libera a ferramenta para a conta (/midia/estado). Usa o que a página
// meu-espaco.html já tem: st, el(), titulo(), falha(), E, SECOES, menu().
(function () {
	var ESTILOS = [["estudio", "Estúdio moderno"], ["ambiente", "Ambiente real"], ["minimalista", "Minimalista"]];
	var TONS = [["proximo", "Próximo"], ["profissional", "Profissional"], ["divertido", "Divertido"]];
	var CSS = '<style>.mm-op{display:flex;gap:8px;flex-wrap:wrap}.mm-op button{border:1px solid var(--line);background:#fff;border-radius:12px;padding:9px 14px;font-weight:700;cursor:pointer}' +
		'.mm-op button.on{border:2px solid var(--magenta);background:#FFF5F9;color:var(--magenta-deep)}.mm-cores{display:flex;gap:10px;align-items:center}' +
		'.mm-cores input{width:46px;height:46px;border:0;padding:0;background:none;cursor:pointer}.mm-logo{width:60px;height:60px;border-radius:12px;object-fit:contain;background:var(--pill)}' +
		'.mm-grade{display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:14px}.mm-card{border:1px solid var(--line);border-radius:14px;overflow:hidden;background:#fff}' +
		'.mm-card img{width:100%;aspect-ratio:1/1;object-fit:cover;display:block}.mm-card div{padding:9px 11px;font-size:12.5px}.mm-card b{display:block;font-size:13.5px}' +
		'.mm-rede{display:flex;align-items:center;gap:10px;border:1px solid var(--line);border-radius:12px;padding:9px 12px;margin-top:8px}</style>';
	var SITE = "https://filuna3d.com";

	function ligada() { return SECOES.some(function (s) { return s[0] == "marca"; }); }

	// Precisa das duas pontas: o servidor liberou para a conta e este programa já tem a ferramenta (midia.*).
	function conferir() {
		if (!Filuna.temToken()) return;
		Promise.all([Filuna.api("GET", "/midia/estado"), Filuna.nativo("midia.preferencia")]).then(function (r) {
			Filuna.nativo("midia.disponivel", { disponivel: !!r[0].disponivel });
			if (r[0].disponivel && !ligada()) { SECOES.push(["marca", "✦ Minha marca"], ["posts", "Posts"]); if (grupoDe(st.secao).id == "negocio") desenhar(); else menu(); }
		}).catch(function () {});
	}

	function opcoes(id, lista, atual) {
		return '<div class="mm-op" id="' + id + '">' + lista.map(function (o) { return '<button data-v="' + o[0] + '" class="' + (o[0] == atual ? "on" : "") + '">' + E(o[1]) + "</button>"; }).join("") + "</div>";
	}

	window.midiaMarca = function () {
		el().innerHTML = CSS + titulo("Post nas redes sociais", "Minha marca", "A sua IA usa isto em todas as fotos e legendas.") + '<div id="mm" class="carregando">Carregando…</div>';
		Promise.all([Filuna.api("GET", "/midia/marca"), Filuna.nativo("midia.preferencia")]).then(function (r) {
			var m = r[0].marca, redes = r[0].redes || {}, logo = r[0].logo || "", cores = (m.cores.length ? m.cores : ["#FF6B1A"]).slice(0, 4);
			var rede = function (k, nome) {
				var x = redes[k] || {};
				return '<div class="mm-rede"><b>' + nome + "</b>" + (x.conta ? " · " + E(x.conta) : "") + '<span style="margin-left:auto" class="' + (x.conectado ? "ok-verde" : "s") + '">' +
					(x.conectado ? "conectado" : '<a href="#" data-conectar="1">Conectar</a>') + "</span></div>";
			};
			document.getElementById("mm").outerHTML = '<div class="card" style="display:grid;grid-template-columns:1fr 1fr;gap:26px">' +
				'<div><div class="campo"><label>Logo</label><div style="display:flex;gap:12px;align-items:center">' + (logo ? '<img class="mm-logo" id="mmLogoImg" src="' + logo + '">' : '<span class="mm-logo" id="mmLogoImg"></span>') +
				'<button class="btn" id="mmLogo">' + (logo ? "Trocar" : "Escolher logo") + "</button></div></div>" +
				'<div class="campo"><label>Nome da empresa</label><input type="text" id="mmNome" value="' + E(m.nome) + '"></div>' +
				'<div class="campo"><label>Cores da marca</label><div class="mm-cores" id="mmCores">' + cores.map(function (c) { return '<input type="color" value="' + E(c) + '">'; }).join("") +
				'<button class="btn" id="mmMaisCor">+</button></div></div>' +
				'<div class="campo"><label>Estilo das fotos</label>' + opcoes("mmEstilo", ESTILOS, m.estilo) + "</div>" +
				'<div class="campo"><label>Tom da legenda</label>' + opcoes("mmTom", TONS, m.tom) + "</div></div>" +
				'<div><div class="campo"><label>Redes conectadas</label>' + rede("instagram", "Instagram") + rede("facebook", "Facebook") + "</div>" +
				'<div class="campo"><label>IA que cria as fotos</label><div class="mm-rede"><b>ChatGPT</b> · sua assinatura<span style="margin-left:auto" class="s">o Claude escreve legendas, mas não cria fotos</span></div></div>' +
				'<div class="campo"><label>Texto fixo no fim da legenda</label><input type="text" id="mmFixo" value="' + E(m.texto_fixo) + '" placeholder="Encomende pelo link da bio"></div>' +
				'<div class="campo"><label>Hashtags fixas</label><input type="text" id="mmHash" value="' + E(m.hashtags) + '" placeholder="#suaempresa #impressao3d"></div>' +
				'<div class="campo"><label><input type="checkbox" id="mmAuto"' + (r[1].post ? " checked" : "") + "> Post nas redes a cada impressão (a caixinha já vem ligada na tela de envio)</label></div>" +
				'<button class="btn primario" id="mmSalvar">Salvar</button> <span id="mmOk" class="ok-verde"></span></div></div>';
			var novoLogo = null;
			var sel = function (id) { var b = document.querySelector("#" + id + " button.on"); return b ? b.dataset.v : ""; };
			["mmEstilo", "mmTom"].forEach(function (id) {
				document.querySelectorAll("#" + id + " button").forEach(function (b) {
					b.onclick = function () { document.querySelectorAll("#" + id + " button").forEach(function (x) { x.classList.remove("on"); }); b.classList.add("on"); };
				});
			});
			document.getElementById("mmMaisCor").onclick = function () {
				if (document.querySelectorAll("#mmCores input").length < 4) this.insertAdjacentHTML("beforebegin", '<input type="color" value="#2C2A28">');
			};
			document.getElementById("mmLogo").onclick = function () {
				Filuna.nativo("imagens.escolher").then(function (x) {
					var f = (x.imagens || [])[0]; if (!f) return;
					novoLogo = f.imagem;
					document.getElementById("mmLogoImg").outerHTML = '<img class="mm-logo" id="mmLogoImg" src="' + novoLogo + '">';
				}).catch(falha);
			};
			document.querySelectorAll("[data-conectar]").forEach(function (a) {
				a.onclick = function (e) { e.preventDefault(); Filuna.nativo("url.abrir", { url: SITE + "/plugins/instagram/conectar" }); };
			});
			document.getElementById("mmSalvar").onclick = function () {
				var corpo = { nome: document.getElementById("mmNome").value, estilo: sel("mmEstilo"), tom: sel("mmTom"),
					cores: [].map.call(document.querySelectorAll("#mmCores input"), function (i) { return i.value.toUpperCase(); }),
					texto_fixo: document.getElementById("mmFixo").value, hashtags: document.getElementById("mmHash").value };
				if (novoLogo) corpo.logo = novoLogo;
				Promise.all([Filuna.api("POST", "/midia/marca", corpo), Filuna.nativo("midia.preferencia", { post: document.getElementById("mmAuto").checked })])
					.then(function () { document.getElementById("mmOk").textContent = "Salvo ✓"; }).catch(falha);
			};
		}).catch(function (e) { document.getElementById("mm").outerHTML = ""; falha(e); });
	};

	window.midiaPosts = function () {
		el().innerHTML = CSS + titulo("Post nas redes sociais", "Posts", "As fotos que a sua IA criou e onde foram publicadas.") +
			'<div style="margin:12px 0"><button class="btn primario" id="mmNovo">Criar post da peça na mesa</button></div><div id="mp" class="carregando">Carregando…</div>';
		document.getElementById("mmNovo").onclick = function () { Filuna.nativo("midia.abrir").catch(falha); };
		Filuna.api("GET", "/midia/posts").then(function (r) {
			var estado = { publicado: "Publicado", pronto: "Esperando você", erro: "Não publicou" };
			document.getElementById("mp").outerHTML = '<div class="sub" style="margin-bottom:10px">' + r.cenarios_usados + " cenário(s) usado(s) · nenhum repetido</div>" +
				(r.posts.length ? '<div class="mm-grade">' + r.posts.map(function (p) {
					var redes = Object.keys(p.resultado || {}).filter(function (k) { return p.resultado[k].ok; }).join(" + ");
					return '<div class="mm-card">' + (p.fotos[0] ? '<img src="' + SITE + E(p.fotos[0]) + '">' : "") + "<div><b>" + E(p.produto || "Peça") + "</b>" +
						E(estado[p.estado] || p.estado) + (redes ? " · " + E(redes) : "") + '<div class="s">Cenário: ' + E(p.cenario) + "</div></div></div>";
				}).join("") + "</div>" : '<div class="vazio">Nenhum post ainda. Ligue “Post nas redes sociais” ao enviar uma impressão.</div>');
		}).catch(function (e) { document.getElementById("mp").outerHTML = ""; falha(e); });
	};

	Filuna.iniciar().then(conferir);
	Filuna.ouvir("mostrar", conferir);
})();
