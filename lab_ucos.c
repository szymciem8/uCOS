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

#define EDIT_PRIO 4

//------------------------------------------------------------------------------
//                                  STRUCTURES
//------------------------------------------------------------------------------

//Struktura pozwalajaca skrocic funkcje display string
typedef struct display_options{
  unsigned char x;		//linia wpisywania
  unsigned char y;		//Przesuniecie
  INT8S mode;
  char str[81]; 			//ciag znakow
  char size; 				//wielkość "Czyszczenia" konsolki

  int task_number;
  INT32U load;
  INT32U counter;
}display_options;

//TASK PARAMETERS
typedef struct task_parameters{
  INT8S task_number;
  INT32U val_loop;
  INT32U load;
  INT32U counter;
  INT32U error;
}task_parameters;

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
void *input_queue_tab[32];

OS_EVENT *main_mailbox;
OS_EVENT *value_mailbox;

//MAILBOX TASKS
OS_EVENT *mailbox[5];
task_parameters mailbox_memory_block[15];
OS_MEM *mailbox_task_memory;

//QUEUE TASKS
OS_EVENT *queue;
void *queue_array[15];
task_parameters queue_memory_block[15];
OS_MEM *queue_task_memory;

//SEMAPHORE TASKS
OS_EVENT *semaphore;

//------------------------------------------------------------------------------
//                          PROTOTYPES OF FUNCTIONS
//------------------------------------------------------------------------------
void TaskStart(void *pdata);

static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

void read_key(void *data);		//obsluga klawiatury
void edit_input(void *data); 	//obsuluga wpisywanych danych
void display(void *data);		//obsluga ekranu

void mailbox_task(void *data);
void queue_task(void *data);
void semaphore_task(void *data);

void set_mailbox_load(void *data);
void set_queue_load(void *data);

//------------------------------------------------------------------------------
//                                  MAIN
//------------------------------------------------------------------------------
void main(void){
  int i;
  INT8U memory_error;
  PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */
  OSInit();                                              /* Initialize uC/OS-II                      */
  PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
  PC_VectSet(uCOS, OSCtxSw);

  input_queue = OSQCreate(input_queue_tab, 32);		//Przechowuje w kolejce dane wejściowe
  main_mailbox = OSMboxCreate(NULL);				//mailbox sluzacy do przesylania danych miedzy read_key(), a edit_input()

  //Inicjalizacja obkietow potrzebnych do zadan obciazajacych
  for(i=0; i<5; i++){
	mailbox[i] = OSMboxCreate(NULL);	//skrzynki
  }
  queue = OSQCreate(queue_array, 15);	//kolejka
  semaphore = OSSemCreate(0);			//semafor

  value_mailbox = OSMboxCreate(NULL);

  //Tworzymy miejsce w pamieci na odpowiednie zadania, czyli partycje
  //Adres pierwszego bloku, ilosc blokow, wielkosc blokow, blad
  mailbox_task_memory = OSMemCreate(mailbox_memory_block, 15, sizeof(task_parameters), &memory_error);
  queue_task_memory = OSMemCreate(queue_memory_block, 15, sizeof(task_parameters), &memory_error);

  //Tworzymy 32 bloki o wielkości 4 bity, nie działa sizeof(INT16S) ani sizeof(char)
  input_memory = OSMemCreate(input_memory_block, 32, 4, &memory_error);

  OSTaskCreate(TaskStart, (void*)0, &TaskStartStk[TASK_STACK_SIZE - 1], 0);
  OSStart();
}

