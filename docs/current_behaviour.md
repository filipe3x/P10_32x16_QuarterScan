# Current Behaviour - P10 32x16 1/4 Scan Panel Pixel Mapping

## Test Environment
- **Test Mode**: DiagnosticTest.ino - Mode 1 (Pixel by Pixel with WRAPPER)
- **Panel**: P10 32x16 outdoor, 1/4 scan, SMD3535
- **Driver**: ESP32-HUB75-MatrixPanel-I2S-DMA with P10_32x16_QuarterScan wrapper

---

## Observed Behaviour

### Core Problem: 16-Column Offset Duplication

Every pixel drawn produces **TWO lit pixels** on the panel - the intended pixel and a "ghost" duplicate exactly 16 columns apart.

### Test Results (Mode 1 - Pixel by Pixel WRAPPER)

| Logical Coord | Wrapper Output | Physical Observation |
|---------------|----------------|----------------------|
| (0, 0)        | (0, 4)         | Pixels at (1,1) AND (17,1)* |
| (1, 0)        | (1, 4)         | Pixels at (2,1) AND (18,1)* |
| (2, 0)        | (2, 4)         | Pixels at (3,1) AND (19,1)* |
| ...           | ...            | Same pattern continues |
| (15, 0)       | (23, 4)        | Pixels at (16,1) AND (32,1)* |
| (16, 0)       | (0, 4)         | REPEATS from (0,0) pattern! |

*Note: Physical positions counted from 1 (human-readable). In 0-indexed: (0,0)→(0,0)+(16,0)

### Pattern Analysis

```
┌────────────────────────────────────────────────────────────────┐
│                        32x16 Panel                              │
├───────────────────────┬────────────────────────────────────────┤
│   LEFT HALF (0-15)    │         RIGHT HALF (16-31)             │
├───────────────────────┼────────────────────────────────────────┤
│  ● Correct pixel      │    ● Ghost duplicate (+16 offset)      │
│  at logical position  │    appears simultaneously               │
└───────────────────────┴────────────────────────────────────────┘
```

### Duplication Rule

```
For any drawPixel(x, y):
  → Appears at physical column: C
  → ALSO appears at physical column: C + 16 (mod 32)
```

### Key Observations

1. **16-column periodicity**: The duplication has a fixed 16-column offset
2. **Cycle at X=16**: When logical X reaches 16, the pattern restarts from the beginning
3. **Consistent across all Y**: The duplication occurs on every row
4. **Single drawPixel call**: Confirmed that `drawPixel()` is called only ONCE per pixel

---

## RAW Mode Test Results (Mode 2 - Critical Discovery!)

### Test Data Summary (Logical Y=0)

| Driver Coord | Physical Position 1 | Physical Position 2 | Physical Row |
|--------------|---------------------|---------------------|--------------|
| (0-7, 0)     | cols 1-8            | cols 17-24          | row 1        |
| (8-15, 0)    | cols 1-8            | cols 17-24          | **row 5**    |
| (16-23, 0)   | cols 9-16           | cols 25-32          | row 1        |
| (24-31, 0)   | cols 9-16           | cols 25-32          | **row 5**    |
| (0-7, 1)     | ...                 | ...                 | row 2        |

*Physical positions in 1-indexed notation*

### Pattern Discovery: 8-Column Block Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          RAW DRIVER COLUMN MAPPING                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Driver X 0-7        Driver X 8-15       Driver X 16-23      Driver X 24-31│
│   ┌─────────┐         ┌─────────┐         ┌─────────┐         ┌─────────┐  │
│   │ Row 0   │         │ Row 4   │         │ Row 0   │         │ Row 4   │  │
│   │         │         │         │         │         │         │         │  │
│   │ Phys    │         │ Phys    │         │ Phys    │         │ Phys    │  │
│   │ 0-7     │         │ 0-7     │         │ 8-15    │         │ 8-15    │  │
│   │  AND    │         │  AND    │         │  AND    │         │  AND    │  │
│   │ 16-23   │         │ 16-23   │         │ 24-31   │         │ 24-31   │  │
│   └─────────┘         └─────────┘         └─────────┘         └─────────┘  │
│                                                                             │
│   BLOCK 0             BLOCK 1             BLOCK 2             BLOCK 3      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Critical Insights from RAW Mode

1. **Duplication is HARDWARE, not software!**
   - Every driver write produces TWO physical columns (+16 offset)
   - This is inherent to the 1/4 scan shift register design
   - **CANNOT be eliminated** - must be worked around

