/*
 * P10_32x16_QuarterScan.cpp
 * 
 * Implementação do driver para painéis P10 32x16 1/4 scan
 * COM herança de Adafruit_GFX para texto funcionar!
 */

#include "P10_32x16_QuarterScan.h"

// Construtor - IMPORTANTE: Inicializar Adafruit_GFX com dimensões!
P10_32x16_QuarterScan::P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display) 
  : Adafruit_GFX(32, 16) {  // ← Dizer à Adafruit_GFX que somos 32x16
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

// ESTA é a função CRÍTICA que Adafruit_GFX chama para TUDO!
// Texto, linhas, retângulos, TUDO passa por aqui!
void P10_32x16_QuarterScan::drawPixel(int16_t x, int16_t y, uint16_t color) {
  // Validar limites
  if (x < 0 || x >= 32 || y < 0 || y >= 16) return;
  
  // Aplicar remapeamento
  int16_t mappedY = remapY(y);
  int16_t mappedX = remapX(x, mappedY);
  
  // Desenhar no display base
  if (y < 8) {
    // Linhas 0-7: usar R1/G1/B1 (metade superior)
    baseDisplay->drawPixel(mappedX, mappedY, color);
  } else {
    // Linhas 8-15: usar R2/G2/B2 (metade inferior)
    baseDisplay->drawPixel(mappedX, mappedY + 8, color);
  }
}

void P10_32x16_QuarterScan::fillScreen(uint16_t color) {
  // Usar função otimizada da classe base Adafruit_GFX
  // que chama drawPixel() para cada pixel
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
