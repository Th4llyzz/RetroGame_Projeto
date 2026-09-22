#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// ==========================================
// CONFIGURAÇÕES E PINOS
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int JOYSTICK_X = 34;
const int BOTAO = 14;

const int LED_VIDA_1 = 16;
const int LED_VIDA_2 = 17;
const int LED_VIDA_3 = 18;

const int BUZZER = 27;

Preferences memoria;
int highScore = 0;

// ==========================================
// SPRITES CORRIGIDOS (16x16 pixels - 2 bytes/linha)
// ==========================================

// Nave do Jogador (16x16)
const unsigned char PROGMEM spriteNave[] = {
  0x01, 0x80, // .....**.........
  0x01, 0x80, // .....**.........
  0x03, 0xC0, // ....****........
  0x03, 0xC0, // ....****........
  0x07, 0xE0, // ...******.......
  0x0F, 0xF0, // ..********......
  0x1F, 0xF8, // .**********.....
  0x3F, 0xFC, // **************..
  0x7F, 0xFE, // ****************
  0x7F, 0xFE, // ****************
  0xE3, 0xC7, // ***..****..***..
  0xC1, 0x83, // **....**....**..
  0x81, 0x81, // *.....**.....*..
  0x80, 0x01, // *............*..
  0x00, 0x00,
  0x00, 0x00
};

// Inimigo Alienígena (16x16)
const unsigned char PROGMEM spriteInimigo[] = {
  0x07, 0xE0, // ...******.......
  0x1F, 0xF8, // .**********.....
  0x3F, 0xFC, // **************..
  0x73, 0xCE, // ***..**..**..***
  0x7F, 0xFE, // ****************
  0x7F, 0xFE, // ****************
  0x3F, 0xFC, // **************..
  0x1F, 0xF8, // .**********.....
  0x0E, 0x70, // ...***..***.....
  0x1C, 0x38, // ..***....***....
  0x38, 0x1C, // .***......***...
  0x30, 0x0C, // .**........**...
  0x20, 0x04, // .*..........*...
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00
};

// ==========================================
// ESTADOS E VARIÁVEIS DO JOGO
// ==========================================
enum Estado { MENU, JOGANDO, GAME_OVER, SCORE };
Estado estadoAtual = MENU;

int jogadorX = 56;
const int jogadorY = 46; // Posicionada para acomodar 16px de altura
int vidas = 3;
int score = 0;

// Tiro
bool tiroAtivo = false;
int tiroX = 0;
int tiroY = 0;
unsigned long ultimoTiro = 0;
const int intervaloTiro = 250;

// Inimigos
const int MAX_INIMIGOS = 4;
int inimigoX[MAX_INIMIGOS];
int inimigoY[MAX_INIMIGOS];
bool inimigoAtivo[MAX_INIMIGOS];

// Dificuldade e Tempo
int velocidadeInimigo = 1;
int intervaloInimigo = 300;
unsigned long ultimoMovimento = 0;

// Estrelas de Fundo (Starfield Dinâmico)
const int NUM_ESTRELAS = 15;
int estrelasX[NUM_ESTRELAS];
int estrelasY[NUM_ESTRELAS];
int estrelasVel[NUM_ESTRELAS];

// Menu e Tratamento de Botão
int opcaoMenu = 0;
unsigned long ultimoBotao = 0;
unsigned long tempoEntradaEstado = 0;

// Protótipos de Funções
void mostrarMenu();
void loopMenu();
void iniciarJogo();
void loopJogo();
void controlarJogador();
void controlarTiro();
void criarInimigo();
void atualizarInimigos();
void verificarColisao();
void desenharJogo();
void atualizarEstrelas();
void perderVida();
void atualizarLEDs();
void mostrarGameOver();
void loopGameOver();
void mostrarScore();
void loopScore();
bool botaoPressionado();

void somInicio();
void somTiro();
void somInimigo();
void somPerdeVida();

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED nao encontrado!"));
    while (true);
  }

  display.clearDisplay();
  display.display();

  pinMode(BOTAO, INPUT_PULLUP);
  pinMode(LED_VIDA_1, OUTPUT);
  pinMode(LED_VIDA_2, OUTPUT);
  pinMode(LED_VIDA_3, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  memoria.begin("spaceattack", false);
  highScore = memoria.getInt("highscore", 0);

  randomSeed(analogRead(35));
  for (int i = 0; i < NUM_ESTRELAS; i++) {
    estrelasX[i] = random(0, 128);
    estrelasY[i] = random(0, 64);
    estrelasVel[i] = random(1, 3);
  }

  atualizarLEDs();
  mostrarMenu();
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
  switch (estadoAtual) {
    case MENU:      loopMenu(); break;
    case JOGANDO:   loopJogo(); break;
    case GAME_OVER: loopGameOver(); break;
    case SCORE:     loopScore(); break;
  }
}

