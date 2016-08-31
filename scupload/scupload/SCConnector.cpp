// Copyright (c) 2013, SoundCloud Ltd.

// scconnector.cpp : implementation file

#include "stdafx.h"

#include "SCConnector.h"
#include "SCToken.h"
#include "MultipartPostMethod.h"
#include "GetMethod.h"
#include "UserProfile.h"
#include "FileUtility.h"
#include "UploadResult.h"

const CString SCConnector::AUTH_URL_TEMPLATE = _T("https://soundcloud.com/connect?scope=non-expiring&client_id=%s&redirect_uri=%s&response_type=token&display=popup");
const CString SCConnector::REDIRECT_URL = _T("https://developers.soundcloud.com/callback.html");
const CString SCConnector::PROFILE_URL = _T("me");
const CString SCConnector::CONNECTIONS_URL = _T("me/connections");
const CString SCConnector::AVATAR_URL = _T("me/avatar");
const CString SCConnector::TRACKS_URL = _T("/tracks");
const INTERNET_PORT SCConnector::PORT = 443;
const CString SCConnector::SERVER = _T("api.soundcloud.com");
const CString SCConnector::OAUTH_TOKEN = _T("oauth_token");
const CString SCConnector::ASSET_DATA = _T("track[asset_data]");
const CString SCConnector::ARTWORK_DATA = _T("track[artwork_data]");
const CString SCConnector::TRACK_TITLE = _T("track[title]");
const CString SCConnector::TRACK_TAGS = _T("track[tag_list]");
const CString SCConnector::TRACK_SHAREDTO = _T("track[shared_to][connections][][id]");
const CString SCConnector::TRACK_SHARING = _T("track[sharing]");
const CString SCConnector::TRACK_LICENSE = _T("track[license]");
const CString SCConnector::PRIVATE = _T("private");
const CString SCConnector::PUBLIC = _T("public");
const CString SCConnector::DEFAULT_TITLE = _T("Unnamed");
const CString SCConnector::LICENSE_HELP_URL = _T("http://help.soundcloud.com/customer/portal/topics/39593-legal/articles");

HWND SCConnector::m_hCallbackWnd;

SCConnector::SCConnector()
{
	m_CmdLineInfo = NULL;
	m_PersistentToken = NULL;
	m_UserAgent.Format(_T("User-Agent: Desktop-Sharing-Kit/1.0 (%s)\r\n"), GetOsId());
	m_pSession = new CInternetSession(
		_T("Desktop Sharing Kit"), 1, PRE_CONFIG_INTERNET_ACCESS,
		NULL, NULL, INTERNET_FLAG_DONT_CACHE);
}

SCConnector::~SCConnector()
{
	if(m_PersistentToken)
	{
		delete m_PersistentToken;
		m_PersistentToken = NULL;
	}

	m_pSession->Close();
	delete m_pSession;
}

CString SCConnector::GetOsId(void)
{
	CString osId;
	OSVERSIONINFOEX verInfo;
	ZeroMemory(&verInfo, sizeof(OSVERSIONINFOEX));
	verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	BOOL bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &verInfo);
	if(bOsVersionInfoEx != TRUE )
		return osId;
	osId.Format(_T("Win %d.%d.%d"), verInfo.dwMajorVersion, verInfo.dwMinorVersion, verInfo.dwBuildNumber);
	return osId;
}

//
// Initializes the parameters used to connect to soundcloud
//
void SCConnector::SetCommandLineInfo(SCCommandLineInfo* cmdLineInfo)
{
	m_CmdLineInfo = cmdLineInfo;
	m_PersistentToken = new SCToken(m_CmdLineInfo->m_sClientId);

	// Set new token if available
	if(!cmdLineInfo->m_sAccessToken.IsEmpty())
		m_PersistentToken->SetToken(cmdLineInfo->m_sAccessToken);
}

//
// Checks if the app can make a simple HEAD request. Async.
//
void SCConnector::IsConnected()
{
	URL_THREADPARAM* tParam = new URL_THREADPARAM;
	tParam->resx = static_cast<LPCTSTR>(_T(""));
	tParam->hWnd = m_hCallbackWnd;
	tParam->session = m_pSession;
	
	AfxBeginThread(SCConnector::RequestHead, tParam);
}

