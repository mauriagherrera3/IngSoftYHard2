// test/test_controlador/mock_hardware.hpp
#pragma once
#include "mock_arduino.hpp"

class Calentador {
  bool _on = false;
public:
  Calentador(int) {}
  void encender()          { _on = true; }
  void apagar()            { _on = false; }
  bool estaEncendido()     { return _on; }
};

class BombaAgua {
  bool _on = false;
public:
  BombaAgua(int) {}
  void encender() { _on = true; }
  void apagar()   { _on = false; }
  bool estaEncendida() { return _on; }
};

class Boton {
  bool _presionado = false;
public:
  Boton(int) {}
  void simularPresion()    { _presionado = true; }
  bool fuePresionado() {
    bool r = _presionado;
    _presionado = false;
    return r;
  }
};