# Executor de laboratório dos 8 fabricantes

A execução de Wine/Xvfb requer aprovação específica do proprietário; sem ela, não rodar. O arquivo principal a versionar é `lab/filuna_lab_run.sh`, com manifesto `lab/filuna_lab_commands.json` preenchido somente com comandos CLI oficiais confirmados por fabricante e chamadas aos módulos reais do Filuna. Não declarar resultados quando não houver artefatos ou logs.

**Situação deste registro:** especificação e gate, não execução. Ordem: Bambu, Elegoo, Flashforge, QIDI, Creality, Snapmaker, Prusa, Anycubic. Guardar logs por tentativa e SHA-256 do 3MF e G-code. Compare SHA bruto e normalizado somente timestamp; mantenha T2/T10/T11 para etapa Windows real. G-code A1 Mini auxiliar SHA 42864cc2b2dde3677bc16763599266a94e5ed8fbeaca623309a2099c0d04446f, sem promoção da A1 4 cores. Falhas históricas E-01 Creality 205 e E-02 Snapmaker sem arquivo não equivalem a nova execução.
