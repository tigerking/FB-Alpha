// Media module
#include "burner.h"

int MediaInit()
{
		dprintf(_T("s1\n"));
	if (ScrnInit()) {					// Init the Scrn Window
		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_UI_WINDOW));
		FBAPopupDisplay(PUF_TYPE_ERROR);
		return 1;
	}
		dprintf(_T("s2\n"));
	if (!bInputOkay) {
		InputInit();					// Init Input
	}
		dprintf(_T("s3\n"));
	nAppVirtualFps = nBurnFPS;
		dprintf(_T("s4\n"));
	if (!bAudOkay) {
		AudSoundInit();					// Init Sound (not critical if it fails)
	}
		dprintf(_T("s5\n"));
	nBurnSoundRate = 0;					// Assume no sound
	pBurnSoundOut = NULL;
	if (bAudOkay) {
		nBurnSoundRate = nAudSampleRate[nAudSelect];
		nBurnSoundLen = nAudSegLen;
	}
		dprintf(_T("s6\n"));
	if (!bVidOkay) {
			dprintf(_T("s7\n"));
		// Reinit the video plugin
		VidInit();
		if (!bVidOkay && nVidFullscreen) {

			dprintf(_T("s8\n"));
			nVidFullscreen = 0;
			dprintf(_T("s9\n"));
			MediaExit();
			return (MediaInit());
		}
		if (!nVidFullscreen) {
			ScrnSize();
		}
		dprintf(_T("s10\n"));
		if (!bVidOkay) {
			// Make sure the error will be visible
			SplashDestroy(1);
			dprintf(_T("s11\n"));
			FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_UI_MODULE), VidGetModuleName());
			FBAPopupDisplay(PUF_TYPE_ERROR);
		}
		dprintf(_T("s12\n"));
		if (bVidOkay && ((bRunPause && bAltPause) || !bDrvOkay)) {
			VidRedraw();
		}
	}
	dprintf(_T("s13\n"));
	return 0;
}

int MediaExit()
{
	nBurnSoundRate = 0;					// Blank sound
	pBurnSoundOut = NULL;

	AudSoundExit();						// Exit Dsound

	VidExit();

	InputExit();

	DestroyWindow(hInpsDlg);			// Make sure the Input Set dialog is exitted
	DestroyWindow(hInpdDlg);			// Make sure the Input Dialog is exitted

	ScrnExit();							// Exit the Scrn Window

	return 0;
}
