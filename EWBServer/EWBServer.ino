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
// O nome "led_intensity" é mantido como exemplo, mas pode ser qualquer variável.
// Adicionamos "samples_per_chunk" para controle dinâmico do buffer de envio.
VariableConfig configurableVariables[] = {
  // Name                 Type         Int Val  Float Val  String Val       Min     Max     Limits?
  {"led_intensity",       TYPE_INT,    128,     0.0f,      "",              0.0,    255.0,  true},
  {"update_interval",     TYPE_INT,    500,     0.0f,      "",              50.0,   5000.0, true},
  {"motor_enable",        TYPE_INT,    0,       0.0f,      "",              0.0,    1.0,    true},
  {"device_label",        TYPE_STRING, 0,       0.0f,      "ESP32-01",      0.0,    0.0,    false},
  {"samples_per_chunk",   TYPE_INT,    20,      0.0f,      "",              1.0,    100.0,  true}
};
const int numConfigurableVariables = sizeof(configurableVariables) / sizeof(configurableVariables[0]);


// --- Configuração do Streaming ---
// CORREÇÃO: Pinos analógicos restantes definidos.
const int ANALOG_PIN_1 = 32;
const int ANALOG_PIN_2 = 33;
const int ANALOG_PIN_3 = 34;
const int ANALOG_PIN_4 = 35;
const int ANALOG_PIN_5 = 36; // Em alguns ESP32, este é o pino ADC1_CH0 (GPIO36)
const int ANALOG_PIN_6 = 39; // Em alguns ESP32, este é o pino ADC1_CH3 (GPIO39)

// Estrutura do pacote de dados (deve ser idêntica à do cliente JS: 16 bytes)
#pragma pack(push, 1)
struct SensorDataPacket {
  uint16_t reading1, reading2, reading3, reading4, reading5, reading6;
  uint32_t time_ms;
};
#pragma pack(pop)

const int PACKET_SIZE_BYTES = sizeof(SensorDataPacket);
// O buffer agora tem um tamanho máximo fixo, mas o gatilho de envio é dinâmico.
SensorDataPacket sensorDataBuffer[100]; 
int currentBufferIndex = 0;

// --- Variáveis de Estado da Aplicação ---
bool isAppStreaming = false;
uint32_t streamStartTimeMs = 0;
// CORREÇÃO: Variável para armazenar o intervalo de amostragem atual
// Isso permite que o callback a atualize para ser usada no loop.
volatile int currentSampleIntervalUs = 500;


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

// --- Função de Callback para Alteração de Variável ---
void onVariableChanged(const char* varName) {
    // Reage imediatamente a mudanças de variáveis críticas
    if (strcmp(varName, "led_intensity") == 0) {
        // Exemplo: Atualiza o LED (simulado aqui no Serial)
        // O valor é lido diretamente do array de configuração.
        Serial.printf("Callback: LED intensity set to %d\n", configurableVariables[0].intValue);
        // Exemplo de uso real: analogWrite(LED_PIN, configurableVariables[0].intValue);
    } 
    else if (strcmp(varName, "update_interval") == 0) {
        // CORREÇÃO: Atualiza a variável de estado usada pelo loop
        currentSampleIntervalUs = configurableVariables[1].intValue;
        Serial.printf("Callback: Sample Interval (us) updated to %d\n", currentSampleIntervalUs);
    }
}


// --- Funções Setup e Loop ---

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- EWB Application Setup ---");

  // Configura os pinos como entrada
  pinMode(ANALOG_PIN_1, INPUT);
  pinMode(ANALOG_PIN_2, INPUT);
  pinMode(ANALOG_PIN_3, INPUT);
  pinMode(ANALOG_PIN_4, INPUT);
  pinMode(ANALOG_PIN_5, INPUT);
  pinMode(ANALOG_PIN_6, INPUT);

  // CORREÇÃO: Define o estado inicial da variável de intervalo de amostragem
  currentSampleIntervalUs = configurableVariables[1].intValue;

  // Inicializa o servidor Web Bluetooth
  ewbServer.begin(DEVICE_NAME, configurableVariables, numConfigurableVariables);
  
  // Registra os callbacks da aplicação
  ewbServer.setStreamCallbacks(application_onStreamStart, application_onStreamStop);
  ewbServer.setOnVariableChangeCallback(onVariableChanged);
  
  Serial.println("--- Setup Complete ---");
}

void loop() {
  if (isAppStreaming && ewbServer.isClientConnected()) {
    // 1. CORREÇÃO: Leituras de todos os 6 sensores
    uint16_t val1 = analogRead(ANALOG_PIN_1);
    uint16_t val2 = analogRead(ANALOG_PIN_2);
    uint16_t val3 = analogRead(ANALOG_PIN_3);
    uint16_t val4 = analogRead(ANALOG_PIN_4);
    uint16_t val5 = analogRead(ANALOG_PIN_5);
    uint16_t val6 = analogRead(ANALOG_PIN_6);

    // 2. Obtém o timestamp
    uint32_t currentTimeMs = millis() - streamStartTimeMs;

    // 3. CORREÇÃO: Preenche o buffer com todos os dados
    sensorDataBuffer[currentBufferIndex] = {val1, val2, val3, val4, val5, val6, currentTimeMs};
    currentBufferIndex++;

    // 4. Se o buffer estiver cheio, envia os dados
    // CORREÇÃO: Utiliza o valor do array de configuração para o tamanho do chunk, que pode ser alterado dinamicamente.
    // O índice 4 corresponde a "samples_per_chunk" no array.
    if (currentBufferIndex >= configurableVariables[4].intValue) {
      size_t chunkSize = currentBufferIndex * PACKET_SIZE_BYTES;
      ewbServer.sendStreamData((uint8_t*)sensorDataBuffer, chunkSize);
      currentBufferIndex = 0;
    }

    // 5. Aguarda o intervalo de amostragem
    // CORREÇÃO: Usa a variável que é atualizada pelo callback
    delayMicroseconds(currentSampleIntervalUs);

  } else {
    // Se não estiver em streaming, pode fazer outras tarefas
    delay(50);
  }
}