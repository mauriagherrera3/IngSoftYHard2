#pragma once
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif
class BotonYPote {
  private:
    int pinPote;
    int pinBoton;
    bool estadoAnteriorBoton;

  public:
    BotonYPote(int pinP, int pinB) {
      pinPote = pinP;
      pinBoton = pinB;
      pinMode(pinPote, INPUT);
      pinMode(pinBoton, INPUT_PULLUP);
      estadoAnteriorBoton = HIGH;
    }

    // Devuelve la temperatura deseada mapeada para el mate
    float obtenerTempDeseada() {
      int lectura = analogRead(pinPote);
      return map(lectura, 0, 4095, 40, 90); 
    }

    // Devuelve true SOLO en el instante que se presiona el botón
    bool fuePresionado() {
      bool estadoActual = digitalRead(pinBoton);
      bool presionado = (estadoAnteriorBoton == HIGH && estadoActual == LOW);
      estadoAnteriorBoton = estadoActual;
      
      if (presionado) delay(50); // Debounce por software
      return presionado;
    }
};