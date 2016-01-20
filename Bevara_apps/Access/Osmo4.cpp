// GPAC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Osmo4.h"
#include <gpac/network.h>
#include <direct.h>
#include "MainFrm.h"
#include "OpenUrl.h"
#include "resource.h"
#include "HtmlHelp.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Osmo4

BEGIN_MESSAGE_MAP(Osmo4, CWinApp)
	//{{AFX_MSG_MAP(Osmo4)
	ON_COMMAND(ID_OPEN_FILE, OnOpenFile)
	ON_COMMAND(ID_FILE_STEP, OnFileStep)
	ON_COMMAND(ID_OPEN_URL, OnOpenUrl)
	ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
	ON_COMMAND(ID_CONFIG_RELOAD, OnConfigReload)
	ON_COMMAND(ID_FILE_PLAY, OnFilePlay)
	ON_UPDATE_COMMAND_UI(ID_FILE_PLAY, OnUpdateFilePlay)
	ON_UPDATE_COMMAND_UI(ID_FILE_STEP, OnUpdateFileStep)
	ON_COMMAND(ID_FILE_STOP, OnFileStop)
	ON_UPDATE_COMMAND_UI(ID_FILE_STOP, OnUpdateFileStop)
	ON_COMMAND(ID_EXTRACT_DATA, OnFileExtract)
	ON_UPDATE_COMMAND_UI(ID_EXTRACT_DATA, OnUpdateFileExtract)
	ON_COMMAND(ID_SWITCH_RENDER, OnSwitchRender)
	ON_UPDATE_COMMAND_UI(ID_FILE_RELOAD, OnUpdateFileStop)
	ON_COMMAND(ID_HELP_ABOUT, OnAbout)
	ON_COMMAND(ID_HELP_BETATESTFEEDBACK, OnBeta)
	ON_COMMAND(ID_HELP_CONTACT, OnHelpContact)
	ON_COMMAND(ID_FILE_MIGRATE, OnFileMigrate)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_FINDER, &OnHelpFinder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Osmo4 construction

Osmo4::Osmo4()
{
	EnableHtmlHelp();
	media = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only Osmo4 object

Osmo4 theApp;



class UserPassDialog : public CDialog
{
// Construction
public:
	UserPassDialog(CWnd* pParent = NULL);   // standard constructor

	Bool GetPassword(const wchar_t *site_url, const wchar_t *user, char *password);

// Dialog Data
	//{{AFX_DATA(UserPassDialog)
	enum { IDD = IDD_PASSWD };
	CStatic	m_SiteURL;
	CEdit m_User;
	CEdit m_Pass;
	//}}AFX_DATA

	void SetSelection(u32 sel);
	char cur_ext[200], cur_mime[200];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UserPassDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	const wchar_t *m_site_url;
	wchar_t *m_user, *m_password;

	// Generated message map functions
	//{{AFX_MSG(UserPassDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
};

UserPassDialog::UserPassDialog(CWnd* pParent /*=NULL*/)
	: CDialog(UserPassDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptStream)
	//}}AFX_DATA_INIT
}

void UserPassDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(UserPassDialog)
	DDX_Control(pDX, IDC_TXT_SITE, m_SiteURL);
	DDX_Control(pDX, IDC_EDIT_USER, m_User);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_Pass);
	//}}AFX_DATA_MAP
}

BOOL UserPassDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_SiteURL.SetWindowText(m_site_url);
	m_User.SetWindowText(m_user);
	m_Pass.SetWindowText(_T(""));
	return TRUE;
}

void UserPassDialog::OnClose()
{
	m_User.GetWindowText(m_user, 50);
	m_Pass.GetWindowText(m_password, 50);
}

Bool UserPassDialog::GetPassword(const wchar_t *site_url, const wchar_t *user, char *password)
{
	m_site_url = site_url;
	m_user = (wchar_t *)user;
	if (DoModal() != IDOK) return GF_FALSE;
	return GF_TRUE;
}


static void Osmo4_progress_cbk(const void *usr, const char *title, u64 done, u64 total)
{
	if (!total) return;
	CMainFrame *pFrame = (CMainFrame *) ((Osmo4 *) usr)->m_pMainWnd;
	s32 prog = (s32) ( (100 * (u64)done) / total);
	if (pFrame->m_last_prog < prog) {
		pFrame->console_err = GF_OK;
		pFrame->m_last_prog = prog;
		pFrame->console_message.Format(_T("%s %02d %%"), title, prog);
		pFrame->PostMessage(WM_CONSOLEMSG, 0, 0);
		if (done==total) pFrame->m_last_prog = -1;
	}
}

#define W32_MIN_WIDTH 120