//
// Starts the validation of the given credentials asynchronously.
//
bool SCConnector::VerifyToken()
{
	if(m_PersistentToken->IsEmpty())
	{
		IsConnected();
		return true;
	}

	URL_THREADPARAM* tParam = new URL_THREADPARAM;
	tParam->resx = static_cast<LPCTSTR>(m_PersistentToken->GetToken());
	tParam->hWnd = m_hCallbackWnd;
	tParam->session = m_pSession;
	
	AfxBeginThread(SCConnector::RequestProfile, tParam);

	return true;
}

//
// Starts the upload of the given file asynchronously.
//
void SCConnector::UploadFile()
{
	ASSERT(!m_PersistentToken->IsEmpty());

	UPLOAD_THREADPARAM* tParam = new UPLOAD_THREADPARAM;
	tParam->token = static_cast<LPCTSTR>(m_PersistentToken->GetToken());
	tParam->assetData = static_cast<LPCTSTR>(m_CmdLineInfo->m_sAssetData);
	tParam->artworkData = static_cast<LPCTSTR>(m_CmdLineInfo->m_sArtworkData);
	tParam->trackProperties = &m_CmdLineInfo->m_mTrackProperties;
	tParam->hWnd = m_hCallbackWnd;
	tParam->session = m_pSession;
	tParam->userAgent = &m_UserAgent;
	
	AfxBeginThread(SCConnector::PostFile, tParam);
}

//
// Sends a HEAD request to the user profile page.
// Blocking call. Posts updates to the UI thread.
//
UINT SCConnector::RequestHead(LPVOID pParam)
{
	URL_THREADPARAM* tParam = (URL_THREADPARAM*) pParam;
	CString resx = tParam->resx;
	HWND hWnd = tParam->hWnd;
	CInternetSession* session = tParam->session;
	delete tParam;

	CString* pMessage = NULL;
	CHttpConnection* pCon = NULL;
	CHttpFile* pFile = NULL;
	DWORD resultCode = 0;
	try
	{
		pCon = session->GetHttpConnection(_T("soundcloud.com"));
		pFile = pCon->OpenRequest(CHttpConnection::HTTP_VERB_HEAD, resx, 0, 1, 0, 0, INTERNET_FLAG_RELOAD);
		pFile->SendRequest();
		pFile->QueryInfoStatusCode(resultCode);
	}
	catch (CInternetException* pEx)
	{
		resultCode = SC_CONNECTION_FAILED;
		TCHAR pError[64];
		pEx->GetErrorMessage(pError, 64);
		pMessage = new CString(pError);
		pEx->Delete();
	}
	
	if(pCon != NULL)
	{
		pCon->Close();
		delete pCon;
		if(pFile != NULL)
		{
			pFile->Close();
			delete pFile;
		}
	}
	
	LPARAM lParam = 0;
	if(resultCode != HTTP_STATUS_OK)
		lParam = reinterpret_cast<LPARAM>(pMessage);
	::PostMessage(hWnd, WM_USER_CONCHECK_COMPLETE, resultCode, lParam);
	return 0;
}

//
// Sends a GET request to the user profile page.
// Blocking call. Posts updates to the UI thread.
//
UINT SCConnector::RequestProfile(LPVOID pParam)
{
	URL_THREADPARAM* tParam = (URL_THREADPARAM*) pParam;
	CString token = tParam->resx;
	HWND hWnd = tParam->hWnd;
	CInternetSession* pSession = tParam->session;
	delete tParam;

	CString* pMessage = NULL;
	CString* pResponse = NULL;
	DWORD resultCode = 0;
	CHttpConnection* pCon = NULL;
	GetMethod getMethod;
	UserProfile* pUserProfile = NULL;
	try
	{
		pCon = pSession->GetHttpConnection(SERVER, PORT);
		
		// Get basic profile info
		CString url;
		CString urlFormat("%s?%s=%s");
		url.Format(urlFormat, PROFILE_URL, OAUTH_TOKEN, token);
		resultCode = getMethod.SendRequest(pCon, url, pResponse);
		if(resultCode == HTTP_STATUS_OK)
		{
			pUserProfile = new UserProfile(pResponse);
			if(pResponse != NULL)
				delete pResponse;

			// Get profile's sharing connections
			pResponse = NULL;
			try
			{
				url.Format(urlFormat, CONNECTIONS_URL, OAUTH_TOKEN, token);
				int result = getMethod.SendRequest(pCon, url, pResponse);
				if(result == HTTP_STATUS_OK)
				{
					pUserProfile->SetConnections(pResponse);
					if(pResponse != NULL)
						delete pResponse;
				}
			}
			catch(CException* pEx)
			{
				OutputDebugString(_T("Failed to retrieve sharing connections from " + url));
				OutputExceptionMessage(pEx);
				pEx->Delete();
			}

			// Get avatar image
			CString* pPath = NULL;
			if(getMethod.DownloadFile(pSession, pUserProfile->GetAvatarUrl(), pPath))
				pUserProfile->SetAvatarFilePath(*pPath);
			if(pPath != NULL)
				delete pPath;
		}
	}
	catch (CException* pEx)
	{
		resultCode = SC_CONNECTION_FAILED;

		TCHAR pError[64];
		pEx->GetErrorMessage(pError, 64);
		pMessage = new CString(pError);
		pEx->Delete();
	}
	
	if(pCon != NULL)
	{
		pCon->Close();
		delete pCon;
	}
	
	LPARAM lParam = (resultCode == HTTP_STATUS_OK) ?
		reinterpret_cast<LPARAM>(pUserProfile) :
		reinterpret_cast<LPARAM>(pMessage);
	::PostMessage(hWnd, WM_USER_SCME_COMPLETE, resultCode, lParam);
	return 0;
}

