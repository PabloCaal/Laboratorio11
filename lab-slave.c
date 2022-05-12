/* 
 * File:   lab-slave.c
 * Author: Pablo Caal
 * 
 * Created on 9 de mayo de 2022, 08:15 PM
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
#define FLAG_SPI 0xFF           // Variable bandera para lectura del esclavo

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t CONTADOR = 5;               // Valor del contador (Esclavo)
uint8_t TEMPORAL;           // Variable para almacenar valores temporales

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(INTCONbits.RBIF){                // Verificación de interrupción del PORTB
        if(!PORTBbits.RB0){             // Verificación de interrupción de RB0 (Incrementar)
            CONTADOR++;                 // Incrementamos el contador del esclavo
        }
        else if(!PORTBbits.RB1){        // Verificación de interrupción de RB1 (Decrementar)
            CONTADOR--;                 // Decrementamos el contador del esclavo
        }
        INTCONbits.RBIF = 0;            // Limpieza de bandera de interrupción del PORTB
    }
    
    if (PIR1bits.SSPIF){                // ¿Recibió datos el esclavo?
        TEMPORAL = SSPBUF;              // Se carga el valor proveniente del maestro a TEMPORAL para verificar que sea un dato
        PORTD = TEMPORAL;           // Mostramos el dato en PORTD
        SSPBUF = CONTADOR;          // Cargamos el valor del contador al maestro
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
    // Configuración del oscilador interno    
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno
    
    // Configuración de puertos
    ANSEL = 0x00;
    ANSELH = 0x00;              // I/O digitales
        
    TRISA = 0b00100000;         // SS y RA7 como entradas
    TRISB = 0b00000011;         // RB0 y RB1 como entradas
    TRISC = 0b00011000;         // SDI y SCK entradas, SD0 como salida
    TRISD = 0x00;               // PORTD como salida
    
    PORTA = 0x00;               // Limpieza del PORTA
    PORTB = 0x00;               // Limpieza del PORTB
    PORTC = 0x00;               // Limpieza del PORTC
    PORTD = 0x00;               // Limpieza del PORTD
    
    // Configuración de interrucpiones
    INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
    //INTCONbits.PEIE = 1;      // Habilitamos interrupciones de perifericos
    INTCONbits.RBIE = 1;        // Habilitación de interrupciones del PORTB
    INTCONbits.RBIF = 0;        // Limpieza bandera de interrupción del PORTB
    
    // Configuración de interrupciones por cambio de estado para PORTB
    IOCBbits.IOCB0 = 1;         // Habilitación de interrupción por cambio de estado para RB0
    IOCBbits.IOCB1 = 1;         // Habilitación de interrupción por cambio de estado para RB1
    OPTION_REGbits.nRBPU = 0;   // Habilitación de resistencias de pull-up del PORTB
    WPUBbits.WPUB0 = 1;         // Habilitación de resistencia de pull-up de RB0
    WPUBbits.WPUB1 = 1;         // Habilitación de resistencia de pull-up de RB1
    
    // Configuración de SPI
    // Configuración del ESCLAVO
    // SSPCON <5:0>
    SSPCONbits.SSPM = 0b0100;   // SPI Esclavo, SS hablitado
    SSPCONbits.CKP = 0;         // Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;        // Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 0;        // Dato al final del pulso de reloj (Siempre debe estar apagado para esclavos)

    PIR1bits.SSPIF = 0;         // Limpieza de bandera de SPI (Se debe limpiar manualmente por medio de software)
    PIE1bits.SSPIE = 1;         // Habilitar interrupciones de SPI
    INTCONbits.GIE = 1;         // Habilitar interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitar interrupciones de periféricos
}