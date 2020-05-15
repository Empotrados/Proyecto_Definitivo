/*
 * Listado de los tipos de comandos empleados en la aplicación, así como definiciones de sus parámetros.
 * (TODO) Incluir aquí las respuestas a los comandos?
*/
#ifndef __USB_COMMANDS_TABLE_H
#define __USB_COMMANDS_TABLE_H

#include<stdint.h>

#pragma pack(1)	//Con esto consigo compatibilizar el alineamiento de las estructuras en memoria del PC (32 bits)
                // y del ARM (aunque, en este caso particular no haria falta ya que ambos son 32bit-Little Endian)

#define PACKED

//Codigos de los comandos. EL estudiante deberá definir los códigos para los comandos que vaya
// a crear y usar. Estos deberan ser compatibles con los usados en la parte Qt

typedef enum {
    COMANDO_NO_IMPLEMENTADO,
    COMANDO_PING,
    COMANDO_LEDS,
    COMANDO_BRILLO,
    COMANDO_BUTTONS,
    COMANDO_MODO,
    COMANDO_SONDEO,
    COMANDO_INTRUSION,
    COMANDO_TEMP,
    COMANDO_HORA,

    //etc, etc...
} commandTypes;


//Estructuras relacionadas con los parametros de los comandos. El estuadiante debera crear las
// estructuras adecuadas a los comandos usados, y asegurarse de su compatibilidad con el extremo Qt

typedef struct {
    uint8_t command;
} PACKED PARAM_COMANDO_NO_IMPLEMENTADO;

typedef union{
    struct {
        uint8_t fRed:1;
        uint8_t fGreen:1;
        uint8_t fBlue:1;
    } PACKED leds;
    uint8_t  ui8Valor;
} PACKED PARAM_COMANDO_LEDS;

typedef struct {
   uint32_t Brillo[3];
} PACKED PARAM_COMANDO_BRILLO;

typedef struct{
    uint8_t Vmodo;
}PACKED PARAM_COMANDO_MODO;

typedef union {
    struct {
        uint8_t fLeft:1;
        uint8_t fRight:1;
    } PACKED button;
    uint8_t  ui8Buttons;
} PACKED PARAM_COMANDO_BUTTONS;
typedef struct{
    uint32_t modo_int;
} PACKED PARAM_COMANDO_INTRUSION;
typedef struct{
    uint32_t modo_temp;
    uint32_t temperatura;
    uint32_t comprobar;
} PACKED PARAM_COMANDO_TEMP;

typedef struct{
    int hora;
    int minuto;
    int segundo;
} PACKED PARAM_COMANDO_HORA;

typedef struct{
    int led;
    int hora,minuto,segundo;
    int estado;
}PACKED programacion;





#pragma pack()    //...Pero solo para los comandos que voy a intercambiar, no para el resto





#endif
