"""Farm local do Filuna Slicer: lê o arquivo do projeto como o site lê (farm_arquivos.abrir + analisar_modelos).
Uso: python -I analisar.py <arquivo> <pasta de saída>
Saída: <pasta>/analise.json ({"ok": true, "pecas": [...], ...} ou {"ok": false, "erro": "..."}) e os modelos em <pasta>/modelos/.
A foto de cada peça vai em base64 (campo "foto")."""
import base64
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
entrada, saida = Path(sys.argv[1]), Path(sys.argv[2])
os.environ.setdefault("FILUNA_TRABALHO", str(saida / "tmp"))
saida.mkdir(parents=True, exist_ok=True)
from farmapp import farm_arquivos   # noqa: E402
from farmapp.fatiamento import motor   # noqa: E402

try:
    modelos = farm_arquivos.abrir(entrada.name, entrada.read_bytes())
    analise = farm_arquivos.analisar_modelos(modelos)
    (saida / "modelos").mkdir(exist_ok=True)
    for nome, dados in modelos:
        alvo = (saida / "modelos" / nome).resolve()
        if not str(alvo).startswith(str((saida / "modelos").resolve())):
            continue
        alvo.parent.mkdir(parents=True, exist_ok=True)
        alvo.write_bytes(dados)
    for p in analise["pecas"]:
        p["foto"] = base64.b64encode(p["foto"]).decode("ascii") if p.get("foto") else None
    resultado = {"ok": True, **analise}
except (farm_arquivos.ErroArquivo, motor.ErroFatiamento) as exc:
    resultado = {"ok": False, "erro": str(exc)}
except Exception as exc:   # arquivo estranho: mensagem curta, sem travar o programa
    resultado = {"ok": False, "erro": f"Não foi possível ler esse arquivo ({type(exc).__name__})."}
(saida / "analise.json").write_text(json.dumps(resultado, ensure_ascii=False), encoding="utf-8")
