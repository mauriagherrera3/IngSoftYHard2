#pragma once
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif

class Boton {
  private:
    int  pin;
    bool estadoAnterior;
    
  public:
    Boton(int p) : pin(p), estadoAnterior(HIGH) {
      // Usamos la resistencia pull-up interna del ESP32
      pinMode(pin, INPUT_PULLUP);
    }
    
    bool fuePresionado() {
      bool actual     = digitalRead(pin);
      // Detecta el flanco de bajada (de HIGH a LOW)
      bool presionado = (estadoAnterior == HIGH && actual == LOW);
      estadoAnterior  = actual;
      
      // Debounce simple por software para evitar falsos positivos
      if (presionado) {
          delay(50); 
      }
      return presionado;
    }
};