// Raio X da Impressora (tela Dispositivo): a foto real do modelo ampliada, com o número da região de cada erro, o
// histórico com a foto da câmera na hora do erro, a camada, a altura, o filamento e a bobina, e os padrões que se
// repetem. Os dados vêm da conta (/raiox); a foto do modelo é a do próprio perfil (resources/profiles).
var RaioX = (function () {
	var E = Filuna.esc, SITE = "https://filuna3d.com";
	var CSS = '<style id="rxCss">.rx{display:flex;gap:26px;padding:22px 32px;flex-wrap:wrap}.rx-foto{flex:1.2;min-width:340px;position:relative;background:radial-gradient(120% 90% at 50% 35%,#fff,#efeae3);border-radius:18px;aspect-ratio:1/1;max-height:620px;overflow:hidden}' +
		'.rx-foto img.capa{position:absolute;inset:6%;width:88%;height:88%;object-fit:contain;image-rendering:auto;filter:drop-shadow(0 18px 30px rgba(40,30,20,.18))}' +
		'.rx-foto .semfoto{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:var(--muted)}' +
		'.rx-pin{position:absolute;transform:translate(-50%,-50%);width:34px;height:34px;border-radius:50%;background:#fff;border:2px solid #C9C1B6;color:var(--ink2);font-weight:800;display:flex;align-items:center;justify-content:center;font-size:15px;cursor:pointer;box-shadow:0 2px 8px rgba(0,0,0,.12);z-index:2}' +
		'.rx-pin.erro{background:var(--warn);border-color:#fff;color:#fff;width:44px;height:44px;font-size:19px;animation:rxPulso 1.6s ease-out infinite}' +
		'.rx-pin.hist{background:#FFE3D3;border-color:var(--orange);color:var(--warn)}.rx-pin.sel{outline:3px solid var(--magenta);outline-offset:2px}' +
		'@keyframes rxPulso{0%{box-shadow:0 0 0 0 rgba(217,72,15,.55)}100%{box-shadow:0 0 0 26px rgba(217,72,15,0)}}' +
		'@media (prefers-reduced-motion:reduce){.rx-pin.erro{animation:none}}' +
		'.rx-leg{position:absolute;left:14px;bottom:12px;right:14px;display:flex;flex-wrap:wrap;gap:6px;z-index:3}.rx-leg span{background:rgba(255,255,255,.92);border-radius:999px;padding:4px 10px;font-size:12px;color:var(--ink2)}' +
		'.rx-leg b{color:var(--ink)}.rx-dir{flex:1;min-width:320px}.rx-num{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:12px 0 16px}.rx-num div{background:var(--side);border:1px solid var(--line);border-radius:14px;padding:12px}' +
		'.rx-num b{display:block;font-size:24px}.rx-num small{color:var(--muted)}.rx-pad{background:#FFF5EC;border:1px solid #FFD9BF;border-radius:14px;padding:12px 14px;margin-bottom:14px}.rx-pad li{margin:4px 0 4px 18px}' +
		'.rx-ev{border:1px solid var(--line);border-radius:14px;padding:12px;margin-bottom:10px;display:flex;gap:12px;background:#fff}.rx-ev.aberto{border-color:#F6B08A}' +
		'.rx-ev .n{flex:none;width:30px;height:30px;border-radius:50%;background:#FFE3D3;color:var(--warn);font-weight:800;display:flex;align-items:center;justify-content:center}.rx-ev.ok .n{background:var(--good-bg);color:var(--good)}' +
		'.rx-ev img{width:120px;height:90px;object-fit:cover;border-radius:10px;cursor:zoom-in;flex:none}.rx-ev .t{font-weight:700}.rx-ev .m{color:var(--ink2);font-size:12.5px;margin-top:4px;line-height:1.5}' +
		'.rx-ev .dica{font-size:12.5px;margin-top:6px;color:var(--ink)}.rx-dot{display:inline-block;width:10px;height:10px;border-radius:50%;vertical-align:-1px;margin-right:4px;border:1px solid rgba(0,0,0,.15)}' +
		'.rx-zoom{position:fixed;inset:0;background:rgba(20,18,16,.82);display:flex;align-items:center;justify-content:center;z-index:50;cursor:zoom-out}.rx-zoom img{max-width:92vw;max-height:88vh;border-radius:12px}</style>';
	var dados = null, carregando = false, erroMsg = "", selRegiao = null;

	function capaLocal(capa, fotoLocal) {
		// a mesma foto de perfil em que o site marcou as regiões, lida do próprio programa; sem ela, a foto do cartão
		if (capa && capa.foto) return "../../profiles/" + capa.foto.replace(/^\/static\/raiox\/capas\//, "");
		return fotoLocal || "";
	}
	function cor(c) { c = String(c || ""); return /^#?[0-9a-f]{6}/i.test(c) ? "#" + c.replace("#", "").slice(0, 6) : "#ccc"; }
	function quando(iso) { try { return new Date(iso).toLocaleString("pt-BR", { day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit" }); } catch (e) { return iso; } }

	function achar(u) {
		if (!dados) return null;
		var v = u.viva || {}, chaves = [v.cadastro_id, v.dev_id, u.farm && u.farm.id].filter(Boolean);
		var nome = String(u.nome || "").trim().toLowerCase();
		return dados.impressoras.filter(function (i) { return chaves.indexOf(i.chave) >= 0; })[0] ||
			dados.impressoras.filter(function (i) { return String(i.nome || "").trim().toLowerCase() == nome; })[0] || null;
	}

	function carregar(redesenhar) {
		if (carregando) return;
		if (!Filuna.temToken()) { erroMsg = "Entre na sua conta Filuna para guardar o Raio X das suas impressoras."; redesenhar(); return; }
		carregando = true;
		Filuna.api("GET", "/raiox").then(function (r) { dados = r; erroMsg = ""; }, function (e) { erroMsg = (e && e.erro) || "Não deu para ler o Raio X agora."; })
			.then(function () { carregando = false; redesenhar(); });
	}

	function desenhar(app, u, voltar, redesenhar) {
		if (!dados && !erroMsg) { app.innerHTML = CSS + '<div style="padding:40px">Carregando o Raio X…</div>'; carregar(redesenhar); return; }
		var r = achar(u) || { eventos: [], padroes: [], impressoes: 0, horas: 0, abertos: 0, capa: {} };
		var v = u.viva || {}, regioes = (dados && dados.regioes) || {}, pontos = (r.capa && r.capa.regioes) || {};
		var abertos = r.eventos.filter(function (e) { return !e.resolvido_em; });
		var atual = abertos[0], porRegiao = {};
		r.eventos.forEach(function (e) { porRegiao[e.regiao] = (porRegiao[e.regiao] || 0) + 1; });
		var foto = capaLocal(r.capa, u.foto);
		var pins = Object.keys(pontos).filter(function (k) { return pontos[k]; }).map(function (k) {
			var p = pontos[k], cls = atual && atual.regiao == k ? " erro" : porRegiao[k] ? " hist" : "";
			return '<div class="rx-pin' + cls + (selRegiao == k ? " sel" : "") + '" data-reg="' + k + '" title="' + E((regioes[k] || "") + (porRegiao[k] ? " · " + porRegiao[k] + " erro(s)" : "")) + '" style="left:' + (+p[0]) + "%;top:" + (+p[1]) + '%">' + k + "</div>";
		}).join("");
		var lista = r.eventos.filter(function (e) { return !selRegiao || e.regiao == selRegiao; });
		var html = CSS + '<div style="padding:16px 32px 0;display:flex;justify-content:space-between"><a class="voltar" id="rxVoltar">‹ ' + E(u.nome) + "</a>" +
			(selRegiao ? '<a class="voltar" id="rxTodas">Ver todas as regiões</a>' : "") + "</div>" +
			'<div style="padding:6px 32px 0"><div class="lbl">Raio X da impressora</div><div class="h1">' + E(u.nome) + '</div><div class="sub">' + E(r.modelo || u.modelo || "") + "</div></div>" +
			(erroMsg ? '<div class="aviso" style="margin:14px 32px"><span>' + E(erroMsg) + "</span></div>" : "") +
			'<div class="rx"><div class="rx-foto">' + (foto ? '<img class="capa" alt="" src="' + E(foto) + '">' : '<div class="semfoto">Sem foto deste modelo</div>') + pins +
			'<div class="rx-leg">' + Object.keys(regioes).map(function (k) { return "<span><b>" + k + "</b> " + E(regioes[k]) + "</span>"; }).join("") + "</div></div>" +
			'<div class="rx-dir">' +
			(v.erro ? '<div class="aviso" style="background:#FDE8DF;color:#D9480F;margin:0 0 10px"><span><b>Agora:</b> ' + E(v.erro) + "</span></div>" : "") +
			'<div class="rx-num"><div><b>' + (r.impressoes || 0) + "</b><small>impressões</small></div><div><b>" + String(r.horas || 0).replace(".", ",") + "</b><small>horas imprimindo</small></div><div><b>" + abertos.length + "</b><small>erro(s) em aberto</small></div></div>" +
			(r.padroes && r.padroes.length ? '<div class="rx-pad"><b>O que se repete</b><ul>' + r.padroes.map(function (p) { return "<li>" + E(p) + "</li>"; }).join("") + "</ul></div>" : "") +
			'<div class="lbl" style="margin-bottom:8px">' + (selRegiao ? "Erros na região " + selRegiao + " · " + E(regioes[selRegiao] || "") : "Histórico de erros") + "</div>" +
			(lista.length ? lista.map(function (e) {
				var f = e.filamento || {}, linha = [];
				if (e.camada) linha.push("camada " + e.camada + (e.camadas ? " de " + e.camadas : ""));
				if (e.z != null) linha.push("altura " + String(Math.round(e.z * 100) / 100).replace(".", ",") + " mm");
				if (e.progresso) linha.push(e.progresso + "% da impressão");
				linha.push(e.impressoes + " impressões · " + String(e.horas).replace(".", ",") + " h até aqui");
				var fil = f.material || f.marca ? '<span class="rx-dot" style="background:' + cor(f.cor) + '"></span>' + E([f.marca, f.material, f.cor_nome].filter(Boolean).join(" ")) + (f.bobina_numero ? " · bobina nº " + E(f.bobina_numero) : "") + (f.posicao ? " · entrada " + f.posicao : "") : "";
				return '<div class="rx-ev' + (e.resolvido_em ? " ok" : " aberto") + '"><div class="n">' + e.regiao + "</div>" +
					(e.foto_url ? '<img alt="Foto da câmera na hora do erro" data-zoom="' + E(SITE + e.foto_url) + '" src="' + E(SITE + e.foto_url) + '">' : "") +
					'<div style="min-width:0"><div class="t">' + E(e.texto || "Erro da impressora") + "</div>" +
					'<div class="m">' + quando(e.criado_em) + " · " + E(e.regiao_nome) + (e.codigo ? " · código " + E(e.codigo) : "") + "<br>" + linha.map(E).join(" · ") + (fil ? "<br>" + fil : "") + "</div>" +
					(e.resolvido_em ? '<div class="m" style="color:var(--good)">Resolvido · ' + E(e.solucao || "") + "</div>"
						: '<div class="dica"><b>O que conferir:</b> ' + E(e.dica) + '</div><div style="margin-top:8px;display:flex;gap:6px"><input type="text" data-sol="' + e.id + '" placeholder="O que resolveu (opcional)" style="flex:1;min-width:0;padding:7px 10px;border:1px solid var(--line);border-radius:10px"><button class="btn peq" data-res="' + e.id + '">Resolvido</button></div>') +
					"</div></div>";
			}).join("") : '<div class="vazio" style="padding:14px 0;color:var(--muted)">' + (selRegiao ? "Nenhum erro nesta região." : "Nenhum erro registrado. Quando a impressora informar um erro, ele aparece aqui com a foto da câmera, a camada e o filamento.") + "</div>") +
			"</div></div>";
		app.innerHTML = html;
		app.querySelector("#rxVoltar").onclick = function () { selRegiao = null; voltar(); };
		var t = app.querySelector("#rxTodas"); if (t) t.onclick = function () { selRegiao = null; redesenhar(); };
		app.querySelectorAll("[data-reg]").forEach(function (p) { p.onclick = function () { selRegiao = selRegiao == p.dataset.reg ? null : p.dataset.reg; redesenhar(); }; });
		app.querySelectorAll("[data-zoom]").forEach(function (im) {
			im.onclick = function () {
				var z = document.createElement("div"); z.className = "rx-zoom"; z.innerHTML = '<img alt="" src="' + E(im.dataset.zoom) + '">';
				z.onclick = function () { z.remove(); }; document.body.appendChild(z);
			};
		});
		app.querySelectorAll("[data-res]").forEach(function (b) {
			b.onclick = function () {
				var campo = app.querySelector('[data-sol="' + b.dataset.res + '"]');
				b.disabled = true;
				Filuna.api("POST", "/raiox/" + encodeURIComponent(b.dataset.res) + "/resolver", { solucao: campo ? campo.value : "" })
					.then(function () { dados = null; carregar(redesenhar); }, function (e) { erroMsg = (e && e.erro) || "Não deu para marcar agora."; redesenhar(); });
			};
		});
	}

	return { desenhar: desenhar, recarregar: function () { dados = null; } };
})();
