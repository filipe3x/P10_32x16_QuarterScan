/*
 * P10_32x16_QuarterScan.h
 *
 * Driver para painéis P10 RGB LED 32x16 outdoor 1/4 scan SMD3535
 * COM SUPORTE COMPLETO A TEXTO via herança Adafruit_GFX
 *
 * SOLUÇÃO BASEADA NA ISSUE #680:
 * https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/680
 *
 * IMPORTANTE: O display base DEVE ser configurado como 64x8 (não 32x16)!
 *
 * Exemplo de inicialização:
 * ```cpp
 * HUB75_I2S_CFG mxconfig(64, 8, 1, pins);  // 64x8, NÃO 32x16!
 * mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;
 * MatrixPanel_I2S_DMA *dma_display = new MatrixPanel_I2S_DMA(mxconfig);
 * dma_display->begin();
 *
 * P10_32x16_QuarterScan *panel = new P10_32x16_QuarterScan(dma_display);
 * panel->print("Hello!");  // Funciona!
 * ```
 */

#ifndef P10_32X16_QUARTERSCAN_H
#define P10_32X16_QUARTERSCAN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/**
 * @brief Classe wrapper para painéis P10 32x16 1/4 scan
 *
 * HERDA de Adafruit_GFX para suporte completo a texto e gráficos!
 *
 * Usa a fórmula #680 para mapear coordenadas lógicas 32x16 para
 * o espaço de driver 64x8, eliminando o problema de duplicação
 * de pixels que ocorre com configuração 32x16 padrão.
 */
class P10_32x16_QuarterScan : public Adafruit_GFX {
private:
  MatrixPanel_I2S_DMA *baseDisplay;

public:
  /**
   * @brief Construtor
   * @param display Ponteiro para instância de MatrixPanel_I2S_DMA
   *                DEVE estar configurado como 64x8!
   */
  P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display);

  /**
   * @brief Desenha um pixel (override de Adafruit_GFX)
   *
   * ESTA é a função que Adafruit_GFX chama para TUDO!
   * Implementa a fórmula #680 para mapeamento correto.
   *
   * @param x Coordenada X lógica (0-31)
   * @param y Coordenada Y lógica (0-15)
   * @param color Cor em formato RGB565
   */
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;

  /**
   * @brief Preenche a tela inteira com uma cor
   */
  void fillScreen(uint16_t color);

  /**
   * @brief Limpa a tela (preenche com preto)
   */
  void clearScreen();

  /**
   * @brief Converte valores RGB (0-255) para formato RGB565
   */
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  /**
   * @brief Define o brilho do painel (0-255)
   */
  void setBrightness(uint8_t brightness);
};

#endif // P10_32X16_QUARTERSCAN_H
