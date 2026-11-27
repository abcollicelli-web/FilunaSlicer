// Teste do tradutor das telas (i18n/traduzir.js) fora do navegador: node scripts/filuna-i18n/testar.js
// Confere frases inteiras, padrões com valores, pedaços e o que NÃO pode mudar (números, nomes, português sem tradução).
"use strict";
const fs = require("fs");
const path = require("path");
const raiz = path.join(__dirname, "..", "..", "resources", "web", "filuna", "i18n");

global.window = global;
global.MutationObserver = class { observe() {} };
global.NodeFilter = { SHOW_ELEMENT: 1, SHOW_TEXT: 4 };
global.document = { documentElement: {}, createTreeWalker: () => ({ nextNode: () => null }), addEventListener() {} };
global.location = { protocol: "file:" };
window.FILUNA_IDIOMA = "en";
window.FILUNA_I18N = JSON.parse(fs.readFileSync(path.join(raiz, "en.json"), "utf8"));
eval(fs.readFileSync(path.join(raiz, "traduzir.js"), "utf8"));

const t = window.FilunaI18n.t;
const casos = [
	["Salvar", "Save"],
	["  Cancelar ", "  Cancel "],
	["Sem conexão — trabalhando no computador", "Offline — working locally"],
	["✦ Assistente de impressão", "✦ Print Assistant"],
	["3 bobinas ativas", "3 active spools"],
	["1 bobina ativa", "1 active spool"],
	["A impressora A1-ASHLEY está desligada ou fora da rede.", "Printer A1-ASHLEY is off or offline."],
	["Etapa 1 de 5 · Peça", "Step 1 of 5 · Part"],
	["Chaveiro do cliente", "Chaveiro do cliente"],   // dado do usuário sem tradução fica igual
	["1.234,56", "1.234,56"],
	["R$ 12,90", "R$ 12,90"],
];
let falhas = 0;
for (const [pt, esperado] of casos) {
	const r = t(pt);
	const ok = r === esperado;
	if (!ok) falhas++;
	console.log((ok ? "ok    " : "FALHOU") + "  " + JSON.stringify(pt) + " -> " + JSON.stringify(r) + (ok ? "" : "  (esperado " + JSON.stringify(esperado) + ")"));
}
// Todo padrão do dicionário precisa compilar e cada frase precisa voltar a própria tradução.
const d = window.FILUNA_I18N;
let ruins = 0;
for (const k of Object.keys(d.frases)) if (d.frases[k] && t(k) !== d.frases[k]) { ruins++; if (ruins <= 10) console.log("frase não volta igual:", k, "->", t(k)); }
console.log(Object.keys(d.frases).length + " frases, " + Object.keys(d.padroes).length + " padrões; " + ruins + " frases que não voltam iguais");
process.exit(falhas || ruins ? 1 : 0);
