#include <unity.h>

// Incluir mocks ANTES que ControladorMate para que no use Arduino.h real
#include "mock_arduino.hpp"
#include "mock_hardware.hpp"

// Stubs de las funciones extern que ControladorMate llama
void encolarEvento(const char*) {}
void encolarSessionStart()      {}
void encolarSessionStop(int)    {}

#include "ControladorMate.hpp"

// ── Fixtures ──────────────────────────────────────────────────
Calentador* cal;
BombaAgua*  bmb;
Boton*      btn;
ControladorMate* ctrl;

void setUp() {
  cal  = new Calentador(4);
  bmb  = new BombaAgua(13);
  btn  = new Boton(32);
  ctrl = new ControladorMate(cal, bmb, btn);
}

void tearDown() {
  delete ctrl;
  delete btn;
  delete bmb;
  delete cal;
}

// ── Tests ─────────────────────────────────────────────────────

// Estado inicial debe ser APAGADO
void test_estado_inicial_es_apagado() {
  TEST_ASSERT_EQUAL(ControladorMate::Estado::APAGADO, ctrl->getEstado());
}

// Toggle desde APAGADO → ENCENDIDO
void test_toggle_enciende_sistema() {
  ctrl->toggleEncendido();
  TEST_ASSERT_EQUAL(ControladorMate::Estado::ENCENDIDO, ctrl->getEstado());
}

// ENCENDIDO → CALENTANDO cuando temp < objetivo - histéresis
void test_encendido_pasa_a_calentando_si_fria() {
  ctrl->toggleEncendido(); // APAGADO → ENCENDIDO
  ctrl->actualizar(50.0f, 70.0f, true, true); // 50 < 70-5=65 → CALENTANDO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::CALENTANDO, ctrl->getEstado());
  TEST_ASSERT_TRUE(cal->estaEncendido());
}

// ENCENDIDO → NO_CALENTANDO cuando temp ya está cerca del objetivo
void test_encendido_pasa_a_no_calentando_si_caliente() {
  ctrl->toggleEncendido();
  ctrl->actualizar(68.0f, 70.0f, true, true); // 68 >= 70-5=65 → NO_CALENTANDO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::NO_CALENTANDO, ctrl->getEstado());
  TEST_ASSERT_FALSE(cal->estaEncendido());
}

// CALENTANDO → NO_CALENTANDO cuando alcanza temperatura objetivo
void test_calentando_para_al_llegar_a_objetivo() {
  ctrl->toggleEncendido();
  ctrl->actualizar(50.0f, 70.0f, true, true); // → CALENTANDO
  ctrl->actualizar(70.0f, 70.0f, true, true); // temp >= objetivo → NO_CALENTANDO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::NO_CALENTANDO, ctrl->getEstado());
  TEST_ASSERT_FALSE(cal->estaEncendido());
}

// NO_CALENTANDO → CALENTANDO cuando baja 5°C
void test_no_calentando_vuelve_a_calentar_si_baja() {
  ctrl->toggleEncendido();
  ctrl->actualizar(68.0f, 70.0f, true, true); // → NO_CALENTANDO
  ctrl->actualizar(64.0f, 70.0f, true, true); // 64 < 70-5=65 → CALENTANDO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::CALENTANDO, ctrl->getEstado());
}

// Seguridad: sin agua → calentador se apaga
void test_sin_agua_apaga_calentador() {
  ctrl->toggleEncendido();
  ctrl->actualizar(50.0f, 70.0f, true, true);  // → CALENTANDO
  ctrl->actualizar(50.0f, 70.0f, false, true); // sin agua → NO_CALENTANDO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::NO_CALENTANDO, ctrl->getEstado());
  TEST_ASSERT_FALSE(cal->estaEncendido());
}

// Seguridad: sensor inválido → calentador se apaga
void test_sensor_invalido_apaga_calentador() {
  ctrl->toggleEncendido();
  ctrl->actualizar(50.0f, 70.0f, true, true);  // → CALENTANDO
  ctrl->actualizar(-1.0f, 70.0f, true, false); // sensor inválido → NO_CALENTANDO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::NO_CALENTANDO, ctrl->getEstado());
}

// Toggle desde estado activo → APAGADO y resetea cebadas
void test_toggle_apaga_y_resetea_contador() {
  ctrl->toggleEncendido(); // → ENCENDIDO
  ctrl->actualizar(50.0f, 70.0f, true, true); // → CALENTANDO
  ctrl->toggleEncendido(); // → APAGADO
  TEST_ASSERT_EQUAL(ControladorMate::Estado::APAGADO, ctrl->getEstado());
  TEST_ASSERT_EQUAL(0, ctrl->getCebosSesion());
  TEST_ASSERT_FALSE(cal->estaEncendido());
}

// Contador de cebadas arranca en cero
void test_contador_cebadas_inicia_en_cero() {
  TEST_ASSERT_EQUAL(0, ctrl->getCebosSesion());
  TEST_ASSERT_EQUAL(0, ctrl->getCebosHistoricos());
}

// Nombre de estado correcto
void test_nombre_estado_apagado() {
  TEST_ASSERT_EQUAL_STRING("APAGADO", ctrl->getNombreEstado());
}

void test_nombre_estado_calentando() {
  ctrl->toggleEncendido();
  ctrl->actualizar(50.0f, 70.0f, true, true);
  TEST_ASSERT_EQUAL_STRING("CALENTANDO", ctrl->getNombreEstado());
}

// ── Main ──────────────────────────────────────────────────────
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_estado_inicial_es_apagado);
  RUN_TEST(test_toggle_enciende_sistema);
  RUN_TEST(test_encendido_pasa_a_calentando_si_fria);
  RUN_TEST(test_encendido_pasa_a_no_calentando_si_caliente);
  RUN_TEST(test_calentando_para_al_llegar_a_objetivo);
  RUN_TEST(test_no_calentando_vuelve_a_calentar_si_baja);
  RUN_TEST(test_sin_agua_apaga_calentador);
  RUN_TEST(test_sensor_invalido_apaga_calentador);
  RUN_TEST(test_toggle_apaga_y_resetea_contador);
  RUN_TEST(test_contador_cebadas_inicia_en_cero);
  RUN_TEST(test_nombre_estado_apagado);
  RUN_TEST(test_nombre_estado_calentando);
  return UNITY_END();
}