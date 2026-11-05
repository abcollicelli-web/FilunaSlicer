# Critérios de aceite

Uma versão só é liberada depois de passar em **todos** os itens nas duas impressoras.

## Gerais
- [ ] O instalador e o portable abrem no Windows 10/11 x64 sem erro.
- [ ] A pasta de dados é `%APPDATA%\Filuna`. Um Orca instalado continua intacto.
- [ ] O mesmo modelo, perfil e configurações geram G-code equivalente ao do OrcaSlicer no SHA fixado (comparar com diff, ignorando cabeçalho e metadados).
- [ ] Nenhuma impressão começa sem que o usuário confirme no diálogo.
- [ ] A tela "Sobre" mostra a licença AGPL, os créditos e o link do código-fonte.

## Elegoo Centauri Carbon 2 (firmware: ______)
- [ ] O perfil oficial da impressora existe e é selecionável.
- [ ] Fatia o cubo de calibração 20 mm e o Benchy sem avisos inesperados.
- [ ] A impressora é encontrada na rede local (ou adicionada por IP).
- [ ] Envio do arquivo funciona, com confirmação antes de iniciar.
- [ ] O status (progresso, temperaturas) aparece durante a impressão.
- [ ] Pausar, retomar e cancelar funcionam.
- [ ] A peça impressa tem a mesma qualidade que a feita pelo Orca/ElegooSlicer.

## Anycubic Kobra 3 V2 (firmware: ______)
- [ ] O perfil oficial existe e é selecionável (com e sem ACE, se aplicável).
- [ ] Fatia o cubo 20 mm e o Benchy sem avisos inesperados.
- [ ] Descoberta ou adição da impressora e envio funcionam, com confirmação antes de iniciar.
- [ ] O status aparece durante a impressão. Pausar, retomar e cancelar funcionam.
- [ ] A peça impressa tem a mesma qualidade que a feita pelo Orca/AnycubicSlicerNext.

## Registro
| Data | Versão Filuna | SHA upstream | Impressora | Resultado | Responsável |
|---|---|---|---|---|---|

## Regra de avanço e evidência (FLN-QG-2026-11-05.1)

A lista acima contém critérios de **produto/release** e permanece sem aprovações. Antes de qualquer avanço de implementação, cumprir [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md) e associar cada requisito do [mapa de migração](MAPA-MIGRACAO-PERSONALIZACAO.md) a teste e evidência.

| Portão | Estado inicial nesta branch | Bloqueio objetivo |
|---|---|---|
| G0 — Inventário e fontes | EM ANDAMENTO | A documentação histórica relevante ainda não foi esgotada; cobertura incompleta |
| G1 — Contrato visual/funcional | NOT RUN | Não há revisão visual do novo fork e aceite registrado |
| G2 — Implementação/código | NOT RUN | Nenhuma funcionalidade nova Filuna submetida a testes de código |
| G3 — Integração/regressão Orca | NOT RUN | Sem corpus comparativo nem teste completo de adapters |
| G4 — Windows real e impressoras | BLOCKED | Falta evidência de Windows 10/11 e impressão real nas duas impressoras |
| G5 — Pacotes, assinatura e rollback | BLOCKED | Instalação não assinada e update C/D não homologado |
| G6 — Homologação/liberação | BLOCKED | Todos os gates anteriores e aprovação específica da versão ausentes |

**Regra objetiva:** 100% dos casos obrigatórios do portão devem ter `PASS` com evidência conferível, cenário e versão; `NOT RUN`, `BLOCKED`, `FAIL` e campos essenciais ausentes contam como NÃO APROVADO. Não são permitidos dispensas implícitas ou marcar `[x]` a partir de relatório de outro aplicativo ou de um workflow verde. G4 exige dispositivo real e avaliação da peça impressa. Registre o SHA e links de evidência sem revelar credenciais.

### Checklist da Home Filuna aprovada — a implementar no novo fork
- [ ] Barra superior vinho 56 px, lateral vinho 208 px e identidade visual aprovada de 03/11.
- [ ] Abas superiores Início/Preparação/Visualização/Dispositivo/Projeto/Calibração; lateral Recentes/Modelos 3D/Paramétricos/Ferramentas/Estoque/Ateliê.
- [ ] Destaque da Home mostra trabalho fatiado REAL e estado verificável; exemplos não se passam por projeto do usuário.
- [ ] Painel Imprimir somente funciona após trabalho real selecionado e confirmação explícita.
- [ ] Faixas Recentes → Modelos → Ferramentas, ordem e dimensões verificadas contra contrato do Grok; sem busca/ações extras revogadas na Home.
- [ ] Temas original/azul/dark com cores corretas; testes de resolução, rolagem e navegação sem JS/layout quebrado.
- [ ] Ao entrar em Preparação, a cena 3D pertence ao Orca real (sem render ilustrativo substituir a mesa).
- [ ] Coexistência com Orca instalado: dados, perfis, arquivos e associação de protocolo testados.
- [ ] Comparação documentada dos estados antigos (C#) e atuais (fork C++) e dos contratos PROPOSTO/HOMOLOGADO.
- [ ] Testes do executável real no Windows e homologação visual expressa do proprietário.

**Controle de candidatas:** builds de CI, inclusive `Filuna Windows build`, são para QA privado; não significam atualização automaticamente instalável, distribuição pública ou gate de release aprovado. O script `FILUNA/tests/check_quality_gate.py` só audita a fundação estática; ele NÃO substitui testes de integração/hardware.
