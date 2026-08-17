#pragma once
// =====================================================
// TOAPERFORM - WebUi.h
// Panel/rapor/OTA sayfalarinin HTML+CSS+JS govdesi. Buyuk sabitler PROGMEM'de
// tutulur (String'e kopyalanmadan dogrudan flash'tan gonderilir). Bu dosya
// sadece SUNUM katmani - hicbir hesaplama mantigi icermez, veriyi /data,
// /team, /history, /backup fetch()'leriyle WebRoutes'tan ceker.
//
// 2026-08: kapsam SADECE nabza daraltildi - GPS (hiz/mesafe/sprint/saha isi
// haritasi), barometre/sicrama, ACWR/Monotonluk ve Kariyer Karti/Yetenek/
// Gelisim puanlamasi kaldirildi. ESP-NOW Takim ozelligi (sadece nabiz/
// yorgunluk ozetiyle) geri eklendi.
// =====================================================

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>TOAPERFORM Athlete Heart Rate</title>

<style>
@font-face{
  font-family:"TP Display";
  src:local("Roboto Condensed"),local("Arial Narrow"),local("Noto Sans Condensed");
  font-weight:700 900;
}
:root{
  --bg:#070c09;
  --bg-glow:#0f2d1c;
  --card:#0e1811;
  --card2:#0b120d;
  --line:#1c2c21;
  --line2:#25392c;
  --muted:#8aa08f;
  --muted-light:#cbdccd;
  --text:#f3f7f1;
  --accent:#4ade80;
  --accent-strong:#22c55e;
  --accent-deep:#16a34a;
  --gold:#f5b942;
}
*{box-sizing:border-box}
html{ scroll-behavior:smooth; }
body{
  margin:0;
  font-family:-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  background:
    radial-gradient(circle at top,var(--bg-glow) 0,var(--bg) 40%),
    var(--bg);
  color:var(--text);
}
.container{
  max-width:520px;
  margin:auto;
  padding:16px;
}
.header{
  position:relative;
  background:linear-gradient(165deg,#101d14 0%,#0b140d 55%,var(--bg) 100%);
  border:1px solid var(--line2);
  border-radius:22px;
  padding:22px;
  margin-bottom:14px;
  box-shadow:0 18px 40px rgba(0,0,0,.5);
  overflow:hidden;
}
.header::before{
  content:"";
  position:absolute;
  inset:0 0 auto 0;
  height:3px;
  background:linear-gradient(90deg,var(--accent),var(--gold));
}
.brand{
  font-family:"TP Display",-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  font-size:40px;
  font-weight:900;
  letter-spacing:1px;
  text-transform:uppercase;
  color:var(--accent);
  line-height:1.05;
}
.title{
  font-family:"TP Display",-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  font-size:16px;
  font-weight:700;
  letter-spacing:.3px;
  margin-top:4px;
  color:var(--muted-light);
}
.sub{
  font-size:13px;
  color:var(--muted-light);
  margin-top:4px;
}
.risk{
  margin-top:16px;
  padding:16px;
  border-radius:18px;
  color:var(--bg);
  text-align:center;
  box-shadow:inset 0 0 0 1px rgba(255,255,255,.25);
}
.risk-main{
  font-family:"TP Display",-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  font-size:24px;
  font-weight:900;
  letter-spacing:.3px;
}
.risk-sub{
  font-size:13px;
  font-weight:700;
  margin-top:4px;
}
.warning-box{
  margin-top:12px;
  background:rgba(2,6,23,.28);
  border:1px solid rgba(255,255,255,.18);
  border-radius:16px;
  padding:12px;
  font-size:14px;
  line-height:1.4;
}
.trend-box{
  background:rgba(250,204,21,.16);
  border:1px solid rgba(250,204,21,.5);
}
.section-title{
  margin:22px 4px 10px;
  padding-left:10px;
  border-left:3px solid var(--accent);
  color:var(--muted-light);
  font-size:13px;
  font-weight:800;
  letter-spacing:1px;
  text-transform:uppercase;
}
.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:12px;
}
.card{
  background:linear-gradient(180deg,var(--card),var(--card2));
  border:1px solid var(--line);
  border-radius:20px;
  padding:16px;
  box-shadow:0 8px 24px rgba(0,0,0,.32);
}
.card.big{
  grid-column:span 2;
}
.label{
  color:var(--muted);
  font-size:12px;
  font-weight:700;
}
.value{
  font-family:"TP Display",-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  font-size:26px;
  font-weight:900;
  font-variant-numeric:tabular-nums;
  margin-top:7px;
}
.big .value{
  font-size:44px;
}
.unit{
  font-size:13px;
  color:var(--muted);
  font-weight:700;
}
.mini{
  font-size:16px !important;
  line-height:1.35;
  font-family:-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
}
.pill{
  background:var(--card2);
  border:1px solid var(--line);
  border-radius:999px;
  padding:10px 7px;
  font-size:11px;
  text-align:center;
  color:var(--muted-light);
}
.btn{
  display:block;
  width:100%;
  border:0;
  border-radius:17px;
  padding:15px;
  margin-top:12px;
  background:#ef4444;
  color:white;
  font-weight:900;
  font-size:15px;
}
.footer{
  text-align:center;
  color:var(--muted);
  font-size:12px;
  margin:18px 0 6px;
  line-height:1.45;
}

