# FILUNA — Política obrigatória de conexão por LAN, conta oficial e chave do dispositivo

**Contrato:** FLN-AUTH-LAN-2026-11-05.1 · **Origem:** decisão expressa do proprietário, 05/11/2026.  
**Status:** projeto/contrato. Nenhum dispositivo foi sondado nem pareado nesta documentação.

## 1. Objetivo e ordem de investigação

Para qualquer fabricante que não permita acesso aberto, **pesquisar e testar exaustivamente todos os meios de integração aplicáveis, documentados, tecnicamente seguros e autorizados**. Isso inclui portas de API local, fontes de fatiadores OEM, contas oficiais de fabricantes, SDKs autorizados, modos LAN e chave/código gerado pelo dispositivo. Referências como SimplyPrint ajudam a encontrar o **método** e as **capacidades por modelo**; não são autorização de uso do serviço SimplyPrint nem do código de rede fechado.

**Protocolo de decisão:**
1. **Pesquisa de modelo/firmware:** identificar impressora exata, firmware, configuração de rede, documentação OEM, repositório do slicer, concorrentes com indicação de modo de acesso e alternativas autorizadas. Registrar fonte/URL/SHA/data.
2. **Descoberta passiva na LAN:** mDNS/SSDP/anúncios oficiais e presença do endereço selecionado; pedir consentimento para pesquisa ativa. Não efetuar varredura na Internet. Se a rede está em VLAN, permitir endereço IP manual.
3. **Porta conhecida e serviço documentado:** em dispositivo do próprio usuário ou laboratório autorizado, conferir serviço da tabela de referência e protocolo, com timeouts e limites, sem exploração de vulnerabilidades, sem brute-force e sem autenticação implícita. Porta aberta significa apenas serviço disponível.
4. **API local oficial:** negociar capacidades e pareamento conforme firmware (Moonraker/OctoPrint/PrusaLink/SDCP/Flashforge/OEM específico). Erro e falta de ACK nunca viram sucesso.
5. **Login de fabricante:** usar apenas autenticação documentada/autorizada (por exemplo OAuth/PKCE no navegador do sistema, SDK de parceiro ou integração oficial). Não recolher senha em formulário Filuna quando existe fluxo delegado. Não extrair refresh tokens de aplicativos fechados nem reutilizar cookies de sessão.
6. **LAN mode / código local:** quando a impressora permite, instruir usuário a habilitar o modo LAN, recuperar a chave/PIN/código do próprio dispositivo e inserir no Filuna. Segredo protegido por armazenamento seguro do sistema operacional, escopos mínimos e revogação. Código incorreto gera erro, não tentativa repetitiva silenciosa.
7. **Teste de funcionalidades:** identidade/online real, temperaturas, câmera, filamentos, envio de arquivo, início com confirmação, pausa, retomada, cancelamento e material/slot. **Cada capacidade possui teste próprio** com firmware e evidências.
8. **Fallback:** se nenhum método autorizado for funcional, deixar integração **NÃO VERIFICADA / NÃO DISPONÍVEL**, com exportação manual G-code/3MF e instruções de envio externo. O motor Orca permanece operacional.

