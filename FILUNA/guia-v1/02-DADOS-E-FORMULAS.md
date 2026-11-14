# 02 — Dados interligados e fórmulas

**Princípio:** um único banco local (SQLite) guarda tudo; cada ação do cliente gera um **evento**; os números das telas são **calculados a partir dos eventos**, nunca digitados em dois lugares. O servidor recebe cópia de tudo (03-SERVIDOR-E-BACKUP.md).

## 1. Onde fica o banco

- Arquivo: `<data_dir>/filuna/filuna.db` (`data_dir()` do Orca; no Windows `%APPDATA%\FilunaSlicer`).
- Biblioteca: SQLite. **Atenção: o Orca não traz SQLite** (verificado: não há `sqlite` em `deps/` nem em `src/`). Adicione a *amalgamation* oficial (`sqlite3.c` + `sqlite3.h`, domínio público, de sqlite.org) em `src/sqlite3/` como biblioteca estática própria no CMake e ligue-a só ao alvo da GUI. Registre versão e SHA-256 do arquivo baixado no PR. Modo WAL. Uma classe `FilunaDb` em `src/slic3r/Filuna/` (pasta nova) é a **única** porta de acesso; nenhuma tela faz SQL direto.
- Todo id é **UUID v4 em texto**, gerado no cliente (permite criar offline e sincronizar depois).
- Toda tabela tem: `id TEXT PRIMARY KEY`, `criado_em`, `atualizado_em` (UTC ISO-8601), `excluido INTEGER DEFAULT 0` (exclusão lógica; nunca `DELETE`), `rev INTEGER DEFAULT 0` (revisão do servidor).
- Pesos em gramas (`REAL`), tempo em segundos (`INTEGER`), dinheiro em centavos (`INTEGER`), comprimento em mm.

## 2. Tabelas

```sql
-- Impressora registrada em Dispositivos
impressora(id, nome, fabricante, modelo, numero_serie, modulo_id,
           horas_impressao_s INTEGER DEFAULT 0,   -- acumulado, ver §8
           ...)

-- Tipo de filamento (o "produto de prateleira"): marca + material + cor
filamento_tipo(id, marca, material, nome_produto, cor_nome, cor_hex,
               diametro_mm REAL DEFAULT 1.75, densidade_g_cm3 REAL,
               preset_orca TEXT)                  -- nome do preset de filamento do Orca

-- BOBINA FÍSICA: uma linha por carretel. 10 bobinas iguais = 10 linhas.
bobina(id, filamento_tipo_id, codigo TEXT UNIQUE,  -- "B-0001", sequencial por conta
       estado TEXT CHECK(estado IN ('lacrada','em_uso','vazia','descartada')),
       capacidade_g REAL NOT NULL,               -- peso líquido de fábrica (ex.: 1000)
       tara_g REAL DEFAULT 0,                    -- peso do carretel vazio
       restante_g REAL NOT NULL,                 -- SEMPRE recalculado dos eventos (§5)
       preco_centavos INTEGER DEFAULT 0, comprada_em TEXT, aberta_em TEXT,
       local TEXT,                               -- 'impressora:<id>:slot:<n>' ou 'prateleira:<texto>'
       ultimo_uso_em TEXT)

-- Evento de estoque: APENAS INSERÇÃO. É a verdade sobre o peso da bobina.
estoque_evento(id, bobina_id, tipo TEXT CHECK(tipo IN
       ('entrada','abertura','consumo','purga','falha','pesagem','ajuste','descarte')),
       delta_g REAL NOT NULL,                    -- negativo para saída
       trabalho_id TEXT, estimado INTEGER DEFAULT 0, obs TEXT)

-- Arquivo no computador do cliente. O ARQUIVO NÃO SOBE; só esta linha.
arquivo_local(id, pc_id, caminho TEXT, nome TEXT, tamanho_bytes INTEGER,
              sha256 TEXT, modificado_em TEXT, encontrado INTEGER DEFAULT 1)

-- Produto: "a mesma coisa impressa de novo" (§3)
produto(id, assinatura TEXT UNIQUE, nome TEXT, arquivo_local_id TEXT,
        miniatura BLOB, preco_venda_centavos INTEGER)

-- Trabalho de impressão: uma linha por envio/impressão
trabalho(id, produto_id, impressora_id, gcode_sha256 TEXT, gcode_arquivo_local_id TEXT,
         estado TEXT CHECK(estado IN ('fatiado','enviado','imprimindo','concluido','cancelado','falhou')),
         copias INTEGER DEFAULT 1,
         tempo_previsto_s INTEGER, tempo_real_s INTEGER,
         camadas_total INTEGER, camada_parada INTEGER,
         trocas_filamento INTEGER, purga_total_g REAL,   -- NULL = desconhecido
         iniciado_em TEXT, terminado_em TEXT)

-- Consumo previsto e real, por filamento do trabalho
trabalho_filamento(id, trabalho_id, indice INTEGER,       -- extrusor/slot 0..n
         bobina_id TEXT,                                  -- confirmada pelo cliente (§4)
         previsto_g REAL, real_g REAL, purga_g REAL, estimado INTEGER DEFAULT 0)

-- Manutenção
manutencao_regra(id, impressora_id, nome, intervalo_h REAL, feito_ate_h REAL DEFAULT 0, ativa INTEGER DEFAULT 1)
manutencao_registro(id, regra_id, horas_maquina REAL, obs TEXT)

-- Fila de sincronização (03-SERVIDOR §4)
sync_fila(seq INTEGER PRIMARY KEY AUTOINCREMENT, tabela, registro_id, op, payload_json, enviado INTEGER DEFAULT 0)
```

