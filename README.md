🚀 Sistema IoT — Monitoramento de Cápsula Espacial
Sistema embarcado desenvolvido em Arduino para monitoramento em tempo real das condições internas de uma cápsula espacial simulada. Coleta dados de temperatura, luminosidade e vibração, classifica automaticamente o nível de risco da missão e exibe as informações em um display LCD 16x2 com alertas visuais e sonoros.

⚙️ Tecnologias

Arduino UNO R3
C++ (Arduino IDE)
Biblioteca LiquidCrystal
Simulado no Tinkercad


🔧 Sensores e Componentes
ComponentePinoFunçãoTMP36A0Temperatura interna da cápsulaLDRA1Luminosidade ambientalTilt SensorD2Detecção de vibração e impactoLCD 16x2D3~D7, D12Exibição dos dados em tempo realLED VerdeD8Status normalLED AmareloD9Status atençãoLED VermelhoD10Status emergênciaBuzzerD11Alerta sonoro (1500Hz)

🚨 Lógica de Alertas
cppbool emergencia = temperatura >= 45 || vibracao == HIGH || luminosidade > 900;
bool atencao    = temperatura >= 35 && temperatura < 45;
StatusCondição✅ NormalTemp < 35°C, sem vibração, luminosidade ≤ 900⚠️ Atenção35°C ≤ Temp < 45°C🚨 EmergênciaTemp ≥ 45°C ou vibração detectada ou luminosidade > 900

📡 Arquitetura
TMP36 ──► A0 ──┐
LDR   ──► A1 ──┤──► Arduino UNO ──► LCD / LEDs / Buzzer
Tilt  ──► D2 ──┘         │
                          ▼
                   Serial Monitor
                   (telemetria em tempo real)
