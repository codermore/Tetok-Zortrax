/* Proyecto Zortrax/tetok-bot */
/* Gabriel Maso & Nicolas Pua */
/**************************************************************************
 *                        Inclusión de bibliotecas                         *
 **************************************************************************/

#include <hidef.h> 
#include "derivative.h" 

/**************************************************************************
 *                        Definición de constantes                         *
 **************************************************************************/
typedef enum {
	BASE, HOMBRO, CODO
} motores;

typedef enum {
	CONFIGURAR, EN_MOVIMIENTO, STOP
} marchaMotores;

typedef enum {
	ATRAS, ADELANTE
} sentidoMotores;

typedef enum {
	ABIERTO, CERRADO
} pinza;

// En placa de izq a der: BASE - CODO - HOMBRO
//pololu base
#define DIR_BASE PTBD_PTBD5 
#define STEP_BASE TPM2C1V //PTB4
//Pololu CODO
#define DIR_CODO PTBD_PTBD6
#define STEP_CODO TPM2C0V //PTA1
#define PER_BASE_CODO TPM2MOD

//Pololu HOMBRO
#define DIR_HOMBRO PTBD_PTBD7
#define STEP_HOMBRO TPM1C0V //PTA0
#define PER_HOMBRO TPM1MOD

#define DATO_DISPONIBLE SCIS1_RDRF

/*Bluethooth Datos*/
#define DATOMAS 103
#define DATOMENOS 104
#define TOUCHUP 109
#define DATOBASE 100
#define DATOHOMBRO 101
#define DATOCODO 102
#define DATOAPERTURA 105
#define DATOCIERRE 106

/*Conteo datos*/
#define POSICION_CODO_FINAL 	6700
#define POSICION_CODO_CENTRO 	POSICION_CODO_FINAL/2
#define POSICION_CODO_INICIAL 	0

#define POSICION_BASE_FINAL 	11700
#define POSICION_BASE_CENTRO 	POSICION_BASE_FINAL/2
#define POSICION_BASE_INICIAL 	0

#define POSICION_HOMBRO_FINAL 	8020
#define POSICION_HOMBRO_CENTRO 	POSICION_BASE_FINAL/2
#define POSICION_HOMBRO_INICIAL 0

/* hola :) */

/************************************************************************** 
 *                        Declaración de funciones                         *
 **************************************************************************/
/*funciones de configuracion*/
void configPuertos(void);
void configTimer(byte, byte);
void configTPM(void);
void configSci(void);
void configIRQ(void);
byte recibirDato(void);
void transmitirDato(byte dato);

/*Uso personal*/
void correcionDeCarrera(marchaMotores *p_estadoMarchaMotores, marchaMotores *p_estadoAnteriorMarchaMotores);

/* Nuevas funciones de MEF */
void funcionPinza(byte, byte);
void incializarMefMotores(marchaMotores *p_estadoMarchaMotores);
void actualizarEstadoMotores(motores *p_estadoMotores, marchaMotores *p_estadoAnteriorMarchaMotores, marchaMotores *p_estadoMarchaMotores, sentidoMotores *p_estadoSentidoMotores);
void actualizarDato(motores *p_estadoMotores, marchaMotores *p_estadoMarchaMotores, pinza *p_estadoPinza, sentidoMotores *p_estadoSentidoMotores);
void actualizarPinza(pinza *p_estadoPinza);

/*Declaracion de variables*/

int posicionCodo = 0, posicionHombro = 0, posicionBase = 0, pasosCodo = 0,
		pasosHombro = 0, pasosBase = 0, contadorPinza = 0, c = 0, i;
char banderaSentido = 50, finalDeCarrera = 0, apert = 1, banderaPwmPinza = 1,
		motor = 1;

byte dato = 0;
motores estadoMotores;
marchaMotores estadoMarchaMotores;
marchaMotores estadoAnteriorMarchaMotores;
sentidoMotores estadoSentidoMotores;
pinza estadoPinza;

int tiempo = 0;