Os passos 4, 5 e 6 podem ser avaliados em ordem diferente conforme a documentação do fabricante. Cada possibilidade aplicável deve ser registrada com resultado \`SUPPORTED\`, \`UNSUPPORTED\`, \`BLOCKED\`, \`NOT DOCUMENTED\`, \`REQUIRES OWNER ACTION\` ou \`NOT TESTED\`.

## 2. Regras legais e de segurança inegociáveis

- Só testar impressoras e redes **próprias ou com autorização explícita**. O Filuna não terá “scanner de portas públicas” na Internet, não abrirá UPnP/port-forward automaticamente e não pedirá exposição da impressora fora da LAN.
- Não ensinar, projetar ou automatizar evasão da autenticação do fabricante, engenharia reversa de credenciais de apps de terceiros, falsificação de identidade/assinatura ou exploração de firmware. **Protocolo público não é permissão para usar uma API de nuvem privada.**
- Não alterar firmware, ativar root, instalar firmware comunitário, atualizar ROM ou executar comandos de movimentação/temperatura por padrão. Ferramentas opcionais de terceiros são **fora do escopo** até aprovação e estudo de segurança próprios.
- Mensagens reais de \`StartPrint\`, \`Cancel\`, \`Move\`, \`Heat\`, \`LoadFilament\` exigem confirmação contextual, capacidade, estado e autorização. Importante: confirmação de UI não compensa ausência de autenticação de protocolo.
- Credenciais de conta e LAN em DPAPI/Windows Credential Manager ou cofre equivalente, separadas por impressora/usuário, criptografadas, não enviadas a servidores Filuna ou telemetria. OAuth/PKCE e escopos mínimos quando aplicável.
- Certificados TLS e origem devem ser verificados; exceções por firmware apenas documentadas, estreitas e aprovadas. Rejeitar conexões de identidade ambígua ou trocada.
- Falha ou timeout = \`UNKNOWN\` / \`FAILED\`, sem iniciar novamente por tentativa cega. Tela deve orientar reconciliação manual de estado.
- Filuna não fará upload de câmera/print de cliente para nuvem por padrão; isolamento e política de retenção específicos.

## 3. Padrão de dossiê por fabricante/modelo

Cada integração cria \`FILUNA/modules/<vendor>/MODULE.md\` com esta matriz:

| Campo | Exigência |
|---|---|
| \`manufacturer_id\`, \`model_id\`, \`firmware\` | preenchidos e verificáveis, sem usar apenas nome comercial genérico |
| \`source_oem\` | link, SHA, arquivo, SPDX, atribuição, compatibilidade de licença |
| \`source_external\` | URL da referência SimplyPrint/Obico/OctoPrint/Moonraker e o que exatamente afirma |
| \`connection_method\` | \`direct_lan\`, \`official_account\`, \`lan_key\`, \`manual_export\`, \`none\` |
| \`ports/protocols\` | protocolos documentados e serviços da impressora com transporte, nunca senha |
| \`auth_method\` | PIN/local access key/OAuth/SDK; titularidade, escopo e revogação |
| \`identity_validation\` | número de série mascarado, modelo, firmware, certificado, handshake |
| \`capability_matrix\` | \`status\`, \`camera\`, \`temps\`, \`files\`, \`start\`, \`pause\`, \`resume\`, \`cancel\`, \`filaments\`, \`multimaterial\`, \`skip_object\` |
| \`test_case/evidence\` | caso, build SHA, dispositivo e firmware, positivos/negativos, confirmação, log sanitizado |
| \`status\` | \`RESEARCH\`, \`CODED\`, \`TESTING\`, \`APPROVED\`, \`UNSUPPORTED\` |
| \`blocked_reason\` | motivo exato (SDK fechado, não há API, falta hardware, requisito de firmware, licenciamento) |

**Definição de esgotar a pesquisa:** pesquisar fontes oficiais, docs de SDK, código aberto legalmente reutilizável, docs de plataformas independentes, modos locais e de conta efetivamente disponibilizados para aquele modelo; registrar fontes e testes que foram possíveis, com lista de impossibilidades. Sem hardware ou sem autorização oficial, \`BLOCKED\` não poderá ser promovido a \`PASS\` por parecer funcionar na SimplyPrint.

## 4. Ensaios de segurança e funcionalidade mínimos

- Resposta em porta LAN certa com mensagem inválida não deve produzir identificação falsa.
- Porta existente mas sem autorização gera \`AUTH_REQUIRED\`; senha/PIN errados não revelam existência de contas nem fazem retries sem limite.
- Login autorizado abre no navegador e retorna apenas escopos e tokens adequados; login cancelado, expirado, revogado, e duas contas distintas não vazam entre si.
- LAN Key correta mas firmware não compatível não pode permitir comandos sem validação de capacidades.
- Certificados falsos ou destino IP alterado durante pareamento são recusados e geram alerta claro.
- Câmera não pode ativar sozinha; stream indisponível desativa preview e não conserva selo “ao vivo”.
- \`StartPrint\` sem consentimento, ACK ou job real nunca retorna sucesso; \`Pause/Resume/Cancel\` sem capability não transmite comando.
- Credenciais não aparecem em logs, tickets de diagnóstico, crash dumps, backups exportáveis ou arquivo 3MF.
- Trinta minutos de conexão e reconexão após suspensão Windows sem memory leak ou impressão duplicada; limiar de desempenho fixado após baseline.
- Modos LAN e cloud devem manter capacidades separadas: o suporte de um modo não transfere aprovação para outro.

**Teste do usuário sobre posse/dispositivo é necessário** em operações ativas; não basta o equipamento estar visível na LAN.

## 5. Uso de fontes independentes

- [Pesquisa SimplyPrint do projeto](REFERENCIAS-COMPATIBILIDADE-SIMPLYPRINT.md) é referência de modelos/portas/métodos, não API do Filuna.
- Documentação OEM e licenças das implementações são fonte primária; código funcional da Filuna e evidências físicas são condição única para declarar “nativo”.
- Obico, OctoPrint, Moonraker/Mainsail/Fluidd, PrusaLink e repositórios OEM completam investigação quando houver modelos atendidos.
- Em Bambu, Flashforge e fabricantes com plugins proprietários: requer autorização e fonte redistribuível; não incluir DLLs sem licença nem contornar firmware.
- Em Anycubic/Creality restritos: avaliar primeiro modo LAN original e API autorizada; firmware comunitário é opção de laboratório separada com consentimento, não requisito silencioso para o cliente.

## 6. Como a tela Dispositivo deve refletir o resultado

**Descoberta local → selecionar máquina → ver métodos de conexão disponíveis → escolher LAN/conta oficial/chave de impressora → autenticar → verificar firmware/capabilities → painel com SOMENTE operações reais.**

Estados apresentados: \`Local detectada\`, \`Precisa de login oficial\`, \`Precisa de chave LAN\`, \`Conectada\`, \`Conectada parcialmente\`, \`Offline\`, \`Protocolo não homologado\`, \`Recurso não suportado\`. O UI deve sempre diferenciar **conectado à rede** de **pode imprimir**, e **suporte SimplyPrint** de **suporte Filuna**.

### Gate final

Nenhuma marca, recurso de câmera ou material é liberado porque uma porta está aberta, conta funcionou em outro app ou a SimplyPrint anunciou compatibilidade. **Testar no Filuna Windows, na impressora real, no firmware real, com 100% dos casos obrigatórios PASS e aprovação do proprietário.**
