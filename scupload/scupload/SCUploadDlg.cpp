// Copyright (c) 2012, SoundCloud Ltd.

// SCUploadDlg.cpp : implementation file

#include "stdafx.h"
#include "SCUploadApp.h"
#include "SCUploadDlg.h"
#include "CWebBrowser2.h"
#include "WebUtility.h"
#include <algorithm>
#include "UserProfile.h"
#include "SharingConnection.h"
#include "CustomMenus.h"
#include <afxtempl.h>
#include <Mshtml.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const int SCUploadDlg::BORDER = 20;

SCUploadDlg::SCUploadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SCUploadDlg::IDD, pParent)
	, m_pUserProfile(NULL)
{	
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);

	m_isStartup = TRUE;
	m_validArgs = FALSE;
	m_appState = SCUploadDlg::Started;
	m_pBrowser = NULL;

	m_OverrideBackground = FALSE;
	m_BackgroundColor = RGB(245, 245, 245);
	m_BackgroundBrush.CreateSolidBrush(m_BackgroundColor);

	m_Size1.SetSize(400, 500); // Auth / SUSI View
	m_Size2.SetSize(540, 370); // Upload config form and progress view
	m_Size3.SetSize(650, 390); // Facebook permission page size
}

void SCUploadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDACTION, m_ActionButton);
	DDX_Control(pDX, IDC_PROGRESS, m_ProgressCtrl);
	DDX_Control(pDX, IDC_STATUS, m_Status);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_OPENBUTTON, m_OpenButton);
	DDX_Control(pDX, IDC_LOGOUTBUTTON, m_LogOutButton);
	DDX_Control(pDX, IDC_USERNAME, m_Username);
	DDX_Control(pDX, IDC_TITLEEDITOR, m_TitleEditor);
	DDX_Control(pDX, IDC_TAGSEDITOR, m_TagEditor);
	DDX_Control(pDX, IDC_TITLELABEL, m_TitleLabel);
	DDX_Control(pDX, IDC_TAGLABEL, m_TagLabel);
	DDX_Control(pDX, IDC_SHARELABEL, m_SharingLabel);
	DDX_Control(pDX, IDC_RADIOPUBLIC, m_PublicRadio);
	DDX_Control(pDX, IDC_RADIOPRIVATE, m_PrivateRadio);
	DDX_Control(pDX, IDC_USAGE, m_Usage);
	DDX_Control(pDX, IDC_LICENSECOMBO, m_LicenseCombo);
	DDX_Control(pDX, IDC_PICTURE, m_PictureBox);
	DDX_Control(pDX, IDC_PIC_BUTTON, m_PictureButton);
	DDX_Control(pDX, IDC_METADATA, m_GroupBox);
	DDX_Control(pDX, IDC_LICENSELAB, m_LicenseLabel);
	DDX_Control(pDX, IDC_PICTURELABEL, m_PictureLabel);
	DDX_Control(pDX, IDC_SHARINGLIST, m_SharingList);
	DDX_Control(pDX, IDC_CONNECTIONLABEL, m_ConnectionsLabel);
	DDX_Control(pDX, IDC_PRIVATE_INFO2, m_PrivateInfo2);
	DDX_Control(pDX, IDC_PRIVATE_INFO1, m_PrivateInfo1);
	DDX_Control(pDX, IDC_LOGOUTBUTTON, m_LogOutButton);
	DDX_Control(pDX, IDC_PERMALINK, m_PermaLink);
	DDX_Control(pDX, IDC_STATUS2, m_Status2);
	DDX_Control(pDX, IDC_LICENSE_HELP, m_LicenseHelp);
	DDX_Control(pDX, IDC_AVATAR, m_Avatar);
}

const UINT WM_CUSTOM_CONTROLSITE_MSG  = RegisterWindowMessage(_T("CustomControlSiteMsg"));

