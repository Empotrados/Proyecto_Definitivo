//*****************************************************************************
//
// Codigo de partida comunicacion TIVA-QT (Marzo2020)
// Autores: Eva Gonzalez, Ignacio Herrero, Jose Manuel Cano
//
//  Estructura de aplicacion basica para el desarrollo de aplicaciones genericas
//  basada en la TIVA, en las que existe un intercambio de comandos con un interfaz
//  grÃ¡fico (GUI) Qt.
//  La aplicacion se basa en un intercambio de comandos con ordenes e informacion, a traves  de la
//  configuracion de un perfil CDC de USB (emulacion de puerto serie) y un protocolo
//  de comunicacion con el PC que permite recibir ciertas ordenes y enviar determinados datos en respuesta.
//   En el ejemplo basico de partida se implementara la recepcion de un comando
//  generico que permite el apagado y encendido de los LEDs de la placa; asi como un segundo
//  comando enviado desde la placa al GUI, para mostrar el estado de los botones.
//
//*****************************************************************************


#include<stdbool.h>
#include<stdint.h>
#include "inc/hw_memmap.h"       // TIVA: Definiciones del mapa de memoria
#include "inc/hw_types.h"        // TIVA: Definiciones API
#include "inc/hw_ints.h"         // TIVA: Definiciones para configuracion de interrupciones
#include "driverlib/gpio.h"      // TIVA: Funciones API de GPIO
#include "driverlib/pin_map.h"   // TIVA: Mapa de pines del chip
#include "driverlib/rom.h"       // TIVA: Funciones API incluidas en ROM de micro (MAP_)
#include "driverlib/rom_map.h"   // TIVA: Mapeo automatico de funciones API incluidas en ROM de micro (MAP_)
#include "driverlib/sysctl.h"    // TIVA: Funciones API control del sistema
#include "driverlib/uart.h"      // TIVA: Funciones API manejo UART
#include "driverlib/interrupt.h" // TIVA: Funciones API manejo de interrupciones
#include "driverlib/adc.h"       // TIVA: Funciones API manejo de ADC
#include "driverlib/timer.h"     // TIVA: Funciones API manejo de timers
#include <utils/uartstdioMod.h>     // TIVA: Funciones API UARTSTDIO (printf)
#include "drivers/buttons.h"     // TIVA: Funciones API manejo de botones
#include "drivers/rgb.h"         // TIVA: Funciones API manejo de leds con PWM
#include "FreeRTOS.h"            // FreeRTOS: definiciones generales
#include "task.h"                // FreeRTOS: definiciones relacionadas con tareas
#include "semphr.h"              // FreeRTOS: definiciones relacionadas con semaforos
#include "queue.h"               // FreeRTOS: definiciones relacionadas con colas de mensajes
#include "utils/cpu_usage.h"
#include "commands.h"
#include <serial2USBprotocol.h>
#include <usb_dev_serial.h>
#include <usb_commands_table.h>
#include <timers.h>
#include "event_groups.h"        // FreeRTOS: definiciones relacionadas con grupos de eventos

#define LED1TASKPRIO 1
#define LED1TASKSTACKSIZE 128

EventGroupHandle_t FlagsEventos;

#define Traza_FLAG 0x0001
#define Sondeo_Flag 0x002
#define Interrupcion_Flag 0x004
#define izquierda 0x008
#define derecha 0x010
#define izquierda_OFF 0x020
#define derecha_OFF 0x040

uint32_t ui32Period;

// Variables globales "main"
uint32_t g_ui32CPUUsage;
uint32_t g_ui32SystemClock;

SemaphoreHandle_t mutexUSB;
SemaphoreHandle_t miSemaforo = NULL,semaforoEnergia;
xQueueHandle cola_freertos;
xQueueHandle cola_temperatura;
xQueueHandle cola_hora;
xQueueHandle cola_terminal;


TimerHandle_t xTimer;

signed portBASE_TYPE higherPriorityTaskWoken=pdFALSE;   //Hay que inicializarlo a False!!

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}

#endif

//*****************************************************************************
//
// Aqui incluimos los "ganchos" a los diferentes eventos del FreeRTOS
//
//*****************************************************************************

//Esto es lo que se ejecuta cuando el sistema detecta un desbordamiento de pila
//
void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
	//
	// This function can not return, so loop forever.  Interrupts are disabled
	// on entry to this function, so no processor interrupts will interrupt
	// this loop.
	//
	while(1)
	{
	}
}

