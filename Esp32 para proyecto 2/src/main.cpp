
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ESP32SPISlave.h>
#include <stdint.h>
#include <string.h>

// FUNCIONES
void lecturas(void);
void pantalla(void);
void onReceived(int len);
void onRequest(void);
void recibirSPI(void);
void controlarLED(void);

// POTENCIOMETRO
const int pot1 = 36;

// I2C
#define I2C_DEV_ADDR 0x55

// LCD
#define rs 2
#define en 4
#define d4 5
#define d5 18
#define d6 19
#define d7 23

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// SPI HSPI
#define PIN_MISO 12
#define PIN_MOSI 13
#define PIN_SCK 14
#define PIN_CS 15

ESP32SPISlave slave;

alignas(4) uint8_t spiRX[4] = {0, 0, 0, 0};

const size_t SPI_TAMANO = 4;

// CONTROL DE LEDS
#define LED1 27
#define LED2 25
#define LED3 33

int ledActual = 0;
int ultimoLED = 0;

uint32_t tiempoLED = 0;
uint32_t inicioLED = 0;

bool ledEncendido = false;

// ADC
int lectura1 = 0;
int lectura = 0;
float voltaje = 0.0;

volatile uint16_t valorADC = 0;
volatile bool solicitarADC = false;

void setup()
{
    Serial.begin(115200);

    // POTENCIOMETRO
    pinMode(pot1, INPUT);

    // LEDS
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);

    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);

    // LCD
    lcd.begin(16, 2);
    lcd.clear();

    // I2C ESCLAVO
    Wire.onReceive(onReceived);
    Wire.onRequest(onRequest);

    Wire.begin((uint8_t)I2C_DEV_ADDR,21, 22, 100000);

    // SPI ESCLAVO

    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(1);

    slave.begin(HSPI, PIN_SCK,PIN_MISO,PIN_MOSI, PIN_CS);

    memset(spiRX, 0, sizeof(spiRX));

    slave.queue(NULL, spiRX, SPI_TAMANO);
    slave.trigger();
}


void loop()
{
    
    recibirSPI();
    controlarLED();
    lecturas();

    if (solicitarADC)
    {
        solicitarADC = false;

        // Actualizar la medicion
        lecturas();

        uint16_t adc = valorADC;

        uint8_t datos[2];

        datos[0] = (uint8_t)(adc >> 8);
        datos[1] = (uint8_t)(adc & 0xFF);

        Wire.slaveWrite(datos, 2);
    }


    static uint32_t anteriorLCD = 0;

    if (millis() - anteriorLCD >= 200)
    {
        anteriorLCD = millis();
        pantalla();
    }
}


// LECTURA DEL POTENCIOMETRO
void lecturas()
{
    lectura1 = analogRead(pot1);
    lectura = map(lectura1, 0, 4095, 0, 255);
    voltaje = lectura1 * (3.3 / 4095.0);

    valorADC = (uint16_t)lectura1;
}

// COMUNICACION SPI


void recibirSPI()
{
    if (slave.hasTransactionsCompletedAndAllResultsReady(1))
    {
        const std::vector<size_t> recibidos = slave.numBytesReceivedAll();

        if (!recibidos.empty() &&recibidos[0] == SPI_TAMANO)
        {
            uint8_t nuevoLED = spiRX[0];
            uint16_t nuevoTiempo =
                ((uint16_t)spiRX[1] << 8) |spiRX[2];
            uint8_t verificacion =spiRX[0] ^ spiRX[1] ^spiRX[2] ^ 0xA5;

            if (spiRX[3] == verificacion &&nuevoLED >= 1 &&nuevoLED <= 3 &&nuevoTiempo >= 1 &&nuevoTiempo <= 60000)
            {
                // Apagar LED anterior
                digitalWrite(LED1, LOW);
                digitalWrite(LED2, LOW);
                digitalWrite(LED3, LOW);

                ledActual = nuevoLED;
                ultimoLED = nuevoLED;

                tiempoLED = nuevoTiempo;
                inicioLED = millis();

                ledEncendido = true;

                if (ledActual == 1)
                {
                    digitalWrite(LED1, HIGH);
                }
                else if (ledActual == 2)
                {
                    digitalWrite(LED2, HIGH);
                }
                else
                {
                    digitalWrite(LED3, HIGH);
                }

                Serial.printf("LED: %d, Tiempo: %u ms\n",ledActual,nuevoTiempo);
            }
            else
            {
                Serial.println(
                    "Comando SPI incorrecto"
                );
            }
        }
    }

    // Preparar la siguiente recepcion
    if (slave.hasTransactionsCompletedAndAllResultsHandled())
    {
        memset(spiRX, 0, sizeof(spiRX));

        if (slave.queue(NULL, spiRX, SPI_TAMANO))
        {
            slave.trigger();
        }
    }
}


// APAGAR LED DESPUES DEL TIEMPO
void controlarLED()
{
    if (ledEncendido &&(uint32_t)(millis() - inicioLED) >= tiempoLED)
    {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);

        ledEncendido = false;
        ledActual = 0;
    }
}

// PANTALLA LCD
void pantalla()
{
    // Primera fila: voltaje
    lcd.setCursor(0, 0);
    lcd.print("Pot1:");
    lcd.print(voltaje, 2);
    lcd.print("V    ");

    // Segunda fila: lectura ADC
    lcd.setCursor(0, 1);
    lcd.print("Pot1:");
    lcd.print(lectura1);
    lcd.print("    ");

    // Ultimo LED activado
    lcd.setCursor(12, 1);
    lcd.print("L:");

    if (ultimoLED == 1)
        lcd.print("R");
    else if (ultimoLED == 2)
        lcd.print("V");
    else if (ultimoLED == 3)
        lcd.print("A");
    else
        lcd.print("-");
}



void onReceived(int len)
{
    while (Wire.available())
    {
        Wire.read();
    }

    solicitarADC = true;
}

void onRequest()
{
    // Respuesta preparada previamente
}