BEGIN_EVENTSINK_MAP(SCUploadDlg, CWebBrowser2)
	//{{AFX_EVENTSINK_MAP(CBrowserPage)
	ON_EVENT(CWebBrowser2, IDWB, 271, OnNavigateError, VTS_DISPATCH VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PBOOL)
	ON_EVENT(CWebBrowser2, IDWB, 252/* NavigateComplete2 */, OnNavigateComplete, VTS_DISPATCH VTS_PVARIANT)
	ON_EVENT(CWebBrowser2, IDWB, 259 /* DocumentComplete */, OnDocumentComplete, VTS_DISPATCH VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

BEGIN_MESSAGE_MAP(SCUploadDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP

	ON_REGISTERED_MESSAGE(WM_CUSTOM_CONTROLSITE_MSG, OnCustomControlSiteMsg)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_MESSAGE (WM_USER_CONCHECK_COMPLETE, OnConnectionCheckComplete)
	ON_MESSAGE (WM_USER_SCME_COMPLETE, OnTokenVerificationComplete)
	ON_MESSAGE (WM_USER_SCPOST_COMPLETE, OnUploadComplete)
	ON_MESSAGE (WM_USER_SCPOST_PROGRESS, OnUploadProgress)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDACTION, &SCUploadDlg::OnBnClickedAction)
	ON_BN_CLICKED(IDC_OPENBUTTON, &SCUploadDlg::OnOpenButtonClicked)
	ON_BN_CLICKED(IDC_LOGOUTBUTTON, &SCUploadDlg::OnLogOutClicked)
	ON_BN_CLICKED(IDC_RADIOPUBLIC, &SCUploadDlg::OnBnClickedVisibility)
	ON_BN_CLICKED(IDC_RADIOPRIVATE, &SCUploadDlg::OnBnClickedVisibility)
	ON_NOTIFY(NM_CLICK, IDC_USAGE, &SCUploadDlg::OnNMClickLink)
	ON_NOTIFY(NM_CLICK, IDC_CONNECTIONLABEL, &SCUploadDlg::OnNMClickLink)
	ON_BN_CLICKED(IDC_PIC_BUTTON, &SCUploadDlg::OnBnClickedPicButton)
	ON_STN_DBLCLK(IDC_PICTURE, &SCUploadDlg::OnBnClickedPicButton)
	ON_NOTIFY(NM_CLICK, IDC_PERMALINK, &SCUploadDlg::OnNMClickLink)
	ON_EN_CHANGE(IDC_TITLEEDITOR, &SCUploadDlg::OnEnChangeTitleeditor)
	ON_STN_CLICKED(IDC_LICENSE_HELP, &SCUploadDlg::OnStnClickedLicenseHelp)
END_MESSAGE_MAP()


// Initializes the dialog
BOOL SCUploadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.
	SetIcon(m_hIcon, TRUE);	// Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon

	// Init bitmap for picture box
	CBitmap whiteCloudBitmap = CBitmap();
	whiteCloudBitmap.LoadBitmapW(IDB_BITMAP3);
	m_PictureBox.SetBitmap((HBITMAP)whiteCloudBitmap.Detach());

	m_ProgressCtrl.SetRange(0, 100);
	m_LicenseCombo.Init();

	return TRUE;
}

// Message handler when dialog was invalidated
void SCUploadDlg::OnPaint()
{
	CRect rect;
	GetClientRect(&rect);
	CPaintDC dc(this);

	if (IsIconic())
	{
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	//else if(m_appState < UploadConfig)
	//{
	//	BITMAP bitmap;
	//	m_Bitmap.GetBitmap(&bitmap);
	//	
	//	// Center logo in client rectangle
	//	int x = (rect.Width() - bitmap.bmWidth + 1) / 2;
	//	//int y = (rect.Height() - bitmap.bmHeight + 1) / 2; // Center
	//	int y = rect.Height() / 5;

	//	CPoint size(bitmap.bmWidth, bitmap.bmHeight);
	//	dc.DPtoLP(&size);
	//	CPoint org(0, 0);
	//	dc.DPtoLP(&org);

	//	CDC dcMem;
	//	dcMem.CreateCompatibleDC(&dc);
	//	CBitmap* pOldBitmap = dcMem.SelectObject(&m_Bitmap);
	//	dcMem.SetMapMode(dc.GetMapMode());

	//	dc.BitBlt(x, y, size.x, size.y, &dcMem, org.x, org.y, SRCCOPY);

	//	dcMem.SelectObject(pOldBitmap);
	//}
	else if(m_appState == UploadConfig)
	{
		CRect leftRect, rightRect;
		m_TitleEditor.GetWindowRect(&leftRect);
		ScreenToClient(&leftRect);
		m_ConnectionsLabel.GetWindowRect(&rightRect);
		ScreenToClient(&rightRect);
		int x0 = leftRect.right + (rightRect.left - leftRect.right) / 2;
		int y0 = leftRect.top;
		int y1 = rightRect.bottom;
		CPen pen = CPen(PS_SOLID, 1, RGB(184,184,184));
		dc.SelectObject(&pen);
		dc.MoveTo(x0, y0);
		dc.LineTo(x0, y1);
	}
	else
	{
		// Call base impl. when no drawing occurs
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display
// while the user drags the minimized window.
HCURSOR SCUploadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Paint the background
BOOL SCUploadDlg::OnEraseBkgnd(CDC* pDC)
{
	if(m_OverrideBackground)
	{
		CRect rect;
		GetClientRect(&rect);
		CBrush brush(m_BackgroundColor);
		CBrush *pOld = pDC->SelectObject(&brush);
		BOOL bRes  = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);
		pDC->SelectObject(pOld); // restore old brush
		return bRes;
	}
	else
	{
		return CDialog::OnEraseBkgnd(pDC);
	}
}

HBRUSH SCUploadDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if(!m_OverrideBackground)
		return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	HBRUSH hBrush = NULL;
	int ctrlId = pWnd->GetDlgCtrlID();
	if(	ctrlId == IDC_STATUS ||
		ctrlId == IDC_LOGOUTBUTTON ||
		ctrlId == IDC_USERNAME ||
		ctrlId == IDC_TITLELABEL ||
		ctrlId == IDC_TAGLABEL ||
		ctrlId == IDC_SHARELABEL ||
		ctrlId == IDC_FBCHECK ||
		ctrlId == IDC_TWITTERCHECK ||
		ctrlId == IDC_RADIOPUBLIC ||
		ctrlId == IDC_RADIOPRIVATE ||
		ctrlId == IDC_VISIBILITYLABEL ||
		ctrlId == IDC_USAGE ||
		ctrlId == IDCANCEL ||
		ctrlId == IDACTION ||
		ctrlId == IDC_OPENBUTTON ||
		ctrlId == IDC_LICENSELAB ||
		ctrlId == IDC_PICTURELABEL ||
		ctrlId == IDC_PIC_BUTTON ||
		ctrlId == IDC_CONNECTIONLABEL ||
		ctrlId == IDC_PRIVATE_INFO1 ||
		ctrlId == IDC_PRIVATE_INFO2)
	{
		pDC->SetTextColor(RGB(32, 32, 32));
		pDC->SetBkColor(m_BackgroundColor);
		hBrush = (HBRUSH)m_BackgroundBrush;
	}
	else
	{
		hBrush = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
	return hBrush;
}

// Control message handler: application window was activated
void SCUploadDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	UpdateWindowSize();
	if(bShow && m_isStartup)
	{
		if(!m_validArgs)
		{
			ShowUsage();
		}
		else
		{
			m_Connector.SetCallbackHandle(m_hWnd);
			m_Connector.VerifyToken();
			m_isStartup = false;
		}
	}
}

// Initializes the connection handler with the command-line arguments
void SCUploadDlg::SetCmdLineInfo(SCCommandLineInfo* cmdLineInfo)
{
	if(cmdLineInfo->IsValid())
	{
		m_Connector.SetCommandLineInfo(cmdLineInfo);
		m_validArgs = true;
	}
}

// Displays the given status message
void SCUploadDlg::ShowStatusMessage(LPCTSTR newStatus)
{
	SetDlgItemTextW(IDC_STATUS, newStatus);
}

// Message handler: processing result of token verification
long SCUploadDlg::OnTokenVerificationComplete(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
		case HTTP_STATUS_OK:
			m_appState = SCUploadDlg::UploadConfig;
			m_Connector.KeepToken();
			m_pUserProfile = reinterpret_cast<UserProfile*>(lParam);
			InitUploadConfig();
			UpdateDisplay(NULL);
			break;

		case HTTP_STATUS_FORBIDDEN:
		case HTTP_STATUS_DENIED:
			m_appState = SCUploadDlg::Started;
			ShowAuth();
			break;

		case SC_CONNECTION_FAILED:
		default:
			CString* pMessage = reinterpret_cast<CString*>(lParam);
			m_appState = SCUploadDlg::NotConnected;
			if(WebUtility::IsOfflineMode()) {
				UpdateDisplay(_T("Your internet connection is in offline mode."));
			} else if(pMessage != NULL) {
				UpdateDisplay(_T("Connection failed: " + *pMessage));
				delete pMessage;
			} else {
				UpdateDisplay(_T("Unknown error interrupted sign in."));
			}
	}
	return 0;
}