/* ---------------- Meter (linear gauge) ---------------- */
.meter{ margin-top:12px; }
.meter-track{
  position:relative;
  height:14px;
  border-radius:7px;
  background:var(--card2);
  overflow:hidden;
}
.meter-zone{
  position:absolute;
  top:0;
  height:100%;
  opacity:.38;
}
.meter-fill{
  position:absolute;
  top:0;
  left:0;
  height:100%;
  border-radius:7px;
  background:var(--accent);
  transition:width .5s ease;
}
.meter-ticks{
  position:relative;
  height:14px;
  margin-top:3px;
}
.meter-ticks span{
  position:absolute;
  transform:translateX(-50%);
  font-size:10px;
  font-weight:700;
  color:var(--muted);
  white-space:nowrap;
}
.meter-ticks i{
  position:absolute;
  top:-17px;
  width:1px;
  height:12px;
  background:rgba(255,255,255,.25);
}
.risk .meter-track{ background:rgba(7,12,9,.35); }
.risk .meter-ticks span{ color:rgba(7,12,9,.65); }

/* ---------------- Stacked bar (nabiz bolgeleri) ---------------- */
.stackbar{
  display:flex;
  gap:2px;
  height:22px;
  border-radius:11px;
  overflow:hidden;
  background:var(--bg);
}
.stackbar-seg{
  height:100%;
  display:flex;
  align-items:center;
  justify-content:center;
  font-size:11px;
  font-weight:800;
  color:var(--bg);
  transition:width .5s ease;
  min-width:0;
}
.legend-row{
  display:flex;
  flex-wrap:wrap;
  gap:12px;
  margin-top:10px;
}
.legend-chip{
  font-size:12px;
  color:var(--muted);
  font-weight:700;
  display:flex;
  align-items:center;
  gap:6px;
}
.legend-chip i{
  width:10px;
  height:10px;
  border-radius:3px;
  display:inline-block;
}
.zone-badge{
  display:inline-block;
  margin-top:6px;
  padding:4px 10px;
  border-radius:999px;
  font-size:12px;
  font-weight:800;
  color:#04120a;
  background:var(--muted);
}

/* ---------------- Yapiskan durum cubugu ---------------- */
.stickybar{
  position:fixed;
  top:0; left:0; right:0;
  z-index:50;
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:10px;
  padding:10px 18px;
  background:var(--card);
  border-bottom:1px solid var(--line);
  box-shadow:0 4px 16px rgba(0,0,0,.4);
  transform:translateY(-100%);
  transition:transform .25s ease;
}
.stickybar.show{ transform:translateY(0); }
.stickybar .sb-name{
  font-size:12px;
  font-weight:800;
  color:var(--muted);
  letter-spacing:.4px;
}
.stickybar .sb-risk{
  font-weight:900;
  font-size:13px;
  padding:5px 12px;
  border-radius:999px;
  color:var(--bg);
}
.stickybar .sb-hr{
  font-weight:900;
  font-size:16px;
  color:var(--text);
}
.stickybar .sb-hr span{ font-size:12px; color:var(--muted); font-weight:700; }
</style>

<script>
function setFill(id, pct, color){
  const el = document.getElementById(id);
  if(!el) return;
  pct = Math.max(0, Math.min(100, pct));
  el.style.width = pct + '%';
  if(color) el.style.background = color;
}

const HR_ZONE_COLORS = ['#8aa08f', '#60a5fa', '#4ade80', '#facc15', '#fb923c', '#ef4444'];
const HR_ZONE_LABELS = ['--', 'Z1', 'Z2', 'Z3', 'Z4', 'Z5'];

function renderHrZones(slot, zoneNow, zoneSec){
  const [s1, s2, s3, s4, s5] = zoneSec;
  document.getElementById('hrZoneLabel'+slot).textContent = HR_ZONE_LABELS[zoneNow] || '--';
  const badge = document.getElementById('hrZoneBadge'+slot);
  badge.textContent = HR_ZONE_LABELS[zoneNow] || '--';
  badge.style.background = HR_ZONE_COLORS[zoneNow] || 'var(--muted)';

  const total = s1 + s2 + s3 + s4 + s5;
  const pct = v => total > 0 ? (v/total)*100 : 0;
  setFill('segZ1_'+slot, pct(s1));
  setFill('segZ2_'+slot, pct(s2));
  setFill('segZ3_'+slot, pct(s3));
  setFill('segZ4_'+slot, pct(s4));
  setFill('segZ5_'+slot, pct(s5));

  const label = (id, v) => {
    const el = document.getElementById(id);
    el.textContent = (total > 0 && pct(v) >= 12) ? pct(v).toFixed(0)+'%' : '';
    el.title = 'Z'+id.split('_')[0].slice(-1)+': '+formatSecs(v);
  };
  label('segZ1_'+slot, s1); label('segZ2_'+slot, s2); label('segZ3_'+slot, s3); label('segZ4_'+slot, s4); label('segZ5_'+slot, s5);
}

