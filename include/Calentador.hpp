#pragma once
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif

// Controla la resistencia calentadora a través de un MOSFET IRLZ44N (Active HIGH).
// El MOSFET conduce cuando el pin de Gate recibe HIGH (3.3V del ESP32).
class Calentador {
  private:
    int  pinGate;
    bool estado;

  public:
    Calentador(int pin) : pinGate(pin), estado(false) {
      pinMode(pinGate, OUTPUT);
      digitalWrite(pinGate, LOW); // Asegurar MOSFET apagado al arrancar
    }

    void encender() {
      digitalWrite(pinGate, HIGH);
      estado = true;
    }

    void apagar() {
      digitalWrite(pinGate, LOW);
      estado = false;
    }

    bool estaEncendido() const { return estado; }
};