void SCUploadDlg::ShowAuth(void)
{
	UpdateDisplay(_T("Loading..."));
	if(!ShowAuthView())
	{
		ShowStatusMessage(_T("Failed to complete sign in."));
		AfxMessageBox(_T("Internet Explorer 6 SP2 or higher is required to sign in with SoundCloud."));
		EndDialog(-1);
	}
}

// Message handler: processing result of token verification
long SCUploadDlg::OnConnectionCheckComplete(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
		case HTTP_STATUS_OK:
			m_appState = Started;
			ShowAuth();
			break;

		default:
			m_appState = SCUploadDlg::NotConnected;
			CString* pMessage = reinterpret_cast<CString*>(lParam);
			if(WebUtility::IsOfflineMode()) {
				UpdateDisplay(_T("Your internet connection is in offline mode."));
			} else if(pMessage != NULL) {
				UpdateDisplay(_T("Connection failed: " + *pMessage));
				delete pMessage;
			} else {
				UpdateDisplay(_T("Unknown error interrupted sign in."));
			}
	}
	return 0;
}

// Event handler: the action button was clicked.
// The current app state is evaluated
void SCUploadDlg::OnBnClickedAction()
{
	if(m_appState < SCUploadDlg::VerifyingToken)
	{
		m_Connector.VerifyToken();
		return;
	}
	else if(m_appState == SCUploadDlg::UploadFailed)
	{
		m_appState = SCUploadDlg::UploadConfig;
		UpdateDisplay(NULL);
	}
	else if(m_appState < SCUploadDlg::UploadSucceeded)
	{
		m_appState = SCUploadDlg::Uploading;
		CArray<int, int&> connections = CArray<int, int&>();
		GetSelectedConnections(connections);
		m_Connector.SetTrackProperties(
			GetEditorText(m_TitleEditor),
			GetEditorText(m_TagEditor),
			m_LicenseCombo.GetSelected(),
			m_PublicRadio.GetCheck() == BST_CHECKED,
			connections);

		UpdateDisplay(_T("Uploading your track..."));
		m_Connector.UploadFile();
	}
}

