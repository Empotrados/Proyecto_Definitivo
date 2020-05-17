/*
 * Listado de los tipos de comandos empleados en la aplicación, así como definiciones de sus parámetros.
 * (TODO) Incluir aquí las respuestas a los comandos?
*/
#ifndef __USB_COMMANDS_TABLE_H
#define __USB_COMMANDS_TABLE_H

#include<stdint.h>

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
    COMANDO_TERMINAL,
    //etc, etc...
} commandTypes;

//Estructuras relacionadas con los parametros de los comandos. El estuadiante debera crear las
// estructuras adecuadas a los comandos usados, y asegurarse de su compatibilidad con el extremo Qt

//#pragma pack(1)   //Con esto consigo que el alineamiento de las estructuras en memoria del PC (32 bits) no tenga relleno.
//Con lo de abajo consigo que el alineamiento de las estructuras en memoria del microcontrolador no tenga relleno
#define PACKED __attribute__ ((packed))

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
    uint32_t valoresBrillo[3];
} PACKED PARAM_COMANDO_BRILLO;
typedef struct{
    uint8_t Vmodo;
}PACKED PARAM_COMANDO_MODO;

typedef struct{
    bool der;
    bool izq;
    uint8_t  sondeo_interrupcion;
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
    uint8_t cmd;
    uint8_t modo;
    uint8_t intesidad;
    uint8_t rgb[3];
    uint8_t led[2];
} PACKED PARAM_COMANDO_TERMINAL;

//#pragma pack()    //...Pero solo para los comandos que voy a intercambiar, no para el resto





#endif
