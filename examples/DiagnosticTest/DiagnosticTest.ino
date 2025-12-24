/*
 * DiagnosticTest.ino - Teste de Diagnóstico Cirúrgico para P10 32x16
 *
 * Script interativo para descobrir o mapeamento correto do painel.
 * Avança cada teste com 'n' na consola serial.
 *
 * MODOS DE TESTE:
 * 1 - Pixel a pixel (32x16 = 512 testes)
 * 2 - Linha a linha horizontal (16 linhas)
 * 3 - Coluna a coluna vertical (32 colunas)
 * 4 - Caracteres assimétricos ('1', 'F', 'G', '7')
 * 5 - Modo RAW (sem wrapper, direto no baseDisplay)
 * 6 - Teste comparativo (wrapper vs raw, lado a lado)
 * 7 - Quadrantes (divide painel em 4 partes)
 * 8 - Scan de Y fixo (todas as colunas de uma linha)
 * 9 - Scan de X fixo (todas as linhas de uma coluna)
 *
 * COMANDOS SERIAL:
 * 'n' - próximo teste
 * 'p' - teste anterior
 * 's' - pular para modo específico
 * 'r' - repetir teste atual
 * 'q' - voltar ao menu
 */

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>

// Pinout para ESP32
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

MatrixPanel_I2S_DMA *dma_display = nullptr;

// ============ FUNÇÕES DE REMAPEAMENTO ATUAIS ============
// (copiadas da biblioteca para teste e modificação)

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

// Desenha pixel COM wrapper/remap
void drawPixelWrapped(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= 32 || y < 0 || y >= 16) return;

  int16_t mappedY = remapY(y);
  int16_t mappedX = remapX(x, mappedY);

  if (y < 8) {
    dma_display->drawPixel(mappedX, mappedY, color);
  } else {
    dma_display->drawPixel(mappedX, mappedY + 8, color);
  }
}

// Desenha pixel SEM wrapper (raw/direto)
void drawPixelRaw(int16_t x, int16_t y, uint16_t color) {
  dma_display->drawPixel(x, y, color);
}

// ============ CORES ============
uint16_t RED, GREEN, BLUE, WHITE, YELLOW, CYAN, MAGENTA;

// ============ VARIÁVEIS GLOBAIS ============
int currentMode = 0;
int currentStep = 0;
bool waitingForInput = true;

// ============ FUNÇÕES AUXILIARES ============

void clearScreen() {
  dma_display->clearScreen();
}

void waitForNext() {
  Serial.println("\n>>> Pressione 'n' para proximo, 'p' para anterior, 'q' para menu <<<");
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'n' || c == 'N') {
        currentStep++;
        return;
      } else if (c == 'p' || c == 'P') {
        if (currentStep > 0) currentStep--;
        return;
      } else if (c == 'q' || c == 'Q') {
        currentMode = 0;
        currentStep = 0;
        return;
      } else if (c == 'r' || c == 'R') {
        return; // repetir
      }
    }
    delay(50);
  }
}

void printCoords(const char* prefix, int16_t x, int16_t y) {
  Serial.print(prefix);
  Serial.print("(");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(")");
}

void printMapping(int16_t logX, int16_t logY) {
  int16_t mappedY = remapY(logY);
  int16_t mappedX = remapX(logX, mappedY);
  int16_t finalY = (logY < 8) ? mappedY : mappedY + 8;

  Serial.print("  Logico: (");
  Serial.print(logX);
  Serial.print(", ");
  Serial.print(logY);
  Serial.print(") -> Fisico: (");
  Serial.print(mappedX);
  Serial.print(", ");
  Serial.print(finalY);
  Serial.println(")");
}

// ============ MENU PRINCIPAL ============

