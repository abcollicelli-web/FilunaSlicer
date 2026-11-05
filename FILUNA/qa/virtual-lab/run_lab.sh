#!/usr/bin/env bash
set -euo pipefail
[[ $EUID -eq 0 ]] || { echo 'Execute: sudo bash run_lab.sh'; exit 1; }
DIR="$(cd "$(dirname "$0")" && pwd)"
pids=()
cleanup () { for pid in "${pids[@]}"; do kill "$pid" 2>/dev/null || true; done; wait || true; }
trap cleanup EXIT INT TERM
ip netns exec flab-cc2 python3 -u "$DIR/lab.py" cc2 --ip 10.77.0.11 & pids+=("$!")
ip netns exec flab-bambu python3 -u "$DIR/lab.py" bambu --ip 10.77.0.12 & pids+=("$!")
ip netns exec flab-moon python3 -u "$DIR/lab.py" moonraker --ip 10.77.0.13 & pids+=("$!")
ip netns exec flab-sdcp python3 -u "$DIR/lab.py" sdcp --ip 10.77.0.14 & pids+=("$!")
printf '\nLab ativo. Em outro terminal, execute o Filuna no Wine e use Procurar Impressoras.\n'
printf 'CC2: 10.77.0.11:1883 / access-code 123456 | Moonraker: 10.77.0.13:7125\n'
printf 'Bambu e SDCP: apenas descoberta nesta primeira versão. Ctrl+C encerra os emuladores.\n\n'
wait