function formatSecs(totalSec){
  const h = Math.floor(totalSec / 3600);
  const m = Math.floor((totalSec % 3600) / 60);
  const s = Math.floor(totalSec % 60);
  const pad = n => n < 10 ? '0'+n : ''+n;
  return pad(h)+':'+pad(m)+':'+pad(s);
}

// Bir slot icin panel karti HTML govdesini uretir (sablon dizesi). Sadece
// slot ilk kez enabled=true oldugunda cagrilir (bkz. buildSlotDom) - tekrar
// tekrar innerHTML yazip dropdown/odagi bozmamak icin.
function slotCardHtml(slot){
  return `
    <div class="section-title">${'Bant '+(slot+1)}</div>
    <div class="card big">
      <div class="label">Bu bandi kim takiyor?</div>
      <select id="playerSelect${slot}" style="width:100%;padding:10px;margin-top:8px;border-radius:10px;border:1px solid var(--border);background:var(--card2);color:var(--text)" onchange="assignSlotNow(${slot})">
        <option value="0">-- Oyuncu secilmedi --</option>
      </select>
    </div>
    <div id="riskBox${slot}" class="risk">
      <div id="riskReadyBlock${slot}">
        <div class="risk-main"><span id="riskText${slot}">NORMAL</span></div>
        <div class="risk-sub">Nabiz Tabanli Yorgunluk (canli gosterge, tani degildir): <span id="riskScore${slot}">0</span>/100</div>
      </div>
      <div id="riskNotReadyBlock${slot}" style="display:none">
        <div class="risk-main" style="font-size:16px">Oyuncu secilmedi / kisisel nabiz rekoru henuz yok</div>
        <div class="risk-sub">Yorgunluk skoru icin oyuncu secin ve en az bir antrenmani "Antrenmani Sifirla" ile tamamlayin (30 saniyeden uzun) - sistem o zaman kisisel tavanini ogrenir.</div>
      </div>
      <div class="meter" id="fatigueMeterWrap${slot}">
        <div class="meter-track">
          <div class="meter-zone" style="left:0%;width:25%;background:#22c55e"></div>
          <div class="meter-zone" style="left:25%;width:15%;background:#facc15"></div>
          <div class="meter-zone" style="left:40%;width:60%;background:#ef4444"></div>
          <div class="meter-fill" id="fatigueMeterFill${slot}" style="width:0%;background:#22c55e"></div>
        </div>
        <div class="meter-ticks">
          <i style="left:25%"></i><span style="left:25%">25</span>
          <i style="left:40%"></i><span style="left:40%">40</span>
        </div>
      </div>
    </div>
    <div class="warning-box">
      <b>Son Uyari:</b><br>
      <span id="warning${slot}">Sistem hazir</span>
    </div>
    <div class="warning-box trend-box" id="trendBox${slot}" style="display:none">
      <b>Ongoru:</b><br>
      <span id="trendWarning${slot}"></span>
    </div>
    <div class="grid">
      <div class="card big">
        <div class="label" id="hrLabel${slot}">Nabiz</div>
        <div class="value"><span id="hrBpm${slot}">--</span> <span class="unit">bpm</span></div>
        <div class="legend-chip" id="hrStatus${slot}" style="margin-top:6px;color:var(--muted-light)">Bagli degil</div>
      </div>
      <div class="card">
        <div class="label">%HRmax</div>
        <div class="value"><span id="hrPctMax${slot}">--</span> <span class="unit">%</span></div>
      </div>
      <div class="card">
        <div class="label">Bolge</div>
        <div class="value" id="hrZoneLabel${slot}">--</div>
        <div class="zone-badge" id="hrZoneBadge${slot}">--</div>
      </div>
      <div class="card">
        <div class="label">ACWR (Akut:Kronik Yuk)</div>
        <div class="value"><span id="acwrValue${slot}">--</span></div>
        <div class="legend-chip" id="acwrBand${slot}" style="margin-top:6px;color:var(--muted-light)">Yetersiz veri</div>
      </div>
      <div class="card">
        <div class="label">Antrenman Monotonlugu</div>
        <div class="value"><span id="monotonyValue${slot}">--</span></div>
        <div class="legend-chip" id="monotonyBand${slot}" style="margin-top:6px;color:var(--muted-light)">Yetersiz veri</div>
      </div>
      <div class="card" id="rmssdCard${slot}" style="display:none">
        <div class="label">HRV (anlik: RMSSD / SDNN / pNN50)</div>
        <div class="value"><span id="rmssdValue${slot}">--</span> <span class="unit">ms</span></div>
        <div class="value mini">SDNN: <span id="sdnnValue${slot}">--</span> ms &nbsp;|&nbsp; pNN50: <span id="pnn50Value${slot}">--</span>%</div>
        <div class="legend-chip" style="margin-top:6px;color:var(--muted-light)">Hareket halinde - dinlenme HRV'si degil</div>
      </div>
      <div class="card big">
        <div class="label">Kalp Hizi Toparlanmasi (HRR)</div>
        <div class="value mini" id="hrrStatus${slot}">Test baslatilmadi</div>
        <button class="btn" style="margin-top:8px" onclick="startHrrTestNow(${slot})">Testi Baslat (efor bitince bas)</button>
      </div>
      <div class="card big">
        <div class="label">Nabiz Bolgeleri (bu oturum)</div>
        <div class="stackbar" style="margin-top:10px">
          <div class="stackbar-seg" id="segZ1_${slot}" style="width:0%;background:#60a5fa"></div>
          <div class="stackbar-seg" id="segZ2_${slot}" style="width:0%;background:#4ade80"></div>
          <div class="stackbar-seg" id="segZ3_${slot}" style="width:0%;background:#facc15"></div>
          <div class="stackbar-seg" id="segZ4_${slot}" style="width:0%;background:#fb923c"></div>
          <div class="stackbar-seg" id="segZ5_${slot}" style="width:0%;background:#ef4444"></div>
        </div>
        <div class="legend-row">
          <span class="legend-chip"><i style="background:#60a5fa"></i>Z1</span>
          <span class="legend-chip"><i style="background:#4ade80"></i>Z2</span>
          <span class="legend-chip"><i style="background:#facc15"></i>Z3</span>
          <span class="legend-chip"><i style="background:#fb923c"></i>Z4</span>
          <span class="legend-chip"><i style="background:#ef4444"></i>Z5</span>
        </div>
      </div>
    </div>
  `;
}

