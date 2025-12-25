/*
 * P10_32x16_QuarterScan.cpp
 *
 * SOLUÇÃO BASEADA NA ISSUE #680:
 * https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/680
 *
 * CONCEITO CHAVE:
 * - Display base configurado como 64x8 (não 32x16!)
 * - Isto dá 64 endereços de coluna únicos, eliminando duplicação
 * - Fórmula #680 mapeia coordenadas lógicas 32x16 → driver 64x8
 */

#include "P10_32x16_QuarterScan.h"

P10_32x16_QuarterScan::P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display)
  : Adafruit_GFX(32, 16) {
  baseDisplay = display;
}

void P10_32x16_QuarterScan::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= 32 || y < 0 || y >= 16) return;

  // ============================================================
  // FÓRMULA #680 - Mapeamento 32x16 lógico → 64x8 driver
  // ============================================================
  //
  // IMPORTANTE: O display base DEVE estar configurado como 64x8!
  // HUB75_I2S_CFG mxconfig(64, 8, 1, pins);
  //
  // Tabela de mapeamento Y (lógico → driver):
  // Y lógico | driverY | Linha física
  // ---------|---------|-------------
  //    0     |    0    |     1
  //    1     |    1    |     2
  //    2     |    2    |     3
  //    3     |    3    |     4
  //    4     |    0    |     5 (via X offset)
  //    5     |    1    |     6
  //    6     |    2    |     7
  //    7     |    3    |     8
  //    8     |    4    |     9
  //   ...    |   ...   |    ...
  //   15     |    7    |    16
  //
  // Tabela de mapeamento X (com pxbase=8):
  // X lógico | y&4==0 (linhas 0-3,8-11) | y&4!=0 (linhas 4-7,12-15)
  // ---------|--------------------------|---------------------------
  //   0-7    |          0-7             |          8-15
  //   8-15   |         16-23            |         24-31
  //  16-23   |         32-39            |         40-47
  //  24-31   |         48-55            |         56-63
  // ============================================================

  const uint8_t pxbase = 8;
  int16_t driverX, driverY;

  // Transformação Y: comprime 16 linhas em 8
  // Fórmula: (y >> 3) * 4 + (y & 0b11)
  // - y >> 3: qual metade (0 para y<8, 1 para y>=8)
  // - y & 0b11: posição dentro do grupo de 4 (0-3)
  driverY = ((y >> 3) * 4) + (y & 0b00000011);

  // Transformação X com pxbase=8
  // A condição y&4 determina em qual "faixa" de colunas desenhar
  if ((y & 4) == 0) {
    // Linhas 0-3 e 8-11: x += (x/8)*8
    // Exemplo: x=10 → 10 + (10/8)*8 = 10 + 8 = 18
    driverX = x + (x / pxbase) * pxbase;
  } else {
    // Linhas 4-7 e 12-15: x += ((x/8)+1)*8
    // Exemplo: x=10 → 10 + ((10/8)+1)*8 = 10 + 16 = 26
    driverX = x + ((x / pxbase) + 1) * pxbase;
  }

  baseDisplay->drawPixel(driverX, driverY, color);
}

void P10_32x16_QuarterScan::fillScreen(uint16_t color) {
  // Usar implementação de Adafruit_GFX que chama drawPixel para cada pixel
  // Isto garante que o mapeamento é aplicado corretamente
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
