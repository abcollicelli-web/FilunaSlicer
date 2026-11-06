# REGRA PRIORITÁRIA FILUNA — 100% MÓDULO OEM DE REDE + 100% MÓDULO OEM DE FATIAMENTO
**Decisão expressa do proprietário em 06/11/2026. Aplicável a TODAS as marcas e modelos.**
**Estado:** REGRA APROVADA. Implementação completa e homologação ainda pendentes.

## Um Orca, dois módulos OEM completos e separados
- Orca é a **única base compartilhada do Filuna**: estruturas comuns, formatos, GUI e infraestrutura que não diferem por fabricante. Filuna mantém a interface visual e a organização; **não incorpora os programas oficiais completos, não instala oito fatiadores, não duplica quatro motores Orca**.
- **Dispositivos = 100% do módulo funcional OEM de REDE** para aquele fabricante/modelo/firmware: descoberta, identificação, pareamento, autenticação exigida, estado real, comunicação, envio e confirmação conforme os recursos suportados. Selecionar a marca manualmente ou detectar na rede ativa automaticamente esse módulo de rede; cadastrar e atrelar impressora com identificador estável. Nenhum pedaço genérico do Orca pode substituir uma diferença OEM exigida para reconhecer/conectar/operar com segurança.
- **Fatiar/Preparar = 100% do módulo funcional OEM de FATIAMENTO** adequado à impressora vinculada em Dispositivos: algoritmos, planner de trajetórias, macros G-code, presets e heranças completas, firmware/capabilities, movimento/limites, temperatura, calibração, purga/AMS/multimaterial e parâmetros que diferem do Orca, quando aplicáveis. Seleção de impressora registrada carrega automaticamente todas as diferenças necessárias, não apenas presets, logger ou funções isoladas. Tudo o que for igual ao Orca continua sendo executado pelo **único** motor compartilhado. Não prometer equivalência quando existir diferença ainda não portada.
- **A palavra “100%” refere-se à cobertura integral das funções OEM necessárias em cada subsistema, não ao uso de bibliotecas proprietárias indisponíveis nem à duplicação de código comum**. Se alguma função OEM não puder ser legal ou tecnicamente incorporada, ela permanece explicitamente BLOQUEADA e o módulo não é anunciado como completo/homologado; não contornar autenticação nem controles de firmware.

## Fluxo obrigatório

```text
Interface Filuna
  ├─ Dispositivos
  │   └─ Marca escolhida OU rede detectada
  │       └─ fabricantes/<marca>/transport/   [MÓDULO OEM REDE COMPLETO]
  │           └─ reconhecer/parear/autenticar/atrelar dispositivo
  │
  └─ Fatiar/Preparar
      └─ impressora registrada escolhida
          └─ fabricantes/<marca>/engine/ + profiles/ + gcode/ + modelos/
              [MÓDULO OEM FATIAMENTO COMPLETO, específico para modelo/firmware]
                  └─ engine Orca compartilhado (apenas partes comuns)
                      └─ G-code compatível → envio via módulo OEM REDE
```

## Gates que provam os dois “100%”
**Rede:** trace real no processo Filuna + protocolo do fabricante (descoberta/pareamento/autenticação/telemetria/upload/ACK, negativa de credencial, timeouts, desconexão) contra simulador representativo e depois firmware/hardware real. **Fatiamento:** trace real na aplicação Filuna carregando o módulo OEM integral, hash do preset efetivo e dos componentes, novo build Windows, mesmo modelo/posição/material e comparação de G-code com binário oficial preservado, incluindo geometria, comandos essenciais e tempo. Um PASS de SSDP, seleção `N2S` ou helper C++ não comprova rede OEM completa nem slicing OEM completo.

## Bambu A1 — estado no momento da decisão
- Código de identificação `N2S` e SSDP mock: PASS isolado, **não 100% rede**, pois autenticação MQTT/TLS, FTPS, upload e ACK pelo Filuna não foram executados.
- `BambuA1Module::activate_for_prepare()` ainda sinaliza `MathCandidateOnly`, sem planner do Bambu executado no Orca: **não 100% fatiamento**.
- Novo fatiamento com Bambu Studio oficial e presets herdados + macros A1 produziu 343563 bytes, 19m19s; Filuna anterior produziu 372272 bytes, 19m53s, **diferença 28709 bytes e 34s**. Isso é uma comparação de binários anteriores à implementação plena do módulo; não homologar.
- Teste isolado no Bambu Studio oficial indicou que `precise_outer_wall=1` aumentou o arquivo de 343563 para 354275 bytes (+10712). Não afirmar que esse único parâmetro elimina todas as diferenças.
- **Próxima tarefa de execução:** incorporar os deltas completos e legalmente reutilizáveis da rede e do planner Bambu A1 ao Filuna, compilar, provar ativação real e repetir G-code oficial × Filuna; anotar número de bytes de módulos e tamanho final do pacote. Caso impossível, informar bloqueio concreto, jamais promover falsamente “100%”.

## Atualização de fontes
Bambu Studio/Orca/Anycubic/Flashforge/Elegoo e outros atualizados → comparar commits, blobs e dependências upstream com a versão pinada; portar todos os novos deltas necessários por fabricante, executar ambos conjuntos de testes e publicar somente módulos versionados/aprovados pelo proprietário. Preservar obrigações de licenças e avisos de fonte. Só usar update interno de código se o pacote for verificável, assinado, com rollback e estado de projeto preservado.
