// Assistência técnica › Raio X › Diagnóstico: o Raio X do EQUIPAMENTO. Lê a impressora inteira (sensores, força dos
// aquecedores, coolers, drivers, placas, umidade, registros), mostra o que está fora do normal e roda testes guiados.
// Os testes que aquecem ou movimentam só começam com o cliente tocando em "Estou ao lado da impressora" — os limites e
// o desligamento do calor ficam no programa (FilunaDiagnostico.cpp), não nesta tela nem na IA. Tudo vai para o Raio X
// da conta: sessões, testes, a impressão digital da impressora saudável, tendências e comparação com a frota.
var RaioXDiag = (function () {
	var E = Filuna.esc;
	var CSS = '<style id="dgCss">.dg-card{background:#fff;border:1px solid var(--line);border-radius:18px;padding:14px 16px;margin-bottom:14px}' +
		'.dg-card h3{font-size:16px;margin:0 0 4px}.dg-card>p{color:var(--ink2);font-size:13.5px;margin:2px 0 10px}' +
		'.dg-ach{display:flex;gap:10px;align-items:flex-start;padding:8px 0;border-top:1px solid var(--line)}.dg-ach:first-of-type{border-top:0}' +
		'.dg-niv{flex:none;font-size:11px;font-weight:800;border-radius:999px;padding:2px 9px;margin-top:2px}.dg-niv.falha{background:#FDE8DF;color:var(--warn)}' +
		'.dg-niv.atencao{background:#FFF1DE;color:#B45309}.dg-niv.info,.dg-niv.ok{background:var(--good-bg);color:var(--good)}.dg-niv.confirmar{background:#EEF2FF;color:#3730A3}' +
		'.dg-ach b{display:block;font-size:14px}.dg-ach span{color:var(--ink2);font-size:13px}' +
		'.dg-testes{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:10px}.dg-t{border:1px solid var(--line);border-radius:14px;padding:10px 12px;display:flex;flex-direction:column;gap:6px}' +
		'.dg-t.sug{border-color:var(--magenta);box-shadow:0 0 0 1px var(--magenta) inset}.dg-t small{color:var(--muted);font-size:12px}.dg-t p{color:var(--ink2);font-size:12.5px;margin:0;flex:1}' +
		'.dg-risco{font-size:11px;font-weight:800;border-radius:999px;padding:1px 8px;margin-left:6px}.dg-risco.leitura{background:var(--good-bg);color:var(--good)}' +
		'.dg-risco.aquece{background:#FFF1DE;color:#B45309}.dg-risco.move{background:#FDE8DF;color:var(--warn)}' +
		'.dg-pres{display:flex;gap:10px;align-items:center;flex-wrap:wrap;background:var(--side);border-radius:12px;padding:10px 12px;margin-bottom:10px;font-size:13.5px}' +
		'.dg-pres.ok{background:var(--good-bg)}.dg-run{border:1px solid var(--magenta);border-radius:14px;padding:12px;margin-bottom:10px}' +
		'.dg-run pre{white-space:pre-wrap;font-size:12px;background:var(--side);border-radius:10px;padding:8px;max-height:160px;overflow:auto;margin:8px 0 0}' +
		'.dg-graf{width:100%;height:120px;background:var(--side);border-radius:12px;margin-top:8px}.dg-leg{display:flex;gap:12px;font-size:12px;color:var(--ink2);margin-top:4px;flex-wrap:wrap}' +
		'.dg-leg i{display:inline-block;width:10px;height:3px;border-radius:2px;margin-right:4px;vertical-align:middle}' +
		'.dg-ses{border-top:1px solid var(--line);padding:10px 0}.dg-ses:first-of-type{border-top:0}.dg-it{font-size:13px;color:var(--ink2);margin:3px 0 0 4px}' +
		'.dg-linha{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:8px}.dg-linha input{flex:1;min-width:180px;padding:7px 10px;border:1px solid var(--line);border-radius:10px;font:inherit}</style>';
	var st = { chave: "", dev: "", painel: null, testes: null, leitura: null, lendo: false, teste: null, erro: "", sup: null, carregado: false, ouvindo: false };

	function quando(iso) { try { return new Date(typeof iso == "number" ? iso * 1000 : iso).toLocaleString("pt-BR", { day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit" }); } catch (e) { return ""; } }
	function nivel(n) { return { falha: "Problema", atencao: "Atenção", info: "Ok", ok: "Normal", confirmar: "Confirmar", interrompido: "Parado", indisponivel: "Sem recurso" }[n] || n || ""; }
	function achadosHtml(l) {
		if (!l || !l.length) return '<p class="rx-txt">Nada registrado.</p>';
		return l.map(function (a) {
			return '<div class="dg-ach"><span class="dg-niv ' + E(a.nivel) + '">' + E(nivel(a.nivel)) + "</span><div><b>" + E(a.titulo) + "</b><span>" + E(a.texto) + "</span>" +
				(a.teste && st.testes && (st.testes.testes || []).some(function (t) { return t.id == a.teste; }) ? ' <a class="voltar" data-dg-sug="' + E(a.teste) + '">Testar agora</a>' : "") + "</div></div>";
		}).join("");
	}
	function grafico(pts, series) {
		var vals = [];
		series.forEach(function (s) { pts.forEach(function (p) { var v = s.v(p); if (v != null && isFinite(v)) vals.push(v); }); });
		if (pts.length < 2 || !vals.length) return "";
		var mn = Math.min.apply(null, vals), mx = Math.max.apply(null, vals);
		if (mx - mn < 1e-6) { mx += 1; mn -= 1; }
		var W = 600, H = 120;
		var linhas = series.map(function (s) {
			var d = "";
			pts.forEach(function (p, i) { var v = s.v(p); if (v == null || !isFinite(v)) return; d += (d ? " L" : "M") + (i / (pts.length - 1) * W).toFixed(1) + " " + (H - 8 - (v - mn) / (mx - mn) * (H - 16)).toFixed(1); });
			return d ? '<path d="' + d + '" fill="none" stroke="' + s.cor + '" stroke-width="2"/>' : "";
		}).join("");
		return '<svg class="dg-graf" viewBox="0 0 ' + W + " " + H + '" preserveAspectRatio="none">' + linhas + "</svg>" +
			'<div class="dg-leg">' + series.map(function (s) { return '<span><i style="background:' + s.cor + '"></i>' + E(s.nome) + "</span>"; }).join("") +
			"<span>" + E(String(Math.round(mn * 10) / 10)) + " – " + E(String(Math.round(mx * 10) / 10)) + "</span></div>";
	}

	function carregar(redesenhar) {
		if (!st.chave) return;
		Promise.all([
			Filuna.api("GET", "/raiox/diagnostico?chave=" + encodeURIComponent(st.chave)).catch(function (e) { return { erro: (e && e.erro) || "sem resposta" }; }),
			st.dev ? Filuna.nativo("diagnostico.testes", { dev_id: st.dev }).catch(function () { return null; }) : Promise.resolve(null),
			Filuna.api("GET", "/suporte/estado").catch(function () { return null; }),
			Filuna.nativo("diagnostico.teste.estado", {}).catch(function () { return null; })
		]).then(function (r) {
			st.painel = r[0] && !r[0].erro ? r[0] : null; st.testes = r[1]; st.sup = r[2];
			if (r[3] && r[3].dev_id == st.dev) st.teste = r[3];
			st.carregado = true; redesenhar();
		});
	}

	function testeHtml() {
		var t = st.teste;
		if (!t || t.dev_id != st.dev || !t.estado) return "";
		var res = t.resultado || {}, rodando = t.estado == "rodando" || t.estado == "esperando_presenca";
		var graf = t.amostras && t.amostras.length > 1 ? grafico(t.amostras, [{ nome: "temperatura °C", cor: "#E6007E", v: function (p) { return p.temp; } }]) : "";
		var cons = (t.console || res.console || []).slice(-12).join("\n");
		return '<div class="dg-run"><b>' + E(t.nome || t.teste) + "</b> · " + (t.estado == "esperando_presenca" ? "esperando você confirmar que está ao lado da impressora" :
			rodando ? "rodando…" : E(nivel(res.resultado))) + (rodando ? ' <button class="btn peq" id="dgParar" style="float:right">Parar agora</button>' : "") +
			(res.texto ? '<div class="rx-m" style="margin-top:6px">' + E(res.texto) + "</div>" : "") + graf + (cons ? "<pre>" + E(cons) + "</pre>" : "") +
			(res.perguntar && !rodando ? '<div class="dg-linha"><span>' + E(res.perguntar) + '</span><button class="btn peq" data-dg-resp="sim">Sim</button><button class="btn peq" data-dg-resp="nao">Não</button></div>' : "") +
			(t.analise && t.analise.achados ? '<div style="margin-top:8px">' + achadosHtml(t.analise.achados) + "</div>" : "") + "</div>";
	}

	function html(u, chave) {
		if (chave != st.chave) { st.painel = null; st.testes = null; st.leitura = null; st.teste = null; st.carregado = false; st.erro = ""; }
		st.chave = chave; st.dev = (u.viva && u.viva.dev_id) || u.dev_id || "";
		if (!st.carregado) return CSS + '<p class="rx-txt">Carregando o diagnóstico…</p>';
		var p = st.painel || { sessoes: [], alertas: [], serie: [], frota: {} }, ts = st.testes || {}, sug = (st.leitura && st.leitura.analise && st.leitura.analise.testes_sugeridos) || [];
		var h = CSS + (st.erro ? '<div class="aviso" style="margin-bottom:12px"><span>' + E(st.erro) + "</span></div>" : "");
		// 1. leitura
		h += '<div class="dg-card"><h3>Raio X do equipamento</h3><p>A Filuna lê tudo o que esta impressora informa e mostra o que está fora do normal. Não aquece nem move nada.</p>' +
			'<button class="btn primario" id="dgLer"' + (st.lendo || !st.dev ? " disabled" : "") + ">" + (st.lendo ? "Lendo a impressora…" : "Ler a impressora agora") + "</button>" +
			(!st.dev ? '<div class="rx-m">Ligue a impressora e abra o cartão dela para o diagnóstico.</div>' : "") +
			(st.leitura && st.leitura.analise ? '<div style="margin-top:10px">' + achadosHtml(st.leitura.analise.achados) + "</div>" +
				(st.leitura.analise.aviso ? '<div class="rx-m">' + E(st.leitura.analise.aviso) + "</div>" : "") : "") + "</div>";
		// 2. testes
		var pres = ts.presenca;
		h += '<div class="dg-card"><h3>Testes guiados</h3><p>Os que aquecem ou movimentam só rodam com você ao lado da impressora. O programa limita a temperatura e desliga tudo no fim.</p>' +
			(!st.dev ? "" : '<div class="dg-pres' + (pres ? " ok" : "") + '">' + (pres ? "✓ Você confirmou que está ao lado da impressora (vale " + (ts.presenca_minutos || 20) + " min)." :
				'Para testes que aquecem ou movem: <button class="btn peq primario" id="dgPresenca">Estou ao lado da impressora</button>') + "</div>") + testeHtml() +
			((ts.testes || []).length ? '<div class="dg-testes">' + ts.testes.map(function (t) {
				return '<div class="dg-t' + (sug.indexOf(t.id) >= 0 ? " sug" : "") + '"><b>' + E(t.nome) + '<span class="dg-risco ' + E(t.risco) + '">' +
					(t.risco == "leitura" ? "só lê" : t.risco == "aquece" ? "aquece" : "movimenta") + "</span></b><small>cerca de " + t.minutos + " min" + (sug.indexOf(t.id) >= 0 ? " · sugerido" : "") +
					"</small><p>" + E(t.explicacao) + '</p><button class="btn peq" data-dg-teste="' + E(t.id) + '">Rodar</button></div>';
			}).join("") + "</div>" : '<p class="rx-txt">' + (st.dev ? "Esta impressora não deixa o Filuna rodar testes pela rede." : "Ligue a impressora e abra o cartão dela para ver os testes.") + "</p>") +
			((ts.guiados || []).length ? '<div style="margin-top:10px"><b>Testes pela tela da impressora</b><ul style="margin:6px 0 0 18px;font-size:13.5px">' +
				ts.guiados.map(function (g) { return "<li>" + E(g) + "</li>"; }).join("") + "</ul></div>" : "") + "</div>";
		// 3. acesso remoto
		var sup = st.sup || {};
		h += '<div class="dg-card"><h3>Acesso remoto da Filuna</h3><p>Para o suporte da Filuna ou a sua IA (Claude/ChatGPT pelo conector) lerem e testarem esta impressora daqui.</p>' +
			(sup.ativa ? '<div class="dg-linha"><span>Autorizado ' + (sup.permanente ? "até você desligar" : "até " + E(quando(sup.expira_em))) + " · " +
				E((sup.categorias || []).join(", ")) + '</span><button class="btn peq" id="dgEncerrar">Encerrar acesso</button></div>' :
				'<button class="btn peq primario" id="dgAutorizar">Autorizar a Filuna por 1 hora</button><div class="rx-m">Diagnóstico e testes da impressora. Você pode encerrar quando quiser; fica tudo registrado.</div>') + "</div>";
		// 4. saúde ao longo do tempo
		var pts = p.serie || [];
		var temPot = pts.some(function (x) { return x.pb != null; }), temUm = pts.some(function (x) { return x.u != null; });
		h += '<div class="dg-card"><h3>Saúde ao longo do tempo</h3>' +
			(p.base ? "<p>Impressão digital gravada em " + E(quando(p.base.criado_em)) + ': os testes novos são comparados com ela. <a class="voltar" id="dgBase">Gravar de novo</a></p>' :
				'<p>Grave a impressão digital com a impressora saudável (depois de uma leitura e de um teste normal): a Filuna passa a mostrar o que mudou. <button class="btn peq" id="dgBase">Gravar impressão digital</button></p>') +
			((p.alertas || []).length ? '<div style="margin-bottom:8px">' + p.alertas.slice(0, 5).map(function (a) {
				return '<div class="dg-ach"><span class="dg-niv atencao">Prevista</span><div><b>Manutenção prevista · ' + E(quando(a.criado_em)) + "</b><span>" + E(a.texto) + "</span></div></div>";
			}).join("") + "</div>" : "") +
			(pts.length > 1 ? "<b style=\"font-size:13px\">Últimos 7 dias</b>" + grafico(pts, [{ nome: "bico °C", cor: "#E6007E", v: function (x) { return x.b; } }, { nome: "mesa °C", cor: "#2F6FEB", v: function (x) { return x.m; } }]) +
				(temPot ? grafico(pts, [{ nome: "força do aquecedor do bico (0–1)", cor: "#F59E0B", v: function (x) { return x.pb; } }]) : "") +
				(temUm ? grafico(pts, [{ nome: "umidade da caixa de filamento %", cor: "#8B5CF6", v: function (x) { return x.u; } }]) : "") :
				'<p class="rx-txt">O histórico aparece aqui com a impressora ligada e o Filuna Slicer aberto.</p>') +
			Object.keys(p.frota || {}).map(function (k) {
				var f = p.frota[k];
				return '<div class="rx-m">' + E((p.testes_nomes || {})[k] || k) + ": " + E(String(f.seu)) + " s; mediana das " + f.n + " impressoras iguais no Filuna: " + E(String(f.mediana)) + " s.</div>";
			}).join("") + "</div>";
		// 5. histórico
		h += '<div class="dg-card"><h3>Histórico de diagnósticos</h3>' + ((p.sessoes || []).length ? p.sessoes.map(function (s, i) {
			return '<div class="dg-ses"><b>' + E(quando(s.aberta_em)) + "</b> · " + (s.fechada_em ? "concluído" : "aberto") + (s.origem && s.origem != "cliente" ? " · pedido pelo " + E(s.origem) : "") +
				(s.conclusao ? '<div class="rx-m"><b>Conclusão:</b> ' + E(s.conclusao) + "</div>" : "") +
				s.itens.map(function (it) { return '<div class="dg-it">• ' + E(it.nome) + " — " + E(nivel(it.resultado)) + (it.resumo ? ": " + E(String(it.resumo).slice(0, 160)) : "") + " <small>(" + E(quando(it.em)) + ")</small></div>"; }).join("") +
				(!s.fechada_em && i == 0 ? '<div class="dg-linha"><input id="dgConc" maxlength="500" placeholder="O que foi resolvido (opcional)"><button class="btn peq" data-dg-conc="' + E(s.id) + '">Concluir diagnóstico</button></div>' : "") + "</div>";
		}).join("") : '<p class="rx-txt">Nenhum diagnóstico ainda.</p>') + "</div>";
		return h;
	}

	function ligar(app, u, chave, redesenhar) {
		var q = function (s) { return app.querySelector(s); };
		if (!st.ouvindo) {
			st.ouvindo = true;
			Filuna.ouvir("diagnostico.teste", function (e) {
				st.teste = e;
				if (e && (e.estado == "concluido" || e.estado == "interrompido") && e.analise) { st.painel = null; carregar(redesenhar); return; }
				redesenhar();
			});
		}
		if (!st.carregado) { carregar(redesenhar); return; }
		var falha = function (e) { st.erro = (e && (e.erro || e.message)) || "Não deu para fazer isso agora."; redesenhar(); };
		var b = q("#dgLer");
		if (b) b.onclick = function () {
			st.lendo = true; st.erro = ""; redesenhar();
			Filuna.nativo("diagnostico.ler", { dev_id: st.dev }).then(function (r) {
				if (r && r.erro) throw r;
				st.leitura = r; st.lendo = false; carregar(redesenhar);
			}).catch(function (e) { st.lendo = false; falha(e); });
		};
		b = q("#dgPresenca");
		if (b) b.onclick = function () { Filuna.nativo("diagnostico.presenca", { dev_id: st.dev }).then(function () { carregar(redesenhar); }, falha); };
		app.querySelectorAll("[data-dg-teste],[data-dg-sug]").forEach(function (x) {
			x.onclick = function () {
				var id = x.dataset.dgTeste || x.dataset.dgSug;
				st.erro = "";
				Filuna.nativo("diagnostico.testar", { dev_id: st.dev, teste: id, origem: "cliente" }).then(function (r) {
					if (r && r.erro) throw r;
					return Filuna.nativo("diagnostico.teste.estado", {});
				}).then(function (e) { st.teste = e; redesenhar(); }).catch(falha);
			};
		});
		b = q("#dgParar");
		if (b) b.onclick = function () { b.disabled = true; Filuna.nativo("diagnostico.parar", {}).catch(falha); };
		app.querySelectorAll("[data-dg-resp]").forEach(function (x) {
			x.onclick = function () {
				var t = st.teste || {};
				Filuna.api("POST", "/raiox/diagnostico/registrar", { chave: chave, tipo: "nota", origem: "cliente",
					texto: (t.nome || "Teste") + ": " + (t.resultado && t.resultado.perguntar || "") + " " + (x.dataset.dgResp == "sim" ? "Sim." : "Não.") })
					.then(function () { st.teste.resultado.perguntar = ""; carregar(redesenhar); }, falha);
			};
		});
		b = q("#dgAutorizar");
		if (b) b.onclick = function () { Filuna.api("POST", "/suporte/autorizar", { horas: 1, categorias: ["diagnostico", "impressora"] }).then(function () { carregar(redesenhar); }, falha); };
		b = q("#dgEncerrar");
		if (b) b.onclick = function () { Filuna.api("POST", "/suporte/encerrar", {}).then(function () { carregar(redesenhar); }, falha); };
		b = q("#dgBase");
		if (b) b.onclick = function () { Filuna.api("POST", "/raiox/diagnostico/base", { chave: chave }).then(function () { carregar(redesenhar); }, falha); };
		app.querySelectorAll("[data-dg-conc]").forEach(function (x) {
			x.onclick = function () {
				var c = q("#dgConc");
				Filuna.api("POST", "/raiox/diagnostico/" + encodeURIComponent(x.dataset.dgConc) + "/concluir", { conclusao: c ? c.value : "" }).then(function () { carregar(redesenhar); }, falha);
			};
		});
	}

	return { html: html, ligar: ligar, contar: function () { return st.painel ? (st.painel.alertas || []).filter(function (a) { return !a.visto_em; }).length : 0; },
		recarregar: function () { st.painel = null; st.testes = null; st.leitura = null; st.carregado = false; st.erro = ""; } };
})();
