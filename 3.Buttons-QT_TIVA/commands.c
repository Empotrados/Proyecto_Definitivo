//*****************************************************************************
//
// commands.c - FreeRTOS porting example on CCS4
//
// Este fichero contiene errores que seran explicados en clase
//
//*****************************************************************************


#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <usb_commands_table.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"        // FreeRTOS: definiciones relacionadas con grupos de eventos



/* Standard TIVA includes */
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"

#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"   // TIVA: Mapa de pines del chip

/* Other TIVA includes */
#include "utils/cpu_usage.h"
#include "utils/cmdline.h"
#include "utils/uartstdioMod.h"

#include "drivers/rgb.h"

extern  EventGroupHandle_t FlagsEventos;


#define Traza_FLAG 0x0001
QueueHandle_t cola_terminal;

extern portBASE_TYPE higherPriorityTaskWoken;


// ==============================================================================
// The CPU usage in percent, in 16.16 fixed point format.
// ==============================================================================
extern uint32_t g_ui32CPUUsage;

// ==============================================================================
// Implementa el comando cpu que muestra el uso de CPU
// ==============================================================================
static int  Cmd_cpu(int argc, char *argv[])
{
    //
    // Print some header text.
    //
    UARTprintf("ARM Cortex-M4F %u MHz - ",SysCtlClockGet() / 1000000);
    UARTprintf("%2u%% de uso\r\n", (g_ui32CPUUsage+32768) >> 16);

    // Return success.
    return(0);
}

// ==============================================================================
// Implementa el comando free, que muestra cuanta memoria "heap" le queda al FreeRTOS
// ==============================================================================
static int Cmd_free(int argc, char *argv[])
{
    //
    // Print some header text.
    //
    UARTprintf("%d bytes libres\r\n", xPortGetFreeHeapSize());

    // Return success.
    return(0);
}
static int Cmd_traza(int argc, char *argv[])
{
    if(argc != 2){
            UARTprintf("traza on//off \r\n");
        }else{
            if (0==strncmp( argv[1], "on",2)){
                xEventGroupSetBits(FlagsEventos,Traza_FLAG);
            }else{
                if (0==strncmp( argv[1], "off",3)){

                    xEventGroupClearBits(FlagsEventos,Traza_FLAG);
                }else{
                    UARTprintf("traza on//off \r\n");
                }
            }
        }

    // Return success.
    return(0);
}

static int Cmd_gpio_pwm(int argc, char *argv[])
{
    if(argc != 2){
            UARTprintf("mode gpio//pwm  \r\n");
        }else{
            PARAM_COMANDO_TERMINAL parametro;
            parametro.cmd=0;
            if (0==strncmp( argv[1], "gpio",2)){
                RGBDisable();
                GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
                parametro.modo=0;
            }else{
                if (0==strncmp( argv[1], "pwm",2)){
                    RGBEnable();
                    parametro.modo=1;
                }else{
                    UARTprintf("error de comando. gpio//pwm \r\n");
                }
            }
            xQueueSend (cola_terminal,&parametro,higherPriorityTaskWoken);   //Escribe en la cola freeRTOS

        }

    // Return success.
    return(0);
}
// ==============================================================================
// Implementa el comando "intensity"
// ==============================================================================
static int Cmd_intensity(int argc, char *argv[])
{

    if(argc !=2){
        //si los parametros no son suficientes, muestro ayuda
        UARTprintf(" intensity [X]\r\n");
    }else{
        PARAM_COMANDO_TERMINAL parametro;
        parametro.cmd = 1;
        float fIntensity = atof(argv[1]);
        parametro.intesidad = fIntensity;
        RGBIntensitySet(fIntensity);
        xQueueSend (cola_terminal,&parametro,higherPriorityTaskWoken);   //Escribe en la cola terminal
    }
return 0;
}

// ==============================================================================
// Implementa el comando "RGB"
// ==============================================================================
static int Cmd_rgb(int argc, char *argv[])
{
    uint32_t arrayRGB[3];

    if (argc != 4)
    {
        //Si los par�metros no son suficientes, muestro la ayuda
        UARTprintf(" rgb [red] [green] [blue]\r\n");
    }
    else
    {
        PARAM_COMANDO_TERMINAL parametro;
        parametro.cmd = 2;
        arrayRGB[0]=strtoul(argv[1], NULL, 10)<<8;
        arrayRGB[1]=strtoul(argv[2], NULL, 10)<<8;
        arrayRGB[2]=strtoul(argv[3], NULL, 10)<<8;

        if ((arrayRGB[0]>=65535)||(arrayRGB[1]>=65535)||(arrayRGB[2]>=65535))
        {

            UARTprintf(" \r\n");
        }
        else{
            RGBColorSet(arrayRGB);
            parametro.rgb[0] = arrayRGB[0];
            parametro.rgb[1] = arrayRGB[1];
            parametro.rgb[2] = arrayRGB[2];
            xQueueSend (cola_terminal,&parametro,higherPriorityTaskWoken);   //Escribe en la cola terminal
        }

    }
    return 0;
}

