/*
 * P10_32x16_QuarterScan.cpp
 */

#include "P10_32x16_QuarterScan.h"

P10_32x16_QuarterScan::P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display) 
  : Adafruit_GFX(32, 16) {
  baseDisplay = display;
}

int16_t P10_32x16_QuarterScan::remapY(int16_t y) {
  int lineInHalf = y % 8;
  int quad = (lineInHalf / 4) % 2;
  int posInQuad = lineInHalf % 4;
  
  if (quad == 0) {
    return posInQuad + 4;
  } else {
    return posInQuad;
  }
}

int16_t P10_32x16_QuarterScan::remapX(int16_t x, int16_t mappedY) {
  // NOVA VERSÃO SIMPLIFICADA
  // Baseado nos testes: linha horizontal funciona = X direto funciona!
  // Problema é só com texto = problema é complexidade do remapeamento
  
  // Tentar mapeamento mais simples:
  // Apenas inverter dentro de blocos de 8 se necessário
  
  if ((mappedY & 4) == 0) {
    // Linhas pares (0-3 depois do remap): inverter X dentro de blocos de 8
    int block = x / 8;
    int offset = x % 8;
    return block * 8 + (7 - offset);
  } else {
    // Linhas ímpares (4-7 depois do remap): X direto
    return x;
  }
}

void P10_32x16_QuarterScan::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= 32 || y < 0 || y >= 16) return;
  
  int16_t mappedY = remapY(y);
  int16_t mappedX = remapX(x, mappedY);
  
  if (y < 8) {
    baseDisplay->drawPixel(mappedX, mappedY, color);
  } else {
    baseDisplay->drawPixel(mappedX, mappedY + 8, color);
  }
}

void P10_32x16_QuarterScan::fillScreen(uint16_t color) {
  Adafruit_GFX::fillScreen(color);
}

void P10_32x16_QuarterScan::clearScreen() {
  baseDisplay->clearScreen();
}

uint16_t P10_32x16_QuarterScan::color565(uint8_t r, uint8_t g, uint8_t b) {
  return baseDisplay->color565(r, g, b);
}

void P10_32x16_QuarterScan::setBrightness(uint8_t brightness) {
  baseDisplay->setBrightness8(brightness);
}
