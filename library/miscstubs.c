/*
 * JBP (Jim Peterson <jspeter@birch.ee.vt.edu>): Lots of stubs needed for
 *      libwine.a.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "dde_mem.h"
#include "global.h"
#include "debug.h"

void SIGNAL_MaskAsyncEvents( BOOL32 mask )
{
    /* FIXME: signals don't work in the library */
}

int CallTo32_LargeStack( int (*func)(), int nbargs, ...)
{
  va_list arglist;
  int i,a[32];

  va_start(arglist,nbargs);

  for(i=0; i<nbargs; i++) a[i]=va_arg(arglist,int);

  switch(nbargs) /* Ewww... Icky.  But what can I do? */
  {
  case 5: return func(a[0],a[1],a[2],a[3],a[4]);
  case 6: return func(a[0],a[1],a[2],a[3],a[4],a[5]);
  case 8: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
  case 10: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9]);
  case 11: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9],a[10]);
  case 14: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9],a[10],a[11],a[12],a[13]);
  case 16: return func(a[0],a[1],a[2],a[3],a[4],a[5],a[6],
                a[7],a[8],a[9],a[10],a[11],a[12],a[13],a[14],a[15]);
  default: fprintf(stderr,"JBP: CallTo32_LargeStack called with unsupported "
                          "number of arguments (%d).  Ignored.\n",nbargs);
           return 0;
  }
}

extern LRESULT ColorDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT FileOpenDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT FileSaveDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT FindTextDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT MDIClientWndProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT PrintDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT PrintSetupDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT ReplaceTextDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT ScrollBarWndProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT StaticWndProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT TASK_Reschedule(void);

/***********************************************************************
 *           MODULE_GetWndProcEntry16 (not a Windows API function)
 *
 * Return an entry point from the WPROCS dll.
 */
FARPROC16 MODULE_GetWndProcEntry16( char *name )
{
#define MAP_STR_TO_PROC(str,proc) if(!strcmp(name,str))return (FARPROC16)proc
  MAP_STR_TO_PROC("ColorDlgProc",ColorDlgProc);
  MAP_STR_TO_PROC("FileOpenDlgProc",FileOpenDlgProc);
  MAP_STR_TO_PROC("FileSaveDlgProc",FileSaveDlgProc);
  MAP_STR_TO_PROC("FindTextDlgProc",FindTextDlgProc);
  MAP_STR_TO_PROC("MDIClientWndProc",MDIClientWndProc);
  MAP_STR_TO_PROC("PrintDlgProc",PrintDlgProc);
  MAP_STR_TO_PROC("PrintSetupDlgProc",PrintSetupDlgProc);
  MAP_STR_TO_PROC("ReplaceTextDlgProc",ReplaceTextDlgProc);
  MAP_STR_TO_PROC("ScrollBarWndProc",ScrollBarWndProc);
  MAP_STR_TO_PROC("StaticWndProc",StaticWndProc);
  MAP_STR_TO_PROC("TASK_Reschedule",TASK_Reschedule);
  fprintf(stderr,"warning: No mapping for %s(), add one in library/miscstubs.c\n",name);
  return NULL;
}

