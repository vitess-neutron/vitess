/*
Module : ShelExec.cpp
Purpose: Implementation of a simple ShellExecute wrapper app

Copyright (c) 1999 - 2003 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 
*/

////////////////////////// Includes ////////////////////////////////////////////////

#include "stdafx.h"
#include "mainfrm.h"
#include "shelexec.h"


////////////////////////// Macros //////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



////////////////////////// Implementation //////////////////////////////////////////

class CMyCommandLineInfo : public CCommandLineInfo
{
public:
//Constructors / Destructors
  CMyCommandLineInfo();

//Methods
  void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);

//Data
  CString m_sVerb;
  CString m_sDir;
  int     m_nCmdShow;
  BOOL    m_bExe;
  CString m_sParams;
  DWORD   m_dwTimeout;
  BOOL    m_bWait;
};

CMyCommandLineInfo::CMyCommandLineInfo()
{
  m_nCmdShow = SW_SHOW;
  m_bExe = FALSE;
  m_dwTimeout = 5000;
  m_bWait = FALSE;
}

void CMyCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
  if (bFlag)
  {
    CString sParam(lpszParam);
    CString sUpCaseParam(lpszParam);
    sUpCaseParam.MakeUpper();

    //Handle the various command line parameters we support
    int nFind = sUpCaseParam.Find(_T("SHOWCMD"));
    int nLength = sUpCaseParam.GetLength();
    if (nFind == 0 && nLength > 8)
    {
      CString sShowVal(sParam.Right(nLength - 8)); 
      m_nCmdShow = _ttoi(sShowVal);
    }
    else
    {
      nFind = sUpCaseParam.Find(_T("VERB"));
      if (nFind == 0 && nLength > 5)
        m_sVerb = sParam.Right(nLength - 5); 
      else
      {
        nFind = sUpCaseParam.Find(_T("DIR"));    
        if (nFind == 0 && nLength > 4)
          m_sDir = sParam.Right(nLength - 4); 
        else
        {
          nFind = sUpCaseParam.Find(_T("PARAMS"));
          if (nFind == 0 && nLength > 7)
            m_sParams = sParam.Right(nLength - 7); 
          else
          {
            nFind = sUpCaseParam.Find(_T("TIMEOUT"));
            if (nFind == 0 && nLength > 8)
            {
              CString sTimeout(sParam.Right(nLength - 8));
              m_dwTimeout = _ttoi(sTimeout); 
            }
            else
            {
              if (sUpCaseParam == _T("WAIT"))
                m_bWait = TRUE;
              else
              {
                if (!m_bExe)
                  m_bExe = (sParam == _T("EXE"));
                else
                  CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
              }
            }
          }
        }
      }
    }
  }
  else
    CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
}


BEGIN_MESSAGE_MAP(CShelExecApp, CWinApp)
	//{{AFX_MSG_MAP(CShelExecApp)
	//}}AFX_MSG
END_MESSAGE_MAP()

CShelExecApp::CShelExecApp()
{
}

CShelExecApp theApp;

