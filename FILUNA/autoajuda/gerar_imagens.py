"""Gera as ilustrações dos cartões da Autoajuda (resources/images/filuna_aj_<chave>.svg), estilo Filuna, 280x150."""
import math, os, sys
OUT = sys.argv[1]
INK, MAG, ORA, SOFT, PAPER, LINE, GRAY, BLUE, GREEN = "#1D1A17", "#E0186B", "#FF6B1A", "#FBE1EC", "#F6F2EB", "#D9D1C5", "#948C83", "#2563EB", "#1F8F5F"
W, H = 280, 150

def svg(corpo, fundo=True):
    bg = f'<rect width="{W}" height="{H}" rx="14" fill="{PAPER}"/>' if fundo else ""
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}">{bg}{corpo}</svg>'

def txt(x, y, t, cor=GRAY, tam=11, peso=600, anc="middle"):
    return f'<text x="{x}" y="{y}" font-family="Inter,Segoe UI,Arial" font-size="{tam}" font-weight="{peso}" fill="{cor}" text-anchor="{anc}">{t}</text>'

def mesa(x=20, y=128, w=240, cor=INK):
    return f'<rect x="{x}" y="{y}" width="{w}" height="8" rx="3" fill="{cor}"/>'

def camadas(x, y_base, w, n, h, cores=(MAG, "#EE3E85"), raio=None):
    s = ""
    for i in range(n):
        c = cores[i % len(cores)]
        r = h / 2 if raio is None else raio
        s += f'<rect x="{x}" y="{y_base - (i + 1) * h}" width="{w}" height="{h - 0.6:.1f}" rx="{r:.1f}" fill="{c}"/>'
    return s

def bico(x, y, cor=INK, quente=False):
    s = f'<rect x="{x - 14}" y="{y - 30}" width="28" height="18" rx="4" fill="{cor}"/><path d="M{x - 8} {y - 12}h16l-5 12h-6z" fill="#C9A15A"/>'
    if quente:
        s += f'<circle cx="{x}" cy="{y + 3}" r="4" fill="{ORA}" opacity=".8"/>'
    return s

def seta(x1, y1, x2, y2, cor=INK, larg=2):
    ang = math.atan2(y2 - y1, x2 - x1); a1, a2 = ang + 2.6, ang - 2.6
    return (f'<path d="M{x1} {y1}L{x2} {y2}" stroke="{cor}" stroke-width="{larg}" stroke-linecap="round"/>'
            f'<path d="M{x2} {y2}L{x2 + 8 * math.cos(a1):.1f} {y2 + 8 * math.sin(a1):.1f}M{x2} {y2}L{x2 + 8 * math.cos(a2):.1f} {y2 + 8 * math.sin(a2):.1f}" stroke="{cor}" stroke-width="{larg}" stroke-linecap="round"/>')

def termo(x, y, nivel, cor=ORA):
    return (f'<rect x="{x - 7}" y="{y}" width="14" height="70" rx="7" fill="#fff" stroke="{LINE}" stroke-width="2"/>'
            f'<rect x="{x - 3}" y="{y + 70 - nivel}" width="6" height="{nivel}" rx="3" fill="{cor}"/><circle cx="{x}" cy="{y + 74}" r="10" fill="{cor}"/>')

def velocimetro(cx, cy, r, frac, cor=MAG):
    a = math.pi * (1 - frac)
    return (f'<path d="M{cx - r} {cy}A{r} {r} 0 0 1 {cx + r} {cy}" fill="none" stroke="{LINE}" stroke-width="8" stroke-linecap="round"/>'
            f'<path d="M{cx - r} {cy}A{r} {r} 0 0 1 {cx + r * math.cos(a):.1f} {cy - r * math.sin(a):.1f}" fill="none" stroke="{cor}" stroke-width="8" stroke-linecap="round"/>'
            f'<path d="M{cx} {cy}L{cx + (r - 12) * math.cos(a):.1f} {cy - (r - 12) * math.sin(a):.1f}" stroke="{INK}" stroke-width="3" stroke-linecap="round"/><circle cx="{cx}" cy="{cy}" r="5" fill="{INK}"/>')

def vista_topo_paredes(x, y, w, h, n, cor=MAG, gap=7, interno=None):
    s = ""
    for i in range(n):
        s += f'<rect x="{x + i * gap}" y="{y + i * gap}" width="{w - 2 * i * gap}" height="{h - 2 * i * gap}" rx="{max(2, 10 - i * 2)}" fill="none" stroke="{cor}" stroke-width="4"/>'
    if interno:
        s += interno
    return s

def grade(x, y, w, h, passo, cor=GRAY, ang=45):
    linhas = ""
    for k in range(-int(h), int(w + h), passo):
        linhas += f'<path d="M{x + k} {y + h}L{x + k + h} {y}" stroke="{cor}" stroke-width="2"/>'
    return f'<clipPath id="cl{x}{y}"><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6"/></clipPath><g clip-path="url(#cl{x}{y})">{linhas}</g>'

