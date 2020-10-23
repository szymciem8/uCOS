/*
------------------------------SYSTEMY OPERACYJNE--------------------------------
Program polegający, który ma za zadnie obciążać wieloma procesami (task)
system operacyjny.
Politechnika Śląska
Wydział Automatyki, Elektroniki i Informatyki
Laboratorium RTOS 2,3
Autorzy: Szymon Ciemała, Jakub Kaniowski
OPIS
5 zadań -> skrzynka pocztowa Mbox
5 zadań -> kolejaka
5 zadań -> semafor
W sumie 15 zdań
*/

//LIBRARIES
#include "includes.h"

//------------------------------------------------------------------------------
//                                  CONSTANTS
//------------------------------------------------------------------------------

#define TASK_STACK_SIZE 512
#define NUMBER_OF_TASKS 21
#define BUFFOR_SIZE 11

#define DONE 0

#define KEYBOARD_PRIO 1
#define DISPLAY_PRIO 2

#define WRITE_PRIO 4


//------------------------------------------------------------------------------
//                              GLOBAL VARIABLES
//------------------------------------------------------------------------------

//STACKS
OS_STK TaskStk[NUMBER_OF_TASKS][TASK_STACK_SIZE];
OS_STK TaskStartStk[TASK_STACK_SIZE];

OS_MEM *input_memory;		//Wskaznik danego bloku pamieci
INT32U input_memory_block[32][4];

OS_MEM *display_memory;
INT32U *display_memory_block;

OS_EVENT *input_queue;
OS_EVENT *print_memory;
void *input_queue_tab[32];

//QUEUES
OS_EVENT *queue;

//Struktura przechowująca status danego tasku
struct task_state{
  char task_number;
  INT32U val_loop;
  INT32U load;
  INT32U counter;
  INT32U error;
};

//Struktura pozwalajaca skrocic funkcje display string
struct display_options{
  unsigned char line;		//linia wpisywania
  unsigned char offset;		//Przesuniecie
  char str[81]; 			//ilosc pustych lini w displayu
  char size; 				//wielkość "Czyszczenia" konsolki
  INT8U bcolor; 			//kolor tla
  INT8U fcolor;				//kolor czcionki
};

//------------------------------------------------------------------------------
//                          PROTOTYPES OF FUNCTIONS
//------------------------------------------------------------------------------
void TaskStart(void *pdata);

static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

void read_key(void *data);    //obsluga klawiatury
void write_text(void *data); 	//
void display(void *data);     //obsluga ekranu
void edit(void* data);

//void queTask(void *data);     //kolejka

//------------------------------------------------------------------------------
//                                  MAIN
//------------------------------------------------------------------------------
void main(void){
  INT8U memory_error;
  PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */
  OSInit();                                              /* Initialize uC/OS-II                      */
  PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
  PC_VectSet(uCOS, OSCtxSw);

  input_queue = OSQCreate(&input_queue_tab[0], 32);		//Przechowuje w kolejce dane wejściowe
  print_memory = OSMboxCreate(NULL);

  display_memory = OSMemCreate(display_memory_block, 32, sizeof(struct display_options), &memory_error);

  input_memory = OSMemCreate(input_memory_block, 32, 4, &memory_error);
  OSTaskCreate(TaskStart, (void*)0, &TaskStartStk[TASK_STACK_SIZE - 1], 0);
  OSStart();
}

void TaskStart(void *pdata){
  int i;
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif

    unsigned char tasknrs[5];

    pdata = pdata;                                         /* Prevent compiler warning                 */

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();

	OSTaskCreate(read_key, 		NULL, 	&TaskStk[0][TASK_STACK_SIZE - 1], 	KEYBOARD_PRIO);
	OSTaskCreate(display, 		NULL, 	&TaskStk[1][TASK_STACK_SIZE - 1], 	DISPLAY_PRIO);
	OSTaskCreate(write_text, 	NULL, 	&TaskStk[2][TASK_STACK_SIZE - 1], 	WRITE_PRIO);

	for(;;){
		TaskStartDisp();
		OSCtxSwCtr = 0;
		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}

static  void  TaskStartDispInit (void)
{
    PC_DispStr( 0,  0, "                         uC/OS-II, The Real-Time Kernel                         ", DISP_FGND_WHITE + DISP_BGND_RED);
    PC_DispStr( 0,  1, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_BLUE);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "No. Load        Counter     Val         delta       dps                 State   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "Q01 4294967296  4294967296  4294967296  4294967296                      done0   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "Q02                                                                     busyX   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "Q03                                                                     ERROR   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "Q04                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "Q05                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "M06                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "M07                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "M08                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "M09                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "M10                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "S11                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "S12                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "S13                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, "S14                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "S15                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "#Tasks          :        CPU Usage:     %                                       ", DISP_FGND_WHITE + DISP_BGND_BLUE);
    PC_DispStr( 0, 23, "#Task switch/sec:                                                               ", DISP_FGND_WHITE + DISP_BGND_BLUE);
    PC_DispStr( 0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_WHITE + DISP_BGND_RED);
}