2. **8-Column Blocks with Row Alternation**
   - Driver X 0-7 → Physical row 0 (for logical Y=0)
   - Driver X 8-15 → Physical row 4 (different row!)
   - Driver X 16-23 → Physical row 0 (same as X 0-7)
   - Driver X 24-31 → Physical row 4 (same as X 8-15)

3. **Column Grouping**
   - Blocks 0 & 1 (X 0-15) share physical columns 0-7 AND 16-23
   - Blocks 2 & 3 (X 16-31) share physical columns 8-15 AND 24-31
   - The row address determines which block is visible

4. **The Wrapper Problem**
   - Current wrapper uses same row for X 0-15 AND X 16-31
   - This causes overlap: X=0 and X=16 both write to row 0
   - Result: duplication we observed

### Solution Derived from RAW Data

To display 32 unique columns on this hardware:

```
Logical X 0-7:   → Driver X = 0-7,   Driver Row = base_row
Logical X 8-15:  → Driver X = 0-7,   Driver Row = base_row + 4 (DIFFERENT!)
Logical X 16-23: → Driver X = 8-15,  Driver Row = base_row
Logical X 24-31: → Driver X = 8-15,  Driver Row = base_row + 4 (DIFFERENT!)
```

**Key**: Use the ROW ADDRESS to select which 8-column segment is active!

### Proposed remapX Formula (Based on RAW Data)

```cpp
void drawPixel(int16_t x, int16_t y, uint16_t color) {
    int block = x / 8;                    // 0, 1, 2, or 3
    int localX = x % 8;                   // 0-7 within block

    // Blocks 0,1 use physical X 0-7; Blocks 2,3 use physical X 8-15
    int physX = (block < 2) ? localX : (localX + 8);

    // Blocks 0,2 use row offset 0; Blocks 1,3 use row offset +4
    int rowOffset = (block % 2 == 0) ? 0 : 4;

    int baseY = remapY(y);
    int physY = baseY + rowOffset;

    if (y >= 8) physY += 8;  // Top/bottom half selection

    baseDisplay->drawPixel(physX, physY, color);
}
```

---

## Root Cause Analysis

### Hardware Architecture (1/4 Scan)

The P10 1/4 scan panel has a specific shift register arrangement:

```
┌─────────────────────────────────────────────────────────────┐
│                    32 Physical Columns                       │
├─────────────────────────────┬───────────────────────────────┤
│     Columns 0-15            │       Columns 16-31           │
│  (Shift Register Bank A)    │  (Shift Register Bank B)      │
└─────────────────────────────┴───────────────────────────────┘
                    ↓                         ↓
              SHARED DATA LINE (clocked simultaneously)
```

**Problem**: The base HUB75 driver treats the panel as 32 independent columns, but the physical hardware has the two 16-column banks wired in parallel for certain scan lines.

### Why This Happens

1. **Base driver column 0-31** maps to **physical column 0-15 in BOTH halves**
2. When driver sends data for column X:
   - Bank A receives data at position X mod 16
   - Bank B receives data at position X mod 16 (SAME data!)
3. The row selection determines WHICH bank is visible, but not column isolation

### Current remapX Logic (INCORRECT)

```cpp
if ((mappedY & 4) == 0) {
    // Tries to mirror within 8-pixel blocks
    result = (x / pxbase) * pxbase + pxbase + 7 - (x & 0x7);
} else {
    // Tries to interleave
    result = x + (x / pxbase) * pxbase;
}
```

**Issue**: This produces results in range 0-31, but the hardware only has 16 unique column addresses per scan line. The `% 32` at the end wraps but doesn't prevent duplication.

---

## Community Solutions (Reference)

