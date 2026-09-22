#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// ============================================================
// RETROGAME - ESP32 + OLED 128x64
// Jogos: SpaceWAR, Flappy Bird e Tetris
// Controles:
//   JOYSTICK_X = esquerda/direita
//   BOTAO      = selecionar / atirar / pular / girar
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------------------- PINOS --------------------
const int JOYSTICK_X = 34;
const int BOTAO = 14;

const int LED_VIDA_1 = 16;
const int LED_VIDA_2 = 17;
const int LED_VIDA_3 = 18;

const int BUZZER = 27;

Preferences memoria;

// -------------------- RECORDES --------------------
int recordSpace = 0;
int recordFlappy = 0;
int recordTetris = 0;

// -------------------- ESTADOS --------------------
enum Estado {
  TELA_INICIAL,
  MENU_JOGOS,
  SPACEWAR,
  FLAPPY,
  TETRIS,
  RECORDES,
  GAME_OVER
};

Estado estadoAtual = TELA_INICIAL;
Estado jogoAnterior = TELA_INICIAL;

int opcaoMenu = 0;
unsigned long ultimoBotao = 0;
unsigned long entradaEstado = 0;

// ============================================================
// MELODIA DO MARIO (SINCRONIZADA E COM STACCATO)
// ============================================================
const int notasMario[] = {
  659, 659, 0, 659, 0, 523, 659, 0, 784, 0, 392, 0,
  523, 0, 392, 0, 330, 0, 440, 0, 494, 0, 466, 440, 0,
  392, 659, 784, 880, 0, 698, 784, 0, 659, 0, 523, 587, 494
};

const int duracaoMario[] = {
  125, 125, 125, 125, 125, 125, 250, 125, 250, 250, 250, 250,
  180, 100, 180, 100, 180, 100, 150, 50,  150, 50,  150, 150, 50,
  120, 120, 120, 150, 50,  150, 150, 50,  150, 50,  150, 150, 200
};

const int totalNotasMario = sizeof(notasMario) / sizeof(notasMario[0]);

int indiceNotaMenu = 0;
unsigned long ultimaNotaTempo = 0;

void tocarMusicaMenu() {
  unsigned long agora = millis();
  
  if (agora - ultimaNotaTempo >= (unsigned long)duracaoMario[indiceNotaMenu]) {
    ultimaNotaTempo = agora;
    
    int freq = notasMario[indiceNotaMenu];
    
    if (freq > 0) {
      tone(BUZZER, freq, (duracaoMario[indiceNotaMenu] * 85) / 100);
    } else {
      noTone(BUZZER);
    }
    
    indiceNotaMenu = (indiceNotaMenu + 1) % totalNotasMario;
  }
}

// ============================================================
// SPRITES DO SPACEWAR
// ============================================================

const unsigned char PROGMEM spriteNave[] = {
  0x01, 0x80,
  0x01, 0x80,
  0x03, 0xC0,
  0x03, 0xC0,
  0x07, 0xE0,
  0x0F, 0xF0,
  0x1F, 0xF8,
  0x3F, 0xFC,
  0x7F, 0xFE,
  0x7F, 0xFE,
  0xE3, 0xC7,
  0xC1, 0x83,
  0x81, 0x81,
  0x80, 0x01,
  0x00, 0x00,
  0x00, 0x00
};

const unsigned char PROGMEM spriteInimigo[] = {
  0x07, 0xE0,
  0x1F, 0xF8,
  0x3F, 0xFC,
  0x73, 0xCE,
  0x7F, 0xFE,
  0x7F, 0xFE,
  0x3F, 0xFC,
  0x1F, 0xF8,
  0x0E, 0x70,
  0x1C, 0x38,
  0x38, 0x1C,
  0x30, 0x0C,
  0x20, 0x04,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00
};

// ============================================================
// SPACEWAR
// ============================================================

int jogadorX = 56;
const int jogadorY = 46;
int vidas = 3;
int scoreSpace = 0;

bool tiroAtivo = false;
int tiroX = 0;
int tiroY = 0;
unsigned long ultimoTiro = 0;
const int intervaloTiro = 250;