// ==============================================================================
// Implementa el comando "LED"
// ==============================================================================
static int Cmd_led(int argc, char *argv[])
{
    if (argc != 3){
        //Si los parametros no son suficientes o son demasiados, muestro la ayuda
        UARTprintf(" led [color] [on|off]\r\n");
    }else{
        PARAM_COMANDO_TERMINAL parametro;
        parametro.cmd = 3;

        if (0==strncmp( argv[2], "on",2))
        {
            if(0==strncmp( argv[1], "verde",5)){
                UARTprintf("LED verde on \r\n");
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3,GPIO_PIN_3);
                parametro.led[0] = 1;
                parametro.led[1] = 1;
            }
            if(0==strncmp( argv[1], "azul",4)){
                UARTprintf("LED azul on \r\n");
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,GPIO_PIN_2);
                parametro.led[0] = 2;
                parametro.led[1] = 1;
            }
            if(0==strncmp( argv[1], "rojo",4)){
                 UARTprintf("LED rojo on \r\n");
                 GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,GPIO_PIN_1);
                 parametro.led[0] = 0;
                 parametro.led[1] = 1;

            }
        }
        else if (0==strncmp( argv[2], "off",3))
        {
            if(0==strncmp( argv[1], "verde",5)){
                UARTprintf("LED verde off \r\n");
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3,0);
                parametro.led[0] = 1;
                parametro.led[1] = 0;
            }
            if(0==strncmp( argv[1], "azul",4)){
                UARTprintf("LED azul off \r\n");
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,0);
                parametro.led[0] = 2;
                parametro.led[1] = 0;
            }
            if(0==strncmp( argv[1], "rojo",4)){
                UARTprintf("LED rojo off \r\n");
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,0);
                parametro.led[0] = 0;
                parametro.led[1] = 0;
            }
        }
        else
        {
            //Si el parametro no es correcto, muestro la ayuda
            UARTprintf(" led [on|off]\r\n");
        }
        xQueueSend (cola_terminal,&parametro,higherPriorityTaskWoken); //Escribe en la cola terminal
    }
    return 0;
}

// ==============================================================================
// Implementa el comando task. S�lo es posible si la opci�n configUSE_TRACE_FACILITY de FreeRTOS est� habilitada
// ==============================================================================
#if ( configUSE_TRACE_FACILITY == 1 )

extern char *__stack;
static  int Cmd_tasks(int argc, char *argv[])
{
	char*	pcBuffer;
	uint8_t*	pi8Stack;
	portBASE_TYPE	x;
	
	pcBuffer = pvPortMalloc(1024);
	vTaskList(pcBuffer);
	UARTprintf("\t\t\t\tUnused\r\nTaskName\tStatus\tPri\tStack\tTask ID\r\n");
	UARTprintf("=======================================================\r\n");
	UARTprintf("%s", pcBuffer);
	
	// Calculate kernel stack usage
	x = 0;
	pi8Stack = (uint8_t *) &__stack;
	while (*pi8Stack++ == 0xA5)
	{
		x++;	//Esto s�lo funciona si hemos rellenado la pila del sistema con 0xA5 en el arranque
	}
	sprintf((char *) pcBuffer, "%%%us", configMAX_TASK_NAME_LEN);
	sprintf((char *) &pcBuffer[10], (const char *) pcBuffer, "kernel");
	UARTprintf("%s\t-\t*%u\t%u\t-\r\n", &pcBuffer[10], configKERNEL_INTERRUPT_PRIORITY, x/sizeof(portBASE_TYPE));
	vPortFree(pcBuffer);
	return 0;
}

#endif /* configUSE_TRACE_FACILITY */

#if configGENERATE_RUN_TIME_STATS
// ==============================================================================
// Implementa el comando "Stats"
// ==============================================================================
static Cmd_stats(int argc, char *argv[])
{
	char*	pBuffer;

	pBuffer = pvPortMalloc(1024);
	if (pBuffer)
	{
		vTaskGetRunTimeStats(pBuffer); //Es un poco inseguro, pero por ahora nos vale...
		UARTprintf("TaskName\tCycles\t\tPercent\r\n");
		UARTprintf("===============================================\r\r\n");
		UARTprintf("%s", pBuffer);
		vPortFree(pBuffer);
	}
	return 0;
}
#endif

