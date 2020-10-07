/*
 * This v1 usermod file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2750+ are reserved for your custom use case. (if you extend #define EEPSIZE in const.h)
 * If you just need 8 bytes, use 2551-2559 (you do not need to increase EEPSIZE)
 * 
 * Consider the v2 usermod API if you need a more advanced feature set!
 */

//Use userVar0 and userVar1 (API calls &U0=,&U1=, uint16_t)
#include "wled.h"
#include "FX.h"

//#define DEVELOPMENT
#ifdef USE_FASTLED
  #define LED_TYPE WS2812
  #define COLOR_ORDER RGB  // strip is GRB, NeoPixelBus uses different order in memory
  // Custom LED driver board uses GPIO D1, D2, D4, and D5 to drive LED strips 
  #define PIN_SEG_0    5  // D5 on ESP8266 NodeMCU - Compatible with FastLED driver
  #define PIN_SEG_1    1  // D1 on ESP8266 NodeMCU - Compatible with FastLED driver
  #define PIN_UNUSED_2 2  // D2 on ESP8266 NodeMCU - Compatible with FastLED driver
  #define PIN_UNUSED_4 4  // D4 on ESP8266 NodeMCU - Compatible with WLED driver

  extern CRGB* leds;// = (CRGB*)bus->GetPixels();  // Pointer to NeoPixelBus led array
#endif

// Installation configuration
#ifdef DEVELOPMENT
  #define NUM_LEDS_SEG_0 16
  #define NUM_LEDS_SEG_1 15
  #define NUM_SEGMENTS 2
#else
  #define NUM_LEDS_SEG_0 419//420  // really 419, but use 420 for balanced mirror effects
  #define NUM_LEDS_SEG_1 298
  #define NUM_SEGMENTS 12//2
#endif
//#define NUM_LEDS (NUM_LEDS_SEG_0 + NUM_LEDS_SEG_1)
#define NUM_LEDS 180
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

enum palette_t {
  P_Default = 0,
  P_Random_Cycle,
  P_Color_1,
  P_Colors_1_2,
  P_Color_Gradient,
  P_Colors_Only,
  P_Party,
  P_Cloud,
  P_Lava,
  P_Ocean,
  P_Forest,
  P_Rainbow,
  P_Rainbow_Bands,
  P_RedWhiteBlue,
  P_BellinghamFlag,
  P_Sunset_Real,               //13-00 Sunset     // Really BASE_PALETTE_COUNT, not "13"
  P_es_rivendell_15,           //14-01 Rivendell
  P_es_ocean_breeze_036,       //15-02 Breeze
  P_rgi_15,                    //16-03 Red & Blue
  P_retro2_16,                 //17-04 Yellowout
  P_Analogous_1,               //18-05 Analogous
  P_es_pinksplash_08,          //19-06 Splash
  P_Sunset_Yellow,             //20-07 Pastel
  P_Another_Sunset,            //21-08 Sunset2
  P_Beech,                     //22-09 Beech
  P_es_vintage_01,             //23-10 Vintage
  P_departure,                 //24-11 Departure
  P_es_landscape_64,           //25-12 Landscape
  P_es_landscape_33,           //26-13 Beach
  P_rainbowsherbet,            //27-14 Sherbet
  P_gr65_hult,                 //28-15 Hult
  P_gr64_hult,                 //29-16 Hult64
  P_GMT_drywet,                //30-17 Drywet
  P_ib_jul01,                  //31-18 Jul
  P_es_vintage_57,             //32-19 Grintage
  P_ib15,                      //33-20 Rewhi
  P_Tertiary_01,               //34-21 Tertiary
  P_lava,                      //35-22 Fire
  P_fierce_ice,                //36-23 Icefire
  P_Colorfull,                 //37-24 Cyane
  P_Pink_Purple,               //38-25 Light Pink
  P_es_autumn_19,              //39-26 Autumn
  P_BlacK_Blue_Magenta_White,  //40-27 Magenta
  P_BlacK_Magenta_Red,         //41-28 Magred
  P_BlacK_Red_Magenta_Yellow,  //42-29 Yelmag
  P_Blue_Cyan_Yellow,          //43-30 Yelblu
  P_Orange_Teal,               //44-31 Orange & Teal
  P_Tiamat,                    //45-32 Tiamat
  P_April_Night,               //46-33 April Night
  P_Orangery,                  //47-34 Orangery
  P_C9,                        //48-35 C9
  P_Sakura,                    //49-36 Sakura
  P_Aurora,                    //50-37 Aurora
  P_Atlantica,                 //51-38 Atlantica
};

