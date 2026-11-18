# VEX Pocket Rival - Build Guide R1.1

For a hobbyist alone, a hobbyist with a mentor, or a design team; see the first page for who checks what.


## 01. Build a pocket rival

A practical first build for a hobbyist alone, a hobbyist with a mentor, or a design team taking it to product

### THE OBJECT

A black, crimson and ochre practice companion with a round pixel pet, activity XP and an experimental tuner.

### THE FIRST BUILD

Teensy 4.1 + Audio Adapter + Adafruit 6178. USB power, one buffered audio input, two finger buttons and one momentary footswitch.

### THE STATUS

Real CAD and original firmware source are included. Geometry and desktop logic checks are separate from the physical tests done on a bench.

### WHO CHECKS

Where this guide says mentor, read: an experienced second person, such as a mentor, a maker-space volunteer or a studio engineer. Building alone is covered in docs/SOLO_BUILD.md; the product brief for a studio is docs/STUDIO_BRIEF.md.

**Checkpoint:** Start with pages 2-3. Buy the bench electronics first; print the final enclosure after measurement and screen tests.

## 02. The first bench session

Suggested 2-hour session, alone or with a mentor; bring this pack and the actual parts

### 00-20 MIN / REVIEW

Confirm the USB-only sensing architecture, display choice and source of buffered dry guitar. Review the input attenuator before connecting the rig.

### 20-50 MIN / LEARN

Practise one solder joint. Identify switch lugs and trimmer wiper with a meter. Mark every power and signal wire before soldering.

### 50-100 MIN / MAKE

Upload Stage 1. Add the display and buttons, then upload Stage 2. Get VEX and the control readings working on the desk.

### 100-120 MIN / DECIDE

Measure screen readability, the purchased boards and connector bodies. Record changes in the measurement sheet and assign the next session.

**Checkpoint:** First-session success = a working screen and controls, plus a measured list of remaining fit questions.

## 03. Build in three milestones

Each milestone produces something usable and teaches one new layer

### 1 / PET ON THE DESK

USB + screen + buttons. Learn power, ground, SPI, upload and button debouncing. No guitar connection is needed yet.

### 2 / LISTEN TO GUITAR

Add the audio board and attenuator. Learn signal level, RMS, clipping and calibration. Keep the existing audible guitar path.

### 3 / ENCLOSE + USE

Fit the real parts, assemble the case, route wires and verify readability and strength. Then test the companion alongside your rig.

**Checkpoint:** R1 does not provide audio bypass, silent muting, 9 V pedal power, MIDI control, daily streaks or technique assessment.

## 04. The important design choices

Read this before ordering or printing

### REAL SCREEN SIZE

Adafruit 6178 has a 1.28-inch round screen: approximately 32.4 mm visible diameter. It is smaller than the concept art, chosen for a documented SPI driver.

### REAL CASE SIZE

The body is 86 x146 x45 mm; controls extend higher. This is not a drop-in replacement for a compact pedal tuner. Use a paper footprint on your pedalboard first.

### PLAIN OPTICS + POWER

R1 uses a flat clear disc and direct USB power. A dome and protected pedalboard 9 V conversion are later engineering changes.

### FOOT LOAD

An aluminum plate sits on four columns below the switch. The printed prototype starts with hand operation; a load test with the electronics removed comes before foot use.

**Checkpoint:** If the 32 mm display fails the standing test, choose a larger screen before committing to this enclosure.

## 05. How the pieces fit

Exploded view of the actual CAD assembly

**Checkpoint:** Letters match the part filenames. Display hardware is recessed; the clear window is captured separately above it.

## 06. Buy: the bench electronics

Exact identities matter; budget columns are allowances, not current quotations

|ID / qty|Part to order|Why / source|Allowance|
|---|---|---|---|
|E01 / 1|PJRC TEENSY41 / TEENSY41_NE; non-lockable|Processor, USB, saved progress [S1]|$32-40|
|E02 / 1|PJRC TEENSY4_AUDIO Rev D or D2; not Rev C|Line input and conversion [S2]|$20-28|
|E03 / 1|Adafruit 6178; GC9A01A; 240x240|Documented driver and mounting geometry [S4]|$17.5-17.5|
|E04 / 2|SparkFun COM-11992, SPST normally open|Mode / Quest; solder lugs [S9]|$2.5-2.5|
|E05 / 1|P-H605-BLK; SPST normally open; M12|Start/pause; not an audio bypass switch [S10]|$5-10|
|E06 / 1|Switchcraft 111X, mono solder lug, with nut/washer|Buffered dry input only [S11]|$3-7|
|E07 / 1|Bourns 3386P-1-103LF|Input level adjustment, tool operated [S12]|$2-4|

