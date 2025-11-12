# Sistema de Monitoramento de Nível d’Água para Aquários (ESP32 + LCD + HC-SR04 + ThingSpeak + Telegram)

Integrantes: Adrielle Valascvijus Fernandes, Lavínia Lopes de Lana e Michael Douglas Santos Costa
Monitoramento automático do nível de água de um aquário utilizando ESP32, sensor ultrassônico HC-SR04, display LCD I²C, LEDs indicadores e integração com ThingSpeak e Telegram.

---

## Visão Geral

O sistema mede a distância da superfície da água até o fundo do aquário utilizando o sensor HC-SR04. A cada ciclo de leitura, ele:

1. Calcula o nível (%) com base na altura total do aquário.  
2. Exibe os dados no display LCD.  
3. Acende LEDs de cores diferentes conforme a faixa de nível.  
4. Armazena o histórico no SPIFFS do ESP32.  
5. Envia dados ao ThingSpeak.  
6. Envia alertas automáticos ao Telegram quando o nível atinge valores críticos.

> <img width="514" height="263" alt="Captura de tela 2025-11-11 204127" src="https://github.com/user-attachments/assets/39745243-1005-432c-b6d9-5a4962af9a2b" />

---

## Funcionalidades

- Leitura precisa de distância via sensor ultrassônico.  
- Exibição em LCD 20x4 (I²C).  
- Sinalização visual com LEDs (verde, amarelo, vermelho).  
- Armazenamento local de histórico no SPIFFS.  
- Envio de dados ao ThingSpeak.  
- Alertas automáticos via Telegram.  
- Modo simulado (sem internet real, para Wokwi) e modo real (com Wi-Fi).

---

## Componentes

| Componente | Quantidade | Observações |
|-------------|-------------|-------------|
| ESP32 DevKit v1 | 1 | Placa principal |
| Sensor ultrassônico HC-SR04 | 1 | Mede a distância da superfície |
| Display LCD I²C 20x4 | 1 | Endereço 0x27 |
| LEDs (vermelho, amarelo, verde) | 3 | Indicadores de nível |
| Resistores (220–330 Ω) | 3 | Proteção dos LEDs |
| Jumpers | - | Ligações elétricas |

> <img width="975" height="499" alt="Captura de tela 2025-11-11 203848" src="https://github.com/user-attachments/assets/8800dbce-8e33-420e-8379-4a0abe0f9573" />
---

## Ligações (Pinagem)

| Módulo | Pino | ESP32 |
|--------|------|-------|
| HC-SR04 | VCC | 5V |
| HC-SR04 | GND | GND |
| HC-SR04 | TRIG | GPIO 5 |
| HC-SR04 | ECHO | GPIO 18 |
| LCD I²C | VCC | 5V |
| LCD I²C | GND | GND |
| LCD I²C | SDA | GPIO 21 |
| LCD I²C | SCL | GPIO 22 |
| LED Vermelho | Ânodo via resistor | GPIO 25 |
| LED Amarelo | Ânodo via resistor | GPIO 26 |
| LED Verde | Ânodo via resistor | GPIO 27 |

---

## Dependências (Arduino IDE)

Instale as seguintes bibliotecas:

- LiquidCrystal_I2C  
- SPIFFS  
- WiFi  
- WiFiClientSecure  
- HTTPClient  
- NTPClient  
- UniversalTelegramBot  
- ArduinoJson  

**Placa:** ESP32 Dev Module  
**Porta:** USB conectada ao ESP32  

---

## Configurações no Código

```cpp
bool modoSimulado = true; // true = simulação (Wokwi) | false = real (Wi-Fi)

const float ALTURA_TANQUE = 50.0;
const float NIVEL_IDEAL   = 45.0;

const unsigned long INTERVALO = 10000;

const int TRIG = 5, ECHO = 18;
const int LED_VERMELHO = 25, LED_AMARELO = 26, LED_VERDE = 27;

const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

const char* server = "http://api.thingspeak.com/update";
String apiKey = "SUA_WRITE_API_KEY";

#define BOT_TOKEN "SEU_BOT_TOKEN"
#define CHAT_ID   "SEU_CHAT_ID"

````
---

## Demonstração de Resultados

A seguir estão os registros visuais do funcionamento completo do sistema, incluindo leituras no LCD, comunicação com o ThingSpeak e mensagens de alerta enviadas pelo bot do Telegram.

### Funcionamento no Simulador (Wokwi)

O projeto em simulação mostra a leitura de nível sendo exibida no display LCD e os LEDs correspondentes às faixas de nível de água.

><img width="1081" height="750" alt="Captura de tela 2025-11-11 202759" src="https://github.com/user-attachments/assets/fa737ec1-3a2c-4ff1-bbef-74098ef0cf05" />

### Comunicação com ThingSpeak

Cada leitura é enviada automaticamente ao ThingSpeak, como mostrado no console do ESP32.  
><img width="1326" height="816" alt="image" src="https://github.com/user-attachments/assets/e061312c-48d4-42d0-a406-a2420108eb73" />

### Alertas via Telegram

O sistema envia mensagens automáticas ao Telegram conforme o nível de água atinge determinadas faixas:

**1. Alerta Crítico (nível muito baixo)**  
><img width="439" height="189" alt="Captura de tela 2025-11-11 202837" src="https://github.com/user-attachments/assets/0b446740-afe5-4770-b00b-eb1a7b7a84b8" />

**2. Nível Normalizado**  
><img width="453" height="164" alt="Captura de tela 2025-11-11 202902" src="https://github.com/user-attachments/assets/5d560c96-27f0-4c1b-afcb-ab22d5f55a3b" />

**3. Alerta de Atenção (nível intermediário)**  
><img width="510" height="189" alt="Captura de tela 2025-11-11 203822" src="https://github.com/user-attachments/assets/7d7e594a-dfa5-4ac7-9e46-fb52c1deb01a" />

Essas mensagens são geradas automaticamente e incluem hora, altura da água e nível percentual calculado no momento da medição.