// Browser event handler when page navigation occurred
void SCUploadDlg::OnNavigateComplete(LPDISPATCH pDisp, VARIANT* szUrl)
{
	ASSERT(szUrl);
	if(m_pBrowser->get_Offline())
	{
		m_appState = NotConnected;
		CloseAuthView();
		UpdateDisplay(_T("Your internet connection is in offline mode."));
		return;
	}

	CString url(*szUrl);
	CString* errorDescription = NULL;
	if(m_Connector.IsSusiUrl(url))
	{
		// The connect page has finished loading
		m_appState = SUSI;
	}
	else if(WebUtility::IsSusiCancelled(url))
	{
		m_appState = NotConnected;
		OnCancel();
	}
	else if(WebUtility::IsSusiError(url, errorDescription))
	{
		AfxMessageBox(*errorDescription);
		EndDialog(-1);
	}
	else if(WebUtility::IsFacebook(url))
	{
		m_appState = SUSI_FB;
	}
	else
	{
		//Parse the access_token from the redirect
		CString token = WebUtility::GetFieldValueFromUrl(url, _T("access_token"));
		
		if(token.IsEmpty() && m_appState == SUSI_FB)
		{
			CString returnTo = WebUtility::UrlDecode(
				WebUtility::GetFieldValueFromUrl(url, _T("returnTo")));
			token = WebUtility::GetFieldValueFromUrl(returnTo, _T("access_token"));
		}

		if(!token.IsEmpty())
		{
			m_Connector.SetAccessToken(token);
			SetDlgItemTextW(IDC_STATUS, _T(""));
			CloseAuthView();

			// Verify the access token
			if(!m_Connector.VerifyToken())
				OnTokenVerificationComplete(HTTP_STATUS_DENIED, 0);
			return;
		}
	}

	UpdateDisplay(NULL);
}

void SCUploadDlg::OnNavigateError(LPDISPATCH pDisp,
								  VARIANT *URL,
								  VARIANT *TargetFrameName,
								  VARIANT *StatusCode,
								  VARIANT_BOOL *Cancel)
{
	if((*StatusCode).vt != VT_I4)
		return; // Don't know what to do with that.
	long status = (*StatusCode).lVal;
	if(status < 100)
	{
		// Assuming we are offline if no valid status code is returned
		*Cancel = VARIANT_TRUE;
		m_appState = NotConnected;
		CloseAuthView();
		UpdateDisplay(_T("Cannot connect to SoundCloud. Are you offline?"));
	}
}

void SCUploadDlg::OnDocumentComplete(LPDISPATCH lpDisp, VARIANT FAR* URL)
{
	CString url(*URL);
	if(!WebUtility::IsFacebook(url))
		return;

	IUnknown* pUnk = m_pBrowser->GetControlUnknown();
	ASSERT(pUnk);

	LPDISPATCH lpWBDisp;
	HRESULT hr = pUnk->QueryInterface(IID_IDispatch, (void**)&lpWBDisp);
	if(FAILED(hr))
		return;

	if (lpDisp == lpWBDisp)
	{
		// This is the top-level window object, so document has been loaded
		IDispatch* pDocDisp = NULL;
		pDocDisp = m_pBrowser->get_Document();
		if (pDocDisp != NULL)
		{
			// Obtained the document object.
			IHTMLDocument3* pDoc = NULL;
			pDocDisp->QueryInterface(IID_IHTMLDocument3, (void**)&pDoc);
			if (pDocDisp != NULL)
			{
				IHTMLElement* pDocRootDisp = NULL;
				pDoc->get_documentElement(&pDocRootDisp);
				if(pDocRootDisp != NULL)
				{
					IHTMLElement2* pDocRoot = NULL;
					pDocRootDisp->QueryInterface(IID_IHTMLElement2, (void**)&pDocRoot);
					if(pDocRoot != NULL)
					{
						long h = NULL;
						long w = NULL;
						pDocRoot->get_scrollHeight(&h);
						pDocRoot->get_scrollWidth(&w);
						CString offsetMessage;
						offsetMessage.Format(_T("SCUploadDlg::OnDocumentComplete: scrollWidth=%d scrollHeight=%d\n"), w, h);
						OutputDebugString(offsetMessage);
						
						m_Size3 = CSize(w + 6, h + 6);
						UpdateWindowSize();
					}
				}
			}
			pDocDisp->Release();
		}
	}
	lpWBDisp->Release();
}

// Message callback, displaying the result of a completed upload
long SCUploadDlg::OnUploadComplete(WPARAM wParam, LPARAM lParam)
{
	CString* pMessage = reinterpret_cast<CString*>(lParam);
	CString statusMessage;

	switch(wParam)
	{
		case SC_CONNECTION_FAILED:
 			m_appState = SCUploadDlg::UploadFailed;
			statusMessage = _T("Upload failed: ") + *pMessage;
			break;

		case HTTP_STATUS_CREATED:
			m_appState = SCUploadDlg::UploadSucceeded;
			statusMessage = *pMessage;
			break;

		default:
			m_appState = SCUploadDlg::UploadFailed;
			if(pMessage != NULL)
				statusMessage.Format(_T("%s (%d)"), *pMessage, wParam);
			else
				statusMessage.Format(_T("The upload failed with error code %d"), wParam);
			break;
	}

	UpdateDisplay(statusMessage);
	return 0;
}

