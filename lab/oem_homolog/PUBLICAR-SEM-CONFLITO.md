# Publicar e compilar sem derrubar outra sessão (regra do dono, 15/11/2026)

Várias conversas do Claude trabalham no mesmo ramo do cliente (`filuna/v1-teste-fisico`) e publicam no mesmo canal.
Em 15/11 uma sessão (câmera) enviou commit e compilação no meio da 65 de outra; publicar a build mais antiga por
último teria tirado do canal o trabalho já publicado. Daqui em diante:

## O que o `publicar_app.py` faz sozinho
1. **Trava**: uma publicação por vez (`/var/lib/filuna-v2/app/.publicar.lock`). A segunda para com
   "RECUSADO: outra publicação está em andamento" (saída 3) e diz qual build está publicando.
2. **Sem retrocesso**: o commit do build novo tem de conter o commit que está no canal (compare do GitHub:
   `ahead` ou `identical`). Senão: "RECUSADO … não contém esse commit" (saída 4).
3. **Versão só sobe**: maior que a do canal e nunca repetida com outro build (saída 4).
4. **Registro**: `/var/lib/filuna-v2/app/publicacoes.log`, uma linha JSON por publicação (build, versão, anterior, run).
`--forcar-retrocesso` passa por cima de 2 e 3: só com ordem expressa do dono.

## O que cada sessão tem de fazer
- **Antes de commitar no ramo**: `git fetch` e juntar o que outra sessão enviou (nunca `push --force`).
- **Nunca cancelar compilação de outra sessão.** O workflow `filuna-windows-rapido.yml` tem
  `concurrency … cancel-in-progress: false`: a nova fica na fila. Cancelar só run que for seu (confira o `headSha`).
- **Publicar só o build mais novo do ramo.** Se outra sessão já enviou commit depois do seu, o seu build não é o
  último: não publique; o build da outra (que contém o seu) é que vai. Se o seu contém o dela, avise a outra para
  não publicar o dela.
- **Número da versão**: o próximo depois do canal (`cat /var/lib/filuna-v2/app/canal.json` + manifesto do build).
  Se o script recusar, leia o motivo; não force.
- **Pasta de compilação Linux** (`/srv/filuna-homolog/build/teste-fisico`): antes de copiar arquivos, `pgrep -af ninja`
  e `find … -mmin -15`; guarde cópia do que trocar.
- **Site** (`publicar.sh`): uma sessão por vez; confira `git status` do worktree e releases recentes antes.

## Update menor (diferença binária, desde 15/11/2026)
O `publicar_app.py` gera, para os 3 builds anteriores mais recentes (`--patches`), também o pacote `<de>__<para>.zp`:
arquivo grande que mudou (o `OrcaSlicer.dll`, 117 MB) vai como diferença `zstd --patch-from` (~9 MB em vez de ~44 MB).
O site só entrega esse pacote a quem pede `?patch=zstd` (Filuna Slicer 2.5.0-beta.67 em diante); os demais recebem o
pacote normal. Se a diferença falhar no cliente (arquivo instalado diferente, SHA-256), ele volta sozinho ao normal.
Canal `todos`: só pelo admin do site (`/admin/atualizacoes`); `--canal todos` é recusado.
