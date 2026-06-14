#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
#else
  #include <Arduino.h>
#endif

class ApiClient {
  private:
    const char* baseUrl;

    int postJson(const char* endpoint, String& body) {
      if (WiFi.status() != WL_CONNECTED) return -1;
      HTTPClient http;
      String url = String(baseUrl) + endpoint;
      http.begin(url);
      
      http.addHeader("Content-Type", "application/json");
      int code = http.POST(body);
      if (code > 0) {
        Serial.print("[API] POST ");
        Serial.print(endpoint);
        Serial.print(" → ");
        Serial.println(code);
      } else {
        Serial.print("[API] Error en ");
        Serial.print(endpoint);
        Serial.print(": ");
        Serial.println(http.errorToString(code).c_str());
      }
      http.end();
      return code;
    }

  public:
    ApiClient(const char* url) : baseUrl(url) {}

    // POST /telemetry
    void enviarTelemetria(float temp, float target, float water) {
      StaticJsonDocument<128> doc;
      doc["temperature"]       = temp;
      doc["targetTemperature"] = target;
      doc["waterLevel"]        = water;
      String body;
      serializeJson(doc, body);
      postJson("/telemetry", body);
    }

    // POST /events  —  solo acepta HEATING_STARTED | HEATING_STOPPED
    void enviarEvento(String tipo) {
      if (tipo != "HEATING_STARTED" && tipo != "HEATING_STOPPED") return;
      StaticJsonDocument<64> doc;
      doc["type"] = tipo;
      String body;
      serializeJson(doc, body);
      postJson("/events", body);
    }

    // POST /sessions  —  abre sesión SYSTEM_STARTED
    void abrirSesion() {
      StaticJsonDocument<64> doc;
      doc["sessionType"] = "SYSTEM_STARTED";
      doc["totalPours"]  = nullptr; // null según contrato
      String body;
      serializeJson(doc, body);
      postJson("/sessions", body);
    }

    // POST /sessions/finish  —  cierra sesión SYSTEM_STOPPED con totalPours
    void cerrarSesion(int totalPours) {
      StaticJsonDocument<64> doc;
      doc["sessionType"] = "SYSTEM_STOPPED";
      doc["totalPours"]  = totalPours;
      String body;
      serializeJson(doc, body);
      postJson("/sessions/finish", body);
    }
};