def antes_depois(esq, dir_, rot_e, rot_d):
    return esq + dir_ + f'<path d="M140 18v112" stroke="{LINE}" stroke-width="2" stroke-dasharray="4 4"/>' + txt(70, 18, rot_e) + txt(210, 18, rot_d)

D = {}
# camadas e linhas
D["layer_height"] = antes_depois(camadas(30, 126, 80, 10, 9) + mesa(20, 126, 100), camadas(170, 126, 80, 22, 4.1) + mesa(160, 126, 100), "0,28 mm · rápido", "0,12 mm · liso")
D["initial_layer_print_height"] = mesa(30, 120, 220) + f'<rect x="60" y="106" width="160" height="14" rx="5" fill="{ORA}"/>' + camadas(60, 106, 160, 10, 7) + txt(140, 30, "1ª camada mais grossa gruda melhor") + seta(240, 112, 228, 112, ORA)
D["line_width"] = antes_depois("".join(f'<rect x="{26 + i * 18}" y="60" width="14" height="50" rx="7" fill="{MAG}"/>' for i in range(5)), "".join(f'<rect x="{164 + i * 12}" y="60" width="8" height="50" rx="4" fill="{MAG}"/>' for i in range(8)), "linha larga", "linha fina")
D["wall_loops"] = vista_topo_paredes(40, 30, 200, 100, 3, interno=grade(80, 52, 120, 56, 14)) + txt(140, 146, "3 paredes em volta do preenchimento", INK, 10)
D["top_shell_layers"] = f'<rect x="50" y="30" width="180" height="96" rx="8" fill="#fff" stroke="{LINE}" stroke-width="2"/>' + grade(56, 60, 168, 60, 14) + camadas(52, 60, 176, 4, 7, (MAG, "#EE3E85")) + mesa(40, 126, 200) + txt(140, 22, "camadas sólidas do topo", INK)
D["bottom_shell_layers"] = f'<rect x="50" y="30" width="180" height="96" rx="8" fill="#fff" stroke="{LINE}" stroke-width="2"/>' + grade(56, 36, 168, 60, 14) + camadas(52, 125, 176, 4, 7, (ORA, "#FF9450")) + mesa(40, 126, 200) + txt(140, 22, "camadas sólidas do fundo", INK)
D["top_shell_thickness"] = D["top_shell_layers"] + seta(244, 33, 244, 58, INK) + seta(244, 58, 244, 33, INK)
D["bottom_shell_thickness"] = D["bottom_shell_layers"] + seta(244, 97, 244, 122, INK) + seta(244, 122, 244, 97, INK)
D["sparse_infill_density"] = antes_depois(f'<rect x="30" y="34" width="80" height="80" rx="8" fill="#fff" stroke="{MAG}" stroke-width="4"/>' + grade(34, 38, 72, 72, 24), f'<rect x="170" y="34" width="80" height="80" rx="8" fill="#fff" stroke="{MAG}" stroke-width="4"/>' + grade(174, 38, 72, 72, 9), "15% · leve", "40% · forte")
D["sparse_infill_pattern"] = "".join(f'<rect x="{18 + i * 88}" y="34" width="70" height="70" rx="8" fill="#fff" stroke="{MAG}" stroke-width="3"/>' for i in range(3)) + grade(21, 37, 64, 64, 12) + "".join(f'<path d="M{109 + j * 12} 37l6 10-6 10 6 10-6 10 6 10-6 10" fill="none" stroke="{GRAY}" stroke-width="2"/>' for j in range(5)) + "".join(f'<polygon points="{208 + (j % 3) * 18 + (9 if (j // 3) % 2 else 0)},{46 + (j // 3) * 16} {216 + (j % 3) * 18 + (9 if (j // 3) % 2 else 0)},{50 + (j // 3) * 16} {216 + (j % 3) * 18 + (9 if (j // 3) % 2 else 0)},{58 + (j // 3) * 16} {208 + (j % 3) * 18 + (9 if (j // 3) % 2 else 0)},{62 + (j // 3) * 16} {200 + (j % 3) * 18 + (9 if (j // 3) % 2 else 0)},{58 + (j // 3) * 16} {200 + (j % 3) * 18 + (9 if (j // 3) % 2 else 0)},{50 + (j // 3) * 16}" fill="none" stroke="{GRAY}" stroke-width="2"/>' for j in range(9)) + txt(53, 122, "grade") + txt(141, 122, "giroide") + txt(229, 122, "colmeia")
D["seam_position"] = f'<ellipse cx="140" cy="40" rx="60" ry="14" fill="#fff" stroke="{LINE}" stroke-width="2"/>' + f'<path d="M80 40v70a60 14 0 0 0 120 0V40" fill="{SOFT}" stroke="{LINE}" stroke-width="2"/>' + "".join(f'<path d="M80 {52 + i * 12}a60 14 0 0 0 120 0" fill="none" stroke="{MAG}" stroke-width="2" opacity=".55"/>' for i in range(5)) + f'<path d="M168 52v58" stroke="{INK}" stroke-width="3" stroke-dasharray="4 3"/>' + seta(232, 80, 176, 80, INK) + txt(236, 74, "costura", INK, 11, 700, "start")
sup = f'<path d="M60 126V70h50v-24h70v24" fill="none"/>'
D["enable_support"] = mesa() + f'<path d="M50 60h180v18H50z" fill="{MAG}"/><rect x="120" y="78" width="40" height="48" fill="{MAG}"/>' + "".join(f'<path d="M{58 + i * 10} 78v48" stroke="{GRAY}" stroke-width="3"/>' for i in range(6)) + "".join(f'<path d="M{168 + i * 10} 78v48" stroke="{GRAY}" stroke-width="3"/>' for i in range(6)) + txt(140, 40, "suporte segura a parte que fica no ar", INK)
D["support_type"] = antes_depois(mesa(20, 126, 100) + f'<path d="M24 60h92v12H24z" fill="{MAG}"/>' + "".join(f'<path d="M{30 + i * 10} 72v54" stroke="{GRAY}" stroke-width="3"/>' for i in range(9)), mesa(160, 126, 100) + f'<path d="M164 60h92v12h-92z" fill="{MAG}"/><path d="M210 126V100M210 100L178 72M210 100L242 72M210 100v-28" stroke="{GRAY}" stroke-width="4" fill="none" stroke-linecap="round"/>', "normal (grade)", "árvore")
D["support_threshold_angle"] = mesa() + f'<path d="M70 126L70 60L200 60L160 126z" fill="{SOFT}" stroke="{MAG}" stroke-width="3"/><path d="M200 60L240 30" stroke="{MAG}" stroke-width="3"/>' + f'<path d="M160 126A40 40 0 0 1 188 100" fill="none" stroke="{ORA}" stroke-width="3"/>' + txt(206, 112, "ângulo", ORA, 11, 700, "start") + txt(140, 22, "acima do ângulo, ganha suporte", INK)
D["support_on_build_plate_only"] = mesa() + f'<rect x="60" y="40" width="160" height="22" rx="4" fill="{MAG}"/><rect x="110" y="62" width="60" height="64" fill="{SOFT}" stroke="{MAG}" stroke-width="2"/>' + "".join(f'<path d="M{68 + i * 9} 62v64" stroke="{GRAY}" stroke-width="3"/>' for i in range(4)) + "".join(f'<path d="M{180 + i * 9} 62v64" stroke="{GRAY}" stroke-width="3"/>' for i in range(4)) + txt(140, 30, "suporte só nasce da mesa", INK)
D["brim_type"] = f'<ellipse cx="140" cy="88" rx="110" ry="40" fill="{SOFT}"/>' + "".join(f'<ellipse cx="140" cy="88" rx="{110 - i * 9}" ry="{40 - i * 3.5}" fill="none" stroke="{MAG}" stroke-width="2"/>' for i in range(5)) + f'<ellipse cx="140" cy="88" rx="62" ry="22" fill="{INK}"/>' + txt(140, 22, "aba em volta da peça (vista de cima)", INK)
D["brim_width"] = D["brim_type"] + seta(202, 88, 250, 88, ORA, 3) + seta(250, 88, 202, 88, ORA, 3)
D["skirt_loops"] = f'<rect x="106" y="50" width="68" height="44" rx="10" fill="{INK}"/>' + "".join(f'<rect x="{84 - i * 9}" y="{36 - i * 7}" width="{112 + i * 18}" height="{72 + i * 14}" rx="14" fill="none" stroke="{MAG}" stroke-width="2.5"/>' for i in range(3)) + txt(140, 140, "saia: limpa o bico antes da peça", INK, 11)
D["raft_layers"] = mesa() + camadas(40, 126, 200, 3, 7, (ORA, "#FF9450")) + f'<rect x="80" y="54" width="120" height="51" rx="6" fill="{MAG}"/>' + txt(140, 40, "jangada: base embaixo da peça", INK)
for k, frac, rot in (("outer_wall_speed", .45, "parede externa: mais devagar fica mais bonita"), ("inner_wall_speed", .7, "parede interna: pode ir mais rápido"),
                     ("sparse_infill_speed", .85, "preenchimento: o mais rápido"), ("initial_layer_speed", .2, "primeira camada: devagar para grudar"),
                     ("travel_speed", .95, "deslocamento sem imprimir"), ("bridge_speed", .35, "ponte: devagar para não cair")):
    D[k] = velocimetro(140, 108, 70, frac) + txt(140, 140, rot, INK, 11)