static void log_msg(wchar_t *msg)
{
	::MessageBox(NULL, msg, _T("GPAC"), MB_OK);
}
Bool Osmo4_EventProc(void *priv, GF_Event *evt)
{
	u32 dur;
	Osmo4 *gpac = (Osmo4 *) priv;
	CMainFrame *pFrame = (CMainFrame *) gpac->m_pMainWnd;
	/*shutdown*/
	if (!pFrame) return GF_FALSE;
	CT2A url_ch(pFrame->m_pPlayList->GetURL());

	switch (evt->type) {
	case GF_EVENT_DURATION:
		
		dur = (u32) (1000 * evt->duration.duration);
		if (dur){
			pFrame->SetPlayLayout();
		}
		//if (dur<1100) dur = 0;
		pFrame->m_pPlayList->SetDuration((u32) evt->duration.duration );
		gpac->max_duration = dur;
		gpac->can_seek = evt->duration.can_seek;
		if (!gpac->can_seek) {
			pFrame->m_Sliders.m_PosSlider.EnableWindow(FALSE);
		} else {
			pFrame->m_Sliders.m_PosSlider.EnableWindow(TRUE);
			pFrame->m_Sliders.m_PosSlider.SetRangeMin(0);
			pFrame->m_Sliders.m_PosSlider.SetRangeMax(dur);
		}

		break;

	case GF_EVENT_MESSAGE:
			if (!evt->message.service || !strcmp(evt->message.service, (LPCSTR)url_ch)) {
			pFrame->console_service = "main service";
		} else {
			pFrame->console_service = evt->message.service;
		}
		if (evt->message.error!=GF_OK) {
			if (evt->message.error<GF_OK || !gpac->m_NoConsole) {
				pFrame->console_err = evt->message.error;
				pFrame->console_message = evt->message.message;
				gpac->m_pMainWnd->PostMessage(WM_CONSOLEMSG, 0, 0);

				/*any error before connection confirm is a service connection error*/
				if (!gpac->m_isopen) pFrame->m_pPlayList->SetDead();
			}
			return GF_FALSE;
		}
		if (gpac->m_NoConsole) return GF_FALSE;

		/*process user message*/
		pFrame->console_err = GF_OK;
		pFrame->console_message = evt->message.message;
		gpac->m_pMainWnd->PostMessage(WM_CONSOLEMSG, 0, 0);
		break;
	case GF_EVENT_PROGRESS:
		char *szType;
		if (evt->progress.progress_type==0) szType = "Buffer ";
		else if (evt->progress.progress_type==1) szType = "Download ";
		else if (evt->progress.progress_type==2) szType = "Import ";
		gf_set_progress(szType, evt->progress.done, evt->progress.total);
		break;
	case GF_EVENT_NAVIGATE_INFO:
		pFrame->console_message = evt->navigate.to_url;
		gpac->m_pMainWnd->PostMessage(WM_CONSOLEMSG, 1000, 0);
		break;

	case GF_EVENT_SCENE_SIZE:
		if (evt->size.width && evt->size.height) {
			gpac->orig_width = evt->size.width;
			gpac->orig_height = evt->size.height;
			if (gpac->m_term && !pFrame->m_bFullScreen) 
				pFrame->PostMessage(WM_SETSIZE, evt->size.width, evt->size.height);
		}
		break;
	/*don't resize on win32 msg notif*/
#if 0
	case GF_EVENT_SIZE:
		if (/*gpac->m_term && !pFrame->m_bFullScreen && */gpac->orig_width && (evt->size.width < W32_MIN_WIDTH) )
			pFrame->PostMessage(WM_SETSIZE, W32_MIN_WIDTH, (W32_MIN_WIDTH*gpac->orig_height) / gpac->orig_width);
		else
			pFrame->PostMessage(WM_SETSIZE, evt->size.width, evt->size.height);
		break;
#endif

	case GF_EVENT_CONNECT:
//		if (pFrame->m_bStartupFile) return 0;
		// Maja commented out buildstream list since menu item discarded
		//pFrame->BuildStreamList(GF_TRUE);
		if (evt->connect.is_connected) {
			//pFrame->BuildChapterList(GF_FALSE);
			gpac->m_isopen = GF_TRUE;
			//resetting sliders when opening a new file creates a deadlock on the window thread which is disconnecting
			// Maja changed: must change when change toolbar
			//pFrame->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, gpac->m_isopen ? 4 : 3);
			//pFrame->m_wndToolBar.SetButtonInfo(2, ID_FILE_PLAY, TBBS_BUTTON, gpac->m_isopen ? 4 : 3);
			pFrame->m_Sliders.m_PosSlider.SetPos(0);
			pFrame->SetProgTimer(GF_TRUE);
		} else {
			gpac->max_duration = 0;
			gpac->m_isopen = GF_FALSE;
			//pFrame->BuildChapterList(GF_TRUE);
		}
		if (!pFrame->m_bFullScreen) {
			pFrame->SetFocus();
			pFrame->SetForegroundWindow();
		}
		break;

	case GF_EVENT_QUIT:
		pFrame->PostMessage(WM_CLOSE, 0L, 0L);
		break;
	case GF_EVENT_MIGRATE:
	{
	}
		break;
	case GF_EVENT_KEYDOWN:
		gf_term_process_shortcut(gpac->m_term, evt);
		/*update volume control*/
		pFrame->m_Sliders.SetVolume();

		switch (evt->key.key_code) {
		case GF_KEY_HOME:
			gf_term_set_option(gpac->m_term, GF_OPT_NAVIGATION_TYPE, 1);
			break;
		case GF_KEY_ESCAPE:
			pFrame->PostMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
			break;
		case GF_KEY_MEDIANEXTTRACK:
			pFrame->m_pPlayList->PlayNext();
			break;
		case GF_KEY_MEDIAPREVIOUSTRACK:
			pFrame->m_pPlayList->PlayPrev();
			break;
		case GF_KEY_H:
			if ((evt->key.flags & GF_KEY_MOD_CTRL) && gpac->m_isopen)
				gf_term_switch_quality(gpac->m_term, GF_TRUE);
			break;
		case GF_KEY_L:
			if ((evt->key.flags & GF_KEY_MOD_CTRL) && gpac->m_isopen)
				gf_term_switch_quality(gpac->m_term, GF_FALSE);
			break;
		case GF_KEY_LEFT:
		case GF_KEY_RIGHT:
			if (gpac->m_isopen && (gf_term_get_option(gpac->m_term, GF_OPT_NAVIGATION) == GF_NAVIGATE_NONE)) {
				if (evt->key.flags & GF_KEY_MOD_CTRL) {
					if (evt->key.key_code==GF_KEY_LEFT) pFrame->m_pPlayList->PlayPrev();
					else if (evt->key.key_code==GF_KEY_RIGHT) pFrame->m_pPlayList->PlayNext();
				}
				else if (gpac->can_seek && (evt->key.flags & GF_KEY_MOD_ALT)) {
					u32 duration = gpac->max_duration;
					s32 current_time = gf_term_get_time_in_ms(gpac->m_term);

					if (evt->key.key_code==GF_KEY_LEFT) {
						current_time -= 5*duration/100;
						if (current_time<0) current_time=0;
						gf_term_play_from_time(gpac->m_term, (u64) current_time, 0);
					}
					else if (evt->key.key_code==GF_KEY_RIGHT) {
						current_time += 5*duration/100;
						if ((u32) current_time < duration) {
							gf_term_play_from_time(gpac->m_term, (u64) current_time, 0);
						}
					}
				}
			}
			break;
		}
		break;
	case GF_EVENT_NAVIGATE:
		/*fixme - a proper browser would require checking mime type & co*/
		/*store URL since it may be destroyed, and post message*/
		gpac->m_navigate_url = evt->navigate.to_url;
		pFrame->PostMessage(WM_NAVIGATE, NULL, NULL);
		return GF_TRUE;
	case GF_EVENT_VIEWPOINTS:
		pFrame->BuildViewList();
		return GF_FALSE;
	case GF_EVENT_STREAMLIST:
		//pFrame->BuildStreamList(GF_FALSE);
		return GF_FALSE;
	case GF_EVENT_SET_CAPTION:
		pFrame->SetWindowText(CString(evt->caption.caption));
		break;
	case GF_EVENT_DBLCLICK:
		pFrame->PostMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
		return GF_FALSE;
	case GF_EVENT_AUTHORIZATION:
	{
		UserPassDialog passdlg;
		return passdlg.GetPassword(CString(evt->auth.site_url), CString(evt->auth.user), evt->auth.password);
	}
	}
	return GF_FALSE;
}