typedef struct {
// Segment physical parameters
  uint16_t startLED;  // first LED of segment in leds[] - 0..NUM_LEDS-1
  uint16_t numLEDs;   // number of LEDS in segment - 1..?
  uint8_t  segmentNum;  // segment number
} Segment;

// Define the properties for each effect
typedef struct {
  uint8_t  mode;      // effect number
  uint8_t  palette;   // palette number
  uint8_t  speed;
  uint8_t  intensity;
  bool     reverse;   // effect moves in reverse
  bool     mirror;    // effect moves from center of segment
  uint32_t colors[NUM_COLORS];  // color[0] is foreground or primary color
  int16_t  time;      // length of time in seconds to display effect
} Effect;

const Effect playList[][NUM_SEGMENTS] = {//RedWhiteBlueplayList[][NUM_SEGMENTS] = {
    { { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 15 } },
    { { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128,  true, false, {RED, WHITE, BLUE}, 15 },   // reverse
      { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128,  true, false, {RED, WHITE, BLUE}, 15 } },
    { { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128, false,  true, {RED, WHITE, BLUE}, 15 } },  // mirror
    { { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128,  true, false, {RED, WHITE, BLUE}, 15 },    // reverse
      { FX_MODE_CHASE_COLOR,        P_RedWhiteBlue, 128, 128,  true,  true, {RED, WHITE, BLUE}, 15 } },  // mirror

    { { FX_MODE_COLORTWINKLE,       P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_COLORTWINKLE,       P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_COLORWAVES,         P_RedWhiteBlue,  32, 255, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_COLORWAVES,         P_RedWhiteBlue,  32, 255, false, false, {RED, WHITE, BLUE}, 15 } },
    { { FX_MODE_COLORWAVES,         P_RedWhiteBlue,  32, 255, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_COLORWAVES,         P_RedWhiteBlue,  32, 255, false,  true, {RED, WHITE, BLUE}, 15 } },  // mirror

    { { FX_MODE_DYNAMIC,            P_RedWhiteBlue, 192, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_DYNAMIC,            P_RedWhiteBlue, 192, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_STATIC_PATTERN,     P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_STATIC_PATTERN,     P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_FILLNOISE8,         P_RedWhiteBlue,   0, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_FILLNOISE8,         P_RedWhiteBlue,   0, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_NOISE16_2,          P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_NOISE16_2,          P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 15 } },
    { { FX_MODE_NOISE16_2,          P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_NOISE16_2,          P_RedWhiteBlue,  64, 128, false,  true, {RED, WHITE, BLUE}, 15 } },  // mirror

    { { FX_MODE_NOISE16_3,          P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_NOISE16_3,          P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_NOISE16_4,          P_RedWhiteBlue,  32, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_NOISE16_4,          P_RedWhiteBlue,  32, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_TRI_STATIC_PATTERN, P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_TRI_STATIC_PATTERN, P_RedWhiteBlue, 128, 128, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_PALETTE,            P_RedWhiteBlue,  96, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_PALETTE,            P_RedWhiteBlue,  96, 128, false, false, {RED, WHITE, BLUE}, 30 } },  // ? mirror

    { { FX_MODE_RAINBOW_CYCLE,      P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_RAINBOW_CYCLE,      P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 15 } },
    { { FX_MODE_RAINBOW_CYCLE,      P_RedWhiteBlue,  64, 128, false, false, {RED, WHITE, BLUE}, 15 },
      { FX_MODE_RAINBOW_CYCLE,      P_RedWhiteBlue,  64, 128, false,  true, {RED, WHITE, BLUE}, 15 } },  // mirror

    { { FX_MODE_TRICOLOR_CHASE,     P_RedWhiteBlue, 192, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_TRICOLOR_CHASE,     P_RedWhiteBlue, 192, 128, false, false, {RED, WHITE, BLUE}, 30 } },  // ? mirror

    { { FX_MODE_TWINKLECAT,         P_RedWhiteBlue, 128, 255, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_TWINKLECAT,         P_RedWhiteBlue, 128, 255, false, false, {RED, WHITE, BLUE}, 30 } },

    { { FX_MODE_SPARKLE,            P_RedWhiteBlue, 192, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_SPARKLE,            P_RedWhiteBlue, 192, 128, false, false, {RED, WHITE, BLUE}, 30 } },
};