D["default_acceleration"] = f'<path d="M30 120C90 120 110 40 250 34" fill="none" stroke="{MAG}" stroke-width="4"/>' + f'<path d="M30 30v92h222" fill="none" stroke="{LINE}" stroke-width="2"/>' + txt(150, 140, "quanto rápido chega na velocidade", INK) + txt(40, 26, "velocidade", GRAY, 10, 600, "start")
D["nozzle_temperature"] = bico(110, 76, quente=True) + f'<path d="M110 84v30" stroke="{MAG}" stroke-width="5" stroke-linecap="round"/>' + termo(190, 30, 50) + txt(212, 120, "220 °C", ORA, 14, 800, "start")
D["nozzle_temperature_initial_layer"] = bico(110, 76, quente=True) + mesa(40, 120, 150) + f'<rect x="70" y="110" width="90" height="10" rx="5" fill="{MAG}"/>' + termo(214, 30, 58) + txt(140, 22, "1ª camada um pouco mais quente", INK)
D["hot_plate_temp"] = f'<rect x="40" y="96" width="200" height="14" rx="5" fill="{INK}"/>' + "".join(f'<path d="M{70 + i * 35} 88c-6-8 6-14 0-22s6-14 0-22" fill="none" stroke="{ORA}" stroke-width="3" stroke-linecap="round"/>' for i in range(5)) + txt(140, 134, "mesa aquecida: a peça gruda e não descola", INK)
D["textured_plate_temp"] = f'<rect x="40" y="96" width="200" height="14" rx="5" fill="#5C5650"/>' + "".join(f'<circle cx="{46 + i * 8}" cy="{100 + (i % 2) * 5}" r="1.6" fill="#8F877D"/>' for i in range(24)) + "".join(f'<path d="M{70 + i * 35} 88c-6-8 6-14 0-22s6-14 0-22" fill="none" stroke="{ORA}" stroke-width="3" stroke-linecap="round"/>' for i in range(5)) + txt(140, 134, "mesa texturizada (PEI)", INK)
D["filament_flow_ratio"] = antes_depois(bico(70, 74) + f'<rect x="40" y="92" width="60" height="6" rx="3" fill="{MAG}"/>', bico(210, 74) + f'<rect x="180" y="88" width="60" height="14" rx="7" fill="{MAG}"/>', "fluxo baixo: falha", "fluxo alto: sobra")
D["filament_max_volumetric_speed"] = bico(80, 80, quente=True) + f'<path d="M80 92v16" stroke="{MAG}" stroke-width="10" stroke-linecap="round"/>' + velocimetro(196, 100, 50, .8, ORA) + txt(140, 140, "limite de plástico que o bico derrete por segundo", INK, 10.5)
for k, rot, n in (("fan_min_speed", "ventoinha mínima", 1), ("fan_max_speed", "ventoinha máxima", 3), ("overhang_fan_speed", "ventoinha em beirais", 2)):
    pas = "".join(f'<ellipse cx="{90 + 22 * math.cos(a):.1f}" cy="{76 + 22 * math.sin(a):.1f}" rx="18" ry="8" transform="rotate({math.degrees(a):.0f} {90 + 22 * math.cos(a):.1f} {76 + 22 * math.sin(a):.1f})" fill="{BLUE}" opacity=".85"/>' for a in [i * 2 * math.pi / 3 for i in range(3)])
    D[k] = f'<circle cx="90" cy="76" r="48" fill="#fff" stroke="{LINE}" stroke-width="3"/>' + pas + f'<circle cx="90" cy="76" r="8" fill="{INK}"/>' + "".join(f'<path d="M{160} {56 + i * 18}c20-6 40 6 70 0" fill="none" stroke="{BLUE}" stroke-width="3" stroke-linecap="round" opacity="{.4 + .2 * i}"/>' for i in range(n)) + txt(200, 134, rot, INK)