**Checkpoint:** Do not substitute the old ST7789 square display or a Rev C audio board into these pin tables.

## 07. Buy: wiring and small parts

Complete the bench circuits before ordering the enclosure

|ID / qty|Part / specification|Purpose|Allowance|
|---|---|---|---|
|E08 / 1|10k ohm, 1%, 0.25W, through-hole|Input attenuator series arm|$0.1-0.5|
|E09 / 2|1k ohm, 1%, 0.25W, through-hole|Audio series resistor / LED limit|$0.2-1|
|E10 / 1|10k ohm, 1%, 0.25W, through-hole|Hold display SDCS high|$0.1-0.5|
|E11 / 1|3 mm amber diffused, ordinary low-current LED|Playing indicator|$0.2-1|
|E12 / 1|2.54 mm pitch; cut 14x28 mm for input board|Hand-wired attenuator; no custom PCB needed|$3-8|
|E13 / 1|Two 14-pin male strips + two 14-pin female socket strips, 2.54 mm; access to used pins|Socketed audio stack, nominal 9.4 mm gap; measure|$4-10|
|E14 / 1|26AWG stranded wire, shielded audio pair, heat shrink, 2 small cable ties|Harness and strain relief|$8-15|
|E15 / 1|Micro-B to your Mac; USB5 V only|Power and firmware upload; measure plug envelope|$5-10|

**Checkpoint:** Budget excludes labour, tools, a new buffered splitter, shipping and tax. BOM.csv contains all links and order stages.

## 08. Buy: mechanical parts

Order after the measured-fit gate

|ID / qty|Part / specification|Purpose|Allowance|
|---|---|---|---|
|M01 / 1|A base, B lid, C charcoal bezel, E tray, K spacers, W gold rim + crimson cheek; O cap optional|PETG; coupon before full print|$20-60|
|M02 / 1|40 mm diameter x1 mm polycarbonate|Flat captured window; no dome in R1.1|$2-6|
|M03 / 1|55x32x2 mm aluminum, R3 corners, hole 12.4 mm|Distributes switch force to four columns|$3-10|
|M04 / 1|See fastener schedule; buy spares|All removable threaded joints|$8-18|
|M05 / 1|Thin peripheral foam, 0.1 mm cosmetic adhesive, insulating washers, feet / hook-loop|Rattle control, electrical clearance, mounting|$3-8|

**Checkpoint:** Use hardware/generated/drawings/ for cut profiles and the fastener schedule on the next page. Test a coupon before any full enclosure print.

## 09. Fasteners and tools

Use the included nuts and washers for the purchased controls

|Location|Qty|Screw / part|Mating hardware|
|---|---|---|---|
|Case corners|4|M3 x10 pan head|M3 nuts x4; pockets 5.9 mm across flats|
|Tray to base|4|M3 x6 pan head|M3 nuts x4; pockets 5.9 mm across flats|
|Charcoal bezel to lid|4|M3 x12 pan head|M3 nuts x4, below lid|
|Display to lid|2|M2 x10, 90-degree countersunk|M2 nuts x2; printed 2.0 mm spacers x2; heads flush with faceplate|
|Audio board to tray|3|M2 x25 pan head|M2 nuts x3; 15 mm nylon spacers x3|
|Perfboard to tray|2|M2 x12 pan head|M2 nuts x2; 6 mm nylon spacers x2|
|Footswitch|1|Included M12 hardware|Lower nut, metal plate, lid, top washer, top nut|
|Buttons / input jack|3|Included retaining hardware|Confirm panel thickness and remaining thread|

**Checkpoint:** Tools: temperature-controlled iron, solder/flux, fume extraction, meter, calipers, cutters, wire stripper, drivers and small files. Drilling and plate cutting: a mentor, a maker space or a cutting service from the DXFs.

## 10. Where guitar audio goes

The companion listens on a separate branch