// Slot henuz DOM'da yoksa (bu oturumda ilk kez enabled=true goruldu) karti
// olusturup slot NUMARASINA gore dogru sirada (slot 0 en ustte) ekler.
function buildSlotDom(slot){
  if (document.getElementById('slotCard'+slot)) return;

  const wrap = document.createElement('div');
  wrap.id = 'slotCard' + slot;
  wrap.innerHTML = slotCardHtml(slot);

  const container = document.getElementById('slotsContainer');
  let inserted = false;
  for (const child of container.children) {
    const childSlot = parseInt(child.id.replace('slotCard', ''), 10);
    if (childSlot > slot) {
      container.insertBefore(wrap, child);
      inserted = true;
      break;
    }
  }
  if (!inserted) container.appendChild(wrap);
}

function renderSlot(slot, s){
  buildSlotDom(slot);

  document.getElementById('riskText'+slot).innerText = s.riskStatus;
  document.getElementById('riskScore'+slot).innerText = s.fatigue;
  document.getElementById('warning'+slot).innerText = s.warning;

  const readyBlock = document.getElementById('riskReadyBlock'+slot);
  const notReadyBlock = document.getElementById('riskNotReadyBlock'+slot);
  const meterWrap = document.getElementById('fatigueMeterWrap'+slot);
  const riskBoxEl = document.getElementById('riskBox'+slot);
  if(s.baselineReady){
    readyBlock.style.display = '';
    notReadyBlock.style.display = 'none';
    meterWrap.style.display = '';
    riskBoxEl.style.background = s.riskColor;
    riskBoxEl.style.color = '';
    setFill('fatigueMeterFill'+slot, s.fatigue, s.riskColor);
  } else {
    readyBlock.style.display = 'none';
    notReadyBlock.style.display = '';
    meterWrap.style.display = 'none';
    riskBoxEl.style.background = 'var(--card2)';
    riskBoxEl.style.color = 'var(--text)';
  }

  const trendBox = document.getElementById('trendBox'+slot);
  if(s.trendWarning && s.trendWarning.length > 0){
    document.getElementById('trendWarning'+slot).innerText = s.trendWarning;
    trendBox.style.display = '';
  } else {
    trendBox.style.display = 'none';
  }

  document.getElementById('hrLabel'+slot).innerText = 'Nabiz (' + (s.playerName || s.bandLabel) + ')';
  document.getElementById('hrBpm'+slot).innerText = s.fresh ? s.bpm : '--';
  document.getElementById('hrStatus'+slot).innerText = s.fresh
    ? (s.contact ? 'Bagli - temas iyi' : 'Bagli - temas zayif olabilir')
    : s.status;

  document.getElementById('hrPctMax'+slot).innerText = s.fresh && s.pctMax > 0 ? s.pctMax.toFixed(0) : '--';
  renderHrZones(slot, s.fresh ? s.zone : 0, s.zoneSec);

  // ACWR/Monotonluk: acwrDays 3'un altindaysa (bkz. PlayerMath::calculateAcwr)
  // sunucu zaten "Yetersiz veri" bandiyla donuyor - burada sadece gosteriyoruz.
  document.getElementById('acwrValue'+slot).innerText = s.acwrDays >= 3 ? s.acwr.toFixed(2) : '--';
  document.getElementById('acwrBand'+slot).innerText = s.acwrBand;
  document.getElementById('monotonyValue'+slot).innerText = s.acwrDays >= 3 ? s.monotony.toFixed(2) : '--';
  document.getElementById('monotonyBand'+slot).innerText = s.monotonyBand;

  // HRV karti sadece cihaz/mod bu veriyi GERCEKTEN gonderiyorsa gorunur -
  // Polar Sense bu yayin modunda gondermeyebilir (bkz. Config.h notu).
  document.getElementById('rmssdCard'+slot).style.display = s.rrSupported ? '' : 'none';
  if (s.rrSupported) {
    document.getElementById('rmssdValue'+slot).innerText = s.fresh ? s.rmssd.toFixed(0) : '--';
    document.getElementById('sdnnValue'+slot).innerText = s.fresh ? s.sdnn.toFixed(0) : '--';
    document.getElementById('pnn50Value'+slot).innerText = s.fresh ? s.pnn50.toFixed(0) : '--';
  }

  // Kalp Hizi Toparlanmasi (HRR) - devam eden veya tamamlanmis son test.
  const hrrEl = document.getElementById('hrrStatus'+slot);
  if (s.hrrActive) {
    hrrEl.innerText = 'Test suruyor... (' + s.hrrElapsedSec + 'sn) HR0=' + s.hrrHr0 + ' bpm'
      + (s.hrr1 >= 0 ? (' | 1dk dusus: ' + s.hrr1 + ' bpm') : '');
  } else if (s.hrr2 >= 0) {
    hrrEl.innerText = 'Son test: HR0=' + s.hrrHr0 + ' bpm | 1dk dusus: ' + s.hrr1 + ' bpm | 2dk dusus: ' + s.hrr2 + ' bpm';
  } else if (s.hrr1 >= 0) {
    hrrEl.innerText = 'Son test: HR0=' + s.hrrHr0 + ' bpm | 1dk dusus: ' + s.hrr1 + ' bpm (2dk olculmedi)';
  } else {
    hrrEl.innerText = 'Test baslatilmadi';
  }

  // Sunucudaki mevcut atamayla dropdown'u senkron tutar (baska bir cihaz/
  // sekmeden atama degistiyse de burada gorunur). Kullanici o an secim
  // yapiyorsa (odakta) dokunmaz.
  const sel = document.getElementById('playerSelect'+slot);
  if (document.activeElement !== sel) sel.value = String(s.playerId);
}

