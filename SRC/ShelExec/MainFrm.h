class CMainFrame : public CFrameWnd
{
public:
//Constructors / Destructors
	CMainFrame();
	virtual ~CMainFrame();

//Data 
  UINT m_nTimerID;
  DWORD m_dwTimeout;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_DYNCREATE(CMainFrame)

	//{{AFX_VIRTUAL(CMainFrame)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

