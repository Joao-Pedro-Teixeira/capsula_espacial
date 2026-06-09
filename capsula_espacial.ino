/*
 * =====================================================
 *   SISTEMA IoT - MONITORAMENTO DE CÁPSULA ESPACIAL
 *   Global Solution FIAP 2026
 * =====================================================
 *
 * MAPEAMENTO DE PINOS:
 *   Sensores:
 *     TMP36  (temperatura)  → A0
 *     LDR    (luminosidade) → A1
 *     Tilt   (vibração)     → D2
 *
 *   Atuadores:
 *     LED Verde             → D8
 *     LED Amarelo           → D9
 *     LED Vermelho          → D10
 *     Buzzer                → D11
 *
 *   Display LCD 16x2:
 *     RS → D7
 *     E  → D6
 *     D4 → D5
 *     D5 → D4
 *     D6 → D3
 *     D7 → D12
 *
 * LÓGICA DE ALERTAS:
 *   Temperatura < 35°C          → STATUS OK    (LED Verde)
 *   35°C ≤ Temperatura < 45°C  → ATENÇÃO       (LED Amarelo)
 *   Temperatura ≥ 45°C          → ALERTA RISCO  (LED Vermelho + Buzzer)
 *   Vibração detectada           → ALERTA RISCO  (LED Vermelho + Buzzer)
 * =====================================================
 */

// Inclui a biblioteca do LCD
#include <LiquidCrystal.h>

// ── Inicialização do LCD ────────────────────────────
// Ordem dos parâmetros: LiquidCrystal(RS, E, D4, D5, D6, D7)
// Atenção: D4 do LCD vai no pino 5 do Arduino, D5 no pino 4, etc.
LiquidCrystal lcd(7, 6, 5, 4, 3, 12);

// ── Pinos dos Sensores ──────────────────────────────
const int PIN_TMP36 = A0;   // Sensor de temperatura TMP36
const int PIN_LDR   = A1;   // Sensor de luminosidade LDR
const int PIN_TILT  = 2;    // Sensor de vibração (Tilt)

// ── Pinos dos Atuadores ─────────────────────────────
const int PIN_LED_VERDE    = 8;
const int PIN_LED_AMARELO  = 9;
const int PIN_LED_VERMELHO = 10;
const int PIN_BUZZER       = 11;

// ── Limites de Temperatura (°C) ─────────────────────
const float TEMP_ATENCAO    = 35.0;  // Acima disso → atenção
const float TEMP_EMERGENCIA = 45.0;  // Acima disso → emergência

// ── Variáveis Globais ───────────────────────────────
float temperatura = 0.0;
int   luminosidade = 0;
bool  vibracaoDetectada = false;

// Controle de atualização do display
unsigned long ultimaAtualizacao = 0;
const unsigned long INTERVALO_DISPLAY = 1000; // Atualiza a cada 1 segundo

// Controle da tela atual (alterna entre temperatura e luminosidade)
bool mostrarTemp = true;
unsigned long ultimaTroca = 0;
const unsigned long INTERVALO_TROCA = 3000; // Troca de tela a cada 3 segundos


// ═══════════════════════════════════════════════════
//   SETUP - Executado uma única vez ao ligar
// ═══════════════════════════════════════════════════
void setup() {

  // Inicializa o monitor serial para debug (opcional)
  Serial.begin(9600);
  Serial.println("=== SISTEMA IoT - CAPSULA ESPACIAL ===");

  // Configura os pinos dos LEDs como saída
  pinMode(PIN_LED_VERDE,    OUTPUT);
  pinMode(PIN_LED_AMARELO,  OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);

  // Configura o pino do buzzer como saída
  pinMode(PIN_BUZZER, OUTPUT);

  // Configura o pino do sensor Tilt como entrada com pull-up interno
  // O pull-up garante leitura estável quando o sensor está em repouso
  pinMode(PIN_TILT, INPUT_PULLUP);

  // Inicializa o LCD com 16 colunas e 2 linhas
  lcd.begin(16, 2);

  // Exibe mensagem de boas-vindas por 2 segundos
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CAPSULA ESPACIAL");
  lcd.setCursor(0, 1);
  lcd.print(" INICIALIZANDO..");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" MISSION CONTROL");
  lcd.setCursor(0, 1);
  lcd.print("   ONLINE  v1.0 ");
  delay(2000);

  lcd.clear();
}


// ═══════════════════════════════════════════════════
//   LOOP - Executado repetidamente
// ═══════════════════════════════════════════════════
void loop() {

  // 1. Lê todos os sensores
  lerSensores();

  // 2. Atualiza o display e atuadores a cada INTERVALO_DISPLAY ms
  unsigned long agora = millis();
  if (agora - ultimaAtualizacao >= INTERVALO_DISPLAY) {
    ultimaAtualizacao = agora;

    // 3. Determina o status atual e aciona LEDs/Buzzer
    atualizarStatus();

    // 4. Atualiza o display LCD
    atualizarDisplay();

    // 5. Envia dados para o monitor serial
    enviarTelemetria();
  }
}


// ═══════════════════════════════════════════════════
//   FUNÇÕES AUXILIARES
// ═══════════════════════════════════════════════════