void showMenu() {
  Serial.println("\n");
  Serial.println("╔═══════════════════════════════════════════════════════╗");
  Serial.println("║   P10 32x16 - DIAGNOSTICO CIRURGICO DO PAINEL         ║");
  Serial.println("╠═══════════════════════════════════════════════════════╣");
  Serial.println("║  1 - Pixel a pixel (WRAPPER) - 32x16 pixels           ║");
  Serial.println("║  2 - Pixel a pixel (RAW) - sem remapeamento           ║");
  Serial.println("║  3 - Linhas horizontais (WRAPPER)                     ║");
  Serial.println("║  4 - Linhas horizontais (RAW)                         ║");
  Serial.println("║  5 - Colunas verticais (WRAPPER)                      ║");
  Serial.println("║  6 - Colunas verticais (RAW)                          ║");
  Serial.println("║  7 - Caracteres asimetricos (WRAPPER)                 ║");
  Serial.println("║  8 - Caracteres asimetricos (RAW)                     ║");
  Serial.println("║  9 - Comparacao lado-a-lado (pixel)                   ║");
  Serial.println("║ 10 - Quadrantes coloridos (WRAPPER)                   ║");
  Serial.println("║ 11 - Quadrantes coloridos (RAW)                       ║");
  Serial.println("║ 12 - Teste de Y fixo (todas X de uma linha)           ║");
  Serial.println("║ 13 - Teste de X fixo (todas Y de uma coluna)          ║");
  Serial.println("║ 14 - Grid 8x8 marcadores                              ║");
  Serial.println("║ 15 - Preencher tela inteira (teste basico)            ║");
  Serial.println("╚═══════════════════════════════════════════════════════╝");
  Serial.println("\nDigite o numero do teste:");
}

// ============ TESTES ============

// Teste 1: Pixel a pixel com wrapper
void testPixelByPixelWrapped() {
  int totalPixels = 32 * 16;
  if (currentStep >= totalPixels) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  int x = currentStep % 32;
  int y = currentStep / 32;

  clearScreen();
  drawPixelWrapped(x, y, WHITE);

  Serial.print("\n[WRAPPER] Pixel ");
  Serial.print(currentStep + 1);
  Serial.print("/");
  Serial.print(totalPixels);
  printMapping(x, y);
  Serial.println("  -> OBSERVE: Onde aparece o pixel branco no painel?");

  waitForNext();
}

// Teste 2: Pixel a pixel RAW (sem wrapper)
void testPixelByPixelRaw() {
  int totalPixels = 32 * 16;
  if (currentStep >= totalPixels) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  int x = currentStep % 32;
  int y = currentStep / 32;

  clearScreen();
  drawPixelRaw(x, y, YELLOW);

  Serial.print("\n[RAW] Pixel ");
  Serial.print(currentStep + 1);
  Serial.print("/");
  Serial.print(totalPixels);
  Serial.print("  Coordenada: (");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");
  Serial.println("  -> OBSERVE: Onde aparece o pixel amarelo no painel?");

  waitForNext();
}

// Teste 3: Linhas horizontais com wrapper
void testHorizontalLinesWrapped() {
  if (currentStep >= 16) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();
  for (int x = 0; x < 32; x++) {
    drawPixelWrapped(x, currentStep, GREEN);
  }

  Serial.print("\n[WRAPPER] Linha horizontal Y=");
  Serial.println(currentStep);
  Serial.print("  Mapeamento Y: ");
  Serial.print(currentStep);
  Serial.print(" -> ");
  Serial.println(remapY(currentStep));
  Serial.println("  -> OBSERVE: Onde aparece a linha verde?");

  waitForNext();
}

// Teste 4: Linhas horizontais RAW
void testHorizontalLinesRaw() {
  if (currentStep >= 16) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();
  for (int x = 0; x < 32; x++) {
    drawPixelRaw(x, currentStep, CYAN);
  }

  Serial.print("\n[RAW] Linha horizontal Y=");
  Serial.println(currentStep);
  Serial.println("  -> OBSERVE: Onde aparece a linha ciano?");

  waitForNext();
}