// Sunucu HR_SLOTS (Config.h HR_DEVICE_MAC[] boyutu, su an 9) kadar slot
// donuyor ama coğu "enabled":false olabilir (fiziksel bant/MAC tanimli
// degil) - panelde SADECE enabled=true olan slotlar icin kart olusturulur
// (bkz. buildSlotDom), digerleri hic gorunmez/DOM'a girmez.
function loadData(){
 // 'ts': telefonun GERCEK epoch saniyesi - ESP32'nin RTC'si yok, ACWR'nin
 // "bugun"unu bu sekilde tahmin ediyor (bkz. Globals.h updateEpochSync).
 fetch('/data?ts=' + Math.floor(Date.now()/1000))
 .then(r=>r.json())
 .then(d=>{
   document.getElementById('trainingTime').innerText = d.trainingTime;

   for (const s of d.slots) {
     if (!s.enabled) continue;
     renderSlot(s.slot, s);
   }

   const s0 = d.slots[0];
   if (s0 && s0.enabled) {
     document.getElementById('sbRisk').innerText = s0.riskStatus;
     document.getElementById('sbRisk').style.background = s0.riskColor;
     document.getElementById('sbHr').innerText = s0.fresh ? s0.bpm : '--';
   }
 })
 .catch(e=>{});
}

// ---------------- Oyuncu Roster'i + Bant Atamasi ----------------
let rosterCache = [];
function loadRoster(){
  fetch('/roster')
  .then(r=>r.json())
  .then(list=>{
    rosterCache = list;
    // Sadece SU AN DOM'da bulunan (enabled=true, buildSlotDom ile olusturulmus)
    // slotlarin dropdown'lari doldurulur.
    document.querySelectorAll('select[id^="playerSelect"]').forEach(sel=>{
      const current = sel.value;
      sel.innerHTML = '<option value="0">-- Oyuncu secilmedi --</option>';
      for (const p of list) {
        const opt = document.createElement('option');
        opt.value = p.id;
        opt.textContent = p.name;
        sel.appendChild(opt);
      }
      if (current && list.some(p => String(p.id) === current)) sel.value = current;
    });
  })
  .catch(e=>{});
}

function addPlayerNow(){
  const input = document.getElementById('newPlayerName');
  const name = input.value.trim();
  if (!name) return;
  fetch('/addplayer?name=' + encodeURIComponent(name), { method: 'POST', cache: 'no-store' })
  .then(r=>r.json())
  .then(res=>{
    if (res.id) {
      input.value = '';
      loadRoster();
    } else if (res.error) {
      alert(res.error);
    }
  })
  .catch(e=>{});
}

function assignSlotNow(slot){
  const sel = document.getElementById('playerSelect'+slot);
  const id = sel.value;
  fetch('/assignslot?slot=' + slot + '&id=' + id, { method: 'POST', cache: 'no-store' })
  .then(() => setTimeout(loadData, 200))
  .catch(e=>{});
}

function startHrrTestNow(slot){
  fetch('/starthrrtest?slot=' + slot, { method: 'POST', cache: 'no-store' })
  .then(r=>r.json())
  .then(res=>{
    if (res.error) alert(res.error);
    else setTimeout(loadData, 200);
  })
  .catch(e=>{});
}

// Panel acilista bir kere okur - "Sunucuya Gonder" butonlarinin hedef URL'i,
// cihaz adi ve anahtari. Cihazin kendisi internete cikmiyor (bkz. Config.h
// GPS_UPLOAD_URL yorumu) - bu fetch() telefonun TARAYICISINDAN, WiFi (cihaza)
// ile mobil veri (internete) AYNI ANDA calisabildigi icin gider.
let gpsCfg = null;
function loadGpsConfig(){
  fetch('/gpsconfig').then(r=>r.json()).then(c=>{ gpsCfg = c; }).catch(e=>{});
}

