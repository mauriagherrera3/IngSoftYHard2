#include <Arduino.h>
#include <WiFi.h>
#include "Boton.hpp"
#include "BombaAgua.hpp"
#include "Calentador.hpp"
#include "SensorTemp.hpp"
#include "SensorAgua.hpp"
#include "ApiClient.hpp"
#include "ControladorMate.hpp" // <-- Nuestro nuevo cerebro

// ============================================================
//  CONFIGURACIÓN
// ============================================================
const char* ssid       = "Fibertel WiFi324 2.4GHz";
const char* password   = "Andres607670";
const char* urlBackend = "http://192.168.0.96:8080";

const int PIN_POTE   = 36;
const int PIN_BOTON1 = 32; // Cebar
const int PIN_BOTON2 = 25; // ON/OFF
const int PIN_TEMP   = 33;
const int PIN_AGUA   = 39;
const int PIN_CALEN  = 4;
const int PIN_BOMBA  = 13;

const unsigned long INTERVALO_TELEMETRIA = 10000;
const unsigned long INTERVALO_LOG        = 1000;

// ============================================================
//  PUNTEROS GLOBALES (Hardware y Control)
// ============================================================
ApiClient* api         = nullptr;
SensorAgua* sensorNivel = nullptr;
SensorTemp* termometro  = nullptr;
Calentador* resistencia = nullptr;
BombaAgua* bomba       = nullptr;
Boton* botonCebar  = nullptr;
Boton* botonOnOff  = nullptr;
ControladorMate* cerebro     = nullptr;

float obtenerTempDeseada() {
  return map(analogRead(PIN_POTE), 0, 4095, 40, 90);
}

// ============================================================
//  COLA FREERTOS Y EXTERN API CALLBACKS
// ============================================================
struct TareaApi {
  enum Tipo { TELEMETRIA, EVENTO, SESSION_START, SESSION_STOP } tipo;
  float temp, target, water;
  char  evento[32];
  int   totalPours;
};

QueueHandle_t colaApi;

void tareaEnvioApi(void*) {
  TareaApi t;
  for (;;) {
    if (xQueueReceive(colaApi, &t, portMAX_DELAY) == pdTRUE && api) {
      switch (t.tipo) {
        case TareaApi::TELEMETRIA:    api->enviarTelemetria(t.temp, t.target, t.water); break;
        case TareaApi::EVENTO:        api->enviarEvento(String(t.evento)); break;
        case TareaApi::SESSION_START: api->abrirSesion(); break;
        case TareaApi::SESSION_STOP:  api->cerrarSesion(t.totalPours); break;
      }
    }
  }
}

void encolarTelemetria(float temp, float target, float water) {
  if (!api) return;
  TareaApi t; t.tipo = TareaApi::TELEMETRIA; t.temp = temp; t.target = target; t.water = water;
  xQueueSend(colaApi, &t, 0);
}

void encolarEvento(const char* nombre) {
  if (!api) return;
  TareaApi t; t.tipo = TareaApi::EVENTO; strncpy(t.evento, nombre, sizeof(t.evento) - 1);
  xQueueSend(colaApi, &t, 0);
}

void encolarSessionStart() {
  if (!api) return;
  TareaApi t; t.tipo = TareaApi::SESSION_START;
  xQueueSend(colaApi, &t, 0);
}

void encolarSessionStop(int totalPours) {
  if (!api) return;
  TareaApi t; t.tipo = TareaApi::SESSION_STOP; t.totalPours = totalPours;
  xQueueSend(colaApi, &t, 0);
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_CALEN, OUTPUT);
  digitalWrite(PIN_CALEN, LOW);
  delay(2000);
  Serial.println("\n=== BOOT: Iniciando hardware ===");

  sensorNivel = new SensorAgua(PIN_AGUA, 0, 2000);
  termometro  = new SensorTemp(PIN_TEMP);
  resistencia = new Calentador(PIN_CALEN);
  bomba       = new BombaAgua(PIN_BOMBA);
  botonCebar  = new Boton(PIN_BOTON1);
  botonOnOff  = new Boton(PIN_BOTON2);
  pinMode(PIN_POTE, INPUT);

  // Instanciamos el controlador inyectándole sus dependencias
  cerebro = new ControladorMate(resistencia, bomba, botonCebar);

  termometro->iniciar();

  colaApi = xQueueCreate(8, sizeof(TareaApi));
  xTaskCreatePinnedToCore(tareaEnvioApi, "tareaApi", 8192, nullptr, 1, nullptr, 0);

  Serial.println("Hardware inicializado OK.");
  WiFi.mode(WIFI_STA); 
  WiFi.disconnect(); 
  delay(100);
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500); Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
    api = new ApiClient(urlBackend);
  } else {
    Serial.println("\nWiFi NO conectado. Continuando sin API.");
  }

  Serial.println("--- SISTEMA LISTO. Presioná Botón 2 para iniciar ---\n");
}

// ============================================================
//  LOOP MINIMALISTA
// ============================================================
unsigned long tiempoUltimaTelemetria = 0;
unsigned long tiempoUltimoLog        = 0;

void loop() {
  // 1. Lectura del entorno
  float tempActual  = termometro->obtenerTemperatura();
  float tempDeseada = obtenerTempDeseada();
  float nivelAgua   = sensorNivel->obtenerPorcentaje();
  bool  hayAgua     = sensorNivel->hayAguaSuficiente();
  bool  sensorOk    = (tempActual >= 0.0f);

  // 2. Control General (Prioridad)
  if (botonOnOff->fuePresionado()) {
    cerebro->toggleEncendido();
    return;
  }

  // 3. Dejamos que el cerebro procese el ciclo
  cerebro->actualizar(tempActual, tempDeseada, hayAgua, sensorOk);

  // 4. Telemetría y Logs
  if (cerebro->getEstado() != ControladorMate::Estado::APAGADO &&
      cerebro->getEstado() != ControladorMate::Estado::ENCENDIDO &&
      millis() - tiempoUltimaTelemetria >= INTERVALO_TELEMETRIA) {
    encolarTelemetria(tempActual, tempDeseada, nivelAgua);
    tiempoUltimaTelemetria = millis();
  }

  if (millis() - tiempoUltimoLog >= INTERVALO_LOG) {
    Serial.print("[");
    Serial.print(cerebro->getNombreEstado());
    Serial.print("] Agua: "); Serial.print(nivelAgua, 0);
    Serial.print("% | Temp: ");
    if (sensorOk) { Serial.print(tempActual, 1); Serial.print("°C"); }
    else          { Serial.print("ERROR"); }
    Serial.print(" | Objetivo: "); Serial.print(tempDeseada, 1);
    Serial.print("°C | Cebadas: "); Serial.print(cerebro->getCebosSesion());
    Serial.print(" (Hist: "); Serial.print(cerebro->getCebosHistoricos()); Serial.println(")");
    
    tiempoUltimoLog = millis();
  }

  delay(20);
}