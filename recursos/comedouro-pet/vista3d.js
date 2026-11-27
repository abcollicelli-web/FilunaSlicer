// Comedouro Filuna — vista 3D: as peças, a tigela de aço apoiada no tampo (só na prévia) e o corte para ver por dentro.
import * as THREE from './three.module.min.js';
import { OrbitControls } from './OrbitControls.js';
import { geometriaSuave } from './suave.js';

export function criarVista3D(canvas) {
  const render = new THREE.WebGLRenderer({ canvas, antialias: true, preserveDrawingBuffer: true });
  render.setPixelRatio(Math.min(2, window.devicePixelRatio || 1));
  render.localClippingEnabled = true;
  const cena = new THREE.Scene(), camera = new THREE.PerspectiveCamera(32, 1, 0.5, 20000);
  camera.up.set(0, 0, 1);
  const controles = new OrbitControls(camera, canvas);
  cena.background = new THREE.Color('#17161A');
  controles.enableDamping = true;
  cena.add(new THREE.HemisphereLight(0xffffff, 0x2a2630, 1.55));
  const sol = new THREE.DirectionalLight(0xffffff, 1.5); sol.position.set(-120, -220, 300); cena.add(sol);
  const contra = new THREE.DirectionalLight(0xffffff, 0.55); contra.position.set(200, 160, 120); cena.add(contra);
  const mesa = new THREE.Mesh(new THREE.CircleGeometry(1, 96), new THREE.MeshStandardMaterial({ color: '#2A2830', roughness: 0.9 }));
  mesa.position.z = -0.05; cena.add(mesa);
  const corte = new THREE.Plane(new THREE.Vector3(0, 1, 0), 0);
  let grupo = null, enquadrado = '', comCorte = false;
  const tamanho = () => {
    const w = canvas.clientWidth, h = canvas.clientHeight, pr = render.getPixelRatio();
    if (canvas.width !== Math.floor(w * pr) || canvas.height !== Math.floor(h * pr)) { render.setSize(w, h, false); camera.aspect = w / (h || 1); camera.updateProjectionMatrix(); }
  };
  (function laco() { requestAnimationFrame(laco); tamanho(); controles.update(); render.render(cena, camera); })();

  const material = (cor, extra = {}) => new THREE.MeshStandardMaterial({ color: cor, roughness: 0.55, metalness: 0.02, side: THREE.DoubleSide, clippingPlanes: comCorte ? [corte] : [], ...extra });

  function tigelaDeAco(f) {
    const pts = [], aba = 9, r = f.rb - 0.4, fundo = r * 0.74, prof = f.prof;
    pts.push(new THREE.Vector2(0.01, -prof));
    for (let k = 0; k <= 8; k++) { const a = k / 8 * Math.PI / 2; pts.push(new THREE.Vector2(fundo - 8 + 8 * Math.sin(a), -prof + 8 - 8 * Math.cos(a))); }
    pts.push(new THREE.Vector2(r, -2), new THREE.Vector2(r + aba, 0.6), new THREE.Vector2(r + aba + 1.2, 1.6), new THREE.Vector2(r + aba - 0.4, 2.2), new THREE.Vector2(r - 1, 0.4));
    for (let k = 8; k >= 0; k--) { const a = k / 8 * Math.PI / 2; pts.push(new THREE.Vector2(Math.max(0.01, fundo - 9 + 8 * Math.sin(a)), -prof + 9 - 8 * Math.cos(a) + 0.8)); }
    pts.push(new THREE.Vector2(0.01, -prof + 1.8));
    const g = new THREE.LatheGeometry(pts, 128);
    g.rotateX(Math.PI / 2);
    const malha = new THREE.Mesh(g, material('#D7DCE2', { metalness: 0.75, roughness: 0.22, transparent: true, opacity: 0.92 }));
    malha.rotation.x = f.alfa; malha.position.set(f.cx, 0, f.H);
    return malha;
  }

  // partes: [{pos, idx, corVista, transparente?}]; fantasmas: tigelas de aço; chave: muda quando o tamanho muda
  function mostrar(partes, fantasmas, chave) {
    if (grupo) { cena.remove(grupo); grupo.traverse(o => { if (o.geometry) o.geometry.dispose(); if (o.material) o.material.dispose(); }); }
    grupo = new THREE.Group();
    for (const pt of partes) {
      if (!pt.idx.length) continue;
      const extra = pt.transparente ? { transparent: true, opacity: 0.35, depthWrite: false } : {};
      grupo.add(new THREE.Mesh(geometriaSuave(pt.pos, pt.idx), material(pt.corVista, extra)));
    }
    for (const f of fantasmas || []) grupo.add(tigelaDeAco(f));
    cena.add(grupo);
    const caixa = new THREE.Box3().setFromObject(grupo), c = caixa.getCenter(new THREE.Vector3()), t = caixa.getSize(new THREE.Vector3());
    mesa.scale.setScalar(Math.max(t.x, t.y) * 0.95);
    mesa.position.x = c.x; mesa.position.y = c.y;
    if (chave === enquadrado) return;
    enquadrado = chave; tamanho();
    const d = Math.max(t.x / Math.max(0.6, camera.aspect), t.y * 0.9, t.z * 1.3) * 2.9 + 40;
    controles.target.set(c.x, c.y, c.z * 0.8); camera.position.set(c.x + d * 0.28, c.y - d * 0.82, c.z + d * 0.62);
    camera.near = d / 100; camera.far = d * 20; camera.updateProjectionMatrix();
  }

  return {
    mostrar,
    cortar(sim) { comCorte = sim; if (grupo) grupo.traverse(o => { if (o.material) { o.material.clippingPlanes = sim ? [corte] : []; o.material.needsUpdate = true; } }); },
    foto: () => canvas.toDataURL('image/png'),
  };
}
