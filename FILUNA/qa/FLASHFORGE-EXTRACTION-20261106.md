# Filuna QA — Flashforge 1.7.15: extração do instalador oficial

**Estado: falha de extração, não homologado.** Sem compilação, instalação em produção, envio à máquina ou merge.

- Release oficial: `FlashForge/Orca-Flashforge`, tag `1.7.15`, `Flash_Studio_Desktop_1.7.15_x64.exe`.
- SHA-256 verificado no runner: `b3cc8804e0426e3f4bb2c0953667e27512c0047af51099c0e1af5c5d1b3ef8f3`.
- `innoextract`: `Not a supported Inno Setup installer!` (código 2).
- `7z x`: extração de 37 componentes do próprio executável PE; **zero executáveis OEM e zero recursos/perfis utilizáveis**. Uma saída 7-Zip com exit 0 **não** constitui fatiador portátil.
- Artefato da tentativa final: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42309746605/artifacts/13547847888

**Próxima investigação:** identificação do empacotador real e avaliação de extração isolada e legal da mesma versão oficial, sem compilar fatiadores; em paralelo, inventário de `resources/` na tag upstream oficial. Somente depois de executar o binário oficial e produzir G-code verificável haverá comparação A/B; nenhuma prova de rede nativa foi gerada.

A única etapa propositalmente adiada continua sendo a validação Bambu com Bambuddy. Os impedimentos técnicos da Flashforge permanecem em análise ativa.