/*
 * lerSensores()
 * Realiza a leitura de todos os sensores e
 * armazena os valores nas variáveis globais.
 */
void lerSensores() {

  // ── Temperatura (TMP36) ────────────────────────
  // O TMP36 retorna uma tensão proporcional à temperatura.
  // Fórmula: tensão = leitura * (5.0 / 1023.0)
  //          temperatura = (tensão - 0.5) * 100
  int leituraBruta = analogRead(PIN_TMP36);
  float tensao = leituraBruta * (5.0 / 1023.0);
  temperatura = (tensao - 0.5) * 100.0;

  // ── Luminosidade (LDR) ─────────────────────────
  // Leitura analógica de 0 (escuro) a 1023 (muito claro)
  // Convertemos para percentual de 0% a 100%
  int leituraLDR = analogRead(PIN_LDR);
  luminosidade = map(leituraLDR, 0, 1023, 0, 100);

  // ── Vibração (Tilt Sensor) ─────────────────────
  // Com INPUT_PULLUP: LOW = vibração detectada, HIGH = estável
  // (o tilt sensor fecha o circuito quando inclinado)
  vibracaoDetectada = (digitalRead(PIN_TILT) == LOW);
}


/*
 * atualizarStatus()
 * Analisa os valores lidos e aciona os LEDs e o buzzer
 * de acordo com a lógica de alertas do sistema.
 */
void atualizarStatus() {

  // Desliga todos os LEDs e o buzzer antes de definir o estado atual
  digitalWrite(PIN_LED_VERDE,    LOW);
  digitalWrite(PIN_LED_AMARELO,  LOW);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  noTone(PIN_BUZZER);

  // ── EMERGÊNCIA: temperatura crítica ou vibração ──
  if (temperatura >= TEMP_EMERGENCIA || vibracaoDetectada) {
    digitalWrite(PIN_LED_VERMELHO, HIGH);
    // Buzzer emite tom de alerta contínuo (880 Hz)
    tone(PIN_BUZZER, 880);
  }
  // ── ATENÇÃO: temperatura elevada ────────────────
  else if (temperatura >= TEMP_ATENCAO) {
    digitalWrite(PIN_LED_AMARELO, HIGH);
  }
  // ── NORMAL: tudo dentro dos parâmetros ──────────
  else {
    digitalWrite(PIN_LED_VERDE, HIGH);
  }
}


/*
 * atualizarDisplay()
 * Escreve as informações no LCD 16x2.
 * Alterna entre duas telas a cada INTERVALO_TROCA ms:
 *   Tela 1: Temperatura + Status
 *   Tela 2: Luminosidade + Vibração
 */
void atualizarDisplay() {

  unsigned long agora = millis();

  // Verifica se é hora de trocar a tela
  if (agora - ultimaTroca >= INTERVALO_TROCA) {
    ultimaTroca = agora;
    mostrarTemp = !mostrarTemp;
    lcd.clear();
  }

  // ── EMERGÊNCIA ───────────────────────────────────
  if (temperatura >= TEMP_EMERGENCIA || vibracaoDetectada) {
    lcd.setCursor(0, 0);
    lcd.print("!!! ALERTA !!!  ");
    lcd.setCursor(0, 1);

    if (vibracaoDetectada) {
      lcd.print("VIBRACAO DETECTA");
    } else {
      lcd.print("TEMP CRITICA!   ");
    }
    return; // Não mostra as telas alternadas em emergência
  }

  // ── TELA 1: Temperatura e Status ─────────────────
  if (mostrarTemp) {
    lcd.setCursor(0, 0);
    lcd.print("TEMP: ");
    lcd.print(temperatura, 1); // 1 casa decimal
    lcd.print((char)223);      // Símbolo de grau °
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    if (temperatura >= TEMP_ATENCAO) {
      lcd.print("STATUS: ATENCAO ");
    } else {
      lcd.print("STATUS: OK      ");
    }
  }
  // ── TELA 2: Luminosidade e Vibração ──────────────
  else {
    lcd.setCursor(0, 0);
    lcd.print("LUZ: ");
    lcd.print(luminosidade);
    lcd.print("%          ");

    lcd.setCursor(0, 1);
    lcd.print("VIB: ");
    lcd.print(vibracaoDetectada ? "DETECTADA  " : "ESTAVEL    ");
  }
}


/*
 * enviarTelemetria()
 * Envia os dados dos sensores pelo Monitor Serial.
 * Útil para debug e para simular o envio ao Mission Control AI.
 */
void enviarTelemetria() {
  Serial.print("[TELEMETRIA] Temp: ");
  Serial.print(temperatura, 1);
  Serial.print("C | Luz: ");
  Serial.print(luminosidade);
  Serial.print("% | Vibracao: ");
  Serial.print(vibracaoDetectada ? "SIM" : "NAO");
  Serial.print(" | Status: ");

  if (temperatura >= TEMP_EMERGENCIA || vibracaoDetectada) {
    Serial.println("EMERGENCIA");
  } else if (temperatura >= TEMP_ATENCAO) {
    Serial.println("ATENCAO");
  } else {
    Serial.println("OK");
  }
}
