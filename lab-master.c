/* 
 * File:   lab-master.c
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
uint8_t LECTURA_POT;            // Valor de lectura del potenciómetro (Maestro)

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.ADIF){                  // Verificación de interrupción del módulo ADC
        if(ADCON0bits.CHS == 0){        // Verificación de canal AN0
            LECTURA_POT = ADRESH;       // Almacenar el resgitro ADRESH en variable LECTURA_POT
        }
        PIR1bits.ADIF = 0;              // Limpieza de bandera de interrupción
    } 
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){
        // Activación del proceso de conversión del módulo ADC
        if(ADCON0bits.GO == 0){     // Si no hay proceso de conversión
            __delay_us(40);
            ADCON0bits.GO = 1;      // Ejecución de proceso de conversión
        }
        
        // Envío de valor al esclavo
        SSPBUF = LECTURA_POT;       // Cargamos valor del potenciómetro al buffer
        while(!SSPSTATbits.BF){}    // Esperamos a que termine el envio
        
        // Cambio en el selector (SS) para generar respuesta del pic
        PORTAbits.RA7 = 1;          // Deshabilitamos el ss del esclavo
        __delay_ms(10);             // Delay para que el PIC pueda detectar el cambio en el pin
        PORTAbits.RA7 = 0;          // habilitamos nuevamente el escalvo

        SSPBUF = FLAG_SPI;          // Master inicia la comunicación y prende el clock
        while(!SSPSTATbits.BF){}    // Esperamos a que se reciba un dato
        PORTD = SSPBUF;             // Mostramos dato recibido en PORTD
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
    ANSEL = 0b001;              // AN0 como entrada analógicas
    ANSELH = 0x00;              // I/O digitales
        
    TRISA = 0b00100001;         // SS y AN0 como entradas
                                // RA7 se conectará al SS (RA5) del esclavo
    TRISC = 0b00010000;         // SDI entrada, SCK y SD0 como salida
    TRISD = 0x00;               // PORTD como salida
    PORTA = 0x00;               // Limpieza del PORTA
    PORTC = 0x00;               // Limpieza del PORTC
    PORTD = 0x00;               // Limpieza del PORTD
    
    // Configuración de interrucpiones
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion de ADC
    INTCONbits.PEIE = 1;        // Habilitamos interrupciones de perifericos
    INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
    
    // Configuración ADC
    ADCON0bits.ADCS = 0b01;         // Fosc/8
    ADCON1bits.VCFG0 = 0;           // VDD
    ADCON1bits.VCFG1 = 0;           // VSS
    ADCON0bits.CHS = 0b0000;        // Selección de canal AN0
    ADCON1bits.ADFM = 0;            // Configuración de justificado a la izquierda
    ADCON0bits.ADON = 1;            // Habilitación del modulo ADC
    __delay_us(40);                 // Display de sample time
        
    // Configuración de SPI    
    // Configuración del MAESTRO    
    // SSPCON<5:0>
    SSPCONbits.SSPM = 0b0000;   // SPI Maestro, Reloj -> Fosc/4 (250kbits/s)
    SSPCONbits.CKP = 0;         // Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI (Para que no los utilice como RPG)
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;        // Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 1;        // Dato al final del pulso de reloj
    SSPBUF = LECTURA_POT;       // Enviamos un dato inicial (valor inicial de la variable)
}