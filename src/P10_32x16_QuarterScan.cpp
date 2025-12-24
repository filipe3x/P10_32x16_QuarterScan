/*
 * P10_32x16_QuarterScan.cpp
 * 
 * Implementação do driver para painéis P10 32x16 1/4 scan
 */

#include "P10_32x16_QuarterScan.h"

P10_32x16_QuarterScan::P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display) {
  baseDisplay = display;
}

int16_t P10_32x16_QuarterScan::remapY(int16_t y) {
  // Linhas 0-7 e 8-15 mapeiam para as mesmas 8 linhas físicas
  // mas usam RGB1 vs RGB2
  int lineInHalf = y % 8;
  
  // Dentro de cada metade, trocar blocos de 4
  int quad = (lineInHalf / 4) % 2;
  int posInQuad = lineInHalf % 4;
  
  if (quad == 0) {
    // Linhas 0-3 → posições 4-7
    return posInQuad + 4;
  } else {
    // Linhas 4-7 → posições 0-3
    return posInQuad;
  }
}

int16_t P10_32x16_QuarterScan::remapX(int16_t x, int16_t mappedY) {
  uint8_t pxbase = 8;
  
  if ((mappedY & 4) == 0) {
    return (x / pxbase) * 2 * pxbase + pxbase + 7 - (x & 0x7);
  } else {
    return x + (x / pxbase) * pxbase;
  }
}

void P10_32x16_QuarterScan::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= 32 || y < 0 || y >= 16) return;
  
  int16_t mappedY = remapY(y);
  int16_t mappedX = remapX(x, mappedY);
  
  if (y < 8) {
    // Linhas 0-7: usar R1/G1/B1 (metade superior)
    baseDisplay->drawPixel(mappedX, mappedY, color);
  } else {
    // Linhas 8-15: usar R2/G2/B2 (metade inferior)
    baseDisplay->drawPixel(mappedX, mappedY + 8, color);
  }
}

void P10_32x16_QuarterScan::writePixel(int16_t x, int16_t y, uint16_t color) {
  // writePixel é chamado por print() e outras funções de texto
  // Redirecionar para drawPixel que já tem o remapeamento!
  drawPixel(x, y, color);
}

void P10_32x16_QuarterScan::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  // Algoritmo de Bresenham
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  while(true) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}

void P10_32x16_QuarterScan::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  for(int16_t j = 0; j < h; j++) {
    for(int16_t i = 0; i < w; i++) {
      drawPixel(x + i, y + j, color);
    }
  }
}

void P10_32x16_QuarterScan::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  drawLine(x, y, x + w - 1, y, color);           // Topo
  drawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Direita
  drawLine(x + w - 1, y + h - 1, x, y + h - 1, color); // Fundo
  drawLine(x, y + h - 1, x, y, color);           // Esquerda
}

void P10_32x16_QuarterScan::fillScreen(uint16_t color) {
  for(int y = 0; y < 16; y++) {
    for(int x = 0; x < 32; x++) {
      drawPixel(x, y, color);
    }
  }
}

void P10_32x16_QuarterScan::clearScreen() {
  baseDisplay->clearScreen();
}

uint16_t P10_32x16_QuarterScan::color565(uint8_t r, uint8_t g, uint8_t b) {
  return baseDisplay->color565(r, g, b);
}

void P10_32x16_QuarterScan::setCursor(int16_t x, int16_t y) {
  baseDisplay->setCursor(x, y);
}

void P10_32x16_QuarterScan::setTextColor(uint16_t c) {
  baseDisplay->setTextColor(c);
}

void P10_32x16_QuarterScan::setTextSize(uint8_t s) {
  baseDisplay->setTextSize(s);
}

void P10_32x16_QuarterScan::print(const char* text) {
  baseDisplay->print(text);
}

void P10_32x16_QuarterScan::setBrightness(uint8_t brightness) {
  baseDisplay->setBrightness8(brightness);
}
