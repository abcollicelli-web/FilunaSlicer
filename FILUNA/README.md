# Filuna Slicer

Filuna Slicer é um fork do **OrcaSlicer**: o motor de fatiamento, os perfis e os protocolos de impressão são os do Orca, sem alterações. Por cima disso fica uma camada visual e de produto Filuna. Ele também traz módulos de fabricante (Elegoo, Anycubic e outros) para envio e controle nativos.

## Continuidade (obrigatório antes de trabalhar)

**Antes de qualquer IA ou colaborador continuar o projeto:** ler [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md), depois [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md) e o [plano mestre](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md). O registro no GitHub é a fonte do estado das etapas; não usar histórico de chat.

## Visão
- Fatiar e imprimir **igual ao Orca** desde o primeiro minuto.
- Um único programa para Orca, Bambu, Elegoo, Creality, Flashforge, Qidi, Snapmaker e Anycubic.
- Aparência própria (Filuna), com o Orca de verdade por baixo.

## Regras do projeto
1. **O upstream fica intacto.** A branch `upstream-mirror` é uma cópia exata do OrcaSlicer e nunca é editada à mão.
2. **A camada Filuna fica isolada.** Ela vive em `FILUNA/`, com o mínimo de pontos de extensão no código do Orca. Cada um desses pontos é listado em ARCHITECTURE.md.
3. **Os módulos de fabricante são separados e versionados**, conforme MODULES.md.
4. **Toda atualização de upstream** passa por merge controlado, CI de teste e aceite (UPDATE-PLAYBOOK.md).
5. **Nenhuma impressão começa sem confirmação explícita do usuário.**
6. **Pasta de configuração própria `Filuna`**, separada do Orca, do mesmo jeito que o Bambu Studio faz. Não se misturam perfis com um Orca instalado.
7. **Nada é considerado pronto** antes de passar no TEST-ACCEPTANCE.md (Elegoo Centauri Carbon 2 e Anycubic Kobra 3 V2).
8. **Sem plugins de rede proprietários empacotados** (Bambu e Flashforge), por compatibilidade com a AGPL.

## Licença
GNU **AGPL-3.0**, a mesma do OrcaSlicer (ver `LICENSE.txt` na raiz). O código-fonte do Filuna é público. Cada binário distribuído deve indicar onde obter o fonte correspondente.

## Créditos
Slic3r (Alessandro Ranellucci e comunidade) → PrusaSlicer (Prusa Research) → Bambu Studio (Bambu Lab) → OrcaSlicer (SoftFever e comunidade) → Filuna Slicer.
Os módulos de fabricante derivam de ElegooSlicer, AnycubicSlicerNext e outros forks AGPL, com crédito aos respectivos autores.

## Regra mestre adicional — qualidade antes de evolução (05/11/2026)

**Fonte obrigatória:** [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md). Nenhuma etapa do novo Filuna Slicer avança enquanto qualquer teste obrigatório estiver em FAIL, NOT RUN ou BLOCKED. Build, artefato, QA técnico, homologação do proprietário e liberação comercial são estados diferentes. Todo requisito, decisão, implementação, evidência e resultado ficam registrados por commit. Nunca declarar pronto um botão ilustrativo, uma integração ainda não testada ou um projeto apenas compilado.

**Referência de personalização do projeto anterior:** [MAPA-MIGRACAO-PERSONALIZACAO.md](MAPA-MIGRACAO-PERSONALIZACAO.md). O antigo C# é fonte de especificações e evidências históricas; não copiar suas bibliotecas para o fork C++ sem design e testes. Todo desenvolvimento visual deve usar a Home Grok homologada de 03/11, conferir regras mais recentes e preservar a experiência de fatiamento real do Orca.

**Estado atual:** somente inventário documental G0 iniciado; a matriz TEST-ACCEPTANCE continua pendente. Esta branch não altera a compilação Windows em curso nem libera publicações.

## Identidade visual e mascote — referência de 05/11/2026

O proprietário forneceu as referências do logotipo **Filuna 3D**, paleta creme/vinho/rosa/laranja/cinza-claro, ícone de aplicativo **F** e mascote oficial em duas versões (circular e com fundo transparente). Consultar obrigatoriamente [IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md) e [manifesto de referências](brand/REFERENCIAS-MANIFESTO.json) antes de editar logotipo, ícones, temas, onboarding, Home, instalador ou Sobre. Os cinco originais estão versionados em `FILUNA/brand/assets/original/` com SHA-256 = manifesto (lote continuidade). **G1 visual permanece não homologado** (tokens HEX, derivados e QA Windows).


## Plano central de engenharia e infraestrutura (05/11/2026)

Para qualquer nova função, primeiro verificar as dependências no [cronograma DAG de 52 entregas](CRONOGRAMA-DEPENDENCIAS.json), o [plano mestre de construção](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md), a [matriz 100%](MATRIZ-TESTES-ACEITE-100-POR-CENTO.md), a [auditoria de migração](MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md) e a [matriz legal](CONFORMIDADE-LEGAL-NORMAS-E-CERTIFICACOES.md).