/*here's the trick: use a storage section shared among all processes for the wnd handle and for the command line
NOTE: this has to be static memory of course, don't try to alloc anything there...*/
#pragma comment(linker, "/SECTION:.shr,RWS") 
#pragma data_seg(".shr") 
HWND static_gpac_hwnd = NULL; 
char static_szCmdLine[MAX_PATH] = "";
#pragma data_seg() 

const char *static_gpac_get_url()
{
	return (const char *) static_szCmdLine;
}

static void osmo4_do_log(void *cbk, u32 level, u32 tool, const char *fmt, va_list list)
{
	FILE *logs = (FILE *) cbk;
    vfprintf(logs, fmt, list);
	fflush(logs);
}

BOOL Osmo4::InitInstance()
{
	CCommandLineInfo cmdInfo;

	//afxAmbientActCtx = FALSE; 

	m_logs = NULL;

	m_term = NULL;

	memset(&m_user, 0, sizeof(GF_User));

	/*get Osmo4.exe path*/
	wcscpy(szApplicationPath, AfxGetApp()->m_pszHelpFilePath);
	while (szApplicationPath[wcslen((wchar_t *)szApplicationPath) - 1] != '\\') szApplicationPath[wcslen((wchar_t *)szApplicationPath) - 1] = 0;
	if (szApplicationPath[wcslen((wchar_t *)szApplicationPath) - 1] != '\\') wcscat(szApplicationPath,_T("\\"));

	gf_sys_init(GF_FALSE);

	/*setup user*/
	memset(&m_user, 0, sizeof(GF_User));

	Bool first_launch = GF_FALSE;
	/*init config and modules*/
	m_user.config = gf_cfg_init(NULL, &first_launch);
	if (!m_user.config) {
		MessageBox(NULL, _T("Configuration file not found"), _T("Fatal Error"), MB_OK);
		m_pMainWnd->PostMessage(WM_CLOSE);
	}

	char *name = gf_cfg_get_filename(m_user.config);
	char *sep = strrchr(name, '\\');
	if (sep) sep[0] = 0;
	strcpy(szUserPath, name);
	if (sep) sep[0] = '\\';
	gf_free(name);

	const char *opt = gf_cfg_get_key(m_user.config, "General", "SingleInstance");
	m_SingleInstance = (opt && !stricmp(opt, "yes")) ? GF_TRUE : GF_FALSE;

	m_hMutex = NULL;
	if (m_SingleInstance) {
		m_hMutex = CreateMutex(NULL, FALSE, _T("Osmo4_GPAC_INSTANCE"));
		if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
			wchar_t szDIR[1024];
			if (m_hMutex) CloseHandle(m_hMutex);
			m_hMutex = NULL;

			if (!static_gpac_hwnd || !IsWindow(static_gpac_hwnd) ) {
				::MessageBox(NULL, _T("Osmo4 ghost process detected"), _T("Error at last shutdown") , MB_OK);
			} else {
				::SetForegroundWindow(static_gpac_hwnd);

				if (m_lpCmdLine && wcslen(m_lpCmdLine)) {
					DWORD_PTR res;
					size_t len;
					char *the_url, *cmd;
					GetCurrentDirectory(1024, szDIR);
					if (szDIR[wcslen(szDIR) - 1] != '\\') wcscat(szDIR, _T("\\"));
					cmd = (char *)(const char *) m_lpCmdLine;
					strcpy(static_szCmdLine, "");
					if (cmd[0]=='"') cmd+=1;

					if (!strnicmp(cmd, "-queue ", 7)) {
						strcat(static_szCmdLine, "-queue ");
						cmd += 7;
					}
					CT2A szDIR_ch(szDIR);
					the_url = gf_url_concatenate(szDIR_ch, cmd);
					if (!the_url) {
						strcat(static_szCmdLine, cmd);
					} else {
						strcat(static_szCmdLine, the_url);
						gf_free(the_url);
					}
					while ( (len = strlen(static_szCmdLine)) ) {
						char s = static_szCmdLine[len-1];
						if ((s==' ') || (s=='"')) static_szCmdLine[len-1]=0;
						else break;
					}
					::SendMessageTimeout(static_gpac_hwnd, WM_NEWINSTANCE, 0, 0, 0, 1000, &res);
				}
			}
			
			return FALSE;
		}
	}

