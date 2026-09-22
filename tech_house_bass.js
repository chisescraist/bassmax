autowatch = 1;
inlets = 1;
outlets = 2;
// outlet 0: pitch velocity duration (ms); outlet 1: status text
var root = 29; // F1 in MIDI (Ableton octave labels can differ)
var groove = 0;
var density = 65;
var variation = 0;
var step = 0;
var active = [0,3,6,8,10,14];
var patterns = [[0,3,6,8,10,14],[2,6,10,14],[0,3,7,10,13,15],[2,7,10,14]];
function clamp(n,a,b){return Math.max(a,Math.min(b,Math.floor(Number(n)||0)));}
function setroot(n){root=clamp(n,24,48); status();}
function setgroove(n){groove=clamp(n,0,3); rebuild();}
function setdensity(n){density=clamp(n,0,100); rebuild();}
function generate(){variation++; rebuild();}
function reset(){step=0;}
function rebuild(){
 active=[];
 for(var i=0;i<16;i++){
  var base=patterns[groove].indexOf(i)>=0;
  var keep=((i*11+variation*7+3)%100)<Math.min(100,density+20);
  var extra=((i*13+variation*17+7)%101)<Math.max(0,density-55);
  if((base&&keep)||(!base&&extra&&i%2===1))active.push(i);
 }
 status();
}
function status(){outlet(1,'groove '+groove+' / root MIDI '+root+' / density '+density+' / variation '+variation+' / notes '+active.length);}
function bang(){
 var s=step%16;
 if(active.indexOf(s)>=0){
  var intervals=[0,0,7,0,3,0,7,0,0,0,3,0,7,0,0,0];
  var pitch=root+intervals[(s+variation)%16];
  var vel=s%4===0?110:88;
  outlet(0,pitch,vel,105);
 }
 step=(step+1)%16;
}
function loadbang(){rebuild();}