//Esto se ejecuta cada Tick del sistema. LLeva la estadistica de uso de la CPU (tiempo que la CPU ha estado funcionando)
void vApplicationTickHook( void )
{
	static uint8_t ui8Count = 0;

	if (++ui8Count == 10)
	{
		g_ui32CPUUsage = CPUUsageTick();
		ui8Count = 0;
	}
	//return;
}

//Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationIdleHook (void)
{
	SysCtlSleep();
}


//Esto se ejecuta cada vez que se produce un fallo de asignacio de heap
void vApplicationMallocFailedHook (void)
{
	while(1);
}
void vTimerCallback( TimerHandle_t pxTimer )
{

    xSemaphoreGive(miSemaforo);
}


//*****************************************************************************
//
// A continuacion van las tareas...
//
//*****************************************************************************

//// Codigo para procesar los comandos recibidos a traves del canal USB

static portTASK_FUNCTION( CommandProcessingTask, pvParameters ){

    uint8_t pui8Frame[MAX_FRAME_SIZE];	//Ojo, esto hace que esta tarea necesite bastante pila
    int32_t i32Numdatos;
    uint8_t ui8Command;
    void *ptrtoreceivedparam; // <---?
	uint32_t ui32Errors=0;


	/* The parameters are not used. */
	( void ) pvParameters;

	//
	// Mensaje de bienvenida inicial.
	//
	UARTprintf("\n\nBienvenido a la aplicacion XXXXX (curso 2019/20)!\n");
	UARTprintf("\nAutores: XXXXXX y XXXXX ");

	for(;;)
	{
        //Espera hasta que se reciba una trama con datos serializados por el interfaz USB
		i32Numdatos=receive_frame(pui8Frame,MAX_FRAME_SIZE); //Esta funcion es bloqueante
		if (i32Numdatos>0)
		{	//Si no hay error, proceso la trama que ha llegado.
			i32Numdatos=destuff_and_check_checksum(pui8Frame,i32Numdatos); // Primero, "destuffing" y comprobaciÃ³n checksum
			if (i32Numdatos<0)
			{
				//Error de checksum (PROT_ERROR_BAD_CHECKSUM), ignorar el paquete
				ui32Errors++;
                // Procesamiento del error (TODO, POR HACER!!)
			}
			else
			{
				//El paquete esta bien, luego procedo a tratarlo.
                //Obtiene el valor del campo comando
				ui8Command=decode_command_type(pui8Frame);
                //Obtiene un puntero al campo de parametros y su tamanio.
                i32Numdatos=get_command_param_pointer(pui8Frame,i32Numdatos,&ptrtoreceivedparam);
				switch(ui8Command)
				{
				case COMANDO_PING :
					//A un comando de ping se responde con el propio comando
					i32Numdatos=create_frame(pui8Frame,ui8Command,0,0,MAX_FRAME_SIZE);
					if (i32Numdatos>=0)
					{
						send_frame(pui8Frame,i32Numdatos);
					}else{
						//Error de creacion de trama: determinar el error y abortar operacion
						ui32Errors++;
						// Procesamiento del error (TODO)
//						// Esto de aqui abajo podria ir en una funcion "createFrameError(numdatos)  para evitar
//						// tener que copiar y pegar todo en cada operacion de creacion de paquete
						switch(i32Numdatos){
						case PROT_ERROR_NOMEM:
							// Procesamiento del error NO MEMORY (TODO)
							break;
						case PROT_ERROR_STUFFED_FRAME_TOO_LONG:
//							// Procesamiento del error STUFFED_FRAME_TOO_LONG (TODO)
							break;
						case PROT_ERROR_COMMAND_TOO_LONG:
//							// Procesamiento del error COMMAND TOO LONG (TODO)
							break;
						}
                        case PROT_ERROR_INCORRECT_PARAM_SIZE:
                        {
                            // Procesamiento del error INCORRECT PARAM SIZE (TODO)
                        }
                        break;
					}
					break;
				case COMANDO_LEDS: //comando que cambia los leds en modo GPIO
				{
				    PARAM_COMANDO_LEDS parametro;
				    if (check_and_extract_command_param(ptrtoreceivedparam, i32Numdatos, sizeof(parametro),&parametro)>0)
				    {
				        if(parametro.leds.led[0]==1){
				        if(parametro.leds.fRed==1){ //si el check esta activado, encenderemos el led

                               GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,GPIO_PIN_1); //encendemos led ROJO en modo gpio
                       }else{ //el check esta desactivado y apagamos el led
                           GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,0);//apaga el led ROJO
                       }
				        }
				        if(parametro.leds.led[1]==1){
                       if(parametro.leds.fGreen==1){
                               GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3,GPIO_PIN_3);
                       }else{
                           GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3,0);
                       }
				        }
				        if(parametro.leds.led[2]==1){
                       if(parametro.leds.fBlue==1){

                               GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,GPIO_PIN_2);
                       }else{
                           GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,0);
                       }
				        }
				    }else//Error de tamaÃ±o de parÃ¡metro
				    ui32Errors++; // Tratamiento del error
				}
				break;
				case COMANDO_BRILLO: //comando que cambia los leds en modo PWM
				{
				    PARAM_COMANDO_BRILLO parametro;

				    if (check_and_extract_command_param(ptrtoreceivedparam, i32Numdatos, sizeof(parametro),&parametro)>0){
				        RGBColorSet(parametro.valoresBrillo);//enciende los led segun los valores que le llega n en el array
				        if(xEventGroupWaitBits(FlagsEventos,Traza_FLAG,pdFALSE,pdFALSE,0 * configTICK_RATE_HZ)==(Traza_FLAG)){
                           UARTprintf(" rgb %d %d %d \r\n",parametro.valoresBrillo[0],parametro.valoresBrillo[1],parametro.valoresBrillo[2]);
                        }//escribimos si estamos en modo traza
				    }else//Error de tamaño de parametro
				    ui32Errors++; // Tratamiento del error
				}
				break;
				case COMANDO_MODO: //nos notifica si estamos en modo GPIO o PWM
				{
				PARAM_COMANDO_MODO parametro;
                uint8_t modo;
                uint32_t apagado[3]={0,0,0};
                if (check_and_extract_command_param(ptrtoreceivedparam, i32Numdatos, sizeof(parametro),&parametro)>0){
                    modo= parametro.Vmodo;
                    if(modo==0){//si modo es GPIO...
                        RGBColorSet(apagado);//apagamos los led
                        RGBDisable();//quitamos el modo pwm
                        GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//activamos gpio

                    }else{
                    RGBEnable();//activamos modo pwm
                    }
                }else//Error de tamaño de parametro
                ui32Errors++; // Tratamiento del error
				}
				break;
			    case COMANDO_INTRUSION: //nos llega un aviso cuando cambiamos de modo en presencia-intrusion
                {
                    PARAM_COMANDO_INTRUSION parametro;
                    if (check_and_extract_command_param(ptrtoreceivedparam, i32Numdatos, sizeof(parametro),&parametro)>0)
                    if(parametro.modo_int==0){//Si estamos en modo presencia(sondeo)...

                         MAP_GPIOIntDisable(GPIO_PORTF_BASE,ALL_BUTTONS);
                         MAP_IntDisable(INT_GPIOF);//deshabilitamos interrupciones del GPIO

                         if(xEventGroupWaitBits(FlagsEventos,Traza_FLAG,pdFALSE,pdFALSE,0 * configTICK_RATE_HZ)==(Traza_FLAG)){
                          UARTprintf("cambiamos a modo presencia  \r\n");//modo traza
                        }

                      }else{//estamos en modo intrusion(interrupciones)
                          if(xEventGroupWaitBits(FlagsEventos,Traza_FLAG,pdFALSE,pdFALSE,0 * configTICK_RATE_HZ)==(Traza_FLAG)){
                             UARTprintf("cambiamos a modo intrusion  \r\n");//modo traza
                           }
                            MAP_GPIOIntEnable(GPIO_PORTF_BASE,ALL_BUTTONS);
                            MAP_IntEnable(INT_GPIOF);//habilitamos interrupciones del GPIO


                            }
                }
                break;
				case COMANDO_SONDEO://nos llega un aviso de que queremos comprobar los botones en modo presencia
				{

				    int32_t i32Status = MAP_GPIOPinRead(GPIO_PORTF_BASE,ALL_BUTTONS);

                    if ((i32Status & LEFT_BUTTON)==0)//boton izq pulsado
                        {
                            //Activa los flags
                            xEventGroupSetBits(FlagsEventos, izquierda );
                        }

                    if ((i32Status & RIGHT_BUTTON)==0){//boton derecho pulsado
                            //Activa los flags
                            xEventGroupSetBits(FlagsEventos, derecha );
                        }

                    if ((i32Status & RIGHT_BUTTON)==RIGHT_BUTTON){//boton derecho no pulsado
                              //Activa los flags
                              xEventGroupSetBits(FlagsEventos, derecha_OFF  );
                          }
                    if ((i32Status & LEFT_BUTTON)==LEFT_BUTTON)//boton izq no pulsado
                        {
                            //Activa los flags
                            xEventGroupSetBits(FlagsEventos, izquierda_OFF );
                        }
                    xEventGroupSetBits(FlagsEventos, Sondeo_Flag );//ativo flag de sondeo

				}break;

			    case COMANDO_TEMP: //comando que llega cuando cambias de modo en temperatura o le das a comprobar
                {
                    PARAM_COMANDO_TEMP parametro;
                    if (check_and_extract_command_param(ptrtoreceivedparam, i32Numdatos, sizeof(parametro),&parametro)>0)
                    if (parametro.modo_temp ==0){//modo periodico
                        TimerEnable(TIMER2_BASE, TIMER_A);//habilito el timer2
                    }else{     //modo manual
                        TimerDisable(TIMER2_BASE, TIMER_A);//deshabilito el timer2
                        if(parametro.comprobar==1){ //si hemos pulsado el boton de comprobar...
                            parametro.comprobar=0; //actualizamos el parametro
                            ADCProcessorTrigger(ADC1_BASE, 1); //usamos adc1 para obtener la temperatura
                        }
                    }
                }break;
			    case COMANDO_HORA: //Te llega cuando pulsas upload en qt
			    {
			        PARAM_COMANDO_HORA parametro;
                    if (check_and_extract_command_param(ptrtoreceivedparam, i32Numdatos, sizeof(parametro),&parametro)>0)

                        xQueueSend(cola_hora,&parametro,portMAX_DELAY);//mandamos el parametro con la hora a la cola
			    }break;


				default:
				{
				    PARAM_COMANDO_NO_IMPLEMENTADO parametro;
					parametro.command=ui8Command;
					//El comando esta bien pero no esta implementado
					i32Numdatos=create_frame(pui8Frame,COMANDO_NO_IMPLEMENTADO,&parametro,sizeof(parametro),MAX_FRAME_SIZE);
					if (i32Numdatos>=0)
					{
						send_frame(pui8Frame,i32Numdatos);
					}
					break;
				 }
				}// switch
			}
		}else{ // if (ui32Numdatos >0)
			//Error de recepcion de trama(PROT_ERROR_RX_FRAME_TOO_LONG), ignorar el paquete
			ui32Errors++;
			// Procesamiento del error (TODO)
		}
	}
}


