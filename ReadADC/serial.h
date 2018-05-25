// serial.h
// Functions available in the stm32f0discovery serial library
// Author: Frank Duignan
#define NEWLINE 0x0d
#define LINEFEED 0x0a

void initUART(int BaudRate);
int ReadCom(int Max,unsigned char *Buffer);
int WriteCom(int Count,unsigned char *Buffer);
int eputs(char *String);
int egets(char *String, int size);
