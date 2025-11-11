// -------------------------------
// Sistema de Monitoramento de Nível d’Água para Aquários com ESP32 (Wi-Fi)
// Adrielle Valascvijus Fernandes, Lavínia Lopes de Lana e Michael Douglas Santos Costa
// -------------------------------

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// -------------------------------
// CONFIGURAÇÕES DO TELEGRAM
// -------------------------------

#define BOT_TOKEN "8434539675:AAEDLNLnmKQz7ILwi2W2DFALSKWlrg-vRcs"  
#define CHAT_ID "5727617812"  

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
bool alertaEnviado = false;  

// -------------------------------
// CONFIGURAÇÕES DE HARDWARE
// -------------------------------

LiquidCrystal_I2C lcd(0x27, 20, 4);

const int TRIG = 5;
const int ECHO = 18;

const int LED_VERMELHO = 25;
const int LED_AMARELO  = 26;
const int LED_VERDE    = 27;

const float ALTURA_TANQUE = 50.0;   
const float NIVEL_IDEAL   = 45.0;   

const unsigned long INTERVALO = 10000;
unsigned long ultimoTempo = 0;

// -------------------------------
// CONFIGURAÇÃO DE REDE E SERVIÇOS
// -------------------------------

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* server = "http://api.thingspeak.com/update";
String apiKey = "EA5GYXPB5V6MZWPG";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

// -------------------------------
// SETUP
// -------------------------------

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS!");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi...");
  Serial.println("Conectando ao WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  const unsigned long timeout = 30000; // 30 segundos

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttempt > timeout) {
      Serial.println();
      Serial.println("Falha ao conectar.");
      lcd.clear();
      lcd.print("Erro WiFi!");
      delay(3000);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.print("WiFi conectado!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());

  timeClient.begin();
  timeClient.update();

  client.setInsecure();

  lcd.clear();
  lcd.print("Sistema pronto!");
  delay(1500);
  lcd.clear();
}

// -------------------------------
// LOOP PRINCIPAL
// -------------------------------

void loop() {
  timeClient.update();

  unsigned long agora = millis();
  if (agora - ultimoTempo >= INTERVALO) {
    ultimoTempo = agora;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calculando...");
    delay(500);

    float distancia = medirDistancia();
    float nivel = (distancia / NIVEL_IDEAL) * 100.0;

    if (nivel < 0) nivel = 0;
    if (nivel > 100) nivel = 100;

    mostrarNoLCD(distancia, nivel);
    atualizarLEDs(distancia);

    String hora = timeClient.getFormattedTime();
    String registro = hora + " | Altura: " + String(distancia, 1) + "cm | Nivel: " + String(nivel, 1) + "%";

    enviarThingSpeak(distancia, nivel);
    verificarNivelCritico(nivel);  
  }
}

// -------------------------------
// FUNÇÕES 
// -------------------------------

float medirDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duracao = pulseIn(ECHO, HIGH, 30000); 
  if (duracao == 0) {
    Serial.println("Aviso: sem retorno do sensor (timeout).");
    return ALTURA_TANQUE; 
  }
  float distancia = duracao * 0.0343 / 2.0;
  return distancia;
}

void mostrarNoLCD(float distancia, float nivel) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Altura: ");
  lcd.print(distancia, 1);
  lcd.print("cm");
  lcd.setCursor(0, 1);
  lcd.print("Nivel: ");
  lcd.print(nivel, 1);
  lcd.print("%");
}

void atualizarLEDs(float distancia) {
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);

  if (distancia >= 40 && distancia <= 45) {
    digitalWrite(LED_VERDE, HIGH);
  } else if (distancia >= 30 && distancia <= 39) {
    digitalWrite(LED_AMARELO, HIGH);
  } else if (distancia < 30) {
    digitalWrite(LED_VERMELHO, HIGH);
  }
}

void enviarThingSpeak(float distancia, float nivel) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + String(distancia, 1) +
                 "&field2=" + String(nivel, 1);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Dados enviados para ThingSpeak. HTTP code: " + String(httpCode));
    } else {
      Serial.println("Falha no envio. Código: " + String(httpCode));
    }
    http.end();
  } else {
    Serial.println("Wi-Fi desconectado!");
  }
}

// -------------------------------
// ALERTA DO TELEGRAM
// -------------------------------

void verificarNivelCritico(float nivel) {
  static unsigned long ultimoAlerta = 0;
  static String ultimoEstado = "normal"; 
  unsigned long agora = millis();

  String hora = timeClient.getFormattedTime();

  // -------------------------------
  // 🚨 NÍVEL CRÍTICO
  // -------------------------------

   if (nivel < 30.0) {
    if (ultimoEstado != "critico" || (agora - ultimoAlerta >= 60000)) { 
      String mensagem = "🚨 *ALERTA DE NÍVEL CRÍTICO!* 🚨\n\n";
      mensagem += "🔹 *Horário:* " + hora + "\n";
      mensagem += "🔹 *Altura da água:* " + String((NIVEL_IDEAL * (nivel / 100.0)), 1) + " cm\n";
      mensagem += "🔹 *Nível:* " + String(nivel, 1) + "%\n\n";
      mensagem += "💧 Verifique o aquário imediatamente!";
      bot.sendMessage(CHAT_ID, mensagem, "Markdown");
      Serial.println("🚨 Alerta CRÍTICO enviado!");
      ultimoEstado = "critico";
      ultimoAlerta = agora;
    }
  } 

  // -------------------------------
  // ⚠️ NÍVEL DE ATENÇÃO
  // -------------------------------

  else if (nivel >= 30.0 && nivel <= 39.0) {
  if (ultimoEstado != "atencao" || (agora - ultimoAlerta >= 10000)) {  // 10 segundos
    String mensagem = "⚠️ *Alerta de Atenção*\n\n";
    mensagem += "🔹 *Horário:* " + hora + "\n";
    mensagem += "🔹 *Altura da água:* " + String((NIVEL_IDEAL * (nivel / 100.0)), 1) + " cm\n";
    mensagem += "🔹 *Nível:* " + String(nivel, 1) + "%\n\n";
    mensagem += "💧 Atenção: o nível está abaixo do ideal. Monitore de perto.";
    bot.sendMessage(CHAT_ID, mensagem, "Markdown");
    Serial.println("⚠️ Alerta de ATENÇÃO enviado!");
    ultimoEstado = "atencao";
    ultimoAlerta = agora;
    }
  } 

  // -------------------------------
  // ✅ NÍVEL NORMAL
  // -------------------------------

  else if (nivel >= 40.0) {
    if (ultimoEstado != "normal") {
      String mensagem = "✅ *Nível de água normalizado*\n\n";
      mensagem += "🔹 *Horário:* " + hora + "\n";
      mensagem += "🔹 *Altura da água:* " + String((NIVEL_IDEAL * (nivel / 100.0)), 1) + " cm\n";
      mensagem += "🔹 *Nível:* " + String(nivel, 1) + "%\n\n";
      mensagem += "💧 O nível voltou ao normal. Sistema estabilizado.";
      bot.sendMessage(CHAT_ID, mensagem, "Markdown");
      Serial.println("✅ Nível normalizado!");
      ultimoEstado = "normal";
      ultimoAlerta = agora;
    }
  }
}