// ==========================================
// MENU
// ==========================================
void mostrarMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.drawRect(8, 2, 112, 38, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(34, 8);
  display.print("SPACE");
  display.setCursor(28, 22);
  display.print("ATTACK");

  display.setTextSize(1);
  if (opcaoMenu == 0) {
    display.setCursor(10, 48);
    display.print("> INICIAR <");
    display.setCursor(78, 48);
    display.print(" SCORE ");
  } else {
    display.setCursor(10, 48);
    display.print("  INICIAR  ");
    display.setCursor(74, 48);
    display.print("> SCORE <");
  }

  display.display();
}

void loopMenu() {
  int valorX = analogRead(JOYSTICK_X);

  if (valorX < 1000 && opcaoMenu != 0) {
    opcaoMenu = 0;
    mostrarMenu();
    delay(150);
  } else if (valorX > 3000 && opcaoMenu != 1) {
    opcaoMenu = 1;
    mostrarMenu();
    delay(150);
  }

  if (botaoPressionado()) {
    if (opcaoMenu == 0) {
      iniciarJogo();
    } else {
      estadoAtual = SCORE;
      tempoEntradaEstado = millis();
      mostrarScore();
    }
  }
}

// ==========================================
// LÓGICA DO JOGO
// ==========================================
void iniciarJogo() {
  estadoAtual = JOGANDO;
  jogadorX = 56;
  vidas = 3;
  score = 0;
  tiroAtivo = false;
  velocidadeInimigo = 1;
  intervaloInimigo = 300;
  ultimoMovimento = millis();

  for (int i = 0; i < MAX_INIMIGOS; i++) {
    inimigoAtivo[i] = false;
  }

  atualizarLEDs();
  criarInimigo();
  somInicio();
}

void loopJogo() {
  controlarJogador();
  controlarTiro();
  atualizarInimigos();
  atualizarEstrelas();
  verificarColisao();
  desenharJogo();
}

void controlarJogador() {
  int valorX = analogRead(JOYSTICK_X);

  if (valorX < 1200) jogadorX -= 3;
  if (valorX > 2800) jogadorX += 3;

  // Ajustado para nave de 16px de largura
  if (jogadorX < 0) jogadorX = 0;
  if (jogadorX > 112) jogadorX = 112;
}

void controlarTiro() {
  if (digitalRead(BOTAO) == LOW) {
    if (!tiroAtivo && (millis() - ultimoTiro > intervaloTiro)) {
      tiroAtivo = true;
      tiroX = jogadorX + 7; // Tiro alinhado no centro do sprite 16x16
      tiroY = jogadorY - 2;
      ultimoTiro = millis();
      somTiro();
    }
  }

  if (tiroAtivo) {
    tiroY -= 5;
    if (tiroY < 0) tiroAtivo = false;
  }
}

void criarInimigo() {
  for (int i = 0; i < MAX_INIMIGOS; i++) {
    if (!inimigoAtivo[i]) {
      inimigoAtivo[i] = true;
      inimigoX[i] = random(0, 112);
      inimigoY[i] = random(-32, -16);
      return;
    }
  }
}

void atualizarInimigos() {
  if (millis() - ultimoMovimento >= intervaloInimigo) {
    ultimoMovimento = millis();

    for (int i = 0; i < MAX_INIMIGOS; i++) {
      if (inimigoAtivo[i]) {
        inimigoY[i] += velocidadeInimigo;

        if (inimigoY[i] >= jogadorY) {
          inimigoAtivo[i] = false;
          perderVida();
        }
      }
    }

    if (random(0, 100) < 35) {
      criarInimigo();
    }
  }
}

void verificarColisao() {
  if (!tiroAtivo) return;

  for (int i = 0; i < MAX_INIMIGOS; i++) {
    if (inimigoAtivo[i]) {
      // Detecção de colisão ajustada para centro do sprite 16x16
      if (abs(tiroX - (inimigoX[i] + 8)) < 9 && abs(tiroY - (inimigoY[i] + 8)) < 9) {
        inimigoAtivo[i] = false;
        tiroAtivo = false;
        score += 10;

        somInimigo();

        if (score >= 600)      { velocidadeInimigo = 3; intervaloInimigo = 110; }
        else if (score >= 400) { velocidadeInimigo = 2; intervaloInimigo = 140; }
        else if (score >= 250) { velocidadeInimigo = 2; intervaloInimigo = 170; }
        else if (score >= 150) { velocidadeInimigo = 2; intervaloInimigo = 200; }
        else if (score >= 100) { velocidadeInimigo = 1; intervaloInimigo = 220; }
        else if (score >= 50)  { velocidadeInimigo = 1; intervaloInimigo = 260; }

        if (score > highScore) {
          highScore = score;
          memoria.putInt("highscore", highScore);
        }
      }
    }
  }
}