void SCUploadDlg::UpdateDisplay(CString statusMessage)
{
	CString title = m_Connector.GetTrackProperty(SCConnector::TRACK_TITLE);
	UpdateWindowTitle(title);
	BOOL notConnected = m_appState == NotConnected;
	switch(m_appState)
	{
	case Started:
	case NotConnected:
		m_GroupBox.ShowWindow(FALSE);
		m_Username.ShowWindow(FALSE);
		m_Avatar.ShowWindow(FALSE);
		m_LogOutButton.ShowWindow(FALSE);
		m_ProgressCtrl.ShowWindow(FALSE);
		ShowUploadConfig(FALSE);
		m_OpenButton.ShowWindow(FALSE);
		m_PermaLink.ShowWindow(FALSE);
		SetDlgItemText(IDACTION, _T("Retry"));
		SetDlgItemText(IDCANCEL, _T("Cancel"));
		m_CancelButton.ShowWindow(notConnected);
		m_ActionButton.ShowWindow(notConnected);
		if(notConnected)
			UpdateWindowSize();
		break;

	case SUSI_FB:
		break;

	case SUSI:
		m_Status.ShowWindow(FALSE);
		UpdateWindowSize();
		m_pBrowser->ShowWindow(TRUE);
		break;

	case UploadConfig:
		m_ProgressCtrl.ShowWindow(FALSE);
		m_Status.ShowWindow(FALSE);
		ShowUploadConfig(TRUE);
		m_GroupBox.ShowWindow(TRUE);
		m_Username.ShowWindow(TRUE);
		m_Avatar.ShowWindow(TRUE);
		m_LogOutButton.ShowWindow(TRUE);
		m_CancelButton.ShowWindow(TRUE);
		SetDefID(IDACTION);
		break;

	case Uploading:
		m_LogOutButton.EnableWindow(FALSE);
		ShowUploadConfig(FALSE);
		m_ProgressCtrl.ShowWindow(TRUE);
		statusMessage.Format(_T("Uploading \"%s\" ..."), title);
		SetDefID(IDCANCEL);
		Invalidate();
		break;

	case UploadFailed:
		m_LogOutButton.EnableWindow(TRUE);
		SetDlgItemText(IDACTION, _T("Retry"));
		m_ActionButton.ShowWindow(TRUE);
		m_ProgressCtrl.SetPos(0);
		SetDefID(IDACTION);
		break;

	case UploadSucceeded:
		m_LogOutButton.EnableWindow(TRUE);
		m_ActionButton.ShowWindow(FALSE);
		m_ProgressCtrl.ShowWindow(FALSE);
		SetDlgItemText(IDCANCEL, _T("Done"));
		SetDefID(IDCANCEL);

		ShowUploadCompleted(statusMessage);
		statusMessage.Format(_T("\"%s\" is now available at"), title);
		break;
	}

	if(m_appState < SUSI)
	{
		m_Status2.ShowWindow(FALSE);
		m_Status.SetWindowTextW(statusMessage);
		m_Status.ShowWindow(TRUE);
	}
	else if(m_appState > UploadConfig)
	{
		m_Status.ShowWindow(FALSE);

		CRect rect, wndRect, progRect;
		GetClientRect(&wndRect);
		m_Status2.GetWindowRect(&rect);
		ScreenToClient(&rect);
		m_ProgressCtrl.GetWindowRect(&progRect);
		ScreenToClient(&progRect);
		UINT textFlags = SWP_NOZORDER | SWP_SHOWWINDOW;

		if(m_appState == UploadSucceeded)
		{
			m_Status2.ModifyStyle(SS_LEFTNOWORDWRAP, SS_CENTER);
			m_Status2.SetWindowPos(NULL, BORDER, rect.top, wndRect.Width() - 2 * BORDER, rect.Height(), textFlags);
		}
		else
		{
			m_Status2.ModifyStyle(SS_CENTER, SS_LEFTNOWORDWRAP);
			m_Status2.SetWindowPos(NULL, progRect.left, rect.top, wndRect.Width() - (progRect.left + BORDER), rect.Height(), textFlags);
		}
		m_Status2.SetWindowTextW(statusMessage);
		m_Status2.Invalidate();
	}
	else
	{
		m_Status.ShowWindow(FALSE);
		m_Status2.ShowWindow(FALSE);
	}
}

// Message callback, displaying upload progress
long SCUploadDlg::OnUploadProgress(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam != 0);
	double div = (100.0 / lParam);
	int percentage = static_cast<int>(min(div * wParam, 100));
	m_ProgressCtrl.SetPos(percentage);
	
	return 0;
}


// Event handler: open button was clicked.
// Opens the url of the track in the default browser.
void SCUploadDlg::OnOpenButtonClicked()
{
	if(m_appState == SCUploadDlg::UploadSucceeded)
	{
		CString url;
		m_PermaLink.GetItemUrl(0, url);
		WebUtility::CopyToClipboard(m_hWnd, url);
	}
}