### 1 / FIND A DRY FEED

Use a buffered tuner or auxiliary output on your multi-effects or pedal, and test that it stays live in every preset you play. If it mutes or switches off during playing, use a dedicated buffered splitter output.

### 2 / KEEP THE MAIN ROUTE

Guitar -> buffer or splitter -> your existing amp or modeller route. The companion has no audio output and does not replace that route.

### 3 / CONNECT THE LISTENER

Buffered dry output -> J1 -> attenuator -> Audio Adapter LINE IN L. No raw passive-pickup input, amplifier speaker output or phantom-powered input.

### WHY

A clean branch avoids scoring reverb repeats or synthesized notes. It also keeps experimental firmware out of your audible guitar path.

**Checkpoint:** With the companion disconnected, the original rig should work exactly as before. Log what the rig's tuner mute does to the dry feed.

## 11. Wire the display

Use the board silkscreen names, not cable colors or a guessed pin order

|Wire|From|To|Detail|
|---|---|---|---|
|W01|Teensy 3.3 V|Display V+ / VIN|Do not connect display 3Vo output to Teensy 3.3 V.|
|W02|Teensy GND|Display GND|Common logic ground.|
|W03|Teensy 11 (MOSI)|Display MOSI|Hardware SPI data.|
|W04|Teensy 13 (SCK)|Display SCK|Hardware SPI clock; onboard LED activity is normal.|
|W05|Teensy 5|Display TFTCS|Display select; audio SD uses pin 10.|
|W06|Teensy 4|Display DC|Data / command.|
|W07|Teensy 3|Display RST|Reset.|
|W08|Display SDCS|R4 10k -> 3.3 V|Keep unused display SD card deselected; no SD card in R1.|
|W09|Display BL / Lite|Leave unconnected|Onboard pullup enables backlight.|
|W10|Display MISO and 3Vo|Leave unconnected|Insulate unused pins; EYESPI unused.|

**Checkpoint:** Teensy logic is 3.3 V. Display VIN is an input; display 3Vo is an output and is left disconnected.

## 12. Wire the controls

Test every switch before connecting it to a GPIO

### 1 / IDENTIFY CONTACTS

Remove power. In continuity mode, the selected pair must be open when released and close only while pressed. Use SPST normally-open momentary switches.

### 2 / CONNECT

MODE -> pin 0; QUEST -> pin 1; FOOT -> pin 2. The other lug of each switch goes to GND. Insulate all solder lugs.

### 3 / ADD THE LED

Pin 9 -> 1k resistor R3 -> LED anode. Cathode -> GND. Identify polarity before trimming the leads.

### WHY

INPUT_PULLUP holds the released pin HIGH. A press connects it to ground, so firmware reads LOW.

**Checkpoint:** Unpowered: the switch wire is not shorted to any power rail. Stage 2 must show exactly one changing input per press.

## 13. Build the audio attenuator

Have a second person check this small circuit before connection to the rig

### 1 / IDENTIFY THE TRIMMER

Bourns 3386P-1-103LF is 10k. Meter terminals 1-3: approximately 10k. Terminal 2 is the wiper; its resistance to either end changes as you turn.

### 2 / WIRE THE DIVIDER

TIP -> R1 10k -> terminal 3. Terminal 1 -> ground. Wiper 2 -> R2 1k -> LINE IN L. Jack sleeve -> LINE IN ground.

### 3 / BEGIN AT MINIMUM

Set the wiper near terminal 1 using the meter. Check continuity and shorts, then begin Stage 3 with a quiet known line-level source.

### WHY

The divider lowers the signal before conversion. Its maximum unloaded output is half the input; codec loading reduces it further. It is not a general overvoltage protector.

**Checkpoint:** The audio board has its own input coupling. Use LINE IN L/GND labels; neither MIC nor headphone ground is a substitute.

## 14. Assemble and check the audio stack

Short aligned headers carry the fast audio clocks

### 1 / ORIENT

Use the PJRC Rev D/D2 pin diagram. Align the shared 14-pin rows with the matching printed Teensy pin numbers; do not align by the board edges alone.

### 2 / SOLDER + INSPECT

Socket the stack for service. Keep the audio clocks on short headers. Check every row for bridges with magnification and continuity.

### 3 / CHECK POWER

