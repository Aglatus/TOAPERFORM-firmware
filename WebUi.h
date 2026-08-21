#pragma once
// =====================================================
// TOAPERFORM - WebUi.h
// Panel/rapor sayfalarinin HTML+CSS+JS govdesi. Buyuk sabitler PROGMEM'de
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
/* Telefonda 520px dar sutun dogru, ama iPad/tablet gibi genis ekranlarda ayni
   sinir tum sayfayi ortada kucuk bir serit halinde birakiyordu (kullanici
   bulgusu, 2026-08: "ortalanmis durumda, tam ekrana sigdir"). Genis ekranlarda
   container'i acip, icindeki grid'leri de ekstra genislige gore sutunlayip
   bosluk yerine icerigi buyutuyoruz.*/
@media (min-width: 700px){
  .container{ max-width:920px; }
  .grid{ grid-template-columns:repeat(4, 1fr); }
}
@media (min-width: 1100px){
  .container{ max-width:1180px; }
  .team-grid{ grid-template-columns:repeat(4, 1fr); }
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
/* Takim Risk Panosu - HR_SLOTS'a kadar (9) oyuncu kartinin tek ekranda
   makul sekilde sigmasi icin, genel .card/.grid'den daha kompakt bir varyant.
   Telefonda (dar ekran) auto-fill ile 2-3 sutuna duser; 700px ustunde
   (iPad portre ve uzeri, bkz. asagidaki @media) SABIT 3 sutuna gecer - 9
   oyuncu TAM 3x3 duzenli sigsin diye (kullanici talebi: "iPad'de baktiginda
   hepsi sigmali"). */
.team-grid{
  display:grid;
  grid-template-columns:repeat(auto-fill, minmax(130px, 1fr));
  gap:8px;
}
@media (min-width: 700px){
  .team-grid{
    grid-template-columns:repeat(3, 1fr);
  }
}
.team-card{
  background:linear-gradient(180deg,var(--card),var(--card2));
  border:1px solid var(--line);
  border-radius:14px;
  padding:8px 10px;
  box-shadow:0 4px 12px rgba(0,0,0,.25);
  min-width:0;  /* uzun isimlerin grid hucresini genisletmesini onler */
}
.team-card .tc-name{
  color:var(--muted);
  font-size:11px;
  font-weight:700;
  white-space:nowrap;
  overflow:hidden;
  text-overflow:ellipsis;
}
.team-card .tc-bpm{
  font-family:"TP Display",-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  font-size:20px;
  font-weight:900;
  font-variant-numeric:tabular-nums;
  margin-top:2px;
}
.team-card .tc-chips{
  display:flex;
  gap:4px;
  flex-wrap:wrap;
  margin-top:4px;
}
.team-card .tc-chips .zone-badge{
  margin-top:0;
  padding:2px 7px;
  font-size:10px;
}
.team-card .tc-chips .legend-chip{
  font-size:10px;
  padding:2px 6px;
  border-radius:999px;
  gap:0;
}
/* Isi Kartlari (F) - Polar Team Pro "Whole Team" ekranina benzer duz-renk
   kutu gorunumu (bkz. renderCardHeat). Zon rengi ARKA PLAN olarak dolduruldugu
   icin metin hep koyu (#020617) - HR_ZONE_COLORS'daki tum renkler (mavi/yesil/
   sari/turuncu/kirmizi) yeterince acik oldugundan koyu metin her zonda okunur. */
.heat-card{
  text-align:center;
  color:#020617;
  border:none;
}
.heat-card .hc-name{
  font-size:11px; font-weight:800; text-transform:uppercase; letter-spacing:.02em;
  white-space:nowrap; overflow:hidden; text-overflow:ellipsis;
}
.heat-card .hc-pct{
  font-family:"TP Display",-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  font-size:28px; font-weight:900; margin-top:2px; line-height:1;
}
.heat-card .hc-unit{ font-size:13px; font-weight:800; margin-left:1px; }
.heat-card .hc-sub{ font-size:10px; font-weight:700; opacity:.75; margin-top:3px; }

/* Detayli Tablo (E) - dar telefon ekraninda TUM metrik sutunlari sigmaz,
   bu yuzden kendi yatay kaydirma kutusuna sarilir (bkz. .detail-table-wrap
   grid-column:1/-1 - team-grid'in tum genisligini kaplar, digerleri gibi
   3'lu sutuna bolunmez). */
.detail-table-wrap{
  grid-column:1/-1;
  overflow-x:auto;
  border:1px solid var(--line);
  border-radius:14px;
  background:linear-gradient(180deg,var(--card),var(--card2));
}
.detail-table{
  border-collapse:collapse;
  width:100%;
  min-width:800px;
  font-size:11px;
}
.detail-table th,.detail-table td{
  padding:8px 10px;
  text-align:center;
  white-space:nowrap;
  border-bottom:1px solid var(--line);
}
.detail-table th{
  color:var(--muted);
  font-weight:800;
  text-transform:uppercase;
  letter-spacing:.04em;
  font-size:9px;
}
.detail-table td.dt-name{
  text-align:left;
  font-weight:700;
  color:var(--text);
  position:sticky;
  left:0;
  background:var(--card2);
}
.detail-table tr:last-child td{ border-bottom:none; }
.detail-table tbody tr{ cursor:pointer; }
.detail-table tbody tr:hover td{ background:rgba(255,255,255,.04); }
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
    <div class="card big">
      <div class="label">Gunluk Wellness Anketi (antrenman oncesi, 1=iyi - 10=kotu)</div>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px">
        <label style="font-size:13px">Uyku kalitesi (kotu)<input type="number" min="1" max="10" value="5" id="wSleep${slot}" style="width:100%;padding:8px;margin-top:4px;border-radius:8px;border:1px solid var(--border);background:var(--card2);color:var(--text)"></label>
        <label style="font-size:13px">Yorgunluk<input type="number" min="1" max="10" value="5" id="wFatigue${slot}" style="width:100%;padding:8px;margin-top:4px;border-radius:8px;border:1px solid var(--border);background:var(--card2);color:var(--text)"></label>
        <label style="font-size:13px">Kas agrisi<input type="number" min="1" max="10" value="5" id="wSoreness${slot}" style="width:100%;padding:8px;margin-top:4px;border-radius:8px;border:1px solid var(--border);background:var(--card2);color:var(--text)"></label>
        <label style="font-size:13px">Stres<input type="number" min="1" max="10" value="5" id="wStress${slot}" style="width:100%;padding:8px;margin-top:4px;border-radius:8px;border:1px solid var(--border);background:var(--card2);color:var(--text)"></label>
        <label style="font-size:13px">Ruh hali (kotu)<input type="number" min="1" max="10" value="5" id="wMood${slot}" style="width:100%;padding:8px;margin-top:4px;border-radius:8px;border:1px solid var(--border);background:var(--card2);color:var(--text)"></label>
      </div>
      <button class="btn" style="margin-top:10px" onclick="submitWellnessNow(${slot})">Anketi Kaydet</button>
      <div class="legend-chip" id="wellnessStatus${slot}" style="margin-top:8px;color:var(--muted-light)">Bugun icin veri yok</div>
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
        <div class="value mini">Solunum (tahmini): <span id="breathingValue${slot}">--</span> nefes/dk</div>
        <div class="legend-chip" style="margin-top:6px;color:var(--muted-light)">Hareket halinde - dinlenme HRV'si/solunumu degil, dusuk tempoda daha guvenilir</div>
      </div>
      <div class="card big">
        <div class="label">Kalp Hizi Toparlanmasi (HRR)</div>
        <div class="value mini" id="hrrStatus${slot}">Test baslatilmadi</div>
        <button class="btn" style="margin-top:8px" onclick="startHrrTestNow(${slot})">Testi Baslat (efor bitince bas)</button>
      </div>
      <div class="card big">
        <div class="label">Ortostatik Toparlanma Testi (yatarken/otururken -&gt; ayaktayken)</div>
        <div class="value mini" id="orthoStatus${slot}">Test baslatilmadi</div>
        <button class="btn" style="margin-top:8px" onclick="startOrthoTestNow(${slot})">Testi Baslat (once yatarken/otururken bas)</button>
        <div class="legend-chip" style="margin-top:6px;color:var(--muted-light)">Ham deger - risk bandi yok. Faz1 2dk sonra panel "Ayaga kalk" der.</div>
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
    document.getElementById('breathingValue'+slot).innerText = (s.fresh && s.breathingRate > 0) ? s.breathingRate.toFixed(0) : '--';
  }

  // Ortostatik Toparlanma Testi - devam eden fazi ("AYAGA KALK" uyarisi Faz2'ye
  // gecince gorunur) ya da tamamlanmis son sonucu gosterir.
  const orthoEl = document.getElementById('orthoStatus'+slot);
  if (s.orthoActive) {
    if (s.orthoPhase === 1) {
      orthoEl.innerText = 'Faz 1 (yatarken/otururken) suruyor... (' + s.orthoElapsedSec + 'sn / 120sn)';
    } else {
      orthoEl.innerHTML = '<b style="color:#facc15">AYAGA KALK</b> - Faz 2 suruyor... (' + s.orthoElapsedSec + 'sn / 120sn)'
        + (s.orthoHr1 >= 0 ? (' | Faz1 ort: ' + s.orthoHr1.toFixed(0) + ' bpm') : '');
    }
  } else if (s.orthoHr2 >= 0) {
    const dBpm = s.orthoHr2 - s.orthoHr1;
    orthoEl.innerText = 'Son test: Faz1 ' + s.orthoHr1.toFixed(0) + ' bpm -> Faz2 ' + s.orthoHr2.toFixed(0) + ' bpm (fark: '
      + (dBpm >= 0 ? '+' : '') + dBpm.toFixed(0) + ' bpm)'
      + (s.orthoRmssd1 >= 0 && s.orthoRmssd2 >= 0 ? (' | RMSSD: ' + s.orthoRmssd1.toFixed(0) + 'ms -> ' + s.orthoRmssd2.toFixed(0) + 'ms') : '');
  } else {
    orthoEl.innerText = 'Test baslatilmadi';
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

  // Wellness anketi - bugun icin kaydedilmis bir deger varsa gosterir
  // (kullanici formu tekrar doldurmadan uzerine yazmiyoruz, sadece durum yazisi).
  const wEl = document.getElementById('wellnessStatus'+slot);
  wEl.innerText = s.wellnessHasData
    ? ('Bugun: ' + s.wellnessSum + '/50 (' + s.wellnessBand + ')')
    : 'Bugun icin veri yok';

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
   renderTeamDashboard(d.slots);

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

function startOrthoTestNow(slot){
  fetch('/startorthotest?slot=' + slot, { method: 'POST', cache: 'no-store' })
  .then(r=>r.json())
  .then(res=>{
    if (res.error) alert(res.error);
    else setTimeout(loadData, 200);
  })
  .catch(e=>{});
}

function submitWellnessNow(slot){
  const sel = document.getElementById('playerSelect'+slot);
  const id = sel.value;
  if (!id || id === '0') { alert('Once bir oyuncu secin'); return; }

  const v = (elId) => document.getElementById(elId).value;
  const params = 'id=' + id
    + '&sleep=' + v('wSleep'+slot)
    + '&fatigue=' + v('wFatigue'+slot)
    + '&soreness=' + v('wSoreness'+slot)
    + '&stress=' + v('wStress'+slot)
    + '&mood=' + v('wMood'+slot);

  fetch('/wellness?' + params, { method: 'POST', cache: 'no-store' })
  .then(r=>r.json())
  .then(res=>{
    if (res.error) alert(res.error);
    else setTimeout(loadData, 200);
  })
  .catch(e=>{});
}

// ---------------- Takim Panosu: 4 secilebilir gorunum ----------------
// Kullanici karari (2026-08-18): tek bir tasarima karar vermek yerine 4
// secenegin (Halka/Monitor/Trend/Odak Modu) hepsi CANLI veriyle sistemde
// dursun, panelden istedigi an degistirebilsin. Secim SADECE o tarayicida
// (localStorage) saklanir - cihaza/diger telefonlara yazilmaz, kisisel bir
// goruntuleme tercihidir.
let dashViewMode = localStorage.getItem('toaDashViewMode') || 'ring';
let lastSlotsGlobal = null;
let openFocusSlot = null;
// Oyuncu bazli cok-oturumlu trend (2026-08 ekleme) - Focus Modu acildiginda
// AYRI bir fetch ile /playertrend'den cekilir (bkz. loadFocusTrend), /data
// gibi surekli yoklanmaz. playerId degismedikce tekrar fetch edilmez.
let focusTrendCache = { playerId: null, entries: [] };

function onDashViewModeChange(){
  dashViewMode = document.getElementById('dashViewMode').value;
  localStorage.setItem('toaDashViewMode', dashViewMode);
  if (dashViewMode !== 'focus') closeFocusMode();
  if (lastSlotsGlobal) renderTeamDashboard(lastSlotsGlobal);
}

// Monitor/Trend gorunumleri "canli dalga/trend" gosterir ama SUNUCU gecmis
// bpm tutmuyor (sadece anlik deger) - bu yuzden her loadData() turunda bir
// ornek eklenerek TARAYICIDA (sayfa yenilenince sifirlanan, kucuk bir
// tampon) tutulur. fresh=false ise (sinyal yok) bosluk birakilir.
const bpmHistory = {};
function pushBpmHistory(slot, bpm, fresh){
  if (!bpmHistory[slot]) bpmHistory[slot] = [];
  const h = bpmHistory[slot];
  h.push(fresh ? bpm : null);
  if (h.length > 60) h.shift();
}
function buildSparklinePoints(hist, width, height, minV, maxV){
  const vals = hist.filter(v => v !== null && v !== undefined);
  if (vals.length < 2) return '';
  const n = hist.length;
  const range = Math.max(1, maxV - minV);
  let lastKnown = vals[0];
  return hist.map((v, i) => {
    const x = (i / (n - 1)) * width;
    if (v !== null && v !== undefined) lastKnown = v;
    const y = height - ((lastKnown - minV) / range) * height;
    return x.toFixed(1) + ',' + y.toFixed(1);
  }).join(' ');
}
// pct: 0-100. Daireyi (verilen cevre uzunlugunda) saat yonunde pct kadar
// doldurmak icin gereken stroke-dashoffset'i hesaplar (12 hizasindan baslar,
// bkz. SVG'deki rotate(-90) donusleri).
function gaugeOffset(circumference, pct){
  const p = Math.max(0, Math.min(100, pct));
  return circumference * (1 - p / 100);
}
function zoneDistBar(zoneSec){
  const total = zoneSec.reduce((a,b)=>a+b,0) || 1;
  const colors = ['#60a5fa','#4ade80','#facc15','#fb923c','#ef4444'];
  return zoneSec.map((v,i) => `<div style="width:${(v/total*100).toFixed(1)}%;background:${colors[i]}"></div>`).join('');
}

// ---------------- Takim Ozet Seridi (2026-08 ekleme) ----------------
// Kart gorunumlerinden (Ring/Monitor/Trend/vb.) BAGIMSIZ, tek bakista "kac
// oyuncu KRITIK/UYARI'da" sayisini veren kompakt bir serit - koc tek tek
// karti taramadan (ozellikle 9 oyuncuya kadar cikan bir takimda) durumu aninda
// gorsun diye. Saf frontend: /data'dan zaten gelen riskStatus'lari sayar,
// yeni bir backend hesaplamasi gerekmez.
function renderTeamRiskSummary(rows){
  const el = document.getElementById('teamRiskSummary');
  if (!el) return;
  if (rows.length === 0) { el.style.display = 'none'; return; }

  let kritik = 0, uyari = 0, normal = 0;
  rows.forEach(s => {
    if (s.riskStatus === 'KRITIK') kritik++;
    else if (s.riskStatus === 'UYARI') uyari++;
    else normal++;
  });

  el.style.display = 'flex';
  el.innerHTML = `
    <div style="flex:1;text-align:center;padding:8px;border-radius:10px;background:${kritik > 0 ? '#ef4444' : 'var(--card2)'}">
      <div style="font-size:20px;font-weight:900;color:${kritik > 0 ? '#020617' : 'var(--text)'}">${kritik}</div>
      <div style="font-size:9px;font-weight:800;color:${kritik > 0 ? '#020617cc' : 'var(--muted)'}">KRITIK</div>
    </div>
    <div style="flex:1;text-align:center;padding:8px;border-radius:10px;background:${uyari > 0 ? '#facc15' : 'var(--card2)'}">
      <div style="font-size:20px;font-weight:900;color:${uyari > 0 ? '#020617' : 'var(--text)'}">${uyari}</div>
      <div style="font-size:9px;font-weight:800;color:${uyari > 0 ? '#020617cc' : 'var(--muted)'}">UYARI</div>
    </div>
    <div style="flex:1;text-align:center;padding:8px;border-radius:10px;background:var(--card2)">
      <div style="font-size:20px;font-weight:900;color:var(--text)">${normal}</div>
      <div style="font-size:9px;font-weight:800;color:var(--muted)">NORMAL</div>
    </div>`;
}

// Her enabled+atanmis slotu bir "oyuncu karti" olarak ozetleyen takim panosu -
// koc tum oyunculari tek bakista gorur, tek tek kart acmasi gerekmez.
function renderTeamDashboard(slots){
  lastSlotsGlobal = slots;
  const grid = document.getElementById('teamDashGrid');
  const rows = slots.filter(s => s.enabled && s.playerId !== 0);

  rows.forEach(s => pushBpmHistory(s.slot, s.bpm, s.fresh));
  checkSyncIntensity(slots);
  renderHalftimeResults();
  renderTeamRiskSummary(rows);

  if (rows.length === 0) {
    grid.innerHTML = '<div class="team-card" id="teamDashEmpty" style="grid-column:1/-1"><div class="tc-name" style="color:var(--muted)">Henuz oyuncu atanmadi</div></div>';
    closeFocusMode();
    return;
  }

  // En yuksek riskli oyuncu en ustte - koc antrenman basinda/sirasinda ilk
  // bakista kimin dikkat istedigini gorsun (KRITIK > UYARI > NORMAL, esit
  // durumda yorgunluk skoruna gore).
  const riskOrder = { 'KRITIK': 0, 'UYARI': 1, 'NORMAL': 2 };
  const sorted = [...rows].sort((a, b) => {
    const ra = riskOrder[a.riskStatus] ?? 3;
    const rb = riskOrder[b.riskStatus] ?? 3;
    if (ra !== rb) return ra - rb;
    return b.fatigue - a.fatigue;
  });

  if (dashViewMode === 'monitor') grid.innerHTML = sorted.map(renderCardMonitor).join('');
  else if (dashViewMode === 'trend') grid.innerHTML = sorted.map(renderCardTrend).join('');
  else if (dashViewMode === 'focus') { grid.innerHTML = sorted.map(renderCardFocusEntry).join(''); updateFocusOverlayIfOpen(); }
  else if (dashViewMode === 'table') grid.innerHTML = renderTeamDetailTable(sorted);
  else if (dashViewMode === 'heat') grid.innerHTML = sorted.map(renderCardHeat).join('');
  else grid.innerHTML = sorted.map(renderCardRing).join('');
}

// A) HALKA (RING) - Apple Watch tarzi ic-ice halkalar: dis = anlik nabiz
// bolgesi doluluk (%HRmax), ic = ACWR doluluk (2.0 ACWR'de tam dolu sayilir -
// zaten 1.5 uzeri "yuksek risk" bandinda, 2.0 gorsel tavan olarak makul).
function renderCardRing(s){
  const bpmTxt = s.fresh ? s.bpm : '--';
  const zoneIdx = s.fresh ? s.zone : 0;
  const zoneColor = HR_ZONE_COLORS[zoneIdx] || '#8aa08f';
  const outerOff = gaugeOffset(264, s.fresh ? s.pctMax : 0);
  const acwrPct = s.acwrDays >= 3 ? (s.acwr / 2.0) * 100 : 0;
  const innerOff = gaugeOffset(188, acwrPct);
  return `
    <div class="team-card" onclick="if(dashViewMode==='focus') openFocusMode(${s.slot})" style="text-align:center">
      <svg viewBox="0 0 100 100" style="width:78px;height:78px">
        <circle cx="50" cy="50" r="42" fill="none" stroke="#1c2c21" stroke-width="7"/>
        <circle cx="50" cy="50" r="42" fill="none" stroke="${zoneColor}" stroke-width="7" stroke-linecap="round"
          stroke-dasharray="264" stroke-dashoffset="${outerOff}" transform="rotate(-90 50 50)"/>
        <circle cx="50" cy="50" r="30" fill="none" stroke="#1c2c21" stroke-width="6"/>
        <circle cx="50" cy="50" r="30" fill="none" stroke="#facc15" stroke-width="6" stroke-linecap="round"
          stroke-dasharray="188" stroke-dashoffset="${innerOff}" transform="rotate(-90 50 50)"/>
        <text x="50" y="46" text-anchor="middle" font-size="18" font-weight="900" fill="#f3f7f1">${bpmTxt}</text>
        <text x="50" y="60" text-anchor="middle" font-size="9" font-weight="700" fill="#8aa08f">bpm</text>
      </svg>
      <div class="tc-name" style="margin-top:4px">${s.playerName}</div>
      <div style="font-size:9px;color:#8aa08f;margin-top:2px">Dis:bolge &middot; Ic:ACWR ${s.acwrDays>=3?s.acwr.toFixed(2):'--'}</div>
    </div>`;
}

// B) MONITOR - koyu, izgarali "hasta basi cihazi" hissi. Cizgi GERCEK BPM
// TRENDI'dir (uydurma EKG sekli DEGIL) - her kartin KENDI son ~40 saniyelik
// min/max araligina gore olceklenir (o oyuncunun kendi degiskenligi
// vurgulanir).
function renderCardMonitor(s){
  const hist = bpmHistory[s.slot] || [];
  const vals = hist.filter(v => v !== null);
  const minV = vals.length ? Math.min(...vals) - 5 : 60;
  const maxV = vals.length ? Math.max(...vals) + 5 : 180;
  const pts = buildSparklinePoints(hist, 200, 50, minV, maxV);
  const bpmTxt = s.fresh ? s.bpm : '--';
  const zoneIdx = s.fresh ? s.zone : 0;
  const zoneColor = HR_ZONE_COLORS[zoneIdx] || '#8aa08f';
  return `
    <div class="team-card" onclick="if(dashViewMode==='focus') openFocusMode(${s.slot})">
      <div class="tc-name">${s.playerName}</div>
      <div class="tc-bpm" style="color:${zoneColor}">${bpmTxt} <span class="unit" style="font-size:10px;color:#8aa08f">bpm</span></div>
      <svg viewBox="0 0 200 50" style="width:100%;height:44px;margin-top:4px;background:#020617;border-radius:8px;display:block">
        <line x1="0" y1="12" x2="200" y2="12" stroke="#1c2c21" stroke-width="1"/>
        <line x1="0" y1="25" x2="200" y2="25" stroke="#1c2c21" stroke-width="1"/>
        <line x1="0" y1="38" x2="200" y2="38" stroke="#1c2c21" stroke-width="1"/>
        ${pts ? `<polyline points="${pts}" fill="none" stroke="${zoneColor}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>` : ''}
      </svg>
      <div class="tc-chips">
        <span class="legend-chip" style="background:${s.riskColor};color:#020617">Y:${s.fatigue}</span>
        <span class="legend-chip">A:${s.acwrDays>=3?s.acwr.toFixed(2):'--'}</span>
      </div>
    </div>`;
}

// C) TREND + BOLGE - sparkline SABIT 60-200bpm eksende (kartlar arasi
// karsilastirilabilir olsun diye, Monitor'un aksine), arka planda Z1-Z5
// bant renkleri + alta o oturumun bolge dagilim cubugu.
function renderCardTrend(s){
  const hist = bpmHistory[s.slot] || [];
  const pts = buildSparklinePoints(hist, 200, 40, 60, 200);
  const bpmTxt = s.fresh ? s.bpm : '--';
  const zoneIdx = s.fresh ? s.zone : 0;
  const zoneLabel = HR_ZONE_LABELS[zoneIdx] || '--';
  const pctTxt = s.fresh && s.pctMax > 0 ? s.pctMax.toFixed(0) + '%' : '--';
  return `
    <div class="team-card" onclick="if(dashViewMode==='focus') openFocusMode(${s.slot})">
      <div class="tc-name">${s.playerName}</div>
      <div style="display:flex;justify-content:space-between;align-items:baseline;margin-top:2px">
        <div class="tc-bpm">${bpmTxt}<span class="unit" style="font-size:10px;color:#8aa08f"> bpm</span></div>
        <span class="legend-chip" style="background:${s.riskColor};color:#020617">${zoneLabel} ${pctTxt}</span>
      </div>
      <svg viewBox="0 0 200 40" style="width:100%;height:36px;margin-top:4px;border-radius:8px;display:block">
        <rect x="0" y="0" width="200" height="6.4" fill="#ef4444" opacity=".22"/>
        <rect x="0" y="6.4" width="200" height="5.6" fill="#fb923c" opacity=".22"/>
        <rect x="0" y="12" width="200" height="6.4" fill="#facc15" opacity=".22"/>
        <rect x="0" y="18.4" width="200" height="7.2" fill="#4ade80" opacity=".22"/>
        <rect x="0" y="25.6" width="200" height="6.4" fill="#60a5fa" opacity=".22"/>
        ${pts ? `<polyline points="${pts}" fill="none" stroke="#f3f7f1" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>` : ''}
      </svg>
      <div style="display:flex;height:6px;border-radius:999px;overflow:hidden;margin-top:6px">${zoneDistBar(s.zoneSec)}</div>
    </div>`;
}

// D) ODAK MODU - takim panosunda sade bir giris karti, tiklaninca tam ekran
// tek-oyuncu detay (buyuk gauge + ikincil gauge'lar + HRV + bolge dagilimi)
// acilir. Detay her loadData() turunda (bkz. updateFocusOverlayIfOpen)
// canli guncellenir, kapatilana kadar acik kalir.
function renderCardFocusEntry(s){
  const bpmTxt = s.fresh ? s.bpm : '--';
  return `
    <div class="team-card" onclick="openFocusMode(${s.slot})" style="cursor:pointer;text-align:center">
      <div class="tc-name">${s.playerName}</div>
      <div class="tc-bpm">${bpmTxt}<span class="unit" style="font-size:10px;color:#8aa08f"> bpm</span></div>
      <span class="legend-chip" style="background:${s.riskColor};color:#020617;margin-top:6px;display:inline-block;padding:2px 8px;border-radius:999px">${s.riskStatus}</span>
    </div>`;
}
// E) DETAYLI TABLO - TUM oyuncular TUM metrikleriyle (nabiz/bolge/yorgunluk/
// ACWR/monotonluk/HRV/HRR/Wellness) TEK tabloda yan yana - Polar Team
// Pro'nun "Whole Team" gorunumune en yakin mod, koc tek tek karta girmeden
// butun takimi karsilastirir. Dar telefon ekraninda yatay kaydirilir (bkz.
// .detail-table-wrap), satira dokununca o oyuncunun Odak Modu'nu acar.
function renderTeamDetailTable(sorted){
  const rows = sorted.map(s => {
    const bpmTxt = s.fresh ? s.bpm : '--';
    const pctTxt = s.fresh && s.pctMax > 0 ? s.pctMax.toFixed(0) + '%' : '--';
    const zoneIdx = s.fresh ? s.zone : 0;
    const zoneTxt = s.fresh ? (HR_ZONE_LABELS[s.zone] || '--') : '--';
    const acwrTxt = s.acwrDays >= 3 ? s.acwr.toFixed(2) : '--';
    const monoTxt = s.acwrDays >= 3 ? s.monotony.toFixed(2) : '--';
    const hrvTxt = s.rrSupported ? (s.rmssd.toFixed(0) + '/' + s.sdnn.toFixed(0) + '/' + s.pnn50.toFixed(0) + '%') : '--';
    const hrrTxt = (s.hrr1 >= 0 || s.hrr2 >= 0)
      ? ((s.hrr1 >= 0 ? s.hrr1 + 'bpm' : '--') + ' / ' + (s.hrr2 >= 0 ? s.hrr2 + 'bpm' : '--'))
      : '--';
    const wellTxt = s.wellnessHasData ? (s.wellnessSum + '/50 (' + s.wellnessBand + ')') : '--';
    const breathTxt = (s.rrSupported && s.fresh && s.breathingRate > 0) ? (s.breathingRate.toFixed(0) + '/dk') : '--';
    const orthoTxt = s.orthoHr2 >= 0
      ? (s.orthoHr1.toFixed(0) + '->' + s.orthoHr2.toFixed(0) + 'bpm')
      : (s.orthoActive ? ('Faz' + s.orthoPhase + '...') : '--');
    // HRV Taban Cizgisi (2026-08 ekleme) - son oturumun RMSSD'sinin oyuncunun
    // KENDI tipik seviyesinden sapmasi, bkz. WebRoutes.cpp/RosterStore.h notu.
    const hrvBaseTxt = s.hrvBaselineReady
      ? (s.hrvBaselineRmssd.toFixed(0) + 'ms (' + (s.hrvDeviationPct >= 0 ? '+' : '') + s.hrvDeviationPct.toFixed(0) + '%)')
      : '--';
    const readyTxt = s.readinessReady ? (s.readinessScore + ' ' + s.readinessBand) : '--';
    return `<tr onclick="openFocusMode(${s.slot})">
      <td class="dt-name">${s.playerName}</td>
      <td><b>${bpmTxt}</b> <span style="color:var(--muted)">bpm</span></td>
      <td>${pctTxt}</td>
      <td><span class="zone-badge" style="margin-top:0;background:${HR_ZONE_COLORS[zoneIdx] || 'var(--muted)'}">${zoneTxt}</span></td>
      <td><span class="legend-chip" style="display:inline-flex;background:${s.riskColor};color:#020617;padding:2px 8px;border-radius:999px;font-size:10px">${s.riskStatus} ${s.fatigue}</span></td>
      <td>${acwrTxt} <span style="color:var(--muted)">(${s.acwrBand})</span></td>
      <td>${monoTxt}</td>
      <td>${hrvTxt}</td>
      <td>${hrrTxt}</td>
      <td>${wellTxt}</td>
      <td>${hrvBaseTxt}</td>
      <td><span class="legend-chip" style="display:inline-flex;background:${s.readinessReady ? s.readinessColor : 'var(--muted)'};color:#020617;padding:2px 8px;border-radius:999px;font-size:10px">${readyTxt}</span></td>
      <td>${breathTxt}</td>
      <td>${orthoTxt}</td>
    </tr>`;
  }).join('');

  return `<div class="detail-table-wrap"><table class="detail-table">
    <thead><tr>
      <th>Oyuncu</th><th>Nabiz</th><th>%HRmax</th><th>Bolge</th><th>Yorgunluk</th>
      <th>ACWR</th><th>Monoton.</th><th>HRV (RMSSD/SDNN/pNN50)</th><th>HRR (1dk/2dk)</th><th>Wellness</th>
      <th>HRV Taban (sapma)</th><th>Hazir Olma</th><th>Solunum</th><th>Ortostatik (F1-&gt;F2)</th>
    </tr></thead>
    <tbody>${rows}</tbody>
  </table></div>`;
}

// F) ISI KARTLARI (Polar Team Pro "Whole Team" ekranina benzer) - her oyuncu
// duz renkli, buyuk %HRmax rakamli bir kutu; renk kutunun ARKA PLANI olarak
// dolduruluyor (Ring/Monitor'daki gibi ince bir gosterge cizgisi degil).
// Amac: en HIZLI tarabilir gorunum - koc uzaktan bakinca hangi kutularin
// "sicak" (kirmizi/turuncu) oldugunu aninda gorsun. Renk skalasi Polar'in
// kendi (bilinmeyen) algoritmasi DEGIL, sistemin zaten her yerde kullandigi
// ayni HR_ZONE_COLORS/bolge esikleri - boylece bu gorunum diger 5 modla
// TUTARLI kalir, sadece sunumu farkli.
function renderCardHeat(s){
  const zoneIdx = s.fresh ? s.zone : 0;
  const zoneColor = HR_ZONE_COLORS[zoneIdx] || 'var(--card2)';
  const pctTxt = s.fresh && s.pctMax > 0 ? s.pctMax.toFixed(0) : '--';
  const bpmTxt = s.fresh ? s.bpm : '--';
  return `
    <div class="team-card heat-card" style="background:${zoneColor}" onclick="if(dashViewMode==='focus') openFocusMode(${s.slot})">
      <div class="hc-name">${s.playerName}</div>
      <div class="hc-pct">${pctTxt}<span class="hc-unit">%</span></div>
      <div class="hc-sub">${bpmTxt} bpm &middot; ${s.riskStatus}</div>
    </div>`;
}

function miniGauge(label, valueTxt, pct, color){
  const off = gaugeOffset(226, pct);
  return `
    <div style="text-align:center">
      <svg viewBox="0 0 90 90" style="width:72px;height:72px">
        <circle cx="45" cy="45" r="36" fill="none" stroke="#1c2c21" stroke-width="9"/>
        <circle cx="45" cy="45" r="36" fill="none" stroke="${color}" stroke-width="9" stroke-linecap="round"
          stroke-dasharray="226" stroke-dashoffset="${off}" transform="rotate(-90 45 45)"/>
        <text x="45" y="50" text-anchor="middle" font-size="13" font-weight="900" fill="#f3f7f1">${valueTxt}</text>
      </svg>
      <div style="font-size:9px;color:#8aa08f;font-weight:800;margin-top:2px">${label}</div>
    </div>`;
}
function openFocusMode(slot){
  openFocusSlot = slot;
  document.getElementById('focusOverlay').style.display = 'block';
  renderFocusOverlay();
}

// Oyuncu bazli cok-oturumlu trend (2026-08 ekleme) - bkz. focusTrendCache notu.
function loadFocusTrend(playerId){
  focusTrendCache = { playerId, entries: [] };  // hemen isaretle: ayni playerId icin tekrar fetch tetiklenmesin
  fetch('/playertrend?id=' + playerId)
    .then(r => r.json())
    .then(data => {
      focusTrendCache = { playerId, entries: Array.isArray(data) ? data : [] };
      updateFocusOverlayIfOpen();
    })
    .catch(e => {});
}

function renderFocusTrendSection(playerId){
  const entries = (focusTrendCache.playerId === playerId) ? focusTrendCache.entries : [];
  const boxStyle = 'margin-top:14px;background:linear-gradient(180deg,#0e1811,#0b120d);border:1px solid #1c2c21;border-radius:16px;padding:14px';

  if (entries.length < 2) {
    return `<div style="${boxStyle}">
      <div style="font-size:11px;color:#8aa08f;font-weight:700">SON OTURUMLAR TRENDI</div>
      <div style="font-size:12px;color:#8aa08f;margin-top:6px">Henuz yeterli oturum gecmisi yok (trend icin en az 2 tamamlanmis oturum gerekir)</div>
    </div>`;
  }

  const fatigueVals = entries.map(e => e.fatigue);
  const fatiguePts = buildSparklinePoints(fatigueVals, 200, 40, 0, 100);
  const lastFatigue = fatigueVals[fatigueVals.length - 1];

  const hrvVals = entries.map(e => e.hrvDeviationPct);
  const hasHrvTrend = hrvVals.some(v => v !== 0);
  const hrvPts = buildSparklinePoints(hrvVals, 200, 40, -50, 50);
  const lastHrv = hrvVals[hrvVals.length - 1];

  return `<div style="${boxStyle}">
    <div style="font-size:11px;color:#8aa08f;font-weight:700">SON ${entries.length} OTURUM &middot; YORGUNLUK TRENDI</div>
    <svg viewBox="0 0 200 40" style="width:100%;height:36px;margin-top:6px;background:#020617;border-radius:8px;display:block">
      <polyline points="${fatiguePts}" fill="none" stroke="#facc15" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>
    </svg>
    <div style="font-size:10px;color:#8aa08f;margin-top:4px">Son oturum: ${lastFatigue}/100</div>
    ${hasHrvTrend ? `
    <div style="font-size:11px;color:#8aa08f;font-weight:700;margin-top:12px">HRV TABAN SAPMASI TRENDI</div>
    <svg viewBox="0 0 200 40" style="width:100%;height:36px;margin-top:6px;background:#020617;border-radius:8px;display:block">
      <line x1="0" y1="20" x2="200" y2="20" stroke="#1c2c21" stroke-width="1"/>
      <polyline points="${hrvPts}" fill="none" stroke="#60a5fa" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>
    </svg>
    <div style="font-size:10px;color:#8aa08f;margin-top:4px">Son oturum: ${lastHrv >= 0 ? '+' : ''}${lastHrv.toFixed(0)}%</div>` : ''}
  </div>`;
}
function closeFocusMode(){
  openFocusSlot = null;
  document.getElementById('focusOverlay').style.display = 'none';
}
function updateFocusOverlayIfOpen(){
  if (openFocusSlot !== null) renderFocusOverlay();
}
function renderFocusOverlay(){
  if (openFocusSlot === null || !lastSlotsGlobal) return;
  const s = lastSlotsGlobal.find(x => x.slot === openFocusSlot);
  const overlay = document.getElementById('focusOverlay');
  if (!s || !s.enabled || s.playerId === 0) { closeFocusMode(); return; }

  const bpmTxt = s.fresh ? s.bpm : '--';
  const pctTxt = s.fresh && s.pctMax > 0 ? s.pctMax.toFixed(0) : 0;
  const zoneIdx = s.fresh ? s.zone : 0;
  const zoneLabel = HR_ZONE_LABELS[zoneIdx] || '--';
  const outerOff = gaugeOffset(578, s.fresh ? s.pctMax : 0);
  const acwrPct = s.acwrDays >= 3 ? (s.acwr / 2.0) * 100 : 0;
  const wellPct = s.wellnessHasData ? Math.max(0, 100 - ((s.wellnessSum - 5) / 45 * 100)) : 0;
  const hrrPct = s.hrr1 >= 0 ? (s.hrr1 / 40) * 100 : 0;
  const hrvTxt = s.rrSupported ? (s.rmssd.toFixed(0) + 'ms &middot; ' + s.sdnn.toFixed(0) + 'ms &middot; %' + s.pnn50.toFixed(0)) : 'Desteklenmiyor';
  const breathTxt = (s.rrSupported && s.fresh && s.breathingRate > 0) ? (s.breathingRate.toFixed(0) + ' nefes/dk (tahmini)') : '--';
  const orthoTxt = s.orthoHr2 >= 0
    ? ('Faz1 ' + s.orthoHr1.toFixed(0) + ' bpm &rarr; Faz2 ' + s.orthoHr2.toFixed(0) + ' bpm')
    : (s.orthoActive ? ('Faz' + s.orthoPhase + ' suruyor (' + s.orthoElapsedSec + 'sn)') : 'Test baslatilmadi');

  if (focusTrendCache.playerId !== s.playerId) loadFocusTrend(s.playerId);
  const trendSectionHtml = renderFocusTrendSection(s.playerId);
  // HRV Taban Cizgisi + Composite Hazir Olma Skoru (2026-08 eklemeleri) - bkz.
  // WebRoutes.cpp/PlayerMath.h notu, ikisi de antrenman ONCESI/gecmis oturum
  // verisinden hesaplanir, "su an"ki nabizdan bagimsizdir.
  const hrvBaseTxt = s.hrvBaselineReady
    ? ('Taban: ' + s.hrvBaselineRmssd.toFixed(0) + 'ms &middot; son oturum sapmasi: ' + (s.hrvDeviationPct >= 0 ? '+' : '') + s.hrvDeviationPct.toFixed(0) + '%')
    : 'Taban henuz olusmadi (en az birkac oturum gerekir)';
  const readinessTxt = s.readinessReady ? (s.readinessScore + ' &middot; ' + s.readinessBand) : 'Yetersiz veri';
  const readinessColor = s.readinessReady ? s.readinessColor : '#8aa08f';

  overlay.innerHTML = `
    <div style="max-width:420px;margin:0 auto;color:#f3f7f1">
      <div style="display:flex;align-items:center;justify-content:space-between">
        <div>
          <div style="font-size:11px;font-weight:800;letter-spacing:.08em;color:#8aa08f;text-transform:uppercase">Odak Modu - ${s.bandLabel}</div>
          <div style="font-size:22px;font-weight:900;margin-top:2px">${s.playerName}</div>
          <div style="margin-top:4px"><span class="legend-chip" style="display:inline-flex;background:${readinessColor};color:#020617;padding:2px 8px;border-radius:999px;font-size:10px;font-weight:800">HAZIR OLMA: ${readinessTxt}</span></div>
        </div>
        <div onclick="closeFocusMode()" style="width:34px;height:34px;border-radius:50%;border:1px solid #1c2c21;display:flex;align-items:center;justify-content:center;color:#8aa08f;font-size:16px;cursor:pointer">&#10005;</div>
      </div>
      <div style="display:flex;justify-content:center;margin-top:14px">
        <svg viewBox="0 0 220 220" style="width:200px;height:200px">
          <circle cx="110" cy="110" r="92" fill="none" stroke="#1c2c21" stroke-width="16"/>
          <circle cx="110" cy="110" r="92" fill="none" stroke="${s.riskColor}" stroke-width="16" stroke-linecap="round"
            stroke-dasharray="578" stroke-dashoffset="${outerOff}" transform="rotate(-90 110 110)"/>
          <text x="110" y="100" text-anchor="middle" font-size="42" font-weight="900" fill="#f3f7f1">${bpmTxt}</text>
          <text x="110" y="126" text-anchor="middle" font-size="12" font-weight="700" fill="#8aa08f">bpm - %${pctTxt} HRmax</text>
          <text x="110" y="150" text-anchor="middle" font-size="13" font-weight="900" fill="${s.riskColor}">${zoneLabel}</text>
        </svg>
      </div>
      <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:4px">
        ${miniGauge('ACWR', s.acwrDays>=3?s.acwr.toFixed(2):'--', acwrPct, '#facc15')}
        ${miniGauge('WELLNESS', s.wellnessHasData?(s.wellnessSum+'/50'):'--', wellPct, '#4ade80')}
        ${miniGauge('HRR 1dk', s.hrr1>=0?(s.hrr1+' bpm'):'--', hrrPct, '#60a5fa')}
      </div>
      <div style="margin-top:14px;background:linear-gradient(180deg,#0e1811,#0b120d);border:1px solid #1c2c21;border-radius:16px;padding:14px">
        <div style="font-size:11px;color:#8aa08f;font-weight:700">HRV (RMSSD &middot; SDNN &middot; pNN50)</div>
        <div style="font-size:15px;font-weight:900;margin-top:4px">${hrvTxt}</div>
        <div style="font-size:11px;color:#8aa08f;margin-top:10px">${hrvBaseTxt}</div>
        <div style="font-size:11px;color:#8aa08f;margin-top:6px">Solunum (tahmini): ${breathTxt}</div>
        <div style="font-size:11px;color:#8aa08f;margin-top:6px">Ortostatik test: ${orthoTxt}</div>
        <div style="display:flex;height:12px;border-radius:999px;overflow:hidden;margin-top:12px">${zoneDistBar(s.zoneSec)}</div>
        <div style="font-size:10px;color:#8aa08f;margin-top:6px">Bu oturum bolge dagilimi</div>
      </div>
      ${trendSectionHtml}
    </div>`;
}

// ---------------- Takim Senkron Yogunluk Tespiti ----------------
// Bizim sistemin tek-kisilik bir Polar saatinin gosteremeyecegi bir sey:
// AYNI ANDA 3+ oyuncu yuksek bolgeye (Z4/Z5) girdiyse, bu muhtemelen mac/
// antrenmanda kritik bir an (pres, gecis, yuksek tempolu faz) - GPS/event
// etiketleme olmadan bile SENKRONIZE nabiz artisindan yakalanabilir.
const SYNC_INTENSITY_MIN_ZONE = 4;
const SYNC_INTENSITY_MIN_PLAYERS = 3;
function checkSyncIntensity(slots){
  const banner = document.getElementById('syncIntensityBanner');
  const namesEl = document.getElementById('syncIntensityNames');
  const highIntensity = slots.filter(s => s.enabled && s.playerId !== 0 && s.fresh && s.zone >= SYNC_INTENSITY_MIN_ZONE);
  if (highIntensity.length >= SYNC_INTENSITY_MIN_PLAYERS) {
    banner.style.display = 'block';
    namesEl.textContent = highIntensity.map(s => s.playerName + ' (' + HR_ZONE_LABELS[s.zone] + ')').join(', ');
  } else {
    banner.style.display = 'none';
  }
}

// ---------------- Devre Arasi Toparlanma Modu ----------------
// Mevcut HRR testini (bkz. startHrrTestNow) TUM atanmis oyunculara AYNI ANDA
// uygulayan bir kisayol - yarı zamanda tek tusla tum takimin toparlanmasini
// olcup, mola sonunda karsilastirmali bir liste gorur.
function startHalftimeRecoveryForAll(){
  if (!lastSlotsGlobal) return;
  const assigned = lastSlotsGlobal.filter(s => s.enabled && s.playerId !== 0);
  if (assigned.length === 0) { alert('Once en az bir oyuncu atayin'); return; }
  if (!confirm(assigned.length + ' oyuncu icin devre arasi toparlanma testi baslatilsin mi?')) return;

  assigned.forEach(s => {
    fetch('/starthrrtest?slot=' + s.slot, { method: 'POST', cache: 'no-store' }).catch(e=>{});
  });
  setTimeout(loadData, 300);
}
function renderHalftimeResults(){
  const el = document.getElementById('halftimeResults');
  if (!el || !lastSlotsGlobal) return;

  const rows = lastSlotsGlobal.filter(s => s.enabled && s.playerId !== 0 && (s.hrrActive || s.hrr1 >= 0));
  if (rows.length === 0) {
    el.innerHTML = '<div style="font-size:12px;color:#8aa08f">Henuz test baslatilmadi</div>';
    return;
  }

  // Iyi toparlanan (dususu buyuk olan) en ustte.
  const sorted = [...rows].sort((a, b) => (b.hrr1 >= 0 ? b.hrr1 : -999) - (a.hrr1 >= 0 ? a.hrr1 : -999));
  el.innerHTML = sorted.map(s => {
    const status = s.hrrActive ? ('suruyor - ' + s.hrrElapsedSec + 'sn') : 'tamamlandi';
    const hrr1Txt = s.hrr1 >= 0 ? (s.hrr1 + ' bpm') : '--';
    const hrr2Txt = s.hrr2 >= 0 ? (s.hrr2 + ' bpm') : '--';
    return `<div style="display:flex;justify-content:space-between;align-items:baseline;padding:6px 0;border-top:1px solid #1c2c21;font-size:12px">
      <span style="font-weight:700">${s.playerName}</span>
      <span style="color:#8aa08f;text-align:right">1dk: <b style="color:#f3f7f1">${hrr1Txt}</b> &middot; 2dk: <b style="color:#f3f7f1">${hrr2Txt}</b><br><span style="font-size:10px">${status}</span></span>
    </div>`;
  }).join('');
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
 document.getElementById('dashViewMode').value = dashViewMode;
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
 // Mac Gunu vs Antrenman ayrimi (bkz. Config.h MATCH_LOAD_MULTIPLIER notu) -
 // ACWR'nin gunluk yukunu etkiler, kisisel rekor/toplam yuku ETKILEMEZ.
 let isMatch = confirm('Bu seans bir MAC miydi?\n\nTamam = Mac (ACWR yuku agirliklandirilir)\nIptal = Antrenman (normal)');
 let sessionType = isMatch ? 'match' : 'training';

 let rpeInput = prompt('Antrenmanin zorluk algisi (RPE) 1-10 arasi girin (bos gecebilirsiniz):', '');
 let rpe = parseInt(rpeInput);
 if(isNaN(rpe) || rpe < 1 || rpe > 10) rpe = 0;

 if(confirm((isMatch ? 'Mac' : 'Antrenman') + ' verileri sifirlansin mi?')){
   fetch('/reset?ts=' + Math.floor(Date.now()/1000) + '&rpe=' + rpe + '&type=' + sessionType, { cache: 'no-store' })
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

  <div class="section-title">Takim Risk Panosu</div>
  <div class="card big">
    <div class="label">Gorunum</div>
    <select id="dashViewMode" onchange="onDashViewModeChange()" style="width:100%;padding:10px;margin-top:8px;border-radius:10px;border:1px solid var(--border);background:var(--card2);color:var(--text)">
      <option value="ring">Halka (Ring)</option>
      <option value="monitor">Monitor (canli dalga)</option>
      <option value="trend">Trend + Bolge</option>
      <option value="focus">Odak Modu (karta dokun)</option>
      <option value="table">Detayli Tablo (tum metrikler)</option>
      <option value="heat">Isi Kartlari (Polar tarzi)</option>
    </select>
  </div>

  <!-- Takim Ozet Seridi - bkz. renderTeamRiskSummary(). Kac oyuncu KRITIK/UYARI/
       NORMAL'de, kart gorunumunden bagimsiz tek bakista sayi. -->
  <div id="teamRiskSummary" style="display:none;gap:8px;margin-bottom:10px"></div>

  <!-- Takim Senkron Yogunluk banner'i - bkz. checkSyncIntensity(). Sadece 3+
       oyuncu AYNI ANDA yuksek bolgedeyken gorunur. -->
  <div id="syncIntensityBanner" style="display:none;background:linear-gradient(135deg,#ef4444,#fb923c);border-radius:16px;padding:12px 14px;margin-bottom:10px">
    <div style="font-size:13px;font-weight:900;color:#020617">TAKIM SENKRON YOGUNLUK ANI</div>
    <div id="syncIntensityNames" style="font-size:11px;font-weight:700;color:#020617cc;margin-top:2px"></div>
  </div>

  <div class="team-grid" id="teamDashGrid">
    <div class="team-card" id="teamDashEmpty" style="grid-column:1/-1">
      <div class="tc-name" style="color:var(--muted)">Henuz oyuncu atanmadi</div>
    </div>
  </div>

  <!-- "Odak Modu" gorunumunde bir karta dokununca tam ekran acilir - bkz.
       openFocusMode()/closeFocusMode(). Diger gorunumlerde hep gizli kalir. -->
  <div id="focusOverlay" style="display:none;position:fixed;inset:0;z-index:200;background:var(--bg);overflow-y:auto;padding:20px"></div>

  <div class="section-title">Devre Arasi Toparlanma</div>
  <div class="card big">
    <div class="label">Tum takimin Kalp Hizi Toparlanma testini ayni anda baslatir (bkz. HRR - her oyuncu kartinda da tek tek var)</div>
    <button class="btn" style="margin-top:8px" onclick="startHalftimeRecoveryForAll()">Devre Arasi Testini Baslat (Tum Takim)</button>
    <div id="halftimeResults" style="margin-top:10px">
      <div style="font-size:12px;color:var(--muted)">Henuz test baslatilmadi</div>
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
