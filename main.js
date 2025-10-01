document.addEventListener('DOMContentLoaded', () => {
    // Elementos da UI - buscar os essenciais para a verificação primeiro
    const statusBar = document.getElementById('status-bar');
    const btnConnect = document.getElementById('btn-connect');
    const controlsDiv = document.getElementById('controls');

    // --- Verificação de compatibilidade ---
    if (!navigator.bluetooth) {
        statusBar.textContent = 'Web Bluetooth não suportado. Use Chrome/Edge (Desktop ou Android).';
        statusBar.className = 'status disconnected';
        btnConnect.disabled = true;
        controlsDiv.style.display = 'none';
        console.error('Web Bluetooth API not available. Please use a compatible browser like Chrome or Edge.');
        return; // Para a execução do restante do script de inicialização.
    }
    
    const ewbClient = new EWBClient();

    // Restante dos Elementos da UI
    const btnDisconnect = document.getElementById('btn-disconnect');
    
    const inputLedIntensity = document.getElementById('led-intensity');
    const valueLedIntensity = document.getElementById('led-intensity-value');
    const inputUpdateInterval = document.getElementById('update-interval');
    const inputMotorEnable = document.getElementById('motor-enable');
    const inputDeviceLabel = document.getElementById('device-label');

    const btnStartStream = document.getElementById('btn-start-stream');
    const btnStopStream = document.getElementById('btn-stop-stream');
    const streamOutput = document.getElementById('stream-output');

    let isStreaming = false;
    let debounceTimer;

    // --- Funções de atualização da UI ---
    
    function setUIConnected(connected) {
        btnConnect.disabled = connected;
        btnDisconnect.disabled = !connected;
        controlsDiv.style.display = connected ? 'block' : 'none';
        
        if (connected) {
            statusBar.textContent = 'Conectado';
            statusBar.className = 'status connected';
        } else {
            statusBar.textContent = 'Desconectado';
            statusBar.className = 'status disconnected';
            isStreaming = false;
            updateStreamButtons();
        }
    }

    function updateVariablesUI(variables) {
        inputLedIntensity.value = variables.led_intensity;
        valueLedIntensity.textContent = variables.led_intensity;
        inputUpdateInterval.value = variables.update_interval;
        inputMotorEnable.checked = variables.motor_enable === 1;
        inputDeviceLabel.value = variables.device_label;
    }
    
    function updateStreamButtons() {
        btnStartStream.disabled = isStreaming;
        btnStopStream.disabled = !isStreaming;
    }
    
    // --- Lógica de Eventos ---

    btnConnect.addEventListener('click', async () => {
        try {
            await ewbClient.connect();
            setUIConnected(true);
            const variables = await ewbClient.getVariables();
            updateVariablesUI(variables);
        } catch (error) {
            console.error('Falha ao conectar:', error);
            setUIConnected(false);
        }
    });

    btnDisconnect.addEventListener('click', () => {
        ewbClient.disconnect();
    });

    ewbClient.onDisconnect(() => {
        setUIConnected(false);
    });

    // Event listeners para os controles de variáveis
    
    inputLedIntensity.addEventListener('input', () => {
        const value = parseInt(inputLedIntensity.value, 10);
        valueLedIntensity.textContent = value;
        // Debounce para não enviar muitos comandos rapidamente
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            ewbClient.setVariables({ led_intensity: value });
        }, 200);
    });

    inputUpdateInterval.addEventListener('change', () => {
        ewbClient.setVariables({ update_interval: parseInt(inputUpdateInterval.value, 10) });
    });

    inputMotorEnable.addEventListener('change', () => {
        ewbClient.setVariables({ motor_enable: inputMotorEnable.checked ? 1 : 0 });
    });

    inputDeviceLabel.addEventListener('change', () => {
        ewbClient.setVariables({ device_label: inputDeviceLabel.value });
    });

    // Event listeners para o streaming

    btnStartStream.addEventListener('click', async () => {
        await ewbClient.startStream(handleStreamData);
        isStreaming = true;
        updateStreamButtons();
    });

    btnStopStream.addEventListener('click', async () => {
        await ewbClient.stopStream();
        isStreaming = false;
        updateStreamButtons();
    });

    function handleStreamData(packet) {
        // Atualiza a UI com os dados do pacote recebido
        streamOutput.textContent = JSON.stringify(packet, null, 2);
    }
    
    // Inicialização da UI
    setUIConnected(false);
    updateStreamButtons();

    // --- Service Worker Registration ---
    if ('serviceWorker' in navigator) {
        window.addEventListener('load', () => {
            navigator.serviceWorker.register('/sw.js').then(registration => {
                console.log('SW registered: ', registration);
            }).catch(registrationError => {
                console.log('SW registration failed: ', registrationError);
            });
        });
    }
});