**Como tudo se liga:** `trabalho` aponta para `produto` e `impressora`; `trabalho_filamento` aponta para `bobina`; ao terminar, o trabalho gera `estoque_evento` (baixa da bobina) e soma `horas_impressao_s` na impressora (manutenção). Rankings, desperdício, custo e previsão são consultas sobre essas tabelas.

## 3. Produto: como saber que "é a mesma coisa"

O cliente imprimiu 3 vezes a mesma peça → 1 produto, 3 trabalhos.

**Assinatura do produto** = identidade da geometria na mesa, independente do nome do arquivo:

1. Para cada objeto da mesa (`ModelObject`), para cada volume (`ModelVolume`) na ordem do modelo: pegar a malha (`mesh().its`) e calcular `SHA-256` sobre: vértices como float32 little-endian (x,y,z em sequência), seguidos dos índices dos triângulos como int32 little-endian.
2. Juntar ao hash do objeto a escala da primeira instância: `"%.3f,%.3f,%.3f"` de `ModelInstance::get_scaling_factor()` (`src/libslic3r/Model.hpp` linha ~1499). A malha de cada volume é `ModelVolume::mesh().its` (`indexed_triangle_set`, `TriangleMesh.hpp` linha ~164: `vertices` e `indices`).
3. Assinatura do objeto = `SHA-256(hash_volumes_concatenados + "|" + escala)` em hexadecimal.
4. **Assinatura do produto** = `SHA-256` da lista das assinaturas dos objetos, **ordenada alfabeticamente** e unida por `\n`. A quantidade de cópias do mesmo objeto **não** entra (vai em `trabalho.copias`).

Regras: se a assinatura já existe em `produto`, reutilize; senão crie com `nome` = nome do projeto, ou do primeiro objeto. O cliente pode **renomear** e **unir** dois produtos (unir = apontar os trabalhos de um para o outro e marcar o primeiro `excluido=1`). Modelo do catálogo protegido do site: usar o id do catálogo como assinatura (`"catalogo:<id>"`), nunca a malha.

## 4. Bobinas: nova ou já usada? E quando há 10 iguais?

O estoque é por **bobina física**. O problema: 10 bobinas de PLA branco da mesma marca são indistinguíveis para o sistema, então o cliente precisa confirmar qual está na máquina — mas com o mínimo de cliques.

