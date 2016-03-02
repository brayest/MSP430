/*
 * msp_serial_com.h
 *
 *  Created on: Feb 18, 2016
 *      Author: Hector
 */

#ifndef MSP_SERIAL_COM_H_
#define MSP_SERIAL_COM_H_

#define TXD BIT2
#define RXD BIT1
#define TX_BUF_SIZE 5
#define RX_BUF_SIZE 10

/* Type defs */
typedef struct {
    const char* cmd_name;
    void (*cmd_func)(char*);
    const char* cmd_help;
} command;


/* Flags definitions */
extern volatile unsigned int RX_TRS;

/* functions declarations */
void PrintStr(const char*);
void PrintHex(const unsigned int);
void PrintDec(const unsigned int);
void Exec_Commands(void);
void Show_help(char*);
void Output_select(char *);


#endif /* MSP_SERIAL_COM_H_ */
