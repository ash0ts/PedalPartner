// Progressive step-by-step guide + inline drill test bench.
// Shows ONE expanded step at a time (completed & upcoming steps stay compact), highlights the
// exact physical control on the pedal (MODE, QUEST, FOOTSWITCH), updates pedal button captions
// to match the current screen, and turns the final step of each drill into a clean test playground.
(function(root){
 'use strict';

 const PC=['C','C#','D','D#','E','F','F#','G','G#','A','Bb','B'];
 const STR_STARTS=[40,45,50,55,59,64];
 const STR_LABELS=['low E string','A string','D string','G string','B string','high E string'];
 const PRESET_ORDER=[
  'pulse-qtr','pulse-8th','pulse-free',
  'hunt-any','hunt-str',
  'bend-half','bend-whole',
  'run-penta','run-major','run-minor',
  'touch-even','touch-acc3','touch-acc4'
 ];
 const PRESET_SCREEN={
  'pulse-qtr':'PULSE QTR','pulse-8th':'PULSE 8TH','pulse-free':'PULSE FREE',
  'hunt-any':'HUNT ANY','hunt-str':'HUNT STR',
  'bend-half':'BEND 1/2','bend-whole':'BEND FULL',
  'run-penta':'RUN PENTA','run-major':'RUN MAJOR','run-minor':'RUN MINOR',
  'touch-even':'TOUCH EVEN','touch-acc3':'TOUCH ACC3','touch-acc4':'TOUCH ACC4'
 };

 function isDrill(act){return !!act&&act!=='pet'&&act!=='tuner';}

 function fmtBest(act,v){
  if(!v)return 'none yet';
  if(act.startsWith('pulse')||act.startsWith('run'))return `${v} BPM`;
  if(act.startsWith('hunt'))return `${(v/100).toFixed(2)} s/note`;
  if(act.startsWith('bend'))return `${(v/10).toFixed(1)} c`;
  if(act.startsWith('touch'))return `${(v/100).toFixed(2)} dB`;
  return String(v);
 }

 function fmtClock(ms){
  const sec=Math.max(0,Math.floor(ms/1000));
  return `${Math.floor(sec/60)}:${String(sec%60).padStart(2,'0')}`;
 }

 function questTapsTo(curSlug,slugs){
  const ci=PRESET_ORDER.indexOf(curSlug);
  if(ci<0)return null;
  let best=99,bestSlug=slugs[0];
  for(const sl of slugs){
   const ti=PRESET_ORDER.indexOf(sl);
   if(ti<0)continue;
   const d=(ti-ci+PRESET_ORDER.length)%PRESET_ORDER.length;
   if(d<best){best=d;bestSlug=sl;}
  }
  return {taps:best,slug:bestSlug,screen:PRESET_SCREEN[bestSlug]||bestSlug.toUpperCase()};
 }

 function huntPromptText(s){
  const note=PC[s.hunt_pc]||'?';
  if(s.hunt_midi>0){
   let si=0;
   for(let i=0;i<STR_STARTS.length;i++){
    if(s.hunt_midi>=STR_STARTS[i]&&s.hunt_midi<=STR_STARTS[i]+12)si=i;
   }
   const fret=s.hunt_midi-STR_STARTS[si];
   return `${note} on ${STR_LABELS[si]} (fret ${fret})`;
  }
  return `Any ${note} on the neck`;
 }

 const DRILL_GUIDES={
  pulse:{
   id:'pulse',
   label:'Timing',
   prefix:'pulse',
   defaultSlug:'pulse-qtr',
   slugs:['pulse-qtr','pulse-8th','pulse-free'],
   modes:[
    {slug:'pulse-qtr',label:'Quarter notes'},
    {slug:'pulse-8th',label:'Eighth notes'},
    {slug:'pulse-free',label:'Your own tempo'}
   ],
   playDesc:'Pick steady notes with the flashing red LED beat. Four clean bars within ±30 ms raises the tempo by +4 BPM.',
   takes:()=>[
    {kind:'steady',label:'▶ Play 4 clean bars (+4 BPM)',primary:true},
    {kind:'rushing',label:'▶ Rush ahead of the beat'},
    {kind:'uneven',label:'▶ Play sloppy / uneven'}
   ],
   scores:(s)=>[
    {k:'Tempo',v:s.bpm?`${s.bpm} BPM`:'your pace'},
    {k:'Personal best',v:fmtBest('pulse',s.drill_best),hi:s.drill_best>0},
    {k:'Rounds',v:String(s.drill_rounds)},
    {k:'5:00 Quest',v:`${fmtClock(s.session_ms)} / ${s.goal_minutes}:00`}
   ]
  },
  hunt:{
   id:'hunt',
   label:'Note Finder',
   prefix:'hunt',
   defaultSlug:'hunt-str',
   slugs:['hunt-any','hunt-str'],
   modes:[
    {slug:'hunt-str',label:'Specific string + note'},
    {slug:'hunt-any',label:'Any string / octave'}
   ],
   playDesc:'Play the note shown on the screen. Ten notes finish a round; your slowest note comes back twice next round.',
   takes:()=>[
    {kind:'steady',label:'▶ Find all 10 notes cleanly',primary:true},
    {kind:'wrong',label:'▶ Play wrong string / note (red flash)'},
    {kind:'uneven',label:'▶ Hesitate on slow notes'}
   ],
   scores:(s)=>[
    {k:'Find now',v:huntPromptText(s),hi:true},
    {k:'Personal best',v:fmtBest('hunt',s.drill_best),hi:s.drill_best>0},
    {k:'Rounds',v:String(s.drill_rounds)},
    {k:'5:00 Quest',v:`${fmtClock(s.session_ms)} / ${s.goal_minutes}:00`}
   ]
  },
  bend:{
   id:'bend',
   label:'Bend & Vibrato',
   prefix:'bend',
   defaultSlug:'bend-half',
   slugs:['bend-half','bend-whole'],
   modes:[
    {slug:'bend-half',label:'Half-step (+100c)'},
    {slug:'bend-whole',label:'Whole-step (+200c)'}
   ],
   playDesc:'Strike a note and bend up to the gold target line, or hold an unbent note with steady vibrato (5–7 Hz).',
   takes:()=>[
    {kind:'steady',label:'▶ Play 8 clean target bends',primary:true},
    {kind:'rushing',label:'▶ Hold steady vibrato'},
    {kind:'wrong',label:'▶ Under-bend (flat)'},
    {kind:'uneven',label:'▶ Over-bend (sharp)'}
   ],
   scores:(s)=>[
    {k:'Bends',v:`${s.bend_count} / 8`},
    {k:'Last bend',v:`${s.bend_err>=0?'+':''}${s.bend_err} c`},
    {k:'Vibrato',v:s.vib_rate_x10>0?`${(s.vib_rate_x10/10).toFixed(1)} Hz · ${s.vib_width} c`:'—'},
    {k:'Personal best',v:fmtBest('bend',s.drill_best),hi:s.drill_best>0}
   ]
  },
  run:{
   id:'run',
   label:'Scale Run',
   prefix:'run',
   defaultSlug:'run-penta',
   slugs:['run-penta','run-major','run-minor'],
   modes:[
    {slug:'run-penta',label:'Minor Pentatonic'},
    {slug:'run-major',label:'Major Scale'},
    {slug:'run-minor',label:'Natural Minor'}
   ],
   playDesc:'Your first note sets the key root. Play a smooth 16-note run in key — outside notes flash red and block a new BPM best.',
   takes:()=>[
    {kind:'steady',label:'▶ Play clean 16-note run',primary:true},
    {kind:'wrong',label:'▶ Play out-of-key notes (red flash)'},
    {kind:'uneven',label:'▶ Play uneven rhythm'}
   ],
   scores:(s)=>[
    {k:'Key root',v:s.run_root>=0?PC[s.run_root]:'first note sets root'},
    {k:'Outside notes',v:String(s.run_outside)},
    {k:'Personal best',v:fmtBest('run',s.drill_best),hi:s.drill_best>0},
    {k:'5:00 Quest',v:`${fmtClock(s.session_ms)} / ${s.goal_minutes}:00`}
   ]
  },
  touch:{
   id:'touch',
   label:'Dynamics',
   prefix:'touch',
   defaultSlug:'touch-acc3',
   slugs:['touch-acc3','touch-acc4','touch-even'],
   modes:[
    {slug:'touch-acc3',label:'Accent every 3rd'},
    {slug:'touch-acc4',label:'Accent every 4th'},
    {slug:'touch-even',label:'Even volume'}
   ],
   playDesc:'Controls pick attack consistency. Keep quiet notes steady within ±1.5 dB, and dig in +4 dB louder on accented downbeats.',
   takes:(s)=>[
    {kind:'steady',label:s&&s.activity==='touch-even'?'▶ Play 12 even picks':'▶ Play clean accented round',primary:true},
    {kind:'wrong',label:s&&s.activity==='touch-even'?'▶ Spike one loud pick':'▶ Accent the wrong beat'},
    {kind:'uneven',label:'▶ Play wild / uneven volume'}
   ],
   scores:(s)=>[
    {k:'Last spread',v:fmtBest('touch',s.drill_last)},
    {k:'Personal best',v:fmtBest('touch',s.drill_best),hi:s.drill_best>0},
    {k:'Rounds',v:String(s.drill_rounds)},
    {k:'5:00 Quest',v:`${fmtClock(s.session_ms)} / ${s.goal_minutes}:00`}
   ]
  }
 };

 function makeDrillTrack(g){
  return {
   id:g.id,
   label:g.label,
   drill:g,
   steps:[
    {
     id:`${g.id}-cal`,
     title:'Calibrate quiet cable',
     how:'Hold the red <b>QUEST</b> button (<kbd>Q</kbd>) for <b>1.2 seconds</b> while strings are muted, then wait 3 seconds for <i>Ready</i>.',
     btn:'Calibrate for me'
    },
    {
     id:`${g.id}-open`,
     title:`Select ${g.label} on the pedal`,
     how:`Switch to Guitar Drills with <b>MODE</b> (<kbd>M</kbd>), then tap <b>QUEST</b> (<kbd>Q</kbd>) while paused until <b>${g.label}</b> appears.`,
     btn:`Jump to ${g.label}`
    },
    {
     id:`${g.id}-start`,
     title:'Tap FOOTSWITCH to start',
     how:'Tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to arm the round.',
     btn:'Start for me'
    },
    {
     id:`${g.id}-play`,
     title:`Play & test ${g.label}`,
     how:g.playDesc,
     playground:true
    }
   ]
  };
 }

 const TRACKS=[
  {
   id:'tour',
   label:'First Run',
   steps:[
    {
     id:'t-cal',
     title:'Hold QUEST (1.2 s) to calibrate',
     how:'With strings muted, press and hold the red <b>QUEST</b> button (<kbd>Q</kbd>) for <b>1.2 seconds</b>, then wait 3 seconds for <i>Ready</i>.',
     btn:'Calibrate for me'
    },
    {
     id:'t-tuner',
     title:'Tap MODE once to check the Tuner',
     how:'Tap <b>MODE</b> (<kbd>M</kbd>) once to open the chromatic tuner. Practice credit pauses automatically while you tune.',
     btn:'Open Tuner & play A4'
    },
    {
     id:'t-pick',
     title:'Tap MODE for Drills, then tap QUEST to pick one',
     how:'Tap <b>MODE</b> (<kbd>M</kbd>) to enter <b>Guitar Drills</b>, then tap <b>QUEST</b> (<kbd>Q</kbd>) while paused to cycle through the drills.',
     btn:'Pick a drill for me'
    },
    {
     id:'t-drill',
     title:'Tap FOOTSWITCH to start & play a round',
     how:'Tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to start, then play a clean round. Practicing a drill earns both a <b>Personal Best</b> and <b>5:00 Quest</b> time!',
     btn:'▶ Play a clean round',
     btnPrimary:true
    },
    {
     id:'t-quest',
     title:'Reach 5:00 to claim your Quest Fragment',
     how:'Keep playing (in <b>Drills</b> or <b>Pet</b> mode) until the gold ring hits 5:00 to earn <b>+1 Fragment</b>.',
     btn:'▶ Fast-forward to 5:00',
     btnPrimary:true
    },
    {
     id:'t-save',
     title:'Pause, hold MODE (1.2 s) to save, & Reboot',
     how:'Pause first (<kbd>Space</kbd>), hold <b>MODE</b> (<kbd>M</kbd>) for 1.2 s until <i>Saved</i> appears, then click <b>Reboot</b> to verify EEPROM restore.',
     btn:'Save & Reboot for me'
    }
   ]
  },
  makeDrillTrack(DRILL_GUIDES.pulse),
  makeDrillTrack(DRILL_GUIDES.hunt),
  makeDrillTrack(DRILL_GUIDES.bend),
  makeDrillTrack(DRILL_GUIDES.run),
  makeDrillTrack(DRILL_GUIDES.touch)
 ];

 function updatePedalCaptions(s,spotlight){
  const modeSub=document.querySelector('[data-ctl-sub="mode"]');
  const questSub=document.querySelector('[data-ctl-sub="quest"]');
  const footSub=document.querySelector('[data-ctl-sub="foot"]');
  if(s&&modeSub&&questSub&&footSub){
   if(s.activity==='pet'){
    modeSub.textContent=s.running?'tap: open Tuner (M)':'tap: Tuner · hold: save (M)';
    questSub.textContent=!s.calibrated?'hold 1.2s: calibrate (Q)':(s.running?'hold 1.2s: calibrate (Q)':`tap: goal (${s.goal_minutes}m) · hold: cal (Q)`);
    footSub.textContent=s.running?'tap: pause quest (Space)':'tap: start quest (Space)';
   }else if(s.tuning){
    modeSub.textContent='tap: open Drills (M)';
    questSub.textContent='hold 1.2s: calibrate (Q)';
    footSub.textContent='paused while tuning';
   }else{
    modeSub.textContent=s.running?'tap: back to Pet (M)':'tap: Pet · hold: save (M)';
    questSub.textContent=s.running?'pause first to switch drill':'tap: next drill · hold: cal (Q)';
    footSub.textContent=s.running?'tap: pause drill (Space)':'tap: start drill (Space)';
   }
  }
  for(const el of document.querySelectorAll('.ctl[data-control]')){
   const name=el.dataset.control;
   const hit=spotlight&&spotlight.ctl===name;
   el.classList.toggle('target',Boolean(hit));
   const cue=el.querySelector('.ctl-cue');
   if(cue)cue.textContent=hit?(spotlight.cue||'HERE'):'';
  }
 }

 function createSession(container,statusEl,api){
  const doneByTrack=new Map(TRACKS.map(t=>[t.id,new Set()]));
  let activeTrackId=localStorage.getItem('vex.guide.track')||'tour';
  if(!TRACKS.some(t=>t.id===activeTrackId))activeTrackId='tour';
  let peekStepId='';

  let base={fragments:0};
  let seen={
   tuner:false,presetStepped:false,tourDrillWon:false,tourFragment:false,
   save:false,rebootAfterSave:false
  };
  let startedFamilies=new Set();
  let wonFamilies=new Set();
  let familyRoundsAtOpen={};
  let prevDrillSlug='';
  let lastSig='';

  function activeTrack(){return TRACKS.find(t=>t.id===activeTrackId)||TRACKS[0];}

  function resetProgress(){
   for(const set of doneByTrack.values())set.clear();
   const s=api.getState();
   base={fragments:s?s.fragments:0};
   seen={
    tuner:false,presetStepped:false,tourDrillWon:false,tourFragment:false,
    save:false,rebootAfterSave:false
   };
   startedFamilies=new Set();
   wonFamilies=new Set();
   familyRoundsAtOpen={};
   prevDrillSlug=s&&isDrill(s.activity)?s.activity:'';
   peekStepId='';
   lastSig='';
   evaluate();
  }

  function onReboot(){
   if(seen.save)seen.rebootAfterSave=true;
   startedFamilies.clear();
  }

  function setTrack(id){
   if(!TRACKS.some(t=>t.id===id))return;
   activeTrackId=id;
   peekStepId='';
   try{localStorage.setItem('vex.guide.track',id);}catch(_){}
   const s=api.getState();
   const tr=activeTrack();
   if(tr.drill&&s&&s.activity.startsWith(tr.drill.prefix)){
    familyRoundsAtOpen[tr.id]=s.drill_rounds;
   }
   lastSig='';
   buildDom();
   evaluate();
  }

  // Helpers driving real pedal controls / shared protocol -----------------------------------------
  function ensureCalibrated(){
   api.ensureStage4();
   let s=api.getState();
   if(!s.calibrated){
    api.setInputMode('quiet');
    api.advance(50);
    api.press('quest',1300);
    api.advance(3100);
   }
  }
  function ensureScreen(kind){
   let s=api.getState();
   for(let g=0;g<4;g++){
    if(kind==='pet'&&s.activity==='pet')break;
    if(kind==='tuner'&&s.tuning)break;
    if(kind==='drill'&&isDrill(s.activity))break;
    api.press('mode',100);
    s=api.getState();
   }
   return s;
  }
  function switchPresetClean(slug,autoStart){
   ensureCalibrated();
   let s=ensureScreen('drill');
   if(s.running){api.press('foot',100);s=api.getState();}
   if(s.activity!==slug){
    api.selectDrill(slug);
    seen.presetStepped=true;
   }
   s=api.getState();
   const tr=activeTrack();
   if(tr.drill&&slug.startsWith(tr.drill.prefix)){
    familyRoundsAtOpen[tr.id]=s.drill_rounds;
   }
   if(autoStart&&!s.running){
    api.press('foot',100);
   }
   evaluate();
  }

  function doStep(stepId){
   api.exitTestPattern();
   const tr=activeTrack();
   if(stepId==='t-cal'||stepId.endsWith('-cal')){
    ensureCalibrated();
   }else if(stepId==='t-tuner'){
    ensureCalibrated();
    ensureScreen('tuner');
    seen.tuner=true;
    api.playTone(650,440,-18);
   }else if(stepId==='t-pick'){
    ensureCalibrated();
    let s=ensureScreen('drill');
    if(s.running)api.press('foot',100);
    api.press('quest',100);
    seen.presetStepped=true;
   }else if(stepId==='t-drill'){
    ensureCalibrated();
    let s=api.getState();
    if(!isDrill(s.activity))s=ensureScreen('drill');
    if(!s.running)api.press('foot',100);
    api.playDrill('steady');
   }else if(stepId==='t-quest'){
    ensureCalibrated();
    let s=api.getState();
    if(s.tuning)s=ensureScreen('pet');
    if(!s.running)api.press('foot',100);
    s=api.getState();
    const remain=s.goal_minutes*60000-s.session_ms;
    if(remain>1500){api.setInputMode('play');api.advance(remain-1000);}
    api.setInputMode('strum');api.advance(1500);
    api.setInputMode('quiet');
   }else if(stepId==='t-save'){
    ensureCalibrated();
    let s=api.getState();
    if(s.tuning)s=ensureScreen('pet');
    s=api.getState();
    if(s.running){api.press('foot',100);s=api.getState();}
    if(!seen.save){
     if(s.active_ms===0&&s.fragments===0&&s.skills_slot<0){
      api.press('foot',100);api.setInputMode('play');api.advance(4000);api.setInputMode('quiet');api.press('foot',100);
     }
     api.press('mode',1350);
     evaluate();
    }
    api.setInputMode('quiet');
    api.reboot();
    api.advance(100);
   }else if(tr.drill&&stepId===`${tr.id}-open`){
    switchPresetClean(tr.drill.defaultSlug,false);
   }else if(tr.drill&&stepId===`${tr.id}-start`){
    const g=tr.drill;
    let s=api.getState();
    if(!s.activity.startsWith(g.prefix))switchPresetClean(g.defaultSlug,true);
    else{
     ensureCalibrated();
     if(!s.running)api.press('foot',100);
    }
   }
   evaluate();
  }

  function runDrillTake(kind){
   api.exitTestPattern();
   const tr=activeTrack();
   if(!tr.drill)return;
   const g=tr.drill;
   ensureCalibrated();
   let s=api.getState();
   if(!s.activity.startsWith(g.prefix)){
    switchPresetClean(g.defaultSlug,true);
   }else if(!s.running){
    api.press('foot',100);
   }
   const before=api.getState();
   const prevRounds=before?before.drill_rounds:0;
   api.playDrill(kind);
   const after=api.getState();
   if(after&&(after.drill_rounds>prevRounds||(kind==='steady'&&after.drill_best>0))){
    wonFamilies.add(tr.id);
   }
   evaluate();
  }

  // Dynamic 1-sentence instruction & pedal spotlight (never two competing instructions!) --------
  function guidanceForCurrentStep(tr,stepId,s){
   if(!s)return {how:'',spot:null};
   if(stepId==='t-cal'||stepId.endsWith('-cal')){
    if(s.calibrating)return {how:'Calibrating noise floor… keep strings muted for <b>3 seconds</b>.',spot:{ctl:'quest',cue:'WAIT 3s'}};
    return {how:'With strings muted, press &amp; hold the red <b>QUEST</b> button (<kbd>Q</kbd>) for <b>1.2 seconds</b>.',spot:{ctl:'quest',cue:'HOLD 1.2s'}};
   }
   if(stepId==='t-tuner'){
    if(isDrill(s.activity))return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) twice to cycle around to the <b>Tuner</b>.',spot:{ctl:'mode',cue:'TAP ×2'}};
    return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) once to open the chromatic tuner (practice credit pauses while tuning).',spot:{ctl:'mode',cue:'TAP'}};
   }
   if(stepId==='t-pick'){
    if(s.activity==='pet')return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) twice to open <b>Guitar Drills</b>.',spot:{ctl:'mode',cue:'TAP ×2'}};
    if(s.tuning)return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) once more to open <b>Guitar Drills</b>.',spot:{ctl:'mode',cue:'TAP'}};
    if(s.running)return {how:'Tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to pause first so <b>QUEST</b> can switch drills.',spot:{ctl:'foot',cue:'PAUSE'}};
    return {how:`Paused on <b>${PRESET_SCREEN[s.activity]||s.activity.toUpperCase()}</b> — tap <b>QUEST</b> (<kbd>Q</kbd>) once to preview the next drill.`,spot:{ctl:'quest',cue:'TAP'}};
   }
   if(stepId==='t-drill'){
    if(!isDrill(s.activity)){
     const taps=s.activity==='pet'?'TAP ×2':'TAP';
     return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) to return to <b>Guitar Drills</b>.',spot:{ctl:'mode',cue:taps}};
    }
    if(!s.running)return {how:`Tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to start <b>${PRESET_SCREEN[s.activity]||s.activity.toUpperCase()}</b>, or click <b>Play a clean round</b>.`,spot:{ctl:'foot',cue:'TAP'}};
    return {how:`<b>${PRESET_SCREEN[s.activity]||s.activity.toUpperCase()}</b> is running — click <b>▶ Play a clean round</b> below (earns a Personal Best + Quest time!).`,spot:null};
   }
   if(stepId==='t-quest'){
    const rem=Math.max(0,s.goal_minutes*60-Math.floor(s.session_ms/1000));
    return {how:`Keep playing until the gold ring reaches <b>5:00</b> (+1 Fragment) — <b>${fmtClock(s.session_ms)} / ${s.goal_minutes}:00</b> (${rem}s left).`,spot:!s.running?{ctl:'foot',cue:'TAP'}:null};
   }
   if(stepId==='t-save'){
    if(seen.save&&!seen.rebootAfterSave){
     return {how:'Saved to EEPROM! Now click <b>Reboot</b> in the top bar (or <b>▶ Reboot pedal</b> below) to verify your Fragment &amp; Personal Bests reload.',spot:null};
    }
    if(s.tuning)return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) once to leave the Tuner before saving.',spot:{ctl:'mode',cue:'TAP'}};
    if(s.running)return {how:'Tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to pause first.',spot:{ctl:'foot',cue:'PAUSE'}};
    return {how:'Hold <b>MODE</b> (<kbd>M</kbd>) for <b>1.2 seconds</b> until <i>Saved - safe to unplug</i> appears.',spot:{ctl:'mode',cue:'HOLD 1.2s'}};
   }
   if(tr.drill&&stepId===`${tr.id}-open`){
    const g=tr.drill;
    if(s.activity==='pet')return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) twice to open <b>Guitar Drills</b>.',spot:{ctl:'mode',cue:'TAP ×2'}};
    if(s.tuning)return {how:'Tap <b>MODE</b> (<kbd>M</kbd>) once more to open <b>Guitar Drills</b>.',spot:{ctl:'mode',cue:'TAP'}};
    if(s.running)return {how:'A drill is running — tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to pause first so <b>QUEST</b> can switch drills.',spot:{ctl:'foot',cue:'PAUSE'}};
    const info=questTapsTo(s.activity,g.slugs);
    if(info&&info.taps>0){
     const word=info.taps===1?'1 more time':`${info.taps} more times`;
     return {
      how:`Tap <b>QUEST</b> (<kbd>Q</kbd>) <b>${word}</b> until the screen shows <b>${info.screen}</b>.`,
      spot:{ctl:'quest',cue:info.taps===1?'TAP':`TAP ×${info.taps}`}
     };
    }
    return {how:`Tap <b>QUEST</b> (<kbd>Q</kbd>) until <b>${g.label}</b> appears.`,spot:{ctl:'quest',cue:'TAP'}};
   }
   if(tr.drill&&stepId===`${tr.id}-start`){
    return {how:'Tap <b>FOOTSWITCH</b> (<kbd>Space</kbd>) to start the round.',spot:{ctl:'foot',cue:'TAP'}};
   }
   if(tr.drill&&stepId===`${tr.id}-play`){
    if(!s.running)return {how:`${tr.drill.playDesc} <b>(Paused — tap <kbd>Space</kbd> or click any ▶ Play button below.)</b>`,spot:{ctl:'foot',cue:'RESUME'}};
    return {how:tr.drill.playDesc,spot:null};
   }
   return {how:'',spot:null};
  }

  function evaluate(){
   const s=api.getState();
   if(!s)return;

   if(s.stage===4&&s.tuning)seen.tuner=true;
   if(s.stage===4&&isDrill(s.activity)){
    if(s.activity!=='pulse-8th'||(prevDrillSlug&&prevDrillSlug!==s.activity))seen.presetStepped=true;
    prevDrillSlug=s.activity;
   }
   if(s.stage===4&&isDrill(s.activity)&&s.drill_rounds>=1&&s.drill_best>0)seen.tourDrillWon=true;
   if(s.stage===4&&(s.fragments>base.fragments||(s.calibrated&&s.fragments>=1&&s.session_ms>=s.goal_minutes*60000))){
    seen.tourFragment=true;
   }
   if(s.stage===4&&!s.tuning&&!s.running&&(s.saved_slot>=0||s.skills_slot>=0)&&(s.notice==='Saved - safe to unplug'||s.notice==='Already saved')){
    seen.save=true;
   }

   // Evaluate First Run (6 steps)
   const tour=doneByTrack.get('tour');
   if(s.stage===4&&(s.calibrated||seen.rebootAfterSave))tour.add('t-cal');
   if(seen.tuner)tour.add('t-tuner');
   if(seen.presetStepped){tour.add('t-cal');tour.add('t-pick');}
   if(seen.tourDrillWon){tour.add('t-cal');tour.add('t-pick');tour.add('t-drill');}
   if(seen.tourFragment)tour.add('t-quest');
   if(seen.save&&seen.rebootAfterSave&&!s.running&&!s.calibrated&&(s.saved_slot>=0||s.skills_slot>=0)){
    tour.add('t-save');
   }

   // Evaluate each of the 5 Drill guides (4 steps each)
   for(const tr of TRACKS){
    if(!tr.drill)continue;
    const g=tr.drill;
    const set=doneByTrack.get(tr.id);
    set.clear();
    const inFam=s.stage===4&&s.activity.startsWith(g.prefix);
    if(inFam&&s.running){
     startedFamilies.add(tr.id);
     if(familyRoundsAtOpen[tr.id]===undefined)familyRoundsAtOpen[tr.id]=s.drill_rounds;
    }
    if(!inFam)startedFamilies.delete(tr.id);
    const baseRounds=familyRoundsAtOpen[tr.id]!==undefined?familyRoundsAtOpen[tr.id]:0;
    if(inFam&&(s.drill_rounds>baseRounds||(s.drill_best>0&&s.drill_rounds>=1))){
     wonFamilies.add(tr.id);
    }

    if(s.stage===4&&s.calibrated)set.add(`${g.id}-cal`);
    if(s.stage===4&&s.calibrated&&inFam)set.add(`${g.id}-open`);
    if(s.stage===4&&s.calibrated&&inFam&&(s.running||startedFamilies.has(tr.id)))set.add(`${g.id}-start`);
    if(s.stage===4&&s.calibrated&&inFam&&wonFamilies.has(tr.id))set.add(`${g.id}-play`);
   }

   const tr=activeTrack();
   const doneSet=doneByTrack.get(tr.id);
   let activeIdx=tr.steps.findIndex(st=>!doneSet.has(st.id));
   // On the 4-step drill guides, keep Step 4 (the playground) focused even after it ticks ✓!
   const allDone=activeIdx===-1;
   if(allDone&&tr.drill)activeIdx=tr.steps.length-1;

   const curStep=activeIdx>=0?tr.steps[activeIdx]:null;
   const guide=curStep?guidanceForCurrentStep(tr,curStep.id,s):{how:'',spot:null};
   updatePedalCaptions(s,guide.spot);

   const scoreJson=tr.drill?JSON.stringify(tr.drill.scores(s)):'';
   const sig=`${tr.id}|${activeIdx}|${allDone}|${peekStepId}|${s.activity}|${+s.running}|${+s.calibrated}|${+s.calibrating}|${guide.how}|${scoreJson}|`+
             TRACKS.map(t=>`${t.id}:${doneByTrack.get(t.id).size}`).join(',');
   if(sig===lastSig)return;
   lastSig=sig;

   if(allDone){
    statusEl.textContent=`All ${tr.steps.length} steps complete ✓`;
    statusEl.className='sub ok';
   }else{
    statusEl.textContent=`Step ${activeIdx+1} of ${tr.steps.length}`;
    statusEl.className='sub';
   }

   const tracksEl=document.getElementById('session-tracks');
   if(tracksEl){
    for(const btn of tracksEl.querySelectorAll('.guide-tab')){
     const tid=btn.dataset.track,tDef=TRACKS.find(x=>x.id===tid),tDone=doneByTrack.get(tid);
     btn.classList.toggle('sel',tid===tr.id);
     btn.classList.toggle('done',tDone.size===tDef.steps.length);
     btn.setAttribute('aria-selected',tid===tr.id?'true':'false');
    }
   }

   for(let i=0;i<tr.steps.length;i++){
    const st=tr.steps[i],row=container.children[i];
    if(!row)continue;
    const done=doneSet.has(st.id);
    const cur=i===activeIdx;
    const upcoming=!done&&!cur;
    row.classList.toggle('done',done);
    row.classList.toggle('current',cur);
    row.classList.toggle('upcoming',upcoming);
    row.classList.toggle('peek',!cur&&peekStepId===st.id);
    row.dataset.done=done?'1':'0';
    row.querySelector('.step-mark').textContent=done?'✓':String(i+1);
    const tag=row.querySelector('.step-tag');
    if(tag){
     tag.textContent=cur?(done?'Ready to test':'Current step'):(done?'Done':'');
    }
    const howEl=row.querySelector('.step-how');
    if(howEl){
     const html=cur&&guide.how?guide.how:st.how;
     if(howEl.innerHTML!==html)howEl.innerHTML=html;
    }
    if(st.playground&&tr.drill){
     updatePlayground(row,tr.drill,s);
    }else if(st.id==='t-save'){
     const actBtn=row.querySelector('.step-do');
     if(actBtn)actBtn.textContent=seen.save&&!seen.rebootAfterSave?'▶ Reboot pedal':'Save & Reboot for me';
    }
   }

   const banner=document.getElementById('session-done-banner');
   if(banner){
    if(allDone&&!tr.drill){
     banner.hidden=false;
     banner.textContent='✓ First run complete! Click Timing, Note Finder, Bend & Vibrato, Scale Run, or Dynamics above to test each guitar drill.';
    }else{
     banner.hidden=true;
    }
   }
  }

  function buildPlaygroundDom(mount,g){
   const box=document.createElement('div');
   box.className='bench-box';

   const modeRow=document.createElement('div');
   modeRow.className='bench-group';
   const modeLbl=document.createElement('span');
   modeLbl.className='bench-lbl';
   modeLbl.textContent='Mode';
   modeRow.appendChild(modeLbl);
   for(const m of g.modes){
    const b=document.createElement('button');
    b.type='button';
    b.className='pill';
    b.dataset.preset=m.slug;
    b.textContent=m.label;
    b.addEventListener('click',e=>{e.stopPropagation();switchPresetClean(m.slug,true);});
    modeRow.appendChild(b);
   }
   box.appendChild(modeRow);

   const takeRow=document.createElement('div');
   takeRow.className='bench-group';
   const takeLbl=document.createElement('span');
   takeLbl.className='bench-lbl';
   takeLbl.textContent='Try playing';
   takeRow.appendChild(takeLbl);
   for(const t of g.takes(null)){
    const b=document.createElement('button');
    b.type='button';
    b.className=t.primary?'btn-primary':'';
    b.dataset.take=t.kind;
    b.textContent=t.label;
    b.addEventListener('click',e=>{e.stopPropagation();runDrillTake(t.kind);});
    takeRow.appendChild(b);
   }
   box.appendChild(takeRow);

   const strip=document.createElement('div');
   strip.className='score-strip';
   box.appendChild(strip);
   mount.appendChild(box);
  }

  function updatePlayground(row,g,s){
   for(const b of row.querySelectorAll('[data-preset]')){
    b.classList.toggle('sel',s.activity===b.dataset.preset);
   }
   const takes=g.takes(s);
   for(const t of takes){
    const b=row.querySelector(`[data-take="${t.kind}"]`);
    if(b&&b.textContent!==t.label)b.textContent=t.label;
   }
   const strip=row.querySelector('.score-strip');
   if(strip){
    const scores=g.scores(s);
    while(strip.children.length<scores.length){
     const chip=document.createElement('span');
     chip.className='score-chip';
     chip.innerHTML='<span></span>: <b></b>';
     strip.appendChild(chip);
    }
    for(let i=0;i<scores.length;i++){
     const sc=scores[i],chip=strip.children[i];
     chip.className='score-chip'+(sc.hi?' hi':'');
     chip.firstElementChild.textContent=sc.k;
     chip.lastElementChild.textContent=sc.v;
    }
   }
  }

  function buildTrackBar(){
   const tracksEl=document.getElementById('session-tracks');
   if(!tracksEl)return;
   tracksEl.innerHTML='';
   for(const t of TRACKS){
    const b=document.createElement('button');
    b.type='button';
    b.role='tab';
    b.className='guide-tab'+(t.id===activeTrackId?' sel':'');
    b.dataset.track=t.id;
    b.innerHTML=`<span>${t.label}</span><i class="dot" aria-hidden="true"></i>`;
    b.addEventListener('click',()=>setTrack(t.id));
    tracksEl.appendChild(b);
   }
  }

  function buildDom(){
   const tr=activeTrack();
   container.innerHTML='';
   tr.steps.forEach((st,i)=>{
    const row=document.createElement('div');
    row.className='step-row';
    row.dataset.step=st.id;
    const head=document.createElement('div');
    head.className='step-head';
    head.innerHTML=
     `<span class="step-mark" aria-hidden="true">${i+1}</span>`+
     `<span class="step-title">${st.title}</span>`+
     `<span class="step-tag"></span>`;
    head.addEventListener('click',()=>{
     peekStepId=peekStepId===st.id?'':st.id;
     lastSig='';
     evaluate();
    });

    const detail=document.createElement('div');
    detail.className='step-detail';
    detail.innerHTML=`<p class="step-how">${st.how}</p>`;

    if(st.playground&&tr.drill){
     const mount=document.createElement('div');
     mount.className='bench-mount';
     buildPlaygroundDom(mount,tr.drill);
     detail.appendChild(mount);
    }else if(st.btn){
     const acts=document.createElement('div');
     acts.className='step-actions';
     const btn=document.createElement('button');
     btn.type='button';
     btn.className='step-do'+(st.btnPrimary?' btn-primary':' btn-gold');
     btn.dataset.do=st.id;
     btn.textContent=st.btn;
     btn.addEventListener('click',e=>{e.stopPropagation();doStep(st.id);});
     acts.appendChild(btn);
     detail.appendChild(acts);
    }

    row.appendChild(head);
    row.appendChild(detail);
    container.appendChild(row);
   });
  }

  buildTrackBar();
  buildDom();
  return {
   TRACKS,
   evaluate,
   resetProgress,
   onReboot,
   doStep,
   setTrack,
   runDrillTake,
   getActiveTrack:()=>activeTrackId,
   getCompleted:()=>Array.from(doneByTrack.get(activeTrackId))
  };
 }

 root.VexSession={createSession,TRACKS,fmtBest};
})(typeof globalThis!=='undefined'?globalThis:window);
