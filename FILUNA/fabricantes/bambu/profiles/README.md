# Bambu fabricante — overlay de origem para engenharia (não habilitado)

O arquivo `profiles/bambu-official-delta-v02090170.tar.xz` é um **snapshot de testes** dos dados oficiais do Bambu Studio 02.09.01.70, gerado comparando semanticamente os perfis JSON com a base Orca usada pelo Filuna. Contém **1.396 JSONs novos/diferentes** e **18 matrizes JSON de impressoras** (17 modelos + lista de bloqueio de filamentos), todos preservados sem alterações de configuração. Os **1.308 JSONs idênticos** ao Orca não foram duplicados. Consultar `models-official-v02090170.json` para cobertura e SHA-256.

**Não ativar nem extrair sobre os perfis Orca automaticamente:** a compatibilidade do parser e do pipeline Bambu Studio com a versão Orca do Filuna não passou em testes de fatiamento, e um teste anterior produziu `Vendor bundle contains invalid presets`. O pacote não inclui o código de rede proprietário BambuSource nem planner OEM integral. A escolha de impressora só pode liberar funções validadas para modelo/firmware, não apenas porque um JSON existe.

**Proveniência:** `bambulab/BambuStudio`, recursos `resources/profiles/BBL` e `resources/printers` presentes no binário oficial 02.09.01.70, com revisão/licença por arquivo pendente antes da distribuição. Este arquivo foi integrado à branch de QA somente para análise/regressão; não é release. Base Orca sem duplicação de motor.
