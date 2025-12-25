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
- **Chips:** SM16208SJ, DP74HC138B, MW245B, MW4953F

## ✅ Problema Resolvido

Painéis P10 1/4 scan têm um problema de **duplicação de pixels** quando usados com a biblioteca base:
- Cada pixel aparece duplicado com offset de +16 colunas
- Texto e gráficos aparecem "espelhados" ou ilegíveis

**Solução implementada** (baseada na [Issue #680](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/680)):
- Configurar o driver como **64x8** (não 32x16!)
- Aplicar fórmula de mapeamento #680 com `pxbase=8`

## 📦 Instalação

### Arduino IDE

1. Baixe a biblioteca como ZIP
2. No Arduino IDE: `Sketch` → `Include Library` → `Add .ZIP Library`
3. Selecione o arquivo ZIP baixado

### PlatformIO
```ini
lib_deps =
    https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
    https://github.com/filipe3x/P10_32x16_QuarterScan
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

// ⚠️ IMPORTANTE: Configurar como 64x8, NÃO 32x16!
HUB75_I2S_CFG mxconfig(64, 8, 1, _pins);
mxconfig.clkphase = false;
mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

// Criar display base
MatrixPanel_I2S_DMA *dma_display = new MatrixPanel_I2S_DMA(mxconfig);
dma_display->setBrightness8(100);
dma_display->begin();

// Criar wrapper P10 (interface lógica 32x16)
P10_32x16_QuarterScan *display = new P10_32x16_QuarterScan(dma_display);

// Usar o display normalmente!
display->fillScreen(display->color565(255, 0, 0)); // Vermelho
display->drawPixel(10, 5, display->color565(0, 255, 0)); // Pixel verde
display->print("Hello!"); // Texto funciona!
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

### Texto (via Adafruit_GFX)

- `setCursor(x, y)` - Define posição do cursor
- `setTextColor(color)` - Define cor do texto
- `setTextSize(size)` - Define tamanho (1, 2, 3...)
- `print(text)` - Imprime texto

### Configuração

- `setBrightness(0-255)` - Define brilho
- `width()` - Retorna largura (32)
- `height()` - Retorna altura (16)

## ⚙️ Detalhes Técnicos

### Arquitetura da Solução

```
┌─────────────────────────────────────────────────────────────┐
│                    P10_32x16_QuarterScan                    │
│                  (Interface lógica: 32x16)                  │
├─────────────────────────────────────────────────────────────┤
│                    Fórmula #680                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ driverY = ((y >> 3) * 4) + (y & 0b11)                 │  │
│  │                                                        │  │
│  │ if (y & 4) == 0:                                      │  │
│  │     driverX = x + (x / 8) * 8                         │  │
│  │ else:                                                  │  │
│  │     driverX = x + ((x / 8) + 1) * 8                   │  │
│  └───────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│              MatrixPanel_I2S_DMA (64x8)                     │
├─────────────────────────────────────────────────────────────┤
│                   Hardware P10 1/4 Scan                     │
└─────────────────────────────────────────────────────────────┘
```

### Tabela de Mapeamento X

| X lógico | y&4==0 (linhas 0-3, 8-11) | y&4!=0 (linhas 4-7, 12-15) |
|----------|---------------------------|----------------------------|
| 0-7      | 0-7                       | 8-15                       |
| 8-15     | 16-23                     | 24-31                      |
| 16-23    | 32-39                     | 40-47                      |
| 24-31    | 48-55                     | 56-63                      |

### Tabela de Mapeamento Y

| Y lógico | driverY |
|----------|---------|
| 0-3      | 0-3     |
| 4-7      | 0-3     |
| 8-11     | 4-7     |
| 12-15    | 4-7     |

### Por que 64x8?

O painel P10 1/4 scan tem uma arquitetura de shift registers que causa duplicação com offset de +16 colunas. Ao configurar o driver como 64x8:
- Obtemos 64 endereços de coluna únicos
- A fórmula #680 distribui os pixels corretamente
- Eliminamos a duplicação "fantasma"

## 🔧 Exemplos

Ver pasta `examples/` para exemplos completos:
- `SimpleTest/` - Testes básicos de linha, retângulo e texto
- `DiagnosticTest/` - Script de diagnóstico interativo

## 🙏 Créditos

- Biblioteca base: [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) por mrfaptastic
- Solução #680: [Issue #680](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/680)
- Discussão original: [Discussion #622](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/discussions/622)

## 📝 Licença

MIT License - Livre para uso comercial e pessoal

## 👨‍💻 Autor

Desenvolvido por Filipe Marques em Dezembro 2025

## 📞 Suporte

- Issues: https://github.com/filipe3x/P10_32x16_QuarterScan/issues