void main(void) {

	/************************************************************************
	 *                                Programa                               *
	 ************************************************************************/
	configPuertos();

	ICSTRM = 184;

	configSci();
	configTimer(125, 1); //medio-mS
	configTPM();
	configIRQ();
	c = 0;

	incializarMefMotores(&estadoMarchaMotores);

	while (c <= 4000)
		;

	while (1) {

		if (finalDeCarrera)
			correcionDeCarrera(&estadoMarchaMotores, &estadoAnteriorMarchaMotores);

		if (DATO_DISPONIBLE)
			actualizarDato(&estadoMotores, &estadoMarchaMotores, &estadoPinza, &estadoSentidoMotores);

		actualizarEstadoMotores(&estadoMotores, &estadoAnteriorMarchaMotores, &estadoMarchaMotores, &estadoSentidoMotores);
		actualizarPinza(&estadoPinza);

	} //while (1)
} //main

//* ----------------------------- Functions Prototypes ----------------------------- */

/*Funciones de configuracion*/
void configPuertos(void) {
	/************************************************************************
	 *                     Habilitación de interrupciones                    *
	 ************************************************************************/

	EnableInterrupts
	;

	/************************************************************************
	 *                    Configuración del micromotorador                 *
	 ************************************************************************/

	SOPT1 = 0x02; // Deshabilita el perro guardián.

	/************************************************************************
	 *                          Configuración puertos                        *
	 ************************************************************************/

	PTADD = 0x3f; // Configuro puertos como entrada/salida.
	PTBDD = 0xff;
	PTCDD = 0x00;
	PTCDD_PTCDD3 = 1;

	PTAPE = 0x00; // Habilito/deshabilito Pull ups de puertos.
	PTBPE = 0x00;
	PTCPE = 0x0f;

	PTAD = 0x00; // Limpio puertos.
	PTBD = 0x00;
	PTCD = 0x00;

}
void configIRQ(void) {
	IRQSC = 0b00110010;
	//           x <-- (0 por flanco/ 1 por nivel, funcionan ambos)
}
void configTimer(byte mod, byte toie) {

	MTIMSC = 0b00010000; //freno el timer
	MTIMCLK = 0b0000101; //Busclock/32 = aprox 1mS
	MTIMMOD = mod;       //Cuenta hasta 250.
	MTIMSC = 0b00000000; //Activo timer
	MTIMSC_TOIE = toie;  // Hab/deshabilito interrupciones del timer.
}
void configTPM(void) {
	TPM1SC = 0b00001110; // busclock/64
	TPM1MOD = 186; //Periodo=1mS
	TPM1C0SC = 0b00101000; //Elijomodo
	TPM1C0V = 0; //ciclo util = .5mS

	TPM2SC = 0b00001110; // busclock/64
	TPM2MOD = 124; //Periodo=1mS
	TPM2C0SC = 0b00101000; //Elijomodo
	TPM2C0V = 0; //ciclo util = .5mS

	TPM2SC = 0b00001110; // busclock/64
	TPM2MOD = 124; //Periodo=1mS
	TPM2C1SC = 0b00101000; //Elijomodo
	TPM2C1V = 0; //ciclo util = .5mS

}
void configSci(void) {

	// Configuro el módulo SCI. 

	SCIBDH = 0x00; // Des/habilito interrupción por LBKDIF ; Des/habilito interrupción por RXEDGIF
	SCIBDL = 0x34; /*Baud Rate = 9600bps*/

	SCIC1_LOOPS = 0; // TxD y RxD por separado.  
	SCIC1_SCISWAI = 0; // SCI continua en "Modo espera".   
	SCIC1_RSRC = 0; // Sin efecto cuando el bit LOOPS = 0; 
	SCIC1_M = 0; // Transmisión a 8 bits: start + 8 data bits (LSB first) + stop.  
	SCIC1_WAKE = 0; // Activación por línea inactiva.                 
	SCIC1_ILT = 0; // El recuento de bits comienza después del bit de inicio.
	SCIC1_PE = 0; // Con/sin paridad.             
	SCIC1_PT = 0; // Tipo de paridad (par o impar).                      

	SCIC2_TIE = 0; // Des/habilito interrupción por TDRE.
	SCIC2_TCIE = 0; // Des/habilito interrupción por TC.             
	SCIC2_RIE = 0; // Des/habilito interrupción por RDRF.  
	SCIC2_ILIE = 0; // Des/habilito interrupción por IDLE.             
	SCIC2_TE = 1; // Des/habilito transmisor.
	SCIC2_RE = 1; // Des/habilito receptor. 
	SCIC2_RWU = 0; // Receptor normal o en standby.
	SCIC2_SBK = 0; // Transmisor normal.

}
void transmitirDato(byte dato) { // Se ejecuta la transmisión de dato por encuesta

	byte bandera;

	while (SCIS1_TDRE == 0)
		;   // No finaliza ciclo hasta que esté libre el registro
	// en el que se guardará el dato que se enviará.
	bandera = SCIS1;	// Aclara la bandera.
	SCID = dato;         // Enviando dato. 
}
byte recibirDato(void) { // Se ejecuta la recepción de dato por encuesta.   
	byte dato_R;
	byte bandera;

	bandera = SCIS1;   // Aclara la Bandera. 
	dato_R = SCID;   // Se Salva el Dato. 

	return dato_R;   // Retorna Dato.
}