const int MAX_INIMIGOS = 4;
int inimigoX[MAX_INIMIGOS];
int inimigoY[MAX_INIMIGOS];
bool inimigoAtivo[MAX_INIMIGOS];

int velocidadeInimigo = 1;
int intervaloInimigo = 300;
unsigned long ultimoMovimento = 0;

const int NUM_ESTRELAS = 15;
int estrelasX[NUM_ESTRELAS];
int estrelasY[NUM_ESTRELAS];
int estrelasVel[NUM_ESTRELAS];

// ============================================================
// FLAPPY BIRD
// ============================================================

float birdY;
float birdVel;
int birdX = 25;

int pipeX[2];
int pipeGapY[2];
bool pipePassou[2];

int scoreFlappy = 0;
unsigned long ultimoPipeMovimento = 0;
const int pipeGap = 20;
const int pipeWidth = 10;
const float gravidade = 0.28;
const float forcaPulo = -2.9;

// ============================================================
// TETRIS
// ============================================================

const int T_COLS = 10;
const int T_ROWS = 16;
bool board[T_ROWS][T_COLS];

const int pecas[7][4][4] = {
  // I
  {
    {0,0,0,0},
    {1,1,1,1},
    {0,0,0,0},
    {0,0,0,0}
  },
  // O
  {
    {0,1,1,0},
    {0,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // T
  {
    {0,1,0,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // L
  {
    {1,0,0,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // J
  {
    {0,0,1,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // S
  {
    {0,1,1,0},
    {1,1,0,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // Z
  {
    {1,1,0,0},
    {0,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  }
};

int tPeca;
int tX;
int tY;
int tRot;
int scoreTetris;
unsigned long ultimoDropTetris = 0;
unsigned long ultimoMovimentoJoystick = 0;
const int TAM_BLOCO = 4;

// ============================================================
// PROTÓTIPOS
// ============================================================

void telaInicial();
void menuJogos();
void recordes();

bool botaoPressionado();

void iniciarSpacewar();
void loopSpacewar();
void controlarSpacewar();
void criarInimigo();
void atualizarInimigos();
void verificarColisaoSpacewar();
void desenharSpacewar();
void atualizarEstrelas();
void perderVida();
void atualizarLEDs();

void iniciarFlappy();
void loopFlappy();
void desenharFlappy();
void gerarPipe(int indice);
void gameOverFlappy();

void iniciarTetris();
void loopTetris();
void novaPecaTetris();
bool podeMoverTetris(int nx, int ny, int nr);
void fixarPecaTetris();
void limparLinhasTetris();
void desenharTetris();
void gameOverTetris();

void mostrarGameOver(const char* nomeJogo, int pontuacao);
void voltarAoMenu();

void somInicio();
void somTiro();
void somInimigo();
void somPerdeVida();
void somPonto();

// ============================================================
// SETUP
// ============================================================

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

  memoria.begin("retrogame", false);

  recordSpace = memoria.getInt("space", 0);
  recordFlappy = memoria.getInt("flappy", 0);
  recordTetris = memoria.getInt("tetris", 0);

  randomSeed(analogRead(35));

  for (int i = 0; i < NUM_ESTRELAS; i++) {
    estrelasX[i] = random(0, 128);
    estrelasY[i] = random(0, 64);
    estrelasVel[i] = random(1, 3);
  }

  atualizarLEDs();
  telaInicial();
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  switch (estadoAtual) {

    case TELA_INICIAL:
      if (botaoPressionado()) {
        estadoAtual = MENU_JOGOS;
        opcaoMenu = 0;
        menuJogos();
      }
      break;

    case MENU_JOGOS:
      menuJogos();
      break;

    case SPACEWAR:
      loopSpacewar();
      break;

    case FLAPPY:
      loopFlappy();
      break;

    case TETRIS:
      loopTetris();
      break;

    case RECORDES:
      recordes();
      break;

    case GAME_OVER:
      if (millis() - entradaEstado > 500) {
        if (botaoPressionado()) {
          voltarAoMenu();
        }
      }
      break;
  }
}

// ============================================================
// TELA INICIAL
// ============================================================

void telaInicial() {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(22, 10);
  display.print("Retro");

  display.setCursor(22, 30);
  display.print("Game");

  display.setTextSize(1);
  display.setCursor(15, 53);
  display.print("APERTE O BOTAO");

  display.display();
}

// ============================================================
// MENU PRINCIPAL
// ============================================================

void menuJogos() {
  tocarMusicaMenu();

  static unsigned long ultimaTela = 0;

  if (millis() - ultimaTela < 50) return;
  ultimaTela = millis();

  int valorX = analogRead(JOYSTICK_X);

  if (millis() - ultimoMovimentoJoystick > 180) {

    if (valorX < 1000) {
      opcaoMenu--;
      if (opcaoMenu < 0) opcaoMenu = 3;
      ultimoMovimentoJoystick = millis();
    }

    if (valorX > 3000) {
      opcaoMenu++;
      if (opcaoMenu > 3) opcaoMenu = 0;
      ultimoMovimentoJoystick = millis();
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("RETROGAME");

  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  const char* nomes[] = {
    "SPACEWAR",
    "FLAPPY BIRD",
    "TETRIS",
    "RECORDES"
  };

  int primeira = (opcaoMenu / 2) * 2;

  for (int i = 0; i < 2; i++) {
    int indice = primeira + i;

    display.setCursor(10, 20 + i * 18);

    if (indice == opcaoMenu) {
      display.print("> ");
    } else {
      display.print("  ");
    }

    display.print(nomes[indice]);
  }

  display.setCursor(5, 57);
  display.print("X: escolher  BOTAO: entrar");

  display.display();

  if (botaoPressionado()) {
    noTone(BUZZER);

    if (opcaoMenu == 0) {
      iniciarSpacewar();
    }
    else if (opcaoMenu == 1) {
      iniciarFlappy();
    }
    else if (opcaoMenu == 2) {
      iniciarTetris();
    }
    else {
      estadoAtual = RECORDES;
      entradaEstado = millis();
      recordes();
    }
  }
}

// ============================================================
// RECORDES
// ============================================================

void recordes() {
  static unsigned long ultimaTela = 0;

  if (millis() - ultimaTela < 80) return;
  ultimaTela = millis();

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(37, 2);
  display.print("RECORDES");

  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  display.setCursor(5, 20);
  display.print("SPACEWAR: ");
  display.print(recordSpace);

  display.setCursor(5, 31);
  display.print("FLAPPY:   ");
  display.print(recordFlappy);

  display.setCursor(5, 42);
  display.print("TETRIS:   ");
  display.print(recordTetris);

  display.setCursor(32, 55);
  display.print("BOTAO: VOLTAR");

  display.display();

  if (botaoPressionado()) {
    estadoAtual = MENU_JOGOS;
    opcaoMenu = 3;
    menuJogos();
  }
}

// ============================================================
// SPACEWAR - INICIO
// ============================================================

void iniciarSpacewar() {
  estadoAtual = SPACEWAR;

  jogadorX = 56;
  vidas = 3;
  scoreSpace = 0;

  tiroAtivo = false;

  velocidadeInimigo = 1;
  intervaloInimigo = 300;
  ultimoMovimento = millis();
  ultimoTiro = 0;

  for (int i = 0; i < MAX_INIMIGOS; i++) {
    inimigoAtivo[i] = false;
  }

  for (int i = 0; i < NUM_ESTRELAS; i++) {
    estrelasX[i] = random(0, 128);
    estrelasY[i] = random(0, 64);
  }

  atualizarLEDs();
  criarInimigo();
  somInicio();

  delay(150);
}

// ============================================================
// SPACEWAR - LOOP
// ============================================================

void loopSpacewar() {
  controlarSpacewar();
  atualizarInimigos();
  atualizarEstrelas();
  verificarColisaoSpacewar();
  desenharSpacewar();
}

void controlarSpacewar() {
  int valorX = analogRead(JOYSTICK_X);

  if (valorX < 1200) jogadorX -= 3;
  if (valorX > 2800) jogadorX += 3;

  if (jogadorX < 0) jogadorX = 0;
  if (jogadorX > 112) jogadorX = 112;

  if (digitalRead(BOTAO) == LOW) {
    if (!tiroAtivo && millis() - ultimoTiro > intervaloTiro) {
      tiroAtivo = true;
      tiroX = jogadorX + 7;
      tiroY = jogadorY - 2;
      ultimoTiro = millis();
      somTiro();
    }
  }

  if (tiroAtivo) {
    tiroY -= 5;

    if (tiroY < 0) {
      tiroAtivo = false;
    }
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

  if (millis() - ultimoMovimento >= (unsigned long)intervaloInimigo) {

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

void verificarColisaoSpacewar() {

  if (!tiroAtivo) return;

  for (int i = 0; i < MAX_INIMIGOS; i++) {

    if (inimigoAtivo[i]) {

      if (abs(tiroX - (inimigoX[i] + 8)) < 9 &&
          abs(tiroY - (inimigoY[i] + 8)) < 9) {

        inimigoAtivo[i] = false;
        tiroAtivo = false;

        scoreSpace += 10;

        somInimigo();

        if (scoreSpace >= 600) {
          velocidadeInimigo = 3;
          intervaloInimigo = 110;
        }
        else if (scoreSpace >= 400) {
          velocidadeInimigo = 2;
          intervaloInimigo = 140;
        }
        else if (scoreSpace >= 250) {
          velocidadeInimigo = 2;
          intervaloInimigo = 170;
        }
        else if (scoreSpace >= 150) {
          velocidadeInimigo = 2;
          intervaloInimigo = 200;
        }
        else if (scoreSpace >= 100) {
          velocidadeInimigo = 1;
          intervaloInimigo = 220;
        }
        else if (scoreSpace >= 50) {
          velocidadeInimigo = 1;
          intervaloInimigo = 260;
        }

        if (scoreSpace > recordSpace) {
          recordSpace = scoreSpace;
          memoria.putInt("space", recordSpace);
        }
      }
    }
  }
}

void atualizarEstrelas() {
  for (int i = 0; i < NUM_ESTRELAS; i++) {

    estrelasY[i] += estrelasVel[i];

    if (estrelasY[i] >= 64) {
      estrelasY[i] = 0;
      estrelasX[i] = random(0, 128);
    }
  }
}

void desenharSpacewar() {

  display.clearDisplay();

  for (int i = 0; i < NUM_ESTRELAS; i++) {
    display.drawPixel(estrelasX[i], estrelasY[i], SSD1306_WHITE);
  }

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("PTS:");
  display.print(scoreSpace);

  display.setCursor(95, 0);
  display.print("HP:");
  display.print(vidas);

  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  display.drawBitmap(
    jogadorX,
    jogadorY,
    spriteNave,
    16,
    16,
    SSD1306_WHITE
  );

  if (tiroAtivo) {
    display.drawFastVLine(
      tiroX,
      tiroY,
      4,
      SSD1306_WHITE
    );
  }

  for (int i = 0; i < MAX_INIMIGOS; i++) {

    if (inimigoAtivo[i]) {

      display.drawBitmap(
        inimigoX[i],
        inimigoY[i],
        spriteInimigo,
        16,
        16,
        SSD1306_WHITE
      );
    }
  }

  display.display();
}

// ============================================================
// VIDAS DO SPACEWAR
// ============================================================

void perderVida() {

  somPerdeVida();

  for (int i = 0; i < 2; i++) {

    digitalWrite(LED_VIDA_1, HIGH);
    digitalWrite(LED_VIDA_2, HIGH);
    digitalWrite(LED_VIDA_3, HIGH);

    delay(60);

    digitalWrite(LED_VIDA_1, LOW);
    digitalWrite(LED_VIDA_2, LOW);
    digitalWrite(LED_VIDA_3, LOW);

    delay(60);
  }

  vidas--;

  atualizarLEDs();

  if (vidas <= 0) {

    if (scoreSpace > recordSpace) {
      recordSpace = scoreSpace;
      memoria.putInt("space", recordSpace);
    }

    mostrarGameOver("SPACEWAR", scoreSpace);
  }
}

void atualizarLEDs() {

  digitalWrite(
    LED_VIDA_1,
    vidas >= 1 ? HIGH : LOW
  );

  digitalWrite(
    LED_VIDA_2,
    vidas >= 2 ? HIGH : LOW
  );

  digitalWrite(
    LED_VIDA_3,
    vidas >= 3 ? HIGH : LOW
  );
}

// ============================================================
// FLAPPY BIRD
// ============================================================

void iniciarFlappy() {

  estadoAtual = FLAPPY;

  birdX = 25;
  birdY = 30;
  birdVel = 0;

  scoreFlappy = 0;

  ultimoPipeMovimento = millis();

  gerarPipe(0);
  pipeX[0] = 128;

  gerarPipe(1);
  pipeX[1] = 192;

  pipePassou[0] = false;
  pipePassou[1] = false;

  delay(150);
}

void gerarPipe(int indice) {

  pipeGapY[indice] = random(16, 45);
  pipePassou[indice] = false;
}

void loopFlappy() {

  if (digitalRead(BOTAO) == LOW) {

    if (millis() - ultimoBotao > 160) {
      birdVel = forcaPulo;
      ultimoBotao = millis();
    }
  }

  int valorX = analogRead(JOYSTICK_X);

  if (valorX < 1000) {
    birdVel -= 0.12;
  }

  if (valorX > 3000) {
    birdVel += 0.12;
  }

  birdVel += gravidade;
  birdY += birdVel;

  if (millis() - ultimoPipeMovimento >= 35) {

    ultimoPipeMovimento = millis();

    for (int i = 0; i < 2; i++) {

      pipeX[i] -= 2;

      if (pipeX[i] + pipeWidth < 0) {

        pipeX[i] = 128 + random(0, 25);
        gerarPipe(i);
      }

      if (!pipePassou[i] &&
          pipeX[i] + pipeWidth < birdX) {

        scoreFlappy++;
        pipePassou[i] = true;
        somPonto();

        if (scoreFlappy > recordFlappy) {
          recordFlappy = scoreFlappy;
          memoria.putInt("flappy", recordFlappy);
        }
      }
    }
  }

  if (birdY < 10 || birdY > 58) {
    gameOverFlappy();
    return;
  }

  for (int i = 0; i < 2; i++) {

    if (birdX + 6 > pipeX[i] &&
        birdX < pipeX[i] + pipeWidth) {

      int centro = pipeGapY[i];

      if (birdY < centro - pipeGap / 2 ||
          birdY + 6 > centro + pipeGap / 2) {

        gameOverFlappy();
        return;
      }
    }
  }

  desenharFlappy();
}

void desenharFlappy() {

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("FLAPPY: ");
  display.print(scoreFlappy);

  display.setCursor(88, 0);
  display.print("REC:");
  display.print(recordFlappy);

  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  for (int i = 0; i < 2; i++) {

    int centro = pipeGapY[i];

    display.fillRect(
      pipeX[i],
      10,
      pipeWidth,
      (centro - pipeGap / 2) - 10,
      SSD1306_WHITE
    );

    display.fillRect(
      pipeX[i],
      centro + pipeGap / 2,
      pipeWidth,
      64 - (centro + pipeGap / 2),
      SSD1306_WHITE
    );
  }

  display.fillCircle(
    birdX + 3,
    (int)birdY + 3,
    3,
    SSD1306_WHITE
  );

  display.drawPixel(
    birdX + 5,
    (int)birdY + 2,
    SSD1306_BLACK
  );

  display.display();
}

void gameOverFlappy() {

  if (scoreFlappy > recordFlappy) {
    recordFlappy = scoreFlappy;
    memoria.putInt("flappy", recordFlappy);
  }

  mostrarGameOver("FLAPPY", scoreFlappy);
}

// ============================================================
// TETRIS
// ============================================================

void iniciarTetris() {

  estadoAtual = TETRIS;

  scoreTetris = 0;

  for (int y = 0; y < T_ROWS; y++) {
    for (int x = 0; x < T_COLS; x++) {
      board[y][x] = false;
    }
  }

  ultimoDropTetris = millis();

  novaPecaTetris();

  delay(150);
}

void novaPecaTetris() {

  tPeca = random(0, 7);
  tRot = 0;

  tX = 3;
  tY = 0;

  if (!podeMoverTetris(tX, tY, tRot)) {
    gameOverTetris();
  }
}

bool podeMoverTetris(int nx, int ny, int nr) {

  for (int py = 0; py < 4; py++) {

    for (int px = 0; px < 4; px++) {

      if (!pecas[tPeca][py][px]) continue;

      int rx = px;
      int ry = py;

      if (nr == 1) {
        rx = 3 - py;
        ry = px;
      }
      else if (nr == 2) {
        rx = 3 - px;
        ry = 3 - py;
      }
      else if (nr == 3) {
        rx = py;
        ry = 3 - px;
      }

      int bx = nx + rx;
      int by = ny + ry;

      if (bx < 0 || bx >= T_COLS) return false;
      if (by >= T_ROWS) return false;

      if (by >= 0 && board[by][bx]) return false;
    }
  }

  return true;
}

void loopTetris() {

  int valorX = analogRead(JOYSTICK_X);

  if (millis() - ultimoMovimentoJoystick > 140) {

    if (valorX < 1000) {

      if (podeMoverTetris(tX - 1, tY, tRot)) {
        tX--;
      }

      ultimoMovimentoJoystick = millis();
    }

    if (valorX > 3000) {

      if (podeMoverTetris(tX + 1, tY, tRot)) {
        tX++;
      }

      ultimoMovimentoJoystick = millis();
    }
  }

  if (botaoPressionado()) {

    int novaRot = (tRot + 1) % 4;

    if (podeMoverTetris(tX, tY, novaRot)) {
      tRot = novaRot;
    }
  }

  int velocidade = 500;

  if (scoreTetris >= 100) velocidade = 420;
  if (scoreTetris >= 300) velocidade = 340;
  if (scoreTetris >= 600) velocidade = 270;

  if (millis() - ultimoDropTetris >= (unsigned long)velocidade) {

    ultimoDropTetris = millis();

    if (podeMoverTetris(tX, tY + 1, tRot)) {
      tY++;
    }
    else {
      fixarPecaTetris();
      limparLinhasTetris();
      novaPecaTetris();
    }
  }

  desenharTetris();
}

void fixarPecaTetris() {

  for (int py = 0; py < 4; py++) {

    for (int px = 0; px < 4; px++) {

      if (!pecas[tPeca][py][px]) continue;

      int rx = px;
      int ry = py;

      if (tRot == 1) {
        rx = 3 - py;
        ry = px;
      }
      else if (tRot == 2) {
        rx = 3 - px;
        ry = 3 - py;
      }
      else if (tRot == 3) {
        rx = py;
        ry = 3 - px;
      }

      int bx = tX + rx;
      int by = tY + ry;

      if (by >= 0 && by < T_ROWS &&
          bx >= 0 && bx < T_COLS) {

        board[by][bx] = true;
      }
    }
  }
}

void limparLinhasTetris() {

  int linhas = 0;

  for (int y = T_ROWS - 1; y >= 0; y--) {

    bool completa = true;

    for (int x = 0; x < T_COLS; x++) {
      if (!board[y][x]) {
        completa = false;
        break;
      }
    }

    if (completa) {

      linhas++;

      for (int yy = y; yy > 0; yy--) {

        for (int x = 0; x < T_COLS; x++) {
          board[yy][x] = board[yy - 1][x];
        }
      }

      for (int x = 0; x < T_COLS; x++) {
        board[0][x] = false;
      }

      y++;
    }
  }

  if (linhas > 0) {

    if (linhas == 1) scoreTetris += 10;
    if (linhas == 2) scoreTetris += 30;
    if (linhas == 3) scoreTetris += 60;
    if (linhas >= 4) scoreTetris += 100;

    somPonto();

    if (scoreTetris > recordTetris) {
      recordTetris = scoreTetris;
      memoria.putInt("tetris", recordTetris);
    }
  }
}

void desenharTetris() {

  display.clearDisplay();

  display.drawRect(0, 0, T_COLS * TAM_BLOCO + 1,
                   T_ROWS * TAM_BLOCO, SSD1306_WHITE);

  for (int y = 0; y < T_ROWS; y++) {

    for (int x = 0; x < T_COLS; x++) {

      if (board[y][x]) {

        display.fillRect(
          1 + x * TAM_BLOCO,
          y * TAM_BLOCO,
          TAM_BLOCO - 1,
          TAM_BLOCO - 1,
          SSD1306_WHITE
        );
      }
    }
  }

  for (int py = 0; py < 4; py++) {

    for (int px = 0; px < 4; px++) {

      if (!pecas[tPeca][py][px]) continue;

      int rx = px;
      int ry = py;

      if (tRot == 1) {
        rx = 3 - py;
        ry = px;
      }
      else if (tRot == 2) {
        rx = 3 - px;
        ry = 3 - py;
      }
      else if (tRot == 3) {
        rx = py;
        ry = 3 - px;
      }

      int bx = tX + rx;
      int by = tY + ry;

      if (by >= 0 && by < T_ROWS &&
          bx >= 0 && bx < T_COLS) {

        display.fillRect(
          1 + bx * TAM_BLOCO,
          by * TAM_BLOCO,
          TAM_BLOCO - 1,
          TAM_BLOCO - 1,
          SSD1306_WHITE
        );
      }
    }
  }

  display.setTextSize(1);

  display.setCursor(45, 5);
  display.print("TETRIS");

  display.setCursor(45, 20);
  display.print("PTS:");

  display.setCursor(45, 29);
  display.print(scoreTetris);

  display.setCursor(45, 42);
  display.print("REC:");

  display.setCursor(45, 51);
  display.print(recordTetris);

  display.display();
}

void gameOverTetris() {

  if (scoreTetris > recordTetris) {
    recordTetris = scoreTetris;
    memoria.putInt("tetris", recordTetris);
  }

  mostrarGameOver("TETRIS", scoreTetris);
}

// ============================================================
// GAME OVER GERAL
// ============================================================

void mostrarGameOver(const char* nomeJogo, int pontuacao) {

  estadoAtual = GAME_OVER;
  entradaEstado = millis();

  display.clearDisplay();

  display.drawRect(4, 4, 120, 56, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(12, 10);
  display.print("GAME");

  display.setCursor(12, 27);
  display.print("OVER");

  display.setTextSize(1);

  display.setCursor(70, 12);
  display.print(nomeJogo);

  display.setCursor(70, 27);
  display.print("PTS:");
  display.print(pontuacao);

  display.setCursor(18, 47);
  display.print("APERTE O BOTAO");

  display.display();

  tone(BUZZER, 250, 180);
}

void voltarAoMenu() {

  estadoAtual = TELA_INICIAL;

  vidas = 3;
  atualizarLEDs();

  telaInicial();
}

// ============================================================
// BOTAO
// ============================================================

bool botaoPressionado() {

  if (digitalRead(BOTAO) == LOW) {

    if (millis() - ultimoBotao > 220) {

      ultimoBotao = millis();

      return true;
    }
  }

  return false;
}

// ============================================================
// SONS
// ============================================================

void somInicio() {

  tone(BUZZER, 523, 100);
  delay(120);

  tone(BUZZER, 659, 100);
  delay(120);

  tone(BUZZER, 784, 150);
  delay(180);

  noTone(BUZZER);
}

void somTiro() {
  tone(BUZZER, 900, 40);
}

void somInimigo() {

  tone(BUZZER, 700, 50);
  delay(55);

  tone(BUZZER, 450, 70);
  delay(75);

  noTone(BUZZER);
}

void somPerdeVida() {

  tone(BUZZER, 350, 150);
  delay(170);

  tone(BUZZER, 250, 180);
  delay(200);

  noTone(BUZZER);
}

void somPonto() {
  tone(BUZZER, 1000, 45);
}