// Teste 5: Colunas verticais com wrapper
void testVerticalColumnsWrapped() {
  if (currentStep >= 32) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();
  for (int y = 0; y < 16; y++) {
    drawPixelWrapped(currentStep, y, RED);
  }

  Serial.print("\n[WRAPPER] Coluna vertical X=");
  Serial.println(currentStep);
  Serial.println("  -> OBSERVE: Onde aparece a coluna vermelha?");

  // Mostrar mapeamento detalhado
  Serial.println("  Mapeamento X para cada Y:");
  for (int y = 0; y < 16; y++) {
    int16_t my = remapY(y);
    int16_t mx = remapX(currentStep, my);
    Serial.print("    Y=");
    Serial.print(y);
    Serial.print(" -> (");
    Serial.print(mx);
    Serial.print(", ");
    Serial.print(y < 8 ? my : my + 8);
    Serial.println(")");
  }

  waitForNext();
}

// Teste 6: Colunas verticais RAW
void testVerticalColumnsRaw() {
  if (currentStep >= 32) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();
  for (int y = 0; y < 16; y++) {
    drawPixelRaw(currentStep, y, MAGENTA);
  }

  Serial.print("\n[RAW] Coluna vertical X=");
  Serial.println(currentStep);
  Serial.println("  -> OBSERVE: Onde aparece a coluna magenta?");

  waitForNext();
}

// Teste 7: Caracteres assimétricos com wrapper
void testCharactersWrapped() {
  const char* chars[] = {"1", "F", "G", "7", "R", "P", "b", "d"};
  int numChars = 8;

  if (currentStep >= numChars) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  // Criar canvas GFX temporário que desenha via wrapper
  // Como não temos a classe wrapper aqui, vamos desenhar manualmente os caracteres

  // Desenhar caractere usando font básica
  // Para isso vamos usar o dma_display mas simular texto

  // Alternativa: desenhar padrões simples que representam os números
  int cx = 8;  // centro X
  int cy = 4;  // centro Y

  Serial.print("\n[WRAPPER] Caractere: '");
  Serial.print(chars[currentStep]);
  Serial.println("'");

  switch(currentStep) {
    case 0: // '1'
      // Desenhar '1' manualmente - linha vertical com pequeno topo
      for (int y = 2; y < 14; y++) drawPixelWrapped(16, y, WHITE);
      drawPixelWrapped(15, 3, WHITE);
      drawPixelWrapped(14, 13, WHITE);
      drawPixelWrapped(15, 13, WHITE);
      drawPixelWrapped(17, 13, WHITE);
      drawPixelWrapped(18, 13, WHITE);
      break;

    case 1: // 'F'
      // Linha vertical esquerda
      for (int y = 2; y < 14; y++) drawPixelWrapped(10, y, WHITE);
      // Linha horizontal topo
      for (int x = 10; x < 20; x++) drawPixelWrapped(x, 2, WHITE);
      // Linha horizontal meio
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 7, WHITE);
      break;

    case 2: // 'G'
      // Arco superior
      for (int x = 12; x < 20; x++) drawPixelWrapped(x, 2, WHITE);
      // Lado esquerdo
      for (int y = 2; y < 14; y++) drawPixelWrapped(10, y, WHITE);
      // Base
      for (int x = 12; x < 20; x++) drawPixelWrapped(x, 13, WHITE);
      // Lado direito (parcial)
      for (int y = 7; y < 14; y++) drawPixelWrapped(20, y, WHITE);
      // Barra horizontal no meio direito
      for (int x = 15; x < 21; x++) drawPixelWrapped(x, 7, WHITE);
      break;

    case 3: // '7'
      // Linha horizontal topo
      for (int x = 10; x < 22; x++) drawPixelWrapped(x, 2, WHITE);
      // Diagonal
      for (int i = 0; i < 11; i++) {
        drawPixelWrapped(21 - i/2, 3 + i, WHITE);
      }
      break;

    case 4: // 'R'
      // Linha vertical esquerda
      for (int y = 2; y < 14; y++) drawPixelWrapped(10, y, WHITE);
      // Arco superior
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 2, WHITE);
      drawPixelWrapped(18, 3, WHITE);
      drawPixelWrapped(18, 4, WHITE);
      drawPixelWrapped(18, 5, WHITE);
      // Linha horizontal meio
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 6, WHITE);
      // Perna diagonal
      for (int i = 0; i < 7; i++) {
        drawPixelWrapped(12 + i, 7 + i, WHITE);
      }
      break;

    case 5: // 'P'
      // Linha vertical esquerda
      for (int y = 2; y < 14; y++) drawPixelWrapped(10, y, WHITE);
      // Arco superior
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 2, WHITE);
      drawPixelWrapped(18, 3, WHITE);
      drawPixelWrapped(18, 4, WHITE);
      drawPixelWrapped(18, 5, WHITE);
      // Linha horizontal meio
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 6, WHITE);
      break;

    case 6: // 'b'
      // Linha vertical esquerda (toda altura)
      for (int y = 2; y < 14; y++) drawPixelWrapped(10, y, WHITE);
      // Barriga
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 7, WHITE);
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 13, WHITE);
      for (int y = 7; y < 14; y++) drawPixelWrapped(18, y, WHITE);
      break;

    case 7: // 'd'
      // Linha vertical direita (toda altura)
      for (int y = 2; y < 14; y++) drawPixelWrapped(18, y, WHITE);
      // Barriga
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 7, WHITE);
      for (int x = 10; x < 18; x++) drawPixelWrapped(x, 13, WHITE);
      for (int y = 7; y < 14; y++) drawPixelWrapped(10, y, WHITE);
      break;
  }

  Serial.println("  -> OBSERVE: O caractere esta correto? Orientacao? Espelhado?");

  waitForNext();
}

