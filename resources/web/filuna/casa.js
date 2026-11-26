// Filuna Home dentro do Filuna Slicer (Dispositivo › Casa). O Slicer está na rede da casa: procura os aparelhos
// (FilunaCasa.cpp), manda a lista para a conta (/casa/dispositivos) e é aqui que cada aparelho é configurado:
// identificar → o Filuna ensina a instalar → teste de conexão (mensagem de teste) → personalizar (texto, cores,
// estilo do popup) → usar (quais avisos, de quais impressoras, horário silencioso). As regras ficam gravadas no
// computador (FilunaCasaAvisos.cpp, funciona sem internet) e na conta; a página filuna3d.com/casa mostra as mesmas.
var FilunaCasa = (function () {
	var E = Filuna.esc;
	var ICO = {
		impressora: '<rect x="3.5" y="3.5" width="17" height="17" rx="2.5"/><path d="M3.5 8h17"/><path d="M11 8v3h2V8"/><path d="M12 11v1.6"/><path d="M7 17.5h10"/>',
		tomada: '<circle cx="12" cy="12" r="8.5"/><circle cx="9" cy="11" r="1.1"/><circle cx="15" cy="11" r="1.1"/><circle cx="12" cy="15.2" r="1.1"/>',
		tv: '<rect x="2.5" y="4.5" width="19" height="12.5" rx="2"/><path d="M8.5 20.5h7"/><path d="M12 17v3.5"/>',
		voz: '<ellipse cx="12" cy="7.5" rx="7" ry="2.5"/><path d="M5 7.5v8c0 1.4 3.1 2.5 7 2.5s7-1.1 7-2.5v-8"/><path d="M5 12.5c0 1.4 3.1 2.5 7 2.5s7-1.1 7-2.5"/>',
		rede: '<rect x="3" y="13" width="18" height="6.5" rx="2"/><path d="M7.5 13 6 6.5"/><path d="M16.5 13 18 6.5"/><path d="M9.5 8.8a4 4 0 0 1 5 0"/><path d="M8 6.8a6.5 6.5 0 0 1 8 0"/>',
		luz: '<path d="M9 17.5h6"/><path d="M10 20.5h4"/><path d="M12 3.5a5.5 5.5 0 0 0-3.3 9.9c.5.4.8 1 .8 1.6v.5h5v-.5c0-.6.3-1.2.8-1.6A5.5 5.5 0 0 0 12 3.5Z"/>',
		central: '<path d="M3.5 11 12 4l8.5 7"/><path d="M5.5 9.5V20h13V9.5"/><circle cx="12" cy="14.5" r="2"/>',
		avisos: '<path d="M6 16V11a6 6 0 0 1 12 0v5l1.5 2h-15Z"/><path d="M10 20.5a2 2 0 0 0 4 0"/>'
	};
	var GRUPOS = [["avisos", "Avisos e serviços"], ["impressora", "Impressoras"], ["tomada", "Tomadas e energia"], ["tv", "TVs"], ["luz", "Luzes"], ["voz", "Alexa e voz"], ["central", "Central da casa"], ["rede", "Rede"]];
	var ESTADOS = { pronto: "Pronto", configurar: "Configurar", atencao: "Precisa de atenção", incompativel: "Ainda não compatível" };
	// [id, nome, cor padrão, texto padrão] — os mesmos do motor (FilunaCasaAvisos.cpp)
	var EVENTOS = [
		["concluida", "Impressão concluída", "#1F8F5F", "{impressora}: {peca} terminou. Retire a peça e limpe a mesa."],
		["erro", "Erro na impressão", "#E0261B", "{impressora} parou: {erro}"],
		["pausada", "Impressão pausada", "#F59E0B", "{impressora} pausou {peca}. Confira a impressora."],
		["filamento", "Filamento trocado", "#8B5CF6", "{impressora}: {texto}"],
		["venda", "Nova venda", "#2F6FEB", "{texto}"],
		["mensagem", "Aviso pela API", "#E6007E", "{texto}"]
	];
	var EXEMPLO = { impressora: "Centauri Carbon", peca: "Suporte de celular", erro: "o filamento acabou", texto: "Pedido #1043: Chaveiro com logo, despachar até amanhã", progresso: "100" };
	var ESTILOS = [["cartao", "Cartão no canto", "Discreto, some sozinho"], ["faixa", "Faixa no topo", "Atravessa a tela, chama atenção"], ["destaque", "Destaque no centro", "Grande; fica até você fechar"]];
	var COMANDO_DIRETO = { shelly: 1, kasa: 1, tasmota: 1, tapo: 1, tuya_nuvem: 1 };
	var LUZ_DIRETA = { kasa: 1, wled: 1, tapo: 1, hue: 1, tuya_nuvem: 1 };
	var WHATS_FILUNA = "https://wa.me/393396054650?text=" + encodeURIComponent("Oi, Filuna! Quero ligar o meu app Smart Life ao Filuna Home.");

	var st = { aparelhos: [], locais: [], procurando: false, procurou: false, iniciou: false, quando: 0, erro: "", janela: null, pedidos: {}, impressoras: [], hist: [] };

	function svg(g) { return '<svg viewBox="0 0 24 24" aria-hidden="true">' + (ICO[g] || ICO.central) + "</svg>"; }
	function protocolo(a) { return (a.dados || {}).protocolo || a.protocolo || ""; }
	function grupo(a) {
		var t = a.tipo, papel = (a.config || {}).papel;
		if (t == "este_pc" || t == "plugin") return "avisos";
		if (t == "smart_life") return papel == "luz" ? "luz" : "tomada";
		if (t == "tv_lg" || t == "tv_android" || t == "tv_samsung" || t == "tv_outra") return "tv";
		if (t == "alexa" || t == "voz_google") return "voz";
		if (t == "roteador") return "rede";
		if (t == "home_assistant") return "central";
		return t == "impressora" || t == "tomada" || t == "luz" ? t : "central";
	}
	// Quem entrega os avisos deste aparelho no motor (null = não recebe avisos)
	function motor(a) {
		var t = a.tipo, p = protocolo(a);
		if (t == "este_pc" || t == "plugin" || t == "tv_lg" || t == "home_assistant" || t == "alexa" || t == "tv_android") return t;
		if (grupo(a) == "luz") return LUZ_DIRETA[p] ? "luz" : "luz_conta";
		if (grupo(a) == "tomada") return "tomada";
		return null;
	}
	function recebeAvisos(a) { var m = motor(a); return m && m != "tomada"; }
	function guardarLocal(chave, cfg) { try { var o = JSON.parse(localStorage.getItem("fl.casa.cfg") || "{}"); o[chave] = cfg; localStorage.setItem("fl.casa.cfg", JSON.stringify(o)); } catch (e) {} }
	function lerLocal(chave) { try { return (JSON.parse(localStorage.getItem("fl.casa.cfg") || "{}") || {})[chave] || null; } catch (e) { return null; } }
	function manuais() { try { return JSON.parse(localStorage.getItem("fl.casa.manuais") || "[]") || []; } catch (e) { return []; } }
	function gravarManuais(m) { try { localStorage.setItem("fl.casa.manuais", JSON.stringify(m)); } catch (e) {} }
	function plugin(id) { return (window.FilunaCasaPlugins || []).filter(function (p) { return p.id == id; })[0]; }

	// ---------------------------------------------------------------- busca e conta

	var ESTE_PC = { chave: "pc:este", tipo: "este_pc", nome: "Este computador", marca: "Filuna", modelo: "popup na tela", ip: "", via: "Filuna Slicer · sempre disponível", estado: "configurar",
		fatos: [["Como avisa", "popup do Filuna na tela, com som"], ["Estilo", "cartão, faixa ou destaque"], ["Funciona", "sem internet, com o Filuna aberto"]] };

	function impressorasComoAparelhos() {
		return st.impressoras.map(function (u) {
			return { chave: "impressora:" + String(u.nome || "").toLowerCase(), tipo: "impressora", nome: u.nome, marca: u.tecnologia == "resina" ? "Resina" : "Filamento",
				modelo: u.modelo || "", ip: (u.viva && u.viva.ip) || "", via: "Filuna Slicer · impressoras deste computador", estado: "configurar", dados: {} };
		});
	}

	function juntarLocal(achados) {
		var por = {};
		[ESTE_PC].concat(achados).concat(manuais()).concat(impressorasComoAparelhos()).forEach(function (a) { if (!por[a.chave]) por[a.chave] = JSON.parse(JSON.stringify(a)); });
		return Object.keys(por).map(function (k) { var a = por[k]; a.id = a.id || a.chave; a.config = a.config || lerLocal(a.chave) || {}; if (a.config._pronto) a.estado = "pronto"; return a; });
	}

	function deConta() {
		return Filuna.api("GET", "/casa/dispositivos").then(function (r) {
			// a conta não guarda o diagnóstico do roteador nem o protocolo de cada um: completa com a busca deste computador
			var local = {};
			st.locais.forEach(function (a) { local[a.chave] = a; });
			var lista = (r.dispositivos || []).map(function (a) {
				var l = local[a.chave];
				if (l) { a.checagens = l.checagens; a.alerta = a.alerta || l.alerta; a.dados = Object.assign({}, a.dados || {}, l.dados || {}); a.fatos = l.fatos; a.plugin = a.plugin || l.plugin; }
				return a;
			});
			// o que só existe neste computador (Este computador, serviços ainda não enviados) continua na lista
			st.locais.forEach(function (l) { if (!lista.some(function (a) { return a.chave == l.chave; })) lista.push(l); });
			st.aparelhos = lista;
			st.daConta = true;
			return deTuya().then(sincronizarRegras);
		});
	}

	function enviarParaConta(lista) {
		if (!Filuna.temToken()) return Promise.resolve();
		var envio = lista.filter(function (a) { return a.tipo != "este_pc"; }).map(function (a) {
			return { chave: a.chave, tipo: a.tipo, nome: a.nome, marca: a.marca, modelo: a.modelo, ip: a.ip, via: a.via, estado: a.estado,
				dados: Object.assign({ fatos: a.fatos || [], alerta: a.alerta || "", checagens: a.checagens || [], plugin: a.plugin || null }, a.dados || {}) };
		});
		return envio.length ? Filuna.api("POST", "/casa/dispositivos", { dispositivos: envio }).catch(function () {}) : Promise.resolve();
	}

	// O que está pronto (aqui ou configurado no site) vira regra do motor deste computador.
	function regraDe(a) { return { tipo: motor(a), protocolo: protocolo(a), ip: a.ip || (a.config || {}).ip || "", nome: a.nome, plugin: a.plugin || (a.dados || {}).plugin || null, id_externo: (a.dados || {}).tuya_id || "" }; }

	// Smart Life ligado pela conta da Filuna (Tuya): a nuvem diz o que é cada aparelho e comanda; junta com a busca local
	function deTuya() {
		return Filuna.api("GET", "/casa/tuya/dispositivos").then(function (r) {
			(r.dispositivos || []).forEach(function (d) {
				var chaveT = "tuya:" + d.id, a = st.aparelhos.filter(function (x) { return x.chave == chaveT; })[0];
				if (!a) { a = { chave: chaveT, id: chaveT, tipo: d.tipo, nome: d.nome, marca: "Smart Life", modelo: d.modelo, ip: d.ip || "", via: "Smart Life · conta ligada à Filuna", estado: "configurar", config: lerLocal(chaveT) || {}, fatos: [] }; st.aparelhos.push(a); }
				a.tipo = d.tipo; a.nome = a.nome == "Aparelho Smart Life" ? d.nome : a.nome; a.modelo = d.modelo || a.modelo;
				a.dados = Object.assign({}, a.dados || {}, { protocolo: "tuya_nuvem", tuya_id: d.id });
				a.fatos = [["Conta", "Smart Life ligada à Filuna"], ["Situação", d.online ? "conectado" : "desconectado"], ["Mede consumo", d.mede_energia ? "sim" : "não"]].concat(d.tipo == "luz" ? [["Cores", d.colorida ? "sim" : "não"]] : []);
				if ((a.config || {})._pronto) a.estado = "pronto";
			});
		}).catch(function () {});
	}
	function sincronizarRegras() {
		st.aparelhos.forEach(function (a) {
			if (!motor(a) || !(a.config || {})._pronto) return;
			Filuna.nativo("casa.regra.salvar", { chave: a.chave, aparelho: regraDe(a), cfg: cfgPadrao(a) }).catch(function () {});
		});
	}

	function procurar() {
		if (st.procurando) return;
		st.procurando = true; st.erro = ""; redesenhar();
		Filuna.nativo("casa.procurar").then(function () {
			var espera = function () {
				Filuna.nativo("casa.resultado").then(function (r) {
					if (r.rodando) { setTimeout(espera, 700); return; }
					st.procurando = false; st.procurou = true; st.quando = Date.now(); st.rede = r.rede || {};
					st.locais = juntarLocal(r.aparelhos || []);
					st.aparelhos = st.locais;
					enviarParaConta(st.locais).then(function () { return Filuna.temToken() ? deConta() : null; }).catch(function (e) { st.erro = (e && e.erro) || ""; }).then(redesenhar);
				}, function (e) { st.procurando = false; st.erro = (e && e.erro) || "A busca falhou."; redesenhar(); });
			};
			espera();
		}, function (e) { st.procurando = false; st.erro = (e && e.erro) || "A busca falhou."; redesenhar(); });
	}

	function lerHistorico() { return Filuna.nativo("casa.historico").then(function (r) { st.hist = r.itens || []; }).catch(function () {}); }

	// ---------------------------------------------------------------- tela

	var alvo = null, cabecalho = "", aoLigar = null;

	function desenhar(app, barra, ligarBarra, impressoras) {
		alvo = app; cabecalho = barra; aoLigar = ligarBarra; st.impressoras = impressoras || [];
		if (st.janela) return; // a leitura de 15 s não fecha a janela aberta
		if (!st.iniciou) { // primeira vez: usa a última busca deste computador; sem nenhuma, procura agora
			st.iniciou = true;
			st.locais = juntarLocal([]);
			st.aparelhos = st.locais;
			lerHistorico().then(redesenhar);
			Filuna.nativo("casa.resultado").then(function (r) {
				if (!(r.aparelhos || []).length) { procurar(); return; }
				st.procurou = true; st.quando = (r.quando || 0) * 1000; st.rede = r.rede || {};
				st.locais = juntarLocal(r.aparelhos); st.aparelhos = st.locais;
				(Filuna.temToken() ? deConta() : Promise.resolve(sincronizarRegras())).catch(function () {}).then(redesenhar);
			}, function () { procurar(); });
		} else if (!st.procurando) { // impressoras novas desta tela
			st.locais = juntarLocal(st.locais.filter(function (a) { return a.tipo != "impressora"; }));
			if (!st.daConta) st.aparelhos = st.locais;
		}
		redesenhar();
	}

	function quando(iso) { try { var d = new Date(iso); return d.toLocaleDateString("pt-BR", { day: "2-digit", month: "2-digit" }) + " " + d.toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" }); } catch (e) { return iso || ""; } }

	function redesenhar() {
		if (!alvo || st.janela) return;
		var lista = st.aparelhos, cont = {};
		lista.forEach(function (a) { cont[a.estado] = (cont[a.estado] || 0) + 1; });
		var prontos = lista.filter(function (a) { return a.estado == "pronto" && recebeAvisos(a); }).length;
		var h = cabecalho + '<div class="fh"><div class="fh-cab"><div><div class="h1">Filuna Home</div>' +
			'<p>O Filuna encontra os aparelhos da sua casa e ensina a ligar cada um. Depois de um teste, você escolhe o texto, a cor e o estilo de cada aviso: impressão concluída, erro, pausa e nova venda chegam na TV, nas luzes, no celular e na tela do computador.</p></div>' +
			'<div class="fh-busca' + (st.procurando ? " girando" : "") + '"><span class="fh-radar"></span><div><b>' +
			(st.procurando ? "Procurando na sua rede…" : lista.length + (lista.length == 1 ? " aparelho" : " aparelhos")) + "</b><small>" +
			(st.procurando ? "TVs, tomadas, luzes, roteador e central" : st.quando ? "Busca deste computador · " + new Date(st.quando).toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" }) : "Ainda não procurou") +
			'</small></div><button class="btn" id="fh-procurar"' + (st.procurando ? " disabled" : "") + ">Procurar de novo</button></div></div>";
		if (!Filuna.temToken()) h += '<div class="fh-aviso-top">Entre na sua conta Filuna para a configuração valer também no site e no app. Sem conta, ela fica só neste computador.</div>';
		if (st.erro) h += '<div class="fh-aviso-top">' + E(st.erro) + "</div>";
		h += '<div class="fh-linha-resumo"><ul class="fh-resumo">' + Object.keys(ESTADOS).filter(function (k) { return cont[k]; }).map(function (k) { return '<li data-estado="' + k + '"><b>' + cont[k] + "</b> " + ESTADOS[k].toLowerCase() + "</li>"; }).join("") + "</ul>" +
			(prontos ? '<button class="btn" id="fh-geral">Mandar aviso de teste para os ' + prontos + " prontos</button>" : "") + "</div>";
		GRUPOS.forEach(function (g) {
			var itens = lista.filter(function (a) { return grupo(a) == g[0]; });
			if (!itens.length) return;
			h += '<section class="fh-grupo"><h2>' + svg(g[0]) + E(g[1]) + " <span>" + itens.length + '</span></h2><div class="fh-grade">' + itens.map(function (a) {
				return '<button class="fh-ap" data-id="' + E(a.id) + '" data-estado="' + E(a.estado) + '"><span class="fh-ico">' + svg(g[0]) + '</span><span class="fh-ap-txt"><b>' + E(a.nome) +
					"</b><small>" + E(rotulo(a)) + '</small><span class="fh-estado">' + E(ESTADOS[a.estado] || a.estado) + "</span></span></button>";
			}).join("") + "</div></section>";
		});
		if (lista.length <= 1 && !st.procurando) h += '<div class="fh-vazio">Nenhum aparelho respondeu. Confira se o computador está na mesma rede da casa (não na rede de visitantes) e toque em Procurar de novo.</div>';
		h += '<div class="fh-mais"><button class="btn primario" id="fh-servico">+ Adicionar serviço (celular, Telegram, WhatsApp…)</button><button class="btn" id="fh-wled">+ Luz WLED pelo endereço</button>' +
			'<button class="btn" id="fh-tasmota">+ Tomada Tasmota pelo endereço</button><button class="btn" id="fh-alexa">+ Alexa</button><button class="btn" id="fh-ha">+ Home Assistant pelo endereço</button></div>';
		h += '<section class="fh-grupo"><h2>' + svg("avisos") + 'Últimos avisos <span>' + st.hist.length + "</span></h2>" + (st.hist.length ? '<ul class="fh-hist">' + st.hist.slice(0, 12).map(function (x) {
			return '<li><i style="background:' + E(x.cor || "#999") + '"></i><div><b>' + E(x.titulo || x.evento) + "</b> · " + E(x.aparelho || "") + (x.teste ? " · teste" : "") + "<small>" + E(x.texto || "") + "</small></div>" +
				'<span class="' + (x.ok ? "ok" : "erro") + '" title="' + E(x.msg || "") + '">' + (x.ok ? "entregue" : "não entregue") + "</span><time>" + E(quando(x.quando)) + "</time></li>";
		}).join("") + "</ul>" : '<p class="fh-nota">Quando uma impressora terminar, der erro ou chegar uma venda, os avisos entregues aparecem aqui.</p>') + "</section>";
		h += '<section class="fh-api"><h2>API e plugins</h2><p>O seu sistema também pode mandar avisos para a casa: um POST em <code>https://filuna3d.com/api/casa/v1/avisos</code> com a sua chave chega em todos os aparelhos prontos, do mesmo jeito que um aviso de impressão. A chave é criada na sua conta, em filuna3d.com/casa. No caminho contrário, o plugin Webhook manda cada aviso do Filuna para o endereço que você escolher.</p>' +
			'<button class="btn" id="fh-api">Criar chave da API no site</button></section>';
		h += '<div class="nota">A Alexa não aparece na busca: ela fala com o Filuna pela conta da Amazon, não pela rede da casa.</div></div>';
		alvo.innerHTML = h;
		if (aoLigar) aoLigar(alvo);
		alvo.querySelector("#fh-procurar").onclick = procurar;
		var geral = alvo.querySelector("#fh-geral");
		if (geral) geral.onclick = function () {
			geral.disabled = true; geral.textContent = "Mandando…";
			Filuna.nativo("casa.evento", { evento: "mensagem", dados: { titulo: "Teste do Filuna Home", texto: "Teste geral: todos os aparelhos prontos receberam este aviso." } }).then(function () { setTimeout(function () { lerHistorico().then(redesenhar); }, 4000); });
		};
		alvo.querySelector("#fh-servico").onclick = escolherServico;
		alvo.querySelector("#fh-wled").onclick = function () { adicionarManual({ chave: "wled:" + Date.now().toString(36), tipo: "luz", nome: "Luz WLED", marca: "WLED", modelo: "fita de LED", ip: "", via: "Filuna Slicer · adicionada à mão", estado: "configurar", dados: { protocolo: "wled" }, fatos: [["Como avisa", "pisca na cor do aviso"], ["Precisa", "WLED na mesma rede"]] }); };
		alvo.querySelector("#fh-tasmota").onclick = function () { adicionarManual({ chave: "tasmota:" + Date.now().toString(36), tipo: "tomada", nome: "Tomada Tasmota", marca: "Tasmota", modelo: "tomada", ip: "", via: "Filuna Slicer · adicionada à mão", estado: "configurar", dados: { protocolo: "tasmota" }, fatos: [["Comando", "direto na rede da casa"], ["Conta", "não precisa"]] }); };
		alvo.querySelector("#fh-alexa").onclick = function () { adicionarManual({ chave: "alexa:conta", tipo: "alexa", nome: "Alexa", marca: "Amazon", modelo: "Echo", ip: "", via: "Filuna Slicer · adicionada à mão", estado: "configurar", fatos: [["Como avisa", "luz amarela; com tela, um banner"], ["Fala o aviso", "quando você pede"], ["Precisa", "skill Filuna ativada"]] }); };
		alvo.querySelector("#fh-ha").onclick = function () { adicionarManual({ chave: "ha:manual", tipo: "home_assistant", nome: "Home Assistant", marca: "Home Assistant", modelo: "servidor da casa", ip: "", via: "Filuna Slicer · adicionado à mão", estado: "configurar", fatos: [["O que o Filuna faz", "publica as impressoras e os avisos lá"], ["Protocolo", "MQTT, na rede da casa"]] }); };
		alvo.querySelector("#fh-api").onclick = function () { Filuna.nativo("url.abrir", { url: "https://filuna3d.com/casa#api" }).catch(function () {}); };
		alvo.querySelectorAll(".fh-ap").forEach(function (b) { b.onclick = function () { abrir(b.dataset.id); }; });
	}

	function adicionarManual(a) {
		var m = manuais().filter(function (x) { return x.chave != a.chave; }); m.push(a);
		gravarManuais(m);
		st.locais = juntarLocal(st.locais.filter(function (x) { return x.chave != a.chave; }));
		if (!st.daConta) st.aparelhos = st.locais; else if (!st.aparelhos.some(function (x) { return x.chave == a.chave; })) st.aparelhos.push(juntarLocal([a]).filter(function (x) { return x.chave == a.chave; })[0]);
		enviarParaConta([a]).then(function () { return Filuna.temToken() ? deConta() : null; }).catch(function () {}).then(function () {
			redesenhar();
			var novo = st.aparelhos.filter(function (x) { return x.chave == a.chave; })[0];
			if (novo) abrir(novo.id);
		});
	}

	function escolherServico() {
		var f = fundo();
		f.innerHTML = '<div class="fh-j"><div class="fh-j-cab"><span class="fh-ico">' + svg("avisos") + '</span><div><h3>Adicionar serviço</h3><p>Avisos fora da rede da casa: no celular, em mensagens e no seu sistema</p></div><button class="fh-fechar" id="fh-x" aria-label="Fechar">×</button></div>' +
			'<div class="fh-j-corpo"><div class="fh-catalogo">' + (window.FilunaCasaPlugins || []).map(function (p) {
				return '<button class="fh-plug" data-p="' + E(p.id) + '"><b>' + E(p.nome) + "</b><small>" + E(p.resumo) + '</small><span class="fh-estado">' + E(p.categoria) + "</span></button>";
			}).join("") + "</div></div></div>";
		f.querySelector("#fh-x").onclick = function () { f.remove(); };
		f.querySelectorAll("[data-p]").forEach(function (b) {
			b.onclick = function () {
				var p = plugin(b.dataset.p);
				f.remove();
				adicionarManual({ chave: "plugin:" + p.id + ":" + Date.now().toString(36), tipo: "plugin", nome: p.nome, marca: p.categoria, modelo: p.resumo, ip: "", via: "Filuna Slicer · serviço", estado: "configurar",
					plugin: { id: p.id, avisar: p.avisar }, dados: { plugin_id: p.id }, fatos: [["Serviço", p.nome], ["Tipo", p.categoria], ["Funciona", "pela internet"]] });
			};
		});
	}

	// "Apollo Premium4K TV · Apollo Premium4K TV": a marca já vem no modelo
	function rotulo(a, semIp) {
		var m = a.marca || "", mo = a.modelo || "";
		var partes = mo && m && mo.toLowerCase().indexOf(m.toLowerCase()) == 0 ? [mo] : [m, mo];
		return partes.filter(Boolean).concat(a.ip && !semIp ? [a.ip] : []).join(" · ");
	}

	// ---------------------------------------------------------------- janela de configuração

	function passos(a) {
		var p = protocolo(a), m = motor(a), semIp = !a.ip && !(a.config || {}).ip;
		switch (a.tipo) {
			case "este_pc": return ["Este computador", "Testar", "Personalizar", "Usar"];
			case "plugin": return ["Instalar", "Conectar", "Testar", "Personalizar", "Usar"];
			case "impressora": return ["Reconhecida", "Tomada"];
			case "tv_lg": return ["Reconhecida", "Parear", "Testar", "Personalizar", "Usar"];
			case "tv_android": return ["Reconhecida", "Instalar", "Testar", "Personalizar", "Usar"];
			case "alexa": return ["Reconhecida", "Ativar skill", "Testar", "Usar"];
			case "roteador": return ["Reconhecido", "Diagnóstico"];
			case "home_assistant": return ["Reconhecido", "Conectar", "Publicar", "Testar", "Usar"];
			case "smart_life": return ["Reconhecido", "O que é", "Conectar"].concat((a.config || {}).papel == "luz" ? ["Personalizar", "Usar"] : ["Impressora"]);
		}
		if (m == "tomada") return COMANDO_DIRETO[p] ? ["Reconhecida"].concat(semIp && p != "tuya_nuvem" ? ["Endereço"] : []).concat(["Conectar", "Impressora", "Testar"]) : ["Reconhecida", "Conectar", "Impressora"];
		if (m == "luz") return ["Reconhecida"].concat(semIp && p != "tuya_nuvem" ? ["Endereço"] : []).concat(p == "hue" ? ["Parear"] : p == "tapo" || p == "tuya_nuvem" ? ["Conectar"] : []).concat(["Testar", "Personalizar", "Usar"]);
		if (m == "luz_conta") return ["Reconhecida", "Conectar", "Personalizar", "Usar"];
		return ["Reconhecida"];
	}

	function cfgPadrao(a) {
		var cores = {}; EVENTOS.forEach(function (e) { cores[e[0]] = e[2]; });
		var c = { avisos: { concluida: true, erro: true, pausada: true, filamento: false, venda: grupo(a) != "luz", mensagem: true }, impressoras: [],
			silencio: { ligado: false, de: "22:00", ate: "07:00", erro_acorda: true }, textos: {},
			estilo: { modelo: "cartao", tema: "claro", duracao_s: motor(a) == "luz" || motor(a) == "luz_conta" ? 8 : 10, som: true, cores: cores }, campos: {} };
		if (grupo(a) == "tomada") Object.assign(c, { impressora: "", desligarFrio: true, limite_c: 50, remoto: true, corte: false });
		if (a.tipo == "home_assistant") Object.assign(c, { broker: (a.ip ? a.ip : "") + ":1883", usuario: "", publicar: { estado: true } });
		if (a.tipo == "plugin") { var p = plugin((a.plugin || {}).id || (a.dados || {}).plugin_id); (p ? p.campos : []).forEach(function (f) { if (f.padrao) c.campos[f.id] = f.padrao; }); }
		var salvo = JSON.parse(JSON.stringify(a.config || {}));
		if (typeof salvo.silencio == "boolean") salvo.silencio = { ligado: salvo.silencio, de: "22:00", ate: "07:00", erro_acorda: true }; // versão anterior
		if (salvo.cores) { c.estilo.cores = Object.assign(c.estilo.cores, salvo.cores); delete salvo.cores; }
		["avisos", "estilo", "campos", "textos", "silencio"].forEach(function (k) { if (salvo[k]) { salvo[k] = Object.assign({}, c[k], salvo[k]); if (k == "estilo") salvo.estilo.cores = Object.assign({}, c.estilo.cores, (salvo.estilo || {}).cores || {}); } });
		return Object.assign(c, salvo);
	}

	function fundo() {
		var f = document.getElementById("fh-fundo");
		if (!f) { f = document.createElement("div"); f.id = "fh-fundo"; f.className = "fh-fundo"; document.body.appendChild(f); f.onclick = function (e) { if (e.target == f) fechar(); }; }
		return f;
	}

	function abrir(id) {
		var a = st.aparelhos.filter(function (x) { return x.id == id; })[0];
		if (!a) return;
		st.janela = { a: a, passo: 0, cfg: cfgPadrao(a), feito: { teste: !!(a.config || {})._pronto }, evPrevia: "concluida" };
		var verPar = a.tipo == "tv_lg" ? "lg.pareada" : protocolo(a) == "hue" ? "hue.pareada" : a.tipo == "tv_android" ? "filunatv.pareada" : "";
		if (verPar && a.ip) Filuna.nativo("casa.acao", { acao: verPar, ip: a.ip }).then(function (r) { if (st.janela && r.pareada) { st.janela.feito.parear = true; desenharJanela(); } }).catch(function () {});
		if (protocolo(a) == "tapo") Filuna.nativo("casa.acao", { acao: "tapo.tem_conta" }).then(function (r) { st.tapoConta = !!r.conta; if (st.janela) desenharJanela(); }).catch(function () {});
		desenharJanela();
	}

	function fechar() { var f = document.getElementById("fh-fundo"); if (f) f.remove(); st.janela = null; redesenhar(); }

	function desenharJanela() {
		var j = st.janela, a = j.a, ps = passos(a), nome = ps[j.passo];
		var ultimo = j.passo == ps.length - 1;
		var html = '<div class="fh-j' + (nome == "Personalizar" ? " largo" : "") + '"><div class="fh-j-cab"><span class="fh-ico">' + svg(grupo(a)) + '</span><div><h3>' + E(a.nome) + "</h3><p>" + E(rotulo(a, true)) +
			'</p></div><button class="fh-fechar" id="fh-x" aria-label="Fechar">×</button></div>' +
			(ps.length > 1 ? '<ol class="fh-passos">' + ps.map(function (p, i) { return '<li class="' + (i == j.passo ? "on" : i < j.passo ? "feito" : "") + '" data-ir="' + i + '"><b>' + (i + 1) + "</b>" + E(p) + "</li>"; }).join("") + "</ol>" : "") +
			'<div class="fh-j-corpo">' + conteudo(a, nome, j.cfg) + '</div><div class="fh-j-rod">' +
			(a.chave.indexOf("plugin:") == 0 || a.chave.indexOf("wled:") == 0 || a.chave.indexOf("tasmota:") == 0 ? '<button class="btn" id="fh-remover">Remover</button>' : "") +
			'<button class="btn" id="fh-voltar"' + (j.passo ? "" : ' style="visibility:hidden"') + '>Voltar</button>' +
			'<button class="btn primario" id="fh-avancar">' + (ps.length == 1 ? "Entendi" : ultimo ? "Concluir" : "Continuar") + "</button></div></div>";
		var f = fundo();
		f.innerHTML = html;
		f.querySelector("#fh-x").onclick = fechar;
		f.querySelector("#fh-voltar").onclick = function () { if (j.passo) { j.passo--; desenharJanela(); } };
		f.querySelector("#fh-avancar").onclick = function () { if (ps.length == 1) fechar(); else if (!ultimo) { j.passo++; desenharJanela(); } else concluir(); };
		f.querySelectorAll("[data-ir]").forEach(function (li) { li.onclick = function () { j.passo = +li.dataset.ir; desenharJanela(); }; });
		var rem = f.querySelector("#fh-remover");
		if (rem) rem.onclick = function () {
			if (!confirm("Remover " + a.nome + " do Filuna Home?")) return;
			gravarManuais(manuais().filter(function (x) { return x.chave != a.chave; }));
			Filuna.nativo("casa.regra.salvar", { chave: a.chave, remover: true }).catch(function () {});
			st.locais = st.locais.filter(function (x) { return x.chave != a.chave; });
			st.aparelhos = st.aparelhos.filter(function (x) { return x.chave != a.chave; });
			fechar();
		};
		ligarCampos(f, a, j.cfg);
		atualizarPrevia();
	}

	// a conta pode ter guardado os fatos como objeto {rótulo: valor} (versão anterior do Slicer)
	function paresDe(f) { return Array.isArray(f) ? f : f && typeof f == "object" ? Object.keys(f).map(function (k) { return [k, f[k]]; }) : []; }
	function fatos(a) {
		a.fatos = paresDe(a.fatos);
		return '<dl class="fh-fatos">' + a.fatos.map(function (x) { return "<div><dt>" + E(x[0]) + "</dt><dd>" + E(x[1]) + "</dd></div>"; }).join("") +
			(a.ip && !a.fatos.some(function (f) { return f[1] == a.ip; }) ? "<div><dt>Endereço na rede</dt><dd>" + E(a.ip) + "</dd></div>" : "") + "<div><dt>Encontrado por</dt><dd>" + E(a.via || "") + "</dd></div></dl>";
	}
	function chave(nome, rot, marcado, dica) {
		return '<label class="fh-chave"><span><b>' + E(rot) + "</b>" + (dica ? "<small>" + E(dica) + "</small>" : "") + '</span><input type="checkbox" data-cfg="' + nome + '"' + (marcado ? " checked" : "") + "></label>";
	}
	function botao(acao, rot) { return '<button class="btn primario" data-acao="' + acao + '">' + E(rot) + '</button><p class="fh-res" id="fh-res"></p>'; }
	function breve(txt) { return '<div class="fh-breve">' + E(txt) + "</div>"; }
	function opcoesImpressoras(cfg) {
		return st.aparelhos.filter(function (x) { return x.tipo == "impressora"; }).map(function (p) { return '<option value="' + E(p.chave) + '"' + (cfg.impressora == p.chave ? " selected" : "") + ">" + E(p.nome) + "</option>"; }).join("");
	}
	function texto(ev, cfg) { var e = EVENTOS.filter(function (x) { return x[0] == ev; })[0] || EVENTOS[0]; return (cfg.textos || {})[ev] || e[3]; }
	function preencher(t, extra) { var v = Object.assign({}, EXEMPLO, extra || {}); return t.replace(/\{(\w+)\}/g, function (m, k) { return v[k] != null ? v[k] : m; }); }

	// O que o Filuna ensina para cada aparelho antes do teste
	function guia(a) {
		var p = protocolo(a), m = motor(a);
		if (m == "luz" && p == "kasa") return ["A lâmpada já está no Wi-Fi da casa (feito pelo app Kasa).", "O Filuna fala com ela direto pela rede: não precisa de conta.", "No teste, ela pisca 3 vezes na cor do aviso e volta como estava."];
		if (m == "luz" && p == "wled") return ["A fita ou luminária com WLED precisa estar no Wi-Fi da casa.", "No app WLED (ou no navegador), veja o endereço dela, como 192.168.0.50.", "No teste, ela pisca na cor do aviso e volta ao efeito que estava."];
		if (m == "tomada" && p == "tasmota") return ["A tomada com Tasmota precisa estar no Wi-Fi da casa.", "Na página da tomada, confira o endereço dela (Information › IP Address).", "O Filuna liga e desliga pela rede, sem conta."];
		return [];
	}

	function conteudo(a, nome, cfg) {
		var p = protocolo(a), m = motor(a);
		if (nome == "Este computador") {
			return '<p class="fh-p">O Filuna mostra os avisos na tela deste computador, por cima dos outros programas, com som. Funciona sem internet, enquanto o Filuna estiver aberto.</p>' + fatos(a) +
				'<p class="fh-nota">Você escolhe o estilo do popup (cartão, faixa ou destaque), a cor de cada aviso e o texto no passo Personalizar.</p>';
		}
		if (/^Reconhecid/.test(nome)) {
			var intro = { pronto: "Este aparelho já está ligado ao Filuna.", configurar: "O Filuna encontrou este aparelho na sua rede. Faltam poucos passos.",
				atencao: "O Filuna encontrou este aparelho, mas há um ponto para ajustar.", incompativel: "O Filuna encontrou este aparelho, mas ele ainda não aceita avisos." }[a.estado] || "";
			if (a.tipo == "impressora") intro = "Esta impressora está cadastrada neste computador. Ligue a tomada inteligente dela para desligar sozinha quando o bico esfriar.";
			if (a.via && a.via.indexOf("à mão") >= 0) intro = "Você adicionou este aparelho à mão. Siga os passos para ligar ao Filuna.";
			var extra = "";
			if (a.tipo == "tv_samsung") extra = '<p class="fh-p">A Samsung só mostra na tela avisos de eletrodomésticos da própria marca. Para receber os avisos nesta TV, ligue um Fire TV Stick ou um aparelho com Google TV no HDMI e instale o app Filuna TV.</p>';
			if (a.tipo == "voz_google") extra = '<p class="fh-p">O Google ainda não deixa outros apps mandarem avisos para o Nest. Para avisos por voz, use um Amazon Echo.</p>';
			var g = guia(a);
			return '<p class="fh-p">' + E(intro) + "</p>" + (a.alerta ? '<div class="fh-alerta">' + E(a.alerta) + "</div>" : "") + fatos(a) + extra +
				(g.length ? '<h4 class="fh-sub">Como o Filuna liga este aparelho</h4><ol class="fh-lista">' + g.map(function (x) { return "<li>" + E(x) + "</li>"; }).join("") + "</ol>" : "");
		}
		if (nome == "Endereço") {
			return '<p class="fh-p">' + E(m == "luz" ? "Qual é o endereço da luz na sua rede?" : "Qual é o endereço da tomada na sua rede?") + '</p><ol class="fh-lista">' + guia(a).map(function (x) { return "<li>" + E(x) + "</li>"; }).join("") + "</ol>" +
				'<label class="fh-campo">Endereço na rede<input data-cfg="ip" value="' + E(cfg.ip || a.ip || "") + '" placeholder="192.168.0.50"></label>';
		}
		if (nome == "Instalar" && a.tipo == "plugin") {
			var pl = plugin((a.plugin || {}).id || (a.dados || {}).plugin_id) || { ajuda: [] };
			return '<p class="fh-p">' + E(pl.resumo || "") + '</p><h4 class="fh-sub">Faça isto uma vez</h4><ol class="fh-lista">' + pl.ajuda.map(function (x) { return "<li>" + E(x) + "</li>"; }).join("") + "</ol>";
		}
		if (nome == "Conectar" && a.tipo == "plugin") {
			var pc = plugin((a.plugin || {}).id || (a.dados || {}).plugin_id) || { campos: [] };
			return pc.campos.map(function (f) {
				return '<label class="fh-campo">' + E(f.rotulo) + '<input data-cfg="campos.' + E(f.id) + '" value="' + E((cfg.campos || {})[f.id] || "") + '"' + (f.segredo ? ' type="password" autocomplete="off"' : "") + (f.dica ? ' placeholder="' + E(f.dica) + '"' : "") + "></label>";
			}).join("") + '<p class="fh-nota">Fica gravado neste computador e na sua conta Filuna. O Filuna usa só para mandar os avisos que você escolher.</p>';
		}
		if (nome == "Tomada") {
			var tomadas = st.aparelhos.filter(function (x) { return grupo(x) == "tomada"; });
			return '<p class="fh-p">Ligue a impressora a uma tomada inteligente para desligar e ligar pelo celular e acompanhar o consumo.</p>' +
				'<label class="fh-campo">Tomada desta impressora<select data-cfg="tomada"><option value="">Nenhuma</option>' +
				tomadas.map(function (t) { return '<option value="' + E(t.chave) + '"' + (cfg.tomada == t.chave ? " selected" : "") + ">" + E(t.nome) + " · " + E(t.modelo || t.marca) + "</option>"; }).join("") + "</select></label>" +
				'<p class="fh-nota">As regras de ligar e desligar ficam na própria tomada, em Tomadas e energia.</p>';
		}
		if (nome == "O que é") {
			return '<p class="fh-p">A Smart Life não diz pela rede se é tomada ou luz. O que é este aparelho?</p><div class="fh-papel">' +
				'<button class="btn' + (cfg.papel != "luz" ? " primario" : "") + '" data-papel="tomada">Tomada</button><button class="btn' + (cfg.papel == "luz" ? " primario" : "") + '" data-papel="luz">Lâmpada ou luz</button></div>';
		}
		if (nome == "Conectar" && p == "tapo")
			return '<p class="fh-p">Tapo de firmware novo só aceita comando com a conta do app Tapo. Digite o e-mail e a senha da sua conta Tapo: ficam guardados só neste computador e valem para todos os seus Tapo.</p>' +
				'<label class="fh-campo">E-mail da conta Tapo<input id="fh-tapo-email" autocomplete="off" value="' + E(cfg.tapo_email || "") + '"></label>' +
				'<label class="fh-campo">Senha da conta Tapo<input id="fh-tapo-senha" type="password" autocomplete="off" placeholder="' + (st.tapoConta ? "já guardada neste computador" : "") + '"></label>' + botao("tapo.conta", "Entrar na conta Tapo");
		if (nome == "Conectar" && p == "tuya_nuvem")
			return '<p class="fh-p">Este aparelho está ligado à sua conta Smart Life pela Filuna. O Filuna comanda pela nuvem da Smart Life: funciona mesmo de fora de casa.</p>' + botao("tomada.estado", "Conferir a conexão");
		if (nome == "Conectar" && a.tipo == "smart_life")
			return '<p class="fh-p">Aparelhos Smart Life (Positivo, Tuya e outros) são comandados pela conta do app Smart Life. A Filuna liga a sua conta em 1 minuto:</p>' +
				'<ol class="fh-lista"><li>Chame a Filuna no WhatsApp pelo botão abaixo: mandamos um QR.</li><li>No app Smart Life, toque em <b>Eu</b> e no leitor de QR no topo, e escaneie.</li><li>Volte aqui e toque em Procurar de novo: os seus aparelhos aparecem prontos para configurar.</li></ol>' +
				'<button class="btn primario" id="fh-whats">Chamar a Filuna no WhatsApp</button>';
		if (nome == "Conectar" && m == "tomada" && COMANDO_DIRETO[p])
			return '<p class="fh-p">Esta tomada aceita comando direto na rede da casa. Não precisa de conta.</p>' + botao("tomada.estado", "Conectar pela rede");
		if (nome == "Conectar" && (grupo(a) == "tomada" || grupo(a) == "luz"))
			return '<p class="fh-p">Este aparelho é comandado pelo app do fabricante, que ainda não abriu o comando para outros programas. Use uma tomada ou luz da lista de compatíveis: Shelly, Tapo, Kasa, Tasmota, Smart Life, Philips Hue ou WLED.</p>';
		if (nome == "Impressora") {
			return '<label class="fh-campo">Impressora ligada nesta tomada<select data-cfg="impressora"><option value="">Escolha</option>' + opcoesImpressoras(cfg) + "</select></label>" +
				chave("desligarFrio", "Desligar quando a impressão terminar", cfg.desligarFrio, "Só depois que o bico esfriar abaixo do limite") +
				'<label class="fh-campo">Limite do bico para desligar (°C)<input type="number" min="30" max="120" data-cfg="limite_c" value="' + E(cfg.limite_c || 50) + '"></label>' +
				chave("remoto", "Ligar e desligar pelo celular", cfg.remoto, "Ligar não começa impressão: alguém confere a mesa antes") +
				chave("corte", "Cortar a energia em falha grave", cfg.corte, "Desligado por padrão. Avisa antes e dá 2 minutos para cancelar");
		}
		if (nome == "Parear" && p == "hue")
			return '<p class="fh-p">Aperte o botão redondo no meio da ponte Hue e, em até 30 segundos, toque em Parear. É só uma vez; o Filuna passa a piscar todas as luzes da ponte na cor do aviso.</p>' +
				(st.janela.feito.parear ? '<p class="fh-res ok">Esta ponte já está pareada com este computador.</p>' : "") + botao("hue.parear", "Parear");
		if (nome == "Parear")
			return '<p class="fh-p">Ligue a TV. Quando tocar em Parear, aparece um pedido na tela da TV: aceite com o controle. É só uma vez.</p>' +
				'<p class="fh-nota">Se o pedido não aparecer: na TV, abra Configurações › Rede e ligue <b>LG Connect Apps</b>.</p>' +
				(st.janela.feito.parear ? '<p class="fh-res ok">Esta TV já está pareada com este computador.</p>' : "") + botao("lg.parear", st.janela.feito.parear ? "Parear de novo" : "Parear agora");
		if (nome == "Instalar")
			return '<ol class="fh-lista"><li>Na TV, instale o app <b>Filuna TV</b>: procure na loja de apps da TV ou, no Fire TV e no Google TV, instale o app <b>Downloader</b> e digite <b>filuna3d.com/casa/filuna-tv.apk</b>.</li>' +
				'<li>Abra o Filuna TV: ele mostra um código de 6 dígitos e pede para aparecer sobre os outros apps (permita).</li><li>Digite o código aqui e toque em Parear.</li></ol>' +
				'<label class="fh-campo">Código que aparece na TV<input id="fh-codigo-tv" inputmode="numeric" maxlength="6" placeholder="000000" value="' + E(cfg.codigo || "") + '"></label>' +
				(st.janela.feito.parear ? '<p class="fh-res ok">Esta TV já está pareada com este computador.</p>' : "") + botao("filunatv.parear", "Parear");
		if (nome == "Ativar skill")
			return '<ol class="fh-lista"><li>No app Alexa, toque em Mais › Skills e jogos e procure <b>Filuna</b>.</li><li>Toque em Ativar e entre com a sua conta Filuna (a mesma deste computador).</li><li>Permita as notificações quando a Alexa pedir.</li></ol>' +
				'<p class="fh-nota">O Echo acende a luz amarela quando chega um aviso; os que têm tela mostram um banner. Para ouvir, diga "Alexa, quais são as minhas notificações?". Para perguntar: "Alexa, pergunte ao Filuna como estão as impressoras".</p>';
		if (nome == "Diagnóstico") {
			var ic = { ok: "Certo", aviso: "Atenção", ruim: "Ajustar" };
			return (a.alerta ? '<div class="fh-alerta">' + E(a.alerta) + "</div>" : "") + '<ul class="fh-checa" id="fh-checa">' + checagensHtml(a.checagens || [], ic) + "</ul>" + botao("rede.diagnostico", "Verificar de novo");
		}
		if (nome == "Conectar" && a.tipo == "home_assistant")
			return '<p class="fh-p">O Filuna publica as impressoras e os avisos no Home Assistant pelo MQTT da sua casa. Elas aparecem lá sozinhas, e cada aviso vira um gatilho "Filuna" nas automações.</p>' +
				'<label class="fh-campo">Endereço do MQTT<input data-cfg="broker" value="' + E(cfg.broker || "") + '" placeholder="192.168.0.10:1883"></label>' +
				'<label class="fh-campo">Usuário do MQTT<input data-cfg="usuario" value="' + E(cfg.usuario || "") + '" autocomplete="off"></label>' +
				'<label class="fh-campo">Senha do MQTT<input type="password" id="fh-senha" autocomplete="off" placeholder="fica só neste computador"></label>' + botao("mqtt.testar", "Testar conexão");
		if (nome == "Publicar")
			return chave("publicar.estado", "Estado e progresso das impressoras", (cfg.publicar || {}).estado, "Imprimindo, livre, erro, tempo restante, bico e mesa") +
				'<p class="fh-nota">Ao concluir, o Filuna publica as impressoras a cada 30 segundos enquanto este computador estiver ligado. No Home Assistant, ligue a integração MQTT: elas aparecem em Dispositivos.</p>';
		if (nome == "Testar") return testarHtml(a, m);
		if (nome == "Personalizar") return personalizarHtml(a, m, cfg);
		if (nome == "Usar") return usarHtml(a, m, cfg);
		return "";
	}

	function testarHtml(a, m) {
		var j = st.janela;
		if (m == "tomada") return '<p class="fh-p">Teste a tomada: ela desliga e liga de novo em 3 segundos. Faça com a impressora parada.</p>' + botao("tomada.testar", "Desligar e ligar");
		var onde = { este_pc: "na tela deste computador", tv_lg: "na tela da TV", luz: "na luz (ela pisca na cor do aviso)", home_assistant: "no Home Assistant (Ferramentas › Eventos MQTT, tópico filuna/evento)", plugin: "no " + a.nome, tv_android: "na tela da TV", alexa: "no seu Echo (a luz amarela acende)" }[m] || "no aparelho";
		var h = '<p class="fh-p">O Filuna manda um aviso de teste ' + E(onde) + '. Confira se chegou.</p>' + botao("teste", "Mandar aviso de teste");
		if (j.testeOk) h += '<div class="fh-confirma"><b>Apareceu?</b><button class="btn primario" data-ok="1">Sim, apareceu</button><button class="btn" data-ok="0">Não apareceu</button></div>';
		if (j.feito.teste) h += '<p class="fh-res ok">Conexão validada. Agora personalize os avisos.</p>';
		if (j.semAviso) h += '<div class="fh-alerta">' + E(dicaSemAviso(a, m)) + "</div>";
		return h;
	}

	function dicaSemAviso(a, m) {
		return {
			este_pc: "Confira se o Windows não está no modo Não incomodar (Foco) e se o som não está mudo. O popup aparece no canto da tela principal.",
			tv_lg: "A TV precisa estar ligada (não em espera) e com LG Connect Apps ativo. Pareie de novo se você reinstalou o Filuna.",
			luz: "Confira se a luz está ligada na tomada e no mesmo Wi-Fi do computador. Lâmpadas Kasa precisam ser coloridas (KL130, KL125) para mostrar cor.",
			home_assistant: "No Home Assistant, abra Configurações › MQTT › Escutar um tópico, escreva filuna/# e mande o teste de novo.",
			plugin: "Confira os dados do passo Conectar (token, número, endereço). O histórico desta tela mostra a resposta do serviço.",
			tv_android: "Abra o app Filuna TV na TV e confira se ele pode aparecer sobre outros apps (Configurações › Apps › Acesso especial). A TV precisa estar na mesma rede.",
			alexa: "No app Alexa, confira se a skill Filuna está ativada, com a conta Filuna ligada e as notificações permitidas."
		}[m] || "Confira se o aparelho está ligado e na mesma rede.";
	}

	function estiloCartao(est, ev, cfg) {
		var cor = (est.cores || {})[ev] || "#E6007E", tit = (EVENTOS.filter(function (x) { return x[0] == ev; })[0] || EVENTOS[0])[1], tx = preencher(texto(ev, cfg));
		if (est.modelo == "faixa") return '<div class="fh-pv faixa" style="background:' + E(cor) + '"><b>' + E(tit) + "</b><span>" + E(tx) + "</span></div>";
		if (est.modelo == "destaque") return '<div class="fh-pv destaque" style="background:linear-gradient(135deg,' + E(cor) + ',' + E(cor) + 'aa)"><b>' + E(tit) + "</b><span>" + E(tx) + "</span><small>Filuna · " + E(EXEMPLO.impressora) + " · clique para fechar</small></div>";
		return '<div class="fh-pv cartao ' + (est.tema == "escuro" ? "escuro" : "") + '" style="--c:' + E(cor) + '"><i></i><b>' + E(tit) + "</b><span>" + E(tx) + "</span><small>Filuna · " + E(EXEMPLO.impressora) + "</small></div>";
	}

	function previaHtml(a, m, cfg) {
		var ev = st.janela.evPrevia, est = cfg.estilo || {}, cor = (est.cores || {})[ev] || "#E6007E", tit = (EVENTOS.filter(function (x) { return x[0] == ev; })[0] || EVENTOS[0])[1], tx = preencher(texto(ev, cfg));
		if (m == "este_pc") return '<div class="fh-tela">' + estiloCartao(est, ev, cfg) + "</div>";
		if (m == "tv_lg") return '<div class="fh-tv"><div class="fh-toast">' + E(tit + ": " + tx) + "</div></div>";
		if (m == "luz" || m == "luz_conta") return '<div class="fh-lamp" style="--c:' + E(cor) + '"><span></span><small>Pisca 3 vezes e fica ' + E(est.duracao_s || 8) + " s nesta cor</small></div>";
		return '<div class="fh-msg"><b>' + E(tit) + "</b><span>" + E(tx) + "</span></div>";
	}

	function personalizarHtml(a, m, cfg) {
		var j = st.janela, est = cfg.estilo || {}, comCor = m != "tv_lg" && m != "home_assistant", comTexto = m != "luz" && m != "luz_conta";
		var evs = EVENTOS.filter(function (e) { return e[0] != "mensagem" || m != "luz"; });
		var h = '<div class="fh-pers"><div class="fh-pers-col">';
		if (m == "este_pc") {
			h += '<h4 class="fh-sub">Estilo do popup</h4><div class="fh-estilos">' + ESTILOS.map(function (s) {
				return '<label class="fh-estilo' + (est.modelo == s[0] ? " on" : "") + '"><input type="radio" name="fh-modelo" value="' + s[0] + '" data-cfg="estilo.modelo"' + (est.modelo == s[0] ? " checked" : "") + "><span class=\"fh-mini " + s[0] + '"></span><b>' + E(s[1]) + "</b><small>" + E(s[2]) + "</small></label>";
			}).join("") + "</div>";
			h += '<div class="fh-linha"><label class="fh-campo">Tema do cartão<select data-cfg="estilo.tema"><option value="claro"' + (est.tema != "escuro" ? " selected" : "") + '>Claro</option><option value="escuro"' + (est.tema == "escuro" ? " selected" : "") + ">Escuro</option></select></label>" +
				'<label class="fh-campo">Fica na tela<select data-cfg="estilo.duracao_s">' + [[5, "5 segundos"], [10, "10 segundos"], [20, "20 segundos"], [60, "1 minuto"], [0, "até eu fechar"]].map(function (o) { return '<option value="' + o[0] + '"' + (+est.duracao_s == o[0] ? " selected" : "") + ">" + o[1] + "</option>"; }).join("") + "</select></label></div>" +
				chave("estilo.som", "Tocar som", est.som !== false, "O erro tem um som diferente");
		}
		if (m == "luz" || m == "luz_conta") h += '<label class="fh-campo">Quanto tempo a luz fica na cor do aviso (segundos)<input type="number" min="2" max="30" data-cfg="estilo.duracao_s" value="' + E(est.duracao_s || 8) + '"></label>';
		h += '<h4 class="fh-sub">' + (comTexto ? "Texto e cor de cada aviso" : "Cor de cada aviso") + "</h4>";
		h += evs.map(function (e) {
			return '<div class="fh-ev" data-ev="' + e[0] + '"><div class="fh-ev-cab">' + (comCor ? '<input type="color" data-cfg="estilo.cores.' + e[0] + '" value="' + E((est.cores || {})[e[0]] || e[2]) + '">' : "") + "<b>" + E(e[1]) + "</b>" +
				(comTexto && (cfg.textos || {})[e[0]] ? '<button class="fh-link" data-restaura="' + e[0] + '">padrão</button>' : "") + "</div>" +
				(comTexto ? '<input class="fh-tx" data-cfg="textos.' + e[0] + '" value="' + E(texto(e[0], cfg)) + '">' : "") + "</div>";
		}).join("");
		if (comTexto) h += '<p class="fh-nota">Use no texto: <code>{impressora}</code> <code>{peca}</code> <code>{erro}</code> <code>{texto}</code> <code>{progresso}</code>. Em venda e API, {texto} é a mensagem que chegou.</p>';
		h += '</div><div class="fh-pers-col"><h4 class="fh-sub">Prévia</h4><div class="fh-abas">' + evs.map(function (e) { return '<button class="' + (j.evPrevia == e[0] ? "on" : "") + '" data-pv="' + e[0] + '">' + E(e[1].split(" ")[0]) + "</button>"; }).join("") + "</div>" +
			'<div id="fh-previa">' + previaHtml(a, m, cfg) + "</div>" + (m != "luz_conta" ? botao("teste.evento", "Mostrar este aviso " + (m == "este_pc" ? "na tela agora" : "no aparelho")) : "") + "</div></div>";
		return h;
	}

	function usarHtml(a, m, cfg) {
		var imp = st.aparelhos.filter(function (x) { return x.tipo == "impressora"; }), sel = cfg.impressoras || [];
		var h = '<p class="fh-p">Quais avisos ' + E(a.nome) + " recebe.</p>" + EVENTOS.map(function (e) {
			return chave("avisos." + e[0], e[1], (cfg.avisos || {})[e[0]], { concluida: "Qual peça terminou", erro: "O erro em português, na hora", pausada: "Pausa pedida pela impressora ou por você", filamento: "Troca de carretel em qualquer entrada", venda: "Das lojas ligadas à sua conta Filuna", mensagem: "Mandados pela API do Filuna Home" }[e[0]]);
		}).join("");
		if (imp.length > 1) h += '<h4 class="fh-sub">De quais impressoras</h4><div class="fh-imps">' + imp.map(function (p) {
			return '<label><input type="checkbox" data-imp="' + E(p.chave) + '"' + (sel.indexOf(p.chave) >= 0 ? " checked" : "") + "> " + E(p.nome) + "</label>";
		}).join("") + '</div><p class="fh-nota">Nenhuma marcada = todas.</p>';
		var s = cfg.silencio || {};
		h += '<h4 class="fh-sub">Horário silencioso</h4>' + chave("silencio.ligado", "Não avisar neste horário", s.ligado, "Os avisos ficam no histórico") +
			'<div class="fh-linha"><label class="fh-campo">Das<input type="time" data-cfg="silencio.de" value="' + E(s.de || "22:00") + '"></label><label class="fh-campo">Até<input type="time" data-cfg="silencio.ate" value="' + E(s.ate || "07:00") + '"></label></div>' +
			chave("silencio.erro_acorda", "Erro de impressão avisa mesmo assim", s.erro_acorda !== false, "Para não perder uma impressão parada de madrugada");
		if (m == "alexa" || m == "tv_android") h += breve("As escolhas ficam guardadas e passam a valer quando " + (m == "alexa" ? "a skill Filuna" : "o app Filuna TV") + " for publicado.");
		return h;
	}

	function checagensHtml(lista, ic) {
		return lista.map(function (c) { return '<li data-s="' + E(c[0]) + '"><span class="fh-selo">' + E(ic[c[0]] || c[0]) + "</span><div><b>" + E(c[1]) + "</b><small>" + E(c[2]) + "</small></div></li>"; }).join("");
	}

	function definir(cfg, caminho, v) {
		var k = caminho.split("."), o = cfg;
		for (var i = 0; i < k.length - 1; i++) { o[k[i]] = Object.assign({}, o[k[i]] || {}); o = o[k[i]]; }
		o[k[k.length - 1]] = v;
	}

	function atualizarPrevia() {
		var j = st.janela, pv = document.getElementById("fh-previa");
		if (!j || !pv) return;
		pv.innerHTML = previaHtml(j.a, motor(j.a), j.cfg);
	}

	function ligarCampos(f, a, cfg) {
		f.querySelectorAll("[data-cfg]").forEach(function (inp) {
			var mudou = function () {
				var v = inp.type == "checkbox" ? inp.checked : inp.type == "number" || inp.dataset.cfg == "estilo.duracao_s" ? +inp.value : inp.value;
				if (inp.type == "radio" && !inp.checked) return;
				definir(cfg, inp.dataset.cfg, v);
				if (inp.dataset.cfg == "ip") a.ip = v;
				if (inp.type == "radio") { f.querySelectorAll(".fh-estilo").forEach(function (l) { l.classList.toggle("on", l.contains(inp)); }); }
				atualizarPrevia();
			};
			inp.addEventListener("input", mudou); inp.addEventListener("change", mudou);
		});
		f.querySelectorAll("[data-imp]").forEach(function (c) {
			c.onchange = function () { cfg.impressoras = Array.prototype.filter.call(f.querySelectorAll("[data-imp]"), function (x) { return x.checked; }).map(function (x) { return x.dataset.imp; }); };
		});
		f.querySelectorAll("[data-pv]").forEach(function (b) { b.onclick = function () { st.janela.evPrevia = b.dataset.pv; f.querySelectorAll("[data-pv]").forEach(function (x) { x.classList.toggle("on", x == b); }); atualizarPrevia(); }; });
		f.querySelectorAll("[data-restaura]").forEach(function (b) { b.onclick = function () { delete (cfg.textos || {})[b.dataset.restaura]; desenharJanela(); }; });
		f.querySelectorAll("[data-papel]").forEach(function (b) { b.onclick = function () { cfg.papel = b.dataset.papel; a.config = Object.assign({}, a.config || {}, { papel: cfg.papel }); desenharJanela(); }; });
		f.querySelectorAll("[data-ok]").forEach(function (b) {
			b.onclick = function () { var j = st.janela; j.testeOk = false; if (b.dataset.ok == "1") { j.feito.teste = true; j.semAviso = false; } else { j.feito.teste = false; j.semAviso = true; } desenharJanela(); };
		});
		f.querySelectorAll("[data-acao]").forEach(function (b) { b.onclick = function () { executar(a, b, cfg); }; });
		var wa = f.querySelector("#fh-whats");
		if (wa) wa.onclick = function () { Filuna.nativo("url.abrir", { url: WHATS_FILUNA }).catch(function () {}); };
	}

	function executar(a, botaoEl, cfg) {
		var acao = botaoEl.dataset.acao, res = botaoEl.parentNode.querySelector(".fh-res") || document.getElementById("fh-res");
		if (acao == "tomada.testar" && !confirm("A tomada vai desligar e ligar de novo. A impressora ligada nela está parada?")) return;
		var pedido = "p" + Date.now() + Math.random().toString(36).slice(2, 6);
		res.className = "fh-res andando";
		botaoEl.disabled = true;
		var fim = function (r) {
			botaoEl.disabled = false;
			if (!res.isConnected) return;
			res.className = "fh-res " + (r.ok ? "ok" : "erro");
			res.textContent = r.msg || (r.ok ? (r.ligada != null ? "Tomada respondeu: " + (r.ligada ? "ligada" : "desligada") + (r.potencia_w != null ? ", " + Math.round(r.potencia_w) + " W agora." : ".") : "Pronto.") : "Não deu certo.");
			var j = st.janela;
			if (!j) return;
			if (r.ok) j.feito["acao:" + acao] = true; // "teste" fica só para a confirmação "Apareceu?"
			if ((acao == "lg.parear" || acao == "hue.parear" || acao == "filunatv.parear") && r.ok) j.feito.parear = true;
			if ((acao == "tomada.testar" || acao == "tomada.estado") && r.ok) j.feito.teste = true;
			if (acao == "teste") { j.testeOk = !!r.ok; j.semAviso = !r.ok; desenharJanela(); var n = document.querySelector(".fh-j .fh-res"); if (n) { n.className = "fh-res " + (r.ok ? "ok" : "erro"); n.textContent = r.ok ? "Aviso mandado." : r.msg || "Não deu certo."; } }
			if (acao == "rede.diagnostico" && r.checagens) { a.checagens = r.checagens; var ul = document.getElementById("fh-checa"); if (ul) ul.innerHTML = checagensHtml(r.checagens, { ok: "Certo", aviso: "Atenção", ruim: "Ajustar" }); }
			lerHistorico();
		};
		if (acao == "teste" || acao == "teste.evento") {
			res.textContent = "Mandando o aviso…";
			var corpo = { pedido: pedido, chave: a.chave, aparelho: regraDe(a), cfg: cfg, evento: acao == "teste" ? "teste" : st.janela.evPrevia, impressora: EXEMPLO.impressora, texto: EXEMPLO.texto };
			st.pedidos[pedido] = fim;
			Filuna.nativo("casa.testar", corpo).catch(function (e) { delete st.pedidos[pedido]; fim({ ok: false, msg: (e && e.erro) || "Não deu certo." }); });
			return;
		}
		var c = { acao: acao, pedido: pedido, ip: a.ip || cfg.ip, protocolo: protocolo(a) };
		if (acao == "tapo.conta") { c.email = (document.getElementById("fh-tapo-email") || {}).value || ""; c.senha = (document.getElementById("fh-tapo-senha") || {}).value || ""; cfg.tapo_email = c.email; if (c.email && c.senha) st.tapoConta = true; }
		if (acao == "filunatv.parear") { c.codigo = ((document.getElementById("fh-codigo-tv") || {}).value || "").replace(/\D/g, ""); }
		if (protocolo(a) == "tuya_nuvem" && (acao == "tomada.estado" || acao == "tomada.testar")) {
			res.textContent = "Falando com a nuvem da Smart Life…";
			var did = (a.dados || {}).tuya_id;
			var passo = acao == "tomada.estado" ? Filuna.api("POST", "/casa/tuya/comando", { dispositivo: did, acao: "estado" }) :
				Filuna.api("POST", "/casa/tuya/comando", { dispositivo: did, acao: "desligar" }).then(function () { return new Promise(function (ok) { setTimeout(ok, 3000); }); })
					.then(function () { return Filuna.api("POST", "/casa/tuya/comando", { dispositivo: did, acao: "ligar" }); });
			passo.then(function (r) { var e = {}; try { e = JSON.parse(r.msg); } catch (x) {} fim({ ok: r.ok !== false, msg: e.ligada != null ? "Respondeu: " + (e.ligada ? "ligado" : "desligado") + (e.potencia_w != null ? ", " + Math.round(e.potencia_w) + " W agora." : ".") : r.msg }); },
				function (e) { fim({ ok: false, msg: (e && (e.msg || e.erro)) || "A Smart Life não respondeu." }); });
			return;
		}
		if (acao == "mqtt.testar") { c.broker = cfg.broker; c.usuario = cfg.usuario; c.senha = (document.getElementById("fh-senha") || {}).value || ""; st.janela.senha = c.senha; }
		if (acao == "lg.aviso") c.texto = "Filuna: aviso de teste da " + a.nome + ". Está tudo certo.";
		res.textContent = { "lg.parear": "Chamando a TV… aceite o pedido que aparece na tela.", "rede.diagnostico": "Lendo a rede e o Wi-Fi…", "tomada.testar": "Desligando…" }[acao] || "Falando com o aparelho…";
		st.pedidos[pedido] = fim;
		Filuna.nativo("casa.acao", c).catch(function (e) { delete st.pedidos[pedido]; fim({ ok: false, msg: (e && e.erro) || "Não deu certo." }); });
	}

	// "Pronto" só quando o que dá para fazer hoje foi feito de verdade (teste de conexão confirmado); o resto fica em "Configurar".
	function estadoFinal(a, j) {
		var m = motor(a), p = protocolo(a);
		if (a.tipo == "impressora" || a.tipo == "roteador") return a.estado == "atencao" ? "atencao" : "pronto";
		if (a.tipo == "home_assistant") return j.feito["acao:mqtt.testar"] || j.feito.teste ? "pronto" : "configurar";
		if (m == "tomada") return COMANDO_DIRETO[p] && j.feito.teste ? "pronto" : "configurar";
		if (m == "este_pc" || m == "tv_lg" || m == "luz" || m == "plugin" || m == "tv_android" || m == "alexa") return j.feito.teste ? "pronto" : "configurar";
		return a.estado == "incompativel" ? "incompativel" : "configurar";
	}

	function concluir() {
		var j = st.janela, a = j.a, cfg = JSON.parse(JSON.stringify(j.cfg)), novo = estadoFinal(a, j);
		if (novo != "pronto" && motor(a) && ["luz_conta"].indexOf(motor(a)) < 0 && !(motor(a) == "tomada" && !COMANDO_DIRETO[protocolo(a)]) &&
			!confirm("O teste de conexão ainda não foi confirmado, então " + a.nome + " fica em Configurar e não recebe avisos. Concluir assim mesmo?")) return;
		delete cfg.senha;
		if (a.tipo == "home_assistant")
			Filuna.nativo("casa.acao", { acao: "ha.configurar", broker: cfg.broker, usuario: cfg.usuario || "", senha: j.senha || "", ligado: !!(cfg.publicar && cfg.publicar.estado) }).catch(function () {});
		if (cfg.ip && !a.ip) a.ip = cfg.ip;
		cfg._pronto = novo == "pronto";
		guardarLocal(a.chave, cfg);
		if (a.via && a.via.indexOf("à mão") >= 0 || a.tipo == "plugin") gravarManuais(manuais().map(function (x) { return x.chave == a.chave ? Object.assign(x, { ip: a.ip }) : x; }));
		if (motor(a)) Filuna.nativo("casa.regra.salvar", { chave: a.chave, aparelho: regraDe(a), cfg: cfg }).catch(function () {});
		var salvar = Filuna.temToken() && st.daConta && a.id != a.chave ? Filuna.api("POST", "/casa/dispositivos/" + encodeURIComponent(a.id) + "/config", { config: cfg, estado: novo }) : Promise.resolve();
		salvar.then(function () { a.config = cfg; a.estado = novo; fechar(); }, function (e) {
			var r = document.querySelector(".fh-j-corpo");
			if (r) r.insertAdjacentHTML("beforeend", '<p class="fh-res erro">Não deu para salvar na conta agora (' + E((e && e.erro) || "sem conexão") + "). Ficou guardado neste computador.</p>");
			a.config = cfg; a.estado = novo;
		});
	}

	Filuna.ouvir("casa.acao", function (r) { var f = st.pedidos[r.pedido]; if (f) { delete st.pedidos[r.pedido]; f(r); } });
	Filuna.ouvir("casa.teste", function (r) { var f = st.pedidos[r.pedido]; if (f) { delete st.pedidos[r.pedido]; f(r); } });
	Filuna.ouvir("casa.aviso", function (r) { st.hist.unshift(r); st.hist = st.hist.slice(0, 200); redesenhar(); });

	return { desenhar: desenhar, procurar: procurar };
})();
