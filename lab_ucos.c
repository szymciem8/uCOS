*
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

//------------------------------------------------------------------------------
//                              GLOBAL VARIABLES
//------------------------------------------------------------------------------

//STACKS
OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];
OS_STK TaskStartStk[TASK_STK_SIZE];

OS_MEM *input_memory;   //Wskaznik danego bloku pamieci
OS_EVENT *input_queue;

struct {
  INT8U task_number;
  INT8U val_loop;
  INT8U input_iterator;
} task_state;

struct taskToolbox
{
  unsigned char line; //linia wpisywania
  unsigned char offset;
  char str[81]; //ilosc pustych lini w displayu
  char size; //wielkość "Czyszczenia" konsolki
  INT8U bcolor; //bckgr kolor
  INT8U fcolor;
}

//Dla tasków od 6-10
OS_EVENT *Queue;
void *QueueTab[8];

//------------------------------------------------------------------------------
//                          PROTOTYPES OF FUNCTIONS
//------------------------------------------------------------------------------
void  TaskStart(void *data);

static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

void read_key(void *data);    //obsluga klawiatury
void display(void *data);     //obsluga ekranu
void edit(void* data);

void queTask(void *data);     //kolejka

//------------------------------------------------------------------------------
//                                  MAIN
//------------------------------------------------------------------------------
void main(void){
  INT8U memerr;
  PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */
  OSInit();                                              /* Initialize uC/OS-II                      */
  PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */

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

//------------------------------------------------------------------------------
//                                  READ_KEY
//------------------------------------------------------------------------------
void read_key(void *pdata){
  INT16S key;           //przycisk
  INT8U memory_error;   //blad pamiecie
  INT16S *msg;          //wiadomosc

  pdata = pdata;        //Dbamy o to, zeby nie wystapil blad kompilatora

  for(;;)
  {
    while (PC_GetKey(&key)){ //Pobieramy przycisk
      msg = OSMemGet(input_memory, &memory_error);  //Funkcja pobiera blok pamięci
      if (memory_error == OS_NO_ERR){
        *msg = key;
        OSQPost(input_queue, (void*)msg); //Przesylamy wskaznik do kolejki
      }else
        post( 1, MEMERR);
    }
    OSTimeDly(6);   //Oczekujemy 6 cykli zegarowych
  }
}

void display(){

  INT8U display_error;
  data = data;
  struct toolBox *tb;
  char clear[64] = "                                                               \0";

  for(;;)
  {
      tb = OSMboxPend(PrintM, 1, &err); //OSMBoxPend zwraca kody bloedow - jesli wiadomosc dostarczona to OS_NO_ERROR

      //zatem:
      if(err = OS_NO_ERROR)
      {
        clear[tb->size]='\0'; //czyscimy linie
        PC_DispStr(tb->offset,tb->line,clear,DISP_FGND_BLACK + tb->color);
        clear[tb->size]= ' ';
        PC_DispStr(tb->offset,tb->line,tb->str,tb->fcolor + tb->color);
        OSMemPut(dispMem,tb);
      }
  }

}