D["retraction_length"] = bico(110, 78) + f'<rect x="104" y="20" width="12" height="40" rx="6" fill="{MAG}"/>' + seta(140, 70, 140, 30, ORA, 3) + txt(150, 50, "puxa o filamento", ORA, 11, 700, "start") + txt(140, 136, "evita fios entre as partes", INK)
D["retraction_speed"] = D["retraction_length"].replace("puxa o filamento", "puxa rápido")
D["z_hop"] = mesa() + f'<rect x="40" y="96" width="50" height="30" rx="4" fill="{MAG}"/><rect x="190" y="96" width="50" height="30" rx="4" fill="{MAG}"/>' + f'<path d="M66 88V60H214V88" fill="none" stroke="{ORA}" stroke-width="3" stroke-dasharray="6 4"/>' + bico(214, 112) + txt(140, 50, "sobe o bico para viajar", ORA)
D["ironing_type"] = bico(150, 64, quente=True) + f'<rect x="40" y="76" width="200" height="40" rx="6" fill="{MAG}"/>' + "".join(f'<path d="M{46 + i * 8} 76v-2" stroke="{SOFT}" stroke-width="3"/>' for i in range(12)) + f'<rect x="140" y="74" width="98" height="4" rx="2" fill="#F7648F"/>' + txt(140, 136, "passa o bico de novo e alisa o topo", INK)
D["fuzzy_skin"] = antes_depois(f'<rect x="36" y="40" width="68" height="80" rx="8" fill="{MAG}"/>', f'<path d="M176 40' + "".join(f'l{3 if i % 2 else -3} 5' for i in range(16)) + f'h68' + "".join(f'l{3 if i % 2 else -3} -5' for i in range(16)) + 'z" fill="' + MAG + '"/>', "lisa", "textura áspera")
D["spiral_mode"] = f'<path d="M100 120' + "".join(f'C{100 - 6} {120 - i * 8 - 4} {180 + 6} {120 - i * 8 - 4} {180} {120 - i * 8}' for i in range(10)) + f'" fill="none" stroke="{MAG}" stroke-width="2.5"/>' + mesa(60, 120, 160) + txt(140, 22, "vaso: uma linha só, subindo em espiral", INK)
D["prime_tower_width"] = mesa() + f'<rect x="50" y="62" width="80" height="64" rx="8" fill="{MAG}"/>' + camadas(190, 126, 40, 12, 5.3, (MAG, BLUE, ORA)) + txt(210, 54, "torre", INK, 11, 700) + txt(90, 54, "peça", INK, 11, 700)
D["enable_prime_tower"] = D["prime_tower_width"]
D["flush_into_infill"] = f'<rect x="60" y="34" width="160" height="90" rx="8" fill="#fff" stroke="{MAG}" stroke-width="4"/>' + grade(66, 40, 148, 78, 12, BLUE) + txt(140, 140, "a purga da troca de cor vai para dentro da peça", INK, 10.5)
D["wall_generator"] = antes_depois(f'<path d="M30 110L70 40L110 110" fill="none" stroke="{MAG}" stroke-width="8" stroke-linejoin="round"/>', f'<path d="M170 110L210 40L250 110" fill="none" stroke="{MAG}" stroke-width="5" stroke-linejoin="round"/><path d="M184 104L210 60L236 104" fill="none" stroke="#EE3E85" stroke-width="3" stroke-linejoin="round"/>', "clássico", "Arachne (largura variável)")
D["elefant_foot_compensation"] = antes_depois(mesa(20, 126, 100) + f'<path d="M40 126h60v-6c-4-2-6-6-6-10V44H46v66c0 4-2 8-6 10z" fill="{MAG}"/>', mesa(160, 126, 100) + f'<rect x="186" y="44" width="48" height="82" fill="{MAG}"/>', "pé de elefante", "compensado")
D["xy_hole_compensation"] = antes_depois(f'<rect x="30" y="34" width="80" height="80" rx="8" fill="{MAG}"/><circle cx="70" cy="74" r="16" fill="{PAPER}"/>', f'<rect x="170" y="34" width="80" height="80" rx="8" fill="{MAG}"/><circle cx="210" cy="74" r="22" fill="{PAPER}"/>', "furo apertado", "furo na medida")
D["xy_contour_compensation"] = antes_depois(f'<rect x="38" y="42" width="64" height="64" rx="8" fill="{MAG}"/><rect x="30" y="34" width="80" height="80" rx="8" fill="none" stroke="{INK}" stroke-width="2" stroke-dasharray="4 3"/>', f'<rect x="170" y="34" width="80" height="80" rx="8" fill="{MAG}"/><rect x="170" y="34" width="80" height="80" rx="8" fill="none" stroke="{INK}" stroke-width="2" stroke-dasharray="4 3"/>', "contorno menor", "na medida")
D["print_sequence"] = antes_depois(mesa(20, 126, 100) + camadas(30, 126, 30, 6, 8) + camadas(80, 126, 30, 6, 8, (BLUE,)), mesa(160, 126, 100) + camadas(170, 126, 30, 12, 8) + camadas(220, 126, 30, 2, 8, (BLUE,)), "por camada", "peça por peça")
D["curr_bed_type"] = "".join(f'<rect x="{24 + i * 84}" y="{60}" width="72" height="44" rx="6" fill="{c}"/>' for i, c in enumerate((INK, "#5C5650", "#C9A15A"))) + txt(60, 124, "lisa (PEI)") + txt(144, 124, "texturizada") + txt(228, 124, "fria / engenharia") + txt(140, 36, "o tipo de mesa muda a temperatura", INK)


