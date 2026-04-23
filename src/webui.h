#pragma once
#include <Arduino.h>

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/webui.h — Web App HTML nhúng vào firmware              ║
// ║  Lưu trong PROGMEM để tiết kiệm RAM                        ║
// ╚══════════════════════════════════════════════════════════════╝

const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0">
<title>EzRover Pro</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#0d1117;color:#c9d1d9;font-family:'Courier New',monospace;font-size:13px}
header{background:#161b22;border-bottom:1px solid #30363d;padding:10px 16px;display:flex;align-items:center;justify-content:space-between}
header h1{font-size:14px;letter-spacing:3px;color:#58a6ff}
.badge{font-size:10px;padding:3px 8px;border-radius:10px;background:#21262d;border:1px solid #30363d}
.badge.on{background:#0d2d0d;border-color:#3fb950;color:#3fb950}
.badge.off{background:#2d0d0d;border-color:#f85149;color:#f85149}
main{display:flex;flex-wrap:wrap;gap:8px;padding:10px}
.panel{background:#161b22;border:1px solid #30363d;border-radius:6px;padding:12px}
.panel-title{font-size:10px;letter-spacing:2px;color:#8b949e;text-transform:uppercase;margin-bottom:10px;border-bottom:1px solid #21262d;padding-bottom:6px}

/* DPad */
.dpad{display:grid;grid-template-columns:repeat(3,56px);grid-template-rows:repeat(3,48px);gap:5px;justify-content:center}
.dpad button{background:#21262d;border:1px solid #30363d;color:#c9d1d9;border-radius:5px;font-size:18px;cursor:pointer;display:flex;align-items:center;justify-content:center;touch-action:none}
.dpad button:active,.dpad button.pressed{background:#1f6feb;border-color:#388bfd}
#btn-stop{font-size:12px;letter-spacing:1px;color:#f85149}

/* Speed slider */
.slider-wrap{margin-top:8px}
.slider-wrap label{font-size:10px;color:#8b949e;display:flex;justify-content:space-between}
input[type=range]{width:100%;margin-top:4px;accent-color:#1f6feb;cursor:pointer}

/* Telemetry grid */
.tgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.titem label{font-size:9px;letter-spacing:1px;color:#8b949e;text-transform:uppercase;display:block}
.titem span{font-size:18px;font-weight:bold;color:#58a6ff}

/* Mode buttons */
.mode-wrap{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:8px}
.mode-btn{background:#21262d;border:1px solid #30363d;color:#c9d1d9;padding:7px 12px;
          border-radius:4px;cursor:pointer;font-family:inherit;font-size:11px;letter-spacing:1px}
.mode-btn.active{background:#0d2b0d;border-color:#3fb950;color:#3fb950}
.btn-util{background:#21262d;border:1px solid #30363d;color:#8b949e;padding:5px 10px;
          border-radius:4px;cursor:pointer;font-family:inherit;font-size:11px;width:100%;margin-top:5px}
.btn-util:hover{border-color:#58a6ff;color:#58a6ff}

/* Map */
#mapCanvas{width:100%;border-radius:4px;cursor:crosshair;display:block}
.map-hint{font-size:10px;color:#8b949e;margin-top:4px}

/* Radar */
#radarCanvas{display:block;margin:0 auto}

/* Servo */
.servo-wrap{margin-top:8px}
.servo-wrap label{font-size:10px;color:#8b949e;display:flex;justify-content:space-between}

/* Log */
#log{height:80px;overflow-y:auto;font-size:10px;color:#8b949e;background:#0d1117;border:1px solid #21262d;border-radius:4px;padding:6px;margin-top:8px}
</style>
</head>
<body>
<header>
  <h1>⚙ EZROVER PRO</h1>
  <span class="badge off" id="connBadge">OFFLINE</span>
</header>

<main>
  <!-- Điều khiển -->
  <div class="panel" style="min-width:220px;flex:1">
    <div class="panel-title">Điều khiển thủ công</div>
    <div class="dpad">
      <div></div>
      <button id="btn-fwd"  ontouchstart="d(1,1)"   onmousedown="d(1,1)"
              ontouchend="s()" onmouseup="s()">▲</button>
      <div></div>
      <button id="btn-lt"   ontouchstart="d(-1,1)"  onmousedown="d(-1,1)"
              ontouchend="s()" onmouseup="s()">◄</button>
      <button id="btn-stop" onclick="s()">STOP</button>
      <button id="btn-rt"   ontouchstart="d(1,-1)"  onmousedown="d(1,-1)"
              ontouchend="s()" onmouseup="s()">►</button>
      <div></div>
      <button id="btn-rev"  ontouchstart="d(-1,-1)" onmousedown="d(-1,-1)"
              ontouchend="s()" onmouseup="s()">▼</button>
      <div></div>
    </div>
    <div class="slider-wrap">
      <label>Tốc độ <span id="spdVal">200</span></label>
      <input type="range" id="spdSlider" min="100" max="240" value="200"
             oninput="document.getElementById('spdVal').textContent=this.value">
    </div>
    <div class="servo-wrap">
      <label>Servo siêu âm <span id="servoVal">90</span>°</label>
      <input type="range" id="servoSlider" min="0" max="180" value="90"
             oninput="onServo(this.value)">
    </div>
  </div>

  <!-- Telemetry -->
  <div class="panel" style="min-width:180px;flex:1">
    <div class="panel-title">Telemetry</div>
    <div class="tgrid">
      <div class="titem"><label>X (cm)</label><span id="tX">0</span></div>
      <div class="titem"><label>Y (cm)</label><span id="tY">0</span></div>
      <div class="titem"><label>Yaw °</label><span id="tYaw">0</span></div>
      <div class="titem"><label>Nhiệt độ</label><span id="tTemp">--</span></div>
      <div class="titem"><label>Spd-L cm/s</label><span id="tSL">0</span></div>
      <div class="titem"><label>Spd-R cm/s</label><span id="tSR">0</span></div>
    </div>
    <div id="log"></div>
  </div>

  <!-- Chế độ -->
  <div class="panel" style="min-width:180px;flex:1">
    <div class="panel-title">Chế độ</div>
    <div class="mode-wrap">
      <button class="mode-btn active" id="m-manual" onclick="setMode('manual')">MANUAL</button>
      <button class="mode-btn"        id="m-avoid"  onclick="setMode('avoid')">AVOID</button>
      <button class="mode-btn"        id="m-scan"   onclick="doScan()">SCAN</button>
    </div>
    <button class="btn-util" onclick="resetPose()">↺  Reset Pose & Map</button>
    <button class="btn-util" onclick="doScan()">⟳  Quét siêu âm ngay</button>

    <!-- Radar -->
    <div class="panel-title" style="margin-top:12px">Sonar Radar</div>
    <canvas id="radarCanvas" width="160" height="160"
            style="background:#0a0d12;border-radius:50%;border:1px solid #30363d"></canvas>
  </div>

  <!-- Bản đồ -->
  <div class="panel" style="flex:2;min-width:280px">
    <div class="panel-title">Occupancy Map — Click để đặt mục tiêu GoToGoal</div>
    <canvas id="mapCanvas" height="280"></canvas>
    <div class="map-hint">🟡 Robot  🟢 Goal  🔴 Vật cản  🔵 Đường đi</div>
  </div>
</main>

<script>
// ─── Config ────────────────────────────────────────────────────
const ROWS=20, COLS=20, CELL=10; // 10cm/ô
let grid   = Array.from({length:ROWS},()=>new Array(COLS).fill(0));
let rPose  = {x:0,y:0,yaw:0};
let goal   = null;
let path   = [];
let ws;

// ─── WebSocket ─────────────────────────────────────────────────
function connect(){
  ws = new WebSocket('ws://'+location.hostname+':81');
  ws.onopen  = ()=>{ setBadge(true);  log('✅ Kết nối thành công'); };
  ws.onclose = ()=>{ setBadge(false); log('❌ Mất kết nối, thử lại...'); setTimeout(connect,2000); };
  ws.onerror = ()=>{ setBadge(false); };
  ws.onmessage = e=>{
    try{
      const d=JSON.parse(e.data);
      if(d.type==='telemetry') onTelemetry(d);
      if(d.type==='scan')      onScan(d);
    }catch(ex){}
  };
}

function send(o){ if(ws&&ws.readyState===1) ws.send(JSON.stringify(o)); }
function setBadge(on){
  const b=document.getElementById('connBadge');
  b.textContent = on?'ONLINE':'OFFLINE';
  b.className   = 'badge '+(on?'on':'off');
}
function log(msg){
  const el=document.getElementById('log');
  el.textContent+=msg+'\n';
  el.scrollTop=el.scrollHeight;
}

// ─── Drive commands ────────────────────────────────────────────
function spd(){ return parseInt(document.getElementById('spdSlider').value); }
function d(l,r){ send({cmd:'drive',l:l*spd(),r:r*spd()}); }
function s(){ send({cmd:'stop'}); }
function setMode(m){
  send({cmd:'mode',mode:m});
  ['manual','avoid'].forEach(x=>document.getElementById('m-'+x).classList.remove('active'));
  const el=document.getElementById('m-'+m);
  if(el) el.classList.add('active');
}
function doScan(){ send({cmd:'mode',mode:'scan'}); }
function resetPose(){
  send({cmd:'reset_pose'});
  rPose={x:0,y:0,yaw:0};
  grid=Array.from({length:ROWS},()=>new Array(COLS).fill(0));
  path=[]; goal=null;
  drawMap(); log('↺ Đã reset pose và map');
}
function onServo(v){
  document.getElementById('servoVal').textContent=v;
  send({cmd:'servo',pan:parseInt(v)});
}

// ─── Telemetry ─────────────────────────────────────────────────
function onTelemetry(d){
  rPose={x:+d.x,y:+d.y,yaw:+d.yaw};
  document.getElementById('tX').textContent   = (+d.x).toFixed(1);
  document.getElementById('tY').textContent   = (+d.y).toFixed(1);
  document.getElementById('tYaw').textContent = (+d.yaw).toFixed(1);
  document.getElementById('tTemp').textContent= (+d.temp).toFixed(1)+'°C';
  document.getElementById('tSL').textContent  = (+d.spdL).toFixed(1);
  document.getElementById('tSR').textContent  = (+d.spdR).toFixed(1);
  if(d.event==='arrived'){ goal=null; log('🎯 Đã đến đích!'); }
  drawMap();
}

// ─── Scan → OccupancyGrid ──────────────────────────────────────
function onScan(d){
  const {angles,dists}=d;
  angles.forEach((deg,i)=>{
    const dist=dists[i];
    if(dist<250){
      const rad=(rPose.yaw+deg-90)*Math.PI/180;
      const ox=rPose.x+dist*Math.cos(rad);
      const oy=rPose.y+dist*Math.sin(rad);
      const gx=Math.floor(ox/CELL)+10;
      const gy=Math.floor(oy/CELL)+10;
      if(gx>=0&&gx<COLS&&gy>=0&&gy<ROWS) grid[gy][gx]=1;
    }
  });
  if(goal) runWavefront();
  drawMap();
  drawRadar(d);
}

// ─── Wavefront BFS ─────────────────────────────────────────────
function runWavefront(){
  const wave=Array.from({length:ROWS},()=>new Array(COLS).fill(-1));
  const {gx,gy}=goal;
  if(gx<0||gx>=COLS||gy<0||gy>=ROWS) return;
  wave[gy][gx]=2;
  const q=[[gx,gy]];
  const D=[[1,0],[-1,0],[0,1],[0,-1]];
  while(q.length){
    const[cx,cy]=q.shift();
    for(const[dx,dy]of D){
      const nx=cx+dx,ny=cy+dy;
      if(nx<0||nx>=COLS||ny<0||ny>=ROWS) continue;
      if(grid[ny][nx]===1||wave[ny][nx]!==-1) continue;
      wave[ny][nx]=wave[cy][cx]+1;
      q.push([nx,ny]);
    }
  }
  const sx=Math.floor(rPose.x/CELL)+10;
  const sy=Math.floor(rPose.y/CELL)+10;
  path=[]; let cx=sx,cy=sy;
  for(let i=0;i<60;i++){
    path.push({x:(cx-10)*CELL,y:(cy-10)*CELL});
    if(cx===gx&&cy===gy) break;
    let best=null,bv=Infinity;
    for(const[dx,dy]of D){
      const nx=cx+dx,ny=cy+dy;
      if(wave[ny]?.[nx]>0&&wave[ny][nx]<bv){bv=wave[ny][nx];best=[nx,ny];}
    }
    if(!best) break;
    [cx,cy]=best;
  }
  send({cmd:'path',waypoints:path});
}

// ─── Map renderer ──────────────────────────────────────────────
function drawMap(){
  const cvs=document.getElementById('mapCanvas');
  const W=cvs.offsetWidth; if(!W) return;
  cvs.width=W; cvs.height=280;
  const ctx=cvs.getContext('2d');
  const cw=W/COLS, ch=280/ROWS;
  ctx.clearRect(0,0,W,280);

  // Lưới nền
  ctx.strokeStyle='#1c2128'; ctx.lineWidth=0.5;
  for(let r=0;r<ROWS;r++) for(let c=0;c<COLS;c++){
    if(grid[r][c]===1){ ctx.fillStyle='rgba(248,81,73,0.75)'; ctx.fillRect(c*cw,r*ch,cw,ch); }
    ctx.strokeRect(c*cw,r*ch,cw,ch);
  }

  // Path
  if(path.length>1){
    ctx.strokeStyle='#388bfd'; ctx.lineWidth=2; ctx.setLineDash([4,3]);
    ctx.beginPath();
    path.forEach((p,i)=>{
      const px=(p.x/CELL+10)*cw+cw/2, py=(p.y/CELL+10)*ch+ch/2;
      if(i===0) ctx.moveTo(px,py); else ctx.lineTo(px,py);
    });
    ctx.stroke(); ctx.setLineDash([]);
  }

  // Goal
  if(goal){
    const gx=goal.gx*cw+cw/2, gy=goal.gy*ch+ch/2;
    ctx.fillStyle='#3fb950';
    ctx.beginPath(); ctx.arc(gx,gy,6,0,Math.PI*2); ctx.fill();
    ctx.strokeStyle='#fff'; ctx.lineWidth=1;
    ctx.stroke();
  }

  // Robot (tam giác chỉ hướng)
  const rx=(rPose.x/CELL+10)*cw+cw/2;
  const ry=(rPose.y/CELL+10)*ch+ch/2;
  const ang=rPose.yaw*Math.PI/180;
  const sz=9;
  ctx.save(); ctx.translate(rx,ry); ctx.rotate(ang);
  ctx.fillStyle='#e3b341';
  ctx.beginPath(); ctx.moveTo(sz,0); ctx.lineTo(-sz*.6,sz*.6); ctx.lineTo(-sz*.6,-sz*.6);
  ctx.closePath(); ctx.fill();
  ctx.restore();
}

// ─── Radar ─────────────────────────────────────────────────────
function drawRadar(scan){
  const cvs=document.getElementById('radarCanvas');
  const ctx=cvs.getContext('2d');
  const cx=80,cy=80,R=72;
  ctx.clearRect(0,0,160,160);
  ctx.strokeStyle='#21262d'; ctx.lineWidth=1;
  [R/4,R/2,R*3/4,R].forEach(r=>{
    ctx.beginPath(); ctx.arc(cx,cy,r,0,Math.PI*2); ctx.stroke();
  });
  ctx.strokeStyle='#30363d';
  [-90,-45,0,45,90,135,180,225].forEach(d=>{
    const r2=d*Math.PI/180;
    ctx.beginPath(); ctx.moveTo(cx,cy);
    ctx.lineTo(cx+R*Math.cos(r2),cy+R*Math.sin(r2)); ctx.stroke();
  });
  scan.angles.forEach((deg,i)=>{
    const dist=Math.min(scan.dists[i],250);
    const rad2=(deg-90)*Math.PI/180;
    const len=dist*R/250;
    ctx.strokeStyle='rgba(63,185,80,0.6)'; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.moveTo(cx,cy);
    ctx.lineTo(cx+len*Math.cos(rad2),cy+len*Math.sin(rad2)); ctx.stroke();
    if(dist<230){
      ctx.fillStyle=dist<(CELL*2.5)?'#f85149':'#58a6ff';
      ctx.beginPath(); ctx.arc(cx+len*Math.cos(rad2),cy+len*Math.sin(rad2),3,0,Math.PI*2); ctx.fill();
    }
  });
  // Tâm robot
  ctx.fillStyle='#e3b341';
  ctx.beginPath(); ctx.arc(cx,cy,5,0,Math.PI*2); ctx.fill();
}

// ─── Click map = đặt goal ──────────────────────────────────────
document.getElementById('mapCanvas').addEventListener('click',function(e){
  const r=this.getBoundingClientRect();
  const gx=Math.floor((e.clientX-r.left)/r.width*COLS);
  const gy=Math.floor((e.clientY-r.top)/r.height*ROWS);
  if(gx<0||gx>=COLS||gy<0||gy>=ROWS) return;
  goal={gx,gy};
  const wx=(gx-10)*CELL, wy=(gy-10)*CELL;
  send({cmd:'goto',x:wx,y:wy});
  log('🎯 Mục tiêu: ('+wx+'cm, '+wy+'cm)');
  if(grid.flat().some(v=>v===1)) runWavefront();
  drawMap();
});

connect();
setInterval(drawMap,250);
</script>
</body>
</html>
)rawhtml";
