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
P10_32x16_QuarterScan : Adafruit_GFX
        │
        └──> MatrixPanel_I2S_DMA (base display, passed to constructor)
```

- **Inheritance pattern:** Extends `Adafruit_GFX` to inherit all text/drawing functions. Only overrides `drawPixel()` - all GFX operations (text, lines, shapes) work automatically by delegating to `drawPixel()`.
- **Wrapper pattern:** Takes initialized `MatrixPanel_I2S_DMA*` in constructor, doesn't own it.

## 1/4 Scan Remapping (Critical Implementation Detail)

The panel's physical pixel layout doesn't match logical coordinates. Two remapping functions handle this:

**`remapY(y)`** - Swaps 4-line blocks within each half:
- Lines 0-3 ↔ Lines 4-7 (for both top and bottom halves)
- Uses modulo arithmetic to swap quad positions

**`remapX(x, mappedY)`** - Adjusts X based on remapped Y:
- For lines 0-3 (after remap): mirrors within 8-pixel blocks
- For lines 4-7: interleaves with offset

**Display split:** Y < 8 uses R1/G1/B1 pins; Y >= 8 uses R2/G2/B2 pins (standard HUB75 behavior)

## File Structure

- `src/P10_32x16_QuarterScan.h` - Header with class definition
- `src/P10_32x16_QuarterScan.cpp` - Implementation with remapping logic
- `examples/SimpleTest/` - Basic usage example
- `library.properties` - Arduino library metadata