// Teste 8: Caracteres assimétricos RAW
void testCharactersRaw() {
  const char* chars[] = {"1", "F", "G", "7", "R", "P", "b", "d"};
  int numChars = 8;

  if (currentStep >= numChars) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  Serial.print("\n[RAW] Caractere: '");
  Serial.print(chars[currentStep]);
  Serial.println("'");

  switch(currentStep) {
    case 0: // '1'
      for (int y = 2; y < 14; y++) drawPixelRaw(16, y, YELLOW);
      drawPixelRaw(15, 3, YELLOW);
      drawPixelRaw(14, 13, YELLOW);
      drawPixelRaw(15, 13, YELLOW);
      drawPixelRaw(17, 13, YELLOW);
      drawPixelRaw(18, 13, YELLOW);
      break;

    case 1: // 'F'
      for (int y = 2; y < 14; y++) drawPixelRaw(10, y, YELLOW);
      for (int x = 10; x < 20; x++) drawPixelRaw(x, 2, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 7, YELLOW);
      break;

    case 2: // 'G'
      for (int x = 12; x < 20; x++) drawPixelRaw(x, 2, YELLOW);
      for (int y = 2; y < 14; y++) drawPixelRaw(10, y, YELLOW);
      for (int x = 12; x < 20; x++) drawPixelRaw(x, 13, YELLOW);
      for (int y = 7; y < 14; y++) drawPixelRaw(20, y, YELLOW);
      for (int x = 15; x < 21; x++) drawPixelRaw(x, 7, YELLOW);
      break;

    case 3: // '7'
      for (int x = 10; x < 22; x++) drawPixelRaw(x, 2, YELLOW);
      for (int i = 0; i < 11; i++) {
        drawPixelRaw(21 - i/2, 3 + i, YELLOW);
      }
      break;

    case 4: // 'R'
      for (int y = 2; y < 14; y++) drawPixelRaw(10, y, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 2, YELLOW);
      drawPixelRaw(18, 3, YELLOW);
      drawPixelRaw(18, 4, YELLOW);
      drawPixelRaw(18, 5, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 6, YELLOW);
      for (int i = 0; i < 7; i++) {
        drawPixelRaw(12 + i, 7 + i, YELLOW);
      }
      break;

    case 5: // 'P'
      for (int y = 2; y < 14; y++) drawPixelRaw(10, y, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 2, YELLOW);
      drawPixelRaw(18, 3, YELLOW);
      drawPixelRaw(18, 4, YELLOW);
      drawPixelRaw(18, 5, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 6, YELLOW);
      break;

    case 6: // 'b'
      for (int y = 2; y < 14; y++) drawPixelRaw(10, y, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 7, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 13, YELLOW);
      for (int y = 7; y < 14; y++) drawPixelRaw(18, y, YELLOW);
      break;

    case 7: // 'd'
      for (int y = 2; y < 14; y++) drawPixelRaw(18, y, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 7, YELLOW);
      for (int x = 10; x < 18; x++) drawPixelRaw(x, 13, YELLOW);
      for (int y = 7; y < 14; y++) drawPixelRaw(10, y, YELLOW);
      break;
  }

  Serial.println("  -> OBSERVE: O caractere esta correto? Orientacao? Espelhado?");

  waitForNext();
}

