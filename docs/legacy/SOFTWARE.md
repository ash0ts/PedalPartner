# Software bring-up on your Mac

1. Install Arduino IDE 2 from https://www.arduino.cc/en/software . PJRC currently documents Teensyduino 1.62 and recommends Arduino 2.3.10 or later. Record the actual versions you install in `checks/target_build_log.md`.
2. Arduino IDE > Settings > Additional Boards Manager URLs: add `https://www.pjrc.com/teensy/package_teensy_index.json`.
3. Boards Manager: install **Teensy**. Select **Teensy 4.1**, **USB Type: Serial**, normal **600 MHz**, default optimization.
4. Library Manager: install **Adafruit GC9A01A**, **Adafruit GFX Library**, and **Adafruit BusIO**, including dependencies. Audio/Wire/SPI/EEPROM are provided by Teensy support.
5. Open `firmware/VEX_Pocket_Rival/VEX_Pocket_Rival.ino`. Keep the .ino and all three .h files in that same folder.
6. Edit `config.h`: `#define VEX_STAGE 1`. Click Verify. Resolve every compiler error before Upload. Select the Teensy USB port; press the physical Program button only if requested by the loader.
7. Serial Monitor:115200 baud. Stage1 prints `VEX USB OK` and blinks the onboard LED. A charge-only cable will not upload.
8. Disconnect USB, wire the display/controls, change to Stage2, Verify/Upload. Read M/Q/F on screen while pressing each button.
9. Disconnect USB, add the socketed audio board and attenuator, change to Stage3. `codec=1` is required. Adjust level from minimum using the serial RMS/peak values.
10. Stage4 enables VEX, calibration, practice, rewards and tuner. Hold QUEST to calibrate while muting strings; tap FOOT to start.

## Version and validation status

The full sketch has not been compiled for Teensy here: no target toolchain was installed and direct toolchain downloads returned HTTP403. No precompiled HEX is supplied. The display API was checked against the official Adafruit GC9A01A header, and the platform-independent logic tests pass on the desktop. Neither check proves target compilation or physical behavior.

## Repeat the desktop tests

From this pack's root on a Mac with Xcode Command Line Tools:

```sh
c++ -std=c++17 -Wall -Wextra -Werror tests/core_tests.cpp -o /tmp/vex-core-tests
/tmp/vex-core-tests
```

These cover activity gates, clipping/stale input, one reward per goal, goal changes, pitch conversion, CRC corruption, restore, sequence rollover and button debouncing. They do not run the hardware libraries.

## State and scoring

- Ten XP per detected active minute; level increments per ten active minutes.
- Goal choices:5,10,20 active minutes. One fragment per completed session.
- Startup is paused and requires noise calibration, even after loading saved progress.
- Changing goal while paused starts a new session. Holding FOOT starts a new session without clearing lifetime XP/fragments.
- MODE tap switches between practice and tuner and pauses practice. It does not mute audio.
- Hold MODE to save before unplugging. Automatic saves wait for quiet after60s; uninterrupted playing may remain unsaved until you pause/save.
- Two 64-byte EEPROM slots hold 48-byte CRC-protected records. This is a best-effort recovery scheme, not guaranteed power-failure immunity.
- No RTC/date, scheduled bonus, daily streak or recognition of songs/hand technique is implemented.

## Safe extensions

1. Sprite edits: modify `make_sprites.py`, run it, upload again. The PNG previews and `sprites.h` come from one pixel source.
2. Rewards: change `core.h` and test threshold crossing, duplicate updates, reset and save/restore.
3. Save format: increment `Record.version` and write a migration before changing its layout. R1 does not migrate older PracticeCompanion records.
4. New pins: update `config.h`, `wiring.csv`, the guide and pin-ownership checks together. Do not reuse I2S pins for SPI or buttons.
5. New display: select driver and wiring before moving mounting holes; verify readable note size on the floor.
