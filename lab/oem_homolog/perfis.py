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
    # Bibliotecas comuns de que os presets de qualquer marca podem herdar (fatiadores base Orca).
    COMUNS = ("OrcaFilamentLibrary",)

    def __init__(self, profiles_dir: Path):
        self.raiz = profiles_dir
        # (vendor, tipo, nome) -> (arquivo, dados): o fatiador resolve 'inherits' DENTRO do pacote
        # do próprio fabricante (depois nas bibliotecas comuns); nomes iguais em outras marcas
        # (ex.: fdm_machine_common) não se misturam.
        self.por_vendor: dict[tuple[str, str, str], tuple[Path, dict]] = {}
        self.indice: dict[tuple[str, str], tuple[Path, dict]] = {}
        self.vendor_de: dict[tuple[str, str], str] = {}
        for vendor_json in sorted(profiles_dir.glob("*.json")):
            vendor = vendor_json.stem
            pasta = profiles_dir / vendor
            if not pasta.is_dir():
                continue
            for arq in sorted(pasta.rglob("*.json")):
                try:
                    dados = json.loads(arq.read_text(encoding="utf-8-sig"))
                except (ValueError, UnicodeDecodeError):
                    continue
                if not isinstance(dados, dict):
                    continue
                tipo, nome = dados.get("type"), dados.get("name")
                if not tipo:
                    # alguns presets oficiais (Flashforge C5) não trazem "type"; o fatiador o tira
                    # da lista do <Vendor>.json, que aponta para a pasta process/filament/machine
                    partes = arq.relative_to(pasta).parts
                    tipo = next((t for t in ("process", "filament", "machine") if t in partes), None)
                if tipo not in ("machine", "process", "filament", "machine_model") or not nome:
                    continue
                self.por_vendor.setdefault((vendor, tipo, nome), (arq, dados))
                self.indice.setdefault((tipo, nome), (arq, dados))
                self.vendor_de.setdefault((tipo, nome), vendor)

    def _busca(self, vendor: str | None, tipo: str, nome: str):
        if vendor:
            for v in (vendor, *self.COMUNS):
                item = self.por_vendor.get((v, tipo, nome))
                if item:
                    return v, item
            return None, None
        item = self.indice.get((tipo, nome))
        return (self.vendor_de.get((tipo, nome)), item) if item else (None, None)

    def efetivo(self, tipo: str, nome: str, vendor: str | None = None) -> tuple[dict, list[str]]:
        """Fusão da herança. Devolve (config efetiva, cadeia de nomes da folha até a raiz).
        Com 'vendor', a cadeia é resolvida no pacote dessa marca (+ bibliotecas comuns)."""
        if vendor is None:
            vendor = self.vendor_de.get((tipo, nome))
        cadeia, visto = [], set()
        atual = nome
        while atual:
            if atual in visto:
                raise ValueError(f"HERANCA_CICLICA {tipo} {atual}")
            visto.add(atual)
            _, item = self._busca(vendor, tipo, atual)
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
        for (vend, tipo, nome), (arq, dados) in sorted(self.por_vendor.items()):
            if tipo != "machine" or str(dados.get("instantiation", "true")).lower() != "true":
                continue
            if vendor and vend not in vendor.split(","):
                continue
            try:
                maq, _ = self.efetivo("machine", nome, vend)
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
            for tipo_p in ("process", "filament"):
                atual = proc if tipo_p == "process" else fil0
                motivo = None
                if not self._resolve(tipo_p, atual, vend):
                    motivo = "PADRAO_OEM_AUSENTE"
                elif not self._aceita(tipo_p, atual, nome, vend):
                    motivo = "PADRAO_OEM_INCOMPATIVEL"
                if motivo:
                    alt = self._compativel(tipo_p, nome, atual, vend)
                    notas.append(f"{motivo} {tipo_p} '{atual}' -> '{alt}'")
                    if tipo_p == "process":
                        proc = alt or ""
                    else:
                        fil0 = alt or ""
            saida.append({"maquina": nome, "vendor": vend, "processo": proc, "filamento": fil0, "notas": notas,
                          "modelo": maq.get("printer_model", ""), "bico": maq.get("nozzle_diameter", [""]),
                          "extrusoras": len(maq.get("nozzle_diameter") or [1]) if isinstance(maq.get("nozzle_diameter"), list) else 1,
                          "arquivo": str(arq.relative_to(self.raiz))})
        return saida

    def _resolve(self, tipo: str, nome: str, vendor: str | None = None) -> bool:
        if not nome:
            return False
        try:
            self.efetivo(tipo, nome, vendor)
            return True
        except ValueError:
            return False

    def _aceita(self, tipo: str, nome: str, maquina: str, vendor: str | None) -> bool:
        """O preset declara a máquina como compatível? Lista vazia = compatível com todas;
        só com condição (expressão) não avaliamos aqui e aceitamos, como no padrão do perfil."""
        ef, _ = self.efetivo(tipo, nome, vendor)
        cp = ef.get("compatible_printers") or []
        cp = cp if isinstance(cp, list) else [cp]
        return not cp or maquina in cp

    def _compativeis(self, tipo: str, vendor: str | None) -> dict[str, list[str]]:
        cache = self.__dict__.setdefault("_cache_compat", {})
        if (tipo, vendor) not in cache:
            m = {}
            for (v, t, n), (_, d) in self.por_vendor.items():
                if t != tipo or (vendor and v not in (vendor, *self.COMUNS)):
                    continue
                if str(d.get("instantiation", "true")).lower() != "true":
                    continue
                try:
                    ef, _ = self.efetivo(t, n, v)
                except ValueError:
                    continue
                cp = ef.get("compatible_printers") or []
                m[n] = cp if isinstance(cp, list) else [cp]
            cache[(tipo, vendor)] = m
        return cache[(tipo, vendor)]

    def _compativel(self, tipo: str, maquina: str, pedido: str, vendor: str | None = None) -> str | None:
        cands = sorted(n for n, cp in self._compativeis(tipo, vendor).items() if maquina in cp)
        if not cands:
            return None

        def chave(x: str) -> str:
            return "".join(x.lower().replace("mm", "").split())
        for c in cands:
            if pedido and chave(c) == chave(pedido):
                return c
        # o fatiador prefere a variante do mesmo preset para a máquina ("Snapmaker PLA" ->
        # "Snapmaker PLA @U1 ..."), depois o mesmo material; só então o primeiro em ordem
        base = pedido.split("@")[0].strip().lower() if pedido else ""
        for c in cands:
            if base and c.lower().startswith(base):
                return c
        for mat in ("pla", "petg", "abs"):
            if mat in base.split():
                for c in cands:
                    if mat in c.lower().split("@")[0].split():
                        return c
        return cands[0]

    def gravar_trio(self, item: dict, destino: Path) -> dict[str, Path]:
        destino.mkdir(parents=True, exist_ok=True)
        caminhos = {}
        for tipo, nome in (("machine", item["maquina"]), ("process", item["processo"]), ("filament", item["filamento"])):
            efetivo, cadeia = self.efetivo(tipo, nome, item.get("vendor"))
            p = destino / f"{tipo}.json"
            p.write_text(json.dumps(efetivo, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            caminhos[tipo] = p
            caminhos[tipo + "_cadeia"] = cadeia
        return caminhos

    def auditoria(self) -> dict:
        """Integridade do catálogo: pais ausentes e ciclos, por tipo."""
        erros, contagem = [], {}
        for (vend, tipo, nome) in self.por_vendor:
            contagem[tipo] = contagem.get(tipo, 0) + 1
            if tipo == "machine_model":
                continue
            try:
                self.efetivo(tipo, nome, vend)
            except ValueError as e:
                erros.append(f"[{vend}] {e}")
        return {"contagem": contagem, "erros": sorted(set(erros))}


if __name__ == "__main__":
    import sys
    c = Catalogo(Path(sys.argv[1]))
    print(json.dumps(c.auditoria(), indent=1, ensure_ascii=False)[:3000])
    for i in c.impressoras(sys.argv[2] if len(sys.argv) > 2 else None)[:400]:
        print(i)
