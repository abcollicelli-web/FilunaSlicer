#!/usr/bin/env python3
"""Filuna — resolve os perfis oficiais de um fabricante (herança 'inherits') sem alterar nenhum valor.

Lê a pasta resources/profiles do PRÓPRIO binário oficial (nunca uma cópia editada) e devolve,
para cada impressora selecionável do catálogo (instantiation=true), o trio
máquina/processo/filamento padrão que o fatiador oficial usa ao escolher essa impressora.

O resultado efetivo é a fusão da cadeia de herança, do ancestral mais distante até a folha,
exatamente como o fatiador faz ao carregar um preset de sistema. Só os campos de identidade
('inherits', 'renamed_from') são removidos; nenhum parâmetro de fatiamento é tocado.
"""
from __future__ import annotations

import json
from pathlib import Path


class Catalogo:
    def __init__(self, profiles_dir: Path):
        self.raiz = profiles_dir
        self.indice: dict[tuple[str, str], tuple[Path, dict]] = {}
        self.vendor_de: dict[tuple[str, str], str] = {}
        for vendor_json in sorted(profiles_dir.glob("*.json")):
            vendor = vendor_json.stem
            pasta = profiles_dir / vendor
            if not pasta.is_dir():
                continue
            for arq in pasta.rglob("*.json"):
                try:
                    dados = json.loads(arq.read_text(encoding="utf-8-sig"))
                except (ValueError, UnicodeDecodeError):
                    continue
                if not isinstance(dados, dict):
                    continue
                tipo, nome = dados.get("type"), dados.get("name")
                if tipo not in ("machine", "process", "filament", "machine_model") or not nome:
                    continue
                chave = (tipo, nome)
                # o primeiro vendor que define o nome vence, como o fatiador (OrcaFilamentLibrary/comuns por último)
                if chave not in self.indice or vendor not in ("OrcaFilamentLibrary",):
                    self.indice.setdefault(chave, (arq, dados))
                    self.vendor_de.setdefault(chave, vendor)

    def efetivo(self, tipo: str, nome: str) -> tuple[dict, list[str]]:
        """Fusão da herança. Devolve (config efetiva, cadeia de nomes da folha até a raiz)."""
        cadeia, visto = [], set()
        atual = nome
        while atual:
            if atual in visto:
                raise ValueError(f"HERANCA_CICLICA {tipo} {atual}")
            visto.add(atual)
            item = self.indice.get((tipo, atual))
            if not item:
                raise ValueError(f"PAI_AUSENTE {tipo} {atual} (de {nome})")
            cadeia.append((atual, item[1]))
            atual = item[1].get("inherits") or None
        efetivo: dict = {}
        for _, dados in reversed(cadeia):
            efetivo.update(dados)
        efetivo.pop("inherits", None)
        efetivo.pop("renamed_from", None)
        efetivo["from"] = "system"
        efetivo["name"] = nome
        return efetivo, [n for n, _ in cadeia]

    def impressoras(self, vendor: str | None = None) -> list[dict]:
        """Impressoras selecionáveis + processo e filamento padrão de cada uma."""
        saida = []
        for (tipo, nome), (arq, dados) in sorted(self.indice.items()):
            if tipo != "machine" or str(dados.get("instantiation", "true")).lower() != "true":
                continue
            if vendor and self.vendor_de.get((tipo, nome)) not in vendor.split(","):
                continue
            try:
                maq, _ = self.efetivo("machine", nome)
            except ValueError as e:
                saida.append({"maquina": nome, "erro": str(e)})
                continue
            proc = maq.get("default_print_profile") or ""
            fil = maq.get("default_filament_profile") or []
            if isinstance(fil, str):
                fil = [fil]
            fil0 = fil[0] if fil else ""
            notas = []
            # O padrão citado pela máquina às vezes não existe no catálogo do próprio fabricante
            # (ex.: Snapmaker U1 "0.20 Standard" × arquivo "0.20mm Standard"). O fatiador então
            # seleciona outro preset compatível; repetimos isso de forma explícita e registrada.
            if not self._resolve("process", proc):
                alt = self._compativel("process", nome, proc)
                notas.append(f"PADRAO_OEM_AUSENTE process '{proc}' -> '{alt}'")
                proc = alt or ""
            if not self._resolve("filament", fil0):
                alt = self._compativel("filament", nome, fil0)
                notas.append(f"PADRAO_OEM_AUSENTE filament '{fil0}' -> '{alt}'")
                fil0 = alt or ""
            saida.append({"maquina": nome, "processo": proc, "filamento": fil0, "notas": notas,
                          "modelo": maq.get("printer_model", ""), "bico": maq.get("nozzle_diameter", [""]),
                          "extrusoras": len(maq.get("nozzle_diameter") or [1]) if isinstance(maq.get("nozzle_diameter"), list) else 1,
                          "arquivo": str(arq.relative_to(self.raiz))})
        return saida

    def _resolve(self, tipo: str, nome: str) -> bool:
        if not nome:
            return False
        try:
            self.efetivo(tipo, nome)
            return True
        except ValueError:
            return False

    def _compativeis(self, tipo: str) -> dict[str, list[str]]:
        cache = self.__dict__.setdefault("_cache_compat", {})
        if tipo not in cache:
            m = {}
            for (t, n), (_, d) in self.indice.items():
                if t != tipo or str(d.get("instantiation", "true")).lower() != "true":
                    continue
                try:
                    ef, _ = self.efetivo(t, n)
                except ValueError:
                    continue
                cp = ef.get("compatible_printers") or []
                m[n] = cp if isinstance(cp, list) else [cp]
            cache[tipo] = m
        return cache[tipo]

    def _compativel(self, tipo: str, maquina: str, pedido: str) -> str | None:
        cands = sorted(n for n, cp in self._compativeis(tipo).items() if maquina in cp)
        if not cands:
            return None

        def chave(x: str) -> str:
            return "".join(x.lower().replace("mm", "").split())
        for c in cands:
            if pedido and chave(c) == chave(pedido):
                return c
        return cands[0]

    def gravar_trio(self, item: dict, destino: Path) -> dict[str, Path]:
        destino.mkdir(parents=True, exist_ok=True)
        caminhos = {}
        for tipo, nome in (("machine", item["maquina"]), ("process", item["processo"]), ("filament", item["filamento"])):
            efetivo, cadeia = self.efetivo(tipo, nome)
            p = destino / f"{tipo}.json"
            p.write_text(json.dumps(efetivo, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            caminhos[tipo] = p
            caminhos[tipo + "_cadeia"] = cadeia
        return caminhos

    def auditoria(self) -> dict:
        """Integridade do catálogo: pais ausentes e ciclos, por tipo."""
        erros, contagem = [], {}
        for (tipo, nome) in self.indice:
            contagem[tipo] = contagem.get(tipo, 0) + 1
            if tipo == "machine_model":
                continue
            try:
                self.efetivo(tipo, nome)
            except ValueError as e:
                erros.append(str(e))
        return {"contagem": contagem, "erros": sorted(set(erros))}


if __name__ == "__main__":
    import sys
    c = Catalogo(Path(sys.argv[1]))
    print(json.dumps(c.auditoria(), indent=1, ensure_ascii=False)[:3000])
    for i in c.impressoras(sys.argv[2] if len(sys.argv) > 2 else None)[:400]:
        print(i)