BOOL CShelExecApp::InitInstance()
{
  //Parse the command line
	CMyCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
  if (cmdInfo.m_strFileName.IsEmpty())
  {
    AfxMessageBox(IDS_ERROR);
    return FALSE;
  }

  //Find the current Diretory and Drive for this instance
  TCHAR pszPath[_MAX_PATH];
  GetModuleFileName(NULL, pszPath, _MAX_PATH);
  TCHAR pszDrive[_MAX_DRIVE];
  TCHAR pszDir[_MAX_DIR];
  TCHAR pszFolder[_MAX_PATH];
  _tsplitpath(pszPath, pszDrive, pszDir, NULL, NULL);
  _tmakepath(pszFolder, pszDrive, pszDir, NULL, NULL);
  CString sFolder(pszFolder);
  int nLength = sFolder.GetLength();
  if (nLength && sFolder.GetAt(nLength-1) == _T('\\'))
    sFolder = sFolder.Left(nLength-1);
  CString sDrive(pszDrive);
  nLength = sDrive.GetLength();
  if (nLength && sDrive.GetAt(nLength-1) == _T('\\'))
    sDrive = sDrive.Left(nLength-1);

  //Replace any variables with their actual values as determined above
  cmdInfo.m_sDir.Replace(_T("@EXEDRV@"), sDrive);
  cmdInfo.m_sParams.Replace(_T("@EXEDRV@"), sDrive);
  cmdInfo.m_strFileName.Replace(_T("@EXEDRV@"), sDrive);

  cmdInfo.m_sDir.Replace(_T("@EXEDIR@"), sFolder);
  cmdInfo.m_sParams.Replace(_T("@EXEDIR@"), sFolder);
  cmdInfo.m_strFileName.Replace(_T("@EXEDIR@"), sFolder);

  cmdInfo.m_sDir.Replace(_T("@QUOTE@"), _T("\""));
  cmdInfo.m_sParams.Replace(_T("@QUOTE@"), _T("\""));
  cmdInfo.m_strFileName.Replace(_T("@QUOTE@"), _T("\""));


  //The structure which will be passed to ShellExecuteEx
  SHELLEXECUTEINFO sei;
  ZeroMemory(&sei,sizeof(sei));
  sei.cbSize = sizeof(sei);

  //Convert the filename to its short version if a verb is specified
  if (cmdInfo.m_sVerb.GetLength())
  {
    //convert to short filename format if we can
    TCHAR path_buffer[_MAX_PATH];
    if (GetShortPathName(cmdInfo.m_strFileName, path_buffer, _MAX_PATH) != 0)
      cmdInfo.m_strFileName = path_buffer;
  }

  //Setup the structure with common parameters
  sei.lpFile = cmdInfo.m_strFileName.GetBuffer(cmdInfo.m_strFileName.GetLength());
  if (!cmdInfo.m_bWait)
    sei.fMask  = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_DDEWAIT;
  sei.nShow = cmdInfo.m_nCmdShow;
  sei.hwnd = GetDesktopWindow();

  //Did we specify a working directory
  if (cmdInfo.m_sDir.GetLength())
    sei.lpDirectory = cmdInfo.m_sDir.GetBuffer(cmdInfo.m_sDir.GetLength());

  //Set any optional parameters
  if (cmdInfo.m_bExe)
  {
    if (cmdInfo.m_sParams.GetLength())
      sei.lpParameters = cmdInfo.m_sParams.GetBuffer(cmdInfo.m_sParams.GetLength());
  }
  else
  {
    if (cmdInfo.m_sVerb.GetLength())
      sei.lpVerb = cmdInfo.m_sVerb.GetBuffer(cmdInfo.m_sVerb.GetLength());
  }
  if (cmdInfo.m_bWait)
    sei.fMask |= SEE_MASK_NOCLOSEPROCESS;

  //Call ShellExecuteEx API
  BOOL bSuccess = ShellExecuteEx(&sei);

  //Handle any errors!
  if (!bSuccess || (sei.hInstApp <= (HINSTANCE)32))
  {
    CString sMsg;
    sMsg.Format(_T("Failed to run \"%s\", Error: %d"), cmdInfo.m_strFileName, GetLastError());
    AfxMessageBox(sMsg);
  }

  //Release all our CString buffers
  cmdInfo.m_strFileName.ReleaseBuffer();
  if (cmdInfo.m_sDir.GetLength())
    cmdInfo.m_sDir.ReleaseBuffer();
  if (cmdInfo.m_bExe)
  {
    if (cmdInfo.m_sParams.GetLength())
      cmdInfo.m_sParams.ReleaseBuffer();
  }
  else
  {
    if (cmdInfo.m_sVerb.GetLength())
      cmdInfo.m_sVerb.ReleaseBuffer();
  }

  if (cmdInfo.m_bWait)
    cmdInfo.m_bWait = (sei.hProcess != 0);

  //We need to start a message pump as some ShellExecute calls will use
  //our message pump to get the operation completed
  CMainFrame* pFrameWnd = new CMainFrame();
  if (cmdInfo.m_bWait)
    pFrameWnd->m_dwTimeout = 0; //A special value to mean that we do want to use a timer to exit
  else
    pFrameWnd->m_dwTimeout = cmdInfo.m_dwTimeout;
  VERIFY(pFrameWnd->LoadFrame(IDR_MAINFRAME));
  m_pMainWnd = pFrameWnd;
	m_pMainWnd->ShowWindow(SW_HIDE);
	m_pMainWnd->UpdateWindow();

  //Spin off the background thread which will do the exiting of the app
  if (cmdInfo.m_bWait && sei.hProcess)
    AfxBeginThread(_WaitForExitThread, sei.hProcess, THREAD_PRIORITY_IDLE);

	return TRUE;
}

UINT CShelExecApp::_WaitForExitThread(LPVOID pParam)
{
  //Wait for the thread to exit
  HANDLE hProcess = (HANDLE) pParam;
  WaitForSingleObject(hProcess, INFINITE);
  AfxGetMainWnd()->PostMessage(WM_CLOSE);

  return 0;
}
