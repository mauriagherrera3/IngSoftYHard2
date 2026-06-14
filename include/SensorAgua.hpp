#pragma once
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif

class SensorAgua {
  private:
    int pinADC;
    int valorVacio;
    int valorLleno;

  public:
    SensorAgua(int pin, int vacio, int lleno) {
      pinADC = pin;
      valorVacio = vacio;
      valorLleno = lleno;
      pinMode(pinADC, INPUT);
    }

    float obtenerPorcentaje() {
      int lectura = analogRead(pinADC);
      float porcentaje = map(lectura, valorVacio, valorLleno, 0, 100);
      
      // Limitamos los topes para que no de valores negativos o mayores a 100
      if (porcentaje < 0) porcentaje = 0;
      if (porcentaje > 100) porcentaje = 100;
      return porcentaje;
    }

    bool hayAguaSuficiente() {
      return obtenerPorcentaje() > 10.0; // Mínimo 10% para operar
    }
};