# FILUNA — Ciclo inicial OEM: portáteis oficiais no Windows

**Execução iniciada em 06/11/2026.** Escopo isolado na branch `filuna/oem-portable-first-qa-20261106`. Nenhuma publicação, instalação de OEM, alteração em produção ou compilação de código de fabricantes.

## Origem e regra de aprovação

O código do laboratório `lab/portable_first_qa.py` baixa os arquivos ZIP oficiais das releases GitHub, fixa o SHA-256 da própria release e só extrai após conferir o hash. Mantém os oito fabricantes na matriz. **A presença de um ZIP não certifica equivalência do Filuna**, nem confirma comunicação com impressora.

Os cinco ZIPs oficiais pinados são:

| Marca | Versão fixada | Fonte | SHA-256 do ZIP |
|---|---|---|---|
| Bambu | 02.09.01.70 | bambulab/BambuStudio | `a4c0d5f94109cb2c2c07fb02ca1f740e96b586fbf3ae557a1647eeb5503a15b8` |
| Elegoo | 1.5.3.5 | elegooofficial/ElegooSlicer | `0215cd958d6a52ae1bd33a5ad7509ecd1b156baf63fbe04333e04cf0305793e5` |
| QIDI | 2.07.02.73 | QIDITECH/QIDIStudio | `30f276c5390e4ca755c0a08d2115d6ea5feea2a2ee7ec7a3a7e3a729d0094224` |
| Snapmaker | 2.4.1 | Snapmaker/OrcaSlicer | `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d` |
| Prusa | 2.9.7 | prusa3d/PrusaSlicer | `5d4f395aeb6c344b300e0e1be37a0e1931622b642c3c3085eb19cdae211bc008` |

**Pendente de pacote ZIP oficial identificado e verificado:** Flashforge 1.7.15, Creality Print 7.3.1 e Anycubic Slicer Next. Não usar instaladores como se fossem portáteis.

## Corpus fiel do laboratório anterior

Os arquivos `fixtures/oem/{bambu,elegoo,qidi}/cube-profiled.3mf.b64` contêm somente cubos de diagnóstico gerados por CLI oficial. Estão em Base64 porque a interface de commits disponível é textual; decodificam para ZIP 3MF exato, verificado com hash original.

- Bambu A1: `212202ece5982aaceccdbe57359c070ecec3c4ddb00d4baf211c80556f50baa4`
- Elegoo CC2: `c5a6bb377548c129cb1c14d2b72ea1cc49197ecccc339da99c3c37b75ea68450`
- QIDI Q2: `2e5a8d293bac8846d547e16a0d4d04ee07e67b63ed1eea6f3b1844b38266ac04`

Origem: rodada local `run-20261106T153215Z` documentada em `FILUNA/qa/HOMOLOGACAO-LAB-8-FABRICANTES-R3-20261106.md`. Os 3MF são de cubos anteriores, **não** substituem o multicor original do proprietário nem validam o CANVAS/CFS/AMS.

## Decisão de escopo do proprietário — Bambuddy fica para depois

**Única validação deliberadamente adiada:** linha **Bambu com Bambuddy**, incluindo o contrato autenticado e a integração com impressora virtual Bambuddy. Marcar `BAMBU_BAMBUDDY = PENDENTE (ADIADO PELO PROPRIETÁRIO)`, nunca `PASS`, `FALHA` ou `HOMOLOGADO` por ausência de execução.

A decisão **não suspende** os testes do portátil Bambu Studio, G-code oficial A×B, comparação Filuna×Bambu, perfis, reconhecimento normal do dispositivo e demais verificações que possam ocorrer sem Bambuddy. Tampouco adia outros fabricantes. Todos os outros bloqueios técnicos, lacunas de corpus e falhas descobertas devem ser investigados e resolvidos agora, registrando evidência real para cada gate; não promovê-los a PASS sem prova.

Não iniciar ou configurar testes Bambuddy, nem tentar autenticação ou ações na impressora virtual nesta fase. Retomar Bambuddy somente após decisão expressa do proprietário.

## Critérios de QA e gates que NÃO podem ser confundidos

1. `PACKAGE_SHA256`: ZIP oficial pinado e íntegro.
2. `PORTABLE_EXE`: executável do ZIP detectado e dependências mantidas na pasta.
3. `PROFILED_FIXTURE`: arquivo 3MF arquivado, com SHA correto.
4. `OFFICIAL_A_B`: duas execuções independentes do mesmo portátil sobre o mesmo 3MF, em diretórios diferentes, exit 0, G-code presente e SHA-256 bruto idêntico. Se houver diferença, registrar **FALHA** antes de autorizar normalização de linhas comprovadamente variáveis.
5. `FILUNA_PARITY`: somente após produzir G-code pelo **aplicativo Filuna**, com execução e presets comprovados, comparar com OEM. O uso do próprio executável OEM duas vezes NÃO conta como paridade Filuna.
6. `NETWORK`: somente após Dispositivos → Preparar → Fatiar → Enviar com SDK real e logs/ACK de simulador ou hardware; provas experimentais de rede não aprovam o aplicativo.
7. `HOMOLOGACAO`: exige todos os gates aplicáveis, decisão explícita do proprietário e prova de todas as capacidades previstas para o modelo.

## GitHub Actions

Workflow: `.github/workflows/filuna-oem-portable-first.yml`. Runner Windows 2022, apenas Python e binários ZIP oficiais, sem CMake/MSVC/compilar fatiadores. Artefato `filuna-portable-first-five-official-eight-brands`: relatório `RESUMO.md`, `results.csv`, `results.json`, lock de cada fabricante, logs A/B e hashes. Arquivos ZIP de centenas de MB permanecem apenas no cache do GitHub Actions, não entram no artefato.

### Limites explícitos

A homologação integral dos oito fabricantes não é possível só com ZIPs: requer módulos Filuna funcionando, modelos e firmware corretos, interfaces de dispositivo, casos multicor oficiais e provas de rede. Não divulgar como recurso homologado nem publicar produto enquanto isso faltar.