**Servidor e acesso remoto:** [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md) rege Hub, domínio/IP, banco privado, backups/restore, sync, erros e status opt-in, sem controle de impressoras pelo admin Filuna. O controle remoto **exclusivo do próprio cliente**, via web/mobile e autenticação/consentimento, é uma etapa FUTURA: requer antes homologação integral de impressão local em hardware, DeviceBridge, isolamento e gates SRV-00..11, com feature flag OFF até liberação específica. Motor Orca e projetos locais não dependem da nuvem.

**Inventário G0 (leitura prioritária):** [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md) · [conflitos G0](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md) · [fontes obrigatórias N1](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md) · [SBOM parcial](sbom/README.md).

**Manifesto de fontes históricas:** [MANIFESTO-FONTES-LEGADO-2026-11-05.md](MANIFESTO-FONTES-LEGADO-2026-11-05.md) lista os **141 arquivos de documentação/contrato** dos dois projetos anteriores com blob SHA e tamanho. O inventário não é leitura integral nem aprovação G0.


## Auditoria do plano mestre

[Auditoria técnica de 05/11/2026](AUDITORIA-PLANO-MESTRE-2026-11-05.md): apresenta o que foi realmente conferido no GitHub/SentinelX e CI, **113 validações documentais PASS**, 141 fontes inventariadas por SHA, 52 entregas/149 dependências, riscos, conflitos e o estado **G0 PASS DOCUMENTAL** / **G1–G6 NÃO HOMOLOGADOS** (G4 Windows NOT RUN). A implementação de servidor, controle remoto e impressoras segue pendente; a PR #2 permanece DRAFT.


## G0 — fontes obrigatórias (N1)

- [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md) — conjunto normativo vs 141 históricos  
- [G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md) — N1–N8 / CF  
- [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md) — profundidades A/B/C  


## Fechamento G0 (documental)

- [G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md) — PASS documental + critérios rastreados  
- [G0-TESTES-CASOS-2026-11-05.md](G0-TESTES-CASOS-2026-11-05.md) — G0-T01..T10  
- [G0-MAPA-141-STATUS-2026-11-05.md](G0-MAPA-141-STATUS-2026-11-05.md) · [G0-MIG-DAG-ALINHAMENTO-2026-11-05.md](G0-MIG-DAG-ALINHAMENTO-2026-11-05.md) · [G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md](G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md)


## Onda 01 — Fundação (atual)

Contrato **ESCOPO APROVADO** (dono 2026-11-05 23:24 Europe/Rome); implementação mínima com testes:

- [FUND-SPEC-ONDA01-2026-11-05.md](FUND-SPEC-ONDA01-2026-11-05.md)
- [FUND-REQ-ONDA01-2026-11-05.md](FUND-REQ-ONDA01-2026-11-05.md)
- [FUND-TEST-ONDA01-2026-11-05.md](FUND-TEST-ONDA01-2026-11-05.md)
- [FUND-ADR-BASELINE-DADOS-2026-11-05.md](FUND-ADR-BASELINE-DADOS-2026-11-05.md)
- [approvals/FUND-SPEC-2026-11-05.1-owner-approval.json](approvals/FUND-SPEC-2026-11-05.1-owner-approval.json)

Próximo passo operacional: aceite de escopo pelo proprietário → LEGAL-02/QA-03. **Não** iniciar Onda 03 (Home/tokens) agora.


## Direção visual C — referência aprovada em 08/11/2026

Antes de criar ou alterar qualquer tela Filuna, consultar `FILUNA/design/visual-c/documentacao/DESIGN-SYSTEM-C.md`, a aprovação literal em `FILUNA/design/visual-c/documentacao/APROVACAO-VISUAL-C.json`, os dois modelos C aprovados e a galeria/matriz em `FILUNA/design/visual-c/README.md` e `FILUNA/design/visual-c/MATRIZ-COBERTURA.md`. O proprietário aprovou a direção visual; as demais telas e detalhamentos são propostas em revisão. O inventário consolidado contém 123 pranchas e 208 itens rastreados, sem equivaler à homologação de software.

Preparação permanece sem painel direito de informações de arquivo; Orca é o motor único. A regra vigente de G-code imutável e a distinção entre risco OEM e divergência de motor permanecem intactas. Esta aprovação não retoma implementação, CI, hardware, distribuição, servidor ou website. No escopo pausado pelo proprietário, G2/G3 permanecem BLOCKED, G4 NOT RUN e G5/G6 BLOCKED. Nenhum requisito funcional ou registro histórico foi revogado.

## Obrigação visual FLN-VISUAL-C-100 — 08/11/2026

Antes de qualquer trabalho de UI, ler obrigatoriamente [REGRA-FIDELIDADE-VISUAL-C.md](design/visual-c/documentacao/REGRA-FIDELIDADE-VISUAL-C.md), contrato de montagem e ordem expressa de 14:16 Europe/Rome. Toda IA/pessoa deve reproduzir 100% identicamente o modelo aprovado; nenhuma mudança estética/estrutural/de recurso sem nova aprovação explícita do proprietário. Derivadas não são aprovadas automaticamente; lacuna ou conflito bloqueia o item. Evidência visual real e aceite humano são obrigatórios; conformidade da aplicação NOT RUN. Pausa funcional e gates preservados.