# ---------------- ilustrações da 2ª leva (todas as abas) ----------------
def peca_lado(x, y, w, h, cor=MAG):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="{cor}"/>'

D["staggered_inner_seams"] = antes_depois(
    "".join(f'<rect x="30" y="{40 + i * 14}" width="80" height="10" rx="5" fill="{MAG}"/><circle cx="96" cy="{45 + i * 14}" r="3.5" fill="{INK}"/>' for i in range(6)),
    "".join(f'<rect x="170" y="{40 + i * 14}" width="80" height="10" rx="5" fill="{MAG}"/><circle cx="{186 + (i * 23) % 60}" cy="{45 + i * 14}" r="3.5" fill="{INK}"/>' for i in range(6)),
    "costuras alinhadas", "costuras alternadas")
D["seam_gap"] = f'<path d="M140 26a44 44 0 1 1-1 0" fill="none" stroke="{MAG}" stroke-width="10" stroke-linecap="round" stroke-dasharray="256 20"/>' + seta(196, 26, 152, 26, ORA, 2) + txt(226, 30, "folga", ORA, 12, 700) + txt(140, 140, "o fim da volta para antes do começo", INK)
D["seam_slope_type"] = antes_depois(
    f'<rect x="30" y="60" width="80" height="12" rx="6" fill="{MAG}"/><rect x="30" y="76" width="80" height="12" rx="6" fill="{MAG}"/><circle cx="70" cy="66" r="7" fill="{INK}"/>',
    f'<path d="M170 88h80v-12h-60l-20 -16z" fill="{MAG}"/><path d="M170 72l20 0h60" stroke="#F7648F" stroke-width="12" stroke-linecap="round"/>',
    "costura comum (gota)", "costura em rampa")