//
// Event handler: Log Out button was clicked.
// Resets the auth token and closes the app.
//
void SCUploadDlg::OnLogOutClicked()
{
	m_Connector.ResetToken();
	m_Connector.VerifyToken();
}

//
// Displays the authentication view
//
bool SCUploadDlg::ShowAuthView()
{
	// Create the browser active x control
	if(m_pBrowser == NULL)
		m_pBrowser = new CWebBrowser2();
	CRect rect;
	GetClientRect(&rect);
	if (!m_pBrowser->Create(NULL, NULL, WS_CHILD, rect, this, IDWB))
	{
		delete m_pBrowser;
		m_pBrowser = NULL;
		return false;
	}
    
	m_pBrowser->put_Silent(VARIANT_TRUE);

    // Navigate to the soundcloud auth page
    COleVariant noArg;
	CString url = m_Connector.GetSusiUrl();
	m_pBrowser->Navigate(url, &noArg, &noArg, &noArg, &noArg);
	return true;
}

// Closes the authentication view
bool SCUploadDlg::CloseAuthView()
{
	if(m_pBrowser != NULL)
		m_pBrowser->ShowWindow(FALSE);

	// TODO: Release the browser quickly and cleanly,
	//       the commented destroy call takes way too much time
	//       on the UI thread to be tolerable. For now, the browser
	//       is only hidden, but remains loaded in memory.
	// m_pBrowser->DestroyWindow();
	// delete m_pBrowser;
	// m_pBrowser = NULL;

	return true;
}

// Initializes the upload configuration form
void SCUploadDlg::InitUploadConfig(void)
{
	// Set the username
	SetDlgItemTextW(IDC_USERNAME, m_pUserProfile->GetName());

	// Set avatar
	if(m_pUserProfile->HasAvatar())
	{
		CString avatarFilePath = m_pUserProfile->GetAvatarFilePath();
		SetImage(avatarFilePath, m_Avatar);
	}

	// Set track properties
	CString title = m_Connector.GetTrackProperty(SCConnector::TRACK_TITLE);
	m_TitleEditor.SetWindowTextW(title);
	m_TitleEditor.SetSel(title.GetLength(), title.GetLength()); // Moves caret to end of string
	CString tags = m_Connector.GetTrackProperty(SCConnector::TRACK_TAGS);
	m_TagEditor.SetWindowTextW(tags);
	m_TagEditor.SetSel(tags.GetLength(), tags.GetLength());

	m_SharingList.ResetContent();
	CList<SharingConnection*>* connections = m_pUserProfile->GetConnections();
	for(POSITION i = connections->GetHeadPosition(); i != NULL;)
	{
        SharingConnection* connection = connections->GetNext(i);
		int listIndex = m_SharingList.AddString(connection->GetLabel());
		m_SharingList.SetItemData(listIndex, connection->GetId());
	}

	CString visibility = m_Connector.GetTrackProperty(SCConnector::TRACK_SHARING);
	m_PrivateRadio.SetCheck(0);
	m_PublicRadio.SetCheck(0);
	HWND radioButtonHwnd = NULL;
	radioButtonHwnd = visibility.MakeLower() == SCConnector::PUBLIC ?
		m_PublicRadio.m_hWnd :
		m_PrivateRadio.m_hWnd; // private by default
	::SendMessage(radioButtonHwnd, BM_CLICK, 0, 0x000A0005);

	CString license = m_Connector.GetTrackProperty(SCConnector::TRACK_LICENSE);
	m_LicenseCombo.Select(license);

	CString artworkPath = m_Connector.GetTrackProperty(SCConnector::ARTWORK_DATA);
	SetImage(artworkPath, m_PictureBox);
}

// Displays the track properties before upload
void SCUploadDlg::ShowUploadConfig(bool show)
{
	UpdateWindowSize();

	m_TitleLabel.ShowWindow(show);
	m_TagLabel.ShowWindow(show);
	m_LicenseLabel.ShowWindow(show);
	m_LicenseHelp.ShowWindow(show);
	m_TitleEditor.ShowWindow(show);
	m_TagEditor.ShowWindow(show);
	m_LicenseCombo.ShowWindow(show);
	m_PublicRadio.ShowWindow(show);
	m_PrivateRadio.ShowWindow(show);
	m_PictureLabel.ShowWindow(show);
	m_PictureBox.ShowWindow(show);
	m_PictureButton.ShowWindow(show);

	bool isPublic = m_PublicRadio.GetCheck() == BST_CHECKED;
	m_SharingLabel.ShowWindow(isPublic && show);
	m_SharingList.ShowWindow(isPublic && show);
	m_ConnectionsLabel.ShowWindow(isPublic && show);
	m_PrivateInfo1.ShowWindow(!isPublic && show);
	m_PrivateInfo2.ShowWindow(!isPublic && show);

	// Set buttons
	SetDlgItemTextW(IDACTION, _T("Upload"));
	m_ActionButton.ShowWindow(show);

	Invalidate();

	if(show)
		m_TitleEditor.SetFocus();
}

