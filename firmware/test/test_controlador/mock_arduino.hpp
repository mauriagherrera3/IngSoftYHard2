#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>

#define HIGH 1
#define LOW  0
#define INPUT_PULLUP 0x02
#define OUTPUT 0x01

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int  digitalRead(int)       { return HIGH; }
inline int  analogRead(int)        { return 2048; }
inline void delay(unsigned long)   {}

// Serial con sobrecarga para print(valor, precision)
struct FakeSerial {
  template<typename T> void print(T)         {}
  template<typename T> void print(T, int)    {}  // ← para Serial.print(float, 1)
  template<typename T> void println(T)       {}
  template<typename T> void println(T, int)  {}
  void println()  {}
  void begin(int) {}
} Serial;

struct String {
  char buf[64];
  String()              { buf[0] = '\0'; }
  String(const char* s) { strncpy(buf, s, sizeof(buf)-1); buf[sizeof(buf)-1]='\0'; }
  const char* c_str() const { return buf; }
};