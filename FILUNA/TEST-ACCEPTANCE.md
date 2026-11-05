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
