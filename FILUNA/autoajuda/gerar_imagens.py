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

os.makedirs(OUT, exist_ok=True)
for k, corpo in D.items():
    open(os.path.join(OUT, f"filuna_aj_{k}.svg"), "w").write(svg(corpo))
print(len(D), "imagens")
