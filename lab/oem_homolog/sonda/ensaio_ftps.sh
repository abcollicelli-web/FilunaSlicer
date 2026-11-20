#!/bin/bash
# Compila o ensaio e roda o envio novo e o antigo contra o emulador Bambu, no fechamento normal e no da A1.
# Uso: BOOST_INC=<cabeçalhos do Boost 1.84> bash ensaio_ftps.sh <checkout do FilunaSlicer> <checkout da versão da loja> [pasta]
set -u
REPO=$1; VELHO=$2; T=${3:-/tmp/ensaio-ftps}
mkdir -p $T && cd $T
awk '/^bool FilunaBambuLanAgent::upload_ftps/,/^int FilunaBambuLanAgent::start_local_print/' \
    $VELHO/src/slic3r/Utils/FilunaBambuLanAgent.cpp | sed '$d' > corpo_velho.inc
cp $REPO/src/slic3r/Utils/FilunaFtps.hpp $REPO/lab/oem_homolog/sonda/ensaio_ftps.cpp .
g++ -std=c++17 -O1 -I. -I${BOOST_INC:-/usr/include} ensaio_ftps.cpp -o teste_ftps -lssl -lcrypto -lpthread 2> compilar.log \
    || { tail -30 compilar.log; exit 1; }
head -c 3500000 /dev/urandom > peca.3mf

emulador() { # porta modo
    local pasta=$T/sd-$1; rm -rf $pasta; mkdir -p $pasta
    python3 $REPO/lab/oem_homolog/emuladores/bambu_lan.py --ip 127.0.0.1 --ftps $1 --mqtt $(( $1 + 1 )) \
        --ssdp $(( $1 + 2 )) --ssdp-destino 127.0.0.1 --codigo 12345678 --pasta $pasta $2 > $T/emu-$1.log 2>&1 &
    echo $! > $T/emu-$1.pid; sleep 2
}
parar() { kill $(cat $T/emu-$1.pid) 2>/dev/null; }
conferir() { # porta
    local f=$T/sd-$1/teste.3mf
    [ -f $f ] && cmp -s $f peca.3mf && echo "   arquivo na impressora: igual ao enviado" || echo "   arquivo na impressora: FALTANDO ou diferente"
}

echo "1) envio antigo, impressora com fechamento normal (o que o emulador fazia)"; emulador 39901 ""
./teste_ftps velho 39901 peca.3mf 12345678; conferir 39901; parar 39901
echo "2) envio antigo, impressora com fechamento da A1"; emulador 39911 --fechamento-a1
./teste_ftps velho 39911 peca.3mf 12345678; conferir 39911; parar 39911
echo "3) envio novo, fechamento normal"; emulador 39921 ""
./teste_ftps novo 39921 peca.3mf 12345678 2>/dev/null; conferir 39921; parar 39921
echo "4) envio novo, fechamento da A1"; emulador 39931 --fechamento-a1
./teste_ftps novo 39931 peca.3mf 12345678 2>&1 | sed 's/^/   /'; conferir 39931; parar 39931
echo "5) envio novo, código de acesso errado"; emulador 39941 ""
./teste_ftps novo 39941 peca.3mf 99999999 2>/dev/null; parar 39941
echo "6) envio novo, impressora que aceita a conexão e não responde (prazo de 3 s)"
python3 -c "
import socket,time
s=socket.create_server(('127.0.0.1',39951)); c,_=s.accept(); time.sleep(30)" & MUDO=$!; sleep 1
./teste_ftps novo 39951 peca.3mf 12345678 3 2>/dev/null; kill $MUDO 2>/dev/null
echo "7) envio novo, cliente cancela enquanto a impressora não responde"
python3 -c "
import socket,time
s=socket.create_server(('127.0.0.1',39961)); c,_=s.accept(); time.sleep(30)" & MUDO=$!; sleep 1
./teste_ftps novo-cancela 39961 peca.3mf 12345678 20 2>/dev/null; kill $MUDO 2>/dev/null
echo "8) envio novo, ninguém na porta"
./teste_ftps novo 39971 peca.3mf 12345678 3 2>/dev/null
