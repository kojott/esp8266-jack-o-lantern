#pragma once
#include <Arduino.h>

static const char PAGE_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Candle</title>
<style>
:root{color-scheme:dark;--bg:#17110c;--card:#241a12;--txt:#f6e7d3;--mut:#b49a80;--acc:#ff9a2e;--line:#4a3a2c}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--txt);font:16px/1.4 -apple-system,system-ui,sans-serif}
main{max-width:460px;margin:0 auto;padding:16px}
header{display:flex;align-items:center;gap:12px;margin:4px 0 16px}
header h1{font-size:22px;margin:0;line-height:1.2;word-break:break-word}
header .host{color:var(--mut);font-size:13px}
.flame{flex:none;width:18px;height:28px;border-radius:50% 50% 45% 45%/60% 60% 40% 40%;background:radial-gradient(circle at 50% 70%,#fff3c4,#ffb347 45%,#ff6a00 80%);box-shadow:0 0 18px #ff8c1a;animation:fl .15s infinite alternate}
.off .flame{background:#4a3a2c;box-shadow:none;animation:none}
@keyframes fl{to{transform:scale(.94,1.04) rotate(2deg);opacity:.85}}
.card{background:var(--card);border-radius:14px;padding:16px;margin-bottom:12px}
.card h2{font-size:15px;margin:20px 0 4px;padding-top:14px;border-top:1px solid var(--line);color:var(--txt);font-weight:600}
label{display:block;color:var(--mut);font-size:14px;margin:12px 0 4px}
label span{float:right;color:var(--txt)}
input[type=range]{width:100%;accent-color:var(--acc)}
input[type=color]{width:100%;height:44px;border:0;background:none;padding:0}
select,input[type=number],input[type=text],input[type=password],input[type=time]{width:100%;padding:10px;border-radius:8px;border:1px solid var(--line);background:#1b140e;color:var(--txt);font-size:16px}
button{padding:12px;border-radius:10px;border:0;background:#3a2b1e;color:var(--txt);font-size:15px;cursor:pointer}
button.on,button.pri{background:var(--acc);color:#1b0f00;font-weight:600}
.big{width:100%;font-size:18px;padding:16px}
.row{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.inl{display:flex;gap:8px;align-items:stretch}.inl>input{flex:1}
.sw{display:flex;gap:6px;justify-content:space-between;margin-top:8px}
.sw i{flex:1;max-width:34px;aspect-ratio:1;border-radius:50%;cursor:pointer;border:2px solid #0003}
.suf{display:flex;align-items:center;gap:6px}.suf input{flex:1}.suf b{color:var(--mut);font-weight:400}
.nets{margin-top:10px;display:flex;flex-direction:column;gap:6px}
.nets button{text-align:left;display:flex;justify-content:space-between}
.nets button.cur{outline:2px solid var(--acc)}
.msg{color:var(--acc);font-size:14px;margin-top:8px;min-height:1em}
details summary{cursor:pointer;color:var(--mut)}
small{color:var(--mut)}
.chk{display:flex;align-items:center;gap:10px;color:var(--txt);font-size:15px}.chk input{width:20px;height:20px;accent-color:var(--acc)}
.two{display:grid;grid-template-columns:1fr 1fr;gap:10px}
</style></head><body><main id="app">
<header><div class="flame"></div><div><h1 id="nm">Candle</h1><div class="host" id="hn"></div></div></header>

<div class="card"><button id="power" class="big">…</button>
<label>Sleep timer <span id="tl"></span></label>
<div class="row"><button data-t="15">15 min</button><button data-t="30">30 min</button><button data-t="60">1 h</button><button data-t="0">cancel</button></div>
<div class="inl" style="margin-top:8px"><input type="number" id="tmin" min="1" max="1440" placeholder="custom (minutes)"><button id="tset">Set</button></div>
</div>

<div class="card">
<label style="margin-top:0">Mode</label>
<select id="mode"><option value="0">Candle</option><option value="1">Fire (wilder)</option><option value="2">Steady light</option></select>
<label>Brightness <span id="vb"></span></label><input type="range" id="brightness" min="1" max="255">
<label>Flicker <span id="vf"></span></label><input type="range" id="flicker" min="0" max="100">
<label>Speed <span id="vs"></span></label><input type="range" id="speed" min="0" max="100">
<label>Flame colour</label><input type="color" id="color">
<div class="sw">
<i style="background:#ff6e14" data-c="#ff6e14" title="flame"></i><i style="background:#ff8c28" data-c="#ff8c28"></i>
<i style="background:#ffa040" data-c="#ffa040"></i>
<i style="background:#ffe2b8" data-c="#ffe2b8" title="warm white"></i><i style="background:#ffffff" data-c="#ffffff" title="white"></i>
<i style="background:#ff3c0a" data-c="#ff3c0a"></i><i style="background:#5a8cff" data-c="#5a8cff"></i>
<i style="background:#a050ff" data-c="#a050ff"></i><i style="background:#40ff80" data-c="#40ff80"></i></div>
</div>

<div class="card"><details><summary>Advanced</summary>
<label>Number of LEDs</label><input type="number" id="ledCount" min="1" max="60">
<label>Data pin (GPIO)</label><select id="pin"><option>2</option><option>4</option><option>5</option><option>12</option><option>13</option><option>14</option></select>
<p><small id="info"></small></p>
<h2>Schedule</h2>
<label class="chk"><input type="checkbox" id="sched"> Turn on and off by the clock</label>
<div class="two"><div><label>On at</label><input type="time" id="schedOn"></div><div><label>Off at</label><input type="time" id="schedOff"></div></div>
<label>Time zone</label><select id="tz">
<option value="UTC0">UTC</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0">London, Dublin, Lisbon</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Prague, Berlin, Paris, Rome</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Helsinki, Kyiv, Athens</option>
<option value="EST5EDT,M3.2.0,M11.1.0">US Eastern</option>
<option value="CST6CDT,M3.2.0,M11.1.0">US Central</option>
<option value="MST7MDT,M3.2.0,M11.1.0">US Mountain</option>
<option value="PST8PDT,M3.2.0,M11.1.0">US Pacific</option>
<option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Sydney, Melbourne</option>
</select>
<p><small id="clock"></small></p>
<small>Plugging it in always lights it with the last settings. Once it knows the time from the internet, it follows the next on/off step.</small>
<div style="margin-top:12px"><button id="schedsave" class="pri">Save schedule</button></div>
<div class="msg" id="schedmsg"></div>

<h2>Device</h2>
<label>Name</label><input type="text" id="name" maxlength="40">
<label>Network address</label><div class="suf"><b>http://</b><input type="text" id="hostname" maxlength="24" autocapitalize="off" spellcheck="false"><b>.local</b></div>
<small>Lowercase letters, digits and dashes only. Every device needs its own address.</small>
<div style="margin-top:12px"><button id="devsave" class="pri">Save</button></div>
<div class="msg" id="devmsg"></div>

<h2>WiFi</h2>
<small id="wcur"></small>
<div style="margin-top:10px"><button id="scan">Scan networks</button></div>
<div class="nets" id="nets"></div>
<div id="wform" hidden>
<label>Password for <b id="wssid"></b></label>
<div class="inl"><input type="password" id="wpass" autocomplete="off"><button id="wgo" class="pri">Connect</button></div>
</div>
<div class="msg" id="wmsg"></div>
<p><small>If it can't join the new network it opens its own hotspot <b id="aphint"></b> – connect with your phone and pick a WiFi.</small></p>
<button id="wifi">Forget WiFi</button>
</details></div>
</main>
<script>
const $=id=>document.getElementById(id);let st={},timers={},selSsid='',schedEdit=false;
const api=q=>fetch('/api?'+q).then(r=>r.json()).then(show);
function send(q,delay=0){const k=q.split('=')[0];clearTimeout(timers[k]);timers[k]=setTimeout(()=>api(q),delay)}
function fmt(s){const m=Math.ceil(s/60);return m>=60?Math.floor(m/60)+' h'+(m%60?' '+m%60+' min':''):m+' min'}
function show(s){st=s;$('app').className=s.on?'':'off';
document.title=s.name;$('nm').textContent=s.name;$('hn').textContent=s.hostname+'.local';
$('power').textContent=s.on?'Blow it out':'Light it up';$('power').className='big'+(s.on?' on':'');
for(const k of['brightness','flicker','speed','mode','ledCount','pin','color','name','hostname']){const el=$(k);if(document.activeElement!==el&&!el.dataset.edit)el.value=s[k]}
$('vb').textContent=Math.round(s.brightness/2.55)+' %';$('vf').textContent=s.flicker+' %';$('vs').textContent=s.speed+' %';
$('tl').textContent=s.timerLeft?'off in '+fmt(s.timerLeft):'';
$('wcur').textContent='Connected to “'+s.ssid+'” · signal '+s.rssi+' dBm';$('aphint').textContent=s.hostname+'-setup';
$('info').textContent='IP '+s.ip;
const hm=m=>String(Math.floor(m/60)).padStart(2,'0')+':'+String(m%60).padStart(2,'0');
if(!schedEdit){$('sched').checked=!!s.sched;$('schedOn').value=hm(s.schedOn);$('schedOff').value=hm(s.schedOff);$('tz').value=s.tz}
$('clock').textContent=s.time?'Pumpkin time: '+s.time:'No internet time yet – running on the last settings.'}
$('power').onclick=()=>send('toggle=1');
for(const k of['brightness','flicker','speed'])$(k).oninput=e=>send(k+'='+e.target.value,80);
$('color').oninput=e=>send('color='+encodeURIComponent(e.target.value),80);
for(const k of['mode','ledCount','pin'])$(k).onchange=e=>send(k+'='+e.target.value);
document.querySelectorAll('[data-t]').forEach(b=>b.onclick=()=>send('timer='+b.dataset.t));
$('tset').onclick=()=>{const m=parseInt($('tmin').value);if(m>0){send('timer='+m);$('tmin').value=''}};
document.querySelectorAll('[data-c]').forEach(b=>b.onclick=()=>send('color='+encodeURIComponent(b.dataset.c)));
for(const k of['name','hostname'])$(k).oninput=()=>$(k).dataset.edit=1;
$('devsave').onclick=()=>{const n=$('name').value.trim(),h=$('hostname').value.trim().toLowerCase();
if(!/^[a-z0-9]([a-z0-9-]{0,22}[a-z0-9])?$/.test(h)){$('devmsg').textContent='Use only a–z, 0–9 and dashes.';return}
const hostChanged=h!==st.hostname;
fetch('/api?name='+encodeURIComponent(n)+'&hostname='+encodeURIComponent(h)).then(r=>r.json()).then(s=>{
delete $('name').dataset.edit;delete $('hostname').dataset.edit;show(s);
if(hostChanged){$('devmsg').textContent='Restarting… new address http://'+h+'.local';setTimeout(()=>location.href='http://'+h+'.local/',9000)}
else{$('devmsg').textContent='Saved.';setTimeout(()=>$('devmsg').textContent='',2000)}})};
function scan(){$('scan').disabled=true;$('wmsg').textContent='Scanning…';$('nets').innerHTML='';
fetch('/wifi/scan').then(r=>r.json()).then(d=>{if(d.scanning)return setTimeout(scan,1200);
$('scan').disabled=false;$('wmsg').textContent=d.networks.length?'':'No networks found.';
d.networks.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{const b=document.createElement('button');
b.innerHTML='<span></span><small>'+(n.open?'':'🔒 ')+n.rssi+' dBm</small>';b.firstChild.textContent=n.ssid;
if(n.ssid===st.ssid)b.className='cur';
b.onclick=()=>{selSsid=n.ssid;$('wssid').textContent=n.ssid;$('wform').hidden=false;$('wpass').value='';if(n.open)$('wgo').click();else $('wpass').focus()};
$('nets').appendChild(b)})}).catch(()=>{$('scan').disabled=false;$('wmsg').textContent='Scan failed.'})}
$('scan').onclick=scan;
for(const k of['sched','schedOn','schedOff','tz'])$(k).oninput=()=>schedEdit=true;
$('schedsave').onclick=()=>{const q='sched='+($('sched').checked?1:0)+'&schedOn='+$('schedOn').value+'&schedOff='+$('schedOff').value+'&tz='+encodeURIComponent($('tz').value);
fetch('/api?'+q).then(r=>r.json()).then(s=>{schedEdit=false;show(s);$('schedmsg').textContent='Saved.';setTimeout(()=>$('schedmsg').textContent='',2000)})};
$('wgo').onclick=()=>{if(!selSsid)return;if(!confirm('Switch the candle to “'+selSsid+'”?'))return;
const b=new URLSearchParams({ssid:selSsid,pass:$('wpass').value});
fetch('/wifi/connect',{method:'POST',body:b}).then(r=>{if(!r.ok)throw 0;
$('wmsg').textContent='Switching to “'+selSsid+'”. Join the same network and open http://'+st.hostname+'.local';
$('wform').hidden=true;$('nets').innerHTML=''}).catch(()=>$('wmsg').textContent='Something went wrong.')};
$('wifi').onclick=()=>{if(confirm('Really forget WiFi? The candle will open the hotspot '+st.hostname+'-setup.'))fetch('/wifireset',{method:'POST'})};
api('');setInterval(()=>fetch('/api').then(r=>r.json()).then(show).catch(()=>{}),5000);
</script></body></html>)HTML";