#if 0
	// Standard initialization
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#endif

	SetRegistryKey(_T("GPAC"));
	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;
	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
	pFrame->LoadAccelTable( MAKEINTRESOURCE(IDR_MAINACCEL));

	m_pMainWnd->DragAcceptFiles();

	if (m_SingleInstance) static_gpac_hwnd = m_pMainWnd->m_hWnd;

	m_user.modules = gf_modules_new(NULL, m_user.config);
	if (!m_user.modules || ! gf_modules_get_count(m_user.modules) ) {
		MessageBox(NULL, _T("No modules currently available. Contact support@bevaratechnologies.com for assistance."), _T("Fatal Error"), MB_OK);
		m_pMainWnd->PostMessage(WM_CLOSE);
	}
	else if (first_launch) {
		/*first launch, register all files ext*/
		u32 i;
		for (i=0; i<gf_modules_get_count(m_user.modules); i++) {
			GF_InputService *ifce = (GF_InputService *) gf_modules_load_interface(m_user.modules, i, GF_NET_CLIENT_INTERFACE);
			if (!ifce) continue;
			if (ifce) {
				ifce->CanHandleURL(ifce, "test.test");
				gf_modules_close_interface((GF_BaseInterface *)ifce);
			}
		}
		/*set some shortcuts*/
		gf_cfg_set_key(m_user.config, "Shortcuts", "VolumeUp", "ctrl+Up");
		gf_cfg_set_key(m_user.config, "Shortcuts", "VolumeDown", "ctrl+Down");
		gf_cfg_set_key(m_user.config, "Shortcuts", "FastRewind", "ctrl+Left");
		gf_cfg_set_key(m_user.config, "Shortcuts", "FastForward", "ctrl+Right");
		gf_cfg_set_key(m_user.config, "Shortcuts", "Play", "ctrl+ ");
	}

	/*check log file*/
	const char *str = gf_cfg_get_key(m_user.config, "General", "LogFile");
	if (str) {
		m_logs = gf_fopen(str, "wt");
		gf_log_set_callback(m_logs, osmo4_do_log);
	}
	else m_logs = NULL;

	/*set log level*/
	if (gf_log_set_tools_levels(gf_cfg_get_key(m_user.config, "General", "Logs")) != GF_OK)
		fprintf(stdout, "osmo4: invalid log level specified\n");

	m_user.opaque = this;
	m_user.os_window_handler = pFrame->m_pWndView->m_hWnd;
	m_user.EventProc = Osmo4_EventProc;

	m_reset = GF_FALSE;
	orig_width = 320;
	orig_height = 240;

	gf_set_progress_callback(this, Osmo4_progress_cbk);

	m_term = gf_term_new(&m_user);
	if (! m_term) {
		MessageBox(NULL, _T("Cannot load Bevara Access. Contact support@bevaratechnologies.com for assistance."), _T("Fatal Error"), MB_OK);
		m_pMainWnd->PostMessage(WM_CLOSE);
		return TRUE;
	}
	SetOptions();
	UpdateRenderSwitch();

	pFrame->SendMessage(WM_SETSIZE, orig_width, orig_height);
	//pFrame->m_Address.ReloadURLs();

	pFrame->m_Sliders.SetVolume();

	m_reconnect_time = 0;


	ParseCommandLine(cmdInfo);

	start_mode = 0;

	if (! cmdInfo.m_strFileName.IsEmpty()) {
		CT2A filename_ch(cmdInfo.m_strFileName);
		//this->media = Media::get_from_file(filename_ch);
		pFrame->m_pPlayList->QueueURL(cmdInfo.m_strFileName, this->media);
		pFrame->m_pPlayList->RefreshList();
		pFrame->m_pPlayList->PlayNext();
	} else {
		char sPL[MAX_PATH];
		strcpy((char *) sPL, szUserPath);
		strcat(sPL, "gpac_pl.m3u");
		pFrame->m_pPlayList->OpenPlayList(sPL);
		const char *sOpt = gf_cfg_get_key(GetApp()->m_user.config, "General", "PLEntry");
		if (sOpt) {
			s32 count = (s32)gf_list_count(pFrame->m_pPlayList->m_entries);
			pFrame->m_pPlayList->m_cur_entry = atoi(sOpt);
			if (pFrame->m_pPlayList->m_cur_entry>=count)
				pFrame->m_pPlayList->m_cur_entry = count-1;
		} else {
			pFrame->m_pPlayList->m_cur_entry = -1;
		}
#if 0
		if (pFrame->m_pPlayList->m_cur_entry>=0) {
			start_mode = 1;
			pFrame->m_pPlayList->Play();
		}
#endif

		sOpt = gf_cfg_get_key(m_user.config, "General", "StartupFile");
		if (sOpt && !strstr(sOpt, "gui") ) gf_term_connect(m_term, sOpt);

		sOpt = gf_cfg_get_key(m_user.config, "General", "PlaylistLoop");
		m_Loop = (sOpt && !strcmp(sOpt, "yes")) ? GF_TRUE : GF_FALSE;
	}
	pFrame->SetFocus();
	pFrame->SetForegroundWindow();
	return TRUE;
}

