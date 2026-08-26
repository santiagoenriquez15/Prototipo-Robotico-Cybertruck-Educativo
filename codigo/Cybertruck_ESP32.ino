#include <BluetoothSerial.h>
#include <ESP32Servo.h>

BluetoothSerial SerialBT;
Servo miServo;

// Pines
#define IN1 25
#define IN2 26
#define ENA 27
#define SERVO_PIN 13
#define LED_PIN 12

bool estabaConectado = false;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Cybertruck");
  Serial.println("Esperando conexión Bluetooth...");


  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  miServo.attach(SERVO_PIN);

  detener();
  miServo.write(90); // posición centro
}

void loop() {
  bool conectado = SerialBT.hasClient();

  // 🔹 Detección de conexión
  if (conectado && !estabaConectado) {
    Serial.println("Bluetooth conectado ✅");
    detener();
    miServo.write(90); // Centrar servo
    digitalWrite(LED_PIN, HIGH);

    // 🔸 Limpiar buffer de entrada Bluetooth
    while (SerialBT.available()) SerialBT.read();
  }

  // 🔹 Detección de desconexión
  if (!conectado && estabaConectado) {
    Serial.println("Bluetooth desconectado ❌");
    detener();
    miServo.write(90); // Centrar servo
    digitalWrite(LED_PIN, LOW);

    // 🔸 También limpiar cualquier dato que haya quedado
    while (SerialBT.available()) SerialBT.read();
  }

  estabaConectado = conectado;

  if (conectado && SerialBT.available()) {
    char c = SerialBT.read();
    Serial.print("Comando recibido: ");
    Serial.println(c);
    procesarComando(c);
  }

  delay(10);
}

void procesarComando(char cmd) {
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
  Serial.println("Avanzando");
}

void retroceder() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 255);
  Serial.println("Retrocediendo");
}

void detener() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  Serial.println("Detenido");
}
