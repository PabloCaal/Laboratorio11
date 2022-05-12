/* 
 * File:   postlab-slave1.c
 * Author: Pablo Caal
 * 
 * MUC 2 - esclavo 1 del postlaboratorio 11 
 *  Recibe la señal de potenciómetro proveniente del MCU3 - master
 *  Transforma la señal del POT a PWM para controlar un servomotor
 * 
 * Created on 11 de mayo de 2022, 02:10 PM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT    // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF               // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF              // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF              // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF                 // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF                // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF              // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF               // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF              // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF                // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V           // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF                // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000      // Frecuencia de oscilador en 1 MHz
#define IN_MIN 0                // Valor minimo de entrada del potenciometro
#define IN_MAX 255              // Valor máximo de entrada del potenciometro
#define OUT_MIN 62              // Valor minimo de ancho de pulso de señal PWM   (18 para servo MG996R)
#define OUT_MAX 125             // Valor máximo de ancho de pulso de señal PWM   (79 para servo MG996R)

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t CCPR;                   // Valor de la señal PWM
uint8_t TEMPORAL;               // Variable para almacenar valores temporales

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);
unsigned short map(uint8_t val, uint8_t in_min, uint8_t in_max, 
            unsigned short out_min, unsigned short out_max);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){    
    if (PIR1bits.SSPIF){                // ¿Recibió datos el esclavo?
        TEMPORAL = SSPBUF;              // Se carga el valor proveniente del maestro a TEMPORAL para verificar que sea un dato
        CCPR = map(TEMPORAL, IN_MIN, IN_MAX, OUT_MIN, OUT_MAX);   // Asignación de valor de ancho de pulso a CCPR
        CCPR1L = (uint8_t)(CCPR>>2);        // Almacenamiento de los 8 bits mas significativos en CCPR1L
        CCP1CONbits.DC1B = CCPR & 0b11;     // Almacenamiento de los 2 bits menos significativos en DC1B
        PIR1bits.SSPIF = 0;             // Limpiamos bandera de interrupción
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){        
        // Envio y recepcion de datos en maestro
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){   
    TRISC = 0b00111000;         // SDI y SCK entradas, SD0 como salida
    PORTCbits.RC5 = 0;
    
    // Configuración del oscilador interno    
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno
    
    // Configuración de puertos
    ANSEL = 0x00;
    ANSELH = 0x00;              // I/O digitales
        
    TRISA = 0b00100000;         // SS como entrada
    TRISD = 0x00;               // PORTD como salida
    
    PORTA = 0x00;               // Limpieza del PORTA
    PORTC = 0x00;               // Limpieza del PORTC
    PORTD = 0x00;               // Limpieza del PORTD
    
    // Configuración de interrucpiones
    INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitamos interrupciones de perifericos
    PIR1bits.SSPIF = 0;         // Limpieza de bandera de SPI (Se debe limpiar manualmente por medio de software)
    PIE1bits.SSPIE = 1;         // Habilitar interrupciones de SPI
    
    // Configuración de SPI
    // Configuración del ESCLAVO
    // SSPCON <5:0>
    SSPCONbits.SSPM = 0b0100;   // SPI Esclavo, SS hablitado
    SSPCONbits.CKP = 0;         // Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;        // Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 0;        // Dato al final del pulso de reloj (Siempre debe estar apagado para esclavos)
    
    // Configuración PWM
    PIR1bits.TMR2IF = 0;        // Limpiamos bandera de interrupcion del TMR2
    T2CONbits.T2CKPS = 0b11;    // prescaler 1:16
    T2CONbits.TMR2ON = 1;       // Encendemos TMR2
    while(!PIR1bits.TMR2IF);    // Esperar un cliclo del TMR2
    PIR1bits.TMR2IF = 0;        // Limpiamos bandera de interrupcion del TMR2 nuevamente
    
    TRISCbits.TRISC2 = 1;       // Deshabilitamos salida de CCP1
    PR2 = 61;                   // periodo de 4 ms
        
    // PR2 = (PWM period)/(4(1/Fosc)(PrescalerTMR2))-1
    // PR2 = (20 ms)/(4(1/1MHz)(16))-1
    // PR2 = 61.5 = 61
    
    // Configuración CCP
    // CCP1
    CCP1CON = 0;                // Apagamos CCP1
    CCP1CONbits.P1M = 0;        // Modo single output
    CCP1CONbits.CCP1M = 0b1100; // PWM       
    CCPR1L = 250>>2;
    CCP1CONbits.DC1B = 250 & 0b11;    
   
    TRISCbits.TRISC2 = 0;       // Habilitamos salida de PWM (CCP1)
}

/*------------------------------------------------------------------------------
 * FUNCIONES 
 ------------------------------------------------------------------------------*/
// Función para hacer la interpolación lineal del valor de la entrada analógica 
unsigned short map(uint8_t x, uint8_t x0, uint8_t x1, 
            unsigned short y0, unsigned short y1){
    return (unsigned short)(y0+((float)(y1-y0)/(x1-x0))*(x-x0));
}