// Teste 9: Comparação lado a lado
void testComparisonSideBySide() {
  if (currentStep >= 256) { // 16 linhas * 16 colunas (metade do painel cada)
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  int x = currentStep % 16;
  int y = currentStep / 16;

  clearScreen();

  // Lado esquerdo: WRAPPER (branco)
  drawPixelWrapped(x, y, WHITE);

  // Lado direito: RAW (amarelo) - offset de 16 em X
  drawPixelRaw(x + 16, y, YELLOW);

  Serial.print("\n[COMPARACAO] Pixel (");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");
  Serial.println("  Esquerda (branco): WRAPPER");
  Serial.println("  Direita (amarelo): RAW");
  printMapping(x, y);
  Serial.println("  -> COMPARE: As posicoes batem?");

  waitForNext();
}

// Teste 10: Quadrantes coloridos WRAPPER
void testQuadrantsWrapped() {
  if (currentStep >= 4) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  int startX = (currentStep % 2) * 16;
  int startY = (currentStep / 2) * 8;
  uint16_t color;
  const char* colorName;

  switch(currentStep) {
    case 0: color = RED; colorName = "VERMELHO"; break;
    case 1: color = GREEN; colorName = "VERDE"; break;
    case 2: color = BLUE; colorName = "AZUL"; break;
    case 3: color = YELLOW; colorName = "AMARELO"; break;
  }

  for (int y = startY; y < startY + 8; y++) {
    for (int x = startX; x < startX + 16; x++) {
      drawPixelWrapped(x, y, color);
    }
  }

  Serial.print("\n[WRAPPER] Quadrante ");
  Serial.print(currentStep + 1);
  Serial.print("/4 - ");
  Serial.println(colorName);
  Serial.print("  Area logica: X=");
  Serial.print(startX);
  Serial.print("-");
  Serial.print(startX + 15);
  Serial.print(", Y=");
  Serial.print(startY);
  Serial.print("-");
  Serial.println(startY + 7);
  Serial.println("  -> OBSERVE: O quadrante aparece na posicao correta?");

  waitForNext();
}

// Teste 11: Quadrantes coloridos RAW
void testQuadrantsRaw() {
  if (currentStep >= 4) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  int startX = (currentStep % 2) * 16;
  int startY = (currentStep / 2) * 8;
  uint16_t color;
  const char* colorName;

  switch(currentStep) {
    case 0: color = RED; colorName = "VERMELHO"; break;
    case 1: color = GREEN; colorName = "VERDE"; break;
    case 2: color = BLUE; colorName = "AZUL"; break;
    case 3: color = YELLOW; colorName = "AMARELO"; break;
  }

  for (int y = startY; y < startY + 8; y++) {
    for (int x = startX; x < startX + 16; x++) {
      drawPixelRaw(x, y, color);
    }
  }

  Serial.print("\n[RAW] Quadrante ");
  Serial.print(currentStep + 1);
  Serial.print("/4 - ");
  Serial.println(colorName);
  Serial.print("  Area: X=");
  Serial.print(startX);
  Serial.print("-");
  Serial.print(startX + 15);
  Serial.print(", Y=");
  Serial.print(startY);
  Serial.print("-");
  Serial.println(startY + 7);
  Serial.println("  -> OBSERVE: O quadrante aparece na posicao correta?");

  waitForNext();
}

// Teste 12: Y fixo - todas as colunas de uma linha
void testFixedY() {
  if (currentStep >= 16) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  // Desenha todos os X da linha Y = currentStep, com cor variando por X
  for (int x = 0; x < 32; x++) {
    // Cor baseada na posição X (gradiente)
    uint8_t r = (x < 16) ? 255 : 0;
    uint8_t g = (x >= 8 && x < 24) ? 255 : 0;
    uint8_t b = (x >= 16) ? 255 : 0;
    uint16_t color = dma_display->color565(r, g, b);
    drawPixelWrapped(x, currentStep, color);
  }

  Serial.print("\n[Y FIXO] Linha Y=");
  Serial.print(currentStep);
  Serial.println(" com todos os X (0-31)");
  Serial.println("  Cores: X[0-7]=Vermelho, X[8-15]=Amarelo, X[16-23]=Ciano, X[24-31]=Azul");
  Serial.print("  remapY(");
  Serial.print(currentStep);
  Serial.print(") = ");
  Serial.println(remapY(currentStep));
  Serial.println("  -> OBSERVE: A linha esta continua? Cores na ordem certa?");

  waitForNext();
}

// Teste 13: X fixo - todas as linhas de uma coluna
void testFixedX() {
  if (currentStep >= 32) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  // Desenha todos os Y da coluna X = currentStep, com cor variando por Y
  for (int y = 0; y < 16; y++) {
    // Cor baseada na posição Y
    uint8_t r = (y < 8) ? 255 : 0;
    uint8_t g = (y >= 4 && y < 12) ? 255 : 0;
    uint8_t b = (y >= 8) ? 255 : 0;
    uint16_t color = dma_display->color565(r, g, b);
    drawPixelWrapped(currentStep, y, color);
  }

  Serial.print("\n[X FIXO] Coluna X=");
  Serial.print(currentStep);
  Serial.println(" com todos os Y (0-15)");
  Serial.println("  Cores: Y[0-3]=Vermelho, Y[4-7]=Amarelo, Y[8-11]=Ciano, Y[12-15]=Azul");
  Serial.println("  -> OBSERVE: A coluna esta continua? Cores na ordem certa?");

  waitForNext();
}

// Teste 14: Grid 8x8 com marcadores
void testGrid8x8() {
  if (currentStep >= 1) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  // Desenhar grid de referência
  // Linhas horizontais a cada 4 pixels
  for (int y = 0; y < 16; y += 4) {
    for (int x = 0; x < 32; x++) {
      drawPixelWrapped(x, y, dma_display->color565(50, 50, 50)); // Cinza escuro
    }
  }

  // Linhas verticais a cada 8 pixels
  for (int x = 0; x < 32; x += 8) {
    for (int y = 0; y < 16; y++) {
      drawPixelWrapped(x, y, dma_display->color565(50, 50, 50)); // Cinza escuro
    }
  }

  // Marcar cantos com cores diferentes
  // Canto superior esquerdo (0,0) - VERMELHO
  drawPixelWrapped(0, 0, RED);
  drawPixelWrapped(1, 0, RED);
  drawPixelWrapped(0, 1, RED);

  // Canto superior direito (31,0) - VERDE
  drawPixelWrapped(31, 0, GREEN);
  drawPixelWrapped(30, 0, GREEN);
  drawPixelWrapped(31, 1, GREEN);

  // Canto inferior esquerdo (0,15) - AZUL
  drawPixelWrapped(0, 15, BLUE);
  drawPixelWrapped(1, 15, BLUE);
  drawPixelWrapped(0, 14, BLUE);

  // Canto inferior direito (31,15) - AMARELO
  drawPixelWrapped(31, 15, YELLOW);
  drawPixelWrapped(30, 15, YELLOW);
  drawPixelWrapped(31, 14, YELLOW);

  // Centro (16,8) - BRANCO
  drawPixelWrapped(15, 7, WHITE);
  drawPixelWrapped(16, 7, WHITE);
  drawPixelWrapped(15, 8, WHITE);
  drawPixelWrapped(16, 8, WHITE);

  Serial.println("\n[GRID] Marcadores nos cantos e centro:");
  Serial.println("  Vermelho = Canto sup-esq (0,0)");
  Serial.println("  Verde = Canto sup-dir (31,0)");
  Serial.println("  Azul = Canto inf-esq (0,15)");
  Serial.println("  Amarelo = Canto inf-dir (31,15)");
  Serial.println("  Branco = Centro (15-16, 7-8)");
  Serial.println("  Grid cinza a cada 4 linhas e 8 colunas");
  Serial.println("  -> OBSERVE: Os cantos estao corretos? Centro?");

  currentStep++;
  waitForNext();
}

// Teste 15: Preencher tela inteira
void testFillScreen() {
  if (currentStep >= 6) {
    Serial.println("\n=== TESTE COMPLETO ===");
    currentMode = 0;
    currentStep = 0;
    return;
  }

  clearScreen();

  uint16_t color;
  const char* colorName;

  switch(currentStep) {
    case 0: color = RED; colorName = "VERMELHO"; break;
    case 1: color = GREEN; colorName = "VERDE"; break;
    case 2: color = BLUE; colorName = "AZUL"; break;
    case 3: color = WHITE; colorName = "BRANCO"; break;
    case 4: color = YELLOW; colorName = "AMARELO"; break;
    case 5: color = CYAN; colorName = "CIANO"; break;
  }

  // Preencher com wrapper
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 32; x++) {
      drawPixelWrapped(x, y, color);
    }
  }

  Serial.print("\n[FILL] Tela inteira - ");
  Serial.println(colorName);
  Serial.println("  -> OBSERVE: Toda a tela esta preenchida uniformemente?");
  Serial.println("             Existem linhas ou areas falhadas?");

  waitForNext();
}

