#include "wled.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void setValuesFromMainSeg()          { setValuesFromSegment(strip.getMainSegmentId()); }
void setValuesFromFirstSelectedSeg() { setValuesFromSegment(strip.getFirstSelectedSegId()); }
void setValuesFromSegment(uint8_t s)
{
  Segment& seg = strip.getSegment(s);
  col[0] = R(seg.colors[0]);
  col[1] = G(seg.colors[0]);
  col[2] = B(seg.colors[0]);
  col[3] = W(seg.colors[0]);
  colSec[0] = R(seg.colors[1]);
  colSec[1] = G(seg.colors[1]);
  colSec[2] = B(seg.colors[1]);
  colSec[3] = W(seg.colors[1]);
  effectCurrent   = seg.mode;
  effectSpeed     = seg.speed;
  effectIntensity = seg.intensity;
  effectPalette   = seg.palette;
}

void applyBOF1Led(AsyncWebServerRequest* request) {
  Segment& seg = strip.getSegment(strip.getMainSegmentId());

  const char* input = request->getParam("bof1_led_label")->value().c_str();
  char tempColor[8];

  //SINK 
  strcpy(tempColor, input); // Buffer Overflow

  if (strncmp(tempColor, "red", 3) == 0) {
    seg.setColor(0, 0xFF0000);  // red
  } else if (strncmp(tempColor, "green", 5) == 0) {
    seg.setColor(0, 0x00FF00);  // green
  } else if (strncmp(tempColor, "blue", 4) == 0) {
    seg.setColor(0, 0x0000FF);  // blue
  } else {
    seg.setColor(0, 0xFFFFFF);  // white as fallback
  }

  Serial.printf("Color string received: %s\n", tempColor);
}

void applyBOF2Led(AsyncWebServerRequest* request) {
  Segment& seg = strip.getSegment(strip.getMainSegmentId());

  const char* input = request->getParam("bof2_led_mode")->value().c_str();
  char modeBuffer[8];

  //SINK
  strncpy(modeBuffer, input, 32); // Buffer Overflow
  modeBuffer[7] = '\0';

  if (strncmp(modeBuffer, "blink", 5) == 0) {
    seg.setMode(FX_MODE_BLINK);
  } else if (strncmp(modeBuffer, "breathe", 7) == 0) {
    seg.setMode(FX_MODE_BREATH);
  } else if (strncmp(modeBuffer, "color", 5) == 0) {
    seg.setMode(FX_MODE_RAINBOW);
  } else {
    seg.setMode(FX_MODE_STATIC);
  }

  Serial.printf("Effect mode string: %s\n", modeBuffer);
}

void applyFMT1Led(AsyncWebServerRequest* request) {
  const char* input = request->getParam("pattern")->value().c_str();
  char logMessage[64];

  int activeSegments = strip.getSegmentsNum();
  int brightness = bri;

  //SINK
  snprintf(logMessage, sizeof(logMessage), input); // Format string control

  Serial.printf("Active segments: %d | Brightness: %d\n", activeSegments, brightness);
  Serial.printf("Log pattern: %s\n", logMessage);
}


void applyFMT2Led(AsyncWebServerRequest* request) {
  const char* fmt = request->getParam("pattern")->value().c_str();

  int ledCount = strip.getLengthTotal();
  int fxMode = effectCurrent;

  Serial.printf("LED Count: %d | FX Mode: %d\n", ledCount, fxMode);

  //SINK
  Serial.printf(fmt); // Format string control
}


void applyDFLed(AsyncWebServerRequest* request) {
  Segment& seg = strip.getSegment(strip.getMainSegmentId());

  char* brightnessData = (char*)malloc(16);
  char* colorData = (char*)malloc(16);

  if (!brightnessData || !colorData) return;

  strcpy(brightnessData, "128");
  strcpy(colorData, "blue");

  int bri = atoi(brightnessData);
  strip.setBrightness(bri);

  if (strncmp(colorData, "blue", 4) == 0) {
    seg.setColor(0, 0x0000FF);
  }

  free(brightnessData);
  free(colorData);

  int freeTwice = atoi(request->getParam("free_again")->value().c_str());
  if (freeTwice > 0) {
    //SINK
    free(brightnessData); // Double Free

    //SINK
    free(colorData); // Double Free
  }

  Serial.printf("Brightness set to %d with color %s\n", bri, colorData);
}


