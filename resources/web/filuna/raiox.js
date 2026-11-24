// Raio X (tela Dispositivo): a central da impressora, a mesma do site (/conta/raiox). Foto real do modelo com o número
// de cada região, o estado de agora, e as abas Erros (com a ficha oficial do erro), Reparo (passos da marca, um por um),
// Impressões (alertas e relatórios da análise) e Manutenção (por horas). Um sistema só: resolver aqui resolve no site.
var RaioX = (function () {
	var E = Filuna.esc, SITE = "https://filuna3d.com";
	var CSS = '<style id="rxCss">' +
		'.rx{padding:18px 32px 40px}.rx-volta{display:flex;justify-content:space-between;align-items:center}.rx h1{font-size:26px;font-weight:800;margin:4px 0 2px}' +
		'.rx-sub{color:var(--ink2);font-size:14px}.rx-grade{display:grid;grid-template-columns:minmax(300px,1.05fr) 1fr;gap:20px;align-items:start;margin-top:16px}' +
		'@media (max-width:980px){.rx-grade{grid-template-columns:1fr}}.rx-card{background:#fff;border:1px solid var(--line);border-radius:20px;padding:14px}' +
		'.rx-foto{position:relative;aspect-ratio:1/1;border-radius:16px;background:radial-gradient(120% 90% at 50% 35%,#fff,#efeae3);overflow:hidden}' +
		'.rx-foto img{position:absolute;inset:6%;width:88%;height:88%;object-fit:contain;filter:drop-shadow(0 18px 30px rgba(40,30,20,.18))}' +
		'.rx-camada{position:absolute;inset:6%}.rx-sem{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:var(--muted)}' +
		'.rx-pin{position:absolute;transform:translate(-50%,-50%);width:32px;height:32px;border-radius:50%;background:#fff;border:2px solid #C9C1B6;color:var(--ink2);font-weight:800;display:flex;align-items:center;justify-content:center;font-size:14px;cursor:pointer;box-shadow:0 2px 8px rgba(0,0,0,.12)}' +
		'.rx-pin.fora{border-style:dashed}.rx-pin.manut{background:#FFE6D6;border-color:var(--orange);color:var(--warn)}.rx-pin.sel{outline:3px solid var(--magenta);outline-offset:2px}' +
		'.rx-pin.erro{background:var(--warn);border-color:#fff;color:#fff;width:42px;height:42px;font-size:18px;animation:rxP 1.6s ease-out infinite}' +
		'@keyframes rxP{0%{box-shadow:0 0 0 0 rgba(217,72,15,.5)}100%{box-shadow:0 0 0 24px rgba(217,72,15,0)}}@media (prefers-reduced-motion:reduce){.rx-pin.erro{animation:none}}' +
		'.rx-leg{display:flex;flex-wrap:wrap;gap:6px;margin-top:12px}.rx-leg span{font-size:12px;background:var(--side);border:1px solid var(--line);border-radius:999px;padding:3px 10px;color:var(--ink2)}.rx-leg b{color:var(--ink);margin-right:4px}' +
		'.rx-estado{display:flex;align-items:center;gap:10px;margin:12px 0;padding:10px 12px;border-radius:12px;background:var(--side);font-size:14px;flex-wrap:wrap}' +
		'.rx-estado i{width:10px;height:10px;border-radius:50%;background:#CFC8BE}.rx-estado.imprimindo i{background:var(--magenta-deep)}.rx-estado.livre i{background:var(--good)}' +
		'.rx-estado.erro{background:#FDE8DF}.rx-estado.erro i{background:var(--warn)}.rx-estado small{margin-left:auto;color:var(--muted)}' +
		'.rx-agora-erro{border:1px solid #F3C3A5;background:#FFF6EF;border-radius:14px;padding:12px 14px;margin-bottom:12px}.rx-agora-erro p{color:var(--ink2);font-size:13.5px;margin:4px 0 10px}' +
		'.rx-num{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}.rx-num div{background:var(--side);border:1px solid var(--line);border-radius:14px;padding:10px 12px}' +
		'.rx-num b{display:block;font-size:21px}.rx-num span{font-size:12px;color:var(--muted);line-height:1.3;display:block}.rx-num .at{background:#FFF6EF;border-color:#F3C3A5}.rx-num .at b{color:var(--warn)}' +
		'.rx-pad{margin-top:12px;background:#FFF6EF;border:1px solid #F7D5BD;border-radius:14px;padding:10px 14px;font-size:13.5px}.rx-pad li{margin:3px 0 3px 18px}' +
		'.rx-abas{display:flex;gap:4px;margin-top:22px;border-bottom:1px solid var(--line)}.rx-abas button{border:0;background:none;padding:10px 16px;font-weight:700;color:var(--ink2);border-bottom:3px solid transparent;cursor:pointer}' +
		'.rx-abas button.on{color:var(--ink);border-color:var(--magenta)}.rx-abas em{font-style:normal;margin-left:6px;background:var(--rose);color:var(--magenta-deep);border-radius:999px;padding:1px 8px;font-size:12px}' +
		'.rx-sec{padding-top:16px}.rx-txt{color:var(--ink2);font-size:14px}.rx-ev{display:flex;gap:12px;background:#fff;border:1px solid var(--line);border-radius:16px;padding:12px;margin-bottom:10px}' +
		'.rx-ev.aberto{border-color:#F6B08A}.rx-ev img{width:124px;height:92px;object-fit:cover;border-radius:10px;flex:none;cursor:zoom-in}.rx-ev .t{font-weight:700}' +
		'.rx-m{color:var(--ink2);font-size:12.5px;margin-top:3px;line-height:1.5}.rx-n{flex:none;width:30px;height:30px;border-radius:50%;background:#FFE3D3;color:var(--warn);font-weight:800;display:flex;align-items:center;justify-content:center}' +
		'.rx-ev.ok .rx-n{background:var(--good-bg);color:var(--good)}.rx-dot{display:inline-block;width:10px;height:10px;border-radius:50%;vertical-align:-1px;margin-right:4px;border:1px solid rgba(0,0,0,.15)}' +
		'.rx-ficha{margin-top:8px;background:var(--side);border-radius:12px;padding:8px 12px;font-size:13.5px}.rx-ficha summary{cursor:pointer;font-weight:700;color:var(--magenta-deep)}.rx-ficha ol{padding-left:20px;margin-top:6px}' +
		'.rx-acoes{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px;align-items:center}.rx-acoes input{flex:1;min-width:160px;padding:7px 10px;border:1px solid var(--line);border-radius:10px}' +
		'.rx-passos{list-style:none;margin:12px 0}.rx-passos li{display:flex;gap:12px;padding:10px 0;border-top:1px solid var(--line)}' +
		'.rx-passos button{width:32px;height:32px;border-radius:50%;border:2px solid #C9C1B6;background:#fff;font-weight:800;cursor:pointer;flex:none}' +
		'.rx-passos li.feito button{background:var(--good);border-color:var(--good);color:#fff}.rx-passos li.feito b{color:var(--muted);text-decoration:line-through}.rx-passos p{color:var(--ink2);font-size:13.5px}' +
		'.rx-duas{display:grid;grid-template-columns:1fr 1fr;gap:18px}.rx-linha{display:flex;gap:10px;align-items:center;background:#fff;border:1px solid var(--line);border-radius:14px;padding:10px;margin-bottom:8px;cursor:pointer}' +
		'.rx-linha img{width:72px;height:54px;object-fit:cover;border-radius:8px}.rx-nota{flex:none;width:40px;height:40px;border-radius:12px;display:flex;align-items:center;justify-content:center;font-weight:800;background:var(--good-bg);color:var(--good)}' +
		'.rx-nota.meio{background:#FFE6D6;color:var(--warn)}.rx-nota.ruim{background:#FDE8DF;color:var(--warn)}' +
		'.rx-manut{display:flex;gap:12px;align-items:center;background:#fff;border:1px solid var(--line);border-radius:14px;padding:12px;margin-bottom:8px}.rx-manut>div{flex:1}' +
		'.rx-manut i{display:block;height:6px;background:var(--side);border-radius:99px;margin-top:6px;overflow:hidden}.rx-manut s{display:block;height:100%;background:var(--good);border-radius:99px}' +
		'.rx-manut.perto s{background:var(--orange)}.rx-manut.vencida s{background:var(--warn)}.rx-manut.vencida{background:#FFF6EF;border-color:#F3C3A5}' +
		'.rx-zoom{position:fixed;inset:0;background:rgba(20,18,16,.82);display:flex;align-items:center;justify-content:center;z-index:50;cursor:zoom-out}.rx-zoom img{max-width:92vw;max-height:88vh;border-radius:12px}</style>';
	var st = { c: null, chave: "", carregando: false, erro: "", aba: "", regiao: null };

	function quando(iso) { try { return new Date(iso).toLocaleString("pt-BR", { day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit" }); } catch (e) { return iso || ""; } }
	function cor(c) { c = String(c || ""); return /^#?[0-9a-f]{6}/i.test(c) ? "#" + c.replace("#", "").slice(0, 6) : "#ccc"; }
	function num(v) { return String(v == null ? 0 : v).replace(".", ","); }
	function capa(c, fotoLocal) { return c && c.foto ? "../../profiles/" + c.foto.replace(/^\/static\/raiox\/capas\//, "") : fotoLocal || ""; }
	function chaveDe(u) { var v = u.viva || {}; return v.cadastro_id || (u.farm && u.farm.ref_slicer) || v.dev_id || ""; }

	function carregar(u, redesenhar) {
		if (st.carregando) return;
		if (!Filuna.temToken()) { st.erro = "Entre na sua conta Filuna para ter o Raio X da impressora."; redesenhar(); return; }
		st.chave = chaveDe(u);
		if (!st.chave) { st.erro = "Esta impressora ainda não tem cadastro na conta. Conecte-a uma vez pelo cartão."; redesenhar(); return; }
		st.carregando = true;
		Filuna.api("GET", "/raiox/central?chave=" + encodeURIComponent(st.chave)).then(function (r) { st.c = r; st.erro = ""; if (!st.aba) st.aba = r.caso ? "reparo" : "erros"; },
			function (e) { st.c = null; st.erro = e && e.status == 404 ? "Ainda não há dados desta impressora. Eles aparecem com a impressora ligada e o Filuna Slicer aberto." : (e && e.erro) || "Não deu para ler o Raio X agora."; })
			.then(function () { st.carregando = false; redesenhar(); });
	}
	function acao(rota, corpo, redesenhar, u) {
		return Filuna.api("POST", rota, corpo).then(function () { st.c = null; carregar(u, redesenhar); }, function (e) { st.erro = (e && e.erro) || "Não deu para fazer isso agora."; redesenhar(); });
	}

	function fichaHtml(f) {
		if (!f) return "";
		if (f.bloqueado) return '<div class="rx-m">' + E(f.titulo) + " · a solução passo a passo é dos planos pagos</div>";
		return '<details class="rx-ficha"><summary>' + E(f.titulo) + ": o que é e como resolver</summary>" + (f.oque ? "<p>" + E(f.oque) + "</p>" : "") +
			(f.causas && f.causas.length ? "<p><b>Causas comuns:</b> " + f.causas.slice(0, 4).map(E).join(" · ") + "</p>" : "") +
			(f.passos && f.passos.length ? "<ol>" + f.passos.slice(0, 6).map(function (p) { return "<li>" + E(p.t) + "</li>"; }).join("") + "</ol>" : "") + "</details>";
	}

	function abaErros(c) {
		var l = c.eventos.filter(function (e) { return !st.regiao || e.regiao == st.regiao; });
		if (!l.length) return '<p class="rx-txt">' + (st.regiao ? "Nenhum erro nesta região." : "Nenhum erro registrado. Quando a impressora informar um erro, ele aparece aqui com a foto da câmera, a camada, o filamento e a solução.") + "</p>";
		return l.map(function (e) {
			var f = e.filamento || {}, m = [quando(e.criado_em), e.regiao_nome];
			if (e.codigo) m.push("código " + e.codigo);
			if (e.camada) m.push("camada " + e.camada + (e.camadas ? " de " + e.camadas : ""));
			if (e.z != null) m.push(num(Math.round(e.z * 100) / 100) + " mm");
			if (e.progresso) m.push(e.progresso + "%");
			m.push(e.impressoes + " impressões e " + num(e.horas) + " h até aqui");
			var fil = f.material || f.marca ? '<div class="rx-m"><span class="rx-dot" style="background:' + cor(f.cor) + '"></span>' + E([f.marca, f.material, f.cor_nome].filter(Boolean).join(" ")) + (f.bobina_numero ? " · bobina " + E(f.bobina_numero) : "") + (f.posicao ? " · entrada " + f.posicao : "") + "</div>" : "";
			return '<div class="rx-ev ' + (e.resolvido_em ? "ok" : "aberto") + '"><div class="rx-n">' + e.regiao + "</div>" +
				(e.foto_url ? '<img alt="Foto da câmera na hora do erro" data-zoom="' + E(SITE + e.foto_url) + '" src="' + E(SITE + e.foto_url) + '">' : "") +
				'<div style="min-width:0;flex:1"><div class="t">' + E(e.texto || "Erro da impressora") + '</div><div class="rx-m">' + m.map(E).join(" · ") + "</div>" + fil +
				(e.resolvido_em ? '<div class="rx-m" style="color:var(--good)">Resolvido · ' + E(e.solucao || "") + "</div>"
					: (e.ficha ? fichaHtml(e.ficha) : '<div class="rx-m"><b>O que conferir:</b> ' + E(e.dica) + "</div>") +
					'<div class="rx-acoes">' + (e.ficha && !e.ficha.bloqueado ? '<button class="btn primario peq" data-reparo="' + E(e.ficha.id) + '">Resolver com a Filuna</button>' : "") +
					'<input type="text" data-sol="' + e.id + '" placeholder="O que resolveu (opcional)"><button class="btn peq" data-res="' + e.id + '">Resolvido</button></div>') +
				"</div></div>";
		}).join("");
	}
	function abaReparo(c) {
		var cs = c.caso;
		if (!cs) return '<p class="rx-txt">Nenhum reparo em andamento. Quando a impressora mostrar um erro, abra o reparo por ele na aba Erros.</p>';
		var fi = null;
		c.eventos.forEach(function (e) { if (!fi && e.ficha && e.ficha.id == cs.erro_id) fi = e.ficha; });
		if (!fi && c.agora && c.agora.ficha && c.agora.ficha.id == cs.erro_id) fi = c.agora.ficha;
		var passos = fi && fi.passos ? fi.passos : [];
		return '<div class="rx-card"><b style="font-size:16px">' + E(cs.codigo + " · " + cs.titulo) + '</b><div class="rx-m">aberto ' + quando(cs.aberto_em) + " · os passos são os oficiais da marca</div>" +
			(passos.length ? '<ol class="rx-passos">' + passos.map(function (p, k) {
				var feito = (cs.passos_feitos || []).indexOf(k) >= 0;
				return '<li class="' + (feito ? "feito" : "") + '"><button data-passo="' + k + '" data-feito="' + (feito ? 0 : 1) + '">' + (feito ? "✓" : k + 1) + "</button><div><b>" + E(p.t) + "</b>" + (p.d ? "<p>" + E(p.d) + "</p>" : "") + "</div></li>";
			}).join("") + "</ol>" : "") +
			'<div class="rx-acoes"><button class="btn peq" id="rxConversa">Conversar com a Filuna sobre este erro</button><button class="btn primario peq" id="rxResolvido">Resolvido</button></div></div>';
	}
	function abaImpressoes(c) {
		return '<div class="rx-duas"><div><b>Alertas durante a impressão</b><div style="margin-top:8px">' + (c.alertas.length ? c.alertas.map(function (a) {
			return '<div class="rx-linha" data-url="' + E(SITE + "/conta/alertas/" + a.id) + '">' + (a.foto_url ? '<img alt="" src="' + E(SITE + a.foto_url) + '">' : "") + "<div><b>" + E(String(a.mensagem || "").slice(0, 110)) + '</b><div class="rx-m">' + quando(a.criado_em) + (a.camada ? " · camada " + a.camada : "") + "</div></div></div>";
		}).join("") : '<p class="rx-txt">Nenhum alerta. A análise avisa aqui quando a impressão sai do previsto.</p>') + "</div></div>" +
			'<div><b>Relatórios das impressões</b><div style="margin-top:8px">' + (c.relatorios.length ? c.relatorios.map(function (r) {
				return '<div class="rx-linha" data-url="' + E(SITE + "/conta/relatorios/" + r.id) + '"><span class="rx-nota ' + (r.problemas ? "ruim" : r.atencoes ? "meio" : "") + '">' + E(r.nota) + "</span><div><b>" + E(r.nome) + '</b><div class="rx-m">' + quando(r.criado_em) + " · " + E(String(r.resumo || "").slice(0, 90)) + "</div></div></div>";
			}).join("") : '<p class="rx-txt">Nenhum relatório ainda. Cada impressão acompanhada gera um.</p>') + "</div></div></div>";
	}
	function abaManutencao(c) {
		return '<p class="rx-txt" style="margin-bottom:10px">Pelas horas que a impressora já imprimiu (' + num(c.horas) + " h). Marque quando fizer: a contagem recomeça.</p>" + c.manutencao.map(function (m) {
			return '<div class="rx-manut ' + m.situacao + '"><span class="rx-n">' + m.regiao + "</span><div><b>" + E(m.texto) + '</b><div class="rx-m">a cada ' + m.cada_h + " h · " + num(m.desde_h) + " h desde " + (m.feito_em ? "a última vez" : "o começo") +
				'</div><i><s style="width:' + m.pct + '%"></s></i></div><button class="btn peq" data-manut="' + E(m.item) + '">Feito</button></div>';
		}).join("");
	}

	function desenhar(app, u, voltar, redesenhar) {
		if (!st.c && !st.erro) { app.innerHTML = CSS + '<div class="rx">Carregando o Raio X…</div>'; carregar(u, redesenhar); return; }
		var c = st.c;
		var topo = '<div class="rx"><div class="rx-volta"><a class="voltar" id="rxVoltar">‹ ' + E(u.nome) + "</a>" + (c ? '<a class="voltar" id="rxSite">Abrir no site</a>' : "") + "</div>" +
			'<div class="lbl" style="margin-top:8px">Raio X</div><h1>' + E(u.nome) + '</h1><div class="rx-sub">' + E((c && c.modelo) || u.modelo || "") + "</div>";
		if (!c) { app.innerHTML = CSS + topo + '<div class="aviso" style="margin-top:14px"><span>' + E(st.erro) + "</span></div></div>"; ligar(app, u, voltar, redesenhar); return; }
		var ag = c.agora || {}, pontos = (c.capa && c.capa.regioes) || {}, fora = (c.capa && c.capa.fora) || [], regioes = c.regioes || {};
		var abertos = {}, vencida = {};
		c.eventos.forEach(function (e) { if (!e.resolvido_em) abertos[e.regiao] = (abertos[e.regiao] || 0) + 1; });
		c.manutencao.forEach(function (m) { if (m.situacao == "vencida") vencida[m.regiao] = 1; });
		var foto = capa(c.capa, u.foto);
		var pins = Object.keys(pontos).filter(function (k) { return pontos[k]; }).map(function (k) {
			var cls = (abertos[k] ? " erro" : vencida[k] ? " manut" : "") + (fora.indexOf(k) >= 0 ? " fora" : "") + (st.regiao == k ? " sel" : "");
			return '<div class="rx-pin' + cls + '" data-reg="' + k + '" title="' + E((regioes[k] || "") + (abertos[k] ? " · " + abertos[k] + " erro(s)" : "")) + '" style="left:' + (+pontos[k][0]) + "%;top:" + (+pontos[k][1]) + '%">' + k + "</div>";
		}).join("");
		var venc = c.manutencao.filter(function (m) { return m.situacao == "vencida"; }).length;
		var estado = ag.erro ? "erro" : ag.estado || "desligada";
		var html = CSS + topo + (st.erro ? '<div class="aviso" style="margin-top:12px"><span>' + E(st.erro) + "</span></div>" : "") +
			'<div class="rx-grade"><div class="rx-card"><div class="rx-foto">' + (foto ? '<img alt="" src="' + E(foto) + '">' : '<div class="rx-sem">Sem foto deste modelo</div>') + '<div class="rx-camada">' + pins + "</div></div>" +
			'<div class="rx-leg">' + Object.keys(regioes).map(function (k) { return "<span><b>" + k + "</b>" + E(regioes[k]) + "</span>"; }).join("") + "</div></div>" +
			'<div class="rx-card"><div class="rx-estado ' + estado + '"><i></i><b>' + (ag.erro ? "Com erro agora" : ag.estado == "imprimindo" ? "Imprimindo · " + (ag.progresso || 0) + "%" : ag.ligada ? "Ligada e livre" : "Desligada") + "</b>" + (ag.visto_em ? "<small>visto " + quando(ag.visto_em) + "</small>" : "") + "</div>" +
			(ag.erro ? '<div class="rx-agora-erro"><b>' + E(ag.erro) + "</b>" + (ag.ficha ? "<p>" + E(ag.ficha.titulo) + "</p>" + (ag.ficha.bloqueado ? "" : '<button class="btn primario peq" data-reparo="' + E(ag.ficha.id) + '">Resolver passo a passo com a Filuna</button>') : "") + "</div>" : "") +
			'<div class="rx-num"><div><b>' + c.impressoes + "</b><span>impressões</span></div><div><b>" + num(c.horas) + " h</b><span>imprimindo</span></div><div><b>" + c.abertos + "</b><span>erro(s) em aberto</span></div>" +
			'<div class="' + (venc ? "at" : "") + '"><b>' + (venc || "✓") + "</b><span>" + (venc ? "manutenção vencida" : "manutenção em dia") + "</span></div></div>" +
			(c.padroes && c.padroes.length ? '<div class="rx-pad"><b>O que se repete</b><ul>' + c.padroes.map(function (p) { return "<li>" + E(p) + "</li>"; }).join("") + "</ul></div>" : "") + "</div></div>" +
			'<div class="rx-abas">' + [["erros", "Erros", c.abertos], ["reparo", "Reparo", c.caso ? 1 : 0], ["impressoes", "Impressões", c.alertas.length], ["manutencao", "Manutenção", venc]].map(function (a) {
				return '<button data-aba="' + a[0] + '" class="' + (st.aba == a[0] ? "on" : "") + '">' + a[1] + (a[2] ? "<em>" + a[2] + "</em>" : "") + "</button>";
			}).join("") + "</div>" +
			'<div class="rx-sec">' + (st.aba == "reparo" ? abaReparo(c) : st.aba == "impressoes" ? abaImpressoes(c) : st.aba == "manutencao" ? abaManutencao(c) : abaErros(c)) + "</div></div>";
		app.innerHTML = html;
		ligar(app, u, voltar, redesenhar);
	}

	function ligar(app, u, voltar, redesenhar) {
		var c = st.c, q = function (s) { return app.querySelector(s); };
		q("#rxVoltar").onclick = function () { st.regiao = null; voltar(); };
		var site = q("#rxSite"); if (site) site.onclick = function () { Filuna.nativo("url.abrir", { url: SITE + "/conta/raiox?chave=" + encodeURIComponent(st.chave) }); };
		app.querySelectorAll("[data-reg]").forEach(function (p) { p.onclick = function () { st.regiao = st.regiao == p.dataset.reg ? null : p.dataset.reg; st.aba = "erros"; redesenhar(); }; });
		app.querySelectorAll("[data-aba]").forEach(function (b) { b.onclick = function () { st.aba = b.dataset.aba; redesenhar(); }; });
		app.querySelectorAll("[data-zoom]").forEach(function (im) {
			im.onclick = function () { var z = document.createElement("div"); z.className = "rx-zoom"; z.innerHTML = '<img alt="" src="' + E(im.dataset.zoom) + '">'; z.onclick = function () { z.remove(); }; document.body.appendChild(z); };
		});
		app.querySelectorAll("[data-url]").forEach(function (l) { l.onclick = function () { Filuna.nativo("url.abrir", { url: l.dataset.url }); }; });
		app.querySelectorAll("[data-reparo]").forEach(function (b) { b.onclick = function () { b.disabled = true; st.aba = "reparo"; acao("/raiox/reparo", { chave: st.chave, erro_id: b.dataset.reparo }, redesenhar, u); }; });
		app.querySelectorAll("[data-res]").forEach(function (b) {
			b.onclick = function () { var s = q('[data-sol="' + b.dataset.res + '"]'); b.disabled = true; acao("/raiox/" + encodeURIComponent(b.dataset.res) + "/resolver", { solucao: s ? s.value : "" }, redesenhar, u); };
		});
		app.querySelectorAll("[data-manut]").forEach(function (b) { b.onclick = function () { b.disabled = true; acao("/raiox/manutencao", { chave: st.chave, item: b.dataset.manut }, redesenhar, u); }; });
		if (c && c.caso) {
			app.querySelectorAll("[data-passo]").forEach(function (b) { b.onclick = function () { b.disabled = true; acao("/raiox/reparo/" + encodeURIComponent(c.caso.id) + "/passo", { passo: +b.dataset.passo, feito: b.dataset.feito == "1" }, redesenhar, u); }; });
			var r = q("#rxResolvido"); if (r) r.onclick = function () { r.disabled = true; acao("/raiox/reparo/" + encodeURIComponent(c.caso.id) + "/encerrar", { resolvido: true }, redesenhar, u); };
			var cv = q("#rxConversa"); if (cv) cv.onclick = function () { Filuna.nativo("url.abrir", { url: SITE + "/assistencia/" + encodeURIComponent(c.caso.marca) + "/" + encodeURIComponent(String(c.caso.codigo).replace(/ /g, "-")) }); };
		}
	}

	return { desenhar: desenhar, recarregar: function () { st.c = null; st.erro = ""; st.aba = ""; st.regiao = null; } };
})();
