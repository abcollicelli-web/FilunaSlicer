/* Filuna — botão Voltar único das telas Filuna (pedido do dono, 28/11/2026: o "‹ Unidades" pequeno passava
 * despercebido). Botão de verdade no topo à esquerda, com a seta e o destino ("Voltar às impressoras").
 *
 *   FilunaVoltar.html("Voltar às impressoras", { id: "voltar" })   -> marcação do botão
 *   FilunaVoltar.html("Voltar ao painel", { href: "/farm" })        -> como link (telas com roteador por href)
 *
 * Esc e o botão "voltar" do mouse acionam o botão Voltar que estiver na tela (o primeiro visível), menos com uma
 * janela por cima ou com o cursor num campo de texto. O estilo vem junto, para valer também nas telas que não usam
 * base.css (Farm, Modelos 3D). */
(function () {
	if (window.FilunaVoltar) return;

	var CSS = ".fl-voltar{display:inline-flex;align-items:center;gap:10px;height:44px;padding:0 20px 0 14px;border-radius:12px;" +
		"border:1px solid #E3DCD2;background:#fff;color:#2C2A28;font-family:Inter,'Segoe UI',system-ui,sans-serif;font-size:15px;" +
		"font-weight:700;line-height:1;cursor:pointer;text-decoration:none;box-shadow:0 1px 2px rgba(60,50,40,.06);white-space:nowrap;" +
		"transition:border-color .15s,background .15s,color .15s;vertical-align:middle;flex:none}" +
		".fl-voltar svg{width:20px;height:20px;flex:none;fill:none;stroke:#E0186B;stroke-width:2.4;stroke-linecap:round;stroke-linejoin:round;" +
		"transition:transform .15s}" +
		".fl-voltar:hover{border-color:#E0186B;background:#FFF4F8;color:#B3125C;text-decoration:none}" +
		".fl-voltar:hover svg{transform:translateX(-2px)}" +
		".fl-voltar:focus-visible{outline:3px solid rgba(224,24,107,.35);outline-offset:2px}" +
		".fl-voltar-linha{display:flex;align-items:center;justify-content:space-between;gap:16px;flex-wrap:wrap}";
	var SETA = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M19 12H5"/><path d="M11 6l-6 6 6 6"/></svg>';

	function poeCss() {
		if (document.getElementById("flVoltarCss")) return;
		var s = document.createElement("style");
		s.id = "flVoltarCss";
		s.textContent = CSS;
		(document.head || document.documentElement).appendChild(s);
	}
	function esc(t) {
		return String(t == null ? "" : t).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; });
	}

	function html(rotulo, op) {
		op = op || {};
		var attrs = ' class="fl-voltar' + (op.classe ? " " + esc(op.classe) : "") + '" data-fl-voltar="1" title="' + esc(rotulo) + '"' +
			(op.id ? ' id="' + esc(op.id) + '"' : "") + (op.style ? ' style="' + esc(op.style) + '"' : "");
		var dentro = SETA + "<span>" + esc(rotulo) + "</span>";
		return op.href ? '<a href="' + esc(op.href) + '"' + attrs + ">" + dentro + "</a>" : '<button type="button"' + attrs + ">" + dentro + "</button>";
	}

	function visivel(el) { return !!(el && el.isConnected && (el.offsetWidth || el.offsetHeight || el.getClientRects().length)); }
	function atual() {
		var l = document.querySelectorAll("[data-fl-voltar]");
		for (var i = 0; i < l.length; i++) if (visivel(l[i]) && !l[i].disabled) return l[i];
		return null;
	}
	// Janela por cima (estoque, preparação, Filuna Home, caixas do sistema): o Esc é dela, não da tela.
	function temJanela() {
		var l = document.querySelectorAll(".estq,.fh-fundo,[role=dialog],[aria-modal=true],dialog[open]");
		for (var i = 0; i < l.length; i++) if (visivel(l[i])) return true;
		return false;
	}
	function emCampo(el) {
		return !!(el && (/^(INPUT|TEXTAREA|SELECT)$/.test(el.tagName) || el.isContentEditable));
	}

	document.addEventListener("keydown", function (e) {
		if (e.key != "Escape" || e.defaultPrevented || e.altKey || e.ctrlKey || e.metaKey || e.shiftKey) return;
		if (emCampo(document.activeElement) || temJanela()) return;
		var b = atual();
		if (!b) return;
		e.preventDefault();
		b.click();
	});
	// Botão "voltar" do mouse (botão 4, e.button == 3). Sem botão Voltar na tela, fica o comportamento normal.
	function mouse(e) {
		if (e.button != 3 || temJanela()) return;
		var b = atual();
		if (!b) return;
		e.preventDefault();
		if (e.type == "mouseup") b.click();
	}
	document.addEventListener("mousedown", mouse, true);
	document.addEventListener("mouseup", mouse, true);
	document.addEventListener("auxclick", function (e) { if (e.button == 3 && atual()) e.preventDefault(); }, true);

	if (document.head) poeCss();
	else document.addEventListener("DOMContentLoaded", poeCss);

	window.FilunaVoltar = { html: html, atual: atual };
})();
