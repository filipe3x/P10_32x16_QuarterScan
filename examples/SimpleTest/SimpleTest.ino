/*
 * Exemplo Simples - P10 32x16 Quarter Scan
 * 
 * Testa as funcionalidades básicas do painel P10
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
  
  // Criar display base
  HUB75_I2S_CFG mxconfig(32, 16, 1, _pins);
  mxconfig.clkphase = false;
  
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setBrightness8(100);
  
  if (!dma_display->begin()) {
    Serial.println("ERRO: Display não inicializou!");
    while(1);
  }
  
  dma_display->clearScreen();
  
  // Criar wrapper P10
  display = new P10_32x16_QuarterScan(dma_display);
  
  Serial.println("Display P10 inicializado!");
}

void loop() {
  // Teste 1: Linha vermelha no topo
  display->clearScreen();
  display->drawLine(0, 0, 31, 0, display->color565(255, 0, 0));
  delay(2000);
  
  // Teste 2: Linha verde no meio
  display->clearScreen();
  display->drawLine(0, 8, 31, 8, display->color565(0, 255, 0));
  delay(2000);
  
  // Teste 3: Linha azul no fundo
  display->clearScreen();
  display->drawLine(0, 15, 31, 15, display->color565(0, 0, 255));
  delay(2000);
  
  // Teste 4: Preencher metade superior/inferior
  display->clearScreen();
  display->fillRect(0, 0, 32, 8, display->color565(100, 0, 100)); // Roxo em cima
  display->fillRect(0, 8, 32, 8, display->color565(0, 100, 100)); // Ciano em baixo
  delay(2000);
  
  // Teste 5: Gradiente
  display->clearScreen();
  for(int y = 0; y < 16; y++) {
    uint8_t brightness = map(y, 0, 15, 30, 255);
    for(int x = 0; x < 32; x++) {
      display->drawPixel(x, y, display->color565(brightness, brightness, 0));
    }
  }
  delay(2000);
}
