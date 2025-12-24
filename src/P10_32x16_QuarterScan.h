/*
 * P10_32x16_QuarterScan.h
 * 
 * Driver para painéis P10 RGB LED 32x16 outdoor 1/4 scan SMD3535
 * 
 * Desenvolvido para resolver o problema de mapeamento de pixels em painéis
 * P10 outdoor com scan 1/4, que requerem remapeamento especial de coordenadas
 * para funcionar corretamente com a biblioteca ESP32-HUB75-MatrixPanel-I2S-DMA.
 * 
 * Hardware suportado:
 * - P10 Outdoor 32x16 SMD3535 RGB LED Module
 * - 1/4 Scan (Constant Current)
 * - HUB75 Interface
 * - Resolução: 32x16 pixels (320x160mm)
 * 
 * Autor: [Seu Nome]
 * Licença: MIT
 * Data: Dezembro 2024
 */

#ifndef P10_32X16_QUARTERSCAN_H
#define P10_32X16_QUARTERSCAN_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/**
 * @brief Classe wrapper para painéis P10 32x16 1/4 scan
 * 
 * Esta classe fornece um wrapper em torno da biblioteca ESP32-HUB75-MatrixPanel-I2S-DMA
 * com remapeamento automático de coordenadas para painéis P10 outdoor 1/4 scan.
 * 
 * O painel P10 32x16 1/4 scan tem uma arquitetura especial onde:
 * - As linhas físicas 0-7 usam R1/G1/B1
 * - As linhas físicas 8-15 usam R2/G2/B2
 * - Há um swap de blocos de 4 linhas dentro de cada metade
 * - O eixo X também requer remapeamento especial
 */
class P10_32x16_QuarterScan {
private:
  MatrixPanel_I2S_DMA *baseDisplay;
  
  /**
   * @brief Remapeia coordenada Y para o espaço físico do painel
   * @param y Coordenada Y lógica (0-15)
   * @return Coordenada Y física remapeada (0-7)
   */
  int16_t remapY(int16_t y);
  
  /**
   * @brief Remapeia coordenada X para o espaço físico do painel
   * @param x Coordenada X lógica (0-31)
   * @param mappedY Coordenada Y já remapeada
   * @return Coordenada X física remapeada
   */
  int16_t remapX(int16_t x, int16_t mappedY);
  
public:
  /**
   * @brief Construtor
   * @param display Ponteiro para instância de MatrixPanel_I2S_DMA já inicializada
   */
  P10_32x16_QuarterScan(MatrixPanel_I2S_DMA *display);
  
  /**
   * @brief Desenha um pixel
   * @param x Coordenada X (0-31)
   * @param y Coordenada Y (0-15)
   * @param color Cor em formato RGB565
   */
  void drawPixel(int16_t x, int16_t y, uint16_t color);

  void writePixel(int16_t x, int16_t y, uint16_t color);
  
  /**
   * @brief Desenha uma linha
   * @param x0 Coordenada X inicial
   * @param y0 Coordenada Y inicial
   * @param x1 Coordenada X final
   * @param y1 Coordenada Y final
   * @param color Cor em formato RGB565
   */
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  
  /**
   * @brief Preenche um retângulo
   * @param x Coordenada X do canto superior esquerdo
   * @param y Coordenada Y do canto superior esquerdo
   * @param w Largura
   * @param h Altura
   * @param color Cor em formato RGB565
   */
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  
  /**
   * @brief Desenha um retângulo (contorno)
   * @param x Coordenada X do canto superior esquerdo
   * @param y Coordenada Y do canto superior esquerdo
   * @param w Largura
   * @param h Altura
   * @param color Cor em formato RGB565
   */
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  
  /**
   * @brief Preenche a tela inteira com uma cor
   * @param color Cor em formato RGB565
   */
  void fillScreen(uint16_t color);
  
  /**
   * @brief Limpa a tela (preenche com preto)
   */
  void clearScreen();
  
  /**
   * @brief Converte valores RGB (0-255) para formato RGB565
   * @param r Vermelho (0-255)
   * @param g Verde (0-255)
   * @param b Azul (0-255)
   * @return Cor em formato RGB565
   */
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
  
  /**
   * @brief Define a posição do cursor de texto
   * @param x Coordenada X
   * @param y Coordenada Y
   */
  void setCursor(int16_t x, int16_t y);
  
  /**
   * @brief Define a cor do texto
   * @param c Cor em formato RGB565
   */
  void setTextColor(uint16_t c);
  
  /**
   * @brief Define o tamanho do texto
   * @param s Tamanho (1 = normal, 2 = 2x, etc)
   */
  void setTextSize(uint8_t s);
  
  /**
   * @brief Imprime texto na posição do cursor
   * @param text String a imprimir
   * @note Texto só funciona corretamente nas linhas 0-7 atualmente
   */
  void print(const char* text);
  
  /**
   * @brief Define o brilho do painel
   * @param brightness Brilho (0-255)
   */
  void setBrightness(uint8_t brightness);
  
  /**
   * @brief Retorna a largura do painel
   * @return 32 pixels
   */
  int16_t width() { return 32; }
  
  /**
   * @brief Retorna a altura do painel
   * @return 16 pixels
   */
  int16_t height() { return 16; }
};

#endif // P10_32X16_QUARTERSCAN_H