D["wall_sequence"] = f'<rect x="40" y="40" width="200" height="80" rx="12" fill="none" stroke="{MAG}" stroke-width="7"/><rect x="54" y="54" width="172" height="52" rx="8" fill="none" stroke="#F7648F" stroke-width="7"/>' + txt(140, 85, "1º dentro  →  2º fora", INK, 13, 700) + txt(140, 140, "a parede de fora sai por último, mais bonita", GRAY, 10)
D["is_infill_first"] = f'<rect x="40" y="40" width="200" height="80" rx="12" fill="none" stroke="{MAG}" stroke-width="7"/>' + grade(50, 50, 180, 60, 12) + txt(140, 140, "o miolo é feito antes das paredes", INK)
D["ironing_pattern"] = antes_depois(f'<rect x="30" y="40" width="80" height="70" rx="6" fill="{MAG}"/>' + "".join(f'<path d="M34 {48 + i * 8}h72" stroke="#F7648F" stroke-width="3"/>' for i in range(8)),
    f'<rect x="170" y="40" width="80" height="70" rx="6" fill="{MAG}"/>' + "".join(f'<rect x="{174 + i * 7}" y="{44 + i * 7}" width="{72 - i * 14}" height="{62 - i * 14}" rx="4" fill="none" stroke="#F7648F" stroke-width="2.5"/>' for i in range(4)),
    "linhas", "concêntrico")
D["top_surface_pattern"] = antes_depois(f'<rect x="30" y="40" width="80" height="70" rx="6" fill="#fff" stroke="{MAG}" stroke-width="3"/>' + "".join(f'<path d="M34 {46 + i * 7}h72" stroke="{MAG}" stroke-width="3.5"/>' for i in range(9)),
    f'<rect x="170" y="40" width="80" height="70" rx="6" fill="#fff" stroke="{MAG}" stroke-width="3"/>' + "".join(f'<rect x="{174 + i * 7}" y="{44 + i * 7}" width="{72 - i * 14}" height="{62 - i * 14}" rx="4" fill="none" stroke="{MAG}" stroke-width="3"/>' for i in range(4)),
    "monotônico", "concêntrico")
D["detect_thin_wall"] = antes_depois(f'<rect x="40" y="40" width="60" height="80" rx="6" fill="{MAG}"/><path d="M70 40v80" stroke="{PAPER}" stroke-width="3"/>' + txt(70, 132, "parede some", GRAY, 10),
    f'<rect x="180" y="40" width="60" height="80" rx="6" fill="{MAG}"/><rect x="206" y="20" width="8" height="20" fill="{MAG}"/>' + txt(210, 132, "parede fina aparece", GRAY, 10), "sem detectar", "detectando")