/*Funciones de MEF*/
void funcionPinza(byte tOn, byte tOff) {

	static char banderaCicloUtil = 1;

	if (banderaPwmPinza == 1) {
		contadorPinza = 0;
		PTBD_PTBD3 = 1;
		banderaPwmPinza = 0;
	}

	if (contadorPinza >= tOn && banderaCicloUtil == 1) {
		PTBD_PTBD3 = 0;
		contadorPinza = 0;
		banderaCicloUtil = 0;
	}

	if (contadorPinza >= tOff && banderaCicloUtil == 0) {
		banderaPwmPinza = 1;
		banderaCicloUtil = 1;
	}

}
void actualizarEstadoMotores(motores *p_estadoMotores,
		marchaMotores *p_estadoAnteriorMarchaMotores,
		marchaMotores *p_estadoMarchaMotores,
		sentidoMotores *p_estadoSentidoMotores) {
	switch (*p_estadoMarchaMotores) {
	case STOP:
		if (*p_estadoAnteriorMarchaMotores != STOP){
			STEP_BASE = 0;
			STEP_CODO = 0;
			STEP_HOMBRO = 0;
			*p_estadoAnteriorMarchaMotores = STOP;
		} else{
			/*nada*/
		}
		break;

	case CONFIGURAR:
		switch (*p_estadoSentidoMotores) {
		case ADELANTE:
			switch (*p_estadoMotores) {
			case BASE:
				DIR_BASE = ADELANTE;
				break;

			case HOMBRO:
				DIR_HOMBRO = ADELANTE;
				break;

			case CODO:
				DIR_CODO = ADELANTE;
				break;
			}
			break;

		case ATRAS:
			switch (*p_estadoMotores) {
			case BASE:
				DIR_BASE = ATRAS;
				break;

			case HOMBRO:
				DIR_HOMBRO = ATRAS;
				break;

			case CODO:
				DIR_CODO = ATRAS;
				break;
			}
			break;

			/* default */
		}
		*p_estadoAnteriorMarchaMotores = CONFIGURAR;
		*p_estadoMarchaMotores = EN_MOVIMIENTO;
		break;

	case EN_MOVIMIENTO:
		if (*p_estadoAnteriorMarchaMotores == CONFIGURAR) {
			switch (*p_estadoMotores) {
			case BASE:
				STEP_BASE = 62;
				break;

			case HOMBRO:
				STEP_HOMBRO = 93;
				break;

			case CODO:
				STEP_CODO = 62;
				break;
			}
			*p_estadoAnteriorMarchaMotores = EN_MOVIMIENTO;
		} else {
			/*conteo de pasos*/
			switch (*p_estadoMotores) {
			case BASE:
				if (TPM2C1SC_CH1F == 1) { //Bandera del TPM BASE
					if (*p_estadoSentidoMotores == ADELANTE)pasosBase++;
					if (*p_estadoSentidoMotores == ATRAS)pasosBase--;
					TPM2C1SC_CH1F = 0;
				}
				break;

			case HOMBRO:
				if (TPM1C0SC_CH0F == 1) { // Bandera del TPM Hombro
					if (*p_estadoSentidoMotores == ADELANTE)pasosHombro++;
					if (*p_estadoSentidoMotores == ATRAS)pasosHombro--;
					TPM1C0SC_CH0F = 0;
				}
				break;

			case CODO:
				if (TPM2C0SC_CH0F == 1) { //bandera del TPM CODO
					if (*p_estadoSentidoMotores == ADELANTE)pasosCodo++;
					if (*p_estadoSentidoMotores == ATRAS)pasosCodo--;
					TPM2C0SC_CH0F = 0;
				}
				break;
			}
		}
		break;
		/*default*/
	}
}
void incializarMefMotores(marchaMotores *p_estadoMarchaMotores) {
	*p_estadoMarchaMotores = STOP;
	DIR_CODO = ATRAS;
	DIR_HOMBRO = ATRAS;
	DIR_BASE = ATRAS;
}
void actualizarDato(motores *p_estadoMotores,
		marchaMotores *p_estadoMarchaMotores, pinza *p_estadoPinza,
		sentidoMotores *p_estadoSentidoMotores) {
	dato = recibirDato();
	switch (dato) {
	case DATOBASE:
		*p_estadoMotores = BASE;
		break;
	case DATOHOMBRO:
		*p_estadoMotores = HOMBRO;
		break;
	case DATOCODO:
		*p_estadoMotores = CODO;
		break;
	case DATOMAS:
		*p_estadoMarchaMotores = CONFIGURAR;
		*p_estadoSentidoMotores = ADELANTE;
		break;
	case DATOMENOS:
		*p_estadoMarchaMotores = CONFIGURAR;
		*p_estadoSentidoMotores = ATRAS;
		break;
	case TOUCHUP:
		*p_estadoMarchaMotores = STOP;
		break;
	case DATOAPERTURA:
		*p_estadoPinza = ABIERTO;
		break;
	case DATOCIERRE:
		*p_estadoPinza = CERRADO;
		break;

	}
}
void actualizarPinza(pinza *p_estadoPinza) {
	switch (*p_estadoPinza) {
	case ABIERTO:
		funcionPinza(5, 35);
		break;
	case CERRADO:
		funcionPinza(2, 38);
	}
}
void correcionDeCarrera(marchaMotores *p_estadoMarchaMotores, marchaMotores *p_estadoAnteriorMarchaMotores) {
	c = 0;
	while (c <= 200)
		; /*anti rebote*/

	if (PTCD == 0b00000100) { // CODO FC delantero
		pasosCodo = POSICION_CODO_FINAL;
		DIR_CODO = ATRAS;
		STEP_CODO = 62;

	} else if (PTCD == 0b00000011) { // CODO FC trasero
		pasosCodo = 0;
		DIR_CODO = ADELANTE;
		STEP_CODO = 62;

	} else if (PTCD == 0b00000001) { // HOMBRO FC trasero
		pasosHombro = POSICION_HOMBRO_FINAL;
		DIR_HOMBRO = ATRAS;
		STEP_HOMBRO = 93;

	} else if (PTCD == 0b00000101) { // HOMBRO FC delantero
		pasosHombro = 0;
		DIR_HOMBRO = ADELANTE;
		STEP_HOMBRO = 93;

	} else if (PTCD == 0b00000010) { //FC BASE IZQUIERDA
		pasosBase = 0;
		DIR_BASE = ADELANTE;
		STEP_BASE = 62;

	} else if (PTCD == 0b00000110) { //FC BASE DERECHA
		pasosBase = POSICION_BASE_FINAL;
		DIR_BASE = ATRAS;
		STEP_BASE = 62;
	}

	c = 0;
	while (c <= 600)
		; /*300ms*/

	finalDeCarrera = 0;
	*p_estadoAnteriorMarchaMotores = EN_MOVIMIENTO;
	*p_estadoMarchaMotores = STOP;

}

/*funcion de interrupciones */
__interrupt 2 void interr_IRQ(void) {

	estadoMarchaMotores = STOP;
	finalDeCarrera = 1;

	IRQSC_IRQACK = 1;

	/*discriminas de la sig manera:
	 * motor > (señal de final de carrera) > dir invertir > enviar pasos para que salga de interrupcion */

}

__interrupt 26 void interr_timer(void) {
	configTimer(125, 1); //re configuro el timer
	c++; //aumenta c en 1
	contadorPinza++;
}

