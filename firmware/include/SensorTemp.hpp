#pragma once
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif
#include <OneWire.h>
#include <DallasTemperature.h>

// Sensor de temperatura digital DS18B20 (protocolo 1-Wire).
// Requiere resistencia pull-up de 4.7kΩ entre DATA y 3.3V.
// Usa conversión asíncrona para no bloquear el loop().
class SensorTemp {
  private:
    OneWire           oneWire;
    DallasTemperature sensor;
    unsigned long     tiempoUltimaConversion;
    float             ultimaTemp;
    // Resolución 9 bits → tiempo de conversión ~94ms
    static const int  RESOLUCION     = 9;
    static const long INTERVALO_MS   = 100;

  public:
    SensorTemp(int pin) : oneWire(pin), sensor(&oneWire) {
      tiempoUltimaConversion = 0;
      ultimaTemp = -1.0f;
    }

    // Llamar una vez en setup() después de crear el objeto
    void iniciar() {
      sensor.begin();

      if (sensor.getDeviceCount() == 0) {
        Serial.println("[SENSOR TEMP] ADVERTENCIA: Ningun DS18B20 detectado en el bus.");
        Serial.println("[SENSOR TEMP] Verificar cableado y resistencia pull-up 4.7k a 3.3V.");
        return;
      }

      sensor.setResolution(RESOLUCION);
      sensor.setWaitForConversion(false); // Modo asíncrono: no bloquea
      sensor.requestTemperatures();       // Dispara la primera conversión
      tiempoUltimaConversion = millis();

      Serial.print("[SENSOR TEMP] DS18B20 detectado. Sensores en bus: ");
      Serial.println(sensor.getDeviceCount());
    }

    // Llamar en cada loop(). Devuelve -1.0 si el sensor no está conectado.
    float obtenerTemperatura() {
      // Si ya pasó el tiempo de conversión, leer y disparar la siguiente
      if (millis() - tiempoUltimaConversion >= INTERVALO_MS) {
        float temp = sensor.getTempCByIndex(0);

        if (temp != DEVICE_DISCONNECTED_C && temp != 85.0f) {
          // 85.0°C es el valor de power-on reset del DS18B20, también inválido
          ultimaTemp = temp;
        } else {
          ultimaTemp = -1.0f;
        }

        sensor.requestTemperatures(); // Dispara la siguiente conversión
        tiempoUltimaConversion = millis();
      }

      return ultimaTemp;
    }
};