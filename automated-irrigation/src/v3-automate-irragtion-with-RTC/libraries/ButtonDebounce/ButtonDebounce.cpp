/*
  Button - a small library for Arduino to handle button debouncing
  MIT licensed.
*/

#include "ButtonDebounce.h"

ButtonDebounce::ButtonDebounce() {}

void ButtonDebounce::runFnDebounce(int reading, int* lastBtnState,
                                   int* btnState, unsigned long* lastDebounce,
                                   int delay, void (*fn)()) {
  unsigned long currentMillis = millis();

  if (reading != *lastBtnState) {
    *lastDebounce = currentMillis;
  }

  if (currentMillis - *lastDebounce >= delay) {
    if (reading != *btnState) {
      (*fn)();
    }
  }

  *lastBtnState = reading;
}
