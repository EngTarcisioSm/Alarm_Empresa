/*
 *  Projeto: Alarme Empresa
 *  Autor: Eng. Tarcísio Souza de Melo
 *  Tipo: Baremetal
 *  Data: 04-02-2021
 * 
 *  Observações:
 * 
 */

//Bibliotecas------------------------------------------------------------------
#include <Arduino.h>
//Inclusão de biblioteca para trabalhar com internet 
#include <WiFi.h>
//Biblioteca de requisição get() http
#include <HTTPClient.h>
//Tratando JSON
#include <ArduinoJson.h>
//Controle RTC
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>
//Gerenciar delays
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
//-----------------------------------------------------------------------------

//Defines----------------------------------------------------------------------

//-----------------------------------------------------------------------------

//Global-----------------------------------------------------------------------
  const char* ssid       = "AEPH do Brasil";
  const char* password   = "Aeph2018";
  const char* ssid2      = "AEPHTEC";
  const char* password2  = "Aeph2021";
  
  struct tm data;//armazena as informações de Data
//-----------------------------------------------------------------------------

//prototype--------------------------------------------------------------------
void vRequestTime(long * unixTime);
long lReturnUnixTime(String getURL);
//-----------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
    delay(500);
    Serial.println("Conectando ao WiFi...");
    }
    Serial.println("Conectado a Rede WiFi...");

}

void loop() 
{
    long int unixTime = 0;
    vRequestTime(&unixTime);
    if(unixTime != -1)
    {
        timeval tv;                         //cria a estrutura temporaria para função abaixo
        tv.tv_sec = unixTime - 10800;       //Atribui data atual retirada do servidor 
        settimeofday(&tv, NULL);            //Configura o RTC para manter a data atribuida atualizada 
    }
    else 
    {
        Serial.println("Erro...");
    }
    


    while(1)
    {
        char data_formatada[64];
        time_t tt = time(NULL); //Obtem o tempo atual em segundos. 
        data = *gmtime(&tt);
        strftime(data_formatada, 64, "%H:%M:%S", &data);

        Serial.print(data_formatada);
        switch(data.tm_wday)
        {
            case 0:
                Serial.println(" Domingo");
                break;
            case 1:
                Serial.println(" Segunda");
                break;
            case 2:
                Serial.println(" Terça");
                break;
            case 3:
                Serial.println(" Quarta");
                break;
            case 4:
                Serial.println(" Quinta");
                break;
            case 5:
                Serial.println(" Sexta");
                break;
            case 6:
                Serial.println(" Sabado");
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}

void vRequestTime(long * unixTime)
{
    HTTPClient http;  
    //ANTES DE EFETUAR A REQUISIÇÃO VERIFICA SE A CONEXÃO AINDA ESTA ATIVA 
    if((WiFi.status() == WL_CONNECTED))
    {
        http.begin("http://worldtimeapi.org/api/timezone/America/Sao_Paulo");
        int httpCode = http.GET();
        //VERIFICA SE A REQUISIÇÃO TEVE RESPOSTA 
        if(httpCode > 0)
        {
            String payload = http.getString();
            *unixTime = lReturnUnixTime(payload); 
        }
        else
        {
            Serial.println("Erro na requisição");
            *unixTime = -1; 
        }
    }
}

long lReturnUnixTime(String getURL)
{
    // String input;
    StaticJsonDocument<0> filter;
    filter.set(true);
    
    StaticJsonDocument<768> doc;
    deserializeJson(doc, getURL, DeserializationOption::Filter(filter));
    
    const char* abbreviation = doc["abbreviation"]; // "-03"
    const char* client_ip = doc["client_ip"]; // "177.30.2.95"
    const char* datetime = doc["datetime"]; // "2021-02-03T11:29:56.640826-03:00"
    int day_of_week = doc["day_of_week"]; // 3
    int day_of_year = doc["day_of_year"]; // 34
    bool dst = doc["dst"]; // false
    // doc["dst_from"] is null
    int dst_offset = doc["dst_offset"]; // 0
    // doc["dst_until"] is null
    int raw_offset = doc["raw_offset"]; // -10800
    const char* timezone = doc["timezone"]; // "America/Sao_Paulo"
    long unixtime = doc["unixtime"]; // 1612362596
    const char* utc_datetime = doc["utc_datetime"]; // "2021-02-03T14:29:56.640826+00:00"
    const char* utc_offset = doc["utc_offset"]; // "-03:00"
    int week_number = doc["week_number"]; // 5

    return unixtime;
}