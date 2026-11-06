# FILUNA — REGRA MESTRE DE ATUALIZAÇÃO INCREMENTAL OEM + ORCA
**ID:** FLN-OEM-UPSTREAM-2026-11-06.1
**Ordem do proprietário:** 06/11/2026, válida para todas as marcas, todos os modelos e atualizações futuras.

## Diretriz inalterável
Filuna Slicer Windows **já existe e está compilado**; a interface Orca e o motor Orca local continuam sendo o produto. Em **Dispositivos**, localizar/parear e registrar impressoras pelo protocolo compatível do fabricante; em **Preparar**, selecionar automaticamente os componentes OEM efetivamente compatíveis com identidade de hardware, modelo, firmware e versão; em **Imprimir**, listar somente dispositivos registrados do cliente. É proibido tratar seleção de nome de módulo, perfil JSON ou emulador como prova de módulo carregado.

## Rede / autenticação
1. Tentar primeiro descoberta e recursos/protocolos locais documentados e permitidos, como SSDP, mDNS, MQTT/MQTT-TLS, HTTP, FTPS e WebSocket conforme modelo/firmware.
2. Cumprir a autenticação exigida desde o primeiro protocolo; uma porta aberta **não** autoriza comando anônimo.
3. Se os recursos forem insuficientes, usar modo LAN com código **fornecido pela própria impressora**, quando suportado pelo firmware. O LAN Code pode autenticar o MQTT/FTPS já escolhido; não supor outro transporte. Sem bypass de fabricante, sem nuvem privada e sem distribuir biblioteca fechada.
4. Respeitar protocolo, estado, autorização e ACK verdadeiros. Nunca iniciar impressão automaticamente. Manter o Orca-base funcional offline e sem plano.

## Regra de rastreabilidade por arquivo
Para cada módulo/algoritmo/macros/perfis incorporados, registrar:
- Fabricante, modelo, revisão, firmware, capacidades e **versão de protocolo**.
- Fonte oficial (URL/repositório), **SHA do commit upstream**, caminho do arquivo, SHA do blob e licença da unidade/dependências; manifestar derivados e avisos de copyright/AGPL.
- Dependências transitivas e compatibilidade com pin Orca e ABI/runtime Filuna, sem invocar fatiador instalado externamente.
- Estado individual: DISCOVERED, LICENSE_REVIEWED, PORT_CANDIDATE, IMPLEMENTED_NOT_TESTED, MOCK_PASS, FILUNA_INTEGRATION_PASS, GCODE_EQUIVALENCE_PASS, HARDWARE_PASS. Nunca promover automaticamente o status de outro estágio.
- Testes positivos/negativos: fixtures com SHA, versão de build, preset efetivo **após herança**, bico, filamento, parâmetros, geometria/posição, comandos, tempos/purga, resultados e evidência.
- Se incompatível, recusar módulo/rollback; um parser que rejeita preset não autoriza descartar opções silenciosamente.

## Workflow de atualização OEM + Orca (diferencial, nunca rebuild total por reflexo)
A. Fixar snapshot da versão **homologada atual** (Orca + OEM por modelo) e a versão **candidata**; verificar upstream oficial e licença por arquivo.
B. Comparar SHAs, tree/diffs de código e presets, inclusive **valores efetivos herdados**. Preservar itens idênticos. Revisar mudanças de API/ABI, comandos G-code, capacidades e firmware.
C. Portar **apenas as alterações necessárias e legalmente reutilizáveis**, mantendo história de origem, commit, licença e dependências; não copiar automaticamente plugins de rede fechados nem chaves/servidores.
D. Compilar e testar unidade, integração nativa Preparar, fatiamento com fixtures idênticas, comparação sem alterar a saída oficial, LAN simulada (com estados e autenticação), Windows real e hardware do modelo antes de marcar homologação.
E. Passagem por G0–G6 conforme critérios atuais; alterar módulos por atualização assinada/hash/rollback, sem reinstalar pacote integral salvo quando dependência binária exigir. Nunca distribuir candidato não homologado.
F. Atualizar manifesto e diário no **mesmo PR**. Detectar novo upstream não é instalar upgrade nem conceder PASS.

## Primeiro piloto — Bambu A1
- Fonte de teste: Bambu Studio oficial v0.29.0.170, Orca pin `a620d804efbe74162c96ad29d755cdab2b823530`, Filuna branch `filuna/qa-gates-hardening-20261105`.
- `FILUNA/qa/gcode/bambu_a1_source_manifest.json` registra SHAs de 5 macros Bambu A1 0.4; todas conferiram idênticas aos macros Filuna no teste Ubuntu/Wine. **Não recopiá-las sem diff**.
- `FILUNA/qa/gcode/audit_oem_preset_parity.py` detectou **33 diferenças efetivas** entre máquina (6), PLA (8), processo (19). `compare_oem_gcode.py` reprovou equivalência binário oficial×Filuna e detectou impacto de presets cruzados.
- Port do mecanismo Bambu/versão e carregamento automático **NÃO REALIZADOS** neste contrato; não declarar integração/pass. O erro `Vendor bundle contains invalid presets` permanece bloqueador.
- Primeiros modelos: A1, AD5X, Anycubic Kobra (identidade/versão a confirmar; não confundir Kobra V3 e Kobra 3 V2) e Elegoo CC2. Depois aplicar a todos os outros adaptadores.

## Aceite e atualização de documentação
Esta regra se aplica ao código, manifestos, CI, documentação, atualizador e geração de releases; é superior a rascunhos antigos que previam copiar todos os fatiadores ou apenas perfis JSON. Não substitui a obrigação de obter evidência funcional e física; apenas organiza reuso correto. Novas fontes AGPL/GPL são portadas com cumprimento das licenças aplicáveis, sem afirmação de que todo componente do fabricante seja necessariamente livre.

**Release atual:** nenhum módulo novo homologado neste documento. Uma atualização upstream jamais altera a versão de produção sem gate e aceite.
