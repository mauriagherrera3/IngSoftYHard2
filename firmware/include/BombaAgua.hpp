#pragma once
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif

// Controla la bomba de agua a través de un módulo relay Active LOW.
// El relay activa su bobina cuando el pin IN recibe LOW (0V).
class BombaAgua {
  private:
    int pinRelay;

  public:
    BombaAgua(int pin) : pinRelay(pin) {
      pinMode(pinRelay, OUTPUT);
      digitalWrite(pinRelay, HIGH); // Relay en reposo (bobina desenergizada)
    }

    void encender() { digitalWrite(pinRelay, LOW);  }
    void apagar()   { digitalWrite(pinRelay, HIGH); }
};