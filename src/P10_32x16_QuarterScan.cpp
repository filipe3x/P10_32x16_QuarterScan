/*
 * P10_32x16_QuarterScan.cpp
 * VERSÃO FINAL CORRIGIDA
 */

#include "P10_32x16_QuarterScan.h"

P10_32x16_QuarterScan::P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display) 
  : Adafruit_GFX(32, 16) {
  baseDisplay = display;
}

int16_t P10_32x16_QuarterScan::remapY(int16_t y) {
  // Este estava correto - não mexer!
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
  // VERSÃO CORRIGIDA - baseada no código original mas SEM duplicação
  uint8_t pxbase = 8;
  int16_t result;
  
  if ((mappedY & 4) == 0) {
    // Linhas 0-3 (depois do remapY): espelhar dentro de blocos de 8
    // ORIGINAL: (x / pxbase) * 2 * pxbase + pxbase + 7 - (x & 0x7)
    // PROBLEMA: o *2 causa duplicação!
    // CORREÇÃO: remover o *2
    result = (x / pxbase) * pxbase + pxbase + 7 - (x & 0x7);
  } else {
    // Linhas 4-7 (depois do remapY): manter original
    result = x + (x / pxbase) * pxbase;
  }
  
  // IMPORTANTE: garantir que fica dentro de 0-31
  return result % 32;
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
