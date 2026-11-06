# Anycubic — Kobra S1 / Kobra 3 V2: origem oficial e testes internos (07/11/2026)

## Fonte efetivamente obtida
- Publicação oficial Anycubic via APT: `https://cdn-universe-slicer.anycubic.com/prod/dists/noble/main/binary-amd64/Packages`.
- Pacote Debian original: `AnycubicSlicerNext_linux-v2.0.0.6-20261030105113.deb`.
- Versão APT: `1:2.0.0.6`; versão do aplicativo declarada nos recursos: `2.0.0.6`.
- Comprimento: **153.586.194 bytes**.
- SHA-256 divulgado no índice oficial e confirmado após download: `fdb134a4e11fad7e795c8d425520ce1305321ccc0bf1375a651a1513f8854678`.
- Este **não é** um ZIP portátil Windows. É o pacote Linux OEM original, **extraído em pasta temporária** com `dpkg-deb -x`, sem instalar, compilar, imprimir ou modificar o sistema do usuário.
- O primeiro ensaio se recusou corretamente a aceitar a versão antiga `2.0.06`; os dados de pin foram atualizados apenas após capturar `Version/Filename/SHA256/Size` atuais do índice do fabricante.

## Evidência real de CI
- Runner GitHub Actions Ubuntu 24.04 `#42314817560`: **SUCCESS**, seis testes de integridade negativos/positivos PASS.
- `VENDOR_APT_INDEX=PASS`, `OFFICIAL_DEB_SHA256=PASS`, `ORIGINAL_PACKAGE_EXTRACTION=PASS`, `OFFICIAL_APPLICATION_VERSION=PASS`.
- Encontrados **1 executável** e **630 arquivos JSON** de perfis.
- Busca textual inicial no inventário: **184 referências para Kobra S1** e **50 referências para Kobra 3 V2**. Estes números medem arquivos que mencionam os nomes, **não** 184 ou 50 perfis homologados.
- Artefato e logs: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42314817560/artifacts/13550426151

## Critérios de promoção
| Prova | Kobra S1 / ACE Pro | Kobra 3 V2 |
|---|---|---|
| Pacote OEM oficial e SHA | PASS (compartilhado) | PASS (compartilhado) |
| Candidato de perfil encontrado | PASS apenas inventário textual | PASS apenas inventário textual |
| Resolução de máquina, processo, filamento, bico e modelo exatos | PENDENTE | PENDENTE |
| CLI oficial A/B em saídas independentes | PENDENTE | PENDENTE |
| G-code real do Filuna com comparação de paridade | PENDENTE | PENDENTE |
| Rede de laboratório, capacidades e integração UI | PENDENTE | PENDENTE |
| HOMOLOGAÇÃO INTERNA do modelo | PENDENTE | PENDENTE |
| HOMOLOGAÇÃO FÍSICA | Apenas no beta vigiado após compilação final | Apenas no beta vigiado após compilação final |

**Próximo passo:** extrair hierarquias e combinações de perfis exatas de cada modelo no pacote oficial 2.0.0.6, testar CLI A/B, depois comparar saída do motor Filuna e executar testes de rede simulada. Não substituir Anycubic Kobra 3 V2 por Kobra 3 V3; não converter o sucesso de extração em sucesso de impressão.

**Governança:** PR #6 permanece DRAFT. A única validação voluntariamente adiada continua Bambu × Bambuddy. Nenhum build final, instalador, merge, upload de produção ou impressão física nesta etapa.