const Effect PrideplayList[][NUM_SEGMENTS] = {
    { { FX_MODE_PALETTE,            P_Rainbow,  96, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_PALETTE,            P_Rainbow,  96, 128, false, false, {RED, WHITE, BLUE}, 30 } },  // ? mirror
    { { FX_MODE_COLORTWINKLE,       P_Rainbow, 128, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_COLORTWINKLE,       P_Rainbow, 128, 128, false, false, {RED, WHITE, BLUE}, 30 } },
    { { FX_MODE_RAINBOW_CYCLE,      P_Rainbow,  64, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_RAINBOW_CYCLE,      P_Rainbow,  64, 128, false, false, {RED, WHITE, BLUE}, 30 } },
    { { FX_MODE_RAINBOW_CYCLE,      P_Rainbow,  64, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_RAINBOW_CYCLE,      P_Rainbow,  64, 128, false,  true, {RED, WHITE, BLUE}, 30 } },  // mirror
    { { FX_MODE_TWINKLECAT,         P_Rainbow, 128, 255, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_TWINKLECAT,         P_Rainbow, 128, 255, false, false, {RED, WHITE, BLUE}, 30 } },
    { { FX_MODE_NOISE16_2,          P_Rainbow,  64, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_NOISE16_2,          P_Rainbow,  64, 128, false, false, {RED, WHITE, BLUE}, 30 } },
    { { FX_MODE_NOISE16_2,          P_Rainbow,  64, 128, false, false, {RED, WHITE, BLUE}, 30 },
      { FX_MODE_NOISE16_2,          P_Rainbow,  64, 128, false,  true, {RED, WHITE, BLUE}, 30 } },  // mirror
};

//const Effect[][] ListplayList[] = {
//  RedWhiteBlueplayList, PrideplayList
//};

/*
const Effect playList[][NUM_SEGMENTS] = {
    { {FX_MODE_STATIC, P_Default, 128, 128, false, false, {RED, GREEN, BLUE}, 30 },
      {FX_MODE_STATIC, P_Default, 128, 128, false, false, {GREEN, RED, BLUE}, 30 } },
    { {FX_MODE_RAINBOW, P_Color_Gradient, 128, 128, false, false, {RED, WHITE, BLUE}, 30 },
      {FX_MODE_RAINBOW, P_Default, 255, 128,  true, false, {WHITE, RED, BLUE}, 30 } },
};
*/

Segment segments[NUM_SEGMENTS];
//Effect [][NUM_SEGMENTS] playList;
uint16_t myNumSegments;

// Initialize playlist segment configuration
void initSpinnerSegments() {
  myNumSegments = NUM_SEGMENTS;
  for (uint16_t s = 0; s < NUM_SEGMENTS; s++) {
    segments[s].startLED = s * 15;
    segments[s].numLEDs = 15;
    segments[s].segmentNum = s;
  }

  for (byte s = 0; s < NUM_SEGMENTS; s++)
    strip.setSegment(s, segments[s].startLED, segments[s].startLED + segments[s].numLEDs);
}

// Initialize star segment configuration
void initStarSegments() {
  myNumSegments = 3;
  segments[0].startLED = 0;
  segments[0].numLEDs = 180;
  segments[0].segmentNum = 0;
  segments[1].startLED = 180;
  segments[1].numLEDs = 108;
  segments[1].segmentNum = 1;
  segments[2].startLED = 288;
  segments[2].numLEDs = 48;
  segments[2].segmentNum = 2;
 
  for (byte s = 0; s < myNumSegments; s++)
    strip.setSegment(s, segments[s].startLED, segments[s].startLED + segments[s].numLEDs);
  for (byte s = myNumSegments; s < NUM_SEGMENTS; s++)
    strip.setSegment(s, 0, 0);  // clear unused segments
}

//gets called once at boot. Do all initialization that doesn't depend on network here
void userSetup()
{
// Force playlist to run on reboot
// should read from EEPROM
  userVar0 = 0;//1;

#if false
  // Initialize playlist segment configuration
  segments[0].startLED = 0;
  segments[0].numLEDs = NUM_LEDS_SEG_0;
  segments[0].segmentNum = 0;
#if NUM_SEGMENTS > 1
  segments[1].startLED = segments[0].numLEDs;
  segments[1].numLEDs = NUM_LEDS_SEG_1;
  segments[1].segmentNum = 1;
#endif
#endif

  if (ledCount < NUM_LEDS) {
    ledCount = NUM_LEDS;
    strip.init(useRGBW, ledCount, skipFirstLed);
  }

//  initSpinnerSegments();

#ifdef USE_FASTLED
  FastLED.addLeds<LED_TYPE,PIN_SEG_0,COLOR_ORDER>(leds, segments[0].startLED, segments[0].numLEDs);
#if NUM_SEGMENTS > 1
  FastLED.addLeds<LED_TYPE,PIN_SEG_1,COLOR_ORDER>(leds, segments[1].startLED, segments[1].numLEDs);
  pinMode(PIN_UNUSED_2, OUTPUT);    // set pin to output mode
  digitalWrite(PIN_UNUSED_2, LOW);  // unused output pins should be set low
  pinMode(PIN_UNUSED_4, OUTPUT);    // set pin to output mode
  digitalWrite(PIN_UNUSED_4, LOW);  // unused output pins should be set low
#endif
  FastLED.setCorrection(UncorrectedColor);//TypicalLEDStrip);
  FastLED.setDither(DISABLE_DITHER);
#endif  
}

//gets called every time WiFi is (re-)connected. Initialize own network interfaces here
void userConnected()
{

}

//loop. You can use "if (WLED_CONNECTED)" to check for successful connection
void userLoop()
{
  static int16_t playListIndex = -1;
  static uint32_t endTime = 0;  // time when effect should end and advance to next effect

  if (userVar0 == 0)  // No user playlist selected
    return;

  uint32_t now = millis();
  if (now < endTime)
    return;

//  playList = RedWhiteBlueplayList;
  playListIndex++;  // advance to next playlist entry
  if (playListIndex == ARRAY_SIZE(playList))
    playListIndex = 0;

  DEBUG_PRINT(now);
  DEBUG_PRINT(" >>>---playlist update--->>> ");
  DEBUG_PRINTLN(playListIndex);

  if (playList[playListIndex][0].mode == FX_MODE_STATIC_PATTERN)
    initStarSegments();
  else
    initSpinnerSegments();

  for (byte s = 0; s < myNumSegments; s++) {  // variable number of segments - spinner vs stars
    WS2812FX::Segment& seg = strip.getSegment(s);
    if (playList[playListIndex][0].mode == FX_MODE_STATIC_PATTERN && s == 0) {
      seg.setOption(SEG_OPTION_ON, false);  // force segment 0 off
    }
    else
      seg.setOption(SEG_OPTION_ON, true);
    seg.setOption(SEG_OPTION_SELECTED, true);
    seg.setOption(SEG_OPTION_REVERSED, playList[playListIndex][0].reverse);
    seg.setOption(SEG_OPTION_MIRROR, playList[playListIndex][0].mirror);
    if (s == strip.getMainSegmentId()) {  // special case for main segment (usually 0)
      effectCurrent =   playList[playListIndex][0].mode;
      effectPalette =   playList[playListIndex][0].palette;
      effectSpeed =     playList[playListIndex][0].speed;
      effectIntensity = playList[playListIndex][0].intensity;
      col[0]          = playList[playListIndex][0].colors[0] >> 16 & 0xff;
      col[1]          = playList[playListIndex][0].colors[0] >> 8  & 0xff;
      col[2]          = playList[playListIndex][0].colors[0]       & 0xff;
      colSec[0]       = playList[playListIndex][0].colors[1] >> 16 & 0xff;
      colSec[1]       = playList[playListIndex][0].colors[1] >>  8 & 0xff;
      colSec[2]       = playList[playListIndex][0].colors[1]       & 0xff;
      strip.setColor(2, playList[playListIndex][0].colors[2]);
    }

    if (playList[playListIndex][0].mode == FX_MODE_STATIC_PATTERN)
        seg.mode =      FX_MODE_PALETTE;
      else
        seg.mode =      playList[playListIndex][0].mode;
      strip.setMode(s, seg.mode);
      seg.palette =   playList[playListIndex][0].palette;
      seg.speed =     playList[playListIndex][0].speed;
      seg.intensity = playList[playListIndex][0].intensity;
      for (byte c = 0; c < NUM_COLORS; c++)
        seg.colors[c] = playList[playListIndex][0].colors[c];
  }
  colorUpdated(NOTIFIER_CALL_MODE_DIRECT_CHANGE);
  endTime = now + (uint32_t)1000 * playList[playListIndex][0].time;
}