USB unplugged: inspect the 3.3 V/GND pair for a sustained short. USB on: black probe to GND, red to 3.3 V; target 3.20-3.40 V. Stop if a board heats abnormally.

### RESERVED PINS

Audio 7/8/20/21/23; I2C18/19. Pins 6 and 10 are held HIGH to deselect optional audio-board memory and SD. No SD cards in R1.

**Checkpoint:** Verify display + audio-board current against the Teensy regulator budget. Record the measured header gap; CAD currently reserves 9.4 mm.

## 15. Upload 1: prove USB on your Mac

One sketch, four stages: change VEX_STAGE in config.h

### 1 / INSTALL

Install Arduino IDE2 from arduino.cc. In Arduino IDE > Settings, add the PJRC boards URL listed in docs/LOCAL_DEVELOPMENT.md; install Teensy using Boards Manager.

### 2 / LIBRARIES

Install Adafruit GC9A01A, Adafruit GFX and Adafruit BusIO using Library Manager. Audio, Wire, SPI and EEPROM come with Teensy support.

### 3 / OPEN + SELECT

Open firmware/VEX_Pocket_Rival/VEX_Pocket_Rival.ino. Choose Teensy 4.1, USB Type Serial, normal 600MHz. Leave VEX_STAGE1 for the first upload.

### 4 / VERIFY + UPLOAD

Click Verify, then Upload. Select the Teensy port. If prompted, press the physical Program button. Open Serial Monitor at 115200. From a terminal the same steps are make firmware STAGE=1, make upload STAGE=1, make monitor.

**Checkpoint:** Onboard LED blinks and Serial says VEX USB OK. No .hex is supplied: the complete target build still needs this verification.

## 16. Upload 2: screen and buttons

Disconnect USB before adding each wiring group

### 1 / CONNECT

Add display wires W01-W10 and controls W11-W14. Keep SPI wires short, initially under 100 mm. Leave audio input disconnected.

### 2 / UPLOAD

Set VEX_STAGE2, Verify and Upload. The display should show the bat and live M/Q/F button states. Serial prints one JSON line a second: flush_us is the time to draw and send one cadence, tx_bytes the rows sent. Type f, b, 1 or 2 in the monitor to compare full-frame, band-diff and 100 vs 200ms redraws.

### 3 / EXERCISE

Press each control ten times. One state changes from 0 to 1 while held, and returns to 0 on release. FOOT also lights the status LED.

### IF IT FAILS

White screen: check VIN/GND, CS/DC/reset and SPI pins. Stuck input: inspect the chosen lug pair. Rotated display: change setRotation, not wiring.

**Checkpoint:** Put the display at the real pedalboard distance. Confirm legibility before fabricating the enclosure.

## 17. Upload 3: audio and level

Begin with a known low-level line source; then test the buffered guitar feed

### 1 / START SMALL

Set VEX_STAGE3. Codec input level stays 0, the least sensitive setting (3.12Vpp full scale). Trimmer begins at minimum.

### 2 / OBSERVE

Serial must report codec=1. Slowly raise the trimmer while playing. Aim for peaks roughly 0.10-0.80 during strong playing; 0.98 means clipping.

### 3 / COMPARE GUITARS

Test each guitar you will practise with separately, for example one with active and one with passive pickups. Record trim position, peak and quiet RMS. Choose a setting that avoids clipping any of them.

### IF IT FAILS

codec=0: inspect 3.3 V, GND and I2C18/19. Flat-zero audio: inspect LINE IN L, input ground and short I2S headers. Low signal: confirm source before increasing gain.

**Checkpoint:** Confirm audible rig noise does not change with USB connected. Avoid correcting hum by disconnecting protective mains earth.

## 18. Upload 4: bring VEX to life

Participation XP and one reward per completed practice session

### 1 / CALIBRATE

Set VEX_STAGE4 and upload. Mute the strings, then hold QUEST for 1.2s. Remain quiet for 3s. Calibration pauses counting and runs again after every boot.

### 2 / PRACTISE

Tap FOOT to start. Detected playing earns 10XP per active minute. Silence, pause, clipping and stale audio do not earn time. A completed goal earns one fragment.

### 3 / SAVE

Tap FOOT to pause. Hold MODE until Saved appears before unplugging. The two-record save scheme checks CRC and retains the previous valid record if one write tears.

### LIMIT

