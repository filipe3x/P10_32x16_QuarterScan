# P10 32x16 Quarter Scan Driver

Driver para painéis P10 RGB LED outdoor 32x16 com 1/4 scan para ESP32.

## 🎯 Descrição

Esta biblioteca fornece um wrapper para a biblioteca `ESP32-HUB75-MatrixPanel-I2S-DMA` com remapeamento automático de coordenadas para painéis P10 outdoor 32x16 pixels com scan 1/4.

### Hardware Suportado

- **Modelo:** P10 Outdoor RGB LED Module
- **Resolução:** 32×16 pixels (320×160mm)
- **Tipo:** SMD3535
- **Scan:** 1/4 (Constant Current)
- **Interface:** HUB75
- **Controladores:** Compatível com Huidu, Novastar, Colorlight, Linsn

## 📦 Instalação

### Arduino IDE

1. Baixe a biblioteca como ZIP
2. No Arduino IDE: `Sketch` → `Include Library` → `Add .ZIP Library`
3. Selecione o arquivo ZIP baixado

### PlatformIO
```ini
lib_deps =
    https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
    https://github.com/seuuser/P10_32x16_QuarterScan
```

## 🔌 Pinout
```cpp
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN -1
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16
```

## 🚀 Uso Básico
```cpp
#include <P10_32x16_QuarterScan.h>

// Definir pinos
HUB75_I2S_CFG::i2s_pins _pins = {
  R1_PIN, G1_PIN, B1_PIN,
  R2_PIN, G2_PIN, B2_PIN,
  A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
  LAT_PIN, OE_PIN, CLK_PIN
};

// Configuração do display base
HUB75_I2S_CFG mxconfig(32, 16, 1, _pins);
mxconfig.clkphase = false;

// Criar display base
MatrixPanel_I2S_DMA *dma_display = new MatrixPanel_I2S_DMA(mxconfig);
dma_display->setBrightness8(100);
dma_display->begin();

// Criar wrapper P10
P10_32x16_QuarterScan *display = new P10_32x16_QuarterScan(dma_display);

// Usar o display!
display->fillScreen(display->color565(255, 0, 0)); // Vermelho
display->drawPixel(10, 5, display->color565(0, 255, 0)); // Pixel verde
display->drawLine(0, 0, 31, 15, display->color565(0, 0, 255)); // Linha azul
```

## 📚 API

### Desenho

- `drawPixel(x, y, color)` - Desenha um pixel
- `drawLine(x0, y0, x1, y1, color)` - Desenha uma linha
- `fillRect(x, y, w, h, color)` - Preenche um retângulo
- `drawRect(x, y, w, h, color)` - Desenha contorno de retângulo
- `fillScreen(color)` - Preenche tela inteira
- `clearScreen()` - Limpa tela (preto)

### Cor

- `color565(r, g, b)` - Converte RGB (0-255) para RGB565

### Texto

- `setCursor(x, y)` - Define posição do cursor
- `setTextColor(color)` - Define cor do texto
- `setTextSize(size)` - Define tamanho (1, 2, 3...)
- `print(text)` - Imprime texto

### Configuração

- `setBrightness(0-255)` - Define brilho
- `width()` - Retorna largura (32)
- `height()` - Retorna altura (16)

## 🔧 Exemplos

### Exemplo 1: Teste Simples
```cpp
void loop() {
  // Linha vermelha no topo
  display->clearScreen();
  display->drawLine(0, 0, 31, 0, display->color565(255, 0, 0));
  delay(1000);
  
  // Linha verde no meio
  display->clearScreen();
  display->drawLine(0, 8, 31, 8, display->color565(0, 255, 0));
  delay(1000);
  
  // Linha azul no fundo
  display->clearScreen();
  display->drawLine(0, 15, 31, 15, display->color565(0, 0, 255));
  delay(1000);
}
```

### Exemplo 2: Gradiente
```cpp
void loop() {
  for(int y = 0; y < 16; y++) {
    uint8_t brightness = map(y, 0, 15, 0, 255);
    for(int x = 0; x < 32; x++) {
      display->drawPixel(x, y, display->color565(brightness, 0, 0));
    }
  }
  delay(5000);
}
```

## ⚙️ Detalhes Técnicos

### Problema Resolvido

Painéis P10 outdoor 32x16 com scan 1/4 têm um mapeamento especial:
- As linhas físicas não correspondem diretamente às linhas lógicas
- Há um swap de blocos de 4 linhas
- As linhas 0-7 usam R1/G1/B1
- As linhas 8-15 usam R2/G2/B2
- O eixo X também requer remapeamento

Esta biblioteca corrige automaticamente todo o mapeamento.

## 🐛 Limitações Conhecidas

- Texto só funciona corretamente nas linhas 0-7 (limitação da biblioteca base)
- Para texto nas linhas 8-15, use `drawPixel()` para desenhar caracteres manualmente

## 📝 Licença

MIT License - Livre para uso comercial e pessoal

## 👨‍💻 Autor

Desenvolvido por [Seu Nome] em Dezembro 2024

## 🙏 Créditos

- Biblioteca base: [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) por mrfaptastic
- Inspirado em discussões da comunidade sobre painéis P10 1/4 scan

## 📞 Suporte

- Issues: https://github.com/seuuser/P10_32x16_QuarterScan/issues
- Discussões: https://github.com/seuuser/P10_32x16_QuarterScan/discussions
