/**
 * @file EWBclient.js
 * @brief Abstrai a comunicação Web Bluetooth para o projeto ESP32WebBluetooth.
 */

// UUIDs devem ser idênticos aos definidos no firmware do ESP32
const EWB_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const JSON_VARS_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const STREAM_DATA_CHAR_UUID = "82b934b0-a02c-4fb5-a818-a35752697d57";
const STREAM_CONTROL_CHAR_UUID = "c8a4a259-4203-48e8-b39f-5a8b71d601b0";

class EWBClient {
    constructor() {
        this.device = null;
        this.server = null;
        this.jsonVarsChar = null;
        this.streamDataChar = null;
        this.streamControlChar = null;

        this.onDisconnectCallback = null;
    }

    /**
     * Inicia o processo de conexão com o dispositivo BLE.
     * @returns {Promise<void>} Promessa resolvida quando a conexão e configuração estiverem completas.
     */
    async connect() {
        if (!navigator.bluetooth) {
            const errorMsg = 'Web Bluetooth API is not available. Please use a compatible browser (Chrome, Edge) on a secure context (HTTPS or localhost).';
            console.error(errorMsg);
            throw new Error(errorMsg);
        }

        try {
            console.log('Requesting Bluetooth Device...');
            this.device = await navigator.bluetooth.requestDevice({
                filters: [{ services: [EWB_SERVICE_UUID] }]
            });

            console.log('Connecting to GATT Server...');
            this.device.addEventListener('gattserverdisconnected', this._onDisconnect.bind(this));
            this.server = await this.device.gatt.connect();

            console.log('Getting Service...');
            const service = await this.server.getPrimaryService(EWB_SERVICE_UUID);

            console.log('Getting Characteristics...');
            this.jsonVarsChar = await service.getCharacteristic(JSON_VARS_CHAR_UUID);
            this.streamDataChar = await service.getCharacteristic(STREAM_DATA_CHAR_UUID);
            this.streamControlChar = await service.getCharacteristic(STREAM_CONTROL_CHAR_UUID);

            console.log('Client connected and ready.');

        } catch (error) {
            console.error('Connection failed!', error);
            throw error;
        }
    }

    /**
     * Desconecta do dispositivo BLE.
     */
    disconnect() {
        if (!this.server || !this.server.connected) {
            return;
        }
        console.log('Disconnecting...');
        this.server.disconnect();
    }

    /**
     * Handler interno para o evento de desconexão.
     */
    _onDisconnect() {
        console.log('Device disconnected.');
        if (this.onDisconnectCallback) {
            this.onDisconnectCallback();
        }
    }

    /**
     * Define uma função a ser chamada quando o dispositivo se desconecta.
     * @param {function} callback 
     */
    onDisconnect(callback) {
        this.onDisconnectCallback = callback;
    }

    /**
     * Lê o estado atual de todas as variáveis do ESP32.
     * @returns {Promise<Object>} Um objeto com o estado das variáveis.
     */
    async getVariables() {
        const value = await this.jsonVarsChar.readValue();
        const textDecoder = new TextDecoder('utf-8');
        const jsonString = textDecoder.decode(value);
        return JSON.parse(jsonString);
    }

    /**
     * Define o valor de uma ou mais variáveis no ESP32.
     * @param {Object} varsToSet - Objeto contendo as variáveis a serem alteradas. Ex: { led_intensity: 100 }
     */
    async setVariables(varsToSet) {
        const command = { set: varsToSet };
        const jsonString = JSON.stringify(command);
        const textEncoder = new TextEncoder();
        await this.jsonVarsChar.writeValue(textEncoder.encode(jsonString));
    }


    /**
     * Inicia o streaming de dados e registra um callback para processar os dados recebidos.
     * @param {function(Object)} onDataCallback - Função chamada com cada pacote de dados decodificado.
     */
    async startStream(onDataCallback) {
        // Registra o listener para as notificações de dados
        this.streamDataChar.addEventListener('characteristicvaluechanged', (event) => {
            this._handleStreamData(event.target.value, onDataCallback);
        });

        // Inicia as notificações no dispositivo
        await this.streamDataChar.startNotifications();
        console.log('Stream notifications started.');

        // Envia o comando para o ESP32 iniciar o envio dos dados
        const startCommand = new Uint8Array([0x01]);
        await this.streamControlChar.writeValue(startCommand);
        console.log('Stream START command sent.');
    }

    /**
     * Para o streaming de dados.
     */
    async stopStream() {
        // Envia o comando para o ESP32 parar o envio dos dados
        const stopCommand = new Uint8Array([0x00]);
        await this.streamControlChar.writeValue(stopCommand);
        console.log('Stream STOP command sent.');
        
        // Para de ouvir as notificações
        await this.streamDataChar.stopNotifications();
        console.log('Stream notifications stopped.');
    }

    /**
     * Processa o buffer de dados binários recebido do ESP32.
     * @param {DataView} dataView - O buffer de dados recebido.
     * @param {function} callback - A função para chamar com os dados decodificados.
     */
    _handleStreamData(dataView, callback) {
        // A estrutura no ESP32 é: 6x uint16_t + 1x uint32_t = 16 bytes por pacote
        const packetSizeBytes = (6 * 2) + 4;
        const numPackets = dataView.byteLength / packetSizeBytes;

        for (let i = 0; i < numPackets; i++) {
            const offset = i * packetSizeBytes;
            const packet = {
                reading1: dataView.getUint16(offset + 0, true), // true para little-endian
                reading2: dataView.getUint16(offset + 2, true),
                reading3: dataView.getUint16(offset + 4, true),
                reading4: dataView.getUint16(offset + 6, true),
                reading5: dataView.getUint16(offset + 8, true),
                reading6: dataView.getUint16(offset + 10, true),
                time_ms: dataView.getUint32(offset + 12, true)
            };
            callback(packet);
        }
    }
}