static  void  TaskStartDisp (void)
{
    char   s[80];


    sprintf(s, "%5d", OSTaskCtr);                                  /* Display #tasks running               */
    PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

#if OS_TASK_STAT_EN > 0
    sprintf(s, "%3d", OSCPUUsage);                                 /* Display CPU usage in %               */
    PC_DispStr(36, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#endif

    sprintf(s, "%5u", OSCtxSwCtr);                                 /* Display #context switches per second */
    PC_DispStr(18, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%1d.%02d", OSVersion() / 100, OSVersion() % 100); /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_RED);

    switch (_8087) {                                               /* Display whether FPU present          */
        case 0:
             PC_DispStr(71, 22, " NO  FPU ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 1:
             PC_DispStr(71, 22, " 8087 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 2:
             PC_DispStr(71, 22, "80287 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 3:
             PC_DispStr(71, 22, "80387 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;
    }
}

//------------------------------------------------------------------------------
//                                  READ_KEY
//------------------------------------------------------------------------------
void read_key(void *pdata){
  INT16S key;           //przycisk
  INT8U memory_error;   //blad pamiecie
  INT16S *msg;          //wiadomosc

  pdata = pdata;        //Dbamy o to, zeby nie wystapil blad kompilatora, nie można inicjować zmiennych

  while(1){
    if(PC_GetKey(&key) == TRUE){ //Pobieramy przycisk
      msg = OSMemGet(input_memory, &memory_error);	//Funkcja pobiera adres pustej komórki pamięci
		if(memory_error == OS_NO_ERR){				//W wypadku kiedy nie ma żadnego błędu możemy kontynuować
			*msg = key;
		}else{
			if(memory_error == OS_MEM_NO_FREE_BLKS){
				PC_DispStr(70, 4, "Cokolwiek1", DISP_FGND_YELLOW + DISP_BGND_BLUE);
			}
			if(memory_error == OS_MEM_INVALID_PMEM){
				PC_DispStr(70, 5, "Cokolwiek2", DISP_FGND_YELLOW + DISP_BGND_BLUE);
			}
		}

	  OSQPost(input_queue, msg);			//Przesylamy wskaznik do kolejki
    }
    OSTimeDly(6);   //Oczekujemy 6 cykli zegarowych -> 30 ms
  }
}

void display(void *data){

  INT8U display_error, pend_error;
  struct display_options *disp_opts;
  char clear[64] = "                                                               \0";

  data = data;

  for(;;)
  {
      disp_opts = OSMboxPend(print_memory, 0, &pend_error); //OSMBoxPend zwraca kody bloedow - jesli wiadomosc dostarczona to OS_NO_ERROR

      //zatem:
      if(pend_error == OS_NO_ERR) //OS_NO_ERROR
      {
        clear[disp_opts->size]='\0'; //czyscimy linie
        PC_DispStr(disp_opts->offset,	disp_opts->line,	clear,	disp_opts->fcolor + disp_opts->bcolor);
        clear[disp_opts->size]= ' ';
        PC_DispStr(disp_opts->offset,	disp_opts->line,	disp_opts->str,	disp_opts->fcolor + disp_opts->bcolor);
        OSMemPut(display_memory, disp_opts);
		PC_DispStr(70, 5, "test_display", DISP_FGND_YELLOW + DISP_BGND_BLUE);
      }
	  else{//Napotkalismy problem z TIMEOUT
		  if (pend_error == OS_TIMEOUT) PC_DispStr(70, 5, "test_display2", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		  else if (pend_error == OS_ERR_EVENT_TYPE) 	PC_DispStr(70, 5, "event", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		  else if (pend_error == OS_ERR_PEND_ISR) 		PC_DispStr(70, 5, "isr", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		  else if (pend_error == OS_ERR_PEVENT_NULL) 	PC_DispStr(70, 5, "pevent", DISP_FGND_YELLOW + DISP_BGND_BLUE);

	  }
  }
}
/*
void q_loaded_task(void *data){
	struct task_state stats;
	INT32U *temporary;
	int temporary_val;
	stats.task_number *(char *) data;
	for(stats.load=100, stats.counter=1; ; stats.counter++){



}
*/
void write_text(void *pdata){
	INT8U pend_error, memory_error;		//ERRORS
	INT8U char_counter;
	INT32U *msg;
	INT16S *received_data;
	INT16S key;
	INT16S temp_key;
	char buffor[BUFFOR_SIZE];
	struct display_options *disp_opts;

	pdata = pdata;

	buffor[BUFFOR_SIZE - 1] = '\0';		//Koniec buffer jest pusty

	while(1){
		received_data = OSQPend(input_queue, 0, &pend_error);
		key = *received_data;
		OSMemPut(input_memory, received_data);

		switch(key){
			case 0x1B:
				PC_DOSReturn();
				break;
			default:
				if (char_counter < BUFFOR_SIZE - 1){
          buffor[char_counter] = temp_key;
          char_counter += 1;
        }
				break;
		}

		disp_opts = OSMemGet(display_memory, &memory_error);
		disp_opts -> line = 4;
		disp_opts -> offset = 0;
		strcpy(disp_opts -> str, buffor);
		disp_opts -> size = BUFFOR_SIZE;
		disp_opts -> fcolor = DISP_FGND_BLACK;
		disp_opts -> bcolor = DISP_BGND_LIGHT_GRAY;
		OSMboxPost(print_memory, disp_opts);
	}
}