// Opens the given url in the default browser
void SCUploadDlg::OpenBrowser(const LPCTSTR url)
{
	::ShellExecute(m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
}

CString SCUploadDlg::GetEditorText(CEdit& editor)
{
	CString url;
	int l = editor.LineLength();
	editor.GetLine(0, url.GetBuffer(l), l);
	url.ReleaseBuffer(l);
	return url;
}

void SCUploadDlg::GetSelectedConnections(CArray<int, int&>& connections)
{
	for(int i = 0; i < m_SharingList.GetCount(); i++)
	{
		if(m_SharingList.GetCheck(i) > 0)
		{
			int connectionId = m_SharingList.GetItemData(i);
			connections.SetAtGrow(i, connectionId);
		}
	}
}

LRESULT SCUploadDlg::OnCustomControlSiteMsg(WPARAM wParam, LPARAM lParam)
{
LRESULT hasBeenHandled = FALSE;

	ASSERT((wParam > kCCSN_NoMessage) && (wParam < kCCSN_MessageLimit));

	switch (wParam)
	{
	case kCCSN_CreateSite:

		if (lParam != NULL)
		{
			kCCSN_CreateSiteParams *params = (kCCSN_CreateSiteParams *)lParam;

			if (params->pCtrlCont != NULL)
			{
				params->pSite = new CCustomControlSite(params->pCtrlCont);
				hasBeenHandled = TRUE;
			}
		}

		break;

	case kCCSN_ShowContextMenu:

		if (lParam != NULL)
		{
			WebContextMenuMode menuModeMap[] =
			{
				kDefaultMenuSupport,
				kNoContextMenu, 
				kTextSelectionOnly,
				kAllowAllButViewSource,
				kCustomMenuSupport
			};
			
			kCCSN_ShowContextMenuParams *params = (kCCSN_ShowContextMenuParams *)lParam;
			params->result = CustomShowContextMenu(kNoContextMenu, params->dwID, params->pptPosition, 
						params->pCommandTarget, params->pDispatchObjectHit);

			hasBeenHandled = TRUE;
		}

		break;
	}

	return hasBeenHandled;
}

void SCUploadDlg::OnBnClickedVisibility()
{
	bool isPublic = m_PublicRadio.GetCheck() == BST_CHECKED;
	m_SharingList.ShowWindow(isPublic);
	m_SharingLabel.ShowWindow(isPublic);
	m_ConnectionsLabel.ShowWindow(isPublic);
	m_PrivateInfo1.ShowWindow(!isPublic);
	m_PrivateInfo2.ShowWindow(!isPublic);
}

void SCUploadDlg::ShowUsage(void)
{
	m_Status.ShowWindow(FALSE);
	m_ActionButton.ShowWindow(FALSE);
	SetDlgItemText(IDCANCEL, _T("Done"));

	CString usageText = _T("Usage:\n\n");
	usageText += _T("soundcloud <filepath> -client_id:<id>\n\n");
	usageText += _T("Supported are any number of track properties in the form:\n-track[<property-name>]:<property-value>\n");
	usageText += _T("See <a href=\"http://developers.soundcloud.com/docs/api/tracks\">developers.soundcloud.com/docs/api/tracks</a>\n\n");
	usageText += _T("Your app's redirect URI must be set to:\n");
	usageText += _T("https://developers.soundcloud.com/callback.html");
	
	SetDlgItemText(IDC_USAGE, usageText);
	m_Usage.ShowWindow(TRUE);
}

void SCUploadDlg::OnNMClickLink(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pNMLink = (PNMLINK)pNMHDR;
	OpenBrowser(pNMLink->item.szUrl);
	*pResult = 0;
}
void SCUploadDlg::OnBnClickedPicButton()
{
	CString filter(_T("Image files|*.png;*.jpg;||"));
	CFileDialog fileDlg(TRUE, 0, 0, 0, filter);
	if( fileDlg.DoModal() == IDOK )
	{
		CString filePath = fileDlg.GetPathName();
		CFileStatus status;
		CFile::GetStatus(filePath, status);
		if(status.m_attribute & CFile::directory)
			return; // Directories should not be selectable
		
		m_Connector.SetTrackProperty(SCConnector::ARTWORK_DATA, filePath);

		SetImage(filePath, m_PictureBox);
	}
}

void SCUploadDlg::SetImage(CString& filePath, CStatic& control)
{
	if(!m_ArtworkImage.IsNull())
		m_ArtworkImage.Destroy();
	if(m_ArtworkImage.Load(filePath) != 0)
		return;

	CDC* dc = GetDC();
	CDC dcMem;
	dcMem.CreateCompatibleDC(dc);

	CSize size;
	RECT rect;
	int w = m_ArtworkImage.GetWidth();
	int h = m_ArtworkImage.GetHeight();
	int s = w > h ? h : w;
	int x = ((w > h ? w : h) - s) / 2;

	control.GetClientRect(&rect);
	size.SetSize(rect.right, rect.bottom);
	//control.ClientToScreen(&rect);
	//ScreenToClient(&rect);

	CBitmap artworkBitmap;
	artworkBitmap.CreateCompatibleBitmap(dc, size.cx, size.cy);
	CBitmap* oldBitmap = dcMem.SelectObject(&artworkBitmap);
	
	// Scale image
	dcMem.SetStretchBltMode(HALFTONE);
	m_ArtworkImage.StretchBlt(dcMem.m_hDC, 0, 0, size.cx, size.cy, x, 0, s, s, SRCCOPY);
	control.SetBitmap((HBITMAP)artworkBitmap.Detach());
	
	// Restore previous bitmap
	dcMem.SelectObject(oldBitmap);
	
	ReleaseDC(dc);

	control.Invalidate();
}

void SCUploadDlg::UpdateWindowSize(void)
{
	CRect rect;
	GetWindowRect(&rect);
	CSize* newSize;
	if(m_appState == NotConnected)
		newSize = &m_Size2;
	else if(m_appState < SUSI_FB)
		newSize = &m_Size1;
	else if(m_appState == SUSI_FB)
		newSize = &m_Size3;
	else
		newSize = &m_Size2;

	if(rect.Width() == newSize->cx && rect.Height() == newSize->cy)
		return;

	int dX = (rect.Width() - newSize->cx) / 2;
	int x = rect.left + dX;
	int y = rect.top;
	
	SetWindowPos(&CWnd::wndTop, x, y, newSize->cx, newSize->cy, SWP_SHOWWINDOW);
}

void SCUploadDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_Status.m_hWnd != NULL)
	{
		CRect rect;
		m_Status.GetWindowRect(&rect);
		ScreenToClient(&rect);
		int x = BORDER;
		int y = (cy / 2) - (rect.Height() / 2);
		int w = cx - (2 * BORDER);
		int h = rect.Height();
		m_Status.SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER);
	}

	if(m_appState < UploadConfig && m_pBrowser != NULL && m_pBrowser->m_hWnd != NULL)
	{
		m_pBrowser->MoveWindow(0, 0, cx, cy, FALSE);
		this->Invalidate();
	}

	if(m_appState >= UploadConfig)
	{
		// Calculate DPI-Aware Window Size
		CWnd* cBtn = GetDlgItem(IDCANCEL);
		CWnd* lBtn = GetDlgItem(IDC_LOGOUTBUTTON);
		if(cBtn != NULL && lBtn != NULL)
		{
			RECT clientRect, winRect;
			POINT delta;
			GetClientRect(&clientRect);
			GetWindowRect(&winRect);
			delta.x = (winRect.right - winRect.left) - clientRect.right;
			delta.y = (winRect.bottom - winRect.top) - clientRect.bottom;

			RECT cRect;
			cBtn->GetWindowRect(&cRect);
			ScreenToClient(&cRect);
			RECT lRect;
			lBtn->GetWindowRect(&lRect);
			ScreenToClient(&lRect);

			m_Size2.cx = cRect.right + lRect.top + delta.x;
			m_Size2.cy = cRect.bottom + lRect.top + delta.y;

			if(cx < m_Size2.cx || cy < m_Size2.cy)
				UpdateWindowSize();
		}
	}
}