//
// Sets the given window handle.
//
void SCConnector::SetCallbackHandle(HWND hWnd)
{
	m_hCallbackWnd = hWnd;
}

//
// Posts an upload progress message to the window
//
void SCConnector::UploadProgress(DWORD progress, DWORD total)
{
	::PostMessage(m_hCallbackWnd, WM_USER_SCPOST_PROGRESS, progress, total);
}

//
// Constructs and sends a multipart/form-data POST request,
// appending the parameters from the UPLOAD_THREADPARAM struct,
// and posts the permanent url to the asset as windows message.
// It posts windows messages to the main thread for UI updates.
// Blocking / Synchronous
//
UINT SCConnector::PostFile(LPVOID pParam)
{
	UPLOAD_THREADPARAM* tParam = (UPLOAD_THREADPARAM*) pParam;
	CInternetSession* session = tParam->session;
	CHttpConnection* pCon = NULL;

	DWORD resultCode = 0;
	CString* pResponse = NULL;
	CString* pMessage = NULL;
	DWORD dwFlags = INTERNET_FLAG_RELOAD |
					INTERNET_FLAG_DONT_CACHE |
					INTERNET_FLAG_SECURE |
					INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
					INTERNET_FLAG_KEEP_CONNECTION;
	
	MultipartPostMethod postMethod(SCConnector::UploadProgress, *tParam->userAgent);

	try
	{
		postMethod.AddTextPart(SCConnector::OAUTH_TOKEN, tParam->token);
		postMethod.AddTextParts(tParam->trackProperties);
		
		int fileError = postMethod.AddBinaryPart(SCConnector::ASSET_DATA, tParam->assetData);
		if(fileError != 0)
			AfxThrowFileException(fileError);
		
		postMethod.AddBinaryPart(SCConnector::ARTWORK_DATA, tParam->artworkData);

		pCon = session->GetHttpConnection(SERVER, dwFlags, PORT);
		pCon->SetOption(INTERNET_OPTION_SEND_TIMEOUT, 0x5265C00);
		pCon->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 0x5265C00);
		resultCode = postMethod.SendRequest(pCon, TRACKS_URL, dwFlags, pResponse);
		
		pMessage = GetMessageFromResponse(resultCode, pResponse);
	}
	catch(CFileException* e)
	{
		resultCode = SC_FAIL_ERROR;
		if( e->m_cause == CFileException::badPath)
			pMessage = new CString(_T("No file was specified."));
		else if( e->m_cause == CFileException::fileNotFound )
			pMessage = new CString(_T("Cannot find the file."));
		else if(e->m_cause == CFileException::sharingViolation)
			pMessage = new CString(_T("The file is being used by another process."));
		else if(e->m_cause == CFileException::accessDenied)
			pMessage = new CString(_T("You do not have permission to access the file."));
		else
			pMessage = new CString(_T("There was a problem reading the file."));
		e->Delete();
	}
	catch (CException* ex)
	{
		resultCode = SC_CONNECTION_FAILED;
		TCHAR   exMsg[255];
		ex->GetErrorMessage(exMsg, 255);
		pMessage = new CString(exMsg);
		ex->Delete();
	}
	
	postMethod.Close();

	delete tParam;
	
	if(pResponse != NULL)
		delete pResponse;
	
	if(pCon != NULL)
	{
		try { pCon->Close(); }
		catch (CException*) { }
		delete pCon;
	}
	
	::PostMessage(m_hCallbackWnd,
				  WM_USER_SCPOST_COMPLETE,
				  resultCode,
				  reinterpret_cast<LPARAM>(pMessage));
	return 0;
}

