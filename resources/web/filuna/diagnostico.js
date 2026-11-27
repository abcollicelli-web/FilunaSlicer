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
		'.dg-linha{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:8px}.dg-linha input{flex:1;min-width:180px;padding:7px 10px;border:1px solid var(--line);border-radius:10px;font:inherit}' +
		'.dg-card.dg-cu{border-color:var(--magenta)}.dg-passos{margin:6px 0 10px 18px;padding:0;font-size:13.5px;color:var(--ink2)}.dg-marca{background:var(--side);border-radius:12px;padding:8px 12px;font-size:13px;margin-bottom:10px}' +
		'.dg-ok{display:flex;gap:8px;align-items:center;font-size:13.5px;margin:4px 0}.dg-ok input{width:18px;height:18px}.dg-cu-l{list-style:none;margin:8px 0;padding:0}' +
		'.dg-cu-l li{display:flex;gap:8px;align-items:baseline;padding:4px 0;font-size:13.5px;border-top:1px solid var(--line)}.dg-cu-l li:first-child{border-top:0}.dg-cu-l small{color:var(--ink2)}</style>';
	var st = { chave: "", dev: "", viva: null, painel: null, testes: null, leitura: null, lendo: false, teste: null, erro: "", sup: null, carregado: false, ouvindo: false,
		checkup: null };
	// Check-up completo ("minha impressora está ruim"): lê todos os sensores e roda, um por vez, os testes que a marca
	// permite. Cada teste passa pelas mesmas travas do programa (presença confirmada aqui, limites, desliga tudo no fim);
	// os que movimentam só entram se o cliente disser que a mesa está vazia. Calibração (PID) fica de fora: é avulsa.
	var ORDEM = ["consultas", "ruido_acelerometro", "coolers", "aquecimento_bico", "aquecimento_mesa"], ORDEM_MOVE = ["eixos", "malha_mesa", "precisao_sonda", "ressonancia"];

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

	function cuTestes(ts, move) {
		var ids = {};
		(ts.testes || []).forEach(function (t) { ids[t.id] = t; });
		return ORDEM.concat(move ? ORDEM_MOVE : []).filter(function (id) { return ids[id]; }).map(function (id) { return ids[id]; });
	}
	function temCamera(v) { return !!(v && (v.camera_flv || v.camera_repasse)); }
	function checkupHtml(ts) {
		var cu = st.checkup, fisicos = (ts.testes || []).some(function (t) { return t.risco != "leitura"; });
		var movem = (ts.testes || []).some(function (t) { return t.risco == "move"; });
		var h = '<div class="dg-card dg-cu"><h3>Algo errado com a impressora?</h3><p>Faça o check-up completo: a Filuna confere tudo o que esta impressora deixa e diz o que fazer.</p>' +
			'<ol class="dg-passos"><li>Lê todos os sensores (temperaturas, coolers, motores, avisos, câmera).</li><li>Roda, um por vez, os testes que esta impressora permite.</li>' +
			"<li>Mostra o resultado e guarda tudo no Raio X da impressora.</li></ol>" + (ts.permite ? '<div class="dg-marca">' + E(ts.permite) + "</div>" : "");
		if (!st.dev) return h + '<div class="rx-m">Ligue a impressora e abra o cartão dela para o check-up.</div></div>';
		if (cu && cu.rodando) {
			return h + '<ul class="dg-cu-l">' + cu.lista.map(function (p, i) {
				var est = p.res ? nivel(p.res) : i == cu.i ? (p.espera ? "esperando você" : "rodando…") : "na fila";
				return '<li><span class="dg-niv ' + E(p.res || (i == cu.i ? "confirmar" : "")) + '">' + E(est) + "</span><div><b>" + E(p.nome) + "</b>" + (p.texto ? " <small>" + E(p.texto) + "</small>" : "") + "</div></li>";
			}).join("") + '</ul><button class="btn peq" id="dgCuParar">Parar o check-up</button></div>';
		}
		if (cu && cu.fim) h += resultadoHtml(cu, ts);
		var min = 2 + cuTestes(ts, true).reduce(function (n, t) { return n + (t.minutos || 1); }, 0);
		return h + (fisicos ? '<label class="dg-ok"><input type="checkbox" id="dgCuPres"> Estou ao lado da impressora (necessário para os testes que aquecem)</label>' : "") +
			(movem ? '<label class="dg-ok"><input type="checkbox" id="dgCuMove"> A mesa está limpa e vazia: incluir os testes que movimentam (eixos, mapa da mesa, sonda e vibração)</label>' : "") +
			'<button class="btn primario" id="dgCheckup"' + (st.lendo ? " disabled" : "") + ">" + (cu && cu.fim ? "Fazer o check-up de novo" : "Fazer o check-up completo") + "</button>" +
			'<div class="rx-m">Leva de 1 a ' + min + " min, conforme os testes. Você pode parar quando quiser: o programa desliga o calor e o cooler.</div></div>";
	}
	function resultadoHtml(cu, ts) {
		var vistos = {}, fazer = [];
		cu.achados.forEach(function (a) {
			if ((a.nivel == "falha" || a.nivel == "atencao") && !vistos[a.titulo + a.texto]) { vistos[a.titulo + a.texto] = 1; fazer.push(a); }
		});
		fazer.sort(function (a, b) { return (a.nivel == "falha" ? 0 : 1) - (b.nivel == "falha" ? 0 : 1); });
		var falhas = fazer.filter(function (a) { return a.nivel == "falha"; }).length;
		var h = '<div class="dg-run"><b>Resultado do check-up · ' + E(quando(cu.fim)) + "</b>" + '<ul class="dg-cu-l">' + cu.lista.map(function (p) {
			return '<li><span class="dg-niv ' + E(p.res || "") + '">' + E(nivel(p.res) || "—") + "</span><div><b>" + E(p.nome) + "</b>" + (p.texto ? " <small>" + E(p.texto) + "</small>" : "") + "</div></li>";
		}).join("") + "</ul>";
		h += fazer.length ? "<b>O que fazer</b>" + achadosHtml(fazer) : '<div class="dg-ach"><span class="dg-niv ok">Normal</span><div><b>Tudo certo no que foi testado</b><span>Se a impressão ainda sai ruim, o problema pode estar no fatiamento ou no filamento: rode o Teste de impressão.</span></div></div>';
		if (cu.pulados) h += '<div class="rx-m">' + E(cu.pulados) + "</div>";
		if ((ts.guiados || []).length) h += '<div class="rx-m">Para completar, rode também pela tela da impressora: ' + E(ts.guiados.join(" ")) + "</div>";
		if (falhas) h += '<div class="rx-m">Precisa de ajuda? Abra o reparo pelo erro na aba Erros, ou toque em "Autorizar a Filuna por 1 hora" mais abaixo para o suporte (ou a sua IA) olhar com você.</div>';
		return h + "</div>";
	}
	// Espera o teste pedido terminar e a análise do site chegar (o programa publica o resultado e, depois, a análise).
	function esperarFim(id, desde) {
		return new Promise(function (ok) {
			var concluidoEm = 0;
			var passo = function () {
				if (st.checkup && st.checkup.parado) return ok({ resultado: { resultado: "interrompido", texto: "Check-up parado." } });
				Filuna.nativo("diagnostico.teste.estado", {}).then(function (e) {
					if (e && e.teste == id && (e.inicio || 0) >= desde - 5) {
						var p = st.checkup && st.checkup.lista[st.checkup.i];
						if (p) p.espera = e.estado == "esperando_presenca";
						if (e.estado == "interrompido" || (e.estado == "concluido" && (e.analise || (concluidoEm && Date.now() - concluidoEm > 60000)))) return ok(e);
						if (e.estado == "concluido" && !concluidoEm) concluidoEm = Date.now();
					}
					setTimeout(passo, 2000);
				}, function () { setTimeout(passo, 3000); });
			};
			setTimeout(passo, 1500);
		});
	}
	function testeCamera() {
		var v = st.viva;
		var r = !temCamera(v) ? Promise.resolve({ resultado: "indisponivel", texto: "A câmera desta impressora não abre por este programa." }) :
			FilunaCameraConta.foto(v).then(function (b) {
				return b ? { resultado: "ok", texto: "A câmera mandou imagem (" + Math.round(b.size / 1024) + " KB).", foto_kb: Math.round(b.size / 1024) } :
					{ resultado: "falha", texto: "A câmera não mandou imagem em 15 s: confira se ela está ligada e se a impressora está na mesma rede deste computador." };
			});
		return r.then(function (res) {
			return Filuna.api("POST", "/raiox/diagnostico/registrar", { chave: st.chave, dev_id: st.dev, tipo: "teste", teste: "camera", origem: "cliente",
				nome: (v && v.nome) || "", modelo: (v && v.modelo_nome) || "", agente: (v && v.agente) || "", dados: res })
				.then(function (an) { return { resultado: res, analise: an }; }, function () { return { resultado: res, analise: null }; });
		});
	}
	function checkup(redesenhar, presente, move) {
		var ts = st.testes || {}, lista = [{ id: "leitura", nome: "Leitura de todos os sensores" }];
		if (st.viva && st.viva.camera && window.FilunaCameraConta && FilunaCameraConta.foto) lista.push({ id: "camera", nome: "Câmera" });
		var pulados = [];
		cuTestes(ts, move).forEach(function (t) {
			if (t.risco == "leitura" || presente) lista.push({ id: t.id, nome: t.nome });
			else pulados.push(t.nome);
		});
		var cu = st.checkup = { lista: lista, i: 0, rodando: true, achados: [], parado: false,
			pulados: pulados.length ? "Ficaram de fora porque você não marcou que está ao lado da impressora: " + pulados.join(", ") + "." : "" };
		st.erro = "";
		var guardar = function (p, e) {
			var res = (e && e.resultado) || {};
			p.res = res.resultado || "falha"; p.texto = res.texto || ""; p.espera = false;
			((e && e.analise && e.analise.achados) || []).forEach(function (a) { cu.achados.push(a); });
		};
		var cadeia = presente ? Filuna.nativo("diagnostico.presenca", { dev_id: st.dev }) : Promise.resolve();
		lista.forEach(function (p, i) {
			cadeia = cadeia.then(function () {
				if (cu.parado) { p.res = "interrompido"; return; }
				cu.i = i; redesenhar();
				if (p.id == "leitura")
					return Filuna.nativo("diagnostico.ler", { dev_id: st.dev }).then(function (r) {
						if (r && r.erro) throw r;
						st.leitura = r; guardar(p, { resultado: { resultado: (r.analise && r.analise.achados && r.analise.achados[0] || {}).nivel || "info",
							texto: (r.analise && r.analise.aviso) || "" }, analise: r.analise });
					});
				if (p.id == "camera") return testeCamera().then(function (e) { guardar(p, e); });
				var desde = Math.floor(Date.now() / 1000);
				return Filuna.nativo("diagnostico.testar", { dev_id: st.dev, teste: p.id, origem: "cliente" }).then(function (r) {
					if (r && r.erro) throw r;
					return esperarFim(p.id, desde);
				}).then(function (e) {
					guardar(p, e);
					if (p.res == "interrompido") cu.parado = true; // parado na impressora ou sem presença: não segue sozinho
				});
			}).catch(function (e) {
				p.res = "falha"; p.texto = (e && (e.erro || e.message)) || "Não deu para rodar agora.";
				if (p.id == "leitura") cu.parado = true; // sem a leitura (impressora fora da rede) os testes também não rodam
			});
		});
		cadeia.then(function () {
			cu.rodando = false; cu.fim = new Date().toISOString();
			var conta = function (n) { return cu.lista.filter(function (p) { return p.res == n; }).length; };
			Filuna.api("POST", "/raiox/diagnostico/registrar", { chave: st.chave, tipo: "nota", origem: "cliente",
				texto: "Check-up completo" + (cu.parado ? " (parado antes do fim)" : "") + ": " + cu.lista.length + " etapas; " + conta("falha") + " com problema, " +
					conta("atencao") + " com atenção." + (cu.pulados ? " " + cu.pulados : "") }).catch(function () {});
			st.painel = null; carregar(redesenhar);
		});
	}

	function html(u, chave) {
		if (chave != st.chave) {
			st.painel = null; st.testes = null; st.leitura = null; st.teste = null; st.carregado = false; st.erro = "";
			if (st.checkup && !st.checkup.rodando) st.checkup = null;
		}
		st.chave = chave; st.dev = (u.viva && u.viva.dev_id) || u.dev_id || ""; st.viva = u.viva || null;
		if (!st.carregado) return CSS + '<p class="rx-txt">Carregando o diagnóstico…</p>';
		var p = st.painel || { sessoes: [], alertas: [], serie: [], frota: {} }, ts = st.testes || {}, sug = (st.leitura && st.leitura.analise && st.leitura.analise.testes_sugeridos) || [];
		var h = CSS + (st.erro ? '<div class="aviso" style="margin-bottom:12px"><span>' + E(st.erro) + "</span></div>" : "");
		h += checkupHtml(ts);
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
		var b = q("#dgCheckup");
		if (b) b.onclick = function () {
			var pres = q("#dgCuPres"), mv = q("#dgCuMove");
			checkup(redesenhar, !!(pres && pres.checked), !!(mv && mv.checked && pres && pres.checked));
		};
		b = q("#dgCuParar");
		if (b) b.onclick = function () { b.disabled = true; if (st.checkup) st.checkup.parado = true; Filuna.nativo("diagnostico.parar", {}).catch(falha); };
		b = q("#dgLer");
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

	// A aba só aparece depois que o dono libera no site (ou para administradores): o mesmo programa pode chegar aos
	// clientes antes da liberação.
	var lib = { valor: null, pedindo: false };
	function visivel(redesenhar) {
		if (lib.valor === null && !lib.pedindo && Filuna.temToken()) {
			lib.pedindo = true;
			Filuna.api("GET", "/raiox/diagnostico/liberado").then(function (r) { lib.valor = !!(r && r.liberado); },
				function () { lib.valor = false; setTimeout(function () { lib.valor = null; }, 60000); })
				.then(function () { lib.pedindo = false; if (lib.valor) redesenhar(); });
		}
		return !!lib.valor;
	}

	return { html: html, ligar: ligar, visivel: visivel, contar: function () { return st.painel ? (st.painel.alertas || []).filter(function (a) { return !a.visto_em; }).length : 0; },
		recarregar: function () { st.painel = null; st.testes = null; st.leitura = null; st.carregado = false; st.erro = ""; if (st.checkup && !st.checkup.rodando) st.checkup = null; } };
})();