void SCUploadDlg::OnEnChangeTitleeditor()
{
	CString newTitle;
	m_TitleEditor.GetWindowText(newTitle);
	UpdateWindowTitle(newTitle);
}

void SCUploadDlg::UpdateWindowTitle(const CString& title)
{
	CString windowTitle = _T("Share on SoundCloud");
	if(!title.IsEmpty())
		windowTitle.Format(_T("Share \"%s\" on SoundCloud"), title);
	SetWindowTextW(windowTitle);
}

void SCUploadDlg::OnStnClickedLicenseHelp()
{
	CString licenseHelpUrl = SCConnector::LICENSE_HELP_URL;
	OpenBrowser(licenseHelpUrl);
}

void SCUploadDlg::ShowUploadCompleted(const LPCTSTR url)
{
	// Measure link text on-screen size
	CDC* pDc = GetDC();
	CFont* pLinkFont = &afxGlobalData.fontDefaultGUIUnderline;
	CGdiObject *pOldFont = pDc->SelectObject(pLinkFont);
	CSize textSize = pDc->GetTextExtent(url);
	ReleaseDC(pDc);
	pDc->SelectObject(pOldFont);

	// Adjust control position
	CRect rect, wndRect;
	m_PermaLink.GetWindowRect(&rect);
	ScreenToClient(&rect);
	GetClientRect(&wndRect);
	int left = (textSize.cx >= wndRect.Width()) ? BORDER :
		((wndRect.Width() - textSize.cx) / 2);
	int width = wndRect.Width() - (left + BORDER);
	m_PermaLink.SetWindowPos(NULL, left, rect.top, width, rect.Height(), SWP_NOZORDER | SWP_SHOWWINDOW);

	// Set control content
	CString formattedUrl;
	formattedUrl.Format(_T("<a href=\"%s\">%s</a>"), url, url);
	m_PermaLink.SetWindowText(formattedUrl);
	
	m_OpenButton.GetWindowRect(&rect);
	ScreenToClient(&rect);
	left = (wndRect.Width() - rect.Width()) / 2;
	m_OpenButton.SetWindowPos(NULL, left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
}
