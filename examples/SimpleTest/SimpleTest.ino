/*
 * Exemplo Simples - P10 32x16 Quarter Scan
 *
 * Testa as funcionalidades básicas do painel P10
 *
 * IMPORTANTE: O display base DEVE ser configurado como 64x8!
 * Isto é necessário para a solução #680 funcionar corretamente.
 */

#include <P10_32x16_QuarterScan.h>

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
P10_32x16_QuarterScan *display = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== P10 32x16 - Teste Simples ===");

  // Configurar pinos
  HUB75_I2S_CFG::i2s_pins _pins = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };

  // ============================================================
  // CONFIGURAÇÃO CRÍTICA: 64x8, NÃO 32x16!
  // ============================================================
  // A solução #680 requer que o driver seja configurado como 64x8
  // para eliminar a duplicação de pixels (+16 colunas ghost).
  //
  // Ver: https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/680
  // ============================================================
  HUB75_I2S_CFG mxconfig(64, 8, 1, _pins);  // 64x8, NÃO 32x16!
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setBrightness8(100);

  if (!dma_display->begin()) {
    Serial.println("ERRO: Display não inicializou!");
    while(1);
  }

  dma_display->clearScreen();

  // Criar wrapper P10
  // O wrapper expõe interface 32x16 e faz o mapeamento para 64x8 internamente
  display = new P10_32x16_QuarterScan(dma_display);

  Serial.println("Display P10 inicializado!");
  Serial.println("Config: 64x8 base, 32x16 lógico (fórmula #680)");
}

void loop() {
  // Teste 1: Linha vermelha no topo
  display->clearScreen();
  display->drawLine(0, 0, 31, 0, display->color565(255, 0, 0));
  Serial.println("Teste 1: Linha vermelha Y=0");
  delay(2000);

  // Teste 2: Linha verde no meio
  display->clearScreen();
  display->drawLine(0, 8, 31, 8, display->color565(0, 255, 0));
  Serial.println("Teste 2: Linha verde Y=8");
  delay(2000);

  // Teste 3: Linha azul no fundo
  display->clearScreen();
  display->drawLine(0, 15, 31, 15, display->color565(0, 0, 255));
  Serial.println("Teste 3: Linha azul Y=15");
  delay(2000);

  // Teste 4: Preencher metade superior/inferior
  display->clearScreen();
  display->fillRect(0, 0, 32, 8, display->color565(100, 0, 100)); // Roxo em cima
  display->fillRect(0, 8, 32, 8, display->color565(0, 100, 100)); // Ciano em baixo
  Serial.println("Teste 4: Quadrantes roxo/ciano");
  delay(2000);

  // Teste 5: Gradiente vertical
  display->clearScreen();
  for(int y = 0; y < 16; y++) {
    uint8_t brightness = map(y, 0, 15, 30, 255);
    for(int x = 0; x < 32; x++) {
      display->drawPixel(x, y, display->color565(brightness, brightness, 0));
    }
  }
  Serial.println("Teste 5: Gradiente amarelo");
  delay(2000);

  // Teste 6: Texto (demonstra que Adafruit_GFX funciona!)
  display->clearScreen();
  display->setTextColor(display->color565(255, 255, 255));
  display->setTextSize(1);
  display->setCursor(1, 4);
  display->print("P10");
  Serial.println("Teste 6: Texto 'P10'");
  delay(2000);

  // Teste 7: Pixels individuais nos 4 cantos
  display->clearScreen();
  display->drawPixel(0, 0, display->color565(255, 0, 0));     // Vermelho - canto sup-esq
  display->drawPixel(31, 0, display->color565(0, 255, 0));    // Verde - canto sup-dir
  display->drawPixel(0, 15, display->color565(0, 0, 255));    // Azul - canto inf-esq
  display->drawPixel(31, 15, display->color565(255, 255, 0)); // Amarelo - canto inf-dir
  Serial.println("Teste 7: Pixels nos 4 cantos");
  delay(2000);
}