int Osmo4::ExitInstance() 
{
	if (m_term) gf_term_del(m_term);
	if (m_user.modules) gf_modules_del(m_user.modules);
	if (m_user.config) gf_cfg_del(m_user.config);
	gf_sys_close();
	/*last instance*/
	if (m_hMutex) {
		CloseHandle(m_hMutex);
		static_gpac_hwnd = NULL;
	}
	if (m_logs) gf_fclose(m_logs);
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Osmo4 layout handlers
void UpdateLayout(Media* media){

}

/////////////////////////////////////////////////////////////////////////////
// Osmo4 message handlers





/////////////////////////////////////////////////////////////////////////////
// CDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnGogpac();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedAboutClose();
	afx_msg void OnHelpContact();
	afx_msg void OnEnChangeEdit1();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
		
	CString autoCpyrightTxt = "This application is based, in part, on portions of the GPAC framework which may be obtained at http://gpac.wp.mines-telecom.fr/  \r\nPortions of the underlying Accessors may be also derived from open-source or licensed code. For those works distributed under the Gnu General Public License source code may be obtained by contacting Bevara at support@bevaratechnologies.com. Specific contributors to the original source code are listed below; not all contributions may be used in the underlying Accessors.  Complete copyright information can be found with the source code and/or documentation. \r\n\r\nAAC:  M. Bakker, Alexander Kurpiers, Volker Fischer, Gian-Carlo Pascutto \r\n\r\nAC-3:  Aaron Holtzman,  Michel Lespinasse, Gildas Bazin, Billy Biggs, Eduard Hasenleithner, H�kan Hjort, Charles M. Hannum, Chris Hodges, Michael Holzt, Angelos Keromytis, David I. Lehn, Don Mahuri, Jim Miller, Takefumi Sayo, Shoji Tokunaga \r\n\r\nGIF: see http://www.netsurf-browser.org/projects/libnsgif/ \r\n\r\nMP3 (libmad): Robert Leslie, Niek Albers, Christian Biere, David Blythe, Simon Burge, Brian Cameron, Joshua Haberman, Timothy King, Felix von Leitner, Andre McCurdy, Haruhiko Ogasawara, Brett Paterson, Sean Perry, Bertrand Petit, Nicolas Pitre \r\n\r\nMPEG-1/2 (libmpeg):  Aaron Holtzman, Michel Lespinasse, Sam Hocevar, Christophe Massiot, Koji Agawa, Bruno Barreyra, Gildas Bazin, Diego Biurrun, Alexander W. Chin, Stephen Crowley, Didier Gautheron, Ryan C. Gordon, Peter Gubanov, H�kan Hjort, Petri Hintukainen, Nicolas Joly, Gerd Knorr, David I. Lehn, Olie Lho,  David S. Miller, Rick Niles, Real Ouellet, Bajusz Peter, Franck Sicard, Brion Vibber, Martin Vogt, Fredrik Vraalsen \r\n\r\nNanoJPEG:  Martin J. Fiedler \r\n\r\nOpenJPEG: see http://www.openjpeg.org/ \r\n\r\nPNG:  Glenn Randers-Pehrson, Cosmin Truta, Andreas Dilger, Dave Martindale, Guy Eric Schalnat, Paul Schmidt, Tim Wegner \r\n\r\nPNM (netpbm): http://netpbm.sourceforge.net/  \r\n\r\nTIFF: see http://www.remotesensing.org/libtiff/ \n ";
	DDX_Text(pDX, IDC_EDIT1, autoCpyrightTxt); 
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_GOGPAC, OnGogpac)
	//}}AFX_MSG_MAP
	//ON_BN_CLICKED(IDOK, &CAboutDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_ABOUT_CLOSE, &CAboutDlg::OnBnClickedAboutClose)
	ON_COMMAND(ID_HELP_CONTACT, &Osmo4::OnHelpContact)
    ON_EN_CHANGE(IDC_EDIT1, &CAboutDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()

void Osmo4::OnAbout() 
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CDlg dialog used for App Beta Test Feedback

class CBetaDlg : public CDialog
{
public:
	CBetaDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_BETATESTDIALOG };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBetaTest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedBetaClose();
};