function uploadSessionToServer(session, btn){
  if(!gpsCfg){
    btn.textContent = 'Yapilandirma yuklenmedi, tekrar dene';
    return;
  }
  btn.disabled = true;
  btn.textContent = 'Gonderiliyor...';
  const payload = Object.assign({ deviceName: gpsCfg.deviceName }, session);
  fetch(gpsCfg.uploadUrl, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'X-Device-Key': gpsCfg.deviceKey },
    body: JSON.stringify(payload)
  })
  .then(r=>{
    if(!r.ok) throw new Error('HTTP '+r.status);
    return r.json();
  })
  .then(d=>{
    btn.textContent = 'Gonderildi ✓';
  })
  .catch(e=>{
    btn.disabled = false;
    btn.textContent = 'Gonderilemedi, tekrar dene (mobil veri acik mi?)';
  });
}

function loadHistory(){
 fetch('/history')
 .then(r=>r.json())
 .then(list=>{
   const el = document.getElementById('historyList');
   if(!list.length){
     el.innerHTML = '<div class="pill">Henuz kayitli antrenman yok</div>';
     return;
   }
   let html = '';
   for(let i=list.length-1;i>=0;i--){
     const s = list[i];
     const dateStr = s.ts ? new Date(s.ts*1000).toLocaleString('tr-TR') : ('Antrenman #'+s.n);
     const loadStr = s.est ? (s.load.toFixed(0)+' AU (tahmini - RPE girilmedi)') : (s.load.toFixed(0)+' AU (RPE '+s.rpe+')');
     const hrStr = s.maxHr > 0 ? (s.maxHr.toFixed(0)+' bpm') : '--';
     html += '<div class="card big">'
       + '<div class="label">'+dateStr+' - '+s.duration+'</div>'
       + '<div class="value mini">Maks Nabiz: '+hrStr+' | Antrenman Yuku: '+loadStr+'</div>'
       + '<button type="button" class="uploadBtn" data-idx="'+i+'" style="margin-top:8px">Sunucuya Gonder</button>'
       + '<button type="button" class="deleteBtn" data-n="'+s.n+'" style="margin-top:8px;background:#7c2d12;color:white;border:0;border-radius:12px;padding:10px;width:100%;font-weight:800">Sil</button>'
       + '</div>';
   }
   el.innerHTML = html;
   el.querySelectorAll('.uploadBtn').forEach(function(btn){
     btn.addEventListener('click', function(){
       const idx = Number(btn.getAttribute('data-idx'));
       uploadSessionToServer(list[idx], btn);
     });
   });
   el.querySelectorAll('.deleteBtn').forEach(function(btn){
     btn.addEventListener('click', function(){
       if(!confirm('Bu antrenman kaydi silinsin mi? Bu islem geri alinamaz.')) return;
       const n = btn.getAttribute('data-n');
       btn.disabled = true;
       fetch('/deletehistory?n='+n, { cache: 'no-store' })
       .then(() => loadHistory());
     });
   });
 })
 .catch(e=>{});
}

// Takim (ESP-NOW ile toplanan diger oyuncular) - sadece hub cihazinda dolu gelir.
function renderTeam(data){
  const title = document.getElementById('teamTitle');
  const list = document.getElementById('teamList');

  if(!data.isHub){
    title.style.display = 'none';
    list.style.display = 'none';
    return;
  }
  title.style.display = '';
  list.style.display = '';

  if(!data.players.length){
    list.innerHTML = '<div class="pill">Henuz baska oyuncu tespit edilmedi</div>';
    return;
  }

  let html = '';
  data.players.forEach((p) => {
    const statusTxt = p.ago > 10 ? ' (baglanti zayif)' : '';
    const zoneLabel = HR_ZONE_LABELS[p.zone] || '--';
    const zoneColor = HR_ZONE_COLORS[p.zone] || 'var(--muted)';
    html += '<div class="card big">'
      + '<div class="label">' + p.name + statusTxt + ' - ' + p.duration + '</div>'
      + '<div class="value" style="font-size:28px">' + (p.bpm > 0 ? p.bpm : '--') + '<span class="unit"> bpm</span>'
      + ' <span class="zone-badge" style="background:' + zoneColor + '">' + zoneLabel + '</span></div>'
      + '<div class="value mini">Yorgunluk: ' + p.fatigue + '/100 (' + p.risk + ')</div>'
      + '</div>';
  });
  list.innerHTML = html;
}

function loadTeam(){
 fetch('/team')
 .then(r=>r.json())
 .then(renderTeam)
 .catch(e=>{});
}

setInterval(loadData,1000);
setInterval(loadTeam,2000);
setInterval(loadRoster,10000);
window.onload=function(){
 loadGpsConfig();
 loadRoster();
 loadData();
 loadHistory();
 loadTeam();
};

window.addEventListener('scroll', function(){
  var bar = document.getElementById('stickyBar');
  if(window.scrollY > 180) bar.classList.add('show');
  else bar.classList.remove('show');
}, { passive: true });