// ============ SETUP ============

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════════════════════╗");
  Serial.println("║     P10 32x16 DIAGNOSTIC TEST - INICIALIZANDO...      ║");
  Serial.println("╚═══════════════════════════════════════════════════════╝");

  // Configurar pinos HUB75
  HUB75_I2S_CFG::i2s_pins _pins = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };

  // Configuração do display
  HUB75_I2S_CFG mxconfig(32, 16, 1, _pins);
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG; // Driver padrão

  // Inicializar display
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setBrightness8(128);

  if (!dma_display->begin()) {
    Serial.println("ERRO FATAL: Display nao inicializou!");
    while(1) delay(1000);
  }

  // Definir cores
  RED = dma_display->color565(255, 0, 0);
  GREEN = dma_display->color565(0, 255, 0);
  BLUE = dma_display->color565(0, 0, 255);
  WHITE = dma_display->color565(255, 255, 255);
  YELLOW = dma_display->color565(255, 255, 0);
  CYAN = dma_display->color565(0, 255, 255);
  MAGENTA = dma_display->color565(255, 0, 255);

  clearScreen();

  Serial.println("\nDisplay inicializado com sucesso!");
  Serial.println("Dimensoes: 32x16 pixels");

  showMenu();
}

// ============ LOOP ============

void loop() {
  // Se não há modo selecionado, aguarda input do menu
  if (currentMode == 0) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      int mode = input.toInt();

      if (mode >= 1 && mode <= 15) {
        currentMode = mode;
        currentStep = 0;
        Serial.print("\nIniciando teste ");
        Serial.println(mode);
      } else {
        Serial.println("Opcao invalida!");
        showMenu();
      }
    }
    return;
  }

  // Executar o teste do modo atual
  switch(currentMode) {
    case 1: testPixelByPixelWrapped(); break;
    case 2: testPixelByPixelRaw(); break;
    case 3: testHorizontalLinesWrapped(); break;
    case 4: testHorizontalLinesRaw(); break;
    case 5: testVerticalColumnsWrapped(); break;
    case 6: testVerticalColumnsRaw(); break;
    case 7: testCharactersWrapped(); break;
    case 8: testCharactersRaw(); break;
    case 9: testComparisonSideBySide(); break;
    case 10: testQuadrantsWrapped(); break;
    case 11: testQuadrantsRaw(); break;
    case 12: testFixedY(); break;
    case 13: testFixedX(); break;
    case 14: testGrid8x8(); break;
    case 15: testFillScreen(); break;
    default:
      currentMode = 0;
      showMenu();
      break;
  }
}
