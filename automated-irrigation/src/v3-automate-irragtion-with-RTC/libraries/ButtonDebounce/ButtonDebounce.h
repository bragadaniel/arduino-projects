/*
  Button - a small library for Arduino to handle button debouncing
  MIT licensed.
*/
#ifndef ButtonDebounce_h
#define ButtonDebounce_h

#include <Arduino.h>

class ButtonDebounce {
 public:
  ButtonDebounce();
  void runFnDebounce(int reading, int* lastBtnState, int* btnState,
                     unsigned long* lastDebounce, int delay, void (*fn)());
};

#endif