D["infill_anchor"] = f'<rect x="40" y="34" width="200" height="90" rx="10" fill="none" stroke="{MAG}" stroke-width="6"/>' + "".join(f'<path d="M{70 + i * 30} 40v78" stroke="{GRAY}" stroke-width="3"/><path d="M{70 + i * 30} 40h14" stroke="{ORA}" stroke-width="4"/>' for i in range(6)) + txt(140, 142, "âncora: o miolo segura na parede", INK)
D["support_top_z_distance"] = mesa() + f'<rect x="60" y="34" width="150" height="26" rx="5" fill="{MAG}"/>' + "".join(f'<path d="M{70 + i * 12} 82v44" stroke="{GRAY}" stroke-width="4"/>' for i in range(12)) + f'<path d="M222 60v22M216 60h12M216 82h12" stroke="{ORA}" stroke-width="2.5" stroke-linecap="round"/>' + txt(234, 75, "vão", ORA, 13, 700, "start") + txt(140, 24, "vão entre o suporte e a peça em cima", INK)
D["support_bottom_z_distance"] = mesa() + f'<rect x="40" y="96" width="200" height="30" rx="5" fill="{MAG}"/><rect x="40" y="30" width="200" height="20" rx="5" fill="{MAG}"/>' + "".join(f'<path d="M{60 + i * 14} 50v40" stroke="{GRAY}" stroke-width="4"/>' for i in range(12)) + txt(140, 145, "vão entre o suporte e a peça embaixo", INK, 10.5)
D["support_interface_top_layers"] = mesa() + f'<rect x="60" y="36" width="160" height="22" rx="5" fill="{MAG}"/>' + camadas(62, 72, 156, 3, 4.5, (BLUE,)) + "".join(f'<path d="M{70 + i * 16} 72v54" stroke="{GRAY}" stroke-width="3"/>' for i in range(10)) + txt(140, 26, "camadas fechadas onde o suporte encosta", INK)
D["support_object_xy_distance"] = mesa() + f'<rect x="60" y="40" width="70" height="86" rx="5" fill="{MAG}"/>' + "".join(f'<path d="M{150 + i * 12} 50v76" stroke="{GRAY}" stroke-width="4"/>' for i in range(6)) + f'<rect x="146" y="40" width="80" height="10" fill="{MAG}"/>' + seta(132, 100, 148, 100, ORA, 2) + txt(140, 26, "vão de lado entre suporte e peça", INK)
D["tree_support_branch_angle"] = mesa() + f'<rect x="40" y="30" width="200" height="14" rx="4" fill="{MAG}"/><path d="M140 126V96M140 96L70 44M140 96L210 44M140 96v-52" stroke="{GRAY}" stroke-width="6" fill="none" stroke-linecap="round"/>' + f'<path d="M140 76A20 20 0 0 1 124 84" fill="none" stroke="{ORA}" stroke-width="3"/>' + txt(170, 90, "ângulo", ORA, 11, 700, "start")
D["support_style"] = D["support_type"]
D["skirt_distance"] = f'<rect x="100" y="54" width="80" height="50" rx="10" fill="{INK}"/><rect x="66" y="30" width="148" height="98" rx="18" fill="none" stroke="{MAG}" stroke-width="3"/>' + seta(182, 78, 212, 78, ORA, 2) + seta(212, 78, 182, 78, ORA, 2) + txt(140, 145, "distância da saia até a peça", INK)
D["draft_shield"] = mesa() + f'<rect x="100" y="60" width="80" height="66" rx="6" fill="{MAG}"/><path d="M72 126V56M208 126V56" stroke="{BLUE}" stroke-width="5" stroke-linecap="round"/>' + "".join(f'<path d="M{18 + i * 4} {70 + i * 14}c12-6 24 6 40 0" fill="none" stroke="{GRAY}" stroke-width="2.5" stroke-linecap="round"/>' for i in range(3)) + txt(140, 40, "cortina protege do vento", INK)
D["skirt_height"] = D["draft_shield"]
D["brim_object_gap"] = f'<ellipse cx="140" cy="84" rx="110" ry="42" fill="none" stroke="{MAG}" stroke-width="10"/><ellipse cx="140" cy="84" rx="64" ry="24" fill="{INK}"/><ellipse cx="140" cy="84" rx="78" ry="30" fill="none" stroke="{PAPER}" stroke-width="5"/>' + txt(140, 22, "vão pequeno: a aba sai sem marcar", INK)
D["close_fan_the_first_x_layers"] = mesa() + camadas(60, 126, 160, 3, 8, (ORA,)) + camadas(60, 102, 160, 6, 8) + txt(140, 30, "primeiras camadas sem ventoinha", INK) + "".join(f'<path d="M232 {50 + i * 14}c8-4 16 4 26 0" fill="none" stroke="{BLUE}" stroke-width="3" stroke-linecap="round"/>' for i in range(3))
D["slow_down_for_layer_cooling"] = mesa() + f'<path d="M120 126h40l-6-80h-28z" fill="{MAG}"/>' + velocimetro(214, 96, 34, .25, ORA) + txt(140, 30, "camada pequena: mais devagar para esfriar", INK, 10.5)
D["enable_arc_fitting"] = antes_depois(f'<path d="M30 110L44 70L64 48L90 40L110 44" fill="none" stroke="{MAG}" stroke-width="4" stroke-linejoin="round"/>' + "".join(f'<circle cx="{x}" cy="{y}" r="3" fill="{INK}"/>' for x, y in ((30, 110), (44, 70), (64, 48), (90, 40), (110, 44))),
    f'<path d="M170 110C176 60 210 38 250 44" fill="none" stroke="{MAG}" stroke-width="4"/>', "muitos segmentos", "um arco")
D["make_overhang_printable"] = antes_depois(mesa(20, 126, 100) + f'<path d="M50 126V70h60v14H66v42z" fill="{MAG}"/>', mesa(160, 126, 100) + f'<path d="M190 126V70h60v14h-14L206 126z" fill="{MAG}"/>', "beiral reto", "beiral com apoio")
D["thick_bridges"] = antes_depois(mesa(20, 126, 100) + f'<rect x="24" y="66" width="20" height="60" fill="{MAG}"/><rect x="96" y="66" width="20" height="60" fill="{MAG}"/><path d="M44 70h52" stroke="#F7648F" stroke-width="3"/>',
    mesa(160, 126, 100) + f'<rect x="164" y="66" width="20" height="60" fill="{MAG}"/><rect x="236" y="66" width="20" height="60" fill="{MAG}"/><path d="M184 70h52" stroke="#F7648F" stroke-width="8"/>', "ponte fina", "ponte grossa")
