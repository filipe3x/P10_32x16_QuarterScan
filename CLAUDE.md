# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino library for ESP32 that drives P10 outdoor RGB LED panels (32x16 pixels, 1/4 scan, SMD3535). It wraps ESP32-HUB75-MatrixPanel-I2S-DMA with automatic coordinate remapping to correct the non-standard pixel layout of 1/4 scan panels.

## Build & Development

This is an Arduino library - no standalone build system. Development options:

**Arduino IDE:**
- Install via `Sketch → Include Library → Add .ZIP Library`
- Open examples from `File → Examples → P10_32x16_QuarterScan`
- Select ESP32 board and upload

**PlatformIO:**
```ini
lib_deps =
    https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
    https://github.com/filipe3x/P10_32x16_QuarterScan
```

**Dependencies:** ESP32-HUB75-MatrixPanel-I2S-DMA, Adafruit_GFX (pulled automatically)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    P10_32x16_QuarterScan                    │
│                  (Interface lógica: 32x16)                  │
│                  Herda de Adafruit_GFX                      │
├─────────────────────────────────────────────────────────────┤
│                    Fórmula #680                             │
│         Mapeia coordenadas 32x16 → 64x8                     │
├─────────────────────────────────────────────────────────────┤
│              MatrixPanel_I2S_DMA (64x8)                     │
│         ⚠️ DEVE ser configurado como 64x8!                  │
├─────────────────────────────────────────────────────────────┤
│                   Hardware P10 1/4 Scan                     │
└─────────────────────────────────────────────────────────────┘
```

- **Inheritance pattern:** Extends `Adafruit_GFX` to inherit all text/drawing functions. Only overrides `drawPixel()` - all GFX operations (text, lines, shapes) work automatically by delegating to `drawPixel()`.
- **Wrapper pattern:** Takes initialized `MatrixPanel_I2S_DMA*` in constructor, doesn't own it.

## ✅ SOLUÇÃO IMPLEMENTADA (Issue #680)

### O Problema Original

Painéis P10 1/4 scan com configuração 32x16 apresentavam:
- Duplicação de pixels com offset +16 colunas
- Texto e gráficos ilegíveis/espelhados
- Cada `drawPixel()` acendia 2 pixels físicos

### A Causa Raiz

O hardware de shift registers do painel 1/4 scan só tem 16 endereços de coluna únicos por linha de scan. Com configuração 32x16:
- Driver envia dados para 32 colunas
- Hardware replica colunas 0-15 nas colunas 16-31
- Resultado: duplicação "fantasma"

### A Solução (#680)

1. **Configurar driver como 64x8** (não 32x16!)
   ```cpp
   HUB75_I2S_CFG mxconfig(64, 8, 1, _pins);
   ```
   - Isto dá 64 endereços de coluna únicos
   - Elimina a duplicação

2. **Fórmula de mapeamento #680** com `pxbase=8`:
   ```cpp
   // Transformação Y
   driverY = ((y >> 3) * 4) + (y & 0b11);

   // Transformação X
   if ((y & 4) == 0) {
       driverX = x + (x / 8) * 8;      // Linhas 0-3, 8-11
   } else {
       driverX = x + ((x / 8) + 1) * 8; // Linhas 4-7, 12-15
   }
   ```

### Tabelas de Mapeamento

**Mapeamento X (pxbase=8):**

| X lógico | y&4==0 (linhas 0-3, 8-11) | y&4!=0 (linhas 4-7, 12-15) |
|----------|---------------------------|----------------------------|
| 0-7      | 0-7                       | 8-15                       |
| 8-15     | 16-23                     | 24-31                      |
| 16-23    | 32-39                     | 40-47                      |
| 24-31    | 48-55                     | 56-63                      |

**Mapeamento Y:**

| Y lógico | driverY |
|----------|---------|
| 0-3      | 0-3     |
| 4-7      | 0-3     |
| 8-11     | 4-7     |
| 12-15    | 4-7     |

## File Structure

- `src/P10_32x16_QuarterScan.h` - Header com definição da classe
- `src/P10_32x16_QuarterScan.cpp` - Implementação com fórmula #680
- `examples/SimpleTest/` - Exemplo básico (usa config 64x8)
- `examples/DiagnosticTest/` - Script de diagnóstico interativo
- `docs/current_behaviour.md` - Documentação detalhada do problema e solução
- `library.properties` - Arduino library metadata

## Hardware Specifications

- **Panel:** P10-O4S-SMD3535 32×16 outdoor (1/4 scan, constant current)
- **Chips:** SM16208SJ, DP74HC138B, MW245B, MW4953F
- **Microcontroller:** ESP32
- **Interface:** HUB75

## Código de Inicialização Correto

```cpp
#include <P10_32x16_QuarterScan.h>

HUB75_I2S_CFG::i2s_pins _pins = { ... };

// ⚠️ CRÍTICO: 64x8, NÃO 32x16!
HUB75_I2S_CFG mxconfig(64, 8, 1, _pins);
mxconfig.clkphase = false;
mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

MatrixPanel_I2S_DMA *dma_display = new MatrixPanel_I2S_DMA(mxconfig);
dma_display->begin();

// Wrapper expõe interface 32x16
P10_32x16_QuarterScan *display = new P10_32x16_QuarterScan(dma_display);

// Usar normalmente
display->print("Hello!"); // ✅ Funciona!
display->drawPixel(0, 0, RED); // ✅ Sem duplicação!
```

## Referências

- [Issue #680](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/680) - Solução final
- [Issue #677](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/677) - Discussão inicial pxbase
- [Discussion #622](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/discussions/622) - VirtualMatrixPanel approach
- [rpi-rgb-led-matrix #242](https://github.com/hzeller/rpi-rgb-led-matrix/issues/242) - P10outdoorTransformer

## Histórico do Problema

### Tentativas Falhadas (para referência)
- Configuração 32x16 com remapX/remapY → duplicação persistente
- pxbase=1 → ordem errada das linhas (2-1-4-3)
- pxbase=16 → mapeamento incorreto
- Remover multiplicador *2 → linhas pararam de funcionar

### O que Finalmente Funcionou
- Configuração **64x8** (dobro da largura, metade da altura)
- Fórmula #680 com **pxbase=8** e **lógica invertida** vs #677
- Resultado: 32x16 pixels únicos sem duplicação