// ==========================================
// RENDERIZAÇÃO
// ==========================================
void atualizarEstrelas() {
  for (int i = 0; i < NUM_ESTRELAS; i++) {
    estrelasY[i] += estrelasVel[i];
    if (estrelasY[i] >= 64) {
      estrelasY[i] = 0;
      estrelasX[i] = random(0, 128);
    }
  }
}

void desenharJogo() {
  display.clearDisplay();

  for (int i = 0; i < NUM_ESTRELAS; i++) {
    display.drawPixel(estrelasX[i], estrelasY[i], SSD1306_WHITE);
  }

  // Interface HUD
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PTS:");
  display.print(score);

  display.setCursor(95, 0);
  display.print("HP:");
  display.print(vidas);

  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  // Renderiza a nave com 16x16
  display.drawBitmap(jogadorX, jogadorY, spriteNave, 16, 16, SSD1306_WHITE);

  // Tiro
  if (tiroAtivo) {
    display.drawFastVLine(tiroX, tiroY, 4, SSD1306_WHITE);
  }

  // Renderiza inimigos com 16x16
  for (int i = 0; i < MAX_INIMIGOS; i++) {
    if (inimigoAtivo[i]) {
      display.drawBitmap(inimigoX[i], inimigoY[i], spriteInimigo, 16, 16, SSD1306_WHITE);
    }
  }

  display.display();
}

// ==========================================
// VIDAS E GAME OVER
// ==========================================
void perderVida() {
  somPerdeVida();

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_VIDA_1, HIGH);
    digitalWrite(LED_VIDA_2, HIGH);
    digitalWrite(LED_VIDA_3, HIGH);
    delay(80);
    digitalWrite(LED_VIDA_1, LOW);
    digitalWrite(LED_VIDA_2, LOW);
    digitalWrite(LED_VIDA_3, LOW);
    delay(80);
  }

  vidas--;
  atualizarLEDs();

  if (vidas <= 0) {
    estadoAtual = GAME_OVER;
    tempoEntradaEstado = millis();
    mostrarGameOver();
  }
}

void atualizarLEDs() {
  digitalWrite(LED_VIDA_1, vidas >= 1 ? HIGH : LOW);
  digitalWrite(LED_VIDA_2, vidas >= 2 ? HIGH : LOW);
  digitalWrite(LED_VIDA_3, vidas >= 3 ? HIGH : LOW);
}

void mostrarGameOver() {
  display.clearDisplay();
  
  display.drawRect(4, 4, 120, 56, SSD1306_WHITE);
  
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(12, 10);
  display.print("GAME OVER");

  display.setTextSize(1);
  display.setCursor(32, 30);
  display.print("SCORE: ");
  display.print(score);

  display.setCursor(14, 45);
  display.print("APERTE O BOTAO");

  display.display();
}

void loopGameOver() {
  if (millis() - tempoEntradaEstado > 500) {
    if (botaoPressionado()) {
      opcaoMenu = 0;
      estadoAtual = MENU;
      atualizarLEDs();
      mostrarMenu();
    }
  }
}

void mostrarScore() {
  display.clearDisplay();

  display.drawRect(4, 4, 120, 56, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(32, 12);
  display.print("HIGH SCORE");

  display.setTextSize(2);
  display.setCursor(40, 26);
  display.print(highScore);

  display.setTextSize(1);
  display.setCursor(18, 46);
  display.print("APERTE O BOTAO");

  display.display();
}

void loopScore() {
  if (millis() - tempoEntradaEstado > 300) {
    if (botaoPressionado()) {
      estadoAtual = MENU;
      mostrarMenu();
    }
  }
}

// ==========================================
// UTILITÁRIOS E SONS
// ==========================================
bool botaoPressionado() {
  if (digitalRead(BOTAO) == LOW) {
    if (millis() - ultimoBotao > 200) {
      ultimoBotao = millis();
      return true;
    }
  }
  return false;
}

void somInicio() {
  tone(BUZZER, 523, 100); delay(120);
  tone(BUZZER, 659, 100); delay(120);
  tone(BUZZER, 784, 150); delay(180);
  noTone(BUZZER);
}

void somTiro() {
  tone(BUZZER, 900, 40);
}

void somInimigo() {
  tone(BUZZER, 700, 50); delay(55);
  tone(BUZZER, 450, 70); delay(75);
  noTone(BUZZER);
}

void somPerdeVida() {
  tone(BUZZER, 350, 150); delay(170);
  tone(BUZZER, 250, 180); delay(200);
  noTone(BUZZER);
}