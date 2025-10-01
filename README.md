# ESP32WebBluetooth (EWB)

Este projeto fornece uma base clara e pragmática para a comunicação entre um microcontrolador ESP32 e uma Progressive Web App (PWA) usando exclusivamente a API Web Bluetooth. A comunicação é dividida em dois canais principais: um para controle de variáveis de baixa frequência (via JSON) e outro para streaming de dados de alta frequência (via pacotes binários).

## Filosofia do Projeto

**Clareza sobre robustez.** O código foi escrito para ser o mais legível e fácil de entender possível. Privilegiamos a clareza, a legibilidade e o pragmatismo em detrimento de testes de borda e checagens excessivas. O objetivo é fornecer um ponto de partida sólido que possa ser facilmente modificado e adaptado, em vez de uma biblioteca "à prova de tudo".

---

## Como Funciona

### Lado do ESP32 (Servidor)
O firmware do ESP32 cria um servidor Bluetooth Low Energy (BLE) com um único serviço que expõe três "características" (characteristics):
1.  **JSON Variables (Read/Write):** Permite ler e escrever variáveis de configuração (como `led_intensity` ou `device_label`) usando strings JSON.
2.  **Stream Control (Write):** Uma característica simples que aceita um único byte para controlar o fluxo de dados (`0x01` para iniciar, `0x00` para parar).
3.  **Stream Data (Notify):** Envia pacotes de dados binários de alta frequência (como leituras de sensores) para o cliente quando o streaming está ativo.

A biblioteca `EWBServer` abstrai toda a complexidade do BLE, permitindo que o arquivo principal (`.ino`) foque apenas na lógica da aplicação.

### Lado do Cliente (PWA)
A PWA usa a API Web Bluetooth do navegador para se conectar ao ESP32.
1.  O `EWBclient.js` encapsula toda a lógica de conexão, leitura/escrita de características e tratamento de notificações.
2.  O `main.js` utiliza o `EWBClient` para conectar os elementos da interface (`index.html`) com a comunicação Bluetooth.
3.  A aplicação é um PWA, o que significa que pode ser "instalada" no dispositivo do usuário para uma experiência semelhante a um aplicativo nativo.

---

## Instalação e Configuração

### Requisitos

1.  **Hardware:** Um ESP32 Dev Kit.
2.  **Firmware:**
    *   [Arduino IDE](https://www.arduino.cc/en/software) com o [board manager do ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) instalado.
    *   Ou [PlatformIO](https://platformio.org/) com VS Code.
    *   Biblioteca **ArduinoJson** (pode ser instalada via Library Manager do Arduino IDE ou PlatformIO).
3.  **PWA (Cliente):**
    *   Um navegador com suporte a Web Bluetooth (Chrome, Edge, Opera em Desktop e Android).
    *   Um servidor web para hospedar os arquivos do PWA. **GitHub Pages** é uma excelente opção gratuita. **Web Bluetooth requer uma conexão segura (HTTPS)**, que o GitHub Pages fornece por padrão.
    *   **Nota sobre o Chrome:** Em alguns ambientes ou versões, o Chrome pode exigir que você ative uma flag para recursos experimentais. Se a aplicação reportar que o Web Bluetooth não é suportado, navegue até `chrome://flags/#enable-experimental-web-platform-features`, ative a opção e reinicie o navegador.

### Passos

#### 1. Gravar o Firmware no ESP32

1.  Copie a pasta `EWBServer` para a sua pasta de projetos do Arduino ou PlatformIO.
2.  Abra o arquivo `EWBServer.ino`.
3.  Instale a dependência `ArduinoJson` se ainda não o fez.
4.  Selecione a placa ESP32 correta e a porta serial.
5.  Compile e envie o código para o seu ESP32.
6.  Abra o Monitor Serial (baud rate 115200) para ver as mensagens de log. Você deverá ver "EWBServer started. Waiting for a client connection...".

#### 2. Hospedar a PWA

1.  Crie um novo repositório no GitHub.
2.  Envie todos os arquivos da raiz do projeto ( `index.html`, `main.js`, etc.) para o repositório.
3.  No seu repositório do GitHub, vá para `Settings` -> `Pages`.
4.  Em "Source", selecione a branch `main` (ou `master`) e a pasta `/root`. Clique em `Save`.
5.  Aguarde alguns minutos. O GitHub irá publicar seu site em um endereço como `https://<seu-usuario>.github.io/<seu-repositorio>/`.

---

## Como Rodar e Testar

1.  Certifique-se de que o seu ESP32 está ligado e executando o firmware.
2.  No seu computador ou smartphone Android, abra o Google Chrome e navegue para o URL da sua GitHub Page.
3.  Clique no botão **"Conectar"**. Uma janela do navegador aparecerá, listando os dispositivos Bluetooth próximos.
4.  Selecione o dispositivo `ESP32_EWB_Control` e clique em "Emparelhar".
5.  Se a conexão for bem-sucedida, a barra de status ficará verde e os controles aparecerão.
6.  **Teste as variáveis:**
    *   Mova o slider "Intensidade do LED". O ESP32 receberá o novo valor.
    *   Altere o texto em "Nome do Dispositivo".
7.  **Teste o Streaming:**
    *   Clique em **"Iniciar Stream"**.
    *   A caixa de texto "Último pacote recebido" começará a ser atualizada rapidamente com os dados dos sensores lidos pelo ESP32.
    *   Clique em **"Parar Stream"** para interromper o fluxo de dados.

---

## Como Modificar para Usos Específicos

Este projeto foi feito para ser um template. Modificá-lo é simples:

### 1. Modificar Variáveis de Controle

*   **No ESP32 (`EWBServer.ino`):**
    *   Edite ou adicione entradas ao array `configurableVariables[]`. Defina o nome, tipo (`TYPE_INT`, `TYPE_FLOAT`, `TYPE_STRING`), valor inicial e limites.
*   **No PWA (`index.html` e `main.js`):**
    *   Adicione ou modifique os elementos HTML correspondentes em `index.html`.
    *   Em `main.js`, adicione os event listeners para os novos elementos, chamando `ewbClient.setVariables()` com o nome e valor corretos.
    *   Atualize a função `updateVariablesUI` para popular os novos campos quando os dados são recebidos.

### 2. Modificar o Pacote de Streaming

*   **No ESP32 (`EWBServer.ino`):**
    *   Altere a estrutura `SensorDataPacket` para incluir os dados que você deseja enviar.
    *   **Importante:** Atualize as constantes `PACKET_SIZE_BYTES` e `CHUNK_BUFFER_SIZE_BYTES` se o tamanho da estrutura mudar.
    *   Na função `loop()`, preencha a nova estrutura com seus dados antes de enviá-la.
*   **No PWA (`EWBclient.js`):**
    *   Na função `_handleStreamData`, ajuste a decodificação para corresponder à sua nova estrutura C++.
    *   Você precisará saber o tamanho (em bytes) e o tipo de cada campo (`getUint16`, `getFloat32`, etc.) e a ordem correta deles. Preste atenção no argumento `little-endian` (`true` para ESP32).