function resetNow(){
 let rpeInput = prompt('Antrenmanin zorluk algisi (RPE) 1-10 arasi girin (bos gecebilirsiniz):', '');
 let rpe = parseInt(rpeInput);
 if(isNaN(rpe) || rpe < 1 || rpe > 10) rpe = 0;

 if(confirm('Antrenman verileri sifirlansin mi?')){
   fetch('/reset?ts=' + Math.floor(Date.now()/1000) + '&rpe=' + rpe, { cache: 'no-store' })
  .then(() => {
    setTimeout(loadData, 300);
    setTimeout(loadHistory, 300);
  });
 }
}

function resetSeasonNow(){
 if(!confirm('DIKKAT: Kisisel sezon nabiz rekoru tamamen silinecek. Bu, cihaz yeni bir sporcuya gecerken kullanilir. Devam edilsin mi?')) return;
 if(!confirm('Emin misiniz? Bu islem geri alinamaz.')) return;

 fetch('/resetseason', { cache: 'no-store' })
 .then(() => {
   setTimeout(loadData, 300);
 });
}
</script>
</head>

<body>

<div class="stickybar" id="stickyBar">
  <span class="sb-name">TOAPERFORM</span>
  <span class="sb-risk" id="sbRisk" style="background:#22c55e">NORMAL</span>
  <span class="sb-hr"><span id="sbHr">--</span> <span>bpm</span></span>
</div>

<div class="container">

  <div class="header">
    <div class="brand">TOAPERFORM</div>
    <div class="title">Athlete Heart Rate</div>
    <div class="sub">Nabiz ile antrenman yorgunlugu takibi - coklu oyuncu</div>
  </div>

  <div class="card big">
    <div class="label">Antrenman Suresi</div>
    <div class="value" id="trainingTime">00:00:00</div>
  </div>

  <div class="section-title">Oyuncular</div>
  <div class="card big">
    <div class="label">Yeni Oyuncu Ekle (ortak havuz)</div>
    <div style="display:flex;gap:8px;margin-top:8px">
      <input id="newPlayerName" type="text" placeholder="Isim" maxlength="23"
        style="flex:1;padding:10px;border-radius:10px;border:1px solid var(--border);background:var(--card2);color:var(--text)">
      <button class="btn" style="width:auto;padding:10px 16px" onclick="addPlayerNow()">Ekle</button>
    </div>
  </div>

  <!-- Bant/oyuncu kartlari sunucudan gelen slot sayisina (enabled=true olanlar)
       gore JS ile DINAMIK olusturulur (bkz. buildSlotDom) - Config.h'daki
       HR_DEVICE_MAC[] SLOT_COUNT'u (9) degistirse bile burada elle blok
       eklemeye gerek kalmaz. -->
  <div id="slotsContainer"></div>

  <div class="section-title" id="teamTitle" style="display:none">Takim</div>
  <div class="grid" id="teamList" style="display:none">
    <div class="pill">Yukleniyor...</div>
  </div>

  <div class="section-title">Gecmis Antrenmanlar</div>
  <div class="grid" id="historyList">
    <div class="pill">Yukleniyor...</div>
  </div>

<div class="card big">
  <div class="label">Titresim Kodlari (oyun icinde anlik geri bildirim)</div>
  <div class="value mini">
    3 titresim: KRITIK yorgunluk
  </div>
</div>

<div class="card big">
  <div class="label">Sistem</div>
  <div class="value mini">
  </div>

  <a href="/update"
     style="
      display:block;
      margin-top:12px;
      background:#16a34a;
      color:white;
      text-decoration:none;
      text-align:center;
      padding:14px;
      border-radius:14px;
      font-weight:800;">
      Yazilim Guncelle
  </a>

  <a href="/backup" download="toaperform_yedek.json"
     style="
      display:block;
      margin-top:12px;
      background:#0d9488;
      color:white;
      text-decoration:none;
      text-align:center;
      padding:14px;
      border-radius:14px;
      font-weight:800;">
      Yedek Indir (Sezon + Gecmis)
  </a>

  <a href="/report" target="_blank"
     style="
      display:block;
      margin-top:12px;
      background:#7c3aed;
      color:white;
      text-decoration:none;
      text-align:center;
      padding:14px;
      border-radius:14px;
      font-weight:800;">
      Oyuncu Raporu (Yazdirilabilir)
  </a>
</div>


  <button class="btn" onclick="resetNow()">Antrenmani Sifirla</button>
  <button class="btn" style="background:#7c2d12" onclick="resetSeasonNow()">Yeni Sporcu / Sezonu Sifirla</button>

  <div class="footer">
    Nabiz tabanli antrenman yuku takip sistemidir. Yorgunluk gostergesi nabiz
    tabanli ic yuk tahminine (kisisel sezon rekoru nabza gore) dayanir ve
    bilgilendirme amaclidir, tibbi teshis amaci tasimaz.
  </div>

</div>
</body>
</html>
)rawliteral";

