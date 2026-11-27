#include "libslic3r/CAD/CadRaster.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Slic3r { namespace CadRaster {

namespace {

using V3 = std::array<double, 3>;

V3     sub(const V3& a, const V3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
double dot(const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3     cross(const V3& a, const V3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
V3     norm(const V3& a)
{
    const double l = std::sqrt(dot(a, a));
    return l > 0 ? V3{a[0] / l, a[1] / l, a[2] / l} : V3{0, 0, 1};
}

struct Camera {
    V3     f, r, u, c;
    double sh{1};   // mm per hi-res pixel
    int    w{0};
    void   px(const V3& p, double& x, double& y, double& d) const
    {
        const V3 q = sub(p, c);
        x = w * 0.5 + dot(q, r) / sh;
        y = w * 0.5 - dot(q, u) / sh;
        d = dot(q, f);
    }
};

void base_da_vista(Vista v, V3& f, V3& r, V3& u)
{
    switch (v) {
    case Vista::Frente:   f = {0, 1, 0};  r = {1, 0, 0};  u = {0, 0, 1};  return;
    case Vista::Tras:     f = {0, -1, 0}; r = {-1, 0, 0}; u = {0, 0, 1};  return;
    case Vista::Direita:  f = {-1, 0, 0}; r = {0, 1, 0};  u = {0, 0, 1};  return;
    case Vista::Esquerda: f = {1, 0, 0};  r = {0, -1, 0}; u = {0, 0, 1};  return;
    case Vista::Topo:     f = {0, 0, -1}; r = {1, 0, 0};  u = {0, 1, 0};  return;
    case Vista::Baixo:    f = {0, 0, 1};  r = {1, 0, 0};  u = {0, -1, 0}; return;
    case Vista::Iso:
    default:
        f = norm({-1, 1, -1});              // seen from the front right, above
        r = norm(cross(f, {0, 0, 1}));
        u = cross(r, f);
        return;
    }
}

struct Tela {
    int                  w;
    std::vector<uint8_t> rgb;
    std::vector<float>   z;
    std::vector<uint8_t> id;
    std::vector<int8_t>  n;
    explicit Tela(int w_) : w(w_), rgb(size_t(w_) * w_ * 3), z(size_t(w_) * w_, std::numeric_limits<float>::infinity()),
                            id(size_t(w_) * w_, 0), n(size_t(w_) * w_ * 3, 0) {}
    void pinta(int x, int y, const std::array<uint8_t, 3>& c)
    {
        if (x < 0 || y < 0 || x >= w || y >= w) return;
        uint8_t* p = &rgb[(size_t(y) * w + x) * 3];
        p[0] = c[0]; p[1] = c[1]; p[2] = c[2];
    }
};

// Segment clipped to the screen (Liang-Barsky), then stepped one pixel at a time.
void linha(Tela& t, double x0, double y0, double x1, double y1, const std::array<uint8_t, 3>& c, int grossura)
{
    double a = 0, b = 1;
    const double dx = x1 - x0, dy = y1 - y0;
    const double pp[4] = {-dx, dx, -dy, dy};
    const double qq[4] = {x0 + 2, t.w + 1 - x0, y0 + 2, t.w + 1 - y0};
    for (int i = 0; i < 4; ++i) {
        if (pp[i] == 0) { if (qq[i] < 0) return; continue; }
        const double r = qq[i] / pp[i];
        if (pp[i] < 0) a = std::max(a, r); else b = std::min(b, r);
        if (a > b) return;
    }
    const double sx = x0 + a * dx, sy = y0 + a * dy, ex = x0 + b * dx, ey = y0 + b * dy;
    const int    n  = std::max(1, int(std::ceil(std::max(std::abs(ex - sx), std::abs(ey - sy)))));
    const int    h0 = -(grossura - 1) / 2, h1 = grossura / 2;
    for (int k = 0; k <= n; ++k) {
        const int x = int(std::floor(sx + (ex - sx) * k / n)), y = int(std::floor(sy + (ey - sy) * k / n));
        for (int oy = h0; oy <= h1; ++oy)
            for (int ox = h0; ox <= h1; ++ox) t.pinta(x + ox, y + oy, c);
    }
}

// A filled triangle in colour only (the bed), no depth.
void triangulo_plano(Tela& t, const double* xs, const double* ys, const std::array<uint8_t, 3>& c)
{
    const int x0 = std::max(0, int(std::floor(std::min({xs[0], xs[1], xs[2]}))));
    const int x1 = std::min(t.w - 1, int(std::ceil(std::max({xs[0], xs[1], xs[2]}))));
    const int y0 = std::max(0, int(std::floor(std::min({ys[0], ys[1], ys[2]}))));
    const int y1 = std::min(t.w - 1, int(std::ceil(std::max({ys[0], ys[1], ys[2]}))));
    const double area = (xs[1] - xs[0]) * (ys[2] - ys[0]) - (ys[1] - ys[0]) * (xs[2] - xs[0]);
    if (area == 0) return;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            const double px = x + 0.5, py = y + 0.5;
            const double w0 = ((xs[2] - xs[1]) * (py - ys[1]) - (ys[2] - ys[1]) * (px - xs[1])) / area;
            const double w1 = ((xs[0] - xs[2]) * (py - ys[2]) - (ys[0] - ys[2]) * (px - xs[2])) / area;
            const double w2 = 1 - w0 - w1;
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) t.pinta(x, y, c);
        }
}

const std::array<uint8_t, 3> kFundo{{246, 247, 249}};
const std::array<uint8_t, 3> kMesa{{232, 235, 239}};
const std::array<uint8_t, 3> kGrade{{212, 216, 222}};
const std::array<uint8_t, 3> kBorda{{150, 157, 166}};
const std::array<uint8_t, 3> kAresta{{38, 42, 50}};

} // namespace

bool vista_de_nome(const std::string& n, Vista& v)
{
    if (n == "iso" || n == "isometrica" || n == "isométrica") { v = Vista::Iso; return true; }
    if (n == "frente" || n == "front")                         { v = Vista::Frente; return true; }
    if (n == "topo" || n == "top" || n == "cima")              { v = Vista::Topo; return true; }
    if (n == "direita" || n == "right")                        { v = Vista::Direita; return true; }
    if (n == "esquerda" || n == "left")                        { v = Vista::Esquerda; return true; }
    if (n == "tras" || n == "trás" || n == "back")             { v = Vista::Tras; return true; }
    if (n == "baixo" || n == "bottom")                         { v = Vista::Baixo; return true; }
    return false;
}

const char* nome_da_vista(Vista v)
{
    switch (v) {
    case Vista::Frente: return "frente";
    case Vista::Topo: return "topo";
    case Vista::Direita: return "direita";
    case Vista::Esquerda: return "esquerda";
    case Vista::Tras: return "tras";
    case Vista::Baixo: return "baixo";
    case Vista::Iso:
    default: return "iso";
    }
}

const char* nomes_das_vistas() { return "iso, frente, topo, direita, esquerda, tras, baixo"; }

std::array<double, 2> projetar(const Imagem& im, const std::array<double, 3>& p)
{
    const V3 q = sub(p, im.centro);
    return {im.largura * 0.5 + dot(q, im.direita) / im.escala_mm_por_px, im.altura * 0.5 - dot(q, im.cima) / im.escala_mm_por_px};
}

Imagem desenhar(const std::vector<Malha>& malhas, Vista vista, int tamanho, const Mesa& mesa)
{
    constexpr int S = 2;   // supersampling
    tamanho = std::max(64, std::min(2048, tamanho));
    Imagem im;
    im.largura = im.altura = tamanho;

    // What is drawn: the triangles' box; with none, the bed (or 100 mm around the origin).
    V3 mn{1e300, 1e300, 1e300}, mx{-1e300, -1e300, -1e300};
    for (const Malha& m : malhas) {
        if (m.n_tri == 0) continue;
        for (size_t i = 0; i < m.n_vertices; ++i)
            for (int k = 0; k < 3; ++k) {
                mn[k] = std::min(mn[k], double(m.xyz[i * 3 + k]));
                mx[k] = std::max(mx[k], double(m.xyz[i * 3 + k]));
            }
        im.vazia = false;
    }
    if (im.vazia) {
        if (mesa.tem) { mn = {mesa.xmin, mesa.ymin, 0}; mx = {mesa.xmax, mesa.ymax, 10}; }
        else          { mn = {-50, -50, 0}; mx = {50, 50, 10}; }
    }
    im.min = mn;
    im.max = mx;

    Camera cam;
    base_da_vista(vista, cam.f, cam.r, cam.u);
    cam.c = {(mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2, (mn[2] + mx[2]) / 2};
    double eu = 0, ev = 0;
    {
        double umin = 1e300, umax = -1e300, vmin = 1e300, vmax = -1e300;
        for (int i = 0; i < 8; ++i) {
            const V3 p{(i & 1) ? mx[0] : mn[0], (i & 2) ? mx[1] : mn[1], (i & 4) ? mx[2] : mn[2]};
            const double u = dot(sub(p, cam.c), cam.r), v = dot(sub(p, cam.c), cam.u);
            umin = std::min(umin, u); umax = std::max(umax, u); vmin = std::min(vmin, v); vmax = std::max(vmax, v);
        }
        eu = umax - umin;
        ev = vmax - vmin;
    }
    const double escala = std::max({eu, ev, 1.0}) * 1.4 / tamanho;   // margin for the labels
    cam.w  = tamanho * S;
    cam.sh = escala / S;
    im.escala_mm_por_px = escala;
    im.centro  = cam.c;
    im.direita = cam.r;
    im.cima    = cam.u;
    im.olhar   = cam.f;

    Tela t(cam.w);
    for (size_t i = 0; i < t.rgb.size(); i += 3) { t.rgb[i] = kFundo[0]; t.rgb[i + 1] = kFundo[1]; t.rgb[i + 2] = kFundo[2]; }

    // The bed.
    const double final_px_10mm = 10.0 / escala;
    const double passo = final_px_10mm >= 8 ? 10 : 50.0 / escala >= 8 ? 50 : 100;
    double bx0, by0, bx1, by1;
    if (mesa.tem) { bx0 = mesa.xmin; by0 = mesa.ymin; bx1 = mesa.xmax; by1 = mesa.ymax; }
    else {
        bx0 = std::floor((mn[0] - 20) / passo) * passo; by0 = std::floor((mn[1] - 20) / passo) * passo;
        bx1 = std::ceil((mx[0] + 20) / passo) * passo;  by1 = std::ceil((mx[1] + 20) / passo) * passo;
    }
    auto P = [&](double x, double y, double& sx, double& sy) { double d; cam.px({x, y, 0}, sx, sy, d); };
    if (vista == Vista::Iso || vista == Vista::Topo) {
        double xs[4], ys[4];
        P(bx0, by0, xs[0], ys[0]); P(bx1, by0, xs[1], ys[1]); P(bx1, by1, xs[2], ys[2]); P(bx0, by1, xs[3], ys[3]);
        const double ax[3] = {xs[0], xs[1], xs[2]}, ay[3] = {ys[0], ys[1], ys[2]};
        const double bxs[3] = {xs[0], xs[2], xs[3]}, bys[3] = {ys[0], ys[2], ys[3]};
        triangulo_plano(t, ax, ay, kMesa);
        triangulo_plano(t, bxs, bys, kMesa);
        // Grid lines on round millimetres, from the bed's own origin.
        for (double x = std::ceil(bx0 / passo) * passo; x <= bx1 + 1e-9; x += passo) {
            double a0, a1, b0, b1; P(x, by0, a0, a1); P(x, by1, b0, b1); linha(t, a0, a1, b0, b1, kGrade, S);
        }
        for (double y = std::ceil(by0 / passo) * passo; y <= by1 + 1e-9; y += passo) {
            double a0, a1, b0, b1; P(bx0, y, a0, a1); P(bx1, y, b0, b1); linha(t, a0, a1, b0, b1, kGrade, S);
        }
        if (mesa.tem)
            for (int k = 0; k < 4; ++k) linha(t, xs[k], ys[k], xs[(k + 1) % 4], ys[(k + 1) % 4], kBorda, 2 * S);
    } else if (vista != Vista::Baixo) {
        double xs[4], ys[4];
        P(bx0, by0, xs[0], ys[0]); P(bx1, by0, xs[1], ys[1]); P(bx1, by1, xs[2], ys[2]); P(bx0, by1, xs[3], ys[3]);
        double l = mesa.tem ? std::min({xs[0], xs[1], xs[2], xs[3]}) : -10.0, r = mesa.tem ? std::max({xs[0], xs[1], xs[2], xs[3]}) : t.w + 10.0;
        const double y0 = ys[0];   // Z = 0 is one screen row in a side view
        for (int y = std::max(0, int(std::ceil(y0))); y < t.w; ++y)
            for (int x = std::max(0, int(std::floor(l))); x <= std::min(t.w - 1, int(std::ceil(r))); ++x) t.pinta(x, y, kMesa);
        linha(t, l, y0, r, y0, kBorda, 2 * S);
    }

    // The bodies.
    const V3 luz = norm({-0.45 * cam.r[0] + 0.65 * cam.u[0] - 0.6 * cam.f[0], -0.45 * cam.r[1] + 0.65 * cam.u[1] - 0.6 * cam.f[1],
                         -0.45 * cam.r[2] + 0.65 * cam.u[2] - 0.6 * cam.f[2]});
    std::vector<double> sx, sy, sd;
    for (size_t mi = 0; mi < malhas.size(); ++mi) {
        const Malha& m = malhas[mi];
        sx.resize(m.n_vertices); sy.resize(m.n_vertices); sd.resize(m.n_vertices);
        for (size_t i = 0; i < m.n_vertices; ++i)
            cam.px({m.xyz[i * 3], m.xyz[i * 3 + 1], m.xyz[i * 3 + 2]}, sx[i], sy[i], sd[i]);
        const uint8_t corpo = uint8_t(1 + mi % 255);
        for (size_t ti = 0; ti < m.n_tri; ++ti) {
            const int32_t a = m.tri[ti * 3], b = m.tri[ti * 3 + 1], c = m.tri[ti * 3 + 2];
            if (a < 0 || b < 0 || c < 0 || size_t(a) >= m.n_vertices || size_t(b) >= m.n_vertices || size_t(c) >= m.n_vertices) continue;
            const V3 pa{m.xyz[a * 3], m.xyz[a * 3 + 1], m.xyz[a * 3 + 2]};
            const V3 pb{m.xyz[b * 3], m.xyz[b * 3 + 1], m.xyz[b * 3 + 2]};
            const V3 pc{m.xyz[c * 3], m.xyz[c * 3 + 1], m.xyz[c * 3 + 2]};
            V3 nrm = norm(cross(sub(pb, pa), sub(pc, pa)));
            if (dot(nrm, cam.f) > 0) nrm = {-nrm[0], -nrm[1], -nrm[2]};   // two-sided
            const double ndl = std::max(0.0, dot(nrm, luz));
            const double luzv = std::min(1.0, 0.32 + 0.55 * ndl + 0.18 * -dot(nrm, cam.f));
            std::array<uint8_t, 3> cor;
            for (int k = 0; k < 3; ++k) cor[k] = uint8_t(std::min(255.0, m.cor[k] * luzv + 0.5));
            const int8_t nq[3] = {int8_t(std::lround(nrm[0] * 127)), int8_t(std::lround(nrm[1] * 127)), int8_t(std::lround(nrm[2] * 127))};

            const double x0 = sx[a], y0 = sy[a], x1 = sx[b], y1 = sy[b], x2 = sx[c], y2 = sy[c];
            const double area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
            if (std::abs(area) < 1e-12) continue;
            const int ix0 = std::max(0, int(std::floor(std::min({x0, x1, x2}))));
            const int ix1 = std::min(t.w - 1, int(std::ceil(std::max({x0, x1, x2}))));
            const int iy0 = std::max(0, int(std::floor(std::min({y0, y1, y2}))));
            const int iy1 = std::min(t.w - 1, int(std::ceil(std::max({y0, y1, y2}))));
            for (int y = iy0; y <= iy1; ++y)
                for (int x = ix0; x <= ix1; ++x) {
                    const double px = x + 0.5, py = y + 0.5;
                    const double w0 = ((x2 - x1) * (py - y1) - (y2 - y1) * (px - x1)) / area;
                    const double w1 = ((x0 - x2) * (py - y2) - (y0 - y2) * (px - x2)) / area;
                    const double w2 = 1 - w0 - w1;
                    if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                    const float d = float(w0 * sd[a] + w1 * sd[b] + w2 * sd[c]);
                    const size_t k = size_t(y) * t.w + x;
                    if (d >= t.z[k]) continue;
                    t.z[k]  = d;
                    t.id[k] = corpo;
                    t.n[k * 3] = nq[0]; t.n[k * 3 + 1] = nq[1]; t.n[k * 3 + 2] = nq[2];
                    t.rgb[k * 3] = cor[0]; t.rgb[k * 3 + 1] = cor[1]; t.rgb[k * 3 + 2] = cor[2];
                }
        }
    }

    // Outlines: silhouettes, body boundaries, depth steps and creases sharper than ~40 degrees.
    const float  dthr = float(std::max(12.0 * cam.sh, 0.3));
    std::vector<uint8_t> borda(size_t(t.w) * t.w, 0);
    auto difere = [&](size_t p, size_t q) {
        if (t.id[p] != t.id[q]) return true;
        if (t.id[p] == 0) return false;
        if (std::abs(t.z[p] - t.z[q]) > dthr) return true;
        const int d = t.n[p * 3] * t.n[q * 3] + t.n[p * 3 + 1] * t.n[q * 3 + 1] + t.n[p * 3 + 2] * t.n[q * 3 + 2];
        return d < int(0.77 * 127 * 127);
    };
    for (int y = 0; y < t.w; ++y)
        for (int x = 0; x < t.w; ++x) {
            const size_t p = size_t(y) * t.w + x;
            if (x + 1 < t.w && difere(p, p + 1)) borda[p] = borda[p + 1] = 1;
            if (y + 1 < t.w && difere(p, p + t.w)) borda[p] = borda[p + t.w] = 1;
        }
    for (size_t p = 0; p < borda.size(); ++p)
        if (borda[p]) { t.rgb[p * 3] = kAresta[0]; t.rgb[p * 3 + 1] = kAresta[1]; t.rgb[p * 3 + 2] = kAresta[2]; }

    // 2x2 box filter down to the asked size.
    im.rgb.assign(size_t(tamanho) * tamanho * 3, 0);
    for (int y = 0; y < tamanho; ++y)
        for (int x = 0; x < tamanho; ++x)
            for (int k = 0; k < 3; ++k) {
                int s = 0;
                for (int dy = 0; dy < S; ++dy)
                    for (int dx = 0; dx < S; ++dx) s += t.rgb[((size_t(y * S + dy) * t.w) + x * S + dx) * 3 + k];
                im.rgb[(size_t(y) * tamanho + x) * 3 + k] = uint8_t(s / (S * S));
            }
    return im;
}

std::string para_ppm(const Imagem& im)
{
    std::string s = "P6\n" + std::to_string(im.largura) + " " + std::to_string(im.altura) + "\n255\n";
    s.append(reinterpret_cast<const char*>(im.rgb.data()), im.rgb.size());
    return s;
}

}} // namespace Slic3r::CadRaster
