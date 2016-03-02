/*
 * Controlador General via USB
 *
 * Auth: Héctor López
 * Remake
 *
 */
#include <msp430g2553.h>  //Header file para msp430g2553
#include <msp_serial_com.h> // Controlador Serial.
#include <string.h> // Libreria para manipulación de caracteres.

#ifndef TIMER0_A1_VECTOR   // Cambios de nomencaltura en vectores de interrupción
#define TIMER0_A1_VECTOR TIMERA1_VECTOR
#define TIMER0_A0_VECTOR TIMERA0_VECTOR
#endif


/* Definiciones */
#define RD_LED BIT0	 	// Renombrado leds
#define GR_LED BIT6
#define CMD_NUMBER 5 		// Numero de compandos a interpretar
#define TXLED BIT0 					// Led rojo para transmision
#define TXD BIT2					// pin de transmisión por hardware
#define RXD BIT1					// pin de recepción por hardware
#define COUNT_LED BIT6				// Led verde para conteos
#define COUNT_PIN BIT4				// P1.4 para entrada de conteos



void FaultRoutine(void);
void ConfigWDT(void);
void ConfigClocks(void);
void ConfigPINs(void);
void ConfigTimerA2(void);
void ConfigUART(void);
void GRN_LED_FUNC(char *);
void TEST_FUNC(char *);
int DecStrToInt(const char *String);
void EnviarConteos(void);
void COUNT_FUNC(char *);


/* Variables globales */
unsigned int GRL_STATUS = 0;
unsigned int BLK_STATUS = 0;
unsigned int CNT_STATUS = 0;
const int D_DELAY = 10000;		// Defaul delay.
unsigned int COUNT = 0;  			// mide el numero de interrupts en pin de conteo
unsigned int TX_BUFFER;				// almacena el valor a transmitir
unsigned int M_SEGUNDOS = 0;		// mide el número de milisegundos
unsigned int TX_BUFFER;				// almacena el valor a transmitir
int limit = 0;
int PULSE = 1;           			// indica que un pulso fue capturado
int TRANS = 0;


/* Commands table */
const command COMMAND_LIST[ CMD_NUMBER ] = {
		{"TF", TEST_FUNC, "TF n Period \t n={0|1}; Function for test purposes"},
		{"GL", GRN_LED_FUNC, "GL n\t \t n={0|1}; Turn off (0) or on (1) on board green led"},
		{"OT", Output_select, "OT n\t \t n={0|1}; Hide (0) or show (1) the text when writing"},
		{"CT", COUNT_FUNC, "CT n limit \t n={0|1}; Count until limit"},
		{"HL", Show_help, "HL\t \t Show commands help"},
};
const unsigned int CMD_NMR = CMD_NUMBER;

void main(void)
{
    ConfigWDT();
    ConfigClocks();
    ConfigPINs();
    ConfigUART();
    ConfigTimerA2();

    __bis_SR_register(GIE); // Enter LPM0 w/ int until Byte RXed

    while (1)
    {
    	if( RX_TRS ) Exec_Commands( );
    }
}


void ConfigUART(void)
{
    UCA0CTL1 |= UCSSEL_2; 			// SMCLK
	UCA0BR0 = 0x68; 				// 1MHz 9600
	UCA0BR1 = 0x00; 				// 1MHz 9600
    UCA0MCTL = UCBRS0;
    UCA0CTL1 &= ~UCSWRST; 			// inicializa USCI, /* USCI Software Reset */
    UC0IE |= UCA0RXIE;				// Enable USCI_A0 RX interrupt
}

void ConfigTimerA2(void)        // Configuracion de Timer
{
    TA1CCR0 = D_DELAY;   // DELAY
    TA1CTL = TASSEL_2 + MC_1 + ID_3;  // Timer controlado por SMCLK/1  + up to CCR0
    TA0CCR0 = D_DELAY;   // DELAY
    TA0CTL = TASSEL_2 + MC_1 + ID_3;  // Timer controlado por SMCLK/1  + up to CCR0
}

void ConfigWDT(void)
 {
	 WDTCTL = WDTPW + WDTHOLD;  // Stop watchdog timer
 }

void ConfigClocks(void)  // COnfiguración del clock
{
	if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF)
		FaultRoutine();  // If calibration data is erased
                                                //  run FaultRoutine()
	BCSCTL1 = CALBC1_1MHZ;  //Set range
    DCOCTL = CALDCO_1MHZ;   // Set DCO step + modulation
}

void FaultRoutine(void)  // En caso de rupturas de configuración
{
	P1OUT = BIT0 + BIT6;
	while(1);
}