**Identidade:** ao cadastrar, cada bobina recebe `codigo` sequencial (`B-0001`, `B-0002`…) e uma etiqueta para imprimir (código + QR com `filuna://bobina/<id>`). Cadastro em lote: "10 bobinas" cria 10 linhas `lacrada` de uma vez.

**Diálogo "Confirmar bobinas" — aparece ao enviar para imprimir** (uma linha por filamento do trabalho). O sistema preenche a sugestão nesta ordem e o cliente só confirma ou troca:

1. A bobina cujo `local` é este slot desta impressora (a que já estava lá).
2. Senão, bobinas `em_uso` do mesmo `filamento_tipo`, a de `ultimo_uso_em` mais recente.
3. Senão, bobinas `lacrada` do mesmo tipo, a de `comprada_em` mais antiga (primeira que entra, primeira que sai).
4. Senão: "Nenhuma bobina cadastrada" → botões **Cadastrar bobina nova** / **Cadastrar bobina já usada**.

**Bobina nova:** `estado` passa de `lacrada` para `em_uso`, grava `aberta_em`, evento `abertura` (delta 0). `restante_g = capacidade_g`.
**Bobina já usada (sem histórico no Filuna):** perguntar o peso. Duas formas:
- "Pesei na balança": `restante_g = peso_na_balanca_g − tara_g`.
- "Não sei": cliente escolhe ¾, ½, ¼ → `restante_g = capacidade_g × fração`, com `estimado = 1` e aviso "peso estimado" na tela até a primeira pesagem.
Gravar evento `entrada` com `delta_g = restante_g`.

**Impressoras que identificam a bobina sozinhas** (ex.: AMS com RFID): se o módulo do fabricante informar um identificador de carretel, guardar em `bobina.local` e pular o diálogo para aquele slot. Nunca presumir; só quando o dado vier da máquina.

**Aviso antes de imprimir:** se `restante_g < previsto_g` de algum filamento, mostrar: "Esta bobina tem 180 g e a impressão pede 240 g." Opções: trocar bobina, pesar de novo, continuar assim mesmo.

## 5. Peso restante da bobina (fórmula única)

```
restante_g(bobina) = Σ delta_g de todos os estoque_evento da bobina (não excluídos)
```
`bobina.restante_g` é só um cache desse somatório; recalcule após cada evento. Nunca edite o campo direto.

- Se o resultado for < 0: gravar `restante_g = 0` e criar evento `ajuste` com `obs='divergência: consumo maior que o estoque'`, e pedir pesagem.
- **Pesagem:** `novo = peso_balanca − tara`; evento `pesagem` com `delta_g = novo − restante_atual`. Zera o `estimado` da bobina.
- `restante_g ≤ 0` → `estado = 'vazia'`.

**Exemplo.** Bobina B-0007, 1000 g. Eventos: entrada +1000; consumo −212,4; purga −18,0; falha −35,5; pesagem −4,1 (a balança mostrou 730 g líquidos).
`restante = 1000 − 212,4 − 18,0 − 35,5 − 4,1 = 730,0 g`.

## 6. O que extrair do G-code ao terminar (e ao fatiar)

O Orca escreve um resumo em comentários no G-code. Campos confirmados no código-fonte (`src/libslic3r/GCode.cpp`), um valor por filamento separado por vírgula quando há vários:

| Linha no G-code | Guardar em |
|---|---|
| `; filament used [g] = 12.34, 5.60` | `trabalho_filamento.previsto_g` (por índice) |
| `; filament used [mm] = …` | conferência (g = mm × área × densidade) |
| `; total filament used [g] = 17.94` | conferência: deve ser a soma da linha acima |
| `; total filament used for wipe tower [g] = 3.20` | `trabalho.purga_total_g` |
| `; total filament change = 14` | `trabalho.trocas_filamento` |
| `; estimated printing time (normal mode) = 1h 45m 3s` | `trabalho.tempo_previsto_s` |
| `; total layers count = 240` | `trabalho.camadas_total` |