void TaskStart(void *pdata){
  int i;
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif

    unsigned char task_numbers[5];
	display_options *disp_opts;

    pdata = pdata;                                         /* Prevent compiler warning                 */

	disp_opts->mode = 3;

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();

	//Taski związane z wczytywaniem danych od uzytkownika
	OSTaskCreate(read_key, 		NULL, 	&TaskStk[0][TASK_STACK_SIZE - 1], 	KEYBOARD_PRIO);
	OSTaskCreate(display, 		NULL, 	&TaskStk[1][TASK_STACK_SIZE - 1], 	DISPLAY_PRIO);
	OSTaskCreate(edit_input, 	NULL, 	&TaskStk[2][TASK_STACK_SIZE - 1], 	EDIT_PRIO);

	//Taski glowne - mailbox, queue, semaphore
	for(i=0; i<5; i++){
		task_numbers[i] = i + 1;
		PC_DispChar(76, i+7,	OSTaskCreate(mailbox_task, &task_numbers[i],	&TaskStk[i+5][TASK_STACK_SIZE - 1],	i+5)	+'0',	DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		PC_DispChar(76, i+12,	OSTaskCreate(queue_task,   &task_numbers[i], 	&TaskStk[i+10][TASK_STACK_SIZE - 1], i+10)	+'0',	DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		PC_DispChar(76, i+17,	OSTaskCreate(semaphore_task,   &task_numbers[i], 	&TaskStk[i+15][TASK_STACK_SIZE - 1], i+15)	+'0',	DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	}

	for(;;){
		TaskStartDisp();
		OSCtxSwCtr = 0;
		OSMboxPost(main_mailbox, disp_opts); //wysylamy za pomoca mailbox'a dane do wyswietlenia delty
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
    PC_DispStr( 0,  6, "No. Load        Counter                 delta                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "M01                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "M02                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "M03                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "M04                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "M05                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "Q06                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "Q07                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "Q08                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "Q09                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "Q10                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
  INT16S key;			//przycisk
  INT8U memory_error;	//blad pamieci
  INT16S *msg;			//wiadomosc

  pdata = pdata;        //Dbamy o to, zeby nie wystapil blad kompilatora, nie można inicjować zmiennych

  while(1){
    if(PC_GetKey(&key) == TRUE){						//Pobieramy przycisk
		msg = OSMemGet(input_memory, &memory_error);	//Funkcja pobiera adres pustej komórki pamięci
		if(memory_error == OS_NO_ERR){					//W wypadku kiedy nie ma żadnego błędu możemy kontynuować
			*msg = key;									//W wybranym miejscu w pamięci zapisujemy key
			OSQPost(input_queue, (void*)msg);			//Przesylamy wskaznik do kolejki
		}else{
			//Obsługa błędów
			if(memory_error == OS_MEM_NO_FREE_BLKS){
				PC_DispStr(60, 4, "OS_MEM_NO_FREE_BLKS", DISP_FGND_YELLOW + DISP_BGND_BLUE);
			}
			if(memory_error == OS_MEM_INVALID_PMEM){
				PC_DispStr(60, 5, "MEM_INVALID_PMEM", DISP_FGND_YELLOW + DISP_BGND_BLUE);
			}
		}
    }
    OSTimeDly(6);   //Oczekujemy 6 cykli zegarowych -> 30 ms
  }
}

void display(void *data){

  INT8U display_error, pend_error;							//errors
  INT32U counter[15], previous_counter[15];					//liczniki
  struct display_options *disp_opts;						//opcje wyswietlania
  char load_to_print[11], counter_to_print[11], delta[11];	//zmienne sluzace do zapisania sformatowanych danych
  char clear[64] = "                                                               \0";
  int i;

  data = data;

  for(;;){
      disp_opts = OSMboxPend(main_mailbox, 0, &pend_error); //OSMBoxPend zwraca kody bledow - jesli wiadomosc dostarczona to OS_NO_ERROR
		switch(disp_opts -> mode){
		//Wyswietlanie linii z danymi do wpisania
		  case 1:
			  if(pend_error == OS_NO_ERR) //OS_NO_ERROR
			  {
				clear[disp_opts->size]='\0'; //czyscimy linie
				PC_DispStr(disp_opts->y,	disp_opts->x,	clear,			DISP_FGND_YELLOW + DISP_BGND_BLUE);
				clear[disp_opts->size]= ' ';
				PC_DispStr(disp_opts->y,	disp_opts->x,	disp_opts->str,	DISP_FGND_YELLOW + DISP_BGND_BLUE);
			  }
			  else{
					   if (pend_error == OS_TIMEOUT) 			PC_DispStr(50, 5, "TIMEOUT ERROR", DISP_FGND_YELLOW + DISP_BGND_BLUE);
				  else if (pend_error == OS_ERR_EVENT_TYPE)		PC_DispStr(50, 5, "EVENT TYPE ERRO", DISP_FGND_YELLOW + DISP_BGND_BLUE);
				  else if (pend_error == OS_ERR_PEND_ISR)		PC_DispStr(50, 5, "ISR ERROR", DISP_FGND_YELLOW + DISP_BGND_BLUE);
				  else if (pend_error == OS_ERR_PEVENT_NULL)	PC_DispStr(50, 5, "PEVENT ERROR", DISP_FGND_YELLOW + DISP_BGND_BLUE);
				  else PC_DispStr(50, 5, "UNKNOWN ERROR", DISP_FGND_YELLOW + DISP_BGND_BLUE);
			  }
			  break;
		//Wyswietlamy wartości load i counter dla każdego zadania
		  case 2:
			  counter[disp_opts->task_number -1] = disp_opts->counter;

			  PC_DispStr(4, 6 + disp_opts->task_number,"         ",DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);	//load
			  PC_DispStr(17, 6 + disp_opts->task_number,"         ",DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);	//counter

			  //Korzystając z funkcji sprintf, formatujemy odpowiednio zapis load i counter
			  sprintf(load_to_print, "%lu", disp_opts->load);
			  sprintf(counter_to_print, "%lu", disp_opts->counter);

			  PC_DispStr(4, 6 + disp_opts->task_number, load_to_print ,DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			  PC_DispStr(17, 6 + disp_opts->task_number, counter_to_print ,DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			  break;
		  case 3:
		  //Dla każdego zadania wyświetlamy wartość delty, czyli zmiany countera w jednostce czasu
			for(i=0; i<15; i++){
				sprintf(delta, "%lu", counter[i] - previous_counter[i]);
				previous_counter[i] = counter[i];
				PC_DispStr(40, 7 + i, "                ",DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				PC_DispStr(40, 7 + i, delta,             DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			}
		  break;
	  }
  }
}

void edit_input(void *pdata){
	int i;
	INT8U post_error, pend_error, memory_error, mail_error, queue_error, error;		//ERRORS
	INT8U char_counter=0;		//Określa ilość aktualnie wpisanych znaków
	INT16S *received_data;
	INT16S key;
	char buffor[BUFFOR_SIZE] = "           ";
	struct display_options disp_opts;

	pdata = pdata;

	buffor[BUFFOR_SIZE - 1] = '\0';	//Koniec buffer jest pusty

	while(1){
		received_data = OSQPend(input_queue, 0, &pend_error);	 //Pobieramy received_data z kolejki, czyli wskaznik
		//wiadomosci, stosujac * pobieramy jego wartość
		key = *received_data;
		OSMemPut(input_memory, received_data);	//received_data to jednocześnie adres bloku pamięci wejściowej,
		//która w tym momencie może zostać zwolniona

		//Na podstawie key wykonujemy odpowiednie funkcje
		switch(key){
			case 0x1B:				//Escape
				PC_DOSReturn();
				break;
			case 0x08:				//Backaspace
				//Jeżeli istnieją jakieś znaki to można je usunąć
				if (char_counter > 0){
					buffor[char_counter-1] = ' ';
					char_counter -= 1;
				}
				break;
			case 0x2E:				//Delete
				//Usuwamy wszystkie znaki
				while(char_counter >0){
					buffor[char_counter-1] = ' ';
					char_counter -= 1;
				}
				break;
			case 0x0D:				//Enter

				//ustawiamy load dla każdego zadania
				set_mailbox_load(buffor);
				set_queue_load(buffor);
				OSMboxPost(value_mailbox, buffor);

				OSSemPost(semaphore);

				//Po zakończeniu usuwamy całą linię danych
				while(char_counter >0){
					buffor[char_counter-1] = ' ';
					char_counter -= 1;
				}

				break;
			default:				//ladowanie danych do bufora
				if (char_counter < BUFFOR_SIZE-1){
					buffor[char_counter] = key;
					char_counter += 1;
				}
				break;
		}

		//Opcje do wyświetlenia wpisywanych danych
		disp_opts.x = 4;
		disp_opts.y = 0;
		disp_opts.mode = 1;
		strcpy(disp_opts.str, buffor);
		disp_opts.size = BUFFOR_SIZE;

		//Przesłanie danych za pomocą skrzynki
		post_error = OSMboxPost(main_mailbox, &disp_opts);

		//Obsługa błędów
		if (post_error == OS_MBOX_FULL) PC_DispStr(50, 5, "OS_MBOX_FULL", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		else if (post_error == OS_ERR_EVENT_TYPE) 		PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		else if (post_error == OS_ERR_PEVENT_NULL) 		PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		else if (post_error == OS_ERR_POST_NULL_PTR) 	PC_DispStr(0, 1, "OS_ERR_POST_NULL_PTR", DISP_FGND_YELLOW + DISP_BGND_BLUE);
	}
}

void mailbox_task(void *data){
	display_options disp_opts;
	task_parameters *mailbox_task_params;

	char task_number;
	INT8U memory_error;
	int i;
	INT32U load=255;
	INT32U load_iterator;
	INT32U counter=0;

	task_number = *(INT8U*)data;

	while(1){
		//Pobieramy parametry danej skrzynki
		mailbox_task_params = OSMboxAccept(mailbox[task_number - 1]);

		//Jesli skrzynka nie jest pusta
		if(mailbox_task_params != NULL){
			//Aktualizujemy load
			load = mailbox_task_params->load;
			OSMemPut(mailbox_task_memory, mailbox_task_params);
		}

		//Opcje do wyświetlenia
		disp_opts.task_number = task_number;
		disp_opts.load = load;
		disp_opts.counter = counter;
		disp_opts.mode = 2;
		OSMboxPost(main_mailbox, &disp_opts);

		//Pętla obciążająca
		for(load_iterator=0; load_iterator < load; load_iterator++);
		counter++;
		OSTimeDly(1);
	}

}

void queue_task(void *data){
	OS_Q_DATA queue_data;
	//Struktura zawiera
	//*OSMsg -> czy kolejna wiadomosc jest dostepna
	//OSNMsgs -> ilosc wiadomosci w kolejce
	//OSQSize -> wielkosc wiadomosci
	//OSEventTbl[OS_EVENT_TBL_SIZE] -> lista oczekujacych wiadomosci

	display_options disp_opts;
	task_parameters *queue_task_params;
	char task_number;
	INT8U memory_error, *queue_error;
	INT32U i;
	INT32U load=255, load_iterator;
	INT32U counter=0;

	task_number = *(INT8U*) data + 5;

	while(1){
		//Pobieramy informacje o kolejce
		OSQQuery(queue, &queue_data);

		//Iterujemy przez kolejne wiadomosci
		for(i=0; i<queue_data.OSNMsgs; i++){
			//Jezeli mamy dostepna wiadomosc w kolejce pobieramy dane
			queue_task_params = OSQAccept(queue);
			/*
			if(queue_error != OS_NO_ERR){
				if(queue_error == OS_ERR_EVENT_TYPE){
					PC_DispStr(62, 6 + task_number, "            ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
					PC_DispStr(62, 6 + task_number, "error1", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				}
				else if(queue_error == OS_ERR_PEVENT_NULL){
					PC_DispStr(62, 6 + task_number, "            ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
					PC_DispStr(62, 6 + task_number, "error3", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				}
				else if(queue_error == OS_Q_EMPTY){
					PC_DispStr(62, 6 + task_number, "            ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
					PC_DispStr(62, 6 + task_number, "error2", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				}
			}
			*/

			//Jesli paramtery przeslane przez kolejke nie sa puste
			//pobieramy dane do tasku
			if (queue_task_params != NULL){
				//Operacja dla pierwszej wiadomosci dla danego zadania kolejki
				if(i == 0 && queue_task_params->task_number == task_number){
					//Pobieramy load z pobranych parametrow
					load = queue_task_params->load;
					memory_error = OSMemPut(queue_task_memory, queue_task_params);
					if(memory_error != OS_NO_ERR){
						if(memory_error == OS_MEM_NO_FREE_BLKS){
							PC_DispStr(62, 6 + task_number, "                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
							PC_DispStr(62, 6 + task_number, "OS_MEM_NO_FREE_BLKS", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
						}
						else if(memory_error == OS_MEM_INVALID_PMEM){
							PC_DispStr(62, 6 + task_number, "                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
							PC_DispStr(62, 6 + task_number, "OS_MEM_INVALID_PMEM", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
						}
					}
				}
				//W przeciwnym wypadku
				else OSQPost(queue, queue_task_params);
			}
		}
		counter++;

		//Pętla obciążająca
		for(load_iterator=0; load_iterator<load; load_iterator++);

		//Aktualizujemy dane do wyświetlenia
		disp_opts.task_number = task_number;
		disp_opts.load = load;
		disp_opts.counter = counter;
		disp_opts.mode = 2;

		//Przesyłamy opcje do wyświetlenia
		OSMboxPost(main_mailbox, &disp_opts);

		OSTimeDly(1);
	}
}

void semaphore_task(void *data){
	display_options disp_opts;
	char task_number;
	INT8U memory_error, mail_error;
	int i;
	INT32U load=255, load_iterator;
	INT32U counter=0;
	INT32U value=255;
	void *msg;

	task_number = *(INT8U *)data + 10;

	while(1){
		//Tutaj trzeba prawdopodobnie stworzyć miejsce w pamieci, żeby działało
		msg = OSMboxAccept(value_mailbox);					//Jesli jest to odbieramy msg, czyli buffor z edit

		if (msg != (void *)0)load = strtoul(msg, NULL, 10);	//W momencie pojawienia sie wiadomosci aktualizujemy load

		if(OSSemAccept(semaphore)){	//Oczekujemy na semafor
			OSSemPost(semaphore);	//Wysylamy semafor
		}

		//Petla obciazajaca
		for(load_iterator=0; load_iterator<load; load_iterator++);
		counter++;

		//Opcje wyswietlania
		disp_opts.task_number = task_number;
		disp_opts.load = load;
		disp_opts.counter = counter;
		disp_opts.mode = 2;
		OSMboxPost(main_mailbox, &disp_opts);

		OSTimeDly(1);
	}

}

//Funkcja ustawiajaca load dla mailboxa
void set_mailbox_load(void *data){
	int i;
	INT8U memory_error, mail_error, error;		//ERRORS
	INT32U value;

	task_parameters *mailbox_params;
	task_parameters *ptr_mailbox_params[5];

	value = strtoul(data, NULL, 10);

	//MAILBOX
	for(i=0; i<5; i++){
		ptr_mailbox_params[i] = OSMemGet(mailbox_task_memory, &memory_error);
		if(memory_error != OS_NO_ERR){
			if(memory_error == OS_MEM_NO_FREE_BLKS){
				PC_DispStr(62,  7 + i, "             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				PC_DispStr(62,  7 + i, "OS_MEM_NO_FREE_BLKS", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			}
			else if(memory_error == OS_MEM_INVALID_PMEM){
				PC_DispStr(62,  7 + i, "             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				PC_DispStr(62,  7 + i, "OS_MEM_INVALID_PMEM", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			}
		}
		ptr_mailbox_params[i] -> load = value;
		mail_error = OSMboxPost(mailbox[i], ptr_mailbox_params[i]);
		if(mail_error == OS_MBOX_FULL){
			PC_DispStr(62, 7 + i, "            ", 	DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispStr(62, 7 + i, "mbox full", 		DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			mailbox_params = OSMboxPend(mailbox[i], 0,&error);
			OSMemPut(mailbox_task_memory, mailbox_params);
			mail_error = OSMboxPost(mailbox[i], ptr_mailbox_params[i]);
		}
	}

}

//Funkcja ustawiajaca load dla kolejki
void set_queue_load(void *data){
	int i;
	INT8U memory_error, queue_error;		//ERRORS
	INT32U value;

	task_parameters *queue_params;
	task_parameters *ptr_queue_params[5];

	value = strtoul(data, NULL, 10);

	//QUEUE
	for(i=0; i<5; i++){
		ptr_queue_params[i] = OSMemGet(queue_task_memory, &memory_error);
		if(memory_error != OS_NO_ERR){
			if(memory_error == OS_MEM_NO_FREE_BLKS){
				PC_DispStr(62, 12 + i, "            ", 	DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				PC_DispStr(62, 12 + i, "OS_MEM_NO_FREE_BLKS", 		DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			}
			else if (memory_error == OS_MEM_INVALID_PMEM){
				PC_DispStr(62, 12 + i, "            ", 	DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				PC_DispStr(62, 12 + i, "OS_MEM_INVALID_PMEM", 		DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			}
			}
		//Aktulizacja load i task_number
		ptr_queue_params[i] -> load = value;
		ptr_queue_params[i] -> task_number = i+6;
		queue_error = OSQPost(queue, ptr_queue_params[i]);
		if(queue_error == OS_Q_FULL){
			PC_DispStr(67, 12 + i, "             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispStr(67, 12 + i, "Q_FULL", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			queue_params = OSQPend(queue, 0, &queue_error);
			PC_DispStr(67, 12 + queue_params->task_number, "             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispStr(67, 12 + queue_params->task_number, "UTRACONE OBC", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			OSMemPut(queue_task_memory, queue_params);
			queue_error = OSQPost(queue, ptr_queue_params[i]);
		}

	}
}
