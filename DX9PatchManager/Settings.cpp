#include "stdafx.h"
#include "FileConstants.h"
#include "Settings.h"
#include "resource.h"

extern HINSTANCE hInst;

TCHAR FileName[BUFSIZE];
TCHAR AppName[SMALLBUFFER];
TCHAR WorkBuffer[BUFSIZE];

BOOL ConfirmClearList=TRUE;
BOOL ConfirmRemove = TRUE;
BOOL ConfirmSend = TRUE;
BOOL AutoSelectLoaded = FALSE;

int SendMIDIChannel = 1;

static TCHAR GeneralSection[] = L"GENERAL";
static TCHAR ItemConfirmCList[] = L"ConfirmListClear";
static TCHAR ItemConfirmRemove[] = L"ConfirmRemove";
static TCHAR ItemConfirmSend[] = L"ConfirmSend";
static TCHAR ItemAutoSelectPatches[] = L"AutoSelectLoaded";
static TCHAR ItemSendMChannel[] = L"SendMIDIChannel";


HWND hMIDIListBox;
HWND hConfirmListClear;
HWND hConfirmRemove;
HWND hAutoSelect;
HWND hConfirmSend;

void ShowSettings(HWND hMainWindow)
{
//display the settings dialog
	DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hMainWindow, (DLGPROC)SettingsDialog);
}

void SetUpConfig()
{

	GetCurrentDirectory(BUFSIZE, WorkBuffer);
	swprintf_s(FileName, BUFSIZE,L"%s\\%s", WorkBuffer, L"DX9PatchManager.ini");	
	
	return;
}

void LoadSettings()
{
	//list clear confirm
	ConfirmClearList = ReadBoolSetting(GeneralSection, ItemConfirmCList, TRUE);

	//remove confirm
	ConfirmRemove = ReadBoolSetting(GeneralSection, ItemConfirmRemove, TRUE);

	//send confirm
	ConfirmSend = ReadBoolSetting(GeneralSection, ItemConfirmSend, TRUE);
	//Auto select
	AutoSelectLoaded = ReadBoolSetting(GeneralSection, ItemAutoSelectPatches, FALSE);

	//midi channel
	SendMIDIChannel = GetPrivateProfileInt(GeneralSection, ItemSendMChannel, 1, FileName);

	return;
}

void SaveSettings()
{
	WriteBoolSetting(ConfirmClearList, GeneralSection, ItemConfirmCList);
	WriteBoolSetting(ConfirmRemove, GeneralSection, ItemConfirmRemove);
	WriteBoolSetting(ConfirmSend, GeneralSection, ItemConfirmSend);
	WriteBoolSetting(AutoSelectLoaded, GeneralSection, ItemAutoSelectPatches);
	WriteIntSetting(SendMIDIChannel, GeneralSection, ItemSendMChannel);

	return;
}

BOOL ReadBoolSetting(LPCWSTR section, LPCWSTR item, BOOL default)
{
	DWORD dRet;
	TCHAR defString[SMALLBUFFER];
	TCHAR workBuffer[SMALLBUFFER];
	BOOL retVal = FALSE;
//work out the default string
	if (default)
		wcscpy_s(defString, SMALLBUFFER, L"Yes");
	else
		wcscpy_s(defString, SMALLBUFFER, L"No");

	
	dRet = GetPrivateProfileString(section, item, defString, workBuffer, SMALLBUFFER, FileName);
	if (!lstrcmpi(workBuffer, L"Yes"))
		retVal = TRUE;

	return retVal;

}

void WriteIntSetting(int value, LPCWSTR section, LPCWSTR item)
{
	BOOL ret;
	DWORD lastErr;

	swprintf_s(WorkBuffer, BUFSIZE, L"%d", value);
	ret = WritePrivateProfileString(section, item, WorkBuffer, FileName);
	if (!ret)
		lastErr = GetLastError();
}

void WriteBoolSetting(BOOL value, LPCWSTR section, LPCWSTR item)
{
	BOOL ret;
	DWORD lastErr;

	if (value)
		ret = WritePrivateProfileString(section, item, L"Yes", FileName);
	else
		ret = WritePrivateProfileString(section, item, L"No", FileName);

	if (!ret)
		lastErr = GetLastError();
	return;
}

INT_PTR CALLBACK SettingsDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD wmID = LOWORD(wParam);
	DWORD wmEvent = HIWORD(wParam);
	
	int i;


	switch (uMsg)
	{
	case WM_INITDIALOG:
//get handles for the various controls
		hConfirmListClear = GetDlgItem(hDlg, IDC_CONFIRM_LIST_CLEAR);
		hConfirmRemove = GetDlgItem(hDlg, IDC_CONFIRM_REMOVE);
		hMIDIListBox = GetDlgItem(hDlg, IDC_MIDI_CHANNEL);
		hAutoSelect = GetDlgItem(hDlg, IDC_AUTO_SELECT_LOADED);
		hConfirmSend = GetDlgItem(hDlg, IDC_CONFIRM_SEND);
//set the midi channel value		
		for (i = 0; i < 16; i++)
		{
			swprintf_s(WorkBuffer, BUFSIZE, L"%d", (i + 1));
			SendMessage(hMIDIListBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)WorkBuffer);
		}
		SendMessage(hMIDIListBox, (UINT)CB_SETCURSEL, (WPARAM)(SendMIDIChannel - 1), (LPARAM)0);

		SettingsCheckBox(hConfirmListClear, ConfirmClearList);
		SettingsCheckBox(hConfirmRemove, ConfirmRemove);
		SettingsCheckBox(hAutoSelect, AutoSelectLoaded);
		SettingsCheckBox(hConfirmSend, ConfirmSend);

		break;
	case WM_CLOSE:
		EndDialog(hDlg, wParam);
		return TRUE;
	case WM_COMMAND:
		switch (wmID)
		{
			case IDOK:
//save the details
				SaveSettingValues(hDlg);
//end the dialog
				EndDialog(hDlg, wParam);
				break;
			case IDCANCEL:
//end without saving
				EndDialog(hDlg, wParam);
				break;
		}
		return TRUE;
	}

	return FALSE;

}

void SaveSettingValues(HWND hDLg)
{	
	ConfirmClearList = RetValCheckBox(hConfirmListClear);
	ConfirmRemove = RetValCheckBox(hConfirmRemove);
	ConfirmSend = RetValCheckBox(hConfirmSend);
	AutoSelectLoaded = RetValCheckBox(hAutoSelect);
	SendMIDIChannel = (int)((SendMessage(hMIDIListBox, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0)) + 1);
}

BOOL RetValCheckBox(HWND item)
{
	LRESULT lRes;
	lRes = SendMessage(item, (UINT)BM_GETCHECK, (WPARAM)0, (LPARAM)0);
	if (lRes == BST_CHECKED)
		return TRUE;
	return FALSE;
}
void SettingsCheckBox(HWND item, BOOL value)
{
	if (value)
		SendMessage(item, (UINT)BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
	else
		SendMessage(item, (UINT)BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);

}