static const char REPORT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>TOAPERFORM Oyuncu Raporu</title>
<style>
*{box-sizing:border-box}
body{
  margin:0;
  font-family:Arial,Helvetica,sans-serif;
  background:#f1f5f9;
  color:#0f172a;
}
.sheet{
  max-width:640px;
  margin:auto;
  background:white;
  padding:32px;
}
.brand-row{
  display:flex;
  justify-content:space-between;
  align-items:baseline;
  border-bottom:3px solid #0f172a;
  padding-bottom:12px;
  margin-bottom:20px;
}
.brand{ font-size:20px; font-weight:900; letter-spacing:.5px; }
.gendate{ font-size:12px; color:#64748b; }
.player-row{
  display:flex;
  justify-content:space-between;
  align-items:center;
  margin-bottom:20px;
}
.player-name{ font-size:26px; font-weight:900; }
h2{
  font-size:13px;
  text-transform:uppercase;
  letter-spacing:.5px;
  color:#475569;
  border-bottom:1px solid #cbd5e1;
  padding-bottom:6px;
  margin:24px 0 12px;
}
.stat-grid{
  display:grid;
  grid-template-columns:1fr 1fr 1fr;
  gap:10px;
}
.stat{
  background:#f8fafc;
  border:1px solid #e2e8f0;
  border-radius:10px;
  padding:10px;
  text-align:center;
}
.stat-val{ font-size:20px; font-weight:900; }
.stat-label{ font-size:10px; color:#64748b; font-weight:700; margin-top:2px; }
.risk-line{ font-size:14px; line-height:1.7; }
.methodology{
  font-size:10.5px;
  color:#64748b;
  line-height:1.6;
  margin-top:24px;
  border-top:1px solid #cbd5e1;
  padding-top:12px;
}
.actions{ margin-top:24px; display:flex; gap:10px; }
.actions button, .actions a{
  flex:1;
  text-align:center;
  padding:12px;
  border-radius:10px;
  border:0;
  font-weight:800;
  cursor:pointer;
  text-decoration:none;
}
.btn-print{ background:#0284c7; color:white; }
.btn-back{ background:#e2e8f0; color:#0f172a; }
@media print{
  body{ background:white; }
  .actions{ display:none; }
}
</style>
</head>
<body>
<div class="sheet">
  <div class="brand-row">
    <div class="brand">TOAPERFORM</div>
    <div class="gendate" id="genDate"></div>
  </div>

  <div class="player-row">
    <div>
      <div class="player-name" id="rDeviceName">-</div>
    </div>
  </div>

  <h2>Kariyer Ozeti</h2>
  <div class="stat-grid">
    <div class="stat"><div class="stat-val" id="rSessions">-</div><div class="stat-label">ANTRENMAN</div></div>
    <div class="stat"><div class="stat-val" id="rMaxHr">-</div><div class="stat-label">MAKS NABIZ bpm</div></div>
    <div class="stat"><div class="stat-val" id="rLoad">-</div><div class="stat-label">TOPLAM YUK AU</div></div>
  </div>

  <h2>Guncel Durum</h2>
  <div class="risk-line" id="rFatigue">Veri yukleniyor...</div>

  <div class="methodology">
    <b>Metodoloji ve kaynaklar:</b> Nabiz tabanli ic yuk / Internal-external
    yuk ayrimi - Bourdon ve ark., Int J Sports Physiol Perform 2017; Antrenman
    yuku (Foster session-RPE) - Foster ve ark., J Strength Cond Res 2001.
    Bu rapor tibbi teshis amaci tasimaz.
  </div>

  <div class="actions">
    <button class="btn-print" onclick="window.print()">Yazdir / PDF Kaydet</button>
    <a class="btn-back" href="/">Panele Don</a>
  </div>
</div>

<script>
document.getElementById('genDate').textContent = new Date().toLocaleDateString('tr-TR');

fetch('/backup').then(r=>r.json()).then(b=>{
  document.getElementById('rDeviceName').textContent = b.deviceName;
  document.getElementById('rSessions').textContent = b.seasonSessionCount;
  document.getElementById('rMaxHr').textContent = b.seasonMaxHrEver.toFixed(0);
  document.getElementById('rLoad').textContent = b.seasonTotalLoad.toFixed(0);
}).catch(e=>{});

fetch('/data').then(r=>r.json()).then(d=>{
  const s0 = d.slots[0];
  document.getElementById('rFatigue').textContent =
    'Anlik yorgunluk skoru: ' + s0.fatigue + '/100 (' + s0.riskStatus + ').';
}).catch(e=>{});
</script>
</body>
</html>
)rawliteral";

static const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>TOAPERFORM OTA</title>
<style>
body{font-family:Arial;background:#020617;color:white;text-align:center;padding:25px}
.card{max-width:420px;margin:auto;background:#0f172a;padding:22px;border-radius:18px}
input,button{width:100%;padding:14px;margin-top:12px;border-radius:12px;border:0}
button{background:#0284c7;color:white;font-weight:bold}
a{color:#38bdf8}
</style>
</head>
<body>
<div class="card">
<h2>TOAPERFORM OTA Update</h2>
<p>.bin dosyasini secip yukleyin.</p>
<form method="POST" action="/update" enctype="multipart/form-data">
<input type="file" name="update">
<button type="submit">Guncellemeyi Yukle</button>
</form>
<p><a href="/">Panele Don</a></p>
</div>
</body>
</html>
)rawliteral";