CBetaDlg::CBetaDlg() : CDialog(CBetaDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

//void CAboutDlg::DoDataExchange(CDataExchange* pDX)
//{
//	CDialog::DoDataExchange(pDX);
		
////	CString autoCpyrightTxt = "This application is based, in part, on portions of the GPAC framework which may be obtained at http://gpac.wp.mines-telecom.fr/  \r\nPortions of the underlying Accessors may be also derived from open-source or licensed code. For those works distributed under the Gnu General Public License source code may be obtained by contacting Bevara at support@bevaratechnologies.com. Specific contributors to the original source code are listed below; not all contributions may be used in the underlying Accessors.  Complete copyright information can be found with the source code and/or documentation. \r\n\r\nAAC:  M. Bakker, Alexander Kurpiers, Volker Fischer, Gian-Carlo Pascutto \r\n\r\nAC-3:  Aaron Holtzman,  Michel Lespinasse, Gildas Bazin, Billy Biggs, Eduard Hasenleithner, H�kan Hjort, Charles M. Hannum, Chris Hodges, Michael Holzt, Angelos Keromytis, David I. Lehn, Don Mahuri, Jim Miller, Takefumi Sayo, Shoji Tokunaga \r\n\r\nGIF: see http://www.netsurf-browser.org/projects/libnsgif/ \r\n\r\nMP3 (libmad): Robert Leslie, Niek Albers, Christian Biere, David Blythe, Simon Burge, Brian Cameron, Joshua Haberman, Timothy King, Felix von Leitner, Andre McCurdy, Haruhiko Ogasawara, Brett Paterson, Sean Perry, Bertrand Petit, Nicolas Pitre \r\n\r\nMPEG-1/2 (libmpeg):  Aaron Holtzman, Michel Lespinasse, Sam Hocevar, Christophe Massiot, Koji Agawa, Bruno Barreyra, Gildas Bazin, Diego Biurrun, Alexander W. Chin, Stephen Crowley, Didier Gautheron, Ryan C. Gordon, Peter Gubanov, H�kan Hjort, Petri Hintukainen, Nicolas Joly, Gerd Knorr, David I. Lehn, Olie Lho,  David S. Miller, Rick Niles, Real Ouellet, Bajusz Peter, Franck Sicard, Brion Vibber, Martin Vogt, Fredrik Vraalsen \r\n\r\nNanoJPEG:  Martin J. Fiedler \r\n\r\nOpenJPEG: see http://www.openjpeg.org/ \r\n\r\nPNG:  Glenn Randers-Pehrson, Cosmin Truta, Andreas Dilger, Dave Martindale, Guy Eric Schalnat, Paul Schmidt, Tim Wegner \r\n\r\nPNM (netpbm): http://netpbm.sourceforge.net/  \r\n\r\nTIFF: see http://www.remotesensing.org/libtiff/ \n ";
//	DDX_Text(pDX, IDC_EDIT1, autoCpyrightTxt); 
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
//}

BEGIN_MESSAGE_MAP(CBetaDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(ID_ACCESSBETATESTLINK, OnBetaTest)
	//}}AFX_MSG_MAP
	//ON_BN_CLICKED(IDOK, &CAboutDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BETA_CLOSE, &CBetaDlg::OnBnClickedBetaClose)
	
END_MESSAGE_MAP()
void Osmo4::OnBeta() 
{
	ShellExecute(NULL, _T("open"), _T("http://www.bevara.com/help_access"), NULL, NULL, SW_SHOWNORMAL);
}

BOOL CBetaDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString str = "Bevara Access ";
	SetWindowText(str);
	return TRUE;  
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString str = "Bevara Access ";
	SetWindowText(str);
	return TRUE;  
}

void CAboutDlg::OnGogpac() 
{
	ShellExecute(NULL, _T("open"), _T("http://gpac.sourceforge.net"), NULL, NULL, SW_SHOWNORMAL);
}


void CBetaDlg::OnBetaTest() 
{
	ShellExecute(NULL, _T("open"), _T("https://www.surveymonkey.com/s/BevaraAccess"), NULL, NULL, SW_SHOWNORMAL);
}

/////////////////////////////////////////////////////////////////////////////
// Osmo4 message handlers


void Osmo4::SetOptions()
{
	const char *sOpt = gf_cfg_get_key(m_user.config, "General", "Loop");
	m_Loop = (sOpt && !stricmp(sOpt, "yes")) ? GF_TRUE : GF_FALSE;
	sOpt = gf_cfg_get_key(m_user.config, "General", "LookForSubtitles");
	m_LookForSubtitles = (sOpt && !stricmp(sOpt, "yes")) ? GF_TRUE : GF_FALSE;
	sOpt = gf_cfg_get_key(m_user.config, "General", "ConsoleOff");
	m_NoConsole = (sOpt && !stricmp(sOpt, "yes")) ? GF_TRUE : GF_FALSE;
	sOpt = gf_cfg_get_key(m_user.config, "General", "ViewXMT");
	m_ViewXMTA  = (sOpt && !stricmp(sOpt, "yes")) ? GF_TRUE : GF_FALSE;
	sOpt = gf_cfg_get_key(m_user.config, "General", "NoMIMETypeFetch");
	m_NoMimeFetch = (!sOpt || !stricmp(sOpt, "yes")) ? GF_TRUE : GF_FALSE;
}


void Osmo4::OnOpenUrl() 
{
	COpenUrl url;
	if (url.DoModal() != IDOK) return;

	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;
	pFrame->m_pPlayList->Truncate();
	pFrame->m_pPlayList->QueueURL(url.m_url);
	pFrame->m_pPlayList->RefreshList();
	pFrame->m_pPlayList->PlayNext();
}


CString Osmo4::GetFileFilter()
{
	u32 keyCount, i;
	CString sFiles;
	CString sExts;
	CString supportedFiles;

	/*force MP4 and 3GP files at beginning to make sure they are selected (Win32 bug with too large filters)*/
	supportedFiles = "All Known Files|*.bvr; *.m3u;*.pls;*.mp4;*.3gp;*.3g2;*.avi;*.ogg;*.mpg;*.mpeg;*.vob;*.vcd;*.svcd;*.ts;*.m2t;*.mp2;*.mp3;*.m1a;*.m2a;*.jpeg;*.jpg;*.j2k;*.jp2;*.pdf";

	sExts = "";
	sFiles = "";
	keyCount = gf_cfg_get_key_count(m_user.config, "MimeTypes");
	for (i=0; i<keyCount; i++) {
		const char *sMime;
		Bool first;
		wchar_t *sKey;
		const char *opt;
		wchar_t szKeyList[1000], sDesc[1000];
		sMime = gf_cfg_get_key_name(m_user.config, "MimeTypes", i);
		if (!sMime) continue;
		CString sOpt;
		opt = gf_cfg_get_key(m_user.config, "MimeTypes", sMime);
		/*remove module name*/
		wcscpy(szKeyList, CString(opt + 1));
		sKey = wcsrchr(szKeyList, '\"');
		if (!sKey) continue;
		sKey[0] = 0;
		/*get description*/
		sKey = wcsrchr(szKeyList, '\"');
		if (!sKey) continue;
		wcscpy(sDesc, sKey + 1);
		sKey[0] = 0;
		sKey = wcsrchr(szKeyList, '\"');
		if (!sKey) continue;
		sKey[0] = 0;

		/*if same description for # mime types skip (means an old mime syntax)*/
		if (sFiles.Find(sDesc)>=0) continue;
		/*if same extensions for # mime types skip (don't polluate the file list)*/
		if (sExts.Find(szKeyList)>=0) continue;

		sExts += szKeyList;
		sExts += " ";
		sFiles += sDesc;
		sFiles += "|";

		first = GF_TRUE;

		sOpt = CString(szKeyList);
		while (1) {
			
			int pos = sOpt.Find(' ');
			CString ext = (pos==-1) ? sOpt : sOpt.Left(pos);
			/*WATCHOUT: we do have some "double" ext , eg .wrl.gz - these are NOT supported by windows*/
			if (ext.Find(_T("."))<0) {
				if (!first) {
					sFiles += ";";
				} else {
					first = GF_FALSE;
				}
				sFiles += "*.";
				sFiles += ext;

				CString sext = ext;
				sext += ";";
				if (supportedFiles.Find(sext)<0) {
					supportedFiles += ";*.";
					supportedFiles += ext;
				}
			}

			if (sOpt==ext) break;
			CString rem;
			rem.Format(_T("%s "), (LPCTSTR) ext);
			sOpt.Replace((LPCTSTR) rem, _T(""));
		}
		sFiles += "|";
	}
	supportedFiles += "|";
	supportedFiles += sFiles;
	supportedFiles += "M3U Playlists|*.m3u|ShoutCast Playlists|*.pls|All Files |*.*|";
	return supportedFiles;
}

void Osmo4::OnOpenFile() 
{
	CString sFiles = GetFileFilter();
	
	/*looks like there's a bug here, main filter isn't used correctly while the others are*/
	CFileDialog fd(TRUE,NULL,NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST , sFiles);
	fd.m_ofn.nMaxFile = 25000;
	fd.m_ofn.lpstrFile = (wchar_t *) gf_malloc(sizeof(wchar_t) * fd.m_ofn.nMaxFile);
	fd.m_ofn.lpstrFile[0] = 0;

	if (fd.DoModal()!=IDOK) {
		gf_free(fd.m_ofn.lpstrFile);
		return;
	}

	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;

	gf_cfg_set_key(this->m_user.config, "Accessor", "File", "");

	/*if several items, act as playlist (replace playlist), otherwise as browser (lost all "next" context)*/
	pFrame->m_pPlayList->Clear();

	POSITION pos = fd.GetStartPosition();
	while (pos) {
		CString file = fd.GetNextPathName(pos);
		CT2A file_ch(file);
//		this->media = Media::get_from_file(file_ch);
		pFrame->m_pPlayList->QueueURL(file, this->media);
	}
	gf_free(fd.m_ofn.lpstrFile);
	pFrame->m_pPlayList->RefreshList();
	pFrame->m_pPlayList->PlayNext();
}


void Osmo4::Pause()
{
	if (!m_isopen) return;
	gf_term_set_option(m_term, GF_OPT_PLAY_STATE, (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) ? GF_STATE_PAUSED : GF_STATE_PLAYING);
}

void Osmo4::OnMainPause() 
{
	Pause();	
}

void Osmo4::OnFileStep() 
{
	gf_term_set_option(m_term, GF_OPT_PLAY_STATE, GF_STATE_STEP_PAUSE);
	//((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, 3);
}
void Osmo4::OnUpdateFileStep(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_isopen && !m_reset);	
}

