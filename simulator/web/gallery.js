// Screens gallery (Storybook-style state switcher backed by tests/scenarios/*.txt and tests/golden/*.png).
// Clicking any thumbnail replays its scenario up to that exact line on the live WASM device
// and compares the rendered 240x240 frame pixel for pixel against the committed PNG.
(function(root){
 'use strict';

 function createGallery(opts){
  const {gridEl,filtersEl,statusEl,scenarioSelectEl,api}=opts;
  const scnCache=new Map(),pngCache=new Map();
  let manifest={scenarios:[],goldens:[]},activeGroup='All',activeName='';
  const off=document.createElement('canvas');off.width=240;off.height=240;
  const offCtx=off.getContext('2d',{willReadFrequently:true});

  async function fetchScenario(file){
   if(scnCache.has(file))return scnCache.get(file);
   const r=await fetch(file);if(!r.ok)throw Error(`missing ${file}`);
   const t=await r.text();scnCache.set(file,t);return t;
  }

  async function loadGoldenRgba(name){
   if(pngCache.has(name))return pngCache.get(name);
   const img=new Image();
   await new Promise((res,rej)=>{img.onload=res;img.onerror=()=>rej(Error(`missing golden/${name}.png`));img.src=`golden/${name}.png`;});
   offCtx.clearRect(0,0,240,240);offCtx.drawImage(img,0,0,240,240);
   const data=new Uint8Array(offCtx.getImageData(0,0,240,240).data);
   pngCache.set(name,data);return data;
  }

  async function selectGolden(item){
   activeName=item.name;
   for(const c of gridEl.querySelectorAll('.gal-card'))c.classList.toggle('sel',c.dataset.golden===item.name);
   statusEl.textContent=`replaying ${item.scenario}:${item.line}…`;
   statusEl.className='sub mono';
   try{
    const [text,wantRgba]=await Promise.all([fetchScenario(item.file),loadGoldenRgba(item.name)]);
    const res=await api.replayToLine(text,item.line,item.scenario);
    const gotRgba=api.currentFrameRgba();
    const diff=root.VexScenario.diffRgba(gotRgba,wantRgba);
    const ok=diff===0&&res.failures===0;
    statusEl.className='sub mono '+(ok?'ok':'bad');
    statusEl.textContent=`${item.name} · ${item.scenario}:${item.line} · ${diff} / 57,600 px differ${res.failures?` · ${res.failures} expect fail`:' · paused (click Run to continue)'}`;
    statusEl.dataset.lastGolden=item.name;
    statusEl.dataset.lastDiff=String(diff);
    const card=gridEl.querySelector(`[data-golden="${item.name}"]`);
    if(card){
     const badge=card.querySelector('.gal-diff');
     badge.textContent=`${diff} px`;
     badge.className='gal-diff mono '+(diff===0?'ok':'bad');
    }
    return {name:item.name,diff,failures:res.failures};
   }catch(e){
    statusEl.className='sub mono bad';
    statusEl.textContent=`${item.name}: ${e.message}`;
    statusEl.dataset.lastDiff='-1';
    throw e;
   }
  }

  function renderCards(){
   gridEl.innerHTML='';
   const items=activeGroup==='All'?manifest.goldens:manifest.goldens.filter(g=>g.group===activeGroup);
   for(const item of items){
    const btn=document.createElement('button');
    btn.type='button';
    btn.className='gal-card'+(item.name===activeName?' sel':'');
    btn.dataset.golden=item.name;
    btn.title=`Replay ${item.scenario} up to line ${item.line} (golden ${item.name})`;
    btn.innerHTML=
     `<span class="gal-thumb"><img src="${item.image}" width="64" height="64" loading="lazy" alt="${item.label}"></span>`+
     `<span class="gal-name">${item.label}</span>`+
     `<span class="gal-meta mono"><span>${item.scenario.replace('.txt','')}:${item.line}</span><b class="gal-diff mono"></b></span>`;
    btn.addEventListener('click',()=>selectGolden(item));
    gridEl.appendChild(btn);
   }
  }

  function renderFilters(){
   filtersEl.innerHTML='';
   const groups=['All',...new Set(manifest.goldens.map(g=>g.group))];
   for(const g of groups){
    const count=g==='All'?manifest.goldens.length:manifest.goldens.filter(x=>x.group===g).length;
    const b=document.createElement('button');
    b.type='button';
    b.className='pill'+(g===activeGroup?' sel':'');
    b.dataset.group=g;
    b.textContent=`${g} (${count})`;
    b.addEventListener('click',()=>{activeGroup=g;renderFilters();renderCards();});
    filtersEl.appendChild(b);
   }
  }

  async function init(){
   try{
    const r=await fetch('manifest.json');
    if(!r.ok)throw Error('manifest.json unavailable');
    manifest=await r.json();
    renderFilters();
    renderCards();
    statusEl.textContent=`${manifest.goldens.length} committed golden screens · click any state to jump`;
    if(scenarioSelectEl){
     scenarioSelectEl.innerHTML='<option value="">Load repository scenario…</option>'+
      manifest.scenarios.map(s=>`<option value="${s.file}">${s.name} — ${s.summary.slice(0,58)}</option>`).join('');
     scenarioSelectEl.onchange=async e=>{
      if(!e.target.value)return;
      const text=await fetchScenario(e.target.value);
      api.setScenarioEditor(text);
     };
    }
   }catch(e){
    statusEl.textContent='gallery unavailable: '+e.message;
   }
  }

  return {
   init,
   loadGoldenRgba,
   fetchScenario,
   selectByName(name){const it=manifest.goldens.find(g=>g.name===name);return it?selectGolden(it):Promise.reject(Error(`unknown golden ${name}`));},
   getGoldens:()=>manifest.goldens.slice()
  };
 }

 root.VexGallery={createGallery};
})(typeof globalThis!=='undefined'?globalThis:this);