// Codigo para enviar los botones pulsados
static portTASK_FUNCTION( ButtonsTask, pvParameters ){

    uint8_t pui8Frame[MAX_FRAME_SIZE];  //Ojo, esto hace que esta tarea necesite bastante pila
    PARAM_COMANDO_BUTTONS parametro;
    int32_t i32Numdatos;
    int32_t xEventGroupValue;

    while(1){
        //miramos si hay algun flag activado...
        xEventGroupValue=xEventGroupWaitBits(FlagsEventos,Interrupcion_Flag|Sondeo_Flag,pdFALSE,pdFALSE,portMAX_DELAY);
        if( ( xEventGroupValue & Sondeo_Flag ) == Sondeo_Flag ){//viene de sondeo
                    parametro.sondeo_interrupcion =0; //modo presencia
                    xEventGroupClearBits(FlagsEventos,Sondeo_Flag); //limpiamos flag de presencia
                }
                if( ( xEventGroupValue & Interrupcion_Flag ) ==Interrupcion_Flag){
                    parametro.sondeo_interrupcion =1; //modo intrusion
                    xEventGroupClearBits(FlagsEventos,Interrupcion_Flag);
                }

        if( ( xEventGroupValue & izquierda ) ==izquierda ){ //comprabamos si el flag izquierda es el activado
            parametro.izq= true; //notifica a qt si esta pulsado(true) o NO pulsado(false)

            xEventGroupClearBits(FlagsEventos,izquierda); //limpiamos el flag que hemos usado
        }
        if( ( xEventGroupValue & derecha ) ==derecha ){
            parametro.der=true;

            xEventGroupClearBits(FlagsEventos,derecha);
        }
        if( ( xEventGroupValue & derecha_OFF ) ==derecha_OFF ){
                parametro.der=false;

                xEventGroupClearBits(FlagsEventos,derecha_OFF);
        }
        if( ( xEventGroupValue & izquierda_OFF ) ==izquierda_OFF ){
                parametro.izq= false;

                xEventGroupClearBits(FlagsEventos,izquierda_OFF);
        }

        //envio de datos a qt
        i32Numdatos=create_frame(pui8Frame,COMANDO_BUTTONS,&parametro,sizeof(parametro),MAX_FRAME_SIZE);
        if (i32Numdatos>=0)
        {
            xSemaphoreTake(mutexUSB,portMAX_DELAY);
            send_frame(pui8Frame,i32Numdatos);
            xSemaphoreGive(mutexUSB);
        }

    }//fin del while
}


