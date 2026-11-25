// Filuna Home: catálogo de plugins. Um plugin é a receita de um pedido HTTP (sem código): o motor de avisos do
// Filuna Slicer (FilunaCasaAvisos.cpp) troca {titulo}, {texto}, {impressora}, {peca}, {erro}, {evento}, {cor} e os
// campos que o cliente preencheu, e manda. "{aviso}" sozinho vira o aviso inteiro em JSON; "{cor_int}" sozinho vira
// a cor como número (Discord). Novos serviços entram aqui sem compilar o programa.
var FilunaCasaPlugins = [
	{
		id: "ntfy", nome: "Celular pelo ntfy", categoria: "Celular", resumo: "Notificação no celular, grátis, sem cadastro.",
		ajuda: ["Instale o app ntfy no celular (Android ou iPhone).", "No app, toque em + e assine um tópico com um nome difícil de adivinhar, por exemplo filuna-oficina-7342.", "Escreva o mesmo nome do tópico aqui embaixo."],
		campos: [{ id: "topico", rotulo: "Nome do tópico", dica: "o mesmo que você assinou no app" }, { id: "servidor", rotulo: "Servidor", padrao: "https://ntfy.sh", dica: "deixe assim, a menos que tenha o seu" }],
		avisar: { metodo: "POST", url: "{servidor}", corpo: { topic: "{topico}", title: "{titulo}", message: "{texto}", tags: ["{evento}"] } }
	},
	{
		id: "telegram", nome: "Telegram", categoria: "Mensagem", resumo: "Mensagem de um bot seu no Telegram.",
		ajuda: ["No Telegram, abra o @BotFather, mande /newbot e escolha um nome. Ele responde com o token do bot.", "Abra o seu bot novo e mande qualquer mensagem para ele (por exemplo, oi).", "Abra o @userinfobot e mande /start: ele mostra o seu número de identificação (Id)."],
		campos: [{ id: "token", rotulo: "Token do bot", dica: "123456:ABC… que o BotFather mandou", segredo: true }, { id: "chat_id", rotulo: "Seu Id no Telegram", dica: "o número que o @userinfobot mostrou" }],
		avisar: { metodo: "POST", url: "https://api.telegram.org/bot{token}/sendMessage", corpo: { chat_id: "{chat_id}", text: "{titulo}\n{texto}" } }
	},
	{
		id: "whatsapp", nome: "WhatsApp (CallMeBot)", categoria: "Mensagem", resumo: "Mensagem no seu WhatsApp pelo serviço gratuito CallMeBot.",
		ajuda: ["Salve o número +34 644 66 32 62 nos seus contatos (é o CallMeBot).", "Mande para ele, pelo WhatsApp: I allow callmebot to send me messages", "Ele responde com a sua apikey. Escreva aqui o seu número com código do país (ex.: 5511999998888) e a apikey."],
		campos: [{ id: "telefone", rotulo: "Seu WhatsApp com código do país", dica: "só números, ex.: 5511999998888" }, { id: "apikey", rotulo: "Apikey do CallMeBot", segredo: true }],
		avisar: { metodo: "GET", url: "https://api.callmebot.com/whatsapp.php?phone={telefone}&apikey={apikey}&text={titulo}%3A%20{texto}" }
	},
	{
		id: "discord", nome: "Discord", categoria: "Equipe", resumo: "Aviso num canal do Discord, com a cor do evento.",
		ajuda: ["No Discord, abra as configurações do canal › Integrações › Webhooks.", "Toque em Novo webhook e depois em Copiar URL do webhook.", "Cole a URL aqui."],
		campos: [{ id: "webhook", rotulo: "URL do webhook", dica: "https://discord.com/api/webhooks/…", segredo: true }],
		avisar: { metodo: "POST", url: "{webhook}", corpo: { username: "Filuna", embeds: [{ title: "{titulo}", description: "{texto}", color: "{cor_int}" }] } }
	},
	{
		id: "slack", nome: "Slack", categoria: "Equipe", resumo: "Aviso num canal do Slack.",
		ajuda: ["No Slack, crie um app em api.slack.com/apps e ligue Incoming Webhooks.", "Toque em Add New Webhook to Workspace e escolha o canal.", "Copie a Webhook URL e cole aqui."],
		campos: [{ id: "webhook", rotulo: "Webhook URL", dica: "https://hooks.slack.com/services/…", segredo: true }],
		avisar: { metodo: "POST", url: "{webhook}", corpo: { text: "*{titulo}*\n{texto}" } }
	},
	{
		id: "webhook", nome: "Webhook (seu sistema)", categoria: "API", resumo: "O Filuna manda cada aviso em JSON para o endereço que você quiser.",
		ajuda: ["Use para ligar o Filuna ao seu sistema, ao n8n, ao Make, ao Zapier ou a um servidor seu.", "Cada aviso chega como POST com JSON: evento, titulo, texto, impressora, peca, erro, cor e quando.", "Para o caminho contrário (o seu sistema mandar avisos para a casa), use a API do Filuna Home no site."],
		campos: [{ id: "url", rotulo: "Endereço que recebe os avisos", dica: "https://…" }],
		avisar: { metodo: "POST", url: "{url}", corpo: "{aviso}" }
	}
];