D["bridge_flow"] = D["thick_bridges"]
D["z_offset"] = bico(140, 74) + f'<rect x="60" y="100" width="160" height="10" rx="4" fill="{INK}"/>' + seta(186, 72, 186, 98, ORA, 2) + seta(186, 98, 186, 72, ORA, 2) + txt(196, 88, "ajuste Z", ORA, 12, 700, "start") + txt(140, 136, "sobe ou desce o bico em tudo", INK)
D["nozzle_diameter"] = antes_depois(bico(70, 74) + f'<path d="M70 86v24" stroke="{MAG}" stroke-width="4" stroke-linecap="round"/>', bico(210, 74) + f'<path d="M210 86v24" stroke="{MAG}" stroke-width="10" stroke-linecap="round"/>', "0,2 mm · detalhe", "0,6 mm · rápido")
D["printable_height"] = mesa() + f'<rect x="110" y="40" width="60" height="86" rx="6" fill="{MAG}"/><path d="M40 34h200" stroke="{ORA}" stroke-width="3" stroke-dasharray="6 4"/>' + txt(140, 26, "altura máxima da impressora", ORA, 11, 700)
D["extruder_clearance_radius"] = f'<circle cx="140" cy="80" r="52" fill="{SOFT}" stroke="{MAG}" stroke-width="2" stroke-dasharray="5 4"/>' + bico(140, 92) + seta(140, 80, 190, 80, ORA, 2) + txt(140, 145, "espaço livre em volta do bico", INK)
D["pressure_advance"] = antes_depois(f'<path d="M30 110V50h80" fill="none" stroke="{MAG}" stroke-width="8"/><circle cx="30" cy="50" r="8" fill="{MAG}"/>', f'<path d="M170 110V50h80" fill="none" stroke="{MAG}" stroke-width="8" stroke-linejoin="miter"/>', "canto com sobra", "canto certo")
D["enable_pressure_advance"] = D["pressure_advance"]
D["filament_diameter"] = f'<circle cx="140" cy="76" r="44" fill="none" stroke="{MAG}" stroke-width="18"/>' + seta(96, 76, 184, 76, INK, 2) + seta(184, 76, 96, 76, INK, 2) + txt(140, 140, "quase sempre 1,75 mm", INK, 12, 700)
D["filament_shrink"] = antes_depois(f'<rect x="30" y="40" width="80" height="80" rx="6" fill="none" stroke="{INK}" stroke-width="2" stroke-dasharray="4 3"/><rect x="38" y="48" width="64" height="64" rx="6" fill="{MAG}"/>',
    f'<rect x="170" y="40" width="80" height="80" rx="6" fill="{MAG}"/>', "encolheu ao esfriar", "compensado")
D["enable_overhang_speed"] = mesa() + f'<path d="M60 126V60h150v14H80v52z" fill="{MAG}"/>' + velocimetro(222, 110, 26, .25, ORA) + txt(140, 40, "beiral mais devagar", INK)
D["small_perimeter_speed"] = antes_depois(f'<circle cx="70" cy="74" r="30" fill="none" stroke="{MAG}" stroke-width="6" stroke-dasharray="8 3"/>', f'<circle cx="210" cy="74" r="30" fill="none" stroke="{MAG}" stroke-width="6"/>', "rápido: furo torto", "devagar: furo redondo")
D["support_interface_spacing"] = D["support_interface_top_layers"]
D["tree_support_branch_diameter"] = D["tree_support_branch_angle"]
D["fuzzy_skin_thickness"] = D["fuzzy_skin"]
D["hole_to_polyhole"] = antes_depois(f'<rect x="30" y="34" width="80" height="80" rx="8" fill="{MAG}"/><circle cx="70" cy="74" r="18" fill="{PAPER}"/>',
    f'<rect x="170" y="34" width="80" height="80" rx="8" fill="{MAG}"/><polygon points="210,54 227,64 227,84 210,94 193,84 193,64" fill="{PAPER}"/>', "furo redondo", "furo poligonal")
D["gcode_flavor"] = f'<rect x="60" y="30" width="160" height="96" rx="10" fill="#fff" stroke="{LINE}" stroke-width="2"/>' + "".join(txt(76, 54 + i * 18, t, INK if i % 2 == 0 else MAG, 12, 600, "start") for i, t in enumerate(("G28 ; zerar", "M104 S220", "G1 X10 Y10", "M106 S255"))) + txt(140, 145, "a linguagem que a impressora entende", INK, 10.5)

os.makedirs(OUT, exist_ok=True)
for k, corpo in D.items():
    open(os.path.join(OUT, f"filuna_aj_{k}.svg"), "w").write(svg(corpo))
print(len(D), "imagens")