void ConfigPINs(void) // Configuración de I/O pins
{
/* Inicializacion de salidas */
	P1DIR |= 0xFF;               	// all es salida en P1
	P1OUT &= 0x00;               	// Todas las salidas P1 en low
    P2DIR |= 0xFF; 					// all es salida en P2
    P2OUT &= 0x00; 					// Todas las salidas P2 en low

/* Interrupts en COUNT_PIN cuando el nivel pase alto a bajo */
   	P1DIR &= ~COUNT_PIN;			// Pin de conteo como entrada
   	P1IE |= COUNT_PIN;				// interrupt en pin de entrada
   	P1IES |= COUNT_PIN;				// configura interrupt en cambios
   	P1IFG &= ~COUNT_PIN;			// limpia el flag en el pin de entrada



/* Inicializacion de transmisión serial e interrupt de recepcion */
    P1SEL |= RXD + TXD ; 			// P1.1 = RXD, P1.2=TXD
    P1SEL2 |= RXD + TXD ; 			// P1.1 = RXD, P1.2=TXD
}


void GRN_LED_FUNC(char *arg)
{
	unsigned int CMD_OK = 0;
	switch( arg[0] )
	{
		case '0':
			if (GRL_STATUS)
			{
				P1OUT &= ~GR_LED;
				GRL_STATUS = 0;
				BLK_STATUS = 0;
				CNT_STATUS = 0;
				CMD_OK = 1;
			}
			else CMD_OK = 2;
			break;
		case '1':
			if (!GRL_STATUS)
			{
				P1OUT |= GR_LED;
				GRL_STATUS = 1;
				BLK_STATUS = 0;
				CNT_STATUS = 0;
				CMD_OK = 1;
			}
			else CMD_OK = 2;
			break;
	}
	switch ( CMD_OK )
	{
		case 0:
			PrintStr( "> Invalid argument\n" );
			break;
		case 1:
			PrintStr( "> OK\n" );
			break;
		case 2:
			PrintStr( "> No change\n" );
			break;
	}
}



void COUNT_FUNC(char *arg)
{
	unsigned int CMD_OK = 0;
	char *CONTEO;
	switch( arg[0])
	{
	case '0':
		if (CNT_STATUS)
		{
			CNT_STATUS = 0;
			TA1CCTL0 &= ~CCIE;
			CMD_OK = 1;
			P1OUT = 0;
			BLK_STATUS = 0;
			GRL_STATUS = 0;
			CNT_STATUS = 0;

		}else CMD_OK=2;
		break;
	case '1':
		if (!CNT_STATUS)
		{
			CONTEO = strtok( NULL, " ");
			limit = DecStrToInt( CONTEO );
			if ( limit )
			{
				PrintStr("\n");
				PrintStr("> Numero de cuentas: \n");
				PrintDec(limit);
				TA1CCTL0 |= CCIE;
				CMD_OK = 1;
				CNT_STATUS = 1;
				BLK_STATUS = 0;
				GRL_STATUS = 0;
			}else CMD_OK = 0;
		}else CMD_OK = 2;
		break;
	}
	switch ( CMD_OK ){
	case 0:
		PrintStr( "> Invalid argument\n" );
		break;
	case 1:
		PrintStr( "> OK\n" );
		break;
	case 2:
		PrintStr( "> No change\n" );
		break;
	}
}

void TEST_FUNC(char *arg){
	char *token;
	unsigned int Period;
	unsigned int CMD_OK = 0;
	switch( arg[0] ){
	case '0':
		if (BLK_STATUS){
			TA0CCTL0 &= ~CCIE;
			P1OUT = 0;
			BLK_STATUS = 0;
			GRL_STATUS = 0;
			CNT_STATUS = 0;
			CMD_OK = 1;
		} else CMD_OK = 2;
		break;
	case '1':
		if (!BLK_STATUS){
			token = strtok( NULL, " ");
			Period = DecStrToInt( token );
			if( Period ){
				TA0CCTL0 |= CCIE;
				TA0CCR0 = Period;
				BLK_STATUS = 1;
				GRL_STATUS = 0;
				CNT_STATUS = 0;
				CMD_OK = 1;
			} else CMD_OK = 0;
		}  else CMD_OK = 2;
		break;
	}
	switch ( CMD_OK ){
	case 0:
		PrintStr( "> Invalid argument\n" );
		break;
	case 1:
		PrintStr( "> OK\n" );
		break;
	case 2:
		PrintStr( "> No change\n" );
		break;
	}
}

/* Interrupt del COUNT_PIN */
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
 	COUNT++;						// Incrementa el contador
   	P1IFG &= ~COUNT_PIN;			// limpia el interrupt en el pin de conteo
}


#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0 (void)
{
	P1OUT ^= RD_LED;
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A1 (void)
{
	P1OUT ^= COUNT_LED; 	// Toggle P1.0
	M_SEGUNDOS++;
	TX_BUFFER++;
	if(M_SEGUNDOS >= 10)
	{
		//TX_BUFFER = COUNT;			// almacena de forma temporal el valor de cuenta
		COUNT = 0;					// reinicia el contador de pulsos
		PrintDec(TX_BUFFER);
		M_SEGUNDOS = 0;				// reinicia el contador de milisegundos
		limit--;
		if (limit < 1)
		{
			TA1CCTL0 &= ~CCIE;
			CNT_STATUS = 0;
			BLK_STATUS = 0;
			GRL_STATUS = 0;
		}
	}
}
