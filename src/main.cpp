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
#define Rele            04
#define LEDSec          02
#define RTCAtt          16
#define Button          17

//Entrada
#define Alarme00Hora    07
#define Alarme00Minuto  40
//Almoço Turno 01
#define Alarme01Hora    11
#define Alarme01Minuto  30
//Almoço Turno 02
#define Alarme02Hora    12
#define Alarme02Minuto  00
//Fim almoço Turno 02
#define Alarme03Hora    13
#define Alarme03Minuto  00
//Café
#define Alarme04Hora    15
#define Alarme04Minuto  15
//Café - Fim
#define Alarme05Hora    15
#define Alarme05Minuto  30
//Saída
#define Alarme06Hora    17
#define Alarme06Minuto  33
//Teste
#define Alarme07Hora    9
#define Alarme07Minuto  27
//Duração
#define AlarmeSegundo   04
#define Duracao         03
//-----------------------------------------------------------------------------

//Global-----------------------------------------------------------------------
    bool led = 0;
    bool internetConnected = 0;
    
    const char* ssid       = "AEPH do Brasil";
    const char* password   = "Aeph2018";
    const char* ssid2      = "AEPHTEC";
    const char* password2  = "Aeph2021";
  
    struct tm data;//armazena as informações de Data

    hw_timer_t *timer = NULL;//fazo controle do temporizador (interrupção por tempo)

    unsigned char ucHora[8] = {Alarme00Hora, Alarme01Hora, Alarme02Hora, Alarme03Hora, Alarme04Hora, Alarme05Hora, Alarme06Hora, Alarme07Hora};
    unsigned char ucMinuto[8] = {Alarme00Minuto, Alarme01Minuto,Alarme02Minuto,Alarme03Minuto,Alarme04Minuto,Alarme05Minuto,Alarme06Minuto,Alarme07Minuto};
//-----------------------------------------------------------------------------

//prototype--------------------------------------------------------------------
void vConnectInternet(bool * answer);
void vDisconnected();
void vTimeGet();
void vRequestTime(long * unixTime);
long lReturnUnixTime(String getURL);
void controleFluxo(unsigned char *ucControl);
void vPrintTime();
void vAlarm();

void IRAM_ATTR resetModule()
{
    ets_printf("(watchdog)Resetado...\n");
    esp_restart();
}
//-----------------------------------------------------------------------------
void setup()
{
    //Configuração de pinos
    pinMode(Rele, OUTPUT);
    pinMode(LEDSec, OUTPUT);
    pinMode(RTCAtt, OUTPUT);
    pinMode(Button, INPUT);
    Serial.begin(115200);

    timer = timerBegin(0,80,true);
    timerAttachInterrupt(timer, &resetModule, true);
    timerAlarmWrite(timer,6000000, true);
    timerAlarmEnable(timer);


}