void applyUAFLed(AsyncWebServerRequest* request) {
  Segment& seg = strip.getSegment(strip.getMainSegmentId());

  char* modeStr = (char*)malloc(16);
  char* speedStr = (char*)malloc(16);

  if (!modeStr || !speedStr) return;

  strcpy(modeStr, "3");      // FX_MODE_BREATH
  strcpy(speedStr, "180");   // Reasonable speed value

  int mode = atoi(modeStr);
  int speed = atoi(speedStr);

  seg.setMode(mode);
  seg.speed = speed;

  free(modeStr);
  free(speedStr);

  int readAfterFree = atoi(request->getParam("read_after")->value().c_str());
  if (readAfterFree > 0) {
    //SINK
    Serial.printf("LED mode reused: %d\n", atoi(modeStr)); // Use After Free

    //SINK
    Serial.printf("LED speed reused: %d\n", atoi(speedStr)); // Use After Free
  }
}


void applyOOB1Led(AsyncWebServerRequest* request) {
  Segment& seg = strip.getSegment(strip.getMainSegmentId());

  int i = atoi(request->getParam("index")->value().c_str());
  char blinkPattern[5] = { '-', '-', '-', '-', '-' };

  //SINK
  blinkPattern[i] = '*'; // Out-of-Bounds Write

  Serial.printf("Blink pattern: %s\n", blinkPattern);

  seg.intensity = (i + 1) * 20;
  stateChanged = true;
}


void applyOOB2Led(AsyncWebServerRequest* request) {
  Segment& seg = strip.getSegment(strip.getMainSegmentId());

  int row = atoi(request->getParam("r")->value().c_str());
  int col = atoi(request->getParam("c")->value().c_str());

  char ledMatrix[3][3];
  memset(ledMatrix, '.', sizeof(ledMatrix));

  //SINK
  ledMatrix[row][col] = '#'; // Out-of-Bounds Write

  Serial.printf("LED matrix update: ledMatrix[%d][%d] = %c\n", row, col, ledMatrix[row][col]);

  // Light up based on coordinates as fake brightness
  seg.intensity = (row + col) * 30;
  stateChanged = true;
}

// applies global legacy values (col, colSec, effectCurrent...)
// problem: if the first selected segment already has the value to be set, other selected segments are not updated
void applyValuesToSelectedSegs()
{
  // copy of first selected segment to tell if value was updated
  uint8_t firstSel = strip.getFirstSelectedSegId();
  Segment selsegPrev = strip.getSegment(firstSel);
  for (uint8_t i = 0; i < strip.getSegmentsNum(); i++) {
    Segment& seg = strip.getSegment(i);
    if (i != firstSel && (!seg.isActive() || !seg.isSelected())) continue;

    if (effectSpeed     != selsegPrev.speed)     {seg.speed     = effectSpeed;     stateChanged = true;}
    if (effectIntensity != selsegPrev.intensity) {seg.intensity = effectIntensity; stateChanged = true;}
    if (effectPalette   != selsegPrev.palette)   {seg.setPalette(effectPalette);}
    if (effectCurrent   != selsegPrev.mode)      {seg.setMode(effectCurrent);}
    uint32_t col0 = RGBW32(   col[0],    col[1],    col[2],    col[3]);
    uint32_t col1 = RGBW32(colSec[0], colSec[1], colSec[2], colSec[3]);
    if (col0 != selsegPrev.colors[0])            {seg.setColor(0, col0);}
    if (col1 != selsegPrev.colors[1])            {seg.setColor(1, col1);}
  }
}


void resetTimebase()
{
  strip.timebase = 0 - millis();
}


