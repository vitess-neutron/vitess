#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


class CShelExecApp : public CWinApp
{
public:
  CShelExecApp();

	//{{AFX_VIRTUAL(CShelExecApp)
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CShelExecApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  static UINT _WaitForExitThread(LPVOID pParam);
};

