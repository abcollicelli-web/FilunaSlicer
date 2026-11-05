#!/usr/bin/env bash
set -euo pipefail
[[ $EUID -eq 0 ]] || { echo 'Execute: sudo bash setup_bridge.sh'; exit 1; }
command -v ip >/dev/null || { echo 'iproute2 necessário'; exit 1; }
if ip route show 10.77.0.0/24 | grep -q .; then
  echo 'Rede 10.77.0.0/24 já possui rota: não alterei nada. Verifique conflitos.'; exit 1
fi
ip link add br-filuna-lab type bridge
ip addr add 10.77.0.1/24 dev br-filuna-lab
ip link set br-filuna-lab up
create_ns () {
  local ns="$1" short="$2" addr="$3"
  ip netns add "$ns"
  ip link add "vh${short}" type veth peer name "vn${short}"
  ip link set "vh${short}" master br-filuna-lab
  ip link set "vh${short}" up
  ip link set "vn${short}" netns "$ns"
  ip -n "$ns" link set lo up
  ip -n "$ns" addr add "$addr/24" dev "vn${short}"
  ip -n "$ns" link set "vn${short}" up
}
create_ns flab-cc2 c2 10.77.0.11
create_ns flab-bambu bm 10.77.0.12
create_ns flab-moon mr 10.77.0.13
create_ns flab-sdcp sd 10.77.0.14
printf '\nFiluna LAN Lab criada (somente bridge interna, sem NAT):\n'
ip -br addr show br-filuna-lab
printf 'CC2 .11 | Bambu .12 | Moonraker .13 | SDCP .14\n'