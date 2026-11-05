# FILUNA SLICER — CONFORMIDADE LEGAL, NORMAS E CERTIFICAÇÕES INTERNACIONAIS

**Contrato:** FLN-COMP-2026-11-05.1 · **Data da pesquisa:** 05/11/2026 · **Escopo inicial:** Filuna Slicer Windows e serviços associados oferecidos na União Europeia, com análise de outros mercados antes da expansão.  
**Estado:** enquadramento preliminar com referências primárias, NÃO parecer jurídico, laudo ou certificado de conformidade. **Certificação e marca CE não são concedidas por checklist interno**. Uma avaliação por profissional/organismo competente será bloqueadora se a legislação aplicável exigir.

## 1. Classificação exata: lei, norma, boa prática ou certificação

| Referência | Natureza e estado em 05/11/2026 | Impacto provável no Filuna e portão |
|---|---|---|
| **AGPL-3.0** [GNU](https://www.gnu.org/licenses/agpl-3.0.en.html) | Licença copyleft do Orca e derivados | **OBRIGAÇÃO DE LICENÇA**: manter créditos, avisos, código-fonte correspondente e obrigações de distribuição/uso em rede conforme análise de componentes e ligação; revisar cada OEM/file. G0/G5 |
| **GDPR — Reg. (UE) 2016/679** [EUR-Lex](https://eur-lex.europa.eu/eli/reg/2016/679/oj) e [EDPB art.25](https://www.edpb.europa.eu/documents/guideline/guidelines-42019-on-article-25-data-protection-by-design-and-by-default_en) | Lei vigente de proteção de dados | **OBRIGAÇÃO POTENCIALMENTE APLICÁVEL** a contas, IP/serial, logs, câmera, pagamento, documentos, IA e Hub; privacy-by-design/default, minimização, bases legais, direitos do titular, retenção, transferências e DPIA quando houver alto risco. G0/G1/G3/G5 |
| **Cyber Resilience Act — Reg. (UE) 2024/2847** [Comissão](https://digital-strategy.ec.europa.eu/en/policies/cyber-resilience-act), [EUR-Lex](https://eur-lex.europa.eu/EN/legal-content/summary/horizontal-cybersecurity-requirements-for-products-with-digital-elements-cyber-resilience-act.html) | Lei da UE para produtos com elementos digitais; categoria de fabricante/comercialidade a determinar | **NOTIFICAÇÃO já aplicável desde 11/09/2026** a fabricantes abrangidos; obrigações principais a partir de **11/12/2027**. Fazer decisão de enquadramento, risk assessment, suporte/vulnerabilidades, documentação técnica e processo de conformidade. G0/G5/G6 |
| **CRA – comunicação de vulnerabilidade** [Comissão](https://digital-strategy.ec.europa.eu/en/policies/cra-reporting), [ENISA](https://www.enisa.europa.eu/topics/product-security/vulnerability-services/eu-incident-response-and-cyber-crisis-management/single-reporting-platform-srp/frequently-asked-questions) | Para fabricantes abrangidos: alerta inicial até **24 h**, notificação completa até **72 h**, relatório final por prazos legais | Regra operacional de resposta a vulnerabilidades/incident reporting, com responsável e clock de notificação. Não esperar 2027 se categoria de fabricante já abrangida. G0/G6 |
| **CRA e software livre** [Comissão](https://digital-strategy.ec.europa.eu/en/policies/cra-open-source) | Software open-source comercialmente disponibilizado pode estar no escopo; distinguem-se fabricante e steward | AGPL não é isenção automática para Filuna distribuído em atividade comercial. Definir papel jurídico antes de venda/branding. G0 |
| **CE marking** [Comissão](https://single-market-economy.ec.europa.eu/single-market/goods/ce-marking_en) | Só quando produto estiver sujeito a norma que exige CE | **NÃO adicionar selo CE no instalador por estética**. Para escopo CRA, concluir avaliação de conformidade, documentação, declaração UE de conformidade e eventual organismo notificado conforme classificação e fase aplicável. Não afirmar certificação já obtida. G5/G6 |
| **EU AI Act — Reg. (UE) 2024/1689** [Comissão](https://digital-strategy.ec.europa.eu/en/faqs/transparency-obligations-under-article-50-ai-act) | Transparência do art.50 em aplicação desde **02/08/2026**; obrigações variam com papel/uso | A IA de ajuda/geração deve informar interação com IA quando cabível, não se passar por humano, identificar conteúdo sintético quando exigido, documentar papéis provider/deployer e modelo. Exigir análise de risco por funcionalidade, não declarar “alto risco” sem base. G1/G3/G5 |
| **Data Act — Reg. (UE) 2023/2854** [EUR-Lex](https://eur-lex.europa.eu/EN/legal-content/summary/rules-on-fair-access-to-and-use-of-data-data-act.html) | Aplicável desde 12/09/2025, com regras específicas para dados de produtos conectados | **ANALISAR PAPEL**: usuário de impressora/terceiro autorizado pode ter direitos de acesso e partilha; não presumir que Filuna tem direito irrestrito à API OEM ou que vira fabricante da impressora. G0/G1 |
| **Diretiva (UE) 2019/770** [EUR-Lex](https://eur-lex.europa.eu/eli/dir/2019/770/oj) | Regras de conformidade de conteúdo/serviço digital ao consumidor, implementação nacional | Assinatura, funcionalidades anunciadas, atualizações, compatibilidade, garantia e remédios devem ser revisados conforme país do cliente. G1/G6 |
| **European Accessibility Act — Diretiva (UE) 2019/882** [EUR-Lex](https://eur-lex.europa.eu/eli/dir/2019/882/oj) | Acessibilidade de produtos/serviços especificados, em aplicação geral desde 28/06/2025, com exceções | Determinar se portal de assinatura/e-commerce ou serviços oferecidos caem no escopo; **não presumir que todo slicer desktop isolado se enquadra**. G0/G1 |
| **LGPD — Lei 13.709/2018** [Planalto](https://www.planalto.gov.br/ccivil_03/_ato2015-2018/2018/lei/L13709.htm) | Brasil, se houver tratamento de dados sob condições de aplicação | Adequar tratamento de dados de clientes brasileiros, contratos, direitos e transferências; revisão por jurisdição. G0/G5 |
| **Direitos de autor/marcas/licenças de OEM** | Legalmente aplicáveis conforme arquivo, API, país e contrato | Proveniência, SPDX, SBOM, NOTICE, nomes de marcas, uso de SDK e redistribuição de plugin; proibir cópia sem licença compatível. G0/G5 |
| **Termos de serviços de fabricantes/marketplaces e PCI** | Contratos e requisitos de pagamento conforme função | Login OEM e ML via fluxo autorizado; tokens em vault; checkout hospedado; revisão de PCI DSS se o Filuna tocar dados de cartão. Não prometer ausência de obrigações PCI só por usar gateway. G1/G3 |

## 2. Normas técnicas: referências para **projetar qualidade**, não certificados existentes

| Norma ou framework | Objetivo no Filuna | Papel e evidencia esperada |
|---|---|---|
| **ISO 9001:2026** [ISO](https://www.iso.org/quality-management/iso-9001-2026) | Sistema de gestão da qualidade, documentos, responsabilidades, ações corretivas, auditorias e melhoria | **Referência voluntária**; edição **2026**, já publicada em setembro e substitui 2015. Certificação futura por terceira parte se decidida. |
| **ISO/IEC 25010:2023** [ISO](https://www.iso.org/standard/78176.html) | Modelo de qualidade do produto de software com nove características | Mapear requisitos mensuráveis de funcionalidade, segurança, confiabilidade, performance, usabilidade, compatibilidade, manutenibilidade e flexibilidade; adoção interna, não selo. |
| **ISO/IEC 27001:2022** [ISO](https://www.iso.org/standard/27001) | SGSI para operações, Hub, segredos, backups, incidentes e fornecedores | Implantar processos e evidências; certificação organizacional só com auditoria independente/acreditada. |
| **NIST SP 800-218 SSDF v1.1** [NIST](https://csrc.nist.gov/pubs/sp/800/218/final) | Desenvolvimento seguro de código, integridade de build, vulnerabilidades e provenance | PO/PS/PW/RV rastreados; **rev. 1 v1.2 é draft em 05/11/2026**, não citar como final. |
| **IEC 62443-4-1:2018** [IEC](https://webstore.iec.ch/en/publication/33615) | Práticas de ciclo de vida seguro de produtos que interagem com sistemas de automação | **Referência técnica condicional** apropriada a controles de máquinas/dispositivos; não afirmar que o slicer está legalmente sujeito a certificação IEC 62443. |
| **ISO/IEC 29147:2018** [ISO](https://www.iso.org/standard/72311.html) | Divulgação coordenada de vulnerabilidades | Política de contato, triagem, resposta e comunicação a usuários/OEM. |
| **ISO/IEC 30111:2019** [ISO](https://www.iso.org/standard/69725.html) | Tratamento/gestão de vulnerabilidades | Processo com ciclo de severidade, patch, revisão, SLA e regressão. |
| **OWASP ASVS 5.0.0** [OWASP](https://asvs.owasp.org/) | Segurança do Hub, APIs, login, export e console administrativo | Aplicar nível adequado por serviço; não é certificação normativa UE. |
| **OWASP SAMM** [OWASP](https://owaspsamm.org/) | Maturidade do processo de desenvolvimento de segurança | Revisão progressiva, evidências de controles; opcional. |
| **WCAG 2.2** [W3C](https://www.w3.org/TR/WCAG22/) | UX acessível, alto contraste, teclado, foco e erros | Padrão de qualidade/integração às exigências aplicáveis ao portal; versão AA como alvo de design quando possível. |
| **ISO/IEC/IEEE 12207:2017** [ISO](https://www.iso.org/standard/63712.html) | Ciclo de vida de software e engenharia de processos | Referência voluntária para processos, rastreabilidade, manutenção e descontinuação. |
| **SLSA / SPDX** [slsa.dev](https://slsa.dev/), [SPDX](https://spdx.dev/) | Proveniência de cadeia de suprimentos, build reproducível e SBOM | Metas internas verificáveis, sem selo implícito. |

**Importante:** não reproduzir integralmente textos pagos da ISO/IEC. Matriz usa descrição pública, e o mapeamento fino exige acesso licenciado às normas. **Ninguém pode anunciar “ISO certified” com mera execução dos nossos testes**.

## 3. Política de segurança e proteção do usuário

### 3.1 Proteção do cliente e impressora
- Uma impressora detectada não é uma impressora autorizada. Pareamento por dispositivo e firmware, autenticação legítima, capabilities e ACK explícito antes de qualquer envio.
- Comandos físicos remotos (iniciar, parar, pausar, aquecer, mover, filamento, pular objeto) exigem confirmação apropriada, validação de limites do equipamento, estado e logging sanitizado.
- Nunca instalar firmware ou fazer sondagens de Internet por padrão; não expor porta pública, não contornar fabricante nem usar SDK não licenciado.
- A existência de recursos de câmera, movimentação e controle pode criar riscos físicos e de privacidade. Threat model por capacidade/firmware e análise de medidas de segurança (incluindo a documentação do fabricante) antes do piloto.
- **Não confundir conformidade do software com certificação da impressora ou das peças impressas**. Para uma peça destinada a uso crítico (médico, alimentar, estrutural etc.), validar normas específicas fora do escopo padrão.

### 3.2 Privacidade, contas, dados e telemetria
- Inventário \`data_category × purpose × legal_basis × retention × processing_location × controller/processor × data_subject_rights × subprocessors\`, antes de qualquer sync/log/chat/câmera.
- Câmera **desligada por padrão**; sem upload/recording na nuvem sem autorização e finalidade; apagar cache e impedir replay indevido. Telemetria opt-in quando apropriado, minimizada, pseudonimizada, com coleta de falhas sem fotos/modelos/tokens.
- Segredos em armazenamento seguro, tokens por escopo e revogação; não enviar chave LAN para o Hub. Política de retenção proporcional, revogação de acesso, portabilidade, exclusão e backup testados.
- GDPR exige avaliar se DPIA é necessária conforme risco; no caso de monitoramento visual remoto, acesso de terceiros e IA em escala, análise deve ser documentada, mesmo quando se conclua por não fazer DPIA.
- Assistente IA: consentimento e informação de tratamento; nunca enviar projeto STL/3MF/foto privada para terceiro sem autorização e política clara.
- Auditoria de compras/assinaturas/contratos e cobranças com segregação de dados financeiros e controle de incidentes.

### 3.3 Segurança de software, supply-chain e resposta a vulnerabilidades
- Definir responsável pela segurança e canal de divulgação \`SECURITY.md\`; endereço de contato público e processo de resposta coordenada.
- Revisão de dependências incluindo \`deps_src\`, OpenCascade, FFmpeg, WebView2, CPython/plugins, libs de fabricantes; SBOM por build com licença, versão e vulnerabilidades conhecidas.
- CI com SAST, unitários, sanitizers, fuzzing de parser de 3MF/JSON/SDCP/Discovery, SCA, secret scanning, assinatura e attestation de artefato.
- Pacotes e manifests assinados, hash, TLS, rollback e proteção contra pacote malicioso; updater não pode executar arquivo não homologado.
- Responsável jurídico/security deve determinar, **antes do release UE**, papel CRA fabricante ou steward, classe do produto, eventuais avaliações por organismo e requisito de CE. Se fabricante abrangido, a obrigação de reportar incidente/vulnerabilidade ativou em 11/09/2026; criar relógio de 24/72h e finais.
- Atualização de segurança e suporte documentados conforme lei/regime e duração de suporte; os contratos e sites devem refletir promessas realistas.

## 4. Processo para certificações e avaliações (não confundir)

| Fase | Resultado |
|---|---|
| **C0 — Enquadramento** | identificar entidade jurídica, produto, mercado, papéis de fabricante/fornecedor/operador, regimes aplicáveis; revisão jurídica assinada |
| **C1 — Gap analysis** | mapa de cada obrigação/cláusula aplicável e evidência existente/faltante, responsáveis e riscos |
| **C2 — Requisitos e implementação** | criar REQ e casos de teste; threat model, segurança, privacidade, documentação e procedimentos |
| **C3 — Auditoria interna** | avaliar por pessoa diferente do autor; 100% dos testes aplicáveis PASS, pendências e desvios documentados |
| **C4 — Avaliação externa** | apenas se legalmente exigida ou escolhida (certificação ISO voluntária, corpo notificado CRA se aplicável etc.); receber laudo verdadeiro |
| **C5 — Declaração / marca** | publicar declarações de conformidade ou marcas SOMENTE com base em processo legal completado; proibir logos falsos |
| **C6 — Manutenção** | vigilância, atualizações de norma/lei, gestão de vulnerabilidades, renovação/auditorias periódicas e reavaliação em mudanças relevantes |

## 5. Portões de teste legais e normativos

| ID | Obrigação ou boa prática | Evidência de PASS |
|---|---|---|
| CMP-001 | AGPL/licença OEM | SPDX/SBOM/NOTICE, fontes correspondentes e revisão jurídica por arquivo |
| CMP-002 | CRA enquadramento fabricante/OSS | parecer escrito, avaliação de risco, reporte e plano conforme escopo |
| CMP-003 | CRA incidentes 24/72h | tabletop exercise com relógio simulado e fluxo para plataforma ENISA |
| CMP-004 | GDPR privacy by design | inventário, bases legais, direitos, retenção, segredos, tests de exclusão e DPIA screening |
| CMP-005 | Câmera e telemetria | nenhum stream/log privado sem consentimento/necessidade comprovada |
| CMP-006 | IA Act art.50 | identificar tarefas elegíveis, aviso de IA/conteúdo quando aplicável |
| CMP-007 | Consumidor / assinatura | condições digitais, atualizações, cancelamento e comunicação verificáveis |
| CMP-008 | Segurança de código | NIST SSDF PO/PS/PW/RV com links de testes |
| CMP-009 | Serviços e API | OWASP ASVS proporcional: auth, multi-tenant, injeção, sessão e rate limits |
| CMP-010 | Produto | métricas ISO/IEC 25010:2023 com baselines e limites aprovados |
| CMP-011 | SGSI | controles ISO/IEC 27001:2022 conforme decisão; sem alegar certificado |
| CMP-012 | Qualidade | governança ISO 9001:2026, CAPA e revisão de melhoria contínua |
| CMP-013 | Acessibilidade | matriz EAA por tipo de serviço, WCAG/EN quando aplicável, evidências de UX |
| CMP-014 | Dados conectados | avaliação Data Act por papel, direitos/acesso OEM e contratos |
| CMP-015 | Distribuição | assinatura real de código, identidade Filuna, fonte e segurança do instalador |
| CMP-016 | Marcas/certificação | sem uso de CE ou logos ISO não autorizados; evidência de certificação externa quando alegada |

**Critério:** todos os CMP aplicáveis ao **escopo real e ao mercado de lançamento** precisam ter PASS no gate; pendência legal bloqueia distribuição. Uma norma de referência voluntária pode ser adotada como requisito interno e passar no QA interno, **mas nunca gerar afirmação de certificação oficial** sem auditoria externa.

## 6. Plano de implantação legal por dependência

**P0 (antes de API/dispositivo novo):** inventário de componentes, avaliação de licença AGPL e OEM, threat model, privacidade e modelo de autoridade.  
**P1 (antes de conta/IA/Hub):** GDPR, terms/privacy/DPA quando aplicável, consentimentos, auth/tenant, AI transparency e contratos de serviços terceiros.  
**P2 (antes do alpha Windows conectado):** segurança do update, pipeline e SBOM, threat models por marca, telemetria e incident response.  
**P3 (antes de disponibilizar ao mercado UE):** CRA classificação e reporte, conformidade com consumo/assinatura, declaração e CE se efetivamente exigidos, distribuidor/editor verdadeiro, aprovação formal.  
**P4 (contínuo):** monitor de mudanças de lei, standards, firmware, vulnerabilidades, contratos e suporte.

## 7. Declaração de situação atual

- **Nenhuma certificação ISO, CE, CRA, IEC, PCI ou aprovação de laboratório foi apresentada como obtida.**
- Esta é uma pesquisa documental de fontes oficiais para **o plano**, não um parecer definitivo sobre incidência legal. As obrigações variam conforme modelo de comercialização, entidade jurídica, dados processados, componentes fornecidos e país.
- A meta de dezembro/2026 não remove as obrigações CRA que começaram em setembro/2026, quando aplicáveis.
- O relatório de auditoria deve registrar revisão externa pendente; nunca marcar \`CMP-002\` ou \`CMP-016\` PASS por simples existência desta tabela.

## Fontes primárias consultadas em 05/11/2026

Comissão Europeia, ENISA, EUR-Lex, EDPB, ISO, IEC, NIST, OWASP e W3C. URLs nos quadros acima. Revalidar atualidade e aplicação antes de cada release — inclusive alterações legais futuras.
