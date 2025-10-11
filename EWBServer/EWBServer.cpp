#include "EWBServer.h"

// UUIDs únicos para o serviço e características. Gerados em https://www.uuidgenerator.net/
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define JSON_VARS_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STREAM_DATA_CHAR_UUID  "82b934b0-a02c-4fb5-a818-a35752697d57"
#define STREAM_CONTROL_CHAR_UUID "c8a4a259-4203-48e8-b39f-5a8b71d601b0"

// --- Implementação da Classe EWBServer ---

EWBServer::EWBServer() {}

void EWBServer::begin(const char* deviceName, VariableConfig* vars, int numVars) {
  this->variables = vars;
  this->numVariables = numVars;

  Serial.println("Starting EWBServer BLE...");

  // 1. Inicializa o dispositivo BLE
  BLEDevice::init(deviceName);

  // 2. Cria o servidor BLE
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks(&this->clientConnected));

  // 3. Cria o serviço BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Cria a Característica para Variáveis (JSON)
  jsonVariablesCharacteristic = pService->createCharacteristic(
                                  JSON_VARS_CHAR_UUID,
                                  BLECharacteristic::PROPERTY_READ |
                                  BLECharacteristic::PROPERTY_WRITE
                                );
  jsonVariablesCharacteristic->setCallbacks(new JsonCharacteristicCallbacks(this));
  jsonVariablesCharacteristic->setValue("{}"); // Valor inicial

  // 5. Cria a Característica para Streaming (Notificação)
  streamDataCharacteristic = pService->createCharacteristic(
                               STREAM_DATA_CHAR_UUID,
                               BLECharacteristic::PROPERTY_NOTIFY
                             );
  streamDataCharacteristic->addDescriptor(new BLE2902()); // Necessário para notificações

  // 6. Cria a Característica para Controle do Streaming (Escrita)
  streamControlCharacteristic = pService->createCharacteristic(
                                  STREAM_CONTROL_CHAR_UUID,
                                  BLECharacteristic::PROPERTY_WRITE
                                );
  // Os callbacks serão atribuídos em setStreamCallbacks

  // 7. Inicia o serviço e o advertising
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("EWBServer started. Waiting for a client connection...");
}

void EWBServer::setStreamCallbacks(StreamCallback onStart, StreamCallback onStop) {
  this->onStreamStartCallback = onStart;
  this->onStreamStopCallback = onStop;
  // Agora que temos os callbacks, podemos atribuí-los à característica
  streamControlCharacteristic->setCallbacks(new StreamControlCallbacks(onStart, onStop));
}

// NOVO: Implementação da função para definir o callback de mudança de variável
void EWBServer::setOnVariableChangeCallback(VariableChangeCallback callback) {
    this->onVariableChange = callback;
}

void EWBServer::sendStreamData(const uint8_t* data, size_t length) {
  if (clientConnected) {
    streamDataCharacteristic->setValue((uint8_t*)data, length);
    streamDataCharacteristic->notify();
  }
}

bool EWBServer::isClientConnected() {
    return this->clientConnected;
}

// --- Funções Privadas para manipulação de JSON ---

void EWBServer::handleJsonGet(JsonDocument& doc) {
  // O cliente pediu para ler as variáveis, preparamos a resposta
  generateJsonState(doc);
}

void EWBServer::handleJsonSet(JsonDocument& doc) {
  JsonObject setObject = doc["set"];
  if (!setObject) return;

  // Itera sobre as variáveis configuradas para ver se alguma corresponde
  for (int i = 0; i < numVariables; i++) {
    const char* varName = variables[i].name;
    if (setObject.containsKey(varName)) {
      JsonVariant val = setObject[varName];
      bool changed = false; // Flag para rastrear a mudança

      // Atualiza o valor da variável, respeitando o tipo e os limites
      switch (variables[i].type) {
        case TYPE_INT:
          {
            int newValue = val.as<int>();
            if (variables[i].useLimits) {
              newValue = constrain(newValue, variables[i].min, variables[i].max);
            }
            if (variables[i].intValue != newValue) {
              variables[i].intValue = newValue;
              changed = true;
            }
          }
          break;
        case TYPE_FLOAT:
          // Lógica de float omitida por brevidade e foco no TYPE_INT do exemplo
          break;
        case TYPE_STRING:
          {
            const char* newString = val.as<const char*>();
            if (strncmp(variables[i].stringValue, newString, sizeof(variables[i].stringValue)) != 0) {
              strncpy(variables[i].stringValue, newString, sizeof(variables[i].stringValue) - 1);
              variables[i].stringValue[sizeof(variables[i].stringValue) - 1] = '\0'; // Garante terminação nula
              changed = true;
            }
          }
          break;
      }

      if (changed) {
        Serial.printf("Set variable '%s' updated.\n", varName);
        // NOVO: Chama o callback da aplicação se a variável mudou
        if (this->onVariableChange) {
            this->onVariableChange(varName);
        }
      }
    }
  }
}

void EWBServer::generateJsonState(JsonDocument& doc) {
    doc.clear();
    for (int i = 0; i < numVariables; i++) {
        switch (variables[i].type) {
            case TYPE_INT:
                doc[variables[i].name] = variables[i].intValue;
                break;
            case TYPE_FLOAT:
                doc[variables[i].name] = variables[i].floatValue;
                break;
            case TYPE_STRING:
                doc[variables[i].name] = variables[i].stringValue;
                break;
        }
    }
}


// --- Implementação dos Callbacks BLE ---

// Callback do Servidor
MyServerCallbacks::MyServerCallbacks(bool* flag) : connectedFlag(flag) {}
void MyServerCallbacks::onConnect(BLEServer* pServer) {
  *connectedFlag = true;
  Serial.println("Client Connected");
}
void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
  *connectedFlag = false;
  Serial.println("Client Disconnected");
  BLEDevice::startAdvertising(); // Reinicia o advertising para permitir nova conexão
}

// Callback da Característica JSON
JsonCharacteristicCallbacks::JsonCharacteristicCallbacks(EWBServer* serverInstance) : server(serverInstance) {}

void JsonCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  String value = pCharacteristic->getValue(); // USA String DO ARDUINO
  if (value.length() > 0) {
    Serial.print("Received JSON: ");
    Serial.println(value);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, value); // Funciona com String

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    if (doc.containsKey("get")) {
      Serial.println("'get' command received. State will be sent on next read.");
    } else if (doc.containsKey("set")) {
      server->handleJsonSet(doc);
    }
  }
}

void JsonCharacteristicCallbacks::onRead(BLECharacteristic* pCharacteristic) {
    StaticJsonDocument<512> doc;
    // Agora esta chamada é permitida por causa da declaração 'friend'
    server->generateJsonState(doc);
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    pCharacteristic->setValue(jsonString.c_str());
    Serial.print("Sent JSON state on read: ");
    Serial.println(jsonString);
}


// Callback da Característica de Controle de Stream
StreamControlCallbacks::StreamControlCallbacks(StreamCallback onStart, StreamCallback onStop)
    : onStreamStart(onStart), onStreamStop(onStop) {}

void StreamControlCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  String value = pCharacteristic->getValue(); // USA String DO ARDUINO
  if (value.length() == 1) {
    if (value[0] == 0x01 && onStreamStart) {
      Serial.println("Stream Start command received.");
      onStreamStart();
    } else if (value[0] == 0x00 && onStreamStop) {
      Serial.println("Stream Stop command received.");
      onStreamStop();
    }
  }
}