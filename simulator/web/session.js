// Multi-track guided walkthrough (MakeCode-style self-ticking steps).
// Covers the full Quest + Drills pedal flow AND dedicated step-by-step walkthroughs for each
// of the five drills (PULSE, HUNT, BEND, RUN, TOUCH) plus Classic Pet & Tuner.
// Every step watches real vex::App state so it ticks whether performed by hand on the pedal
// (mouse or keys M, Q, Space) or via "Do it for me".
(function(root){
 'use strict';

 const PC=['C','C#','D','D#','E','F','F#','G','G#','A','Bb','B'];
 const STR_NAMES=['E6 STR','A STR','D STR','G STR','B STR','E1 STR'];
 function isDrill(act){return !!act&&act!=='pet'&&act!=='tuner';}
 function fmtBest(act,v){
  if(!v)return 'none yet';
  if(act.startsWith('pulse')||act.startsWith('run'))return `${v} BPM`;
  if(act.startsWith('hunt'))return `${(v/100).toFixed(2)} s/note`;
  if(act.startsWith('bend'))return `${(v/10).toFixed(1)} c`;
  if(act.startsWith('touch'))return `${(v/100).toFixed(2)} dB`;
  return String(v);
 }

 const TRACKS=[
  {
   id:'tour',
   label:'Quest + Drills Tour',
   summary:'Full 3-button pedal loop: calibrate → enter Drills (MODE ×2) → pick preset (QUEST) → play a drill (earns both a Personal Best and Quest time!) → complete 5-min Quest → save & reboot.',
   steps:[
    {
     id:'plug',
     title:'Plug in (quiet cable)',
     how:'Keep Guitar input on Not playing (strings muted on buffered dry branch).'
    },
    {
     id:'calibrate',
     title:'Calibrate noise floor (hold QUEST 1.2 s)',
     how:'Hold QUEST (key Q) for 1.2 s while muted, then wait 3 s until screen reads Ready: tap footswitch.'
    },
    {
     id:'enter-drill',
     title:'Switch screen to Guitar Drills (MODE × 2)',
     how:'Tap MODE (key M) twice: Pet → Tuner → Drill. You land on PULSE 8TH (paused).'
    },
    {
     id:'cycle-preset',
     title:'Cycle drill presets (tap QUEST while paused)',
     how:'While paused on Drill, tap QUEST (key Q) to step presets (or click PULSE QTR / any preset pill below).'
    },
    {
     id:'start-drill',
     title:'Start the drill (tap FOOTSWITCH)',
     how:'Tap FOOTSWITCH (key Space) to start. On PULSE, watch the amber LED flash the 4/4 beat; QUEST locks while running.'
    },
    {
     id:'win-drill',
     title:'Play a clean drill round (Personal Best + Quest time)',
     how:'Click Clean take in Guitar drills (or play notes). VEX saves a NEW PERSONAL BEST to Skills EEPROM AND credits your Quest timer!'
    },
    {
     id:'fragment',
     title:'Finish the 5-minute Quest (+1 Fragment)',
     how:'Playing on Pet OR any Drill earns Quest time. Reach 5:00 (use 100× speed or Advance 5 min) for +1 Fragment.'
    },
    {
     id:'save-all',
     title:'Check Tuner, then Pause & Save (hold MODE 1.2 s)',
     how:'Tap MODE to check Tuner (credit pauses), tap MODE to Pet or Drill, pause if running (FOOTSWITCH), then hold MODE 1.2 s.'
    },
    {
     id:'replug',
     title:'Unplug & replug (Reboot)',
     how:'Click Reboot — both your Quest fragments (EEPROM 0/64) and Drill personal bests (EEPROM 128/192) restore cleanly.'
    }
   ]
  },
  {
   id:'pulse',
   label:'Timing (PULSE)',
   summary:'PULSE checks picked note attacks against a 4-bar LED beat ladder (+4 / −4 BPM, ±30 ms pass window) or your own spacing in PULSE FREE.',
   steps:[
    {
     id:'p-open',
     title:'Open PULSE QTR (MODE → Drill, QUEST → preset)',
     how:'Calibrate if needed, tap MODE to Drill, and select PULSE QTR while paused (starts at 92 BPM, or 8 BPM under saved best).'
    },
    {
     id:'p-start',
     title:'Start the 4/4 LED beat (tap FOOTSWITCH)',
     how:'Tap FOOTSWITCH (Space). The amber LED flashes longer on beat 1 (75 ms) and shorter on beats 2–4 (32 ms).'
    },
    {
     id:'p-steady',
     title:'Play 4 clean bars (+4 BPM & NEW PERSONAL BEST)',
     how:'Click Clean 4-bar take in Guitar drills. All 16 quarter notes land within ±30 ms: tempo steps up +4 BPM and saves best = 92 BPM.'
    },
    {
     id:'p-rush',
     title:'Try a rushing / uneven take (red early/late dots)',
     how:'Click Rushing take or Uneven take. Dots plot outside ±30 ms in red and report RUSHING / SPREAD ms (two misses step −4 BPM).'
    },
    {
     id:'p-free',
     title:'Try PULSE FREE (self-paced / external metronome)',
     how:'Pause (FOOTSWITCH), select PULSE FREE (LED beat turns off), tap FOOTSWITCH, and play steady notes — VEX infers your BPM from spacing.'
    }
   ]
  },
  {
   id:'hunt',
   label:'Note Finder (HUNT)',
   summary:'HUNT prompts notes across 10-note seeded rounds — anywhere on the neck (HUNT ANY) or on a named string verified by exact MIDI pitch (HUNT STR) — and repeats slow notes.',
   steps:[
    {
     id:'h-open',
     title:'Open HUNT STR (string-specific pitch check)',
     how:'Calibrate if needed, enter Drill (MODE), and select HUNT STR while paused.'
    },
    {
     id:'h-start',
     title:'Start the 10-note round (tap FOOTSWITCH)',
     how:'Tap FOOTSWITCH (Space). Read the big target note and gold string prompt (e.g. F# on E6 STR = F#2).'
    },
    {
     id:'h-wrong',
     title:'Play the wrong string/octave (red rejection)',
     how:'Click Wrong string / note (plays F#3 on D string when F#2 on E6 is asked). Screen flashes HEARD F#3 - TRY AGAIN in red.'
    },
    {
     id:'h-win',
     title:'Find all 10 notes (NEW PERSONAL BEST & slow-note repeat)',
     how:'Click Find all 10 notes. VEX times every find, saves best avg seconds/note, and brings slow notes back sooner next round.'
    },
    {
     id:'h-any',
     title:'Switch to HUNT ANY (any octave accepted)',
     how:'Pause (FOOTSWITCH), select HUNT ANY, tap FOOTSWITCH, and play the target pitch class in any octave.'
    }
   ]
  },
  {
   id:'bend',
   label:'Bend & Vibrato (BEND)',
   summary:'BEND uses the fast 11.6 ms pitch profile (≥140 Hz) to score half-step (+100 c) and whole-step (+200 c) bends AND measure live vibrato rate & width.',
   steps:[
    {
     id:'b-open',
     title:'Open BEND 1/2 (+100 c half-step target)',
     how:'Calibrate if needed, enter Drill (MODE), and select BEND 1/2 while paused.'
    },
    {
     id:'b-vib',
     title:'Start & hold a note with Vibrato (rate + width)',
     how:'Tap FOOTSWITCH (Space), then click Steady E4 vibrato. Screen shows live VIB 5.6HZ 41C beside TYP 5-7HZ 25-70C without counting a bend.'
    },
    {
     id:'b-flat',
     title:'Try an under-bent note (flat cents error)',
     how:'Click Under-bent (−32 c flat). A red dot marks the flat apex below the gold target line and shows LAST −25 CENTS.'
    },
    {
     id:'b-win',
     title:'Complete 8 clean bends (median error Personal Best)',
     how:'Click 8 clean target bends. Fills the 8-bend history bar in gold and saves median |error| (0.1 c) to Skills EEPROM.'
    },
    {
     id:'b-whole',
     title:'Switch to BEND FULL (+200 c whole-step bend)',
     how:'Pause (FOOTSWITCH), select BEND FULL, tap FOOTSWITCH, and bend up a full whole tone (+200 c).'
    }
   ]
  },
  {
   id:'run',
   label:'Scale Runs (RUN)',
   summary:'RUN sets your key root from the first note of each 16-note phrase, lights 12 chromatic dots gold (in key) or red (outside), tracks hammer-ons, and saves clean tempo.',
   steps:[
    {
     id:'r-open',
     title:'Open RUN PENTA (minor pentatonic)',
     how:'Calibrate if needed, enter Drill (MODE), and select RUN PENTA while paused.'
    },
    {
     id:'r-start',
     title:'Start phrase (FOOTSWITCH — first note sets root)',
     how:'Tap FOOTSWITCH (Space). Screen reads FIRST NOTE = ROOT (16 NOTES).'
    },
    {
     id:'r-clean',
     title:'Play a clean 16-note scale run (NEW PERSONAL BEST)',
     how:'Click Clean 16-note scale run (starts on A2 → sets A MIN PENTA, flashes scale degrees gold, saves CLEAN 112 BPM).'
    },
    {
     id:'r-out',
     title:'Play outside notes (red flash & best protected)',
     how:'Click Out-of-key notes (red). Chromatic outside notes flash red, increment the OUT count, and leave your clean best intact.'
    },
    {
     id:'r-diatonic',
     title:'Try RUN MAJOR or RUN MINOR (7-note scales)',
     how:'Pause (FOOTSWITCH), select RUN MAJOR or RUN MINOR, tap FOOTSWITCH, and run a 16-note diatonic phrase.'
    }
   ]
  },
  {
   id:'touch',
   label:'Dynamics (TOUCH)',
   summary:'TOUCH gates pitch off (0 YIN work) and plots each note’s 36 ms attack crest in dB against your running median — for even picking or +4 dB accents every 3rd/4th note.',
   steps:[
    {
     id:'t-open',
     title:'Open TOUCH ACC3 (accent 1 of 3 ≥ +4 dB)',
     how:'Calibrate if needed, enter Drill (MODE), and select TOUCH ACC3 while paused.'
    },
    {
     id:'t-start',
     title:'Start 12-note round (tap FOOTSWITCH)',
     how:'Tap FOOTSWITCH (Space) to arm the 12-bar dB level graph.'
    },
    {
     id:'t-wrong',
     title:'Accent the wrong beat (beat 2 rejected)',
     how:'Click Accent on wrong beat. Bars plot against the gold median line and warn ACCENT BEAT 1 >= +4DB in red (no best saved).'
    },
    {
     id:'t-win',
     title:'Nail beat-1 accents (NEW PERSONAL BEST dB spread)',
     how:'Click Clean take (hits accents). Four tall gold bars rise ≥ +4 dB above unaccented picks and save your dB spread.'
    },
    {
     id:'t-even',
     title:'Try TOUCH EVEN (uniform picking consistency)',
     how:'Pause (FOOTSWITCH), select TOUCH EVEN, tap FOOTSWITCH, and play 12 matched notes for minimal dB spread.'
    }
   ]
  },
  {
   id:'pet',
   label:'Pet & Tuner (8)',
   summary:'Original 8-step Pet companion walkthrough: 40 ms smoothing, 500 ms chord-gap hold, participation Quest fragment, Tuner pause rule, and EEPROM save/reboot.',
   steps:[
    {id:'c-plug',title:'Plug in (quiet cable)',how:'Keep Guitar input on Not playing (buffered dry branch, strings muted).'},
    {id:'c-cal',title:'Calibrate noise floor',how:'Hold QUEST (key Q) for 1.2 s while muted, then wait 3 s for Ready.'},
    {id:'c-start',title:'Start the session',how:'On Pet screen, tap FOOTSWITCH (key Space) to start the 5-minute quest.'},
    {id:'c-play',title:'Play & watch the 500 ms hold',how:'Choose Strumming or Playing for 3 s — the 500 ms hold bridges chord gaps.'},
    {id:'c-frag',title:'Earn the quest fragment',how:'Reach the 5:00 goal (use 100× speed or Advance 5 min) for +1 fragment.'},
    {id:'c-tune',title:'Check tuning (credit pauses)',how:'Tap MODE (key M) to open the tuner; try Synth note to tune a string.'},
    {id:'c-save',title:'Pause & save to EEPROM',how:'Return to Pet or Drill (MODE), pause if running, then hold MODE 1.2 s.'},
    {id:'c-replug',title:'Unplug & replug',how:'Click Reboot — totals return from EEPROM; boot stays paused & uncalibrated.'}
   ]
  }
 ];

 // Backwards-compatible export of the classic 8 steps.
 const STEPS=TRACKS[0].steps;

 function createSession(container,statusEl,api){
  const doneByTrack=new Map(TRACKS.map(t=>[t.id,new Set()]));
  let activeTrackId=localStorage.getItem('vex.walk.track')||'tour';
  if(!TRACKS.some(t=>t.id===activeTrackId))activeTrackId='tour';

  let base={fragments:0,activeMs:0,onsets:0};
  let seen={
   tuner:false,save:false,rebootAfterSave:false,
   presetChanged:false,pulseRush:false,pulseFreeDone:false,
   huntWrong:false,huntAnyHit:false,
   bendVib:false,bendFlat:false,bendWholeHit:false,
   runOut:false,runDiatonicDone:false,
   touchWrong:false,touchEvenDone:false
  };
  let prev={drillSlug:'',huntPc:-1,huntMidi:-1,onsets:0,pulseRounds:0,pulseBpm:0,touchRounds:0},lastRenderedSig='';

  function activeTrack(){return TRACKS.find(t=>t.id===activeTrackId)||TRACKS[0];}

  function resetProgress(){
   for(const set of doneByTrack.values())set.clear();
   const s=api.getState();
   base={
    fragments:s?s.fragments:0,
    activeMs:s?s.active_ms:0,
    onsets:s?s.onsets:0
   };
   seen={
    tuner:false,save:false,rebootAfterSave:false,
    presetChanged:false,pulseRush:false,pulseFreeDone:false,
    huntWrong:false,huntAnyHit:false,
    bendVib:false,bendFlat:false,bendWholeHit:false,
    runOut:false,runDiatonicDone:false,
    touchWrong:false,touchEvenDone:false
   };
   prev={
    drillSlug:s&&isDrill(s.activity)?s.activity:'',
    huntPc:s?s.hunt_pc:-1,huntMidi:s?s.hunt_midi:-1,onsets:s?s.onsets:0,
    pulseRounds:s&&s.activity.startsWith('pulse')?s.drill_rounds:0,
    pulseBpm:s?s.bpm:0,
    touchRounds:s&&s.activity.startsWith('touch')?s.drill_rounds:0
   };
   lastRenderedSig='';
   evaluate();
  }

  function onReboot(){
   if(seen.save)seen.rebootAfterSave=true;
  }

  function setTrack(id){
   if(!TRACKS.some(t=>t.id===id))return;
   activeTrackId=id;
   try{localStorage.setItem('vex.walk.track',id);}catch(_){}
   lastRenderedSig='';
   buildDom();
   evaluate();
  }

  // Helpers that drive the real pedal controls / shared protocol cleanly -------------------------
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
  function ensurePreset(slug){
   ensureCalibrated();
   let s=ensureScreen('drill');
   if(s.running){api.press('foot',100);s=api.getState();}
   if(s.activity!==slug){
    api.selectDrill(slug);
    seen.presetChanged=true;
   }
   evaluate();
   return api.getState();
  }
  function ensurePresetRunning(slug){
   let s=ensurePreset(slug);
   if(!s.running)api.press('foot',100);
   evaluate();
   return api.getState();
  }
  function ensurePetRunning(){
   ensureCalibrated();
   let s=ensureScreen('pet');
   if(!s.running)api.press('foot',100);
   return api.getState();
  }

  function doStep(id){
   api.exitTestPattern();
   // --- Track 1: Quest + Drills Tour ---
   if(id==='plug'||id==='c-plug'){
    api.ensureStage4();api.setInputMode('quiet');api.advance(100);
   }else if(id==='calibrate'||id==='c-cal'){
    api.ensureStage4();ensureScreen('pet');api.setInputMode('quiet');api.advance(50);api.press('quest',1300);api.advance(3100);
   }else if(id==='enter-drill'){
    ensureCalibrated();ensureScreen('drill');
   }else if(id==='cycle-preset'){
    ensureCalibrated();
    let s=ensureScreen('drill');
    if(s.running){api.press('foot',100);s=api.getState();}
    // Step with real QUEST tap if on pulse-8th, then land on pulse-qtr so LED beat is ready for step 5!
    api.press('quest',100);
    api.selectDrill('pulse-qtr');
    seen.presetChanged=true;
   }else if(id==='start-drill'){
    let s=api.getState();
    const target=isDrill(s.activity)?s.activity:'pulse-qtr';
    ensurePresetRunning(target);
   }else if(id==='win-drill'){
    let s=api.getState();
    if(!isDrill(s.activity))ensurePresetRunning('pulse-qtr');
    else if(!s.running)api.press('foot',100);
    api.playDrill('steady');
   }else if(id==='fragment'||id==='c-frag'){
    let s=api.getState();
    if(!s.calibrated)ensureCalibrated();
    s=api.getState();
    if(s.tuning)s=ensureScreen('pet');
    if(!s.running)api.press('foot',100);
    s=api.getState();
    const goalMs=s.goal_minutes*60000;
    const remain=goalMs-s.session_ms;
    if(remain>1500){api.setInputMode('play');api.advance(remain-1000);}
    api.setInputMode('strum');api.advance(1500);
    api.setInputMode('quiet');
   }else if(id==='save-all'){
    ensureCalibrated();
    ensureScreen('tuner');seen.tuner=true;
    api.setInputMode('synth',{hz:440,cents:0,db:-20});api.advance(500);
    api.setInputMode('quiet');
    let s=ensureScreen('pet');
    if(s.running){api.press('foot',100);s=api.getState();}
    if(s.active_ms===0&&s.fragments===0){
     ensurePetRunning();api.setInputMode('play');api.advance(4000);api.setInputMode('quiet');api.press('foot',100);
    }
    api.press('mode',1350);
   }else if(id==='replug'||id==='c-replug'){
    if(!seen.save)doStep(id==='c-replug'?'c-save':'save-all');
    evaluate();
    api.setInputMode('quiet');
    api.reboot();
    api.advance(100);

   // --- Track 2: Timing (PULSE) ---
   }else if(id==='p-open'){
    ensurePreset('pulse-qtr');
   }else if(id==='p-start'){
    ensurePresetRunning('pulse-qtr');
   }else if(id==='p-steady'){
    ensurePresetRunning('pulse-qtr');api.playDrill('steady');
   }else if(id==='p-rush'){
    ensurePresetRunning('pulse-qtr');api.playDrill('rushing');
   }else if(id==='p-free'){
    ensurePresetRunning('pulse-free');api.playDrill('steady');

   // --- Track 3: Note Finder (HUNT) ---
   }else if(id==='h-open'){
    ensurePreset('hunt-str');
   }else if(id==='h-start'){
    ensurePresetRunning('hunt-str');
   }else if(id==='h-wrong'){
    ensurePresetRunning('hunt-str');api.playDrill('wrong');
   }else if(id==='h-win'){
    ensurePresetRunning('hunt-str');api.playDrill('steady');
   }else if(id==='h-any'){
    ensurePresetRunning('hunt-any');api.playDrill('rushing');

   // --- Track 4: Bend & Vibrato (BEND) ---
   }else if(id==='b-open'){
    ensurePreset('bend-half');
   }else if(id==='b-vib'){
    ensurePresetRunning('bend-half');api.playDrill('rushing');
   }else if(id==='b-flat'){
    ensurePresetRunning('bend-half');api.playDrill('wrong');
   }else if(id==='b-win'){
    ensurePresetRunning('bend-half');api.playDrill('steady');
   }else if(id==='b-whole'){
    ensurePresetRunning('bend-whole');api.playDrill('steady');

   // --- Track 5: Scale Runs (RUN) ---
   }else if(id==='r-open'){
    ensurePreset('run-penta');
   }else if(id==='r-start'){
    ensurePresetRunning('run-penta');
   }else if(id==='r-clean'){
    ensurePresetRunning('run-penta');api.playDrill('steady');
   }else if(id==='r-out'){
    ensurePresetRunning('run-penta');api.playDrill('wrong');
   }else if(id==='r-diatonic'){
    ensurePresetRunning('run-major');api.playDrill('steady');

   // --- Track 6: Dynamics (TOUCH) ---
   }else if(id==='t-open'){
    ensurePreset('touch-acc3');
   }else if(id==='t-start'){
    ensurePresetRunning('touch-acc3');
   }else if(id==='t-wrong'){
    ensurePresetRunning('touch-acc3');api.playDrill('wrong');
   }else if(id==='t-win'){
    ensurePresetRunning('touch-acc3');api.playDrill('steady');
   }else if(id==='t-even'){
    ensurePresetRunning('touch-even');api.playDrill('steady');

   // --- Track 7: Classic Pet & Tuner ---
   }else if(id==='c-start'){
    ensurePetRunning();
   }else if(id==='c-play'){
    ensurePetRunning();api.setInputMode('strum');api.advance(3500);
   }else if(id==='c-tune'){
    api.ensureStage4();ensureScreen('tuner');
    api.setInputMode('synth',{hz:440,cents:0,db:-20});api.advance(600);
   }else if(id==='c-save'){
    api.ensureStage4();
    let s=ensureScreen('pet');
    if(s.running){api.press('foot',100);s=api.getState();}
    if(s.active_ms===0&&s.fragments===0){
     ensurePetRunning();api.setInputMode('play');api.advance(6500);api.press('foot',100);
    }
    api.press('mode',1350);
   }
   evaluate();
  }

  function liveHintForStep(stepId,s){
   if(!s)return '';
   const scr=s.activity==='pet'?'PET':s.tuning?'TUNER':`DRILL (${s.activity.toUpperCase()})`;
   const runTxt=s.running?'running':'paused';
   if(stepId==='calibrate'||stepId==='c-cal'){
    if(s.calibrating)return 'Live: calibrating noise floor… keep strings muted for 3 s';
    return `Live: ${scr} · uncalibrated — hold QUEST (key Q) for 1.2 s`;
   }
   if(stepId==='enter-drill'){
    if(s.activity==='pet')return 'Live: on PET — tap MODE (key M) twice to reach DRILL';
    if(s.tuning)return 'Live: on TUNER — tap MODE (key M) once more to reach DRILL';
   }
   if(stepId==='cycle-preset'){
    if(!isDrill(s.activity))return `Live: on ${scr} — tap MODE to enter DRILL first`;
    if(s.running)return `Live: ${s.activity.toUpperCase()} is running — tap FOOTSWITCH to pause before tapping QUEST`;
    return `Live: ${s.activity.toUpperCase()} · paused — tap QUEST (key Q) to cycle presets`;
   }
   if(stepId==='start-drill'||stepId.endsWith('-start')){
    return `Live: ${scr} · ${runTxt} — tap FOOTSWITCH (key Space) to start`;
   }
   if(s.activity.startsWith('hunt')&&s.running){
    const str=s.hunt_midi>0?STR_NAMES[['40','45','50','55','59','64'].findIndex((_,i)=>s.hunt_midi>=[40,45,50,55,59,64][i]&&s.hunt_midi<[40,45,50,55,59,64][i]+12)]||'STR':'ANY STRING';
    return `Live: target ${PC[s.hunt_pc]||'?'} (${str}) · rounds ${s.drill_rounds} · best ${fmtBest(s.activity,s.drill_best)}`;
   }
   if(s.activity.startsWith('bend')&&s.running){
    return `Live: bends ${s.bend_count}/8 · last ${s.bend_err>=0?'+':''}${s.bend_err} c · vib ${(s.vib_rate_x10/10).toFixed(1)} Hz / ${s.vib_width} c · best ${fmtBest(s.activity,s.drill_best)}`;
   }
   if(s.activity.startsWith('run')&&s.running){
    const root=s.run_root>=0?PC[s.run_root]:'unset';
    return `Live: root ${root} · ${s.bpm||0} BPM · ${s.run_outside} outside · best ${fmtBest(s.activity,s.drill_best)}`;
   }
   if(isDrill(s.activity)){
    return `Live: ${s.activity.toUpperCase()} · ${runTxt}${s.bpm?` · ${s.bpm} BPM`:''} · rounds ${s.drill_rounds} · best ${fmtBest(s.activity,s.drill_best)} · quest ${Math.floor(s.session_ms/1000)}s/${s.goal_minutes*60}s`;
   }
   return `Live: ${scr} · ${runTxt} · quest ${Math.floor(s.session_ms/1000)}s/${s.goal_minutes*60}s · fragments ${s.fragments}`;
  }

  function evaluate(){
   const s=api.getState();
   if(!s)return;
   const mode=api.getInputMode();

   // Latching observations so quick manual actions never miss a step
   if(s.stage===4&&s.tuning)seen.tuner=true;
   if(s.stage===4&&isDrill(s.activity)){
    if(s.activity!=='pulse-8th'||(prev.drillSlug&&prev.drillSlug!==s.activity))seen.presetChanged=true;
    prev.drillSlug=s.activity;
   }
   if(s.stage===4&&!s.tuning&&!s.running&&(s.saved_slot>=0||s.skills_slot>=0)&&(s.notice==='Saved - safe to unplug'||s.notice==='Already saved')){
    seen.save=true;
   }
   if(s.activity.startsWith('pulse')){
    if(s.drill_rounds>prev.pulseRounds&&s.activity!=='pulse-free'&&s.bpm<=prev.pulseBpm)seen.pulseRush=true;
    prev.pulseRounds=s.drill_rounds;prev.pulseBpm=s.bpm;
   }
   if(s.activity==='pulse-free'&&s.drill_rounds>=1&&s.bpm>=40)seen.pulseFreeDone=true;
   if(s.activity.startsWith('hunt')&&s.running){
    if(s.onsets>prev.onsets&&s.hunt_pc===prev.huntPc&&s.hunt_midi===prev.huntMidi)seen.huntWrong=true;
    if(s.activity==='hunt-any'&&prev.huntPc>=0&&(s.hunt_pc!==prev.huntPc||s.drill_rounds>0))seen.huntAnyHit=true;
    prev.huntPc=s.hunt_pc;prev.huntMidi=s.hunt_midi;
   }
   if(s.activity.startsWith('bend')&&s.vib_rate_x10>=29&&s.vib_width>=12)seen.bendVib=true;
   if(s.activity.startsWith('bend')&&(s.bend_count>=1||s.drill_rounds>=1)&&Math.abs(s.bend_err)>=15)seen.bendFlat=true;
   if(s.activity==='bend-whole'&&(s.drill_rounds>=1||(s.bend_count>=1&&Math.abs(s.bend_err)<=15)))seen.bendWholeHit=true;
   if(s.activity.startsWith('run')&&s.run_outside>=1)seen.runOut=true;
   if((s.activity==='run-major'||s.activity==='run-minor')&&s.drill_rounds>=1&&s.run_outside===0)seen.runDiatonicDone=true;
   if(s.activity.startsWith('touch')){
    if(s.drill_rounds>prev.touchRounds&&(s.drill_best===0||s.drill_last>220||s.drill_last>s.drill_best))seen.touchWrong=true;
    prev.touchRounds=s.drill_rounds;
   }
   if(s.activity==='touch-even'&&s.drill_rounds>=1&&s.drill_best>0)seen.touchEvenDone=true;
   prev.onsets=s.onsets;

   const tour=doneByTrack.get('tour');
   if(s.stage===4&&!s.calibrating&&mode==='quiet'&&s.now_ms>=50&&s.rms<=0.02)tour.add('plug');
   if(s.stage===4&&s.calibrated){tour.add('plug');tour.add('calibrate');}
   if(s.stage===4&&isDrill(s.activity))tour.add('enter-drill');
   if(s.stage===4&&(seen.presetChanged||(isDrill(s.activity)&&s.activity!=='pulse-8th')))tour.add('cycle-preset');
   if(s.stage===4&&isDrill(s.activity)&&s.running)tour.add('start-drill');
   if(s.stage===4&&s.skills_slot>=0&&(s.active_ms>base.activeMs||s.session_ms>0)){tour.add('enter-drill');tour.add('start-drill');tour.add('win-drill');}
   if(s.stage===4&&(s.fragments>base.fragments||(s.calibrated&&s.fragments>=1&&s.session_ms>=s.goal_minutes*60000)))tour.add('fragment');
   if(seen.save)tour.add('save-all');
   if(seen.save&&seen.rebootAfterSave&&s.stage===4&&!s.running&&!s.calibrated&&(s.saved_slot>=0||s.skills_slot>=0))tour.add('replug');

   const pulse=doneByTrack.get('pulse');
   if(s.activity==='pulse-qtr'&&s.calibrated)pulse.add('p-open');
   if(s.activity.startsWith('pulse')&&s.running&&s.beat_ms>0){pulse.add('p-open');pulse.add('p-start');}
   if(s.activity.startsWith('pulse')&&s.drill_best>0&&s.drill_rounds>=1){pulse.add('p-open');pulse.add('p-start');pulse.add('p-steady');}
   if(seen.pulseRush)pulse.add('p-rush');
   if(seen.pulseFreeDone||(s.activity==='pulse-free'&&s.running&&s.bpm>=40))pulse.add('p-free');

   const hunt=doneByTrack.get('hunt');
   if(s.activity==='hunt-str'&&s.calibrated)hunt.add('h-open');
   if(s.activity==='hunt-str'&&s.running){hunt.add('h-open');hunt.add('h-start');}
   if(seen.huntWrong)hunt.add('h-wrong');
   if(s.activity.startsWith('hunt')&&s.drill_best>0&&s.drill_rounds>=1){hunt.add('h-open');hunt.add('h-start');hunt.add('h-win');}
   if(seen.huntAnyHit||(s.activity==='hunt-any'&&s.running&&s.onsets>base.onsets))hunt.add('h-any');

   const bend=doneByTrack.get('bend');
   if(s.activity==='bend-half'&&s.calibrated)bend.add('b-open');
   if(seen.bendVib){bend.add('b-open');bend.add('b-vib');}
   if(seen.bendFlat)bend.add('b-flat');
   if(s.activity.startsWith('bend')&&s.drill_best>0&&s.drill_rounds>=1){bend.add('b-open');bend.add('b-win');}
   if(seen.bendWholeHit||(s.activity==='bend-whole'&&s.bend_count>=1))bend.add('b-whole');

   const run=doneByTrack.get('run');
   if(s.activity==='run-penta'&&s.calibrated)run.add('r-open');
   if(s.activity.startsWith('run')&&s.running){run.add('r-open');run.add('r-start');}
   if(s.activity.startsWith('run')&&s.drill_best>0&&s.drill_rounds>=1&&s.run_outside===0){run.add('r-open');run.add('r-start');run.add('r-clean');}
   if(seen.runOut)run.add('r-out');
   if(seen.runDiatonicDone)run.add('r-diatonic');

   const touch=doneByTrack.get('touch');
   if(s.activity==='touch-acc3'&&s.calibrated)touch.add('t-open');
   if(s.activity.startsWith('touch')&&s.running){touch.add('t-open');touch.add('t-start');}
   if(seen.touchWrong)touch.add('t-wrong');
   if(s.activity.startsWith('touch')&&s.drill_best>0&&s.drill_rounds>=1){touch.add('t-open');touch.add('t-start');touch.add('t-win');}
   if(seen.touchEvenDone)touch.add('t-even');

   const pet=doneByTrack.get('pet');
   if(s.stage===4&&!s.calibrating&&mode==='quiet'&&s.now_ms>=50&&s.rms<=0.02)pet.add('c-plug');
   if(s.stage===4&&s.calibrated){pet.add('c-plug');pet.add('c-cal');}
   if(s.stage===4&&s.calibrated&&s.running)pet.add('c-start');
   if(s.stage===4&&((s.active_ms-base.activeMs)>=2500||(s.running&&s.active&&s.session_ms>=2500))){pet.add('c-start');pet.add('c-play');}
   if(s.stage===4&&(s.fragments>base.fragments||(s.calibrated&&s.fragments>=1&&s.session_ms>=s.goal_minutes*60000))){pet.add('c-play');pet.add('c-frag');}
   if(seen.tuner)pet.add('c-tune');
   if(seen.save)pet.add('c-save');
   if(seen.save&&seen.rebootAfterSave&&s.stage===4&&!s.running&&!s.calibrated&&s.saved_slot>=0&&s.sequence>=1)pet.add('c-replug');

   render(s);
  }

  function render(s){
   const tr=activeTrack(),doneSet=doneByTrack.get(tr.id);
   const activeIdx=tr.steps.findIndex(st=>!doneSet.has(st.id));
   const curStep=activeIdx>=0?tr.steps[activeIdx]:null;
   const liveTxt=curStep?liveHintForStep(curStep.id,s):'';
   const countsSig=TRACKS.map(t=>`${t.id}:${doneByTrack.get(t.id).size}`).join(',');
   const sig=`${tr.id}|${Array.from(doneSet).sort().join(',')}|${activeIdx}|${countsSig}|${liveTxt}`;
   if(sig===lastRenderedSig)return;
   lastRenderedSig=sig;

   statusEl.textContent=`${doneSet.size} / ${tr.steps.length} complete`;
   const sumEl=document.getElementById('session-summary');
   if(sumEl)sumEl.textContent=tr.summary;

   const tracksEl=document.getElementById('session-tracks');
   if(tracksEl){
    for(const btn of tracksEl.querySelectorAll('.track-pill')){
     const tid=btn.dataset.track,tDef=TRACKS.find(x=>x.id===tid),tDone=doneByTrack.get(tid);
     btn.classList.toggle('sel',tid===tr.id);
     btn.classList.toggle('done',tDone.size===tDef.steps.length);
     const b=btn.querySelector('b');
     if(b)b.textContent=`${tDone.size}/${tDef.steps.length}`;
    }
   }

   for(let i=0;i<tr.steps.length;i++){
    const st=tr.steps[i],row=container.children[i];
    if(!row)continue;
    const done=doneSet.has(st.id),cur=i===activeIdx;
    row.classList.toggle('done',done);
    row.classList.toggle('current',cur);
    row.dataset.done=done?'1':'0';
    row.querySelector('.step-mark').textContent=done?'✓':String(i+1);
    const liveEl=row.querySelector('.step-live');
    if(liveEl)liveEl.textContent=cur?liveTxt:'';
   }
  }

  function buildTrackBar(){
   const tracksEl=document.getElementById('session-tracks');
   if(!tracksEl)return;
   tracksEl.innerHTML='';
   for(const t of TRACKS){
    const b=document.createElement('button');
    b.type='button';
    b.className='track-pill'+(t.id===activeTrackId?' sel':'');
    b.dataset.track=t.id;
    b.innerHTML=`<span>${t.label}</span><b class="mono">0/${t.steps.length}</b>`;
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
    row.innerHTML=
     `<span class="step-mark" aria-hidden="true">${i+1}</span>`+
     `<div class="step-body"><strong>${st.title}</strong><p>${st.how}</p><div class="step-live mono"></div></div>`+
     `<button type="button" class="step-do" data-do="${st.id}">Do it for me</button>`;
    row.querySelector('.step-do').addEventListener('click',()=>doStep(st.id));
    container.appendChild(row);
   });
  }

  buildTrackBar();
  buildDom();
  return {
   STEPS,
   TRACKS,
   evaluate,
   resetProgress,
   onReboot,
   doStep,
   setTrack,
   getActiveTrack:()=>activeTrackId,
   getCompleted:()=>Array.from(doneByTrack.get(activeTrackId))
  };
 }

 root.VexSession={STEPS,TRACKS,createSession,fmtBest};
})(typeof globalThis!=='undefined'?globalThis:this);
