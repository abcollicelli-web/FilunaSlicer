# Módulos de fabricante

Cada módulo fica em `FILUNA/modules/<id>/` e contém:

```
FILUNA/modules/<id>/
  MODULE.md       # origem, licença, versão, mantenedor
  src/            # código de envio/controle importado
  patches/        # pontos de integração com o Orca (se houver)
  tests/
```

## Modelo de MODULE.md
| Campo | Exemplo |
|---|---|
| id | `elegoo` |
| Origem | https://github.com/ELEGOO-3D/ElegooSlicer |
| Licença | AGPL-3.0 |
| Versão/SHA de origem | `<tag ou sha>` |
| Recursos | descoberta na rede, envio, status, câmera, controle |
| Protocolo | ex.: SDCP/MQTT/HTTP |
| Impressoras | Centauri Carbon 2 |
| Confirmação de impressão | obrigatória (diálogo Filuna) |

## Planejados
| id | Origem | Impressora de teste | Status |
|---|---|---|---|
| elegoo | ElegooSlicer | Elegoo Centauri Carbon 2 | planejado |
| anycubic | AnycubicSlicerNext | Anycubic Kobra 3 V2 | planejado |
| creality | CrealityPrint | – | futuro |
| snapmaker | Snapmaker Orca | – | futuro |

## Regras
- Um módulo nunca altera o motor de fatiamento nem os perfis do Orca.
- Para atualizar um módulo, troca-se o SHA de origem, roda-se o CI e o aceite, e só então ele é liberado.
- Código proprietário (plugins Bambu/Flashforge) não é empacotado.
