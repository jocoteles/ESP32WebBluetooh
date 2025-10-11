# ESP32WebBluetooth (EWB)

Este projeto fornece uma base moderna, robusta e reativa para a comunicação entre um microcontrolador ESP32 e uma Progressive Web App (PWA) usando exclusivamente a API Web Bluetooth.

O EWB implementa dois canais de comunicação:
1.  **Canal JSON de Baixa Frequência:** Para leitura e escrita de variáveis de configuração (`led_intensity`, `device_label`).
2.  **Canal Binário de Alta Frequência:** Para streaming de dados de sensores.

## Filosofia do Projeto

**Robustez e Clareza.** O código foi refatorado para ser altamente confiável, especialmente no manuseio de dados de alta frequência via Bluetooth Low Energy (BLE). A arquitetura é construída para ser reativa, permitindo que o ESP32 responda imediatamente a comandos do cliente. O objetivo é fornecer um template sólido, eficiente e de fácil compreensão para qualquer projeto de Internet das Coisas (IoT) via BLE.

---

## Recursos Principais

### Servidor (ESP32 - C++)
A biblioteca `EWBServer` abstrai toda a complexidade do BLE e oferece dois recursos arquitetônicos chave:

*   **Comunicação Reativa com Callback:** A aplicação principal do ESP32 (`.ino`) pode registrar um **callback de alteração de variável**. Isso significa que, em vez de verificar repetidamente (polling) se uma variável mudou, o ESP32 é notificado instantaneamente assim que um comando `set` é recebido do cliente. Isso resulta em um código mais limpo e eficiente.
*   **Gestão de Variáveis Tipadas:** Variáveis de controle são definidas em um array (`VariableConfig`), permitindo que a biblioteca trate automaticamente a conversão de/para JSON, aplique limites de valor (`min`/`max`) e gerencie a comunicação BLE.

### Cliente (PWA - JavaScript)
O `EWBclient.js` encapsula a API Web Bluetooth e introduz uma camada de resiliência:

*   **Tratamento Robusto de Fragmentação:** O Web Bluetooth, por vezes, fragmenta pacotes de dados grandes (notificações) em múltiplas transmissões. O `EWBclient.js` agora possui um **mecanismo de buffering** que coleta os fragmentos e só processa um pacote de dados quando ele estiver completo. Isso garante que não haja perda ou corrupção de dados durante o streaming.
*   **Interface Simples:** O cliente expõe métodos simples e baseados em `Promise` (`connect()`, `getVariables()`, `setVariables()`, `startStream()`, `stopStream()`), facilitando a conexão com os elementos da interface (`main.js`).

---

## Como Funciona o Exemplo

O projeto de exemplo demonstra a configuração de variáveis (intensidade de LED, intervalo de update) e a visualização de um stream de 6 leituras de sensor.

1.  **Lado do ESP32 (`EWBServer.ino`):**
    *   Define as variáveis `led_intensity` e `update_interval`.
    *   Registra a função `onVariableChanged` para que o firmware possa agir imediatamente quando o cliente altera esses valores.
    *   A cada ciclo, lê 6 pinos analógicos e agrupa os dados em um buffer, enviando-os pela característica de streaming.

2.  **Lado do Cliente (`main.js`):**
    *   Usa `ewbClient.setVariables()` para enviar o novo valor de um slider (ex: `led_intensity`).
    *   O ESP32 recebe, atualiza o valor e notifica a aplicação via callback, que pode responder a comandos de controle instantaneamente (ex: mudar a frequência de leitura).
    *   Recebe o fluxo de dados binários, que é decodificado de forma segura pelo `EWBclient.js` (graças ao mecanismo de buffering) e exibido na interface.

---

## Instalação e Configuração

### Requisitos

1.  **Hardware:** Um ESP32 Dev Kit.
2.  **Firmware:** [Arduino IDE](https://www.arduino.cc/en/software) com o board manager do ESP32 e a biblioteca **ArduinoJson**.
3.  **PWA (Cliente):** Um navegador com suporte a Web Bluetooth (Chrome, Edge) em um contexto seguro (HTTPS ou `localhost`).

### Passos

#### 1. Gravar o Firmware no ESP32

1.  Copie o conteúdo da pasta `EWBServer` para um novo sketch no Arduino IDE.
2.  Instale a biblioteca `ArduinoJson`.
3.  Compile e envie o código para o seu ESP32.
4.  Abra o Monitor Serial (baud rate 115200) para ver "EWBServer started. Waiting for a client connection...".

#### 2. Hospedar a PWA

*   Hospede os arquivos da raiz do projeto (`index.html`, `main.js`, `EWBclient.js`, etc.) em um servidor web com suporte a HTTPS (ex: GitHub Pages).

---

## Como Rodar e Testar

1.  Certifique-se de que o seu ESP32 está ligado.
2.  No seu navegador compatível, abra o URL da sua PWA.
3.  Clique em **"Conectar"**, selecione o dispositivo `ESP32_EWB_Control` e emparelhe.
4.  **Teste o Controle:** Mova o slider "Intensidade do LED". O `EWBServer.ino` receberá o comando e fará o *echo* do novo valor no Monitor Serial imediatamente, graças ao **callback de alteração de variável**.
5.  **Teste o Streaming:** Clique em **"Iniciar Stream"**. O console do navegador agora lida de forma robusta com a transmissão, exibindo o JSON do último pacote recebido. Clique em **"Parar Stream"** para interromper.