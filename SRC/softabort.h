#ifndef SOFTABORT_H
#define SOFTABORT_H

#ifdef SOFTABORTMAIN
int finishSoftabort = 0;
#else
extern int finishSoftabort;
#endif

#ifdef _MSC_VER

# include <windows.h>
# include <stdio.h>

# ifndef SOFTABORTMAIN
  void enableHandler();
  extern int sCount;
  int stopEvent();
# else
  static const char *hName = "VitessStopHandle";
  static HANDLE hS;
  int sCount;
  static int HandlerEnabled;
  void enableHandler() {
    if (HandlerEnabled) return;
    hS = CreateEvent( NULL, FALSE, FALSE, hName);
    if (hS == NULL) {
      fprintf(LogFilePtr, "CreateEvent failed [%x]\n", GetLastError());
    }
    ResetEvent(hS);
    HandlerEnabled = 1;
  }
  int stopEvent () {
    int rc;
    finishSoftabort = 1;
    rc = WaitForSingleObject(hS, 0);
    if (rc == WAIT_FAILED) {
      fprintf(LogFilePtr,"WaitForSingleObject failed %d\n", rc);
      return 1;
    }
  return WAIT_OBJECT_0 == rc;
  }
# endif

# define CHECK if (++sCount > 8) { sCount=0; if (stopEvent()) {goto my_exit;}}
# define DECLARE_ABORT enableHandler();

#else

# include <signal.h>
  void my_handler(int sig);
# ifdef SOFTABORTMAIN
  void my_handler(int sig) {finishSoftabort = 1;}
# endif
# ifdef OSF
#   define CHECK if (docheck()) {goto my_exit;}
    int docheck() {if (finishSoftabort) return 1;  return 0;}
# else
#   define CHECK if (finishSoftabort) {goto my_exit;}
# endif
# define DECLARE_ABORT signal(SIGTERM, my_handler);
#endif

#endif
