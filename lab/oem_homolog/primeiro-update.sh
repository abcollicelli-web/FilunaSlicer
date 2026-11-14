#!/usr/bin/env bash
# Primeiro teste do auto-update (14/11): publica o build completo como base e, por cima, o update r1
# (suporte remoto com autorização — só telas, sem compilar). Rodar quando o run do build completo passar.
#   bash primeiro-update.sh 43630918204 1784dfe41
set -euo pipefail
export HOME=/root
RUN="${1:?run}"; BUILD="${2:?build (9 primeiros do commit)}"
R=abcollicelli-web/FilunaSlicer
[ "$(gh run view "$RUN" -R $R --json conclusion -q .conclusion)" = "success" ] || { echo "RUN_NAO_PASSOU"; exit 1; }
N=$(gh run view "$RUN" -R $R --json number -q .number)
cd /srv/filuna-homolog/repo
python3 lab/oem_homolog/publicar_app.py --run "$RUN" --build "$BUILD" --versao "2.5.0-beta.$N" \
  --nota "Atualização automática do Filuna Slicer" --nota "Diagnóstico de erros automático" \
  --nota "Conexão de impressoras: o perfil é instalado ao conectar" --nota "QIDI e Prusa no assistente de conexão" \
  --interno "update base Filuna" --canal nenhum
python3 lab/oem_homolog/publicar_app.py --base "$BUILD" --sobrepor /srv/filuna-homolog/overlay/r1 --build "$BUILD-r1" --versao "2.5.0-beta.$N.1" \
  --nota "Suporte remoto: você autoriza, escolhe o prazo e encerra quando quiser" \
  --nota "Histórico do que o suporte fez fica visível para você" \
  --interno "update de recursos internos" --interno "update de segurança" --canal beta
cat /var/lib/filuna-v2/app/canal.json; echo
ls -la /var/lib/filuna-v2/app/docs | grep "$BUILD" ; ls -la /var/lib/filuna-v2/app/pacotes | grep "${BUILD}__"
