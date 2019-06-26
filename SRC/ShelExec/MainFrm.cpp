#include "stdafx.h"
#include "resource.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
{
  m_nTimerID = 0;
  m_dwTimeout = 5000;
}

CMainFrame::~CMainFrame()
{
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  //Let the parent class do its thing
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//Create a timer which will close the app down (if we have to)
  if (m_dwTimeout)
    m_nTimerID = SetTimer(1, m_dwTimeout, NULL);
	
	return 0;
}

void CMainFrame::OnTimer(UINT /*nIDEvent*/) 
{
  KillTimer(m_nTimerID);
  PostMessage(WM_CLOSE);
}
