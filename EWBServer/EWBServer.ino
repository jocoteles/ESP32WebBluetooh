/**
 * @file EWBServer.ino
 * @brief Aplicação de exemplo para a biblioteca ESP32WebBluetooth (EWBServer).
 *        Demonstra o controle de variáveis via JSON e o streaming de dados binários
 *        com a nova arquitetura de callback.
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
// Estas constantes devem ser obtidas do array configurableVariables
int SAMPLES_PER_CHUNK = 20;
int SAMPLE_INTERVAL_US = 250;

const int ANALOG_PIN_1 = 32;
const int ANALOG_PIN_2 = 33;
// ... (outros pinos analógicos) ...

// Estrutura do pacote de dados (deve ser idêntica à do cliente JS: 16 bytes)
#pragma pack(push, 1)
struct SensorDataPacket {
  uint16_t reading1, reading2, reading3, reading4, reading5, reading6;
  uint32_t time_ms;
};
#pragma pack(pop)

const int PACKET_SIZE_BYTES = sizeof(SensorDataPacket);
// O buffer agora será alocado dinamicamente no setup/teoricamente, mas para Arduino usaremos um array fixo grande
SensorDataPacket sensorDataBuffer[100]; 
int currentBufferIndex = 0;

// --- Variáveis de Estado da Aplicação ---
bool isAppStreaming = false;
uint32_t streamStartTimeMs = 0;

// Protótipos
void onVariableChanged(const char* varName);


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

// --- NOVO: Função de Callback para Alteração de Variável ---
void onVariableChanged(const char* varName) {
    // Reage imediatamente a mudanças de variáveis críticas
    if (strcmp(varName, "led_intensity") == 0) {
        // Exemplo: Atualiza o LED (simulado aqui no Serial)
        Serial.printf("LED intensity set to: %d\n", configurableVariables[0].intValue);
        // analogWrite(LED_PIN, configurableVariables[0].intValue);
    } else if (strcmp(varName, "update_interval") == 0) {
        SAMPLE_INTERVAL_US = configurableVariables[1].intValue;
        Serial.printf("Sample Interval (us) updated to: %d\n", SAMPLE_INTERVAL_US);
    }
}


// --- Funções Setup e Loop ---

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- EWB Application Setup ---");

  // Configura o LED (simulação)
  // pinMode(LED_PIN, OUTPUT);
  // analogWrite(LED_PIN, configurableVariables[0].intValue);

  // Inicializa o servidor Web Bluetooth
  ewbServer.begin(DEVICE_NAME, configurableVariables, numConfigurableVariables);
  
  // Registra os callbacks da aplicação
  ewbServer.setStreamCallbacks(application_onStreamStart, application_onStreamStop);
  // NOVO: Registra o callback para variáveis
  ewbServer.setOnVariableChangeCallback(onVariableChanged);
  
  Serial.println("--- Setup Complete ---");
}

void loop() {
  if (isAppStreaming && ewbServer.isClientConnected()) {
    // 1. Lê os sensores (simulado)
    uint16_t val1 = analogRead(ANALOG_PIN_1);
    uint16_t val2 = analogRead(ANALOG_PIN_2);
    // ... (outras leituras)
    uint16_t val6 = analogRead(39);

    // 2. Obtém o timestamp
    uint32_t currentTimeMs = millis() - streamStartTimeMs;

    // 3. Preenche o buffer
    sensorDataBuffer[currentBufferIndex] = {val1, val2, val3, val4, val5, val6, currentTimeMs};
    currentBufferIndex++;

    // 4. Se o buffer estiver cheio, envia os dados
    if (currentBufferIndex >= configurableVariables[0].intValue) {
      size_t chunkSize = currentBufferIndex * PACKET_SIZE_BYTES;
      ewbServer.sendStreamData((uint8_t*)sensorDataBuffer, chunkSize);
      currentBufferIndex = 0;
    }

    // 5. Aguarda o intervalo de amostragem (usa a variável atualizada pelo callback)
    delayMicroseconds(SAMPLE_INTERVAL_US);

  } else {
    // Se não estiver em streaming, pode fazer outras tarefas
    delay(50);
  }
}