// Codigo para enviar la temperatura
static portTASK_FUNCTION( TempTask, pvParameters ){
    PARAM_COMANDO_TEMP parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];  //Ojo, esto hace que esta tarea necesite bastante pila
    int32_t i32Numdatos;
    while(1)
       { //si recibimos datos(temperatura) de la cola...
           if (xQueueReceive(cola_temperatura,&parametro.temperatura,portMAX_DELAY)==pdTRUE)
           { //los enviamos a qt
               i32Numdatos=create_frame(pui8Frame,COMANDO_TEMP,&parametro,sizeof(parametro),MAX_FRAME_SIZE);
                          if (i32Numdatos>=0)
                          {
                              xSemaphoreTake(mutexUSB,portMAX_DELAY);
                              send_frame(pui8Frame,i32Numdatos);
                              xSemaphoreGive(mutexUSB);
                          }

           }
           if(xEventGroupWaitBits(FlagsEventos,Traza_FLAG,pdFALSE,pdFALSE,0 * configTICK_RATE_HZ)==(Traza_FLAG)){
               UARTprintf("temperatura %d �C \r\n",parametro.temperatura); //modo traza
           }

       }
}
static portTASK_FUNCTION( horaTask, pvParameters ){
    PARAM_COMANDO_HORA parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];  //Ojo, esto hace que esta tarea necesite bastante pila
    int32_t i32Numdatos;
    if (xQueueReceive(cola_hora,&parametro,portMAX_DELAY)==pdTRUE) //recibimos datos (hora)
     {
        xTimer = xTimerCreate("TimerSW", 1 * configTICK_RATE_HZ,pdTRUE,NULL,vTimerCallback); //creamos timer SW de 1s
                if( xTimer == NULL ){
                    /* The timer was not created. */
                    while(1);
                }
        if( xTimerStart( xTimer, 0 ) != pdPASS ){
         /* The timer could not be set into the Active state.*/

            while(1);
        }
        while(1){
            xSemaphoreTake(miSemaforo,portMAX_DELAY);//esperamos con un semaforo que es activado por el timer SW
            parametro.segundo++; //actualizamos todos los datos del parametro
            if(parametro.segundo==60){
                parametro.segundo=0;
                parametro.minuto++;
                if(parametro.minuto==60){
                    parametro.minuto=0;
                    parametro.hora++;
                    if(parametro.hora==24){
                        parametro.hora=0;
                    }
                }
            }
            //enviamos a QT..
            i32Numdatos=create_frame(pui8Frame,COMANDO_HORA,&parametro,sizeof(parametro),MAX_FRAME_SIZE);
              if (i32Numdatos>=0)
              {
                  xSemaphoreTake(mutexUSB,portMAX_DELAY);
                  send_frame(pui8Frame,i32Numdatos);
                  xSemaphoreGive(mutexUSB);
              }


        }

      }
}
// Codigo para enviar los cambios realizados por terminal
static portTASK_FUNCTION( TerminalTask, pvParameters ){
    PARAM_COMANDO_TERMINAL parametro;
    uint8_t pui8Frame[MAX_FRAME_SIZE];  //Ojo, esto hace que esta tarea necesite bastante pila
    int32_t i32Numdatos;
    while(1){ //Si recibimos de la cola que viene de los comandos del terminal...
        if (xQueueReceive(cola_terminal,&parametro,portMAX_DELAY)==pdTRUE){
            i32Numdatos=create_frame(pui8Frame,COMANDO_TERMINAL,&parametro,sizeof(parametro),MAX_FRAME_SIZE);
          if (i32Numdatos>=0)
          {
              xSemaphoreTake(mutexUSB,portMAX_DELAY);  //los enviamos a QT
              send_frame(pui8Frame,i32Numdatos);
              xSemaphoreGive(mutexUSB);
          }
        }
    }

}

