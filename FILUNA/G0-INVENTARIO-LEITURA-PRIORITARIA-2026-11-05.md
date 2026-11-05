# FILUNA G0 — Inventário de leitura prioritária (lote 2026-11-05 Europe/Rome)

**ID** FLN-G0-READ-2026-11-05.2  
**Gate** G0 EM ANDAMENTO — esta leitura **não** marca G0 PASS.  
**Base normativa final confirmada no GitHub:** `filuna/quality-master-20261105` @ `d1935aefa91a1eda86f57b67b342a3857247294e` (PR #2 draft).  
**Compatibilidade com anúncio do dono (t18u):** verificado em `FILUNA/CRONOGRAMA-DEPENDENCIAS.json` → **52** nós, **149** referências de dependência, **SRV-00…SRV-11** (12 etapas); `FILUNA/AUDITORIA-PLANO-MESTRE-2026-11-05.md` → auditoria documental **113/113** (estática; **≠** G0/QA funcional/Windows/hardware). Nenhuma tip mais nova que `d1935aef` existia no remoto no momento desta leitura.

> Leitura **integral** das 13 fontes abaixo (conteúdo baixado via API GitHub raw e conferido com `git hash-object` = blob SHA do [MANIFESTO-FONTES-LEGADO-2026-11-05.md](MANIFESTO-FONTES-LEGADO-2026-11-05.md)). **Não** afirmar que as 141 fontes do manifesto foram todas lidas.

## 1. Fontes lidas nesta passagem (13/13 do lote §6)

| Repo | Caminho | Blob SHA | Bytes | Status leitura |
|---|---|---|---:|---|
| S | `Filuna-Slicer/04-MOTORES.md` | `f54595583b42566efb544a6a3c94c8a3329319d8` | 5443 | INTEGRAL |
| S | `Filuna-Slicer/14-CONTINUIDADE-INTEGRAL.md` | `7e08a5d2ffbd76112208188833cd838521af4df6` | 27602 | INTEGRAL |
| S | `Filuna-Slicer/18-HOME-GROK-2026-11-03.md` | `d0172ef0ec69474bab142e0ba03f8b10bc9b2074` | 2631 | INTEGRAL |
| S | `Filuna-Slicer/25-REGRA-MESTRE-ATUALIZACOES-2026-11-03.md` | `f2fe986e6e5a497ea650dec63da5a835c093e30c` | 8276 | INTEGRAL |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md` | `6723b31d8ddbaa0d6ef249323edfd1a129d19ccc` | 15849 | INTEGRAL |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-PREPARAR.md` | `ced8d3822d044005abdf81001e2889377e0764c0` | 17280 | INTEGRAL |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-DISPOSITIVOS.md` | `49536aa0bea3ec612c1ca6816d3380e2c040a682` | 13130 | INTEGRAL |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-PARAMETRICOS.md` | `185a7965b74f8e901ae331f5207e04a7ae2909f2` | 14256 | INTEGRAL |
| W | `docs/GROK_CONTRACT.md` | `ceef090a3e9232380fb4d22ced79295caa399805` | 1174 | INTEGRAL |
| W | `docs/ENGINE_CONTRACT.md` | `b490b2d6ab1317ecedf8638c814ddd25faa8d77c` | 4811 | INTEGRAL |
| W | `docs/CONTINUAR-PROJETO-FILUNA-SLICER.md` | `e69e17aa52b309a1698d32f8ae4af99cc1be6374` | 29563 | INTEGRAL |
| W | `docs/PROTOCOLOS-IMPRESSORAS.md` | `c4b6fe6496ca4a37e71f7bcd3565bcdebf28f1bc` | 17735 | INTEGRAL |
| W | `docs/AUDITORIA-SEGURANCA-2026-11-05.md` | `a95534356a9c996dcac5c4cd762a21941ef211bb` | 7102 | INTEGRAL |

**Não lidas neste lote:** as outras **128** entradas do manifesto 141 (61 S + 80 W − 13). Continuam PENDING no G0.

## 2. REQ extraídos (imutáveis de intenção; implementação no fork F = futuro)

IDs novos deste lote; classe A=obrigatório produto, B=importante, C=proposta UI ainda não homologada no fork C++.

| REQ | Classe | Fonte | Texto condensado | Precedência no fork F |
|---|---|---|---|---|
| REQ-G0-ENG-01 | A | S `04-MOTORES` + W `ENGINE_CONTRACT` | Quatro motores Orca+Prusa+Cura+Kiri como backends | **SUPERADO** por D-FLN-ENGINE-05NOV / plano final: **Orca único**; adapters OEM ≠ segundo motor de fatiamento |
| REQ-G0-ENG-02 | A | W `ENGINE_CONTRACT` | Não recriar fatiador; 1 job → 1 motor; preservar perfis; falha honesta se preset ausente; `--arrange 0` / multiplaca | Mantém-se para **Orca nativo** no FilunaSlicer |
| REQ-G0-CORE-01 | A | S `14-CONTINUIDADE` | Orca livre offline mesmo sem assinatura; não misturar site legado / DB admin antigo | Mantém |
| REQ-G0-CORE-02 | A | S `14` / W `CONTINUAR` | Novo produto isolado; não copiar Windows/site antigos como base de código | Mantém; base atual = fork C++ `FilunaSlicer` |
| REQ-G0-HOME-01 | A | S `18-HOME` + `ESTRUTURA-HOME…` | Home Grok 03/11 aprovada (barra 56px, lateral 208px, faixas Recentes/Modelos/Ferramentas) | **Reimplementar** em C++/wx; HTML C# é evidência, não código a colar |
| REQ-G0-HOME-02 | B | S `18` | Print Chromium ≠ EXE Windows; fatiamento físico ainda não homologado naquele marco | Mantém honestidade de evidência |
| REQ-G0-UI-PREP-01 | C | S `MESTRE-PREPARAR` | Contrato PROPOSTO da tela Preparar; fatiar = Orca real; esboço não gera G-code | Só G1+ após SPEC no fork |
| REQ-G0-UI-DEV-01 | C | S `MESTRE-DISPOSITIVOS` | Contrato PROPOSTO; esboço **não** fala com impressora; comandos só após confirmação | DeviceBridge futuro; sem falso ACK |
| REQ-G0-UI-PAR-01 | C | S `MESTRE-PARAMETRICOS` | Galeria/item PROPOSTO; gerador Filuna ≠ Orca; `arvore-porta-bottons` fora | Onda 10+ |
| REQ-G0-UPD-01 | A | S `25-REGRA-MESTRE-ATUALIZACOES` | Classes A/B/C/D; instalador só se indispensável; ficha+SHA+aprovação dono; Grok não publica | Mantém |
| REQ-G0-CAT-01 | A | S `14` | Catálogo print-only: sem export STL/3MF mestre | Mantém |
| REQ-G0-PEI-01 | A | S `14` / W `ENGINE` | Mesma cor/material por PEI; Z=0; multiplaca; QA real | Mantém |
| REQ-G0-PRT-01 | A | W `PROTOCOLOS-IMPRESSORAS` | Dois canais: Filuna lê status/câmera/material; Orca `host_type` imprime; descoberta por assinaturas de porta | Informa DeviceBridge; validar licença antes de copiar OEM |
| REQ-G0-PRT-02 | A | W `PROTOCOLOS` | Plugin fechado (Bambu/Flashforge/ElegooLink) → LAN/docs; não empacotar binário proprietário sem direito | Alinhado AGPL do plano |
| REQ-G0-SEC-01 | A | W `AUDITORIA-SEGURANCA` | Diagnósticos sanitizados; sem paths/tokens; classe D targeted updates recusada; evidência Hub legado | Reaplicar princípios no Hub novo (SRV-*), sem portar segredos |
| REQ-G0-GROK-01 | B | W `GROK_CONTRACT` | Grok escreve só em `Filuna/…/ATUALIZACOES`; não scrape do site Grok | Mantém para docs S |
| REQ-G0-SRV-01 | A | Plano final + `ARQUITETURA-SERVIDOR` + cronograma | Prioridades dono: (1) fundação domínio/HTTPS/DB/Filuna ID → (2) sync/backup/restore → (3) diagnósticos **sem controle admin** → (4) portal cliente só próprias impressoras | Mapear a **SRV-00…04** etc.; impressão remota default OFF até após LAN física |

## 3. Conflitos / superação (G0)

| ID | Conflito | Decisão vigente |
|---|---|---|
| CF-01 | Quatro motores (S/W) vs Orca único (05/11) | **SUPERADO** — Orca integral; OEM = DeviceBridge |
| CF-02 | Orca como EXE externo (legado W 0.1.x) vs fork nativo C++ | **Produto atual F** = Orca embutido no FilunaSlicer; legado = referência |
| CF-03 | Paleta Home 03/11 (`#64184a`…) vs amostras JPEG 05/11 | **CONFLITANTE PARCIAL** — tokens finais exigem aprovação; não implementar G1 visual ainda |
| CF-04 | Admin updates monitora 4 engines | Ajustar mentalmente a **Orca + OEMs**; sem auto-update de motor |
| CF-05 | Continuity 02/11 pede repo privado novo | **Histórico** — repo público AGPL `FilunaSlicer` é a base autorizada agora |

## 4. Servidor no G0 (sem provisionar)

Nós `SRV-00`…`SRV-11` presentes no DAG final. Para inventário G0 basta registrar:

- Admin Filuna: erros sanitizados / saúde — **proibido** comando, câmera ou impressão nas máquinas dos clientes (dono t18u + arquitetura servidor).
- Portal cliente: ver **próprias** impressoras e histórico — leitura, não controle neste lote.
- Impressão remota: futuro, feature flag OFF, depende de homologação LAN física + gates SRV.

**Não feito aqui:** domínio, HTTPS, banco, secrets, deploy, gasto, impressão remota.

## 5. Assets de marca (bloqueio G1 — busca read-only)

Esperados ([IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md) / `brand/REFERENCIAS-MANIFESTO.json`):

| ID | Arquivo | Tipo | SHA-256 | No GitHub FilunaSlicer / Filuna / filuna-slicer / attachments agente |
|---|---|---|---|---|
| FLN-BRAND-LOGO-01 | `105207.jpg` | JPEG 1536×1031 | `4533476b…80639` | **AUSENTE** |
| FLN-BRAND-PALETTE-01 | `105206.jpg` | JPEG 1536×1031 | `2f35535f…2c33e` | **AUSENTE** |
| FLN-BRAND-ICON-01 | `105208.jpg` | JPEG 1536×1031 | `770810b2…8c245` | **AUSENTE** |
| FLN-MASCOT-CIRCLE-01 | `99422.jpg` | JPEG 1536×1536 | `f5474437…c0d11` | **AUSENTE** |
| FLN-MASCOT-CUTOUT-01 | `99421.webp` | WebP RGBA 360×443 | `6ed33f99…7f8a5` | **AUSENTE** |

Somente manifesto JSON presente. Logos Orca/SoftFever no tree **não** substituem Filuna. Destino previsto: `FILUNA/brand/assets/original/` após o dono fornecer os bytes.

## 6. O que ainda pode avançar sem os assets

- Continuar G0: ler mais fontes do manifesto 141; fechar REQ/conflitos; SBOM inicial do tree F.
- Manter checks estáticos; documentar DeviceBridge **somente** em SPEC (sem código UI).
- Preparar lista de perguntas de firmware/assinatura **sem** implementar G1.

## 7. Próxima ação exata

1. Reler [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md) + tip quality.  
2. Ler próximo lote do manifesto (sugerido S: `09-SEGURANCA`, `10-APIS`, `11-ACESSO-POS-ASSINATURA`, `12-REFERENCIA-VISUAL`, `20-MODELOS`, `LEIA-PRIMEIRO-GROK`, `ESTADO-DO-PROJETO`; W: `UPDATE-RELEASE-POLICY`, `PLATE-LAYOUT-CONTRACT`, `SECURE-CATALOG-PRINT-CONTRACT`, `FILUNA-ID`, `HOMOLOGACAO-SITE`, `PENDENCIAS`).  
3. **Só o dono:** anexar/commitar os 5 originais de marca (ou indicar URL/path autorizado com bytes conferíveis).  
4. Não Home/DeviceBridge/código motor; não merge PR #1/#2/#3; não provisionar SRV.