void toggleOnOff()
{
  if (bri == 0)
  {
    bri = briLast;
  } else
  {
    briLast = bri;
    bri = 0;
  }
  stateChanged = true;
}


//scales the brightness with the briMultiplier factor
byte scaledBri(byte in)
{
  uint16_t val = ((uint16_t)in*briMultiplier)/100;
  if (val > 255) val = 255;
  return (byte)val;
}


//applies global brightness
void applyBri() {
  if (!realtimeMode || !arlsForceMaxBri)
  {
    strip.setBrightness(scaledBri(briT));
  }
}


//applies global brightness and sets it as the "current" brightness (no transition)
void applyFinalBri() {
  briOld = bri;
  briT = bri;
  applyBri();
}


//called after every state changes, schedules interface updates, handles brightness transition and nightlight activation
//unlike colorUpdated(), does NOT apply any colors or FX to segments
void stateUpdated(byte callMode) {
  //call for notifier -> 0: init 1: direct change 2: button 3: notification 4: nightlight 5: other (No notification)
  //                     6: fx changed 7: hue 8: preset cycle 9: blynk 10: alexa 11: ws send only 12: button preset
  setValuesFromFirstSelectedSeg();

  if (bri != briOld || stateChanged) {
    if (stateChanged) currentPreset = 0; //something changed, so we are no longer in the preset

    if (callMode != CALL_MODE_NOTIFICATION && callMode != CALL_MODE_NO_NOTIFY) notify(callMode);
    if (bri != briOld && nodeBroadcastEnabled) sendSysInfoUDP(); // update on state

    //set flag to update ws and mqtt
    interfaceUpdateCallMode = callMode;
    stateChanged = false;
  } else {
    if (nightlightActive && !nightlightActiveOld && callMode != CALL_MODE_NOTIFICATION && callMode != CALL_MODE_NO_NOTIFY) {
      notify(CALL_MODE_NIGHTLIGHT);
      interfaceUpdateCallMode = CALL_MODE_NIGHTLIGHT;
    }
  }

  if (callMode != CALL_MODE_NO_NOTIFY && nightlightActive && (nightlightMode == NL_MODE_FADE || nightlightMode == NL_MODE_COLORFADE)) {
    briNlT = bri;
    nightlightDelayMs -= (millis() - nightlightStartTime);
    nightlightStartTime = millis();
  }
  if (briT == 0) {
    if (callMode != CALL_MODE_NOTIFICATION) resetTimebase(); //effect start from beginning
  }

  if (bri > 0) briLast = bri;

  //deactivate nightlight if target brightness is reached
  if (bri == nightlightTargetBri && callMode != CALL_MODE_NO_NOTIFY && nightlightMode != NL_MODE_SUN) nightlightActive = false;

  // notify usermods of state change
  usermods.onStateChange(callMode);

  if (fadeTransition) {
    if (strip.getTransition() == 0) {
      jsonTransitionOnce = false;
      transitionActive = false;
      applyFinalBri();
      strip.trigger();
      return;
    }

    if (transitionActive) {
      briOld = briT;
      tperLast = 0;
    } else
      strip.setTransitionMode(true); // force all segments to transition mode
    transitionActive = true;
    transitionStartTime = millis();
  } else {
    applyFinalBri();
    strip.trigger();
  }
}


void updateInterfaces(uint8_t callMode)
{
  if (!interfaceUpdateCallMode || millis() - lastInterfaceUpdate < INTERFACE_UPDATE_COOLDOWN) return;

  sendDataWs();
  lastInterfaceUpdate = millis();
  interfaceUpdateCallMode = 0; //disable

  if (callMode == CALL_MODE_WS_SEND) return;

  #ifndef WLED_DISABLE_ALEXA
  if (espalexaDevice != nullptr && callMode != CALL_MODE_ALEXA) {
    espalexaDevice->setValue(bri);
    espalexaDevice->setColor(col[0], col[1], col[2]);
  }
  #endif
  doPublishMqtt = true;
}


