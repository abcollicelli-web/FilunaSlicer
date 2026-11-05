#!/usr/bin/env bash
set -euo pipefail
[[ $EUID -eq 0 ]] || { echo 'Execute: sudo bash cleanup_bridge.sh'; exit 1; }
for n in flab-cc2 flab-bambu flab-moon flab-sdcp; do
  if ip netns list | grep -Eq "^${n}( |$)"; then ip netns del "$n"; fi
done
ip link del br-filuna-lab 2>/dev/null || true
echo 'Rede virtual Filuna removida. Nenhum serviço público foi alterado.'