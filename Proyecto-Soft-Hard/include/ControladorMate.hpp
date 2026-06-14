#pragma once

#ifdef NATIVE_TEST
  #include "mock_arduino.hpp"
  // En modo test, Calentador/BombaAgua/Boton vienen del mock_hardware.hpp
  // que ya fue incluido antes en test_controlador.cpp — NO los incluimos acá
#else
  #include <Arduino.h>
  #include "Calentador.hpp"
  #include "BombaAgua.hpp"
  #include "Boton.hpp"
#endif

// Le avisamos al compilador que estas funciones existen en main.cpp
extern void encolarEvento(const char* nombre);
extern void encolarSessionStart();
extern void encolarSessionStop(int totalPours);

class ControladorMate {
  public:
    enum class Estado {
      APAGADO,
      ENCENDIDO,
      CALENTANDO,
      NO_CALENTANDO
    };

  private:
    Estado      estadoActual;
    Calentador* resistencia;
    BombaAgua* bomba;
    Boton* botonCebar;

    int cebosDeSesion;
    int cebosHistoricos;

    const float         HISTERESIS      = 5.0f;
    const unsigned long TIEMPO_BOMBA_MS = 6000;

    void ejecutarCebado(float tempActual) {
      bool calentadorEstabaEncendido = resistencia->estaEncendido();
      if (calentadorEstabaEncendido) resistencia->apagar();

      cebosDeSesion++;
      Serial.println("\n=================================");
      Serial.print(" CEBANDO MATE N°");
      Serial.println(cebosDeSesion);
      Serial.print(" Temperatura: ");
      Serial.print(tempActual, 1);
      Serial.println(" °C");
      Serial.println("=================================\n");

      bomba->encender();
      delay(TIEMPO_BOMBA_MS);
      bomba->apagar();

      if (calentadorEstabaEncendido) resistencia->encender();
      Serial.println("[BOMBA] Cebado completado.");
    }

  public:
    // Constructor: Inyectamos las dependencias de hardware
    ControladorMate(Calentador* calen, BombaAgua* bmb, Boton* btnCebar)
      : resistencia(calen), bomba(bmb), botonCebar(btnCebar),
        estadoActual(Estado::APAGADO), cebosDeSesion(0), cebosHistoricos(0) {}

    // Transición de ON/OFF del sistema general
    void toggleEncendido() {
      if (estadoActual == Estado::APAGADO) {
        transicionarA(Estado::ENCENDIDO);
      } else {
        transicionarA(Estado::APAGADO);
      }
    }

    // El cerebro de la máquina de estados
    void actualizar(float tempActual, float tempDeseada, bool hayAgua, bool sensorOk) {
      switch (estadoActual) {
        case Estado::APAGADO:
          break;

        case Estado::ENCENDIDO:
          if (hayAgua && sensorOk && tempActual < (tempDeseada - HISTERESIS)) {
            transicionarA(Estado::CALENTANDO);
          } else {
            transicionarA(Estado::NO_CALENTANDO);
          }
          break;

        case Estado::CALENTANDO:
          if (!hayAgua || !sensorOk) {
            transicionarA(Estado::NO_CALENTANDO);
            Serial.println("[SEGURIDAD] Calentador apagado: condición de seguridad.");
            break;
          }
          if (tempActual >= tempDeseada) {
            transicionarA(Estado::NO_CALENTANDO);
            break;
          }
          if (botonCebar->fuePresionado()) {
            if (hayAgua) ejecutarCebado(tempActual);
            else Serial.println("[ALERTA] Sin agua para cebar.");
          }
          break;

        case Estado::NO_CALENTANDO:
          if (hayAgua && sensorOk && tempActual < (tempDeseada - HISTERESIS)) {
            transicionarA(Estado::CALENTANDO);
            break;
          }
          if (botonCebar->fuePresionado()) {
            if (hayAgua) ejecutarCebado(tempActual);
            else Serial.println("[ALERTA] Sin agua para cebar.");
          }
          break;
      }
    }

    void transicionarA(Estado nuevo) {
      if (nuevo == estadoActual) return;

      Serial.print("[STATE] ");
      Serial.print(getNombreEstado());
      estadoActual = nuevo; // Actualizamos antes para imprimir bien el nuevo estado
      Serial.print(" → ");
      Serial.println(getNombreEstado());

      // Acciones automáticas al entrar a un nuevo estado
      switch (estadoActual) {
        case Estado::APAGADO:
          resistencia->apagar();
          cebosHistoricos += cebosDeSesion;
          encolarSessionStop(cebosDeSesion);
          Serial.print("[SESIÓN] Cerrada. Cebadas sesión: ");
          Serial.print(cebosDeSesion);
          Serial.print(" | Históricas: ");
          Serial.println(cebosHistoricos);
          cebosDeSesion = 0;
          break;

        case Estado::ENCENDIDO:
          cebosDeSesion = 0;
          encolarSessionStart();
          Serial.println("[SISTEMA] Encendido. Evaluando condiciones...");
          break;

        case Estado::CALENTANDO:
          resistencia->encender();
          encolarEvento("HEATING_STARTED");
          Serial.println("[TERMOSTATO] Calentando...");
          break;

        case Estado::NO_CALENTANDO:
          resistencia->apagar();
          encolarEvento("HEATING_STOPPED");
          Serial.println("[TERMOSTATO] Temperatura alcanzada / en espera.");
          break;
      }
    }

    // Getters para la telemetría y logs
    Estado getEstado() { return estadoActual; }
    int getCebosSesion() { return cebosDeSesion; }
    int getCebosHistoricos() { return cebosHistoricos; }
    
    const char* getNombreEstado() {
      switch (estadoActual) {
        case Estado::APAGADO:       return "APAGADO";
        case Estado::ENCENDIDO:     return "ENCENDIDO";
        case Estado::CALENTANDO:    return "CALENTANDO";
        case Estado::NO_CALENTANDO: return "NO_CALENTANDO";
        default:                    return "DESCONOCIDO";
      }
    }
};