Automatic saves wait for quiet gaps after 60s. Abrupt power loss can lose the unsaved portion of a continuous session. This is activity detection, not proof of musical quality.

**Checkpoint:** Complete a5-minute active goal, verify one fragment, save and reboot. Confirm totals persist and startup is paused.

## 19. Controls, tuning and pet behavior

Keep the foot action predictable

|Control|Tap|Hold 1.2 seconds|
|---|---|---|
|MODE|Practice / tuner; pauses practice|Save current progress|
|QUEST|While paused: choose 5/10/20 min; resets session|Calibrate noise; pauses practice|
|FOOT|Start / pause (practice mode, calibrated)|New session; keeps lifetime XP and fragments|
|Tuner|Large note and cents, A440, single notes only|No audio muting; your rig's tuner or mute handles that|
|VEX|Riffs while active; rests after 30s without activity|No death, debt or punishment for missed days|

**Checkpoint:** Streaks, timed schedules, rhythm scores, bend challenges and battles are future work. No clock/calendar or technique judging is implied.

## 20. Understand the CAD

A real assembly, with explicit limits on purchased-part models

### OPEN

Import VEX_Pocket_Rival_Assembly.step into Fusion. It contains separate named components. STEP preserves geometry, not the Python feature history.

### EDIT

build_cad.py is the parametric source. Base/lid width and length are linked; component positions and cutouts also appear as explicit construction values. Re-run checks after edits.

### DATUM

Origin is the footprint centre on the underside. +Y points toward rear sockets; +Z points up. The faceplate spans z42-45 mm.

### EVIDENCE

Display model is manufacturer CAD. Teensy/audio models are community references. Button, footswitch, jack, headers and trimmer include labeled simplified envelopes.

**Checkpoint:** Use named part files for fabrication. Do not print the combined assembly, commercial electronics, wire reserves or the clear-window STL.

## 21. Measure and print a coupon first

A cheap fit check before a full case print

### 1 / MEASURE

Record body envelopes, threaded shafts, nut thickness, plug size and solder tails. The jack body is a reserve, not a verified model. Confirm its nut grips the 3 mm rear wall.

### 2 / PRINT

Print Fit_Coupon.stl flat at 100% scale. Try the M3 clearance,7.2 mm button hole,12.4 mm switch hole,9.8 mm jack hole and M3 nut pocket. Enter the results in measurements.csv; make measure-check lists which parts are ready to print.

### 3 / ADJUST

Use calipers and the coupon to set printer clearances. A nut should seat by hand; a threaded control should pass without force. Change the CAD if purchased parts differ.

### 4 / PRINT THE CASE

Suggested starting point: PETG,0.2 mm layers,5perimeters,6top/bottom layers,35% infill. This is a print trial, not a structural rating.

**Checkpoint:** Only measured-fit approval releases the full print. Print base open-side up; faceplate/tray flat; bezel pocket facing up.

## 22. Assembly 1: base and rear input

A_base + case/tray nuts + Q_input_jack

### PARTS

Base A;8M3 captive nuts; Switchcraft 111X and included hardware; USB cable. File the test print gently; remove debris.

### 1 / INSERT NUTS

Seat 4case nuts at the top corner posts and 4tray nuts near the floor. Confirm each screw starts freely before installing electronics.

### 2 / FIT INPUT

Insert the jack from inside through the rear 9.8 mm hole. Add the stock washer/nut outside. Rotate lugs away from the board area and tighten without twisting wires.

### 3 / TEST USB ACCESS

Feed the selected USB plug through the service opening and check that it can reach the Teensy connector. The opening is not a panel-mounted USB socket.

**Checkpoint:** Jack cannot rotate; stock threads have proper engagement; cable and corner posts clear the measured jack body.

## 23. Assembly 2: removable electronics

E_tray + F_Teensy + G_audio + R_input board

### PARTS

Tray E;3x15 mm audio spacers;2x6 mm perfboard spacers; associated M2 hardware;4M3x6 tray screws;2small nylon ties.

### 1 / BUILD ON THE DESK

Attach the socketed Teensy/audio stack. Secure the Teensy through the tray slots with ties clear of components. Install the audio spacers at the three modeled holes.

### 2 / INPUT BOARD

