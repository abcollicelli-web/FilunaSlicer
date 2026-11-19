// Filuna Home dentro do Filuna Slicer (Dispositivo › Casa). O Slicer está na rede da casa: procura os aparelhos
// (FilunaCasa.cpp), manda a lista para a conta (/casa/dispositivos) e é aqui que cada aparelho é configurado.
// A página filuna3d.com/casa mostra a mesma lista e a mesma configuração.
var FilunaCasa = (function () {
	var E = Filuna.esc;
	var ICO = {
		impressora: '<rect x="3.5" y="3.5" width="17" height="17" rx="2.5"/><path d="M3.5 8h17"/><path d="M11 8v3h2V8"/><path d="M12 11v1.6"/><path d="M7 17.5h10"/>',
		tomada: '<circle cx="12" cy="12" r="8.5"/><circle cx="9" cy="11" r="1.1"/><circle cx="15" cy="11" r="1.1"/><circle cx="12" cy="15.2" r="1.1"/>',
		tv: '<rect x="2.5" y="4.5" width="19" height="12.5" rx="2"/><path d="M8.5 20.5h7"/><path d="M12 17v3.5"/>',
		voz: '<ellipse cx="12" cy="7.5" rx="7" ry="2.5"/><path d="M5 7.5v8c0 1.4 3.1 2.5 7 2.5s7-1.1 7-2.5v-8"/><path d="M5 12.5c0 1.4 3.1 2.5 7 2.5s7-1.1 7-2.5"/>',
		rede: '<rect x="3" y="13" width="18" height="6.5" rx="2"/><path d="M7.5 13 6 6.5"/><path d="M16.5 13 18 6.5"/><path d="M9.5 8.8a4 4 0 0 1 5 0"/><path d="M8 6.8a6.5 6.5 0 0 1 8 0"/>',
		luz: '<path d="M9 17.5h6"/><path d="M10 20.5h4"/><path d="M12 3.5a5.5 5.5 0 0 0-3.3 9.9c.5.4.8 1 .8 1.6v.5h5v-.5c0-.6.3-1.2.8-1.6A5.5 5.5 0 0 0 12 3.5Z"/>',
		central: '<path d="M3.5 11 12 4l8.5 7"/><path d="M5.5 9.5V20h13V9.5"/><circle cx="12" cy="14.5" r="2"/>'
	};
	var GRUPOS = [["impressora", "Impressoras"], ["tomada", "Tomadas e energia"], ["tv", "TVs"], ["voz", "Alexa e voz"], ["rede", "Rede"], ["luz", "Luzes"], ["central", "Central da casa"]];
	var ESTADOS = { pronto: "Pronto", configurar: "Configurar", atencao: "Precisa de atenção", incompativel: "Ainda não compatível" };
	var EVENTOS = [["concluida", "Impressão concluída"], ["erro", "Erro de impressão"], ["venda", "Nova venda"]];
	var CONTA = { tapo: "Tapo", kasa: "Kasa", tuya: "Smart Life" };

	var st = { aparelhos: [], locais: [], procurando: false, procurou: false, iniciou: false, quando: 0, erro: "", janela: null, pedidos: {}, impressoras: [] };

	function svg(g) { return '<svg viewBox="0 0 24 24" aria-hidden="true">' + (ICO[g] || ICO.central) + "</svg>"; }
	function grupo(a) {
		var t = a.tipo, papel = (a.config || {}).papel;
		if (t == "smart_life") return papel == "luz" ? "luz" : "tomada";
		if (t == "tv_lg" || t == "tv_android" || t == "tv_samsung" || t == "tv_outra") return "tv";
		if (t == "alexa" || t == "voz_google") return "voz";
		if (t == "roteador") return "rede";
		if (t == "home_assistant") return "central";
		return t == "impressora" || t == "tomada" || t == "luz" ? t : "central";
	}
	function protocolo(a) { return (a.dados || {}).protocolo || a.protocolo || ""; } // da conta, os dados vêm soltos no aparelho
	function guardarLocal(chave, cfg) { try { var o = JSON.parse(localStorage.getItem("fl.casa.cfg") || "{}"); o[chave] = cfg; localStorage.setItem("fl.casa.cfg", JSON.stringify(o)); } catch (e) {} }
	function lerLocal(chave) { try { return (JSON.parse(localStorage.getItem("fl.casa.cfg") || "{}") || {})[chave] || null; } catch (e) { return null; } }
	function manuais() { try { return JSON.parse(localStorage.getItem("fl.casa.manuais") || "[]") || []; } catch (e) { return []; } }

	// ---------------------------------------------------------------- busca e conta

	// As impressoras desta tela também vão para a conta: a tomada de cada uma é escolhida aqui.
	function impressorasComoAparelhos() {
		return st.impressoras.map(function (u) {
			return { chave: "impressora:" + String(u.nome || "").toLowerCase(), tipo: "impressora", nome: u.nome, marca: u.tecnologia == "resina" ? "Resina" : "Filamento",
				modelo: u.modelo || "", ip: (u.viva && u.viva.ip) || "", via: "Filuna Slicer · impressoras deste computador", estado: "configurar", dados: {} };
		});
	}

	function juntarLocal(achados) {
		var por = {};
		achados.concat(manuais()).concat(impressorasComoAparelhos()).forEach(function (a) { if (!por[a.chave]) por[a.chave] = a; });
		return Object.keys(por).map(function (k) { var a = por[k]; a.id = a.id || a.chave; a.config = a.config || lerLocal(a.chave) || {}; if (a.config._pronto) a.estado = "pronto"; return a; });
	}

	function deConta() {
		return Filuna.api("GET", "/casa/dispositivos").then(function (r) {
			// a conta não guarda o diagnóstico do roteador nem o protocolo de cada um: completa com a busca deste computador
			var local = {};
			st.locais.forEach(function (a) { local[a.chave] = a; });
			st.aparelhos = (r.dispositivos || []).map(function (a) {
				var l = local[a.chave];
				if (l) { a.checagens = l.checagens; a.alerta = a.alerta || l.alerta; a.dados = Object.assign({}, a.dados || {}, l.dados || {}); a.fatos = l.fatos; }
				return a;
			});
			st.daConta = true;
		});
	}

	function enviarParaConta(lista) {
		if (!Filuna.temToken()) return Promise.resolve();
		var envio = lista.map(function (a) {
			return { chave: a.chave, tipo: a.tipo, nome: a.nome, marca: a.marca, modelo: a.modelo, ip: a.ip, via: a.via, estado: a.estado,
				dados: Object.assign({ fatos: a.fatos || [], alerta: a.alerta || "", checagens: a.checagens || [] }, a.dados || {}) };
		});
		return Filuna.api("POST", "/casa/dispositivos", { dispositivos: envio }).catch(function () {});
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

	// ---------------------------------------------------------------- tela

	var alvo = null, cabecalho = "", aoLigar = null;

	function desenhar(app, barra, ligarBarra, impressoras) {
		alvo = app; cabecalho = barra; aoLigar = ligarBarra; st.impressoras = impressoras || [];
		if (st.janela) return; // a leitura de 15 s não fecha a janela aberta
		if (!st.iniciou) { // primeira vez: usa a última busca deste computador; sem nenhuma, procura agora
			st.iniciou = true;
			st.locais = juntarLocal([]);
			st.aparelhos = st.locais;
			Filuna.nativo("casa.resultado").then(function (r) {
				if (!(r.aparelhos || []).length) { procurar(); return; }
				st.procurou = true; st.quando = (r.quando || 0) * 1000; st.rede = r.rede || {};
				st.locais = juntarLocal(r.aparelhos); st.aparelhos = st.locais;
				(Filuna.temToken() ? deConta() : Promise.resolve()).catch(function () {}).then(redesenhar);
			}, function () { procurar(); });
		} else if (!st.procurando) { // impressoras novas desta tela
			st.locais = juntarLocal(st.locais.filter(function (a) { return a.tipo != "impressora"; }));
			if (!st.daConta) st.aparelhos = st.locais;
		}
		redesenhar();
	}

	function redesenhar() {
		if (!alvo || st.janela) return;
		var lista = st.aparelhos, cont = {};
		lista.forEach(function (a) { cont[a.estado] = (cont[a.estado] || 0) + 1; });
		var h = cabecalho + '<div class="fh"><div class="fh-cab"><div><div class="h1">Filuna Home</div>' +
			'<p>O Filuna encontra os aparelhos da sua casa e organiza tudo aqui. Toque em um aparelho e configure em poucos passos: impressão concluída, erro e nova venda chegam na TV, na Alexa, nas luzes e no celular.</p></div>' +
			'<div class="fh-busca' + (st.procurando ? " girando" : "") + '"><span class="fh-radar"></span><div><b>' +
			(st.procurando ? "Procurando na sua rede…" : lista.length + (lista.length == 1 ? " aparelho na sua rede" : " aparelhos na sua rede")) + "</b><small>" +
			(st.procurando ? "TVs, tomadas, luzes, roteador e central" : st.quando ? "Busca deste computador · " + new Date(st.quando).toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" }) : "Ainda não procurou") +
			'</small></div><button class="btn" id="fh-procurar"' + (st.procurando ? " disabled" : "") + ">Procurar de novo</button></div></div>";
		if (!Filuna.temToken()) h += '<div class="fh-aviso-top">Entre na sua conta Filuna para a configuração valer também no site e no app. Sem conta, ela fica só neste computador.</div>';
		if (st.erro) h += '<div class="fh-aviso-top">' + E(st.erro) + "</div>";
		h += '<ul class="fh-resumo">' + Object.keys(ESTADOS).filter(function (k) { return cont[k]; }).map(function (k) { return '<li data-estado="' + k + '"><b>' + cont[k] + "</b> " + ESTADOS[k].toLowerCase() + "</li>"; }).join("") + "</ul>";
		GRUPOS.forEach(function (g) {
			var itens = lista.filter(function (a) { return grupo(a) == g[0]; });
			if (!itens.length) return;
			h += '<section class="fh-grupo"><h2>' + svg(g[0]) + E(g[1]) + " <span>" + itens.length + '</span></h2><div class="fh-grade">' + itens.map(function (a) {
				return '<button class="fh-ap" data-id="' + E(a.id) + '" data-estado="' + E(a.estado) + '"><span class="fh-ico">' + svg(g[0]) + '</span><span class="fh-ap-txt"><b>' + E(a.nome) +
					"</b><small>" + E([a.marca, a.modelo].filter(Boolean).join(" · ")) + (a.ip ? " · " + E(a.ip) : "") + '</small></span><span class="fh-estado">' + E(ESTADOS[a.estado] || a.estado) + "</span></button>";
			}).join("") + "</div></section>";
		});
		if (!lista.length && !st.procurando) h += '<div class="fh-vazio">Nenhum aparelho respondeu. Confira se o computador está na mesma rede da casa (não na rede de visitantes) e toque em Procurar de novo.</div>';
		h += '<div class="fh-mais"><button class="btn" id="fh-alexa">+ Adicionar Alexa</button><button class="btn" id="fh-ha">+ Adicionar Home Assistant pelo endereço</button></div>';
		h += '<div class="nota">A Alexa não aparece na busca: ela fala com o Filuna pela conta da Amazon, não pela rede da casa.</div></div>';
		alvo.innerHTML = h;
		if (aoLigar) aoLigar(alvo);
		alvo.querySelector("#fh-procurar").onclick = procurar;
		alvo.querySelector("#fh-alexa").onclick = function () { adicionarManual({ chave: "alexa:conta", tipo: "alexa", nome: "Alexa", marca: "Amazon", modelo: "Echo", ip: "", via: "Filuna Slicer · adicionada à mão", estado: "configurar", fatos: [["Como avisa", "luz amarela; com tela, um banner"], ["Fala o aviso", "quando você pede"], ["Precisa", "skill Filuna ativada"]] }); };
		alvo.querySelector("#fh-ha").onclick = function () { adicionarManual({ chave: "ha:manual", tipo: "home_assistant", nome: "Home Assistant", marca: "Home Assistant", modelo: "servidor da casa", ip: "", via: "Filuna Slicer · adicionado à mão", estado: "configurar", fatos: [["O que o Filuna faz", "publica todas as impressoras lá"], ["Protocolo", "MQTT, na rede da casa"]] }); };
		alvo.querySelectorAll(".fh-ap").forEach(function (b) { b.onclick = function () { abrir(b.dataset.id); }; });
	}

	function adicionarManual(a) {
		var m = manuais().filter(function (x) { return x.chave != a.chave; }); m.push(a);
		try { localStorage.setItem("fl.casa.manuais", JSON.stringify(m)); } catch (e) {}
		st.locais = juntarLocal(st.locais || []);
		st.aparelhos = st.locais;
		enviarParaConta([a]).then(function () { return Filuna.temToken() ? deConta() : null; }).catch(function () {}).then(function () {
			redesenhar();
			var novo = st.aparelhos.filter(function (x) { return x.chave == a.chave; })[0];
			if (novo) abrir(novo.id);
		});
	}

	// ---------------------------------------------------------------- janela de configuração

	function passos(a) {
		var p = protocolo(a);
		switch (a.tipo) {
			case "impressora": return ["Reconhecida", "Tomada", "Avisos"];
			case "tomada": return ["Reconhecida", "Conectar", "Impressora", p == "shelly" ? "Testar" : null].filter(Boolean);
			case "smart_life": return ["Reconhecido", "O que é", "Conectar", (a.config || {}).papel == "luz" ? "Cores" : "Impressora"];
			case "tv_lg": return ["Reconhecida", "Parear", "Avisos", "Testar"];
			case "tv_android": return ["Reconhecida", "Instalar", "Avisos"];
			case "alexa": return ["Reconhecida", "Ativar skill", "Avisos"];
			case "roteador": return ["Reconhecido", "Diagnóstico"];
			case "luz": return ["Reconhecida", "Conectar", "Cores"];
			case "home_assistant": return ["Reconhecido", "Conectar", "Publicar"];
			default: return ["Reconhecida"];
		}
	}

	function cfgPadrao(a) {
		var c = { avisos: { concluida: true, erro: true, venda: grupo(a) != "luz" }, silencio: true };
		if (grupo(a) == "tomada") Object.assign(c, { impressora: "", desligarFrio: true, remoto: true, corte: false });
		if (grupo(a) == "luz" || a.tipo == "smart_life") c.cores = { concluida: "#1F8F5F", erro: "#E0261B", venda: "#2F6FEB" };
		if (a.tipo == "home_assistant") Object.assign(c, { broker: (a.ip ? a.ip : "") + ":1883", usuario: "", publicar: { estado: true, filamento: true } });
		var salvo = JSON.parse(JSON.stringify(a.config || {}));
		return Object.assign(c, salvo);
	}

	function abrir(id) {
		var a = st.aparelhos.filter(function (x) { return x.id == id; })[0];
		if (!a) return;
		st.janela = { a: a, passo: 0, cfg: cfgPadrao(a), feito: {} };
		if (a.tipo == "tv_lg" && a.ip) Filuna.nativo("casa.acao", { acao: "lg.pareada", ip: a.ip }).then(function (r) { if (st.janela && r.pareada) { st.janela.feito.parear = true; desenharJanela(); } });
		desenharJanela();
	}

	function fechar() { var f = document.getElementById("fh-fundo"); if (f) f.remove(); st.janela = null; redesenhar(); }

	function desenharJanela() {
		var j = st.janela, a = j.a, ps = passos(a), nome = ps[j.passo];
		var f = document.getElementById("fh-fundo");
		if (!f) { f = document.createElement("div"); f.id = "fh-fundo"; f.className = "fh-fundo"; document.body.appendChild(f); f.onclick = function (e) { if (e.target == f) fechar(); }; }
		var ultimo = j.passo == ps.length - 1;
		f.innerHTML = '<div class="fh-j"><div class="fh-j-cab"><span class="fh-ico">' + svg(grupo(a)) + '</span><div><h3>' + E(a.nome) + "</h3><p>" + E([a.marca, a.modelo].filter(Boolean).join(" · ")) +
			'</p></div><button class="fh-fechar" id="fh-x" aria-label="Fechar">×</button></div>' +
			(ps.length > 1 ? '<ol class="fh-passos">' + ps.map(function (p, i) { return '<li class="' + (i == j.passo ? "on" : i < j.passo ? "feito" : "") + '"><b>' + (i + 1) + "</b>" + E(p) + "</li>"; }).join("") + "</ol>" : "") +
			'<div class="fh-j-corpo">' + conteudo(a, nome, j.cfg) + '</div><div class="fh-j-rod"><button class="btn" id="fh-voltar"' + (j.passo ? "" : ' style="visibility:hidden"') + '>Voltar</button>' +
			'<button class="btn primario" id="fh-avancar">' + (ps.length == 1 ? "Entendi" : ultimo ? "Concluir" : "Continuar") + "</button></div></div>";
		f.querySelector("#fh-x").onclick = fechar;
		f.querySelector("#fh-voltar").onclick = function () { if (j.passo) { j.passo--; desenharJanela(); } };
		f.querySelector("#fh-avancar").onclick = function () { if (ps.length == 1) fechar(); else if (!ultimo) { j.passo++; desenharJanela(); } else concluir(); };
		ligarCampos(f, a, j.cfg);
	}

	function fatos(a) {
		return '<dl class="fh-fatos">' + (a.fatos || []).map(function (x) { return "<div><dt>" + E(x[0]) + "</dt><dd>" + E(x[1]) + "</dd></div>"; }).join("") +
			(a.ip ? "<div><dt>Endereço na rede</dt><dd>" + E(a.ip) + "</dd></div>" : "") + "<div><dt>Encontrado por</dt><dd>" + E(a.via || "") + "</dd></div></dl>";
	}
	function chave(nome, rotulo, marcado, dica) {
		return '<label class="fh-chave"><span><b>' + E(rotulo) + "</b>" + (dica ? "<small>" + E(dica) + "</small>" : "") + '</span><input type="checkbox" data-cfg="' + nome + '"' + (marcado ? " checked" : "") + "></label>";
	}
	function botao(acao, rotulo) { return '<button class="btn primario" data-acao="' + acao + '">' + E(rotulo) + '</button><p class="fh-res" id="fh-res"></p>'; }
	function breve(txt) { return '<div class="fh-breve">' + E(txt) + "</div>"; }
	function opcoesImpressoras(cfg) {
		return st.aparelhos.filter(function (x) { return x.tipo == "impressora"; }).map(function (p) { return '<option value="' + E(p.chave) + '"' + (cfg.impressora == p.chave ? " selected" : "") + ">" + E(p.nome) + "</option>"; }).join("");
	}

	function conteudo(a, nome, cfg) {
		var p = protocolo(a);
		if (/^Reconhecid/.test(nome)) {
			var intro = { pronto: "Este aparelho já está ligado ao Filuna.", configurar: "O Filuna encontrou este aparelho na sua rede. Faltam poucos passos.",
				atencao: "O Filuna encontrou este aparelho, mas há um ponto para ajustar.", incompativel: "O Filuna encontrou este aparelho, mas ele ainda não aceita avisos." }[a.estado] || "";
			if (a.tipo == "impressora") intro = "Esta impressora está cadastrada neste computador. Ligue a tomada inteligente dela e escolha os avisos.";
			var extra = "";
			if (a.tipo == "tv_samsung") extra = '<p class="fh-p">A Samsung só mostra na tela avisos de eletrodomésticos da própria marca. Para receber os avisos nesta TV, ligue um Fire TV Stick ou um aparelho com Google TV no HDMI e instale o app Filuna TV.</p>';
			if (a.tipo == "voz_google") extra = '<p class="fh-p">O Google ainda não deixa outros apps mandarem avisos para o Nest. Para avisos por voz, use um Amazon Echo.</p>';
			return '<p class="fh-p">' + E(intro) + "</p>" + (a.alerta ? '<div class="fh-alerta">' + E(a.alerta) + "</div>" : "") + fatos(a) + extra;
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
		if (nome == "Conectar" && (grupo(a) == "tomada" || a.tipo == "smart_life") && p == "shelly")
			return '<p class="fh-p">Esta tomada aceita comando direto na rede da casa. Não precisa de conta.</p>' + botao("shelly.estado", "Conectar pela rede");
		if (nome == "Conectar" && (grupo(a) == "tomada" || grupo(a) == "luz" || a.tipo == "smart_life")) {
			var conta = CONTA[p] || (a.marca == "Philips Hue" ? "Hue" : "do fabricante");
			return '<p class="fh-p">Este aparelho é comandado pela conta ' + E(conta) + ". O Filuna entra com a sua conta uma vez e guarda só a autorização, cifrada.</p>" +
				breve("A entrada com a conta " + conta + " chega numa próxima versão do Filuna. Já dá para escolher a impressora e as regras: elas passam a valer assim que a conta estiver ligada.");
		}
		if (nome == "Impressora") {
			return '<label class="fh-campo">Impressora ligada nesta tomada<select data-cfg="impressora"><option value="">Escolha</option>' + opcoesImpressoras(cfg) + "</select></label>" +
				chave("desligarFrio", "Desligar quando a impressão terminar", cfg.desligarFrio, "Só depois que o bico esfriar abaixo de 50 °C") +
				chave("remoto", "Ligar e desligar pelo celular", cfg.remoto, "Ligar não começa impressão: alguém confere a mesa antes") +
				chave("corte", "Cortar a energia em falha grave", cfg.corte, "Desligado por padrão. Avisa antes e dá 2 minutos para cancelar");
		}
		if (nome == "Parear")
			return '<p class="fh-p">Ligue a TV. Quando tocar em Parear, aparece um pedido na tela da TV: aceite com o controle. É só uma vez.</p>' +
				'<p class="fh-nota">Se o pedido não aparecer: na TV, abra Configurações › Rede e ligue <b>LG Connect Apps</b>.</p>' +
				(st.janela.feito.parear ? '<p class="fh-res ok">Esta TV já está pareada com este computador.</p>' : "") + botao("lg.parear", st.janela.feito.parear ? "Parear de novo" : "Parear agora");
		if (nome == "Instalar")
			return '<ol class="fh-lista"><li>Na TV, abra a loja de apps e instale <b>Filuna TV</b>.</li><li>Abra o app: ele mostra um código de 6 dígitos.</li><li>Digite o código aqui.</li></ol>' +
				breve("O app Filuna TV ainda não foi publicado nas lojas da Google e da Amazon. Assim que sair, o código de 6 dígitos liga esta TV.") +
				'<label class="fh-campo">Código que aparece na TV<input inputmode="numeric" maxlength="6" placeholder="000000" data-cfg="codigo" value="' + E(cfg.codigo || "") + '" disabled></label>';
		if (nome == "Ativar skill")
			return '<ol class="fh-lista"><li>No app Alexa, toque em Mais › Skills e jogos e procure <b>Filuna</b>.</li><li>Toque em Ativar e entre com a sua conta Filuna.</li><li>Permita as notificações quando a Alexa pedir.</li></ol>' +
				breve("A skill Filuna ainda não foi publicada na Amazon. Os avisos escolhidos aqui ficam guardados e passam a valer quando ela sair.") +
				'<p class="fh-nota">O Echo acende a luz amarela quando chega um aviso; os que têm tela mostram um banner. Para ouvir, diga "Alexa, quais são as minhas notificações?".</p>';
		if (nome == "Diagnóstico") {
			var ic = { ok: "Certo", aviso: "Atenção", ruim: "Ajustar" };
			return (a.alerta ? '<div class="fh-alerta">' + E(a.alerta) + "</div>" : "") + '<ul class="fh-checa" id="fh-checa">' + checagensHtml(a.checagens || [], ic) + "</ul>" + botao("rede.diagnostico", "Verificar de novo");
		}
		if (nome == "Cores")
			return '<p class="fh-p">Uma cor para cada aviso. A luz pisca 3 vezes e volta ao normal.</p><div class="fh-cores">' +
				EVENTOS.map(function (e) { return '<label><input type="color" data-cfg="cores.' + e[0] + '" value="' + E(cfg.cores[e[0]]) + '">' + E(e[1]) + "</label>"; }).join("") + "</div>";
		if (nome == "Conectar" && a.tipo == "home_assistant")
			return '<p class="fh-p">O Filuna publica as impressoras no Home Assistant pelo MQTT da sua casa. Elas aparecem lá sozinhas, prontas para automações.</p>' +
				'<label class="fh-campo">Endereço do MQTT<input data-cfg="broker" value="' + E(cfg.broker || "") + '" placeholder="192.168.0.10:1883"></label>' +
				'<label class="fh-campo">Usuário do MQTT<input data-cfg="usuario" value="' + E(cfg.usuario || "") + '" autocomplete="off"></label>' +
				'<label class="fh-campo">Senha do MQTT<input type="password" id="fh-senha" autocomplete="off" placeholder="fica só neste computador"></label>' + botao("mqtt.testar", "Testar conexão");
		if (nome == "Publicar")
			return chave("publicar.estado", "Estado e progresso das impressoras", cfg.publicar.estado, "Imprimindo, livre, erro, tempo restante, bico e mesa") +
				'<p class="fh-nota">Ao concluir, o Filuna publica as impressoras a cada 30 segundos enquanto este computador estiver ligado. No Home Assistant, ligue a integração MQTT: elas aparecem em Dispositivos.</p>';
		if (nome == "Avisos") {
			var casa = a.tipo != "impressora";
			return '<p class="fh-p">Escolha o que este aparelho avisa.</p>' + EVENTOS.map(function (e) {
				return chave("avisos." + e[0], e[1], cfg.avisos[e[0]], { concluida: "Com a capa da peça e o próximo da fila", erro: "Com o erro em português e o que fazer", venda: "Produto, prazo de despacho e se tem peça pronta" }[e[0]]);
			}).join("") + (casa ? chave("silencio", "Horário silencioso, das 22h às 7h", cfg.silencio, 'Erro de impressão ainda avisa se você marcar "acordar em erro" no celular') : "");
		}
		if (nome == "Testar") {
			if (grupo(a) == "tomada") return '<p class="fh-p">Teste a tomada: ela desliga e liga de novo em 3 segundos. Faça com a impressora parada.</p>' + botao("shelly.testar", "Desligar e ligar");
			return '<p class="fh-p">Mande um aviso de teste e confira se apareceu na TV.</p>' + botao("lg.aviso", "Mandar aviso de teste");
		}
		return "";
	}

	function checagensHtml(lista, ic) {
		return lista.map(function (c) { return '<li data-s="' + E(c[0]) + '"><span class="fh-selo">' + E(ic[c[0]] || c[0]) + "</span><div><b>" + E(c[1]) + "</b><small>" + E(c[2]) + "</small></div></li>"; }).join("");
	}

	function ligarCampos(f, a, cfg) {
		f.querySelectorAll("[data-cfg]").forEach(function (inp) {
			var mudou = function () {
				var v = inp.type == "checkbox" ? inp.checked : inp.value, k = inp.dataset.cfg.split(".");
				if (k[1]) { cfg[k[0]] = Object.assign({}, cfg[k[0]] || {}); cfg[k[0]][k[1]] = v; } else cfg[k[0]] = v;
			};
			inp.addEventListener("input", mudou); inp.addEventListener("change", mudou);
		});
		f.querySelectorAll("[data-papel]").forEach(function (b) { b.onclick = function () { cfg.papel = b.dataset.papel; a.config = Object.assign({}, a.config || {}, { papel: cfg.papel }); desenharJanela(); }; });
		f.querySelectorAll("[data-acao]").forEach(function (b) { b.onclick = function () { executar(a, b, cfg); }; });
	}

	function executar(a, botaoEl, cfg) {
		var acao = botaoEl.dataset.acao, res = document.getElementById("fh-res");
		if (acao == "shelly.testar" && !confirm("A tomada vai desligar e ligar de novo. A impressora ligada nela está parada?")) return;
		var pedido = "p" + Date.now() + Math.random().toString(36).slice(2, 6);
		var corpo = { acao: acao, pedido: pedido, ip: a.ip };
		if (acao == "mqtt.testar") { corpo.broker = cfg.broker; corpo.usuario = cfg.usuario; corpo.senha = (document.getElementById("fh-senha") || {}).value || ""; st.janela.senha = corpo.senha; }
		if (acao == "lg.aviso") corpo.texto = "Filuna: aviso de teste da " + a.nome + ". Está tudo certo.";
		res.className = "fh-res andando";
		res.textContent = { "lg.parear": "Chamando a TV… aceite o pedido que aparece na tela.", "rede.diagnostico": "Lendo a rede e o Wi-Fi…", "shelly.testar": "Desligando…" }[acao] || "Falando com o aparelho…";
		botaoEl.disabled = true;
		st.pedidos[pedido] = function (r) {
			botaoEl.disabled = false;
			var el = document.getElementById("fh-res");
			if (!el) return;
			el.className = "fh-res " + (r.ok ? "ok" : "erro");
			el.textContent = r.msg || (r.ok ? (r.ligada != null ? "Tomada respondeu: " + (r.ligada ? "ligada" : "desligada") + (r.potencia_w != null ? ", " + Math.round(r.potencia_w) + " W agora." : ".") : "Pronto.") : "Não deu certo.");
			if (r.ok && st.janela) st.janela.feito[acao] = true;
			if (acao == "lg.parear" && r.ok && st.janela) st.janela.feito.parear = true;
			if (acao == "rede.diagnostico" && r.checagens) { a.checagens = r.checagens; var ul = document.getElementById("fh-checa"); if (ul) ul.innerHTML = checagensHtml(r.checagens, { ok: "Certo", aviso: "Atenção", ruim: "Ajustar" }); }
		};
		Filuna.nativo("casa.acao", corpo).catch(function (e) { delete st.pedidos[pedido]; botaoEl.disabled = false; res.className = "fh-res erro"; res.textContent = (e && e.erro) || "Não deu certo."; });
	}

	// "Pronto" só quando o que dá para fazer hoje foi feito de verdade; o resto fica em "Configurar".
	function estadoFinal(a, j) {
		var p = protocolo(a);
		if (a.tipo == "impressora" || a.tipo == "roteador") return a.estado == "atencao" ? "atencao" : "pronto";
		if (a.tipo == "tv_lg") return j.feito.parear ? "pronto" : "configurar";
		if (a.tipo == "home_assistant") return j.feito["mqtt.testar"] ? "pronto" : "configurar";
		if (grupo(a) == "tomada" && p == "shelly") return j.feito["shelly.estado"] || j.feito["shelly.testar"] ? "pronto" : "configurar";
		return a.estado == "incompativel" ? "incompativel" : "configurar";
	}

	function concluir() {
		var j = st.janela, a = j.a, cfg = JSON.parse(JSON.stringify(j.cfg)), novo = estadoFinal(a, j);
		delete cfg.senha;
		if (a.tipo == "home_assistant")
			Filuna.nativo("casa.acao", { acao: "ha.configurar", broker: cfg.broker, usuario: cfg.usuario || "", senha: j.senha || "", ligado: !!(cfg.publicar && cfg.publicar.estado) }).catch(function () {});
		cfg._pronto = novo == "pronto";
		guardarLocal(a.chave, cfg);
		var salvar = Filuna.temToken() && st.daConta && a.id != a.chave ? Filuna.api("POST", "/casa/dispositivos/" + encodeURIComponent(a.id) + "/config", { config: cfg, estado: novo }) : Promise.resolve();
		salvar.then(function () { a.config = cfg; a.estado = novo; fechar(); }, function (e) {
			var r = document.getElementById("fh-res") || document.querySelector(".fh-j-corpo");
			if (r) r.insertAdjacentHTML("beforeend", '<p class="fh-res erro">Não deu para salvar na conta agora (' + E((e && e.erro) || "sem conexão") + "). Ficou guardado neste computador.</p>");
			a.config = cfg; a.estado = novo;
		});
	}

	Filuna.ouvir("casa.acao", function (r) { var f = st.pedidos[r.pedido]; if (f) { delete st.pedidos[r.pedido]; f(r); } });

	return { desenhar: desenhar, procurar: procurar };
})();
