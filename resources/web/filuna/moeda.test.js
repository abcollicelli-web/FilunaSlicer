// Confere a formatação de dinheiro das telas Filuna (ponte.js e farm-plano.js) em real e em dólar. Rodar: node resources/web/filuna/moeda.test.js
const fs = require("fs"), vm = require("vm"), assert = require("assert");
const base = __dirname + "/";
const ouv = {};
const ctx = {
	console, setTimeout: () => 0, setInterval: () => 0, location: { pathname: "/x.html", reload() {} }, navigator: { userAgent: "t" },
	localStorage: { getItem: () => null, setItem() {}, removeItem() {} },
	document: { addEventListener() {}, getElementById: () => null, createElement: () => ({ style: {} }), body: { appendChild() {} },
		documentElement: { style: { setProperty() {} } } },
	addEventListener() {}, wx: { postMessage() { throw new Error("fora"); } },
	fetch: () => Promise.reject(new Error("sem rede")), Promise, JSON, Math, Number, String, isFinite, parseFloat,
};
ctx.globalThis = ctx;
ctx.window = ctx;   // no navegador a janela é o objeto global
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(base + "ponte.js", "utf8") + "\nthis.Filuna = Filuna;", ctx);
vm.runInContext(fs.readFileSync(base + "farm-plano.js", "utf8"), ctx);
const F = ctx.Filuna, FP = ctx.FarmPlano;
// Brasil (sem moeda conhecida): igual ao de antes
assert.strictEqual(F.moeda(), "BRL");
assert.strictEqual(F.reais(1234.5), "R$ 1234,50");
assert.strictEqual(F.simbolo(), "R$");
assert.strictEqual(F.valorCampo(12.5), "12,50");
assert.strictEqual(F.lerValor("12,50"), 12.5);
assert.strictEqual(FP.reais(1234.5), "R$ 1.234,50");
// conta dos EUA: o site manda "moeda": "USD" em /resumo e a ponte guarda
ctx.fetch = () => Promise.resolve({ ok: true, json: () => Promise.resolve({ moeda: "USD", conta: {} }) });
(async () => {
	ctx.wx = { postMessage(m) { const p = JSON.parse(m); setImmediate(() => F._resposta(p.id, true, p.acao == "config.ler" ? { valor: "tok" } : {})); } };
	await F.iniciar();
	await F.api("GET", "/resumo");
	assert.strictEqual(F.moeda(), "USD");
	assert.strictEqual(F.reais(1234.5), "$1,234.50");
	assert.strictEqual(F.reais(-3), "-$3.00");
	assert.strictEqual(F.simbolo(), "$");
	assert.strictEqual(F.valorCampo(12.5), "12.50");
	assert.strictEqual(F.lerValor("1,234.56"), 1234.56);
	assert.strictEqual(F.lerValor("$12.50"), 12.5);
	assert.strictEqual(F.lerValor("12,50"), 12.5);
	assert.strictEqual(F.lerValor(""), null);
	assert.strictEqual(FP.reais(1234.5), "$1,234.50");
	console.log("ok: real igual ao de antes; dólar $1,234.50");
})().catch(e => { console.error(e); process.exit(1); });
