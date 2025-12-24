/*
 * P10_32x16_QuarterScan.h
 * 
 * Driver para painéis P10 RGB LED 32x16 outdoor 1/4 scan SMD3535
 * COM SUPORTE COMPLETO A TEXTO via herança Adafruit_GFX
 */

#ifndef P10_32X16_QUARTERSCAN_H
#define P10_32X16_QUARTERSCAN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/**
 * @brief Classe wrapper para painéis P10 32x16 1/4 scan
 * HERDA de Adafruit_GFX para suporte completo a texto!
 */
class P10_32x16_QuarterScan : public Adafruit_GFX {
private:
  MatrixPanel_I2S_DMA *baseDisplay;
  
  /**
   * @brief Remapeia coordenada Y para o espaço físico do painel
   */
  int16_t remapY(int16_t y);
  
  /**
   * @brief Remapeia coordenada X para o espaço físico do painel
   */
  int16_t remapX(int16_t x, int16_t mappedY);
  
public:
  /**
   * @brief Construtor
   * @param display Ponteiro para instância de MatrixPanel_I2S_DMA já inicializada
   */
  P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display);
  
  /**
   * @brief Desenha um pixel (override de Adafruit_GFX)
   * ESTA é a função que Adafruit_GFX chama para TUDO!
   */
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  
  /**
   * @brief Preenche a tela inteira (otimizado)
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
   * @brief Define o brilho do painel
   */
  void setBrightness(uint8_t brightness);
};

#endif // P10_32X16_QUARTERSCAN_H
