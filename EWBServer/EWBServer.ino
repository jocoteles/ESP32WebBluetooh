/**
 * @file EWBServer.ino
 * @brief Aplicação de exemplo para a biblioteca ESP32WebBluetooth (EWBServer).
 *        Demonstra o controle de variáveis via JSON e o streaming de dados binários
 *        de 6 entradas analógicas, tudo via Web Bluetooth.
 */

#include "EWBServer.h"

// --- Configuração do Dispositivo ---
const char* DEVICE_NAME = "ESP32_EWB_Control";

// --- Instância do Servidor ---
EWBServer ewbServer;

// --- Configuração das Variáveis (Comunicação JSON) ---
VariableConfig configurableVariables[] = {
  // Name             Type         Int Val  Float Val  String Val       Min     Max     Limits?
  {"led_intensity",   TYPE_INT,    128,     0.0f,      "",              0.0,    255.0,  true},
  {"update_interval", TYPE_INT,    500,     0.0f,      "",              50.0,   5000.0, true},
  {"motor_enable",    TYPE_INT,    0,       0.0f,      "",              0.0,    1.0,    true},
  {"device_label",    TYPE_STRING, 0,       0.0f,      "ESP32-01",      0.0,    0.0,    false}
};
const int numConfigurableVariables = sizeof(configurableVariables) / sizeof(configurableVariables[0]);


// --- Configuração do Streaming ---
const int SAMPLES_PER_CHUNK = 20;       // Quantas leituras agrupar antes de enviar
const int SAMPLE_INTERVAL_US = 250;     // Intervalo entre leituras em microssegundos

const int ANALOG_PIN_1 = 32;
const int ANALOG_PIN_2 = 33;
const int ANALOG_PIN_3 = 34;
const int ANALOG_PIN_4 = 35;
const int ANALOG_PIN_5 = 36;
const int ANALOG_PIN_6 = 39;

// Estrutura do pacote de dados (deve ser idêntica à do cliente JS)
#pragma pack(push, 1)
struct SensorDataPacket {
  uint16_t reading1, reading2, reading3, reading4, reading5, reading6;
  uint32_t time_ms;
};
#pragma pack(pop)

const int PACKET_SIZE_BYTES = sizeof(SensorDataPacket);
const int CHUNK_BUFFER_SIZE_BYTES = SAMPLES_PER_CHUNK * PACKET_SIZE_BYTES;

SensorDataPacket sensorDataBuffer[SAMPLES_PER_CHUNK];
int currentBufferIndex = 0;

// --- Variáveis de Estado da Aplicação ---
bool isAppStreaming = false;
uint32_t streamStartTimeMs = 0;


// --- Funções de Callback para o Streaming ---

void application_onStreamStart() {
  Serial.println("Application Callback: START STREAM");
  currentBufferIndex = 0;
  streamStartTimeMs = millis();
  isAppStreaming = true;
}

void application_onStreamStop() {
  Serial.println("Application Callback: STOP STREAM");
  isAppStreaming = false;
}


// --- Funções Setup e Loop ---

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- EWB Application Setup ---");

  // Configura os pinos analógicos
  pinMode(ANALOG_PIN_1, INPUT);
  pinMode(ANALOG_PIN_2, INPUT);
  pinMode(ANALOG_PIN_3, INPUT);
  pinMode(ANALOG_PIN_4, INPUT);
  pinMode(ANALOG_PIN_5, INPUT);
  pinMode(ANALOG_PIN_6, INPUT);

  // Inicializa o servidor Web Bluetooth
  ewbServer.begin(DEVICE_NAME, configurableVariables, numConfigurableVariables);
  
  // Registra os callbacks da aplicação
  ewbServer.setStreamCallbacks(application_onStreamStart, application_onStreamStop);
  
  Serial.println("--- Setup Complete ---");
}

void loop() {
  if (isAppStreaming && ewbServer.isClientConnected()) {
    // 1. Lê os sensores
    uint16_t val1 = analogRead(ANALOG_PIN_1);
    uint16_t val2 = analogRead(ANALOG_PIN_2);
    uint16_t val3 = analogRead(ANALOG_PIN_3);
    uint16_t val4 = analogRead(ANALOG_PIN_4);
    uint16_t val5 = analogRead(ANALOG_PIN_5);
    uint16_t val6 = analogRead(ANALOG_PIN_6);

    // 2. Obtém o timestamp
    uint32_t currentTimeMs = millis() - streamStartTimeMs;

    // 3. Preenche o buffer
    sensorDataBuffer[currentBufferIndex] = {val1, val2, val3, val4, val5, val6, currentTimeMs};
    currentBufferIndex++;

    // 4. Se o buffer estiver cheio, envia os dados
    if (currentBufferIndex >= SAMPLES_PER_CHUNK) {
      ewbServer.sendStreamData((uint8_t*)sensorDataBuffer, CHUNK_BUFFER_SIZE_BYTES);
      currentBufferIndex = 0;
    }

    // 5. Aguarda o intervalo de amostragem
    delayMicroseconds(SAMPLE_INTERVAL_US);

  } else {
    // Se não estiver em streaming, pode fazer outras tarefas
    delay(50);
  }
}