Based on discussions from [ESP32-HUB75-MatrixPanel-DMA #622](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/discussions/622) and [rpi-rgb-led-matrix #242](https://github.com/hzeller/rpi-rgb-led-matrix/issues/242).

### Solution A: VirtualMatrixPanel Override (ESP32-HUB75 Library)

The recommended approach from the ESP32 library is to create a derived class from `VirtualMatrixPanel` and override `getCoords()`:

**Y-Coordinate Transformation:**
```cpp
coords.y = (coords.y >> 3) * 4 + (coords.y & 0b00000011)
```

**X-Coordinate Transformation (conditional):**
```cpp
uint8_t pxbase = 8;  // Key: "pixel base" concept

if ((coords.y & 4) == 0) {
    // Mirror within blocks
    coords.x = (coords.x / pxbase) * 2 * pxbase + pxbase + 7 - (coords.x & 0x7);
} else {
    // Interleave with offset
    coords.x += (coords.x / pxbase) * pxbase;
}
```

**Key Insight**: The `pxbase` value (4, 8, or panel width) must be determined experimentally for each panel variant.

### Solution B: P10outdoorTransformer (Raspberry Pi Library)

From the rpi-rgb-led-matrix library, VolkerGoeschl's transformer uses:

**Y-axis Remapping:**
```cpp
new_y = 4 * (y / 8) + (y % 2)
```

**X-axis Remapping with Row-Based Offset:**
```cpp
// x_offset varies by row position within the group
switch(y % 8) {
    case 0:
    case 1: x_offset = 64; break;
    case 2:
    case 3: x_offset = 72; break;
    case 4:
    case 5: x_offset = 0; break;
    case 6:
    case 7: x_offset = 8; break;
}
```

**Configuration Requirement**:
- Set rows = 8 (not 16!)
- Chain length = 4 × actual panels
- This treats the 32x16 as a chain of four 8x8 virtual panels

### Solution C: Panel Configuration Adjustment

Try toggling `mxconfig.clkphase` between `true` and `false` - different driver chips (FM6124D, ICN2037BP) may require different settings.

---

## Analysis: Current Implementation vs Community Solutions

### Comparing remapX Formulas

| Source | Formula (when mappedY & 4 == 0) | Formula (when mappedY & 4 != 0) |
|--------|--------------------------------|--------------------------------|
| **Current Implementation** | `(x/8)*8 + 8 + 7 - (x&0x7)` | `x + (x/8)*8` |
| **Community #622** | `(x/8)*2*8 + 8 + 7 - (x&0x7)` | `x + (x/8)*8` |

**Critical Difference**: The community version has `* 2 *` multiplier that our current version removed!

### The `*2` Mystery

Looking at the formulas:
```cpp
// Community (with *2)
(x / pxbase) * 2 * pxbase + pxbase + 7 - (x & 0x7)

// Current (without *2)
(x / pxbase) * pxbase + pxbase + 7 - (x & 0x7)
```

**Trace for x=0, pxbase=8:**
- Community: `(0/8)*2*8 + 8 + 7 - 0 = 0 + 8 + 7 = 15`
- Current: `(0/8)*8 + 8 + 7 - 0 = 0 + 8 + 7 = 15` ← Same result!

**Trace for x=8, pxbase=8:**
- Community: `(8/8)*2*8 + 8 + 7 - 0 = 16 + 8 + 7 = 31`
- Current: `(8/8)*8 + 8 + 7 - 0 = 8 + 8 + 7 = 23` ← Different!

**Trace for x=16, pxbase=8:**
- Community: `(16/8)*2*8 + 8 + 7 - 0 = 32 + 8 + 7 = 47` → `47 % 32 = 15` (wraps!)
- Current: `(16/8)*8 + 8 + 7 - 0 = 16 + 8 + 7 = 31`

### Insight: The `*2` Creates the Column Doubling

The community formula with `*2` intentionally maps:
- Logical columns 0-7 → Physical columns 8-15
- Logical columns 8-15 → Physical columns 24-31
- Logical columns 16-23 → Physical columns 8-15 (OVERLAPS with 0-7!)
- Logical columns 24-31 → Physical columns 24-31 (OVERLAPS with 8-15!)

**This is the source of duplication!** The formula is designed for a different panel configuration where the hardware expects this overlapping behavior.

### Why Our Panel May Be Different

Our panel exhibits 16-column offset duplication, suggesting:
- The shift registers are wired for 16-column segments (not 8)
- The base driver configuration (32x16, 1 panel) doesn't match the physical architecture

### Recommended Approach: Virtual Chain Configuration

Based on the Raspberry Pi solution, the key insight is:
- **Configure as 4 panels chained** (not 1 panel)
- **Set rows = 8** (half the physical height)
- This tells the driver the "true" architecture: 4 × (8x8) virtual panels

For ESP32-HUB75-MatrixPanel-DMA, this means:
```cpp
HUB75_I2S_CFG mxconfig(
    8,     // width: 8 (not 32!)
    8,     // height: 8 (not 16!)
    4,     // chain: 4 panels
    _pins
);
```

Then apply coordinate transformation to map 32x16 logical → 4×(8×8) physical.

---

## Proposed Fix Strategy

### Strategy 1: Column Bank Selection via Row Encoding

Instead of trying to address 32 columns independently, leverage the hardware's design:

```
For logical column X (0-31):
  - If X < 16: Use base column = X, select row that makes LEFT half visible
  - If X >= 16: Use base column = X-16, select row that makes RIGHT half visible
```

This requires modifying BOTH `remapX` AND `remapY` to work together:

```cpp
// Pseudocode
int16_t remapX_new(int16_t x, int16_t y) {
    // Output is always in range 0-15
    return x % 16;
}

int16_t remapY_new(int16_t x, int16_t y) {
    // Row selection encodes which column bank (left/right) is active
    int baseRow = calculateBaseRow(y);
    int bankSelect = (x >= 16) ? 1 : 0;
    return baseRow + (bankSelect * BANK_OFFSET);
}
```

### Strategy 2: Empirical Mapping Table

1. Run DiagnosticTest Mode 2 (RAW) to map every physical position
2. Build a lookup table: `physical[x][y] = {baseX, baseY}`
3. Implement as direct lookup instead of formula

### Strategy 3: Half-Panel Multiplexing

Treat each 16x16 half as a separate logical panel:

```cpp
void drawPixel(int16_t x, int16_t y, uint16_t color) {
    int half = x / 16;  // 0 = left, 1 = right
    int localX = x % 16;

    // Each half has its own row mapping
    int baseY = getRowForHalf(y, half);
    int baseX = getColumnForHalf(localX, baseY);

    baseDisplay->drawPixel(baseX, baseY, color);
}
```

---

## Required Data for Solution

To implement any strategy, we need more diagnostic data:

### Test 1: RAW Mode Column Sweep
For each base driver column (0-31), record which physical columns light up.

### Test 2: RAW Mode Row Isolation
For each base driver row (0-15), record which physical rows are illuminated.

### Test 3: Combined Grid Test
Draw a 16x8 grid using only base columns 0-15 and observe if any half of the panel can be isolated.

---

## Constraints for Fix (UPDATED based on RAW data)

1. ~~**DO NOT modify `remapY()`**~~ → **MUST modify how Y is calculated!**
   - RAW data proves that row address encodes the 8-column block selection
   - The fix requires adding row offset based on X position
2. **Horizontal lines work** - Don't break sequential X drawing
3. **Must work for single pixels AND sequences** - Text rendering depends on this
4. **Output must be in valid range** - baseDisplay expects 0-15 for X (not 0-31!), 0-15 for Y
5. **NEW: X range is 0-15, not 0-31!** - The driver only has 16 unique column addresses per row

---

## Next Steps

1. Run Mode 2 (RAW pixel by pixel) to understand base driver mapping
2. Document which physical positions each (baseX, baseY) illuminates
3. Identify if there's a Y-row that isolates left half from right half
4. Implement new remapX that uses only 16 unique column values
5. If needed, modify remapY to encode left/right half selection

---

## Appendix: Current Implementation Reference

### remapY (Working - DO NOT MODIFY)
```cpp
int16_t remapY(int16_t y) {
    int lineInHalf = y % 8;
    int quad = (lineInHalf / 4) % 2;
    int posInQuad = lineInHalf % 4;

    if (quad == 0) {
        return posInQuad + 4;
    } else {
        return posInQuad;
    }
}
```

### remapX (NEEDS FIX)
```cpp
int16_t remapX(int16_t x, int16_t mappedY) {
    uint8_t pxbase = 8;
    int16_t result;

    if ((mappedY & 4) == 0) {
        result = (x / pxbase) * pxbase + pxbase + 7 - (x & 0x7);
    } else {
        result = x + (x / pxbase) * pxbase;
    }

    return result % 32;
}
```

### drawPixel Entry Point
```cpp
void drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= 32 || y < 0 || y >= 16) return;

    int16_t mappedY = remapY(y);
    int16_t mappedX = remapX(x, mappedY);

    if (y < 8) {
        baseDisplay->drawPixel(mappedX, mappedY, color);
    } else {
        baseDisplay->drawPixel(mappedX, mappedY + 8, color);
    }
}
```

---

## Prioritized Action Plan

### Priority 1: Test Different pxbase Values (QUICK TEST)

The `pxbase` variable controls the block size for coordinate calculations. Test values:

```cpp
// In remapX, try these values for pxbase:
uint8_t pxbase = 4;   // Smaller blocks
uint8_t pxbase = 8;   // Current value
uint8_t pxbase = 16;  // Match the duplication offset
uint8_t pxbase = 32;  // Full panel width
```

**How to test**: Modify `remapX()` in DiagnosticTest.ino and run Mode 1 again.

### Priority 2: Test Virtual Chain Configuration (MEDIUM EFFORT)

Modify the display initialization in DiagnosticTest.ino:

```cpp
// BEFORE (current)
HUB75_I2S_CFG mxconfig(32, 16, 1, _pins);

// AFTER (try this)
HUB75_I2S_CFG mxconfig(32, 8, 1, _pins);  // Half height
// OR
HUB75_I2S_CFG mxconfig(16, 16, 1, _pins); // Half width
// OR
HUB75_I2S_CFG mxconfig(16, 8, 2, _pins);  // Two 16x8 panels chained
```

### Priority 3: Run RAW Mode Tests (DATA COLLECTION)

Execute DiagnosticTest Mode 2 (RAW pixel by pixel) and document:
1. For each base coordinate (x,y), where does the pixel physically appear?
2. Is there duplication in RAW mode too, or only with the wrapper?
3. Build a complete mapping table: `base(x,y) → physical(px, py)`

### Priority 4: Implement 16-Column Segment Mapping (IF NEEDED)

If the panel truly has 16-column segments, implement:

```cpp
int16_t remapX_v2(int16_t x, int16_t y, int16_t mappedY) {
    uint8_t pxbase = 16;  // Match the 16-column segment
    int half = x / 16;    // 0 = left, 1 = right
    int localX = x % 16;  // Position within segment

    // Apply transformation only within the 16-column segment
    // ... formula to be determined from RAW test data

    return result;
}
```

### Priority 5: Consider VirtualMatrixPanel Approach (MAJOR REFACTOR)

If simpler fixes don't work, refactor to use `VirtualMatrixPanel` from the base library:

```cpp
class P10_32x16_Virtual : public VirtualMatrixPanel {
public:
    VirtualCoords getCoords(int16_t x, int16_t y) override {
        VirtualCoords coords;
        // Custom transformation logic here
        return coords;
    }
};
```

This integrates better with the base library's architecture.

---

## Quick Reference: Diagnostic Commands

```
Mode 1:  Pixel by pixel (WRAPPER) - Observe duplication pattern
Mode 2:  Pixel by pixel (RAW) - Baseline without remapping
Mode 3:  Horizontal lines (WRAPPER) - Test line continuity
Mode 5:  Vertical columns (WRAPPER) - Test column consistency
Mode 10: Quadrants (WRAPPER) - Test half-panel isolation
Mode 14: Grid 8x8 - Test reference points
```

---

## Document Version

- **Created**: Based on DiagnosticTest Mode 1 observations
- **Updated**: Added RAW mode (Mode 2) test results - CRITICAL DISCOVERY
- **Panel**: P10 32x16 outdoor, 1/4 scan, SMD3535
- **Firmware**: ESP32 with ESP32-HUB75-MatrixPanel-I2S-DMA
- **Status**: ✅ ROOT CAUSE IDENTIFIED - Ready for implementation

### Key Discovery Summary

The duplication is **HARDWARE BEHAVIOR** inherent to 1/4 scan panels. The fix requires:
1. Use only 16 unique X addresses (0-15) instead of 32
2. Encode the 8-column block selection in the Y row address
3. Each logical Y has 4 sub-rows for the 4 blocks of 8 columns

---

## Test Results: R1/R2 and Ghost Behavior (Modes 20-21)

### Mode 20: R1/R2 Test Results

| Step | Driver Y | Visual Result | Conclusion |
|------|----------|---------------|------------|
| 1 | 0-3 | Top half RED | R1 = Top |
| 2 | 4-7 | Top half GREEN | Still R1 (modulo 4) |
| 3 | 8-11 | Bottom half BLUE | R2 = Bottom |
| 4 | 12-15 | Bottom half YELLOW | Still R2 |

**Confirmed: R1/R2 control TOP/BOTTOM, NOT left/right.**

The +16 column ghost is caused by the shift register architecture, not R1/R2 addressing.

### Why Modes 17 and 19 Work (But Mode 16 Shows Duplication)

The current formula v3 maps both X=0 and X=16 to the SAME driver coordinates:

```
For X=0, Y=0:
  rightHalf=false, localY<4
  → driverX=0, driverY=0
  → Physical: (0,0) + ghost (16,0)

For X=16, Y=0:
  rightHalf=true, localY<4 → SWAP to blocks 0,2
  → driverX=0, driverY=0  ← SAME AS X=0!
  → Physical: (0,0) + ghost (16,0)
```

**Result:**
- When filling (Mode 19): Both X=0 and X=16 write same color → looks correct
- When drawing single pixel (Mode 16): X=0 writes → ghost appears at X=16

### Fundamental Hardware Limitation

The panel's shift register design means:
- **Columns 0-7 and 16-23 are ALWAYS identical** (same data line)
- **Columns 8-15 and 24-31 are ALWAYS identical** (same data line)

This is NOT a software bug - it's how 1/4 scan panels work. Each "unique" pixel write affects TWO physical columns (+16 offset).

### Implications for Display Usage

1. **For solid fills**: Works perfectly (Mode 19)
2. **For horizontal lines**: Works perfectly (Mode 17)
3. **For text/graphics**: The "ghost" at +16 columns is visible
4. **For truly independent 32 columns**: May require treating as 16x32 (rotated) or different hardware

### Possible Solutions to Explore

1. **Accept 16 unique columns**: Use only X 0-15, let ghosts fill 16-31
2. **Different row mapping**: Put X 0-15 and X 16-31 on different physical rows
3. **Hardware configuration**: Check if panel has jumpers/settings for column splitting
4. **Alternative library config**: Try `mxconfig` with different panel chain settings

---

## 🎯 Vetores de Ataque - Estratégias para Resolver Duplicação

Baseado em análise cruzada entre os dados recolhidos do painel e as discussões da comunidade:
- [Issue #677](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/677)
- [Discussion #622](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/discussions/622)

### Vetor 1: Configuração Alternativa do mxconfig (ALTA PRIORIDADE)

A discussão #677 sugere que painéis 1/4 scan devem ser configurados com dimensões "virtuais" diferentes:

```cpp
// CONFIGURAÇÃO ATUAL (problemática)
HUB75_I2S_CFG mxconfig(32, 16, 1, _pins);

// CONFIGURAÇÃO ALTERNATIVA #1 - Dobrar largura, dividir altura
HUB75_I2S_CFG mxconfig(
    PANEL_RES_X * 2,    // 64 (não 32!)
    PANEL_RES_Y / 2,    // 8 (não 16!)
    NUM_PANELS,
    _pins
);

// CONFIGURAÇÃO ALTERNATIVA #2 - Chain de painéis virtuais
HUB75_I2S_CFG mxconfig(
    16,    // largura de cada "sub-painel"
    8,     // altura de cada "sub-painel"
    4,     // 4 painéis em chain (simula 32x16)
    _pins
);

// CONFIGURAÇÃO ALTERNATIVA #3 - Apenas metade da altura
HUB75_I2S_CFG mxconfig(32, 8, 1, _pins);
```

**Justificação**: O driver assume que o painel tem uma certa arquitetura de shift registers. Alterar as dimensões "mente" ao driver sobre o layout físico, potencialmente alinhando com a realidade do hardware.

**Teste rápido**:
```cpp
// Em DiagnosticTest.ino, linha 867, alterar para:
HUB75_I2S_CFG mxconfig(64, 8, 1, _pins);
```

### Vetor 2: Algoritmo pxbase=16 da Issue #677

A issue #677 apresenta um algoritmo diferente que usa `pxbase = 16`:

```cpp
uint8_t pxbase = 16;  // Nota: não 8!

if ((coords.y & 4) == 0) {
    coords.x += ((coords.x / pxbase) + 1) * pxbase;
} else {
    coords.x += (coords.x / pxbase) * pxbase;
}
coords.y = (coords.y >> 3) * 4 + (coords.y & 0b00000011);
```

**Diferenças chave vs implementação atual**:
| Aspeto | Atual (pxbase=8) | Issue #677 (pxbase=16) |
|--------|------------------|------------------------|
| Tamanho do bloco | 8 pixels | 16 pixels |
| Fórmula Y | `(lineInHalf/4)%2` swap | `(y>>3)*4 + (y&3)` |
| Fórmula X (y&4==0) | `(x/8)*8 + 8 + 7 - (x&7)` | `x + ((x/16)+1)*16` |

**Teste**:
```cpp
int16_t remapX_v677(int16_t x, int16_t mappedY) {
    uint8_t pxbase = 16;
    int16_t result;

    if ((mappedY & 4) == 0) {
        result = x + ((x / pxbase) + 1) * pxbase;
    } else {
        result = x + (x / pxbase) * pxbase;
    }

    return result;  // Sem % 32 - pode ir além de 31!
}
```

### Vetor 3: Classe VirtualMatrixPanel Customizada

Em vez de wrapper com Adafruit_GFX, usar a classe `VirtualMatrixPanel` da biblioteca base:

```cpp
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

class P10_QuarterScan_Virtual : public VirtualMatrixPanel {
public:
    P10_QuarterScan_Virtual(
        MatrixPanel_I2S_DMA &display,
        int panelsX, int panelsY,
        int virtualWidth, int virtualHeight
    ) : VirtualMatrixPanel(display, panelsX, panelsY, virtualWidth, virtualHeight) {}

protected:
    VirtualCoords getCoords(int16_t x, int16_t y) override {
        VirtualCoords coords;
        coords.x = x;
        coords.y = y;

        uint8_t pxbase = 8;  // ou 16

        if ((coords.y & 4) == 0) {
            coords.x = (coords.x / pxbase) * 2 * pxbase + pxbase + 7 - (coords.x & 0x7);
        } else {
            coords.x += (coords.x / pxbase) * pxbase;
        }

        coords.y = (coords.y >> 3) * 4 + (coords.y & 0b00000011);

        return coords;
    }
};
```

**Vantagens**:
- Integra nativamente com a biblioteca base
- Suporta chains de painéis
- Não precisa de wrapper separado

**Uso**:
```cpp
MatrixPanel_I2S_DMA *dma_display = new MatrixPanel_I2S_DMA(mxconfig);
P10_QuarterScan_Virtual *virtualDisplay = new P10_QuarterScan_Virtual(
    *dma_display, 1, 1, 32, 16
);
// Usar virtualDisplay->drawPixel() etc.
```

### Vetor 4: Alteração do Pinout / Clock Phase

Diferentes chips drivers (SM16208SJ, FM6124D, ICN2037BP) podem requerer configurações diferentes:

```cpp
// Testar diferentes valores de clkphase
mxconfig.clkphase = false;  // Atual
mxconfig.clkphase = true;   // Alternativa

// Testar diferentes drivers
mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;    // Atual
mxconfig.driver = HUB75_I2S_CFG::FM6124;      // Alternativa 1
mxconfig.driver = HUB75_I2S_CFG::ICN2038S;    // Alternativa 2
mxconfig.driver = HUB75_I2S_CFG::MBI5124;     // Alternativa 3

// Testar latch blanking
mxconfig.latch_blanking = 1;  // Default
mxconfig.latch_blanking = 2;  // Alternativa
mxconfig.latch_blanking = 4;  // Mais agressivo
```

**Chips do nosso painel**: SM16208SJ, DP74HC138B, MW245B, MW4953F

### Vetor 5: Scan Rate Explícito via setPhysicalPanelScanRate

A biblioteca tem uma função para definir explicitamente o scan rate:

```cpp
// Tentar forçar 1/4 scan explicitamente
dma_display->setPhysicalPanelScanRate(FOUR_SCAN_16PX_HIGH);

// Alternativas
dma_display->setPhysicalPanelScanRate(FOUR_SCAN_32PX_HIGH);
dma_display->setPhysicalPanelScanRate(FOUR_SCAN_8PX_HIGH);
```

**Nota**: Esta função pode não existir em todas as versões da biblioteca.

### Vetor 6: Tratamento como Painel 16x32 (Rotação)

Se o hardware realmente só suporta 16 colunas únicas, podemos tratar como 16x32 rotado:

```cpp
// Configurar como 16 colunas, 32 linhas
HUB75_I2S_CFG mxconfig(16, 32, 1, _pins);

// No drawPixel, fazer rotação:
void drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Lógico: 32x16
    // Físico: 16x32 rotado 90°
    int16_t physX = y;       // Y lógico → X físico
    int16_t physY = 31 - x;  // X lógico → Y físico (invertido)
    baseDisplay->drawPixel(physX, physY, color);
}
```

### Vetor 7: Double-Buffering com Mascaramento

Aceitar a duplicação mas usar double-buffering para "limpar" a metade fantasma:

```cpp
void drawPixelMasked(int16_t x, int16_t y, uint16_t color) {
    // Desenhar pixel normal
    drawPixel(x, y, color);

    // Apagar o ghost (+16)
    int16_t ghostX = (x + 16) % 32;
    // Só apagar se for diferente do pixel atual
    if (ghostX != x) {
        drawPixel(ghostX, y, 0);  // Preto
    }
}
```

**Problema**: Pode não funcionar se a duplicação é simultânea (mesmo clock cycle).

### Vetor 8: Mapeamento Empírico Completo

Criar tabela de lookup baseada em testes RAW completos:

```cpp
// Estrutura para mapear coordenadas
struct PhysicalCoord {
    int16_t x;
    int16_t y;
};

// Tabela de 32x16 = 512 entradas
const PhysicalCoord PIXEL_MAP[32][16] PROGMEM = {
    // Preenchida com dados do DiagnosticTest Mode 2
    {{0, 4}, {0, 5}, ...},  // x=0, y=0..15
    {{1, 4}, {1, 5}, ...},  // x=1, y=0..15
    // ... etc
};

void drawPixel(int16_t x, int16_t y, uint16_t color) {
    PhysicalCoord p = pgm_read_word(&PIXEL_MAP[x][y]);
    baseDisplay->drawPixel(p.x, p.y, color);
}
```

### Vetor 9: Combinação mxconfig + Fórmula #677

A solução mais provável requer AMBAS as alterações:

```cpp
// Passo 1: Configurar display com dimensões virtuais
HUB75_I2S_CFG mxconfig(
    PANEL_RES_X * 2,  // 64
    PANEL_RES_Y / 2,  // 8
    1,
    _pins
);
mxconfig.clkphase = false;

// Passo 2: Usar fórmula da issue #677
uint8_t pxbase = 16;
if ((coords.y & 4) == 0) {
    coords.x += ((coords.x / pxbase) + 1) * pxbase;
} else {
    coords.x += (coords.x / pxbase) * pxbase;
}
coords.y = (coords.y >> 3) * 4 + (coords.y & 0b00000011);
```

---

## 📋 Plano de Testes Sistemático

### Fase 1: Testes de Configuração (Sem alterar código de remapping)

| Teste | mxconfig | Esperado | Resultado |
|-------|----------|----------|-----------|
| A1 | 32x16x1 | Baseline (atual) | Duplicado |
| A2 | 64x8x1 | Possível correção | ? |
| A3 | 32x8x1 | Metade do painel | ? |
| A4 | 16x16x2 | Chain de 2 | ? |
| A5 | 16x8x4 | Chain de 4 | ? |

### Fase 2: Testes de Algoritmo (Com mxconfig 64x8x1)

| Teste | pxbase | Fórmula | Resultado |
|-------|--------|---------|-----------|
| B1 | 8 | Atual | ? |
| B2 | 16 | Atual | ? |
| B3 | 8 | #677 | ? |
| B4 | 16 | #677 | ? |

### Fase 3: Testes de Driver/Clock

| Teste | Driver | clkphase | Resultado |
|-------|--------|----------|-----------|
| C1 | SHIFTREG | false | Atual |
| C2 | SHIFTREG | true | ? |
| C3 | FM6124 | false | ? |
| C4 | FM6124 | true | ? |

---

## 🔗 Referências das Discussões

### Issue #677 - Pontos Chave
- Painéis com chips SM16208SJ, DP74HC138B, MW245B, MW4953F
- Configuração `mxconfig(PANEL_RES_X * 2, PANEL_RES_Y / 2, NUM_PANELS)`
- Algoritmo com `pxbase = 16`
- Delay de escrita para debug visual
- Cabos longos (>1m) podem causar problemas de sinal

### Discussion #622 - Pontos Chave
- Conceito de "pixel base" (pxbase)
- Classe `EightPxBasePanel` derivada de VirtualMatrixPanel
- Fórmula diferente: `(x/pxbase)*2*pxbase + pxbase + 7 - (x&0x7)`
- Variantes para 1/5 e 1/8 scan
- Não usar `setPhysicalPanelScanRate()` com classes custom

---

## ⚡ Próximas Ações Prioritárias

1. **IMEDIATO**: Testar `mxconfig(64, 8, 1, _pins)` sem alterar remapping
2. **SE FUNCIONAR**: Adaptar fórmulas para novo espaço de coordenadas
3. **SE NÃO**: Testar classe VirtualMatrixPanel com override de getCoords()
4. **BACKUP**: Mapeamento empírico completo via Mode 2 do DiagnosticTest
