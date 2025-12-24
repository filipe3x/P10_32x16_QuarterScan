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

## Hardware Specifications

- **Panel:** P10-O4S-SMD3535 32×16 outdoor (1/4 scan, constant current)
- **Microcontroller:** ESP32
- **Interface:** HUB75

## Critical Problem Identified

### Original Library (ESP32-HUB75-MatrixPanel-I2S-DMA)

**Problem:** Does not work directly with P10 outdoor 1/4 scan panels.

**Why?**

1. **Non-linear coordinate mapping:** P10 1/4 scan panels have special architecture:
   - Physical lines 0-7 use R1/G1/B1
   - Physical lines 8-15 use R2/G2/B2
   - There is swap of 4-line blocks within each half
   - X axis requires special remapping (inversion within 8-pixel blocks)

2. **Inadequate default configuration:** Library assumes panels with different scan rates (1/2, 1/4, 1/8, 1/16) but the driver has no specific support for P10 outdoor.

**Symptoms:**
- Duplicated lines (2-4 simultaneous lines)
- Unreadable display
- Text appears as left/right mirrored garbage

## Solution Implemented

Created wrapper library `P10_32x16_QuarterScan` that:
1. Inherits from `Adafruit_GFX` (CRITICAL for text to work)
2. Implements custom coordinate remapping
3. Overrides `drawPixel()` to intercept ALL drawing operations

Repository: https://github.com/filipe3x/P10_32x16_QuarterScan

This custom wrapper was built based on comments in:
- https://github.com/hzeller/rpi-rgb-led-matrix/issues/242
- https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/discussions/622

## 🔴 CURRENT STATE - PROBLEM NOT RESOLVED

### ✅ What Works PERFECTLY:
- `drawPixel()` individual
- `drawLine()` horizontal/vertical
- `fillRect()`, `drawRect()`
- `fillScreen()`, `clearScreen()`
- All basic graphic operations

### ❌ What Does NOT Work:
- `print()` for text → continues to appear duplicated and mirrored
- Despite correct `Adafruit_GFX` inheritance
- `print()` IS calling `drawPixel()` (confirmed in tests)
- But X remapping causes duplication when there are multiple sequential pixels

## 🔍 Detailed Diagnosis

### Tests Performed:
- `drawPixel(5,5)` → appears 2 pixels (duplicated)
- `print("1")` → appears 2 numbers '1' mirrored (same problem!)
- Horizontal line → appears 1 perfect line ✅
- `print("10:30")` → duplicated left/right garbage

### Conclusion:
- GFX inheritance works ✅ (`print()` calls `drawPixel()`)
- Problem is in the `remapX()` algorithm
- When drawing sequential pixels (like text), remapping spreads/duplicates

### Observed Mapping Data:
```
X logical 0  → physical columns 1 AND 17 (duplicated +16)
X logical 10 (wrapper) → columns 14 AND 30
X logical 10 (base without wrapper) → columns 3 AND 19
Pattern: Everything duplicated with 16-column offset
```

## 🎯 Task for Claude Code

**OBJECTIVE:** Fix the `remapX()` function so that text works without duplication.

**CRITICAL RESTRICTIONS:**
1. ⚠️ **DO NOT TOUCH** `remapY()` - it's perfect!
2. ⚠️ Horizontal/vertical lines **ALREADY WORK** - don't break this!
3. ✅ `Adafruit_GFX` inheritance is correct - keep it!
4. ✅ `drawPixel()` override is correct - keep the structure!

**SPECIFIC PROBLEM:**
- `remapX()` has `* 2` that duplicates coordinates
- Attempt to remove `* 2` made lines stop working
- Need algorithm that works for **BOTH**: individual pixels AND pixel sequences (text)

**DATA TO HELP:**
```
Observations from tests:
- Horizontal line Y=8 with drawLine → 1 perfect line
- Text "10:30" with print() → duplicated and mirrored
- drawPixel(5,5) individual → 2 pixels (duplicated)
- print("1") → 2 numbers '1' (same drawPixel behavior!)

Duplication pattern:
- Everything appears duplicated with 16-column offset
- X=0 → columns 1 and 17
- X=10 → columns 14 and 30 (or 3 and 19 depending on Y)
```

### 🔧 Previous Attempts (All Failed)
- Remove `* 2` → lines stopped working
- Simplify to `return x;` → everything went wrong
- Try `return x % 32;` → still duplicated
- Complex conditional remapping → made it worse

## 📚 References

- Base library: [ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA)
- Discussion about P10: [GitHub issue #622](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/discussions/622) (custom VirtualMatrixPanel classes)
- [rpi-rgb-led-matrix issue #242](https://github.com/hzeller/rpi-rgb-led-matrix/issues/242)
- Datasheet: P10 SMD3535 1/4 scan outdoor module
