# OEM Batch GitHub Actions — laboratório R5
Os manifestos em `fabricantes/<marca>/vendor.yaml` são JSON válido como YAML, para leitura Python sem PyYAML. Números SHA256 desconhecidos ficam `null`; nunca são fabricados.

`filuna_batch_test.py --all --platform ubuntu|windows --output ...` percorre as oito marcas, independentemente das falhas. Não normaliza uma linha até que a divergência apareça em oficial A×B e esteja incluída no manifesto com prova da marca. Todo PASS histórico é identificado como histórico.

O `filuna_batch_test` C++ apenas confirma vínculo das bibliotecas (via `--list`). **Não é um validador T3–T9**. As outras sete bibliotecas C++ são placeholders explicitamente inoperantes: sua compilação não aprova um protocolo. O SDK OEM, os perfis por marca, os portáteis com SHA e os simuladores precisam ser adicionados e validados.

Não colocar segredos e projetos particulares em arquivos abertos. 3MF privados somente `.gpg` com secret disponibilizado em Actions. Sem credenciais de Bambuddy neste workflow.

Sem dependência do Ubuntu de produção; containers simuladores devem ser fixados por digests e validados antes de habilitar o contrato. Artefatos de QA são únicos outputs.