// ==============================================================================
// Implementa el comando help
// ==============================================================================
static int Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    UARTprintf("Comandos disponibles\r\n");
    UARTprintf("------------------\r\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_psCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        UARTprintf("%s%s\r\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}


// ==============================================================================
// Tabla con los comandos y su descripcion. Si quiero anadir alguno, debo hacerlo aqui
// ==============================================================================
//Este array tiene que ser global porque es utilizado por la biblioteca cmdline.c para implementar el interprete de comandos
//No es muy elegante, pero es lo que ha implementado Texas Instruments.
tCmdLineEntry g_psCmdTable[] =
{
    { "help",     Cmd_help,      "     : Lista de comandos" },
    { "?",        Cmd_help,      "        : lo mismo que help" },
    { "cpu",      Cmd_cpu,       "      : Muestra el uso de  CPU " },
    { "free",     Cmd_free,      "     : Muestra la memoria libre" },
    { "traza",    Cmd_traza,     "    : Activa modo traza" },
    { "mode",    Cmd_gpio_pwm,     "    : cambia de modo" },
    { "intensity",Cmd_intensity,      "     : Cambia la intensidad de los leds" },
    { "rgb",      Cmd_rgb,       "      : Establece el color RGB de cada led" },
    { "led",      Cmd_led,       "      : Apaga y enciende en modo gpio el led deseado" },
#if ( configUSE_TRACE_FACILITY == 1 )
	{ "tasks",    Cmd_tasks,     "    : Muestra informacion de las tareas" },
#endif
#if (configGENERATE_RUN_TIME_STATS)
	{ "stats",    Cmd_stats,      "     : Muestra estadisticas de las tareas" },
#endif

    { 0, 0, 0 }
};

// ==============================================================================
// Tarea UARTTask.  Espera la llegada de comandos por el puerto serie y los ejecuta al recibirlos...
// ==============================================================================
static void vUARTTask( void *pvParameters )
{
    char    pcCmdBuf[64];
    int32_t i32Status;
	
    //
    // Mensaje de bienvenida inicial.
    //
    UARTprintf("\r\n\r\nWelcome to the TIVA FreeRTOS Demo!\r\n");
	UARTprintf("\r\n\r\n FreeRTOS %s \r\n",
		tskKERNEL_VERSION_NUMBER);
	UARTprintf("\r\n Teclee ? para ver la ayuda \r\n");
	UARTprintf("> ");    

	/* Loop forever */
	while (1)
	{

		/* Read data from the UART and process the command line */
		UARTgets(pcCmdBuf, sizeof(pcCmdBuf));
		if (strlen(pcCmdBuf) == 0)
		{
			UARTprintf("> ");
			continue;
		}

		//
		// Pass the line from the user to the command processor.  It will be
		// parsed and valid commands executed.
		//
		i32Status = CmdLineProcess(pcCmdBuf);

		//
		// Handle the case of bad command.
		//
		if(i32Status == CMDLINE_BAD_CMD)
		{
			UARTprintf("Comando erroneo!\r\n");	//No pongo acentos adrede
		}

		//
		// Handle the case of too many arguments.
		//
		else if(i32Status == CMDLINE_TOO_MANY_ARGS)
		{
			UARTprintf("El interprete de comandos no admite tantos parametros\r\n");	//El maximo, CMDLINE_MAX_ARGS, esta definido en cmdline.c
		}

		//
		// Otherwise the command was executed.  Print the error code if one was
		// returned.
		//
		else if(i32Status != 0)
		{
			UARTprintf("El comando devolvio el error %d\r\n",i32Status);
		}

		UARTprintf("> ");

	}
}




//
// Create la tarea que gestiona los comandos (definida en el fichero commands.c)
//
BaseType_t initCommandLine(uint16_t stack_size,uint8_t prioriry )
{

    // Inicializa la UARTy la configura a 115.200 bps, 8-N-1 .
    //se usa para mandar y recibir mensajes y comandos por el puerto serie
    // Mediante un programa terminal como gtkterm, putty, cutecom, etc...
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //Esta funcion habilita la interrupcion de la UART y le da la prioridad adecuada si esta activado el soporte para FreeRTOS
    UARTStdioConfig(0,115200,SysCtlClockGet());
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);   //La UART tiene que seguir funcionando aunque el micro esta dormido
    // vUARTTask era antes vCommandTask
    return xTaskCreate(vUARTTask, (portCHAR *)"UartComm", stack_size,NULL,prioriry, NULL);
}
