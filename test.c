/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"
/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        10       /* Number of identical tasks                          */

//Priorytety zadań
#define TASK_READ_PRIO 1
#define TASK_BUFF_PRIO 2
#define TASK_DISPLAY_PRIO 3

#define MSG_QUEUE_SIZE 20
#define lineSize 40			//Maksymalna ilość znaków w linii

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];

//Dodatkowe stosy dla Read, Buff oraz DisplayStk
OS_STK        TaskReadStk[TASK_STK_SIZE];
OS_STK        TaskBuffStk[TASK_STK_SIZE];
OS_STK        TaskDisplayStk[TASK_STK_SIZE];

char          TaskData[N_TASKS];                      /* Parameters to pass to each task               */
OS_EVENT      *RandomSem;

OS_EVENT      *Mbox;
OS_EVENT      *MsgQueue;
void          *MsgQueueTb1[MSG_QUEUE_SIZE];

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  Task(void *data);                       /* Function prototypes of tasks                  */
        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

//Nowe funkcje
void read(void *data);
void display(void *data);
void buff(void *data);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

    RandomSem   = OSSemCreate(1);                          /* Random number semaphore                  */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    char       s[100];
    INT16S     key;


    pdata = pdata;                                         /* Prevent compiler warning                 */

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

//NOWE
    Mbox = OSMboxCreate((void *)0);                        /*Tworzmy skrzynkę                           */
    MsgQueue = OSQCreate(&MsgQueueTb1[0], MSG_QUEUE_SIZE);   /*Oraz kolejkę                               */

    TaskStartCreateTasks();                                /* Create all the application tasks         */

    for (;;) {
        TaskStartDisp();                                  /* Update the display                       */

        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait one second                          */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDispInit (void)
{
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    PC_DispStr( 0,  0, "                                                                                ", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
    PC_DispStr( 0,  1, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "#Tasks          :        CPU Usage:     %                                       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "#Task switch/sec:                                                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY + DISP_BLINK);
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           UPDATE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDisp (void)
{
    char   s[80];


    sprintf(s, "%5d", OSTaskCtr);                                  /* Display #tasks running               */
    PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

#if OS_TASK_STAT_EN > 0
    sprintf(s, "%3d", OSCPUUsage);                                 /* Display CPU usage in %               */
    PC_DispStr(36, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#endif

    sprintf(s, "%5d", OSCtxSwCtr);                                 /* Display #context switches per second */
    PC_DispStr(18, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%1d.%02d", OSVersion() / 100, OSVersion() % 100); /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

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

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
    INT8U  i;

    //Tworzmy nowe zadania. Każde z nich ma określonyh priorytet, swój stak ("stos")
    //Task READ
    OSTaskCreate(read, (void *)0, &TaskReadStk[TASK_STK_SIZE - 1], TASK_READ_PRIO);

    //Task DISPLAY
    OSTaskCreate(display, (void *)0, &TaskDisplayStk[TASK_STK_SIZE - 1], TASK_DISPLAY_PRIO);

    //Task BUFF
    OSTaskCreate(buffers, (void *)0, &TaskBuffStk[TASK_STK_SIZE -1], TASK_BUFF_PRIO);


    for (i = 0; i < N_TASKS; i++) {                        /* Create N_TASKS identical tasks           */
        TaskData[i] = '0' + i;                             /* Each task will display its own letter    */
        OSTaskCreate(Task, (void *)&TaskData[i], &TaskStk[i][TASK_STK_SIZE - 1], i + 1);
    }
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void  Task (void *pdata)
{
    INT8U  x;
    INT8U  y;
    INT8U  err;

    for (;;) {
        OSSemPend(RandomSem, 0, &err);           /* Acquire semaphore to perform random numbers        */
        x = random(80);                          /* Find X position where task number will appear      */
        y = random(16);                          /* Find Y position where task number will appear      */
        OSSemPost(RandomSem);                    /* Release semaphore                                  */
                                                 /* Display the task number on the screen              */
        PC_DispChar(x, y + 5, *(char *)pdata, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
        OSTimeDly(1);                            /* Delay 1 clock tick                                 */
    }
}

//Funkcja wczytująca kliknięcia przycisku
void read (void *data){
  char key;   //przycisku
  char *ptr;  //wzkźnik
  INT8U ctr;  //counter 'ctr' -> ilość znaków w linii

  data = data; //Zapobiegamy ostrzeżeniom kompilatora
  ptr = &key;  //Zapisujemy wskaźnika przycisku
  ctr = 0;     //Zerujemy counter 'ctr'

  for (;;){ //Pętla nieskończona
    if(PC_GetKey((void *)ptr)){  //Wywołujemy funkcję PC_GetKey przekazując jej wskaźnik nieokreślonego typu
      if(key == 13 && ctr!=0){ //Enter
        OSQPost(MsgQueue, (void *)&key); //Dokładamy znak do kolejki
        ctr = 0; //Restartujemy ctr
      }else{
        OSQPost(MsgQueue, (void *) &key); //Dodajemy każdy inny znak
        ctr++; //Zwiększamy
      }
    }
    OSTimeDly(1); //Opóźnienie 1clk
  }
}

//Pobiera i przesyła dane
void buffer (void *data){
  INT8U err;
  char *msg;

  data = data;

  for(;;){
    msg = (char *)OSQPend(MsgQueue, 0, &err); //Odbieramy wiadomości z kolejki
    OSMboxPost(Mbox, (char *) &msg[0]);       //Adres pierwszej komórki msg
    OSTimeDly(1);                             //Tick
  }
}

//Wyswietlanie danych
void display (void *data){
  INT8U x, y;
  INT8U counter;      //Zmianna pomocnicza licząca ilość znaków w linii niezależnie od x
  char *msg;
  INT8U err;
  char text[lineSize]; //Tablica text zawierająca wszystkie znaki w linii

  data = data;
  x = 0;
  y = 1;
  counter = 0;

  for(;;){
    msg = (char *)OSMboxPend(Mbox, 0, &err); //Odbieramy wiadomość za pomocą funkcji
    //OSMboxPend ze sksrzynki Mbox,
    //0 -> oznacza, że nie ma ograniczenia czasowego na odebranie wiadomości
    //err -> błąd

    if (msg[0] == 0x1B){    //ESC -> wychodzimy z programu
      PC_DOSReturn();       //Koniec programu
    }
    else if (msg[0] == 8){  //Backspace -> usunięce znaku po lewej stronie kursora
      if(x != 0){           //Sprawdzamy czy x znaku nie znajduje się na współrzędnej x
        x--;                //Zmniejszamy współrzędną x
        text[x] = '\0';     //Czyścimy daną komórkę tablicy text
        PC_DispStr(x, 21, " ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
        counter--;
      }
    }
    else if (msg[0] == 0x2E){ //Delete -> Czyścimy linię
      while (x>0){            //Dopóki x jest większe od 0 -> istnieją jakieś znaki w linii
        x--;                  //Zmniejszamy x -> przesuwamy się w lewo
        text[x] = '\0';
        PC_DispStr(x, 21, " ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
        counter--;
      }
    }
    else if (msg[0] == 13){ //Enter przenosimy linię
      if (x != 0){
        text[x] = '\0';
        PC_DispStr(0, y, text, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
        y++;
        if (y==5) y=1; //ile linii można zapisać

        while (x!=0){
          text[x] = '\0';
          x--;
          PC_DispStr(x, 21, " ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
        }
        counter = 0;
      }
    }
    else{ //Inne znaki
      if (counter < lineSize){
        PC_DispChar(x, 21, (char) *msg, DISP_FGND_RED + DISP_BGND_LIGHT_GRAY);
        text[x] = (char)msg[0]; //Rzutujemy msg[0] na char
        x++;
        counter++;
      }
      else{
        counter=lineSize;
      }
    }
    OSTimeDly(1);
  }
}
