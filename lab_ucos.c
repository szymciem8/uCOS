/*
------------------------------SYSTEMY OPERACYJNE--------------------------------

Program polegający, który ma za zadnie obciążać wieloma procesami (task)
system operacyjny.

Politechnika Śląska
Wydział Automatyki, Elektroniki i Informatyki
Laboratorium RTOS 2,3

Autorzy: Szymon Ciemała, Jakub Kaniowski
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
OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];
OS_STK TaskStartStk[TASK_STK_SIZE];

//------------------------------------------------------------------------------
//                          PROTOTYPES OF FUNCTIONS
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//                                  MAIN
//------------------------------------------------------------------------------
void main(void){

}

void TaskStart(void *pdata){

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


void read_key(void *pdata){
  INT16S key;
  INT8U memerr;
  INT16S *msg;

  pdata = pdata;

  for(;;)
  {
    while (PC_GetKey(&key))
    {
      msg = OSMemGet(inMem,&memerr);
      if (memerr == OS_NO_ERR)
      {
      *msg = key;
      OSQPost(inQ,(void*)msg);
      }
      else post( 1, MEMERR);
    }
    OSTimeDly(6);
  }
}

void display(){


}

void read_key(){


}