//*****************************************************************************
//
// Funcion main(), Inicializa los perifericos, crea las tareas, etc... y arranca el bucle del sistema
//
//*****************************************************************************
int main(void)
{

	//
	// Set the clocking to run at 40 MHz from the PLL.
	//
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
			SYSCTL_OSC_MAIN);	//Ponermos el reloj principal a 50 MHz (200 Mhz del Pll dividido por 4)


	// Get the system clock speed.
	g_ui32SystemClock = SysCtlClockGet();


	//Habilita el clock gating de los perifericos durante el bajo consumo --> perifericos que se desee activos en modo Sleep
	//                                                                        deben habilitarse con SysCtlPeripheralSleepEnable
	MAP_SysCtlPeripheralClockGating(true);

	// Inicializa el subsistema de medida del uso de CPU (mide el tiempo que la CPU no esta dormida)
	// Para eso utiliza un timer, que aqui hemos puesto que sea el TIMER3 (ultimo parametro que se pasa a la funcion)
	// (y por tanto este no se deberia utilizar para otra cosa).
	CPUUsageInit(g_ui32SystemClock, configTICK_RATE_HZ/10, 3);
    //Inicializa el puerto F (LEDs)
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); //LEDS APAGADOS

    //Inicializa la biblioteca RGB (sin configurar las salidas como RGB)
    RGBInit(1);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);  //Esto es necesario para que el timer0 siga funcionando en bajo consumo
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);  //Esto es necesario para que el timer1 siga funcionando en bajo consumo

    //Inicializa los botones (tambien en el puerto F) y habilita sus interrupciones
    ButtonsInit();
    MAP_GPIOIntTypeSet(GPIO_PORTF_BASE, ALL_BUTTONS,GPIO_BOTH_EDGES);
    MAP_IntPrioritySet(INT_GPIOF,configMAX_SYSCALL_INTERRUPT_PRIORITY);// Misma prioridad que configMAX_SYSCALL_INTERRUPT_PRIORITY
    // Una prioridad menor (mayor numero) podria dar problemas si la interrupcion
    // ejecuta llamadas a funciones de FreeRTOS


    // CONFIGURACION TIMER2
    // Habilita periferico Timer2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER2);
    // Configura el Timer2 para cuenta periodica de 32 bits (no lo separa en TIMER0A y TIMER0B)
    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
    // Periodo de cuenta de 2s. SysCtlClockGet() te proporciona la frecuencia del reloj del sistema, por lo que una cuenta
    // del Timer a SysCtlClockGet() tardara 1 segundo * 2 = 2 sg de periodo
    ui32Period = (SysCtlClockGet()*2);
    // Carga la cuenta en el Timer1A
    TimerLoadSet(TIMER2_BASE, TIMER_A, ui32Period -1);/*no entendemos este -1*/

    //CONFIGURACION ADC
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);   // Habilita ADC1
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC1);
    ADCSequenceDisable(ADC1_BASE, 0); // Deshabilita el secuenciador 0 del ADC1 para su configuracion
    // Tasa de muestreo es 1Msps por defecto. Se puede cambiar con ADCClockConfigSet (eligiendo un reloj para el ADC)
    // Disparo de muestreo por instrucciones de timer
    //ADCSequenceConfigure(ADCX_BASE,SSX,DISPARO,PRIORIDAD)
    ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_TIMER, 0);//configuramos el adc para que el sequenciador 0 del adc1 este controlado por timer
    ADCSequenceStepConfigure(ADC1_BASE, 0, 0,ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);//configuro el paso 0 de sequenciador 0 del adc1 mire por el canal 1 por el sensor de temperatura que genere interrupcion y qe es el paso final
    TimerControlTrigger(TIMER2_BASE,TIMER_A,true);//para configurar que el disparo del adc sea controlado por el timer 2
    ADCHardwareOversampleConfigure(ADC1_BASE, 64);/*promediado hardware de 64*/
    IntEnable( INT_ADC1SS0);//habilito interrupcion del sequencer 0
    // Tras configurar el secuenciador, se vuelve a habilitar
    ADCSequenceEnable(ADC1_BASE, 0);//ya configurada habilito sequencia 0 del adc1
    ADCIntEnable(ADC1_BASE, 0);
    //IntMasterEnable();


    //CONFIGURACION ADC por SW

        ADCSequenceDisable(ADC1_BASE, 1); // Deshabilita el secuenciador 1 del ADC1 para su configuracion
        // Tasa de muestreo es 1Msps por defecto. Se puede cambiar con ADCClockConfigSet (eligiendo un reloj para el ADC)
        // Disparo de muestreo por instrucciones de timer
        //ADCSequenceConfigure(ADCX_BASE,SSX,DISPARO,PRIORIDAD)
        ADCSequenceConfigure(ADC1_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);//configuramos el adc para que el sequenciador 0 del adc1 este controlado por timer

        ADCSequenceStepConfigure(ADC1_BASE, 1, 0,ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);//configuro el paso 0 de sequenciador 0 del adc1 mire por el canal 1 por el sensor de temperatura que genere interrupcion y qe es el paso final

        IntEnable( INT_ADC1SS1);//habilito interrupcion del sequencer 1
        // Tras configurar el secuenciador, se vuelve a habilitar
        ADCSequenceEnable(ADC1_BASE, 1);//ya configurada habilito sequencia 1 del adc1
        ADCIntEnable(ADC1_BASE, 1);





    /**      Creacion de recursos IPC               **/
    // Cola para el envio del estado del puertoF (botones)
    cola_freertos=xQueueCreate(3,sizeof(int32_t));  //espacio para 3items de tam ulong
    if (NULL==cola_freertos)
        while(1);
    // Cola para el envio de la temperatura
        cola_temperatura=xQueueCreate(3,sizeof(uint32_t));  //espacio para 3items de tamaÃ±o ulong
        if (NULL==cola_temperatura)
            while(1);
    // Cola para el envio de la hora
        cola_hora=xQueueCreate(3,sizeof(PARAM_COMANDO_HORA));  //espacio para 3items de tamaÃ±o ulong
        if (NULL==cola_hora)
            while(1);
    // Cola para el envio del estado de terminal
        cola_terminal = xQueueCreate(1,sizeof(PARAM_COMANDO_TERMINAL));  //espacio para 3items de tamaÃ±o ulong
        if (NULL==cola_terminal)
            while(1);

    // Mutex para proteccion del canal USB (por si 2 tareas deciden usarlo a la vez!)
    mutexUSB=xSemaphoreCreateMutex();
    if(mutexUSB==NULL) // Error de creacion de semaforos

        while(1);

    miSemaforo = xSemaphoreCreateBinary(); //Creamos el semaforo para la tarea de la hora

    semaforoEnergia = xSemaphoreCreateCounting(2,0); //semaforo contador para el modo energia de los botones




    RGBDisable(); //partimos en modo gpio, asi que desactivamos el rgb
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); //habilitamos GPIO
    MAP_GPIOIntEnable(GPIO_PORTF_BASE,ALL_BUTTONS);
    MAP_IntEnable(INT_GPIOF);//habilitamos interrupciones del GPIO


    /**                                              Creacion de tareas                                                 **/

    // Inicializa el sistema de depuracion por terminal UART
    if (initCommandLine(512,tskIDLE_PRIORITY + 1) != pdTRUE)
    {
        while(1);
    }

	USBSerialInit(32,32);	//Inicializo el  sistema USB
	//
	// Crea la tarea que gestiona los comandos USB (definidos en CommandProcessingTask)
	//
	if(xTaskCreate(CommandProcessingTask, (portCHAR *)"usbser",512, NULL, tskIDLE_PRIORITY + 2, NULL) != pdTRUE)
	{
		while(1);
	}

	//

	//
	// Crea la tarea que envia el estado de los botones
	//
	if(xTaskCreate(ButtonsTask, "Botones",512, NULL, tskIDLE_PRIORITY + 2, NULL) != pdTRUE)
	{
	    while(1);
	}
	// Crea la tarea que envia la temperatura

	if(xTaskCreate(TempTask, "temperatura",512, NULL, tskIDLE_PRIORITY + 2, NULL) != pdTRUE)
	{
	     while(1);
	}
	// Crea la tarea que envia la hora
	if(xTaskCreate(horaTask, "hora",512, NULL, tskIDLE_PRIORITY + 2, NULL) != pdTRUE)
    {
         while(1);
    }
	// Crea la tarea que envia los cambios realizados por terminal
    if(xTaskCreate(TerminalTask, "terminal envios",512, NULL, tskIDLE_PRIORITY + 2, NULL) != pdTRUE)
    {
         while(1);
    }


	   //Crea el grupo de eventos
	        FlagsEventos = xEventGroupCreate();
	        if( FlagsEventos == NULL )
	        {
	            while(1);
	        }


	//
	// Arranca el  scheduler.  Pasamos a ejecutar las tareas que se hayan activado.
	//
	vTaskStartScheduler();	//el RTOS habilita las interrupciones al entrar aqui, asi que no hace falta habilitarlas

	//De la funcion vTaskStartScheduler no se sale nunca... a partir de aqui pasan a ejecutarse las tareas.
	while(1)
	{
		//Si llego aqui es que algo raro ha pasado
	}
}
void ADCSequence0Handler (void){
    signed portBASE_TYPE higherPriorityTaskWoken=pdFALSE;   //Hay que inicializarlo a False!!
    uint32_t ui32TempValueC;
    ADCIntClear(ADC1_BASE, 0); // Limpia el flag de interrupcion del ADC
     //leemos los datos del secuenciador
     ADCSequenceDataGet(ADC1_BASE, 0, &ui32TempValueC);
     // Y lo convertimos a grados centigrados y Farenheit, usando la formula indicada en el Data Sheet
     ui32TempValueC = (1475 - ((2475 * ui32TempValueC)) / 4096)/10;
     xQueueSendFromISR (cola_temperatura,&ui32TempValueC,&higherPriorityTaskWoken); //enviamos los datos a la cola
     portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
void ADCSequence1Handler (void){
    signed portBASE_TYPE higherPriorityTaskWoken=pdFALSE;   //Hay que inicializarlo a False!!
     uint32_t ui32TempValueC;
     ADCIntClear(ADC1_BASE, 1); // Limpia el flag de interrupcion del ADC
      //leemos los datos del secuenciador
      ADCSequenceDataGet(ADC1_BASE, 1, &ui32TempValueC);
     // Y lo convertimos a grados centigrados y Farenheit, usando la formula indicada en el Data Sheet
     ui32TempValueC = (1475 - ((2475 * ui32TempValueC)) / 4096)/10;
     xQueueSendFromISR (cola_temperatura,&ui32TempValueC,&higherPriorityTaskWoken);//enviamos los datos a la cola
     portEND_SWITCHING_ISR(higherPriorityTaskWoken);

}
void GPIOFIntHandler(void) //interrupcion de los botones
{

    int32_t i32Status = MAP_GPIOPinRead(GPIO_PORTF_BASE,ALL_BUTTONS);
    //comprobamos el estado de los botones y activamos su flag...
    if ((i32Status & LEFT_BUTTON)==0){
        xEventGroupSetBitsFromISR(FlagsEventos,izquierda,&higherPriorityTaskWoken);
    }
    if((i32Status & RIGHT_BUTTON)==RIGHT_BUTTON){
        xEventGroupSetBitsFromISR(FlagsEventos,derecha_OFF,&higherPriorityTaskWoken);
    }
    if ((i32Status & RIGHT_BUTTON)==0){
        xEventGroupSetBitsFromISR(FlagsEventos,derecha,&higherPriorityTaskWoken);
    }
    if((i32Status & LEFT_BUTTON)==LEFT_BUTTON){
           xEventGroupSetBitsFromISR(FlagsEventos,izquierda_OFF,&higherPriorityTaskWoken);
    }

    MAP_GPIOIntClear(GPIO_PORTF_BASE,ALL_BUTTONS);              //limpiamos flags
    xEventGroupSetBitsFromISR(FlagsEventos,Interrupcion_Flag,&higherPriorityTaskWoken);
    //Cesion de control de CPU si se ha despertado una tarea de mayor prioridad
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);

}
