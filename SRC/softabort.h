#ifndef SOFTABORT_H
#define SOFTABORT_H

#ifdef _MSC_VER

# include <windows.h>
# include <stdio.h>
  static const char *hName = "VitessStopHandle";
  static HANDLE hS;
  static sCount;
# define CHECK if (++sCount > 8) { sCount=0; if (stopEvent()) {goto my_exit;}}
# define DECLARE_ABORT enableHandler();
  void enableHandler() {
    hS = CreateEvent( NULL, FALSE, FALSE, hName);
    if (hS == NULL) {
      fprintf(LogFilePtr, "CreateEvent failed [%x]\n", GetLastError());
    }
    ResetEvent( hS);
  }
  int stopEvent () {
    int rc = WaitForSingleObject(hS, 0);
    if (rc == WAIT_FAILED) {
      fprintf(LogFilePtr,"WaitForSingleObject failed %d\n", rc);
      return 1;
    }
  return WAIT_OBJECT_0 == rc;
  }

#else

  static int finish = 0;
# include <signal.h>
  void my_handler(int sig) {finish = 1;}
# ifdef OSF
#   define CHECK if (docheck()) {goto my_exit;}
    int docheck() {if (finish) return 1;  return 0;}
# else
#   define CHECK if (finish) {goto my_exit;}
# endif
# define DECLARE_ABORT signal(SIGTERM, my_handler);
#endif

#endif