void Osmo4::PlayFromTime(u32 time)
{
	Bool do_pause;
	if (start_mode==1) do_pause = GF_TRUE;
	else if (start_mode==2) do_pause = GF_FALSE;
	else do_pause = /*!m_AutoPlay*/GF_FALSE;
	gf_term_play_from_time(m_term, time, do_pause);
	m_reset = GF_FALSE;
}


void Osmo4::OnFileReload() 
{
	gf_term_disconnect(m_term);
	m_pMainWnd->PostMessage(WM_OPENURL);
}

void Osmo4::OnFileMigrate() 
{
}

void Osmo4::OnConfigReload() 
{
	gf_term_set_option(m_term, GF_OPT_RELOAD_CONFIG, 1); 
}

bool Osmo4::isPlaying(){
	return (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING);
}

void Osmo4::OnFilePlay() 
{
	if (m_isopen) {
		if (m_reset) {
			m_reset = GF_FALSE;
			PlayFromTime(0);
			((CMainFrame *)m_pMainWnd)->SetProgTimer(GF_TRUE);
		} else {
			Pause();
		}
	} else {
		((CMainFrame *) m_pMainWnd)->m_pPlayList->Play();
	}
}

void Osmo4::OnUpdateFilePlay(CCmdUI* pCmdUI)
{
	if (m_isopen) {
		pCmdUI->Enable(TRUE);
		if (pCmdUI->m_nID==ID_FILE_PLAY) {
			if (!m_isopen) {
				pCmdUI->SetText(_T("Play/Pause\tCtrl+P"));
			} else if (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) {
				pCmdUI->SetText(_T("Pause\tCtrl+P"));
			} else {
				pCmdUI->SetText(_T("Resume\tCtrl+P"));
			}
		}
	} else {
		pCmdUI->Enable(((CMainFrame *)m_pMainWnd)->m_pPlayList->HasValidEntries() );
		pCmdUI->SetText(_T("Play\tCtrl+P"));
	}
}