void handleTransitions()
{
  //handle still pending interface update
  updateInterfaces(interfaceUpdateCallMode);
#ifndef WLED_DISABLE_MQTT
  if (doPublishMqtt) publishMqtt();
#endif

  if (transitionActive && strip.getTransition() > 0) {
    float tper = (millis() - transitionStartTime)/(float)strip.getTransition();
    if (tper >= 1.0f) {
      strip.setTransitionMode(false); // stop all transitions
      // restore (global) transition time if not called from UDP notifier or single/temporary transition from JSON (also playlist)
      if (jsonTransitionOnce) strip.setTransition(transitionDelay);
      transitionActive = false;
      jsonTransitionOnce = false;
      tperLast = 0;
      applyFinalBri();
      return;
    }
    if (tper - tperLast < 0.004f) return;
    tperLast = tper;
    briT = briOld + ((bri - briOld) * tper);

    applyBri();
  }
}


// legacy method, applies values from col, effectCurrent, ... to selected segments
void colorUpdated(byte callMode) {
  applyValuesToSelectedSegs();
  stateUpdated(callMode);
}


void handleNightlight()
{
  static unsigned long lastNlUpdate;
  unsigned long now = millis();
  if (now < 100 && lastNlUpdate > 0) lastNlUpdate = 0; // take care of millis() rollover
  if (now - lastNlUpdate < 100) return; // allow only 10 NL updates per second
  lastNlUpdate = now;

  if (nightlightActive)
  {
    if (!nightlightActiveOld) //init
    {
      nightlightStartTime = millis();
      nightlightDelayMs = (int)(nightlightDelayMins*60000);
      nightlightActiveOld = true;
      briNlT = bri;
      for (byte i=0; i<4; i++) colNlT[i] = col[i]; // remember starting color
      if (nightlightMode == NL_MODE_SUN)
      {
        //save current
        colNlT[0] = effectCurrent;
        colNlT[1] = effectSpeed;
        colNlT[2] = effectPalette;

        strip.setMode(strip.getFirstSelectedSegId(), FX_MODE_STATIC); // make sure seg runtime is reset if it was in sunrise mode
        effectCurrent = FX_MODE_SUNRISE;
        effectSpeed = nightlightDelayMins;
        effectPalette = 0;
        if (effectSpeed > 60) effectSpeed = 60; //currently limited to 60 minutes
        if (bri) effectSpeed += 60; //sunset if currently on
        briNlT = !bri; //true == sunrise, false == sunset
        if (!bri) bri = briLast;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
    }
    float nper = (millis() - nightlightStartTime)/((float)nightlightDelayMs);
    if (nightlightMode == NL_MODE_FADE || nightlightMode == NL_MODE_COLORFADE)
    {
      bri = briNlT + ((nightlightTargetBri - briNlT)*nper);
      if (nightlightMode == NL_MODE_COLORFADE)                                         // color fading only is enabled with "NF=2"
      {
        for (byte i=0; i<4; i++) col[i] = colNlT[i]+ ((colSec[i] - colNlT[i])*nper);   // fading from actual color to secondary color
      }
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    if (nper >= 1) //nightlight duration over
    {
      nightlightActive = false;
      if (nightlightMode == NL_MODE_SET)
      {
        bri = nightlightTargetBri;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
      if (bri == 0) briLast = briNlT;
      if (nightlightMode == NL_MODE_SUN)
      {
        if (!briNlT) { //turn off if sunset
          effectCurrent = colNlT[0];
          effectSpeed = colNlT[1];
          effectPalette = colNlT[2];
          toggleOnOff();
          applyFinalBri();
        }
      }

      if (macroNl > 0)
        applyPreset(macroNl);
      nightlightActiveOld = false;
    }
  } else if (nightlightActiveOld) //early de-init
  {
    if (nightlightMode == NL_MODE_SUN) { //restore previous effect
      effectCurrent = colNlT[0];
      effectSpeed = colNlT[1];
      effectPalette = colNlT[2];
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    nightlightActiveOld = false;
  }
}

//utility for FastLED to use our custom timer
uint32_t get_millisecond_timer()
{
  return strip.now;
}
