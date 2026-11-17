#pragma once
// Upload the SAME sketch four times, changing only this number: 1, 2, 3, 4.
#ifndef VEX_STAGE
#define VEX_STAGE 1
#endif
constexpr int PIN_MODE=0, PIN_QUEST=1, PIN_FOOT=2;
constexpr int TFT_RST=3, TFT_DC=4, TFT_CS=5, PIN_LED=9;
// Hardware SPI: MOSI=11, SCK=13. Audio uses 7,8,18,19,20,21,23.
constexpr unsigned long TFT_HZ=16000000;
constexpr unsigned char INPUT_LEVEL=0; // 0 = 3.12V peak-to-peak full scale. Lowest gain first.
constexpr unsigned short GOALS[]={5,10,20};
constexpr unsigned long HOLD_MS=1200;
