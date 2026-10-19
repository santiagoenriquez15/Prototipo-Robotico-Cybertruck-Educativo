#include <BluetoothSerial.h>
#include <ESP32Servo.h>

BluetoothSerial SerialBT;
Servo miServo;

// ---------- Pines ----------
#define IN1 25
#define IN2 26
#define ENA 27
#define SERVO_PIN 13
#define LED_PIN 12
#define BAT_PIN 32          // ADC1 -> divisor de voltaje

// ---------- Divisor de voltaje (5.1 kohm arriba, 2 kohm abajo) ----------
const float R1 = 5100.0;
const float R2 = 2000.0;
const float FACTOR_CAL = 1.00;   // Ajustar comparando con el multimetro

// ---------- Opciones de medicion ----------
const unsigned long INTERVALO_ENVIO = 2000;  // ms entre envios a la app
const unsigned long TIEMPO_REPOSO   = 2500;  // esperar antes de medir tras movimiento
const float ALFA = 0.02;                     // suavizado de lectura

float vBateria = 0;
float vPinUlt = 0;
bool lecturaValida = false;
bool filtroReiniciar = true;
bool motorActivo = false;
unsigned long ultimaActividad = 0;
unsigned long ultimoEnvio = 0;
unsigned long ultimoDebug = 0;
bool estabaConectado = false;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Cybertruck");
  Serial.println("Esperando conexion Bluetooth...");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BAT_PIN, INPUT);
  analogSetPinAttenuation(BAT_PIN, ADC_11db);

  miServo.attach(SERVO_PIN);
  detener();
  miServo.write(90);
}

void loop() {
  bool conectado = SerialBT.hasClient();

  if (conectado && !estabaConectado) {
    Serial.println("Bluetooth conectado");
    detener();
    miServo.write(90);
    digitalWrite(LED_PIN, HIGH);
    while (SerialBT.available()) SerialBT.read();
    ultimoEnvio = millis() - INTERVALO_ENVIO;
  }

  if (!conectado && estabaConectado) {
    Serial.println("Bluetooth desconectado");
    detener();
    miServo.write(90);
    digitalWrite(LED_PIN, LOW);
    while (SerialBT.available()) SerialBT.read();
  }

  estabaConectado = conectado;

  if (conectado && SerialBT.available()) {
    char c = SerialBT.read();
    Serial.print("Comando recibido: ");
    Serial.println(c);
    procesarComando(c);
  }

  actualizarBateria();

  if (conectado && lecturaValida && millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    SerialBT.print("BAT:");
    SerialBT.print(porcentajeBateria(vBateria));
    SerialBT.print('\n');
  }

  if (lecturaValida && millis() - ultimoDebug >= 2000) {
    ultimoDebug = millis();
    bool reposo = !motorActivo && (millis() - ultimaActividad >= TIEMPO_REPOSO);
    Serial.printf("Pin GPIO32: %.3f V | Bateria: %.2f V | %.2f V/celda | %d %% | reposo:%s\n",
                  vPinUlt, vBateria, vBateria / 2.0, porcentajeBateria(vBateria),
                  reposo ? "SI" : "NO");
  }

  delay(10);
}

// ================= BATERIA =================

void actualizarBateria() {
  // Durante el movimiento y unos segundos despues, conservar la ultima lectura estable.
  if (motorActivo || millis() - ultimaActividad < TIEMPO_REPOSO) {
    filtroReiniciar = true;
    return;
  }

  long suma = 0;
  for (int i = 0; i < 16; i++) {
    suma += analogReadMilliVolts(BAT_PIN);
    delayMicroseconds(200);
  }

  float vPin = (suma / 16.0) / 1000.0;
  vPinUlt = vPin;
  float v = vPin * ((R1 + R2) / R2) * FACTOR_CAL;

  if (!lecturaValida || filtroReiniciar) {
    vBateria = v;
    lecturaValida = true;
    filtroReiniciar = false;
  } else {
    vBateria = vBateria * (1.0 - ALFA) + v * ALFA;
  }
}

int porcentajeBateria(float v) {
  float vc = v / 2.0;  // voltaje aproximado por celda para un paquete 2S
  if (vc >= 4.20) return 100;
  if (vc >= 4.10) return (int)(90 + (vc - 4.10) / 0.10 * 10);
  if (vc >= 4.00) return (int)(80 + (vc - 4.00) / 0.10 * 10);
  if (vc >= 3.90) return (int)(60 + (vc - 3.90) / 0.10 * 20);
  if (vc >= 3.80) return (int)(40 + (vc - 3.80) / 0.10 * 20);
  if (vc >= 3.70) return (int)(20 + (vc - 3.70) / 0.10 * 20);
  if (vc >= 3.50) return (int)(5 + (vc - 3.50) / 0.20 * 15);
  if (vc >= 3.30) return (int)((vc - 3.30) / 0.20 * 5);
  return 0;
}

// ================= CONTROL DEL CARRO =================

void procesarComando(char cmd) {
  if (cmd == 'F' || cmd == 'B' || cmd == 'S' || cmd == 'I' || cmd == 'R' || cmd == 'C') {
    ultimaActividad = millis();
  }

  switch (cmd) {
    case 'F': avanzar(); break;
    case 'B': retroceder(); break;
    case 'S': detener(); break;
    case 'I': miServo.write(60); break;
    case 'R': miServo.write(120); break;
    case 'C': miServo.write(90); break;
    default: break;
  }
}

void avanzar() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 255);
  motorActivo = true;
  Serial.println("Avanzando");
}

void retroceder() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 255);
  motorActivo = true;
  Serial.println("Retrocediendo");
}

void detener() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  motorActivo = false;
  Serial.println("Detenido");
}