CString* SCConnector::GetMessageFromResponse(DWORD resultCode, CString* pResponse)
{
	if(pResponse == NULL)
		return NULL;

	if(resultCode == MultipartPostMethod::ENCODING_ERROR)
		return new CString(*pResponse); // the plain error message

	CString* pMessage = NULL;
	UploadResult pResult(pResponse);
	if(400 <= resultCode || pResult.HasError())
	{
		pMessage = pResult.GetErrorMessage();
	}
	else if(201 == resultCode)
	{
		pMessage = pResult.GetPermaLink();
	}
	else
	{
		CString logMessage;
		logMessage.Format(_T("Unexpected upload response: %d"), resultCode);
		OutputDebugString(logMessage);
		OutputDebugString(*pResponse);
	}

	return pMessage;
}

// Sets the access token temporarily
int SCConnector::SetAccessToken(const CString& accessToken)
{
	m_PersistentToken->SetToken(accessToken);
	return 0;
}

// Returns true if the given url matches the authentication page url
bool SCConnector::IsSusiUrl(const CString& url)
{
	int i = 0;
	return url.Find(AUTH_URL_TEMPLATE.Tokenize(_T("?"), i), 0) == 0;
}

// Returns the complete authentication page url
CString SCConnector::GetSusiUrl(void)
{
	ASSERT(m_CmdLineInfo);

	CString url;
	url.Format(AUTH_URL_TEMPLATE, m_CmdLineInfo->m_sClientId, REDIRECT_URL);
	return url;
}

// Saves the currently assigned token to a file
void SCConnector::KeepToken(void)
{
	m_PersistentToken->Save();
}

// Removes the current token from the file
void SCConnector::ResetToken(void)
{
	// Delete persistent token
	m_PersistentToken->Reset();
	
	// Clear cookies of current session
	m_pSession->SetOption(INTERNET_OPTION_END_BROWSER_SESSION, NULL);
}

// Sets a track property. Will override the value of an existing key.
void SCConnector::SetTrackProperty(LPCTSTR key, LPCTSTR data)
{
	if(key == ARTWORK_DATA)
		m_CmdLineInfo->m_sArtworkData = data;
	else
		m_CmdLineInfo->m_mTrackProperties.SetAt(key, data);
}

// Removes the track property with the given key.
void SCConnector::RemoveTrackProperty(LPCTSTR key)
{
	m_CmdLineInfo->m_mTrackProperties.RemoveKey(key);
}

// Returns the track property specified by the given key.
CString SCConnector::GetTrackProperty(LPCTSTR key)
{
	if(key == ARTWORK_DATA)
		return m_CmdLineInfo->m_sArtworkData;
	return m_CmdLineInfo->m_mTrackProperties[key];
}

void SCConnector::SetTrackProperties(CString title, CString tags, CString license, bool isPublic, const CArray<int, int&>& connections)
{
	if(title.IsEmpty())
		title = DEFAULT_TITLE;
	SetTrackProperty(TRACK_TITLE, title);

	if(!tags.IsEmpty())
		SetTrackProperty(TRACK_TAGS, tags);
	else
		RemoveTrackProperty(TRACK_TAGS);

	if(!license.IsEmpty())
		SetTrackProperty(TRACK_LICENSE, license);
	else
		RemoveTrackProperty(TRACK_LICENSE);

	// Track specific sharing settings
	for(int i = 0; i < connections.GetSize(); i++)
	{
		int connectionId = connections.GetAt(i);
		if(connectionId < 0)
			continue; // Invalid ID!
		CString id;
		id.Format(_T("%d"), connectionId);
		SetTrackProperty(SCConnector::TRACK_SHAREDTO, id);
	}

	// Track visibility
	SetTrackProperty(TRACK_SHARING, isPublic ? PUBLIC : PRIVATE);
}
