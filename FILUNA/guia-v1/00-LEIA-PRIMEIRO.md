# Guia de implementação da v1 — LEIA PRIMEIRO

**Para quem é:** o modelo (Opus) que vai implementar o Filuna Slicer v1. Este guia foi escrito para você não precisar adivinhar nada. Se algo não estiver aqui, **pare e pergunte ao dono**; não invente.

**Decisão do dono (10/11/2026):** a v1 nasce completa e sólida; depois só correções. O sistema é **único e interligado**: tudo que o cliente faz gera dados que alimentam estoque, custos, manutenção, histórico e previsões, com backup automático no servidor.

## Ordem de leitura (obrigatória)

1. Este arquivo.
2. [01-TELAS.md](01-TELAS.md) — as telas aprovadas e como aplicá-las sem destruí-las.
3. [02-DADOS-E-FORMULAS.md](02-DADOS-E-FORMULAS.md) — modelo de dados, eventos e todas as fórmulas, com exemplos numéricos.
4. [03-SERVIDOR-E-BACKUP.md](03-SERVIDOR-E-BACKUP.md) — contas, API, sincronização e vínculo de arquivos.
5. [04-CRONOGRAMA.md](04-CRONOGRAMA.md) — fases, tarefas, arquivos a tocar e teste de aceite de cada uma.
6. Regras canônicas já existentes: `../REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md`, `../REGRA-CANONICA-CONECTIVIDADE-OPEN-20261110.md`, `../SPEC-V1-BASE-COMPLETA-20261110.md`, `../REGISTRO-CONTINUIDADE-ETAPAS.md`, e `AGENTS.md` na raiz.

## Os dois repositórios

| Repositório | O que é | Onde mexer |
|---|---|---|
| `abcollicelli-web/FilunaSlicer` | Fatiador Windows (fork AGPL do OrcaSlicer 2.5-dev, C++17/wxWidgets) | branch de trabalho `filuna/oem-portable-first-qa-20261106` |
| `abcollicelli-web/Filuna` → pasta `site/` | Servidor e site (Python FastAPI + SQLite), domínio `filuna3d.com` | ler `site/AGENTS.md` e `site/REGRAS.md` antes |

## Regras que você nunca quebra

1. **Não altere `src/libslic3r/**`** (motor). Toda a v1 é camada Filuna: GUI, dados, rede com o servidor.
2. **Não mude a função de nenhuma ferramenta ou parâmetro.** Você muda aparência, texto e ajuda. Nunca decida comportamento comparando texto traduzido; compare ids/flags.
3. **Nada de inglês na tela do cliente.** Só permanecem em inglês: nomes de marca/produto, materiais (PLA, PETG…), formatos (STL, 3MF), "G-code" e tokens de G-code.
4. **Nada fechado, nada de contornar autenticação** de impressora ou serviço. Só código aberto e SDKs abertos.
5. **Nenhum segredo no repositório nem no binário.** Chaves ficam no cofre do servidor.
6. **Arquivo 3D do cliente nunca sobe para o servidor.** Só o vínculo (caminho no PC) e os metadados.
7. **Não invente número.** Se um dado não existe (ex.: o G-code não traz a purga), grave `desconhecido` e mostre "sem dado"; nunca estime sem marcar `estimado = 1`.
8. **Nenhuma impressão começa sem confirmação explícita do usuário.**
9. **Sem merge em `main`, sem release e sem deploy em produção** sem ordem expressa do dono.
10. Ao terminar cada fase: atualizar `../REGISTRO-CONTINUIDADE-ETAPAS.md` com o que foi feito, o que foi testado e o que **não** foi.

## Como trabalhar (passo a passo, em toda fase)

1. Ler a fase em `04-CRONOGRAMA.md` inteira antes de escrever código.
2. Criar branch a partir da branch de trabalho: `filuna/v1-fNN-<nome-curto>`.
3. Implementar somente o que a fase lista. Se precisar tocar em arquivo fora da lista, anote o motivo no PR.
4. Rodar os testes da fase. No fatiador: `ctest` das suítes afetadas e o workflow `filuna-windows-qa-exe.yml` (compila o executável Windows em ~45 min; dependências em cache).
5. Para telas: tirar captura da tela real e colocar lado a lado com a referência em `FILUNA/design/visual-c/ui/` (ver 01-TELAS.md §6). Sem captura comparada, a tela não está pronta.
6. Abrir PR em rascunho descrevendo: o que mudou, como testou, o que ficou de fora.
7. Só passar para a próxima fase com o "Aceite" da fase cumprido.

## Estado real em 10/11/2026 (não presuma mais do que isto)

- **Pronto:** desenho das 9 telas em HTML/CSS (`FILUNA/design/visual-c/ui/`); regras canônicas; fonte oficial dos 8 fabricantes fixada (`FILUNA/fabricantes/*/module.json`); portátil do Snapmaker compilado a partir da fonte aberta.
- **Não existe ainda:** visual Filuna dentro do fatiador (o app ainda se chama OrcaSlicer e usa a cor `#009688` em 54 arquivos de código e 502 ícones); banco de dados local; contas de cliente no servidor (hoje só há `admin_users`); API de sincronização; auto-update; login Filuna; qualquer impressora homologada.
- **Existe e deve ser reaproveitado:** sistema de plugins Python do Orca (`src/slic3r/plugin/`, doc `docs/HLSD/preset-plugin-api.md`); ilustrações em tooltip (`docs/HLSD/tooltip-illustrations.md`); login por provedor (`src/slic3r/Utils/ICloudServiceAgent.hpp`, `OrcaCloudServiceAgent.*`); verificação de versão (`GUI_App::check_new_version_sf`); no servidor, estoque de bobinas do ateliê (`site/app/atelie_stock.py`), calculadora (`site/app/price_calculator.py`) e canal de releases (`site/deploy/slicer-releases/*.windows.json`).

## Decisões que ainda são do dono (pergunte antes de implementar o item)

1. Miniatura da peça (PNG ≤ 100 KB) pode subir ao servidor para o histórico mostrar a imagem? (o arquivo 3D não sobe de jeito nenhum)
2. Limite de dias para o alerta "vai faltar filamento" (este guia usa 7).
3. Intervalos padrão de manutenção (este guia traz sugestões editáveis, não são especificação de fabricante).
4. Preços e limites dos planos (`site/app/public_plans.json` está marcado como não aprovado).
5. Acesso ao cofre e ao deploy para o implementador.