void Osmo4::OnFileStop() 
{
	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;
	if (m_reset) return;
	if (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) Pause();
	m_reset = GF_TRUE;
	pFrame->m_Sliders.m_PosSlider.SetPos(0);
	pFrame->SetProgTimer(GF_FALSE);
	//pFrame->m_wndToolBar.SetButtonInfo(2, ID_FILE_PLAY, TBBS_BUTTON, 3);
	start_mode = 2;
}

void Osmo4::OnUpdateFileStop(CCmdUI* pCmdUI) 
{
//	pCmdUI->Enable(m_isopen);	
}

void Osmo4::OnFileExtract() 
{
	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;
	pFrame->FileExtract();
}

void Osmo4::OnUpdateFileExtract(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(true);
}

void Osmo4::OnSwitchRender()
{
	const char *opt = gf_cfg_get_key(m_user.config, "Compositor", "OpenGLMode");
	Bool use_gl = (opt && !stricmp(opt, "always")) ? GF_TRUE : GF_FALSE;
	gf_cfg_set_key(m_user.config, "Compositor", "OpenGLMode", use_gl ? "disable" : "always");

	gf_term_set_option(m_term, GF_OPT_USE_OPENGL, !use_gl);

	UpdateRenderSwitch();
}




void Osmo4::UpdateRenderSwitch()
{
	const char *opt = gf_cfg_get_key(m_user.config, "Compositor", "ForceOpenGL");
	// Maja changed hardcoded button locations
	/*
	if (opt && !stricmp(opt, "no"))
		((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(8, ID_SWITCH_RENDER, TBBS_BUTTON, 7);
	else
		((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(8, ID_SWITCH_RENDER, TBBS_BUTTON, 7);
		*/
		
		}


void CAboutDlg::OnBnClickedAboutClose()
{
	// TODO: Add your control notification handler code here
	EndDialog(1); // return 1 for success
}

void CBetaDlg::OnBnClickedBetaClose()
{
	// TODO: Add your control notification handler code here
	EndDialog(1); // return 1 for success
}

void Osmo4::OnHelpContact()
{
	// TODO: Add your command handler code here
	CAboutDlg contactDlg;
	contactDlg.DoModal();

}



void CAboutDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO:  Add your control notification handler code here
}