Regras do leitor (`FilunaGcodeResumo` em `src/slic3r/Filuna/`):
1. Ler só comentários; o resumo fica no fim do arquivo (ler os últimos 64 KB) e, em alguns casos, no início (ler os primeiros 64 KB).
2. Tempo: converter `Xd Xh Xm Xs` em segundos: `d×86400 + h×3600 + m×60 + s`. Exemplo: `1h 45m 3s = 3600 + 2700 + 3 = 6303 s`.
3. **Campo ausente = `NULL` (desconhecido)**. Motores de fabricante (módulos) podem não escrever os mesmos campos; não invente. O servidor já segue essa regra em `site/app/price_calculator.py::parse_orca_gcode`.
4. **Verificação obrigatória antes de usar a purga (faça uma vez e registre o resultado no PR):** fatiar um cubo de 2 cores e conferir se `total filament used [g]` **já inclui** a torre de purga. Se incluir: `peça_g = total − purga`. Se não incluir: `consumo_total = total + purga`. Grave a resposta em `FILUNA/guia-v1/VERIFICACOES.md` e use a fórmula correspondente abaixo.

**Rateio da purga por filamento** (o G-code só dá o total): se há 1 filamento, toda a purga é dele. Se há vários:
`purga_g[i] = purga_total_g × previsto_g[i] / Σ previsto_g`, com `estimado = 1` nessas linhas.

## 7. Fim de impressão: o que o sistema faz, na ordem

Gatilho: o módulo da impressora informa término, cancelamento ou falha (ou o cliente marca manualmente em Histórico, quando a impressora não informa).

1. Calcular a **fração impressa** `f`:
   - concluído: `f = 1`
   - cancelado/falhou: `f = camada_parada / camadas_total` (se a impressora informar camada); senão `f = progresso_percentual / 100`; senão perguntar ao cliente ("quanto imprimiu?" 25/50/75%). Marcar `estimado = 1`.
2. Para cada filamento `i`: `real_g[i] = previsto_g[i] × f × copias_ok`, onde `copias_ok` = cópias na mesa (já está no G-code; normalmente 1).
3. Gravar eventos na bobina confirmada (§4):
   - concluído: `consumo` com `delta_g = −(real_g[i] − purga_g[i])` e `purga` com `delta_g = −purga_g[i]`.
   - cancelado/falhou: um evento `falha` com `delta_g = −real_g[i]` (tudo vira desperdício).
4. `tempo_real_s`: o que a impressora informou; senão `tempo_previsto_s × f`.
5. Somar na impressora: `horas_impressao_s += tempo_real_s` (§8).
6. Atualizar `bobina.restante_g`, `ultimo_uso_em`, `local`.
7. Enfileirar tudo em `sync_fila`.

**Exemplo completo.** Trabalho de 2 cores, G-code: `filament used [g] = 80.0, 20.0`; torre de purga 6,0 g; 14 trocas; 2h 30m; 200 camadas. Verificação do §6.4 indicou que o total inclui a torre.
- Concluído (`f=1`): purga rateada = 6×80/100 = 4,8 g e 6×20/100 = 1,2 g. Bobina A: consumo −75,2 e purga −4,8. Bobina B: consumo −18,8 e purga −1,2. Tempo real = 9000 s.
- Cancelado na camada 50 (`f=0,25`): bobina A evento falha −20,0; bobina B falha −5,0; tempo real = 2250 s; tudo marcado `estimado=1`.

## 8. Manutenção por horas de máquina

`horas_maquina = impressora.horas_impressao_s / 3600`.

Para cada `manutencao_regra` ativa:
```
usado_h  = horas_maquina − feito_ate_h
situacao = 'em dia'  se usado_h < 0,9 × intervalo_h
           'próxima' se 0,9 × intervalo_h ≤ usado_h < intervalo_h
           'vencida' se usado_h ≥ intervalo_h
faltam_h = intervalo_h − usado_h
```
Ao registrar manutenção feita: inserir `manutencao_registro` e gravar `feito_ate_h = horas_maquina`.

**Exemplo.** Regra "Limpar bico", intervalo 100 h, feita às 340 h. Máquina com 432 h: usado = 92 h → 92 ≥ 90 → **próxima**, faltam 8 h.

Regras iniciais sugeridas ao cadastrar impressora (editáveis; **são sugestões, não especificação do fabricante** — confirmar com o dono): limpar bico 100 h; lubrificar eixos 200 h; verificar correias 300 h; trocar bico 500 h.

## 9. Custo, desperdício e rankings

```
custo_por_grama(bobina)   = preco_centavos / capacidade_g
custo_material(trabalho)  = Σ_i  |delta_g dos eventos do trabalho na bobina i| × custo_por_grama(bobina i)
desperdicio_g(período)    = Σ |delta_g| dos eventos tipo 'purga' + 'falha' no período
consumo_total_g(período)  = Σ |delta_g| dos eventos tipo 'consumo' + 'purga' + 'falha'
taxa_desperdicio          = desperdicio_g / consumo_total_g        (mostrar em %)
```
**Exemplo.** Bobina de R$ 120,00 e 1000 g → 12 centavos/g. Trabalho consumiu 80 g → R$ 9,60. No mês: consumo total 2.000 g, dos quais 150 g de purga e 90 g de falhas → desperdício 240 g → 240/2000 = **12%**.

Custo completo (energia, máquina, margem): portar as fórmulas de `Filuna/site/app/price_calculator.py`; não reinventar.

Rankings (período escolhido: 7, 30, 90 dias):
- **Produtos mais impressos:** `COUNT(*)` de `trabalho` com `estado='concluido'` agrupado por `produto_id`, somando `copias`.
- **Filamento mais gasto:** `Σ |delta_g|` de eventos `consumo+purga+falha` agrupado por `filamento_tipo`.
- **Maior desperdício:** mesmo agrupamento, só `purga+falha`.

## 10. Previsão: "vai faltar filamento na semana que vem"

Por `filamento_tipo` (somando todas as bobinas daquele tipo):

```
janela_dias      = 28
gasto_janela_g   = Σ |delta_g| dos eventos consumo+purga+falha do tipo nos últimos 28 dias
media_diaria_g   = gasto_janela_g / 28
estoque_tipo_g   = Σ restante_g das bobinas do tipo com estado lacrada ou em_uso
dias_de_estoque  = estoque_tipo_g / media_diaria_g          (se media_diaria_g = 0 → "sem consumo")
necessidade_7d_g = media_diaria_g × 7
falta_g          = max(0, necessidade_7d_g − estoque_tipo_g)
alerta           = dias_de_estoque < 7                      (limite configurável)
```
Só mostrar previsão com dados mínimos: pelo menos 3 trabalhos **e** 7 dias de histórico daquele tipo; senão exibir "dados insuficientes para prever".

**Exemplo.** PLA branco: gasto em 28 dias = 2.800 g → média 100 g/dia. Estoque = 450 g → 4,5 dias → **alerta**. Necessidade em 7 dias = 700 g → faltam 250 g. Texto: "PLA branco acaba em cerca de 4 dias. Para a próxima semana faltam 250 g."

Previsão por produto (para "o que mais vende"): `media_semanal = impressões concluídas nos últimos 28 dias / 4`; filamento necessário na semana = `media_semanal × consumo médio por impressão do produto`.

## 11. Onde cada número aparece

| Tela | Bloco | Fonte |
|---|---|---|
| Estoque | tabela de bobinas, peso restante, custo, alerta de estoque baixo | `bobina`, §5, §9 (`estoque baixo` = `restante_g < 250`, configurável) |
| Estoque | cartões de totais | soma de `bobina`; valor = Σ `restante_g × custo_por_grama` |
| Meu espaço | atividade recente, meus projetos | `trabalho`, `arquivo_local` |
| Meu espaço (novo bloco "Painel") | mais impressos, filamento mais gasto, desperdício, previsão, manutenção | §8, §9, §10 |
| Dispositivo | materiais/slots, trabalho atual | `bobina.local`, `trabalho` |
| Preparação (rodapé) | peso, custo, tempo | resumo do G-code + §9 |
| Detalhe do modelo | tempo, gramas | resumo do G-code |