Cut/drill the 14x28 mm perfboard; fit the trimmer and resistors, then inspect wiring. Mount it on the 6 mm spacers. Keep solder tails off the tray and adjacent PCB.

### 3 / INSTALL

Lower the tested carrier into the base and fasten the four tray screws. Preserve access to USB and Program; avoid clamping any wire under the tray.

**Checkpoint:** Rerun Stage 3 before adding the lid. No carrier flex; no metal fastener touches a trace or solder joint.

## 24. Assembly 3: display and finger controls

Build the lid assembly while its underside is accessible

### PARTS

Faceplate B; display H; two 2.0 mm spacers; two M2x10 countersunk screws and nuts; two COM-11992 buttons; LED.

### 1 / DISPLAY

Place the display behind the opening with its solder-header tail toward the buttons. Put the 2 mm spacers between the front PCB mounting pads and the faceplate underside.

### 2 / RETAIN

Seat the M2 countersunk heads flush in the faceplate, then fit nuts below the PCB. Verify flushness before adding the window. Tighten gently; do not load the glass or flex tail.

### 3 / CONTROLS

Fit MODE and QUEST with stock nuts. Insulate leads and leave service length. Fit the LED without adhesive covering its leads or adjacent contacts.

**Checkpoint:** Measured display holes should be 22.86 mm apart. Confirm screen centring and no contact between lid and display glass.

## 25. Footswitch retention detail

Local cutaway of the faceplate; parts separated vertically for clarity

### FROM BELOW

Momentary switch N -> lower retaining nut -> aluminum plate M. The nut meets the underside of the plate.

### THROUGH THE LID

The threaded bushing passes through the plate and faceplate B. Fit the stock washer and upper nut above the lid.

### SUPPORT

When the lid closes, the plate rests on the four base columns. Verify contact at all four locations; the display is not part of this load path.

### OPTIONAL CAP

The crimson cap lifts off for service. Leave it out unless its fit and full switch travel are confirmed. Validate the structure before foot use.

**Checkpoint:** This illustration explains assembly order. It is not a force rating; use the exact purchased switch and hardware for the fit test.

## 26. Assembly 4: footswitch and window

Separate the screen cover from the foot-load path

### 1 / FOOTSWITCH

On the switch bushing: lower nut ->2 mm aluminum plate ->3 mm lid ->top washer ->top nut. Position the plate so it rests on all four base columns when closed.

### 2 / WINDOW

Place the 40x1 mm clear disc in the bezel pocket. It has 0.3 mm nominal radial and axial allowance. Use only a thin peripheral anti-rattle gasket, never pressure on the LCD.

### 3 / BEZEL

Fasten charcoal C with 4M3x12 screws and nuts. Bond W gold rim into its shallow recess and W crimson cheek beside C with thin adhesive. Keep the cap clear of full travel.

### WHY

The replaceable window protects the display. The plate spreads switch force into the base. Both need physical fit and load tests.

**Checkpoint:** No screen pressure when screws tighten. Hand-actuate the switch through its full travel before any foot test.

## 27. Assembly 5: harness, close and test

Finish with service access and repeatable checks

### 1 / ROUTE

Keep the shielded analog pair short and away from display/SPI wires. Use the right-side wire lane. Add heat shrink at every exposed solder lug and resistor lead.

### 2 / SERVICE LOOP

Leave enough slack to lift the lid about 50 mm and rest it beside the base without pulling display wires. Add strain relief without loading a connector.

### 3 / CLOSE

Inspect from every side while lowering the lid. If it resists, reopen and find the obstruction. Tighten 4M3x10 case screws evenly; do not force the printed posts.

### 4 / ACCEPT

Run the CSV acceptance log: electrical, software, optical, mechanical, thermal and rig-noise checks. Keep your existing tuner until the new tuner passes reference tests.

**Checkpoint:** A valid STEP does not establish real wiring clearance or foot strength. Whoever runs each physical test signs the checked_by column.

## 28. Develop it after the first working build

Small code changes with observable results

### 1 / PERSONALITY

Edit the PNG/JSON art in art/ and run tools/make_sprites.py to regenerate the headers, then make sim-build and tools/render_screen.py. Shared ui.h renders the pet on desktop and device.

### 2 / A NEW REWARD

Change quest rules in core.h. Add a test proving the reward happens once at the threshold and persists after save/restore. Run tests before uploading.

