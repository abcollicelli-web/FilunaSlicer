/* Filuna Farm no Slicer — o pedido em 6 etapas (porte de farm/pedido.html e farm/novo.html), a navegação e o projeto local. */
(function () {
"use strict";
var FP = window.FarmPlano, W = window.FarmWizard, T = window.FarmTelas, st = T.st, tela = T.tela, E = T.E;
var horas = T.horas, g = T.g, brl = T.brl, cm = T.cm, quando = T.quando, pl_ = T.pl_, foto = T.foto, cor = T.cor, botao = T.botao, pq = T.pq;
var API = "/farm";

// ---------------- etapa 1: novo pedido (sempre local no Slicer) ----------------
function vNovo(d) {
	var lim = (st.dados || {}).lim || { farm_impressoras: 1, plano: "" };
	var h = '<div class="cab-pag"><div><h1>Imprimir</h1><p>Escolha o arquivo. O Filuna abre aqui no computador, separa as peças e as cores e faz as perguntas uma etapa por vez.</p></div><a class="btn btn-line btn-sm" href="/farm">Voltar para a Farm</a></div>';
	if (!lim.farm_impressoras) return h + T.sem_plano(lim);
	h += '<div class="fw">' + T.etapas(W.PASSOS, 1, 1) + '<div class="fw-palco">';
	if (d.erro) h += '<div class="alerta erro">' + E(d.erro) + "</div>";
	h += '<section class="bloco"><p class="fm-passo">Etapa 1 de 6</p><h2 class="fm-h2">Escolha seu arquivo</h2><form method="post" data-local="novo">';
	if (d.de) h += '<div class="fm-aviso violeta"><b>Imprimir de novo: pedido #' + E(d.de.numero) + " · " + E(d.de.nome) + "</b><p>Escolha o mesmo arquivo outra vez. A quantidade, os materiais e as opções do pedido #" + E(d.de.numero) + " já vêm preenchidos; é só conferir.</p></div>";
	h += '<label class="fw-solta" data-escolher><svg width="54" height="54" viewBox="0 0 56 56" aria-hidden="true"><rect x="8" y="6" width="40" height="44" rx="7" fill="none" stroke="currentColor" stroke-width="2"/><path d="M28 38V19M20 26l8-8 8 8" fill="none" stroke="var(--mag)" stroke-width="2.8" stroke-linecap="round" stroke-linejoin="round"/></svg>' +
		"<b>" + (d.arquivo ? E(d.arquivo.nome) : "Clique para escolher o arquivo") + "</b><small>" + (d.arquivo ? (d.arquivo.tamanho / 1048576).toFixed(1).replace(".", ",") + " MB" : "ZIP, 3MF ou STL, até 100 MB") + "</small></label>" +
		'<p style="margin:-6px 0 16px"><button type="button" class="btn btn-line btn-sm" data-preparacao>Usar o projeto aberto na Preparação</button></p>' +
		'<label class="campo"><span>Nome do pedido (opcional)</span><input name="nome" maxlength="80" placeholder="Ex.: Porta-alho" value="' + E(d.nome || (d.de ? d.de.nome : "")) + '"></label>' +
		'<button class="btn btn-mag"' + (d.arquivo && !d.analisando ? "" : " disabled") + ">" + (d.analisando ? "Abrindo o arquivo e separando as peças…" : 'Analisar <span class="seta">→</span>') + "</button>" +
		'<p class="fm-nota">Tudo acontece neste computador: o Filuna lê o arquivo, mede tempo e filamento fatiando com os perfis da sua impressora e monta o plano. No fim, o projeto vai para a sua conta só para você acompanhar pelo site e pelo app.</p></form></section></div></div>';
	return h;
}

// ---------------- etapas 2 a 6 (iguais à página do site) ----------------
function vPedido(c) {
	var ped = c.ped, r = c.r, passo = c.passo, volta = "/farm/pedidos/" + ped.id + "?passo=" + passo, h = "";
	h += '<div class="cab-pag"><div><h1>Pedido' + (ped.numero ? " #" + E(ped.numero) : "") + "</h1><p>" + E(ped.nome) + " · " + E(ped.arquivo) + (ped.criado_em ? " · enviado em " + quando(ped.criado_em) : " · no Filuna Slicer") +
		'</p></div><a class="btn btn-line btn-sm" href="/farm">Voltar para a Farm</a></div><div class="fw">' + T.etapas(W.PASSOS, passo, c.alcancado, ped.id, c.enviado) + '<div class="fw-palco">';
	if (ped.estado == "calculando" && passo < 6) h += '<div class="fw-medindo"><span class="fw-gira"></span><div><b>Medindo tempo e filamento</b><small>' + E(ped.progresso || "Preparando") + "… " + (c.local ? "O Filuna Slicer fatia cada arquivo com os perfis da sua impressora." : "O servidor fatia cada peça de verdade.") + "</small></div></div>";
	if (c.medida_erro) h += '<div class="alerta erro">' + E(c.medida_erro) + "</div>";
	if (passo == 2) {
		var n = r.conjuntos || 1;
		h += '<section class="bloco"><p class="fm-passo">Etapa 2 de 6</p><h2 class="fm-h2">Quantos conjuntos você quer imprimir?</h2><p class="fw-sub">O arquivo tem ' + c.pecas.length + " peça" + pl_(c.pecas.length, "", "s") + " diferente" + pl_(c.pecas.length, "", "s") + " e " + c.n_cores + " cor" + pl_(c.n_cores, "", "es") +
			'. O Filuna monta as peças na mesa de cada impressora, com folga entre elas, e divide em mais mesas quando não cabe.</p><form method="post" id="fw-qtd"><input type="hidden" name="acao" value="salvar"><input type="hidden" name="passo" value="3"><div class="fw-qtd"><label class="campo fm-curto"><span>Conjuntos</span><input type="number" name="conjuntos" min="1" max="100000" value="' + n + '" required data-fw-qtd></label><div class="fw-rapidos">' +
			[1, 2, 5, 10, 20].map(function (k) { return '<button type="button" class="fw-chip' + (n == k ? " on" : "") + '" data-fw-n="' + k + '">' + k + "</button>"; }).join("") + "</div></div>" + pq(c.exp, "conjuntos") + '<h3 class="fm-h3">Peças iguais vão juntas na mesa?</h3><div class="fm-ops">' +
			[["juntas", "Encher a mesa com peças iguais"], ["uma_por_mesa", "Uma peça por mesa"]].map(function (o) { return '<label class="fm-op"><input type="radio" name="iguais" value="' + o[0] + '"' + ((r.iguais || "juntas") == o[0] ? " checked" : "") + "><span>" + o[1] + "</span></label>"; }).join("") + "</div>" + pq(c.exp, "iguais") + "</form></section>";
		h += '<section class="bloco"><h2 class="fw-h">Como cada peça cabe nas suas mesas</h2><div class="fw-pecas">';
		c.encaixe.forEach(function (e) {
			h += '<article class="fw-peca">' + foto(e.peca.id) + '<div class="fw-peca-txt"><b>' + E(e.peca.nome) + (e.peca.copias > 1 ? " × " + e.peca.copias + " por conjunto" : "") + "</b><small>" + cm(e.peca.x) + " × " + cm(e.peca.y) + " × " + cm(e.peca.z) + " cm · " + e.total + " peça" + pl_(e.total, "", "s") + ' no total</small><ul class="fw-cabem">' +
				(e.imps.length ? e.imps.map(function (i) { return i.cabem ? "<li><span>" + E(i.nome) + "</span><b>" + i.cabem + " por mesa · " + i.mesas + " mesa" + pl_(i.mesas, "", "s") + "</b></li>" : "<li><span>" + E(i.nome) + '</span><b class="warn">não cabe (' + E(i.mesa) + ")</b></li>"; }).join("") : '<li><span>Nenhuma impressora cadastrada.</span><a href="/farm/impressoras">Cadastrar</a></li>') + "</ul></div></article>";
		});
		h += '</div><p class="fm-nota">Na próxima etapa o Filuna confere se o filamento de cada cor dá para essa quantidade.</p></section><div class="fw-rodape"><span></span><button class="btn btn-mag" form="fw-qtd">Continuar <span class="seta">→</span></button></div>';
	}
	if (passo == 3) {
		var nok = c.cores.filter(function (x) { return x.confirmado; }).length;
		h += '<section class="bloco fw-cab3"><div><p class="fm-passo">Etapa 3 de 6</p><h2 class="fm-h2">Cor e material</h2><p class="fw-sub">Procurei cada cor em todas as impressoras e conferi o carretel para ' + (r.conjuntos || 1) + " conjunto" + pl_(r.conjuntos || 1, "", "s") + ". Confirme o material de cada cor, ou peça outro.</p></div>" +
			'<span class="pilula ' + (nok == c.cores.length ? "ok" : "warn") + '">' + nok + "/" + c.cores.length + " confirmada" + pl_(c.cores.length, "", "s") + "</span></section>";
		if (c.falta_cor) h += '<div class="alerta erro">Escolha a cor das peças que vieram sem cor no arquivo.</div>';
		h += '<form method="post" id="fw-cores"><input type="hidden" name="acao" value="responder"><input type="hidden" name="passo" value="4"></form>';
		var sem = c.pecas.filter(function (p) { return !p.cores || !p.cores.length; });
		if (sem.length) {
			h += '<section class="bloco"><h3 class="fw-h">Estas peças vieram sem cor no arquivo. Em que cor imprimir?</h3><div class="fm-escolhas">';
			sem.forEach(function (p) { h += '<label class="campo"><span>' + E(p.nome) + '</span><select name="cor_' + E(p.id) + '" form="fw-cores" required><option value="">Escolha a cor</option>' + c.cores_estoque.map(function (x) { return '<option value="' + E(x.hex) + '"' + (p.cor == x.hex ? " selected" : "") + ">" + E(x.nome) + (x.estoque ? " (no estoque)" : "") + "</option>"; }).join("") + "</select></label>"; });
			h += '</div><p class="fm-nota">STL não guarda cor. Depois de escolher, a cor aparece aqui para você confirmar o material.</p></section>';
		}
		c.cores.forEach(function (x) {
			var achou = x.materiais, fora = x.confirmado && achou.indexOf(x.confirmado) < 0;
			h += '<article class="fw-cor' + (x.confirmado && !fora ? " ok" : "") + (!achou.length || fora ? " pend" : "") + '">' + foto(x.foto, x.hex) + '<div class="fw-cor-txt">';
			if (!achou.length) h += '<p class="fw-q">Não temos a cor ' + E(x.nome) + " nas impressoras.</p><small>Usado em: " + E(x.pecas.join(", ")) + ". Aguardo você trocar o filamento numa impressora, ou avanço a impressão sem essa cor.</small>";
			else if (achou.length == 1) h += '<p class="fw-q">Achei a cor ' + E(x.nome) + " na impressora " + E(T.ou(T.unicos(x.achou.map(function (a) { return a.impressora; })))) + " em " + E(achou[0]) + ". Confirma?</p><small>" +
				x.achou.map(function (a) { return E(a.impressora) + (a.restante_g != null ? ": " + g(a.restante_g) : ""); }).join(" · ") + " · Usado em: " + E(x.pecas.join(", ")) + "</small>";
			else h += '<p class="fw-q">Achei a cor ' + E(x.nome) + " em " + achou.length + " materiais. Qual você quer?</p><small>" + achou.map(function (mt) { return E(mt) + " na impressora " + E(T.ou(T.unicos(x.achou.filter(function (a) { return a.material == mt; }).map(function (a) { return a.impressora; })))); }).join(" · ") + " · Usado em: " + E(x.pecas.join(", ")) + "</small>";
			h += '<div class="fw-mats">' + achou.map(function (mt) { return '<label class="fw-op"><input type="radio" form="fw-cores" name="mat_' + E(x.chave) + '" value="' + E(mt) + '"' + (x.confirmado == mt ? " checked" : "") + " required><span>" + (achou.length == 1 ? "Confirmar " + E(mt) : E(mt)) + "</span></label>"; }).join("") + "</div>";
			x.correcoes.filter(function (k) { return k.tipo == "filamento"; }).forEach(function (k) { h += '<div class="fw-alerta"><b>' + E(k.titulo) + ".</b> " + E(k.porque) + " <span>" + E(k.como.charAt(0).toUpperCase() + k.como.slice(1)) + "</span>" + T.acao_correcao(k.acao, volta) + "</div>"; });
			h += '<details class="fw-outro"' + (!achou.length || fora ? " open" : "") + "><summary>" + (!achou.length ? "Escolher o material" : "Quero outro material") + '</summary><p class="fw-o-tit">1. Qual material em ' + E(x.nome) + '?</p><div class="fw-mats">' +
				W.MATERIAIS.filter(function (mt) { return achou.indexOf(mt) < 0; }).map(function (mt) { var n = x.estoque[mt]; return '<label class="fw-op"><input type="radio" form="fw-cores" name="mat_' + E(x.chave) + '" value="' + mt + '"' + (x.confirmado == mt ? " checked" : "") + " required><span>" + mt + " <em>" + (n ? n + (n > 1 ? " rolos" : " rolo") + " no estoque" : "sem estoque") + "</em></span></label>"; }).join("") +
				'</div><p class="fw-o-tit">2. Em qual impressora eu troco o filamento?</p><label class="campo fw-imp-sel"><select name="troca_' + E(x.chave) + '" form="fw-cores"><option value="">O Filuna escolhe a mais livre</option>' +
				c.imps.filter(function (i) { return i.slots && i.slots.length; }).map(function (i) { return '<option value="' + E(i.id) + '"' + ((r.troca || {})[x.hex] == i.id ? " selected" : "") + ">" + E(i.nome) + " · hoje: " + i.slots.map(function (s) { return s.material ? E(s.material + " " + (s.cor_nome || "")) : "vazia"; }).join(", ") + "</option>"; }).join("") +
				'</select></label><p class="fw-o-tit">3. Enquanto isso, o que eu faço?</p><div class="fw-mats"><label class="fw-op"><input type="radio" form="fw-cores" name="espera_' + E(x.chave) + '" value="aguardar"' + (x.espera ? " checked" : "") + '><span>Aguardar a troca</span></label><label class="fw-op"><input type="radio" form="fw-cores" name="espera_' + E(x.chave) + '" value="avancar"' + (x.espera ? "" : " checked") + "><span>Avançar sem esta cor</span></label></div>" +
				'<p class="fm-nota">Aguardar: o pedido só vai para a fila depois da troca. Avançar: o resto segue, e as peças em ' + E(x.nome) + " ficam no projeto como pendentes, sem perder nada, até a cor estar numa impressora.</p></details></div></article>";
		});
		h += '<p class="fm-nota">Impressoras desligadas ficam fora da busca. A troca de filamento é registrada em <a href="/farm/impressoras">Impressoras</a> ou pelo Filuna Slicer, e o pedido segue sozinho.</p><div class="fw-rodape"><a class="btn btn-line" href="/farm/pedidos/' + E(ped.id) + '?passo=2">Voltar</a><button class="btn btn-mag" form="fw-cores">Montar o plano <span class="seta">→</span></button></div>';
	}
	var pl = c.pl;
	if (passo == 4) {
		if (!pl) h += '<section class="bloco"><p class="fm-passo">Etapa 4 de 6</p><h2 class="fm-h2">Montando o plano…</h2><p class="fw-sub">Assim que a medida terminar, o plano aparece aqui.</p></section>';
		else {
			h += '<section class="bloco"><p class="fm-passo">Etapa 4 de 6</p><h2 class="fm-h2">Como você quer imprimir?</h2><p class="fw-sub">Olhei as impressoras agora e montei as opções. A recomendada é a mais rápida sem falta de filamento.</p><form method="post" id="fw-plano"><input type="hidden" name="acao" value="responder"><input type="hidden" name="passo" value="4">';
			var op = c.opcoes || {};
			if (Object.keys(op).length) {
				var rec = (!op.juntas || (op.separadas && op.separadas.prazo_s <= op.juntas.prazo_s)) ? "separadas" : "juntas";
				h += '<div class="fw-opcoes">';
				[["separadas", "Uma cor por impressora", "Cada cor na impressora que já está com ela, ao mesmo tempo. Sem purga; as peças saem soltas para montar."], ["juntas", "Todas as cores na mesma mesa", "Numa impressora que troca de cor. Sai montado, mas gasta filamento em cada troca."]].forEach(function (m) {
					var o = op[m[0]];
					h += '<label class="fw-opcao"><input type="radio" name="modo_cores" value="' + m[0] + '"' + (pl.modo == m[0] ? " checked" : "") + " data-fw-auto><span>" + (m[0] == rec ? '<em class="pilula ok">Recomendado</em>' : "") + "<b>" + m[1] + "</b><small>" + m[2] + "</small>" +
						(o ? '<span class="fw-kpis"><span><small>Tempo</small><b>' + (o.prazo_s ? horas(o.prazo_s) : "a medir") + "</b></span><span><small>Custo</small><b>" + brl(o.custo) + "</b></span><span><small>Impressoras</small><b>" + o.impressoras + "</b></span></span>" +
							(o.purga_g ? '<span class="fw-tag warn">' + g(o.purga_g) + " de purga</span>" : '<span class="fw-tag ok">Sem purga</span>') + (o.pendentes ? '<span class="fw-tag warn">' + o.pendentes + " correç" + (o.pendentes != 1 ? "ões" : "ão") + "</span>" : "")
							: '<span class="fw-tag warn">Sem impressora que troque de cor</span>') + "</span></label>";
				});
				h += "</div>" + pq(c.exp, "modo_cores");
			}
			h += '<h3 class="fm-h3">O que pesa mais?</h3><div class="fm-ops">' + [["prazo", "Entregar mais rápido"], ["filamento", "Usar a bobina mais cheia"], ["impressoras", "Usar menos impressoras"]].map(function (o) { return '<label class="fm-op"><input type="radio" name="prioridade" value="' + o[0] + '"' + ((r.prioridade || "prazo") == o[0] ? " checked" : "") + " data-fw-auto><span>" + o[1] + "</span></label>"; }).join("") + "</div>" + pq(c.exp, "prioridade") + "</form></section>";
			h += '<section class="bloco"><div class="fm-tit"><h2>Linha do tempo</h2><span>' + pl.resumo.mesas + " mesa" + pl_(pl.resumo.mesas, "", "s") + " · " + (pl.resumo.prazo_s ? horas(pl.resumo.prazo_s) : "tempo a medir") + "</span></div>" + (c.linha.lanes.length ? T.linha(c.linha) : '<p class="vazio">Nenhuma mesa tem impressora ainda. Veja o que precisa ser corrigido abaixo.</p>') + "</section>";
			if (c.msgs.length) h += '<section class="bloco"><h2 class="fw-h">Como vai ser</h2><div class="fw-msgs">' + c.msgs.map(function (t) { return '<p><span class="fw-av">F</span><span>' + E(t) + "</span></p>"; }).join("") + "</div></section>";
			h += '<section class="bloco"><h2 class="fw-h">O que precisa ser corrigido</h2>';
			pl.correcoes.forEach(function (k) { h += '<div class="fm-corr ' + (k.resolvido || !k.precisa_agir ? "ok" : "") + '"><div class="fm-corr-cab"><b>' + E(k.titulo) + "</b><span>" + E(k.rotulo) + "</span></div><p><b>Por quê:</b> " + E(k.porque) + "</p><p><b>Como corrigir:</b> " + E(k.como) + "</p><p><b>Resultado:</b> " + E(k.resultado) + "</p>" + T.acao_correcao(k.acao, volta) + "</div>"; });
			if (!pl.correcoes.length) h += '<p class="vazio">Nada para corrigir: todas as cores estão em impressoras e há filamento para o pedido inteiro.</p>';
			(pl.sugestoes || []).forEach(function (s) { h += '<div class="fm-aviso violeta"><b>Sugestão</b><p>' + E(s.texto.charAt(0).toUpperCase() + s.texto.slice(1)) + "</p>" + T.acao_correcao(s.acao, volta) + "</div>"; });
			Object.keys(c.erros_fatia || {}).forEach(function (a) { h += '<p class="fm-nota">Sem medida para ' + E(a) + ": " + E(c.erros_fatia[a]) + "</p>"; });
			h += '</section><div class="fw-rodape"><a class="btn btn-line" href="/farm/pedidos/' + E(ped.id) + '?passo=3">Voltar</a><a class="btn btn-mag" href="/farm/pedidos/' + E(ped.id) + '?passo=5">Continuar <span class="seta">→</span></a></div>';
		}
	}
	if (passo == 5) {
		if (!pl) h += '<section class="bloco"><p class="fm-passo">Etapa 5 de 6</p><h2 class="fm-h2">Montando o plano…</h2></section>';
		else {
			h += '<div class="fw-duas"><section class="bloco"><p class="fm-passo">Etapa 5 de 6</p><h2 class="fm-h2">Custo e envio</h2><div class="fm-rola"><table class="tab"><tr><th>Impressora</th><th>Peças</th><th>Filamento</th><th>Tempo</th><th>Custo</th></tr>';
			c.por_imp.forEach(function (y) { h += "<tr><td><b>" + E(y.nome) + "</b></td><td>" + Object.keys(y.pecas).map(function (k) { return E(k) + " ×" + y.pecas[k]; }).join(", ") + "</td><td>" + (y.gramas ? g(y.gramas) : "a medir") + "</td><td>" + (y.tempo_s ? horas(y.tempo_s) : "a medir") + "</td><td>" + brl(y.custo) + "</td></tr>"; });
			c.pendentes.forEach(function (z) { h += '<tr class="fw-pend"><td><span class="pilula warn">Pendente</span></td><td>' + E(z.peca) + " ×" + z.quantidade + '</td><td colspan="3">' + E(z.material + " " + z.cor_nome) + ": fica no projeto e entra na fila quando a cor estiver numa impressora</td></tr>"; });
			h += '</table></div><p class="fm-nota">Custo = filamento pelo preço da bobina (ou do material) e a purga das trocas de cor. Tempo e gramas vêm do fatiamento real' + (c.local ? ", feito aqui no Filuna Slicer com os perfis da impressora." : "; o Filuna Slicer atualiza ao fatiar cada mesa.") + "</p></section>";
			var res = pl.resumo;
			h += '<aside class="bloco fw-total"><span class="mono">Fica pronto em</span><b class="fw-grande">' + (res.prazo_s ? horas(res.prazo_s) : "a medir") + '</b><p class="fm-linha"><span>Peças</span><b>' + res.pecas + " em " + res.mesas + " mesa" + pl_(res.mesas, "", "s") + '</b></p><p class="fm-linha"><span>Filamento</span><b>' + (res.gramas ? g(res.gramas) : "a medir") + (res.purga_g ? " + " + g(res.purga_g) + " de purga" : "") +
				'</b></p><p class="fm-linha"><span>Impressoras</span><b>' + res.impressoras + '</b></p><p class="fm-linha grande"><span>Custo</span><b>' + brl(res.custo) + '</b></p><p class="fm-linha"><span>Por conjunto</span><b>' + brl(res.custo / (r.conjuntos || 1)) + "</b></p>";
			if (c.aguardando.length) h += '<div class="fm-aviso laranja"><b>Aguardando a troca de ' + E(c.aguardando.join(", ")) + '</b><p>Você pediu para esperar. Registre a troca em Impressoras (ou pelo Filuna Slicer) ou volte à etapa 3 e escolha "Avançar sem esta cor".</p></div><button class="btn btn-mag btn-block" disabled>Aguardando a troca de filamento</button>';
			else h += (res.pendentes ? '<div class="fm-aviso laranja"><b>' + res.pendentes + " correç" + (res.pendentes != 1 ? "ões" : "ão") + " pendente" + pl_(res.pendentes, "", "s") + "</b><p>As mesas com pendência ficam esperando na fila e são liberadas sozinhas quando você corrigir.</p></div>" : "") + botao("enviar", c.local ? "Enviar para impressão · " + brl(res.custo) : "Enviar para a fila · " + brl(res.custo), {}, "btn btn-mag btn-block");
			h += botao("cancelar", c.local ? "Cancelar este projeto" : "Cancelar pedido e apagar o arquivo", {}, "btn btn-line btn-sm btn-block") + '</aside></div><div class="fw-rodape"><a class="btn btn-line" href="/farm/pedidos/' + E(ped.id) + '?passo=4">Voltar</a><span></span></div>';
		}
	}
	if (passo == 6) h += vAcompanhar(c);
	return h + "</div></div>";
}

function vAcompanhar(c) {
	var ped = c.ped, v = c.vivo, volta = "/farm/pedidos/" + ped.id + "?passo=6", h = '<div><div class="fm-kpis"><div class="fm-kpi"><span>Pedido #' + E(ped.numero) + "</span><b>" + v.pct + '%</b><div class="fm-barra"><i style="width:' + v.pct + '%"></i></div></div>' +
		'<div class="fm-kpi"><span>Tempo restante</span><b>' + (v.resta_s ? horas(v.resta_s) : "—") + "</b><small>" + E({ na_fila: "na fila", concluido: "concluído", cancelado: "cancelado" }[ped.estado] || "") + "</small></div>" +
		'<div class="fm-kpi"><span>Peças prontas</span><b>' + v.prontas + "/" + v.pecas + "</b>" + (v.pendentes ? '<small class="warn">' + v.pendentes + " pendente" + pl_(v.pendentes, "", "s") + "</small>" : "") + "</div>" +
		'<div class="fm-kpi"><span>Precisa de você</span><b>' + v.avisos.length + "</b><small>aviso" + pl_(v.avisos.length, "", "s") + "</small></div></div>";
	var rg = c.resultado_grupo;
	if (rg) h += '<div class="alerta ' + (rg.liberadas ? "ok" : "erro") + '">' + (rg.liberadas ? rg.liberadas + " de " + rg.total + " mesa" + pl_(rg.total, "", "s") + " entr" + (rg.liberadas != 1 ? "aram" : "ou") + " na fila da " + E(rg.impressora) + ". Com o Filuna Slicer ligado, a impressão começa sozinha quando ela estiver livre." : "As mesas continuam paradas: a " + E(rg.impressora) + " não está com essa cor e material, ou a peça não cabe na mesa dela.") + "</div>";
	if ((c.grupos || []).length) {
		h += '<section class="bloco" id="parados"><div class="fm-tit"><h2>Peças esperando para imprimir</h2><span>separadas por cor</span></div><div class="fd-grupos">';
		c.grupos.forEach(function (gr) { h += '<div class="fd-grupo">' + foto(gr.foto, gr.hex) + '<div class="fd-grupo-txt"><b>' + cor(gr.hex) + " " + E(gr.material + " " + gr.nome) + " · " + gr.quantidade + " peça" + pl_(gr.quantidade, "", "s") + "</b><small>" + E(gr.pecas.join(", ")) + '</small><small class="fd-warn">' + E((c.bloqueios || {})[gr.bloqueio] || "Aguardando correção") + '</small></div><a class="btn btn-mag btn-sm" href="/farm/pedidos/' + E(ped.id) + "?passo=6&grupo=" + E(gr.chave) + '">Imprimir agora</a></div>'; });
		h += "</div></section>";
		c.grupos.filter(function (gr) { return gr.chave == c.grupo_aberto; }).forEach(function (gr) {
			h += '<div class="fw-modal-fundo"><div class="fw-modal" role="dialog" aria-modal="true"><div class="fw-modal-cab">' + foto(gr.foto, gr.hex) + "<div><h2>Imprimir agora: " + E(gr.material + " " + gr.nome) + '</h2><p class="fw-sub">Consultei as impressoras agora. ' + gr.quantidade + " peça" + pl_(gr.quantidade, "", "s") + ": " + E(gr.pecas.join(", ")) + ".</p></div></div>";
			if ((gr.candidatas || []).length) h += '<form method="post"><input type="hidden" name="acao" value="grupo_imprimir"><input type="hidden" name="cor" value="' + E(gr.hex) + '"><input type="hidden" name="material" value="' + E(gr.material) + '"><p class="fw-o-tit">Achei ' + E(gr.material + " " + gr.nome) + ' em</p><div class="fw-mats">' +
				gr.candidatas.map(function (k, i) { return '<label class="fw-op"><input type="radio" name="impressora_id" value="' + E(k.id) + '"' + (i == 0 ? " checked" : "") + " required><span>" + E(k.nome) + " <em>" + E(((c.estados || {})[k.estado] || k.estado || "").toLowerCase()) + " · posição " + k.posicao + "</em></span></label>"; }).join("") +
				'</div><div class="fw-modal-acoes"><button class="btn btn-mag">Imprimir</button></div><p class="fm-nota">As mesas entram na fila dessa impressora e começam sozinhas quando ela estiver livre.</p></form>';
			else h += '<div class="fm-aviso laranja"><b>Nenhuma impressora está com ' + E(gr.material + " " + gr.nome) + ' agora.</b><p>Coloque a bobina numa impressora (o Filuna Slicer informa sozinho) e clique em "Imprimir agora" de novo, ou troque a cor abaixo.</p><div class="fm-acoes"><a class="btn btn-ink btn-sm" href="/farm/impressoras">Colocar a bobina</a></div></div>';
			if ((gr.outras || []).length) h += '<form method="post" class="fw-modal-troca"><input type="hidden" name="acao" value="grupo_trocar"><input type="hidden" name="cor" value="' + E(gr.hex) + '"><input type="hidden" name="material" value="' + E(gr.material) + '"><p class="fw-o-tit">Ou trocar a cor dessas peças por uma que já está nas impressoras</p><div class="fw-mats">' +
				gr.outras.map(function (o) { return '<label class="fw-op"><input type="radio" name="slot" value="' + E(o.impressora_id + ":" + o.posicao) + '" required><span>' + cor(o.hex) + E(o.material + " " + o.nome) + " <em>" + E(o.impressora) + " · posição " + o.posicao + "</em></span></label>"; }).join("") +
				'</div><div class="fw-modal-acoes"><button class="btn btn-ink">Trocar a cor e imprimir</button></div><p class="fm-nota">O projeto é atualizado: essas peças passam a sair na cor escolhida, na impressora dessa bobina.</p></form>';
			h += '<div class="fw-modal-acoes"><a class="btn btn-line btn-sm" href="/farm/pedidos/' + E(ped.id) + '?passo=6">Fechar</a></div></div></div>';
		});
	}
	h += '<div class="fm-duas"><div class="fm-principal">' + (c.linha && c.linha.lanes.length ? '<section class="bloco"><div class="fm-tit"><h2>Linha do tempo</h2><span>a partir de agora</span></div>' + T.linha(c.linha, true) + "</section>" : "") +
		'<section class="bloco"><h2 class="fw-h">Mesas deste pedido</h2><div class="fm-rola"><table class="tab"><tr><th>Peça</th><th>Mesa</th><th>Impressora</th><th>Filamento</th><th>Tempo</th><th>Estado</th></tr>';
	(c.trabalhos || []).forEach(function (t) { h += '<tr><td><span class="fm-peca">' + foto(t.peca_id, t.cor_hex, true) + "<span><b>" + E(t.descricao) + " ×" + t.quantidade + "</b><small>" + E(t.material + " " + (t.cor_nome || "")) + "</small></span></span></td><td>" + t.numero + " de " + t.total + "</td><td>" + (t.impressora_id ? '<a href="/farm/impressoras/' + E(t.impressora_id) + '">' + E(t.imp_nome) + "</a>" : "—") + "</td><td>" + (t.gramas ? g(t.gramas) : "a medir") + "</td><td>" + (t.tempo_s ? horas(t.tempo_s) : "a medir") + "</td><td>" + T.estado_trab(t, c.bloqueios) + "</td></tr>"; });
	h += "</table></div>" + (ped.estado == "na_fila" ? '<div style="margin-top:12px">' + botao("cancelar", "Cancelar as mesas que ainda não começaram") + "</div>" : "") + '</section><div class="fm-grade">';
	(v.imps || []).forEach(function (i) {
		var t = i.trabalho;
		h += '<article class="fm-imp"><div class="fm-imp-cab"><div><b>' + E(i.nome) + "</b><small>" + E(i.modelo_nome || "Impressora") + "</small></div>" + T.estado_imp(i, c.estados) + "</div>" +
			(t ? '<div class="fm-job">' + foto(t.peca_id, t.cor_hex, true) + "<div><b>" + E(t.descricao) + "</b><small>mesa " + t.numero + " de " + t.total + (t.estado == "imprimindo" ? " · " + Math.round(t.progresso || 0) + "%" : "") + "</small></div></div>" + (t.estado == "imprimindo" ? '<div class="fm-barra"><i style="width:' + (t.progresso || 0) + '%"></i></div>' : "") : "") +
			'<div class="fm-slots">' + (i.slots || []).map(function (s) { return '<div class="fm-slot">' + cor(s.cor_hex) + "<div><b>" + E(s.material ? s.material + " " + (s.cor_nome || "") : "Sem bobina") + "</b><small>" + (s.bobina_id ? "restam " + g(s.restante_g) : "posição " + s.posicao) + "</small></div></div>"; }).join("") + "</div></article>";
	});
	h += '</div></div><aside class="fm-lado"><section class="bloco"><h2 class="fw-h">Avisos</h2>';
	v.avisos.forEach(function (a) {
		h += '<div class="fm-aviso ' + E(a.tom || "") + '"><b>' + E(a.titulo) + "</b><p>" + E(a.texto) + '</p><div class="fm-acoes">' +
			(a.tipo == "limpar" ? botao("mesa_limpa", "Mesa limpa, continuar", { impressora_id: a.impressora_id }, "btn btn-ink btn-sm", volta) : a.tipo == "falhou" ? botao("repetir", "Repetir esta mesa", { trabalho_id: (a.trabalho || {}).id }, "btn btn-ink btn-sm", volta)
				: a.tipo == "falta_cor" && a.pedido_id ? '<a class="btn btn-ink btn-sm" href="/farm/pedidos/' + E(a.pedido_id) + "?passo=6&grupo=" + E(((a.trabalho || {}).cor_hex || "#000000").replace("#", "").toUpperCase()) + '">Imprimir agora</a>'
				: a.impressora_id ? '<a class="btn btn-ink btn-sm" href="/farm/impressoras/' + E(a.impressora_id) + '">Trocar bobina</a>' : "") + "</div></div>";
	});
	if (!v.avisos.length) h += '<p class="vazio">Nada precisa de você agora.</p>';
	return h + '</section><a class="btn btn-line btn-block" href="/farm/novo">Novo pedido</a></aside></div></div>';
}

// ---------------- projeto local: o mesmo contexto que o site monta (rota pedido do farm.py) ----------------
function planoLocal(L, r) {
	r = r || L.r;
	if (!r.material) return null;
	var pcs = L.pecas.map(function (p) { var q = Object.assign({}, p); if ((!q.cores || !q.cores.length) && q.cor) q.cores = [q.cor]; return q; });
	var D = st.dados, mats = D.materiais || {}, precos = {}, dens = {};
	Object.keys(mats).forEach(function (m) { precos[m] = mats[m].preco_kg; dens[m] = mats[m].densidade; });
	var f = L.fatias || {}, base = f.material || r.material;
	r = Object.assign({}, r, { _base: base });
	var fat = {};
	if (f.material == base) Object.keys(f.arquivos || {}).forEach(function (k) { if ("tempo_s" in f.arquivos[k]) fat[k] = f.arquivos[k]; });
	return FP.planejar(pcs, r, D.impressoras, D.bobinas, fat, D.params || {}, precos, dens);
}

function ctxLocal(L, passoPedido, falta_cor) {
	var D = st.dados, r = L.r, pcs = L.pecas, bobs = D.bobinas, imps = D.impressoras;
	var confirmado = !!r.materiais || !r.wizard;
	var alcancado = confirmado && (L.estado == "plano" || L.estado == "calculando") ? 5 : 3;
	var passo = Math.max(2, Math.min(passoPedido || (confirmado && r.materiais ? 4 : 2), alcancado));
	var cores = W.cores_do_pedido(pcs, imps, bobs, r, null), r_prev = r, pl = null;
	if (passo == 3) {
		var prev = {}; cores.forEach(function (x) { prev[x.hex] = x.confirmado || (x.materiais[0] || r.material || "PLA"); });
		r_prev = Object.assign({}, r, { materiais: prev });
	}
	if (L.estado == "plano") pl = planoLocal(L, r_prev);
	if (passo == 3) { cores = W.cores_do_pedido(pcs, imps, bobs, r, pl); pl = null; }
	else if (pl) cores = W.cores_do_pedido(pcs, imps, bobs, r, pl);
	var cores_estoque = [];
	bobs.forEach(function (b) { if (!cores_estoque.some(function (x) { return x.hex == b.cor_hex; })) cores_estoque.push({ hex: b.cor_hex, nome: b.cor_nome || FP.nome_cor(b.cor_hex), estoque: true }); });
	FP.CORES_NOME.forEach(function (cn) { if (!cores_estoque.some(function (x) { return FP.distancia_cor(x.hex, cn[1]) < 25; })) cores_estoque.push({ hex: cn[1], nome: cn[0], estoque: false }); });
	var pendNomes = cores.filter(function (x) { return !x.materiais.length || (x.confirmado && x.materiais.indexOf(x.confirmado) < 0); }).map(function (x) { return x.nome; });
	var c = { local: true, ped: { id: "local", numero: "", nome: L.nome, arquivo: L.arquivo, estado: L.estado, progresso: L.progresso }, r: r, passo: passo, alcancado: alcancado, enviado: false,
		pecas: pcs, n_cores: (function () { var s = []; pcs.forEach(function (p) { ((p.cores && p.cores.length) ? p.cores : [p.cor || p.id]).forEach(function (x) { if (s.indexOf(x) < 0) s.push(x); }); }); return s.length; })(),
		encaixe: W.encaixe(pcs, imps, r.conjuntos || 1), cores: cores, cores_estoque: cores_estoque, imps: imps, pl: pl, falta_cor: falta_cor,
		exp: pl ? W.explicacoes(pl, r, imps.length) : {}, erros_fatia: L.erros_fatia || {}, medida_erro: L.medida_erro, estados: D.estados, bloqueios: D.bloqueios };
	if (pl) {
		c.linha = W.linha_do_tempo(pl, imps);
		c.msgs = W.mensagens(pl, cores.length, cores.filter(function (x) { return !x.espera && pendNomes.indexOf(x.nome) >= 0; }).map(function (x) { return x.nome; }));
		c.aguardando = W.aguardando(pl, r);
		var por = {}, ordem = [], tot = pl.mesas.filter(function (x) { return !x.bloqueio; }).reduce(function (a, x) { return a + x.gramas; }, 0) || 1;
		pl.mesas.forEach(function (x) {
			if (x.bloqueio == "falta_cor" || !x.impressora_id) return;
			if (!(x.impressora in por)) { por[x.impressora] = { nome: x.impressora, tempo_s: 0, gramas: 0, pecas: {} }; ordem.push(x.impressora); }
			var y = por[x.impressora]; y.tempo_s += x.tempo_s || 0; y.gramas += x.gramas || 0; y.pecas[x.peca] = (y.pecas[x.peca] || 0) + x.quantidade;
		});
		c.por_imp = ordem.map(function (k) { var y = por[k]; y.custo = pl.resumo.custo * y.gramas / tot; return y; });
		var pend = {}, po = [];
		pl.mesas.forEach(function (x) { if (x.bloqueio == "falta_cor" || !x.impressora_id) { if (!(x.peca in pend)) { pend[x.peca] = { peca: x.peca, peca_id: x.peca_id, cor_nome: x.cor_nome, material: x.material, quantidade: 0 }; po.push(x.peca); } pend[x.peca].quantidade += x.quantidade; } });
		c.pendentes = po.map(function (k) { return pend[k]; });
		c.opcoes = {};
		if (cores.length > 1) ["separadas", "juntas"].forEach(function (modo) { var alt = planoLocal(L, Object.assign({}, r, { modo_cores: modo })); c.opcoes[modo] = alt ? alt.resumo : null; });
	}
	return c;
}

function medir(L) {
	L.estado = "calculando"; L.progresso = "Fatiando no Filuna Slicer"; L.medida_erro = null;
	Filuna.nativo("farm.medir", { projeto: L.projeto, material: L.r.material || "PLA", arquivos: L.modelos }).catch(function (e) { L.estado = "plano"; L.medida_erro = (e && (e.erro || e.message)) || "Não foi possível medir."; desenhar(); });
}

function acaoLocal(L, f, passoForm) {
	var a = f.acao;
	if (a == "salvar") { L.r = W.respostas_do_form(f, L.r); return "/farm/pedidos/local?passo=" + (passoForm || 3); }
	if (a == "responder") {
		L.r = W.respostas_do_form(f, L.r);
		var falta = false;
		L.pecas.forEach(function (p) { var c = (f["cor_" + p.id] || "").trim(); if ((!p.cores || !p.cores.length) && /^#[0-9a-fA-F]{6}$/.test(c)) p.cor = c.toUpperCase(); else if ((!p.cores || !p.cores.length) && !p.cor) falta = true; });
		if (falta) return "/farm/pedidos/local?passo=3&falta_cor=1";
		return "/farm/pedidos/local?passo=" + (passoForm || 4);
	}
	if (a == "cancelar") { st.local = null; return "/farm"; }
	if (a == "enviar") {
		var pl = planoLocal(L);
		if (pl && W.aguardando(pl, L.r).length) return "/farm/pedidos/local?passo=5";
		if (!pl || !pl.mesas.length) return "/farm/pedidos/local?passo=5";
		L.enviando = true;
		var pecas = L.pecas.map(function (p) { return Object.assign({}, p, { foto: L.fotosB64[p.id] || null }); });
		Filuna.nativo("farm.salvar", { projeto: L.projeto, dados: { nome: L.nome, arquivo: L.arquivo, tamanho: L.tamanho, analise: { arquivos: L.analise.arquivos, modelos: L.modelos, placas_arquivo: L.analise.placas_arquivo, pecas: pecas },
			respostas: L.r, fatias: L.fatias || {}, mesas: pl.mesas } }).catch(function (e) { L.enviando = false; aviso((e && (e.erro || e.message)) || "Não foi possível enviar."); });
		return null;
	}
	return null;
}

// ---------------- navegação, dados e formulários ----------------
var ultimoPedido = null;
function aviso(t) { var d = document.createElement("div"); d.className = "alerta erro"; d.textContent = t; tela.insertBefore(d, tela.firstChild); }
function servidor(metodo, caminho, corpo) { return Filuna.api(metodo, caminho.replace(/^\/farm/, "/farm"), corpo); }
function dados() { return Filuna.api("GET", "/farm-dados").then(function (d) { st.dados = d; return d; }); }

function ir(caminho) { st.rota = caminho; desenhar(); window.scrollTo(0, 0); }
function topo() {
	var r = st.rota, itens = [["/farm", "Painel"], ["/farm/novo", "Novo projeto"], ["/farm/impressoras", "Impressoras"], ["/farm/estoque", "Estoque"]];
	document.getElementById("topo").innerHTML = itens.map(function (i) { return '<a href="' + i[0] + '" class="' + (r.split("?")[0] == i[0] || (i[0] == "/farm/novo" && /local/.test(r)) ? "on" : "") + '">' + i[1] + "</a>"; }).join("") +
		'<span class="fl-local">● Rodando no Filuna Slicer</span>';
}
function desenhar() {
	clearTimeout(st.recarga); topo();
	var rota = st.rota, q = {}, base = rota.split("?")[0];
	(rota.split("?")[1] || "").split("&").forEach(function (p) { if (p) { var kv = p.split("="); q[decodeURIComponent(kv[0])] = decodeURIComponent(kv[1] || ""); } });
	if (!Filuna.temToken()) { Filuna.telaLogin(tela, desenhar, "Entre na sua conta Filuna para usar a Farm."); return; }
	if (base == "/farm/novo") {
		if (!st.local || st.local.salvo) st.local = { etapa1: true, r: Object.assign({}, W.RESPOSTAS_PADRAO), de: null };
		var L = st.local;
		var mostrar = function () { tela.innerHTML = vNovo({ arquivo: L.arquivoEscolhido, nome: L.nome, de: L.de, erro: L.erro, analisando: L.analisando }); fotos(); };
		if (q.de && (!L.de || L.de.id != q.de)) servidor("GET", "/farm/pedidos/" + q.de).then(function (d) {
			var velho = JSON.parse((d.ped || {}).respostas_json || "{}"); L.de = d.ped;
			["conjuntos", "iguais", "modo_cores", "prioridade", "material", "materiais", "troca"].forEach(function (k) { if (k in velho) L.r[k] = velho[k]; });
			L.r.reimpressao_de = d.ped.numero; mostrar();
		}).catch(mostrar);
		(st.dados ? Promise.resolve() : dados()).then(mostrar, mostrar);
		return;
	}
	var m = base.match(/^\/farm\/pedidos\/([A-Za-z0-9_-]+)$/);
	if (m && m[1] == "local") {
		var L2 = st.local;
		if (!L2 || !L2.pecas) { ir("/farm/novo"); return; }
		dados().then(function () {
			var c = ctxLocal(L2, parseInt(q.passo || "0", 10) || 0, !!q.falta_cor);
			st.ctx = c; tela.innerHTML = vPedido(c); fotos(); extras();
		}).catch(function (e) { tela.innerHTML = '<div class="alerta erro">' + E(e.erro || "Sem conexão.") + "</div>"; });
		return;
	}
	var api = base == "/farm" ? "/farm/painel" : base;
	servidor("GET", api + (rota.indexOf("?") >= 0 ? "?" + rota.split("?")[1] : "")).then(function (d) {
		if (d.ir) { ir(d.ir); return; }
		st.ctx = d;
		var t = d.tela || "";
		tela.innerHTML = t == "farm/painel.html" ? T.vPainel(d) : t == "farm/pedido.html" ? vPedido(d) : t == "farm/impressoras.html" ? T.vImpressoras(d) : t == "farm/impressora.html" ? T.vImpressora(d) : t == "farm/estoque.html" ? T.vEstoque(d) : '<div class="vazio">Tela não encontrada.</div>';
		fotos(); extras();
		var seg = t == "farm/painel.html" ? 30 : t == "farm/impressora.html" ? 20 : t == "farm/pedido.html" && d.passo == 6 && d.ped.estado == "na_fila" ? 30 : t == "farm/pedido.html" && d.ped.estado == "calculando" ? 4 : 0;
		if (seg) st.recarga = setTimeout(function () { var a = document.activeElement; if (!document.hidden && !(a && /^(INPUT|SELECT|TEXTAREA)$/.test(a.tagName))) desenhar(); else st.recarga = setTimeout(desenhar, seg * 1000); }, seg * 1000);
	}).catch(function (e) { tela.innerHTML = '<div class="alerta erro">' + E((e && e.erro) || "Sem conexão com a sua conta.") + "</div>"; });
}

function fotos() {
	document.querySelectorAll("img[data-foto]").forEach(function (im) {
		var id = im.getAttribute("data-foto");
		if (im.getAttribute("src")) return;
		if (st.fotos[id]) { im.src = st.fotos[id]; return; }
		Filuna.imagem("/farm/foto/" + encodeURIComponent(id) + ".png").then(function (b) { st.fotos[id] = URL.createObjectURL(b); im.src = st.fotos[id]; }).catch(function () { im.parentNode.innerHTML = '<span class="fm-foto-vazia"></span>'; });
	});
}

function extras() {   // pagina-farm.js do site: atalhos de quantidade e plano refeito ao trocar a opção
	var qtd = document.querySelector("[data-fw-qtd]");
	document.querySelectorAll("[data-fw-n]").forEach(function (b) { b.onclick = function () { if (qtd) qtd.value = b.getAttribute("data-fw-n"); document.querySelectorAll("[data-fw-n]").forEach(function (x) { x.classList.toggle("on", x === b); }); }; });
	document.querySelectorAll("[data-fw-auto]").forEach(function (i) { i.onchange = function () { enviarForm(i.form); }; });
}

function camposDe(form, botaoClicado) {
	var f = {};
	Array.prototype.forEach.call(form.elements.length ? form.elements : [], function (el) { lerCampo(el, f); });
	if (form.id) document.querySelectorAll('[form="' + form.id + '"]').forEach(function (el) { lerCampo(el, f); });
	if (botaoClicado && botaoClicado.name) f[botaoClicado.name] = botaoClicado.value;
	return f;
}
function lerCampo(el, f) {
	if (!el.name || el.disabled || el.tagName == "BUTTON") return;
	if ((el.type == "radio" || el.type == "checkbox") && !el.checked) return;
	f[el.name] = el.value;
}

var botaoAtual = null;
function enviarForm(form) {
	var f = camposDe(form, botaoAtual); botaoAtual = null;
	var rota = st.rota.split("?")[0];
	if (form.getAttribute("data-local") == "novo") { analisar(f); return; }
	if (/^\/farm\/pedidos\/local$/.test(rota) && st.local) {
		if (["mesa_limpa", "carregar", "repetir"].indexOf(f.acao) >= 0) {   // correção de bobina: dado do site, o plano é refeito aqui
			form.classList.add("fl-ocupado");
			servidor("POST", "/farm/painel", f).then(function () { desenhar(); }, function (e) { form.classList.remove("fl-ocupado"); aviso(e.erro || "Não foi possível."); });
			return;
		}
		var destino = acaoLocal(st.local, f, f.passo);
		if (destino) ir(destino); else desenhar();
		return;
	}
	form.classList.add("fl-ocupado");
	var alvo = rota == "/farm" ? "/farm/painel" : rota;
	servidor("POST", alvo, f).then(function (d) { if (d.ir) ir(d.ir.indexOf("/farm") == 0 ? d.ir : "/farm"); else desenhar(); }, function (e) { form.classList.remove("fl-ocupado"); aviso((e && e.erro) || "Não foi possível."); });
}

function analisar(f) {
	var L = st.local;
	if (!L || !L.arquivoEscolhido) return;
	L.nome = (f.nome || "").trim() || L.arquivoEscolhido.nome.replace(/\.[^.]+$/, "");
	L.analisando = true; L.erro = null; desenhar();
	Filuna.nativo("farm.analisar", { caminho: L.arquivoEscolhido.caminho }).catch(function (e) { L.analisando = false; L.erro = (e && (e.erro || e.message)) || "Não foi possível abrir."; desenhar(); });
}

Filuna.ouvir("farm.analise", function (d) {
	var L = st.local;
	if (!L || !L.analisando) return;
	L.analisando = false;
	if (!d.ok) { L.erro = d.erro || "Não foi possível ler esse arquivo."; desenhar(); return; }
	L.projeto = d.projeto; L.analise = d; L.modelos = d.modelos || []; L.arquivo = L.arquivoEscolhido.nome; L.tamanho = L.arquivoEscolhido.tamanho;
	L.fotosB64 = {}; L.pecas = (d.pecas || []).map(function (p, i) {
		var id = "p" + i;
		if (p.foto) { L.fotosB64[id] = p.foto; st.fotos[id] = "data:image/png;base64," + p.foto; }
		var q = Object.assign({}, p, { id: id }); delete q.foto; q.cor = q.cor || (q.cores && q.cores[0]) || ""; return q;
	});
	L.estado = "calculando";
	medir(L);
	ir("/farm/pedidos/local?passo=2");
});
Filuna.ouvir("farm.medida", function (d) {
	var L = st.local;
	if (!L || d.projeto != L.projeto) return;
	L.estado = "plano"; L.progresso = "";
	if (d.erro) L.medida_erro = "Tempo e filamento sem medida: " + d.erro + ". O plano segue, e o Slicer mede ao fatiar cada mesa.";
	L.fatias = { material: d.material, arquivos: d.arquivos || {} }; L.erros_fatia = d.erros || {};
	if (/^\/farm\/pedidos\/local/.test(st.rota)) desenhar();
});
Filuna.ouvir("farm.salvo", function (d) {
	var L = st.local;
	if (L) L.enviando = false;
	if (d.erro) { aviso("O projeto não foi salvo: " + d.erro); return; }
	if (L) L.salvo = true;
	ir("/farm/pedidos/" + d.pid + "?passo=6");
});

document.addEventListener("click", function (ev) {
	var b = ev.target.closest ? ev.target.closest("button") : null;
	if (b && b.form) botaoAtual = b;
	var prep = ev.target.closest ? ev.target.closest("[data-preparacao]") : null;
	if (prep) {
		ev.preventDefault();
		Filuna.nativo("farm.projeto_atual").then(function (r) { if (r && r.caminho) { st.local.arquivoEscolhido = r; st.local.erro = null; if (!st.local.nome) st.local.nome = r.nome.replace(/\.3mf$/, ""); desenhar(); } })
			.catch(function (e) { st.local.erro = (e && (e.erro || e.message)) || "Não foi possível."; desenhar(); });
		return;
	}
	var esc = ev.target.closest ? ev.target.closest("[data-escolher]") : null;
	if (esc) {
		ev.preventDefault();
		Filuna.nativo("farm.escolher").then(function (r) { if (r && r.caminho) { st.local.arquivoEscolhido = r; st.local.erro = null; desenhar(); } });
		return;
	}
	var a = ev.target.closest ? ev.target.closest("a[href]") : null;
	if (!a) return;
	var href = a.getAttribute("href");
	if (href.indexOf("/farm") == 0) { ev.preventDefault(); ir(href); }
	else if (href.charAt(0) == "/") { ev.preventDefault(); Filuna.abrirLink("https://filuna3d.com" + href); }
});
document.addEventListener("submit", function (ev) { ev.preventDefault(); enviarForm(ev.target); });

Filuna.iniciar().then(function () { return Filuna.nativo("secao.pendente").then(function (r) { return r.secao; }).catch(function () { return ""; }); })
	.then(function (secao) { if (secao && secao.indexOf("/farm") == 0) st.rota = secao; desenhar(); });
Filuna.ouvir("conta", function () { st.dados = null; desenhar(); });
Filuna.ouvir("secao", function (d) { if (d.secao && d.secao.indexOf("/farm") == 0) { Filuna.nativo("secao.pendente").catch(function () {}); ir(d.secao); } });
Filuna.ouvir("mostrar", function () { if (!/local|novo/.test(st.rota)) desenhar(); });
window.FarmPedido = { vPedido: vPedido, ctxLocal: ctxLocal, planoLocal: planoLocal, acaoLocal: acaoLocal };
})();