void loop() 
{
    unsigned char control = 0x00;
    unsigned int second = 0;
    time_t tt = 0;
    digitalWrite(Rele, LOW);
    while(1)
    {
        timerWrite(timer,0); //reseta o temporizador alimenta o watchdog
        controleFluxo(&control);
        if(control == 0x02)
        {
            vAlarm();
            control = 0x01;
        }

        tt = time(NULL); //Obtem o tempo atual em segundos. 
        data = *gmtime(&tt);

        if(second != data.tm_sec)
        {
            vPrintTime();
            second = data.tm_sec;
            led = !led;
            digitalWrite(LEDSec, led);
        }
        
    }
}
//-----------------------------------------------------------------------------
void vConnectInternet(bool * answer)
{
    unsigned char tentativas = 0xFF;
    
    while(tentativas--)
    {
        WiFi.begin(ssid, password);
        
        delay(500);
        Serial.println("Conectando ao WiFi...");
        if(WiFi.status() == WL_CONNECTED)
        {
            digitalWrite(RTCAtt,LOW);
            tentativas = 0;
            *answer = 1;
        }
    }
    Serial.println("Conectado a Rede WiFi...");
    
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vDisconnected()
{
    WiFi.disconnect();
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vTimeGet()
{
    long int unixTime = 0;
    vRequestTime(&unixTime);
    if(unixTime != -1)
    {
        timeval tv;                         //cria a estrutura temporaria para função abaixo
        tv.tv_sec = unixTime - 10803;       //Atribui data atual retirada do servidor 
        settimeofday(&tv, NULL);            //Configura o RTC para manter a data atribuida atualizada 
        Serial.println("Timer Atualizado!");
    }
    else 
    {
        Serial.println("Erro...");
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vRequestTime(long * unixTime)
{
    bool answerConnect = 0;
    unsigned char tentativas = 0xFF;
    HTTPClient http;  
    //ANTES DE EFETUAR A REQUISIÇÃO VERIFICA SE A CONEXÃO AINDA ESTA ATIVA 
    vConnectInternet(&answerConnect);
    while(tentativas--)
    {
        if((WiFi.status() == WL_CONNECTED))
        {
            http.begin("http://worldtimeapi.org/api/timezone/America/Sao_Paulo");
            int httpCode = http.GET();
            //VERIFICA SE A REQUISIÇÃO TEVE RESPOSTA 
            if(httpCode > 0)
            {
                String payload = http.getString();
                *unixTime = lReturnUnixTime(payload); 
                digitalWrite(RTCAtt,LOW);
                vDisconnected();
                break;
            }
            else
            {
                Serial.println("Erro na requisição");
                digitalWrite(RTCAtt,HIGH);
                *unixTime = -1; 
            }
        }
        else
        {
            vConnectInternet(&answerConnect);
        }
    }
    if(tentativas == 0x00) digitalWrite(RTCAtt,HIGH);

}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
long lReturnUnixTime(String getURL)
{
    // String input;
    StaticJsonDocument<0> filter;
    filter.set(true);
    
    StaticJsonDocument<768> doc;
    deserializeJson(doc, getURL, DeserializationOption::Filter(filter));
    
    //const char* abbreviation = doc["abbreviation"]; // "-03"
    //const char* client_ip = doc["client_ip"]; // "177.30.2.95"
    //const char* datetime = doc["datetime"]; // "2021-02-03T11:29:56.640826-03:00"
    //int day_of_week = doc["day_of_week"]; // 3
    //int day_of_year = doc["day_of_year"]; // 34
    //bool dst = doc["dst"]; // false
        // doc["dst_from"] is null
    //int dst_offset = doc["dst_offset"]; // 0
        // doc["dst_until"] is null
    //int raw_offset = doc["raw_offset"]; // -10800
    //const char* timezone = doc["timezone"]; // "America/Sao_Paulo"
    long unixtime = doc["unixtime"]; // 1612362596
    //const char* utc_datetime = doc["utc_datetime"]; // "2021-02-03T14:29:56.640826+00:00"
    //const char* utc_offset = doc["utc_offset"]; // "-03:00"
    //int week_number = doc["week_number"]; // 5

    return unixtime;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void controleFluxo(unsigned char *ucControl)
{
    unsigned char _aux00 = *ucControl;
    static unsigned int ucDayofWeek = 0;
    static unsigned char buttonbouce = 0;
    time_t tt = time(NULL); //Obtem o tempo atual em segundos.
    time_t tt2 = 0;
    tt = time(NULL); //Obtem o tempo atual em segundos.
    data = *gmtime(&tt);
    if (_aux00 == 0x00)
    {
        _aux00 = 0x01;
        ucDayofWeek = data.tm_wday;
        vTimeGet();
    }
    else
    {
        for (size_t i = 0; i < sizeof(ucHora); i++)
        {
            if(ucHora[i] == data.tm_hour && ucMinuto[i] == data.tm_min && data.tm_sec < AlarmeSegundo)
            {
                _aux00 = 0x02;
                break;
            }
        }
        if((digitalRead(Button) == 0) && (buttonbouce == 0))
        {
            tt = time(NULL); //Obtem o tempo atual em segundos.
            tt2 = tt;
            while(tt2+1 > tt)
            {
                tt = time(NULL); //Obtem o tempo atual em segundos.  
            }
            if(digitalRead(Button) == 0) 
            {
                _aux00 = 0x02;
                buttonbouce = 0x01;
            }  
            
        }
        else if((digitalRead(Button) == 1) && (buttonbouce == 1))
        {
            buttonbouce = 0x00;
        }
    }
    if (ucDayofWeek != data.tm_wday)
    {
        _aux00 = 0x00;
    }
    *ucControl = _aux00; 
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vPrintTime()
{
    char data_formatada[64];
    time_t tt = time(NULL); //Obtem o tempo atual em segundos. 
    data = *gmtime(&tt);
    strftime(data_formatada, 64, "%H:%M:%S", &data);
    Serial.println(data_formatada);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vAlarm()
{
    time_t tt = time(NULL); //Obtem o tempo atual em segundos.
    unsigned int tempo = data.tm_sec;
    
    digitalWrite(Rele, HIGH);
    
    while(tempo + 3 > data.tm_sec )
    {
        tt = time(NULL); //Obtem o tempo atual em segundos.
        data = *gmtime(&tt);
    }
    
    digitalWrite(Rele, LOW);
    
    tt = time(NULL); //Obtem o tempo atual em segundos.
    data = *gmtime(&tt);
    tempo = data.tm_sec;
    while(tempo + 2 > data.tm_sec )
    {
        tt = time(NULL); //Obtem o tempo atual em segundos.
        data = *gmtime(&tt);
    }
}
//-----------------------------------------------------------------------------