### 3 / BETTER CHALLENGES

Add rhythm/bend scoring as separate exercises with known targets. Keep participation XP independent. Test noise, chords and wrong notes before trusting scores.

### 4 / HARDWARE NEXT

Larger screen, dome, MIDI or 9 V power each needs a new wiring/CAD revision. Freeze the working R1 files and change one subsystem at a time.

**Checkpoint:** No new feature should silently change pin ownership, saved-record layout or the meaning of a practice score.

## 29. Troubleshoot by the failed checkpoint

Use the earliest failing stage, then repeat later checks

|Symptom|First checks|Do next|
|---|---|---|
|No USB port / upload|Known data cable; board selection; physical Program button|Return to Stage 1 with other wiring unplugged|
|White / wrong screen|Send t for the test pattern: border, corner colours, TOP label; VIN/GND; CS/DC/RST; MOSI11 and SCK13|Stage 2; shorten wires; send - to lower the SPI clock; rotated means setRotation, not wiring|
|Button always pressed|Wrong lug pair or solder bridge to ground|Meter with power off; retest one button|
|codec=0 / no audio|Audio board revision;3.3 V;I2C18/19;header alignment|Stage 3; known low-level line source|
|False XP in silence|Threshold too low; wrong/wet source; ground noise|Fix source/noise, then recalibrate|
|No XP while playing|Paused / tuner mode; calibration missing; clipping; stale samples|Watch Serial; reduce level or inspect timing|
|Lid will not close|Jack body, solder tails, service loop, stack height|Reopen and measure; never pull shut with screws|
|Tuning unreadable / unstable|Screen size/angle, glare, pitch confidence, clipping, polyphonic input|Use one reference note; retain existing tuner|

**Checkpoint:** Open issues and measured values belong in acceptance_log.csv and measurements.csv, not in assumptions. make bench-report summarises a session; make measure-check reads the measurements.

## 30. What has been checked

Understand what the package proves and what the bench will prove

### COMPUTER CHECKS

Valid CAD solids and STEP re-import; all-component overlap checks including fasteners; source-derived display mounts; host tests of counting, rewards, pitch math, saves and button logic.

### NOT YET PROVEN

Actual display/audio behavior, component revisions, wiring clearance, electrical noise, temperature, lens readability and enclosure strength.

### TARGET BUILD EVIDENCE

make setup-firmware and make firmware-all compile all four stages without hardware (passes locally, 17 Nov 2026). Display timing is instrumented (flush_us, tx_bytes) but unmeasured until a board is on the bench.

### PHYSICAL RELEASE

Review all purchased parts and the input circuit; complete the log before guitar/foot use. A successful desktop test is useful evidence, not a hardware certificate.

**Checkpoint:** See hardware/generated/checks/ and GitHub Actions. Compile success and physical acceptance are separate evidence.

## 31. Review and edit the grouped CAD

Five service groups keep related hardware together

### OPEN THE ASSEMBLY

Use hardware/generated/VEX_Pocket_Rival_Assembly.step: Chassis, Front Panel, Electronics Carrier, Rear IO and Footswitch. Separate subassembly exports and a flat fallback are also supplied.

### EDIT THE SOURCE

hardware/cad/build_cad.py is the construction source. assembly_tree.py controls ownership. STEP has named solids, not a native Fusion feature timeline.

### CHECK THE RESULT

Run make cad cad-check. R1.1 passes 72component validity,2556pairwise overlap checks and grouped STEP re-import. Real parts still require measurement.

### PERSONALITY

The charcoal clipped bezel, thin gold ring, crimson cheek and original pixel bat are small practical additions. Keep the actual 32.4 mm display size.

**Checkpoint:** See docs/CAD_REVIEW.md for coordinates, file roles and modification constraints.

## 32. Try the device before hardware arrives

Shared C++ practice logic and screen renderer run locally

### RUN

From the repo: make test, then make sim. Open http://127.0.0.1:8765. No Arduino toolchain or third-party Python packages are required for the simulator.

### EXERCISE

Calibrate, start, choose Playing and advance five active minutes. Try silence, clipping, stale samples, tuner, Save, Reboot and corrupt-record fallback.

### COMPILE

Install Arduino CLI; make setup-firmware; make firmware-all. This compiles all four stages with pinned dependencies and never uploads.

### UNDERSTAND THE LIMIT

The simulator injects RMS/peak/Hz; it does not emulate the ADC, pitch detector, USB noise, switches or physical screen optics.

### WITH A TEENSY ATTACHED

make sim DEVICE=auto opens the Device tab: the real panel mirrored over USB beside the simulator frame, a pixel diff, the serial switches and telemetry sparklines.

**Checkpoint:** See docs/LOCAL_DEVELOPMENT.md for Mac setup, dependencies, staged Verify/Upload instructions and the bench loop.

## 33. Sources and design provenance

Product references checked 16Nov 2026; exact source URLs also in SOURCES.md

|ID|Primary source / purpose|Link|
|---|---|---|
|S1|Teensy 4.1 hardware / power / pinout|<link href="https://www.pjrc.com/store/teensy 41.html" color="#98394A">Open source</link>|
|S2|Audio Adapter Rev D / D2 and shared pins|<link href="https://www.pjrc.com/store/teensy 3_audio.html" color="#98394A">Open source</link>|
|S3|SGTL5000 input levels and functions|<link href="https://www.pjrc.com/teensy/gui/?info=AudioControlSGTL5000" color="#98394A">Open source</link>|
|S4|Adafruit 6178 round display|<link href="https://www.adafruit.com/product/6178" color="#98394A">Open source</link>|
|S5|Display pin names and power|<link href="https://learn.adafruit.com/adafruit-1-28-240x240-round-tft-lcd/pinouts" color="#98394A">Open source</link>|
|S6|Display CAD / PCB / downloads|<link href="https://learn.adafruit.com/adafruit-1-28-240x240-round-tft-lcd/downloads" color="#98394A">Open source</link>|
|S7|GC9A01A library|<link href="https://github.com/adafruit/Adafruit_GC9A01A" color="#98394A">Open source</link>|
|S8|Arduino 2 / Teensy installation|<link href="https://www.pjrc.com/teensy/td_download.html" color="#98394A">Open source</link>|
|S9|SparkFun red panel button COM-11992|<link href="https://www.sparkfun.com/momentary-button-panel-mount-red.html" color="#98394A">Open source</link>|

**Checkpoint:** Adafruit CAD/PCB attribution is retained in vendor/. Imported community models are references; verify the actual hardware.

## 34. Sources and next-session handoff

Product references checked 16Nov 2026; exact source URLs also in SOURCES.md

|ID|Primary source / purpose|Link|
|---|---|---|
|S10|SPST soft footswitch P-H605-BLK|<link href="https://www.amplifiedparts.com/products/footswitch-spst-momentary-solder-lugs-soft-switch-black" color="#98394A">Open source</link>|
|S11|Switchcraft 111X mono jack|<link href="https://www.switchcraft.com/open-circuit-solder-lug-termination/111x/" color="#98394A">Open source</link>|
|S12|Bourns 3386P trimmer dimensions / pinout|<link href="https://www.bourns.com/docs/product-datasheets/3386.pdf" color="#98394A">Open source</link>|
|S13|Teensy Audio Workshop teaching reference|<link href="https://www.pjrc.com/store/workshop_t4.pdf" color="#98394A">Open source</link>|
|S14|Teenage Engineering assembly-layout reference|<link href="https://assets.teenage.engineering/_img/6399cd 15132d7e0af 3c8620c_original.pdf" color="#98394A">Open source</link>|
|S15|Display manufacturer STEP directory|<link href="https://github.com/adafruit/Adafruit_CAD_Parts/tree/main/6178%201.28in%20Round%20TFT" color="#98394A">Open source</link>|
|S16|Display manufacturer PCB|<link href="https://github.com/adafruit/Adafruit-1.28-240x240-Round-Display-PCB" color="#98394A">Open source</link>|
|S17|build 123d documentation|<link href="https://build 123d.readthedocs.io/en/latest/" color="#98394A">Open source</link>|
|S18|Example multi-effects manual (Zoom G3): check how your own rig treats its tuner/AUX dry feed|<link href="https://cdn.shopify.com/s/files/1/0756/7638/5564/files/G32026.pdf" color="#98394A">Open source</link>|

**Checkpoint:** Adafruit CAD/PCB attribution is retained in vendor/. Imported community models are references; verify the actual hardware.
