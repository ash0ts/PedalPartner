// Guided 8-step practice session walkthrough (MakeCode-style self-ticking steps).
// Every step watches real vex::App state so it ticks whether performed by hand on the
// pedal mock or via "Do it for me".
(function(root){
 'use strict';

 const STEPS=[
  {
   id:'plug',
   title:'Plug in (quiet cable)',
   how:'Keep Guitar input on Not playing (buffered dry branch, strings muted).'
  },
  {
   id:'calibrate',
   title:'Calibrate noise floor',
   how:'Hold QUEST (key Q) for 1.2 s while muted, then wait 3 s for Ready.'
  },
  {
   id:'start',
   title:'Start the session',
   how:'Tap FOOTSWITCH (key Space) to start the 5-minute quest.'
  },
  {
   id:'play',
   title:'Play & watch the hold',
   how:'Choose Strumming or Playing for 3 s — the 500 ms hold bridges chord gaps.'
  },
  {
   id:'fragment',
   title:'Earn the quest fragment',
   how:'Reach the 5:00 goal (use 100× speed or Advance 5 min) for +1 fragment.'
  },
  {
   id:'tune',
   title:'Check tuning (credit pauses)',
   how:'Tap MODE (key M) to open the tuner; try Synth note to tune a string.'
  },
  {
   id:'save',
   title:'Pause & save to EEPROM',
   how:'Leave tuner if open (MODE), pause if running, then hold MODE 1.2 s.'
  },
  {
   id:'replug',
   title:'Unplug & replug',
   how:'Click Reboot — totals return from EEPROM; boot stays paused & uncalibrated.'
  }
 ];

 function createSession(container,statusEl,api){
  const completed=new Set();
  let baselineFragments=0,saveCompleted=false,rebootAfterSave=false,lastRenderedSig='';

  function resetProgress(){
   completed.clear();
   const s=api.getState();
   baselineFragments=s?s.fragments:0;
   saveCompleted=false;
   rebootAfterSave=false;
   lastRenderedSig='';
   evaluate();
  }

  function onReboot(){
   if(saveCompleted)rebootAfterSave=true;
  }

  function ensurePetScreen(){
   let s=api.getState();
   for(let guard=0;guard<4&&s.activity!=='pet';guard++){api.press('mode',100);s=api.getState();}
   return s;
  }

  function ensureCalibrated(){
   api.ensureStage4();
   let s=ensurePetScreen();
   if(!s.calibrated){
    api.setInputMode('quiet');
    api.advance(50);
    api.press('quest',1300);
    api.advance(3100);
   }
  }

  function ensureRunning(){
   ensureCalibrated();
   let s=ensurePetScreen();
   if(!s.running)api.press('foot',100);
  }

  function doStep(id){
   api.exitTestPattern();
   if(id==='plug'){
    api.ensureStage4();
    api.setInputMode('quiet');
    api.advance(100);
   }else if(id==='calibrate'){
    api.ensureStage4();
    ensurePetScreen();
    api.setInputMode('quiet');
    api.advance(50);
    api.press('quest',1300);
    api.advance(3100);
   }else if(id==='start'){
    ensureRunning();
   }else if(id==='play'){
    ensureRunning();
    api.setInputMode('strum');
    api.advance(3500);
   }else if(id==='fragment'){
    ensureRunning();
    let s=api.getState();
    const goalMs=s.goal_minutes*60000;
    const remain=goalMs-s.session_ms;
    if(remain>1500){
     api.setInputMode('play');
     api.advance(remain-1000);
    }
    api.setInputMode('strum');
    api.advance(1500);
   }else if(id==='tune'){
    api.ensureStage4();
    let s=api.getState();
    for(let guard=0;guard<4&&!s.tuning;guard++){api.press('mode',100);s=api.getState();}
    api.setInputMode('synth',{hz:440,cents:0,db:-20});
    api.advance(600);
   }else if(id==='save'){
    api.ensureStage4();
    let s=ensurePetScreen();
    if(s.running){api.press('foot',100);s=api.getState();}
    if(s.active_ms===0&&s.fragments===0){
     ensureRunning();
     api.setInputMode('play');
     api.advance(6500);
     api.press('foot',100);
    }
    api.press('mode',1350);
   }else if(id==='replug'){
    if(!saveCompleted)doStep('save');
    evaluate();
    api.setInputMode('quiet');
    api.reboot();
    api.advance(100);
   }
   evaluate();
  }

  function evaluate(){
   const s=api.getState();
   if(!s)return;
   const mode=api.getInputMode();
   if(s.stage===4&&!s.calibrating&&mode==='quiet'&&s.now_ms>=50&&s.rms<=0.02)completed.add('plug');
   if(s.stage===4&&s.calibrated){completed.add('plug');completed.add('calibrate');}
   if(s.stage===4&&s.calibrated&&s.running)completed.add('start');
   if(s.stage===4&&s.running&&s.active&&s.session_ms>=2500)completed.add('play');
   if(s.stage===4&&(s.fragments>baselineFragments||(s.calibrated&&s.fragments>=1&&s.session_ms>=s.goal_minutes*60000)))completed.add('fragment');
   if(s.stage===4&&s.tuning)completed.add('tune');
   if(s.stage===4&&!s.tuning&&!s.running&&s.saved_slot>=0&&s.sequence>=1&&(s.notice==='Saved - safe to unplug'||s.notice==='Already saved')){
    completed.add('save');
    saveCompleted=true;
   }
   if(saveCompleted&&rebootAfterSave&&s.stage===4&&!s.running&&!s.calibrated&&s.saved_slot>=0&&s.sequence>=1){
    completed.add('replug');
   }
   render();
  }

  function render(){
   const activeIdx=STEPS.findIndex(st=>!completed.has(st.id));
   const sig=Array.from(completed).sort().join(',')+'|'+activeIdx;
   if(sig===lastRenderedSig)return;
   lastRenderedSig=sig;
   statusEl.textContent=`${completed.size} / ${STEPS.length} complete`;
   for(let i=0;i<STEPS.length;i++){
    const st=STEPS[i],row=container.children[i];
    if(!row)continue;
    const done=completed.has(st.id),cur=i===activeIdx;
    row.classList.toggle('done',done);
    row.classList.toggle('current',cur);
    row.dataset.done=done?'1':'0';
    row.querySelector('.step-mark').textContent=done?'✓':String(i+1);
   }
  }

  function buildDom(){
   container.innerHTML='';
   STEPS.forEach((st,i)=>{
    const row=document.createElement('div');
    row.className='step-row';
    row.dataset.step=st.id;
    row.innerHTML=
     `<span class="step-mark" aria-hidden="true">${i+1}</span>`+
     `<div class="step-body"><strong>${st.title}</strong><p>${st.how}</p></div>`+
     `<button type="button" class="step-do" data-do="${st.id}">Do it for me</button>`;
    row.querySelector('.step-do').addEventListener('click',()=>doStep(st.id));
    container.appendChild(row);
   });
  }

  buildDom();
  return {STEPS,evaluate,resetProgress,onReboot,doStep,getCompleted:()=>Array.from(completed)};
 }

 root.VexSession={STEPS,createSession};
})(typeof globalThis!=='undefined'?globalThis:this);
