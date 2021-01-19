// DX9PatchManager.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DX9PatchManager.h"
#include "DX9Converter.h"
#include "FileConstants.h"
#include "FMStructs.h"
#include "SendPatch.h"
#include "Settings.h"
#include "SettingValues.h"
#include "ProcessFile.h"
#include "Extern.h"


#include <stdio.h>

#pragma comment(lib, "comctl32.lib")





// Global Variables:
BOOL FileNotSaved = FALSE;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

TCHAR textBuffer[BUFSIZE];

// Forward declarations of functions included in this code moduler
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HMENU hMenu;
HWND hMainWindow = NULL;
HWND hFileList = NULL;
HWND hPatchSet = NULL;
HWND hMIDIDevices = NULL;
BOOL bLogFile = FALSE;
HANDLE hLogFile = NULL;
char workBuf[BUFSIZE];
SYSTEMTIME sysTime;
DWORD bytesWritten;
int selMIDIDevice = -1;
TCHAR PatchFileName[BUFSIZE];
TCHAR PatchFilePath[BUFSIZE];
TCHAR PatchFileNameFull[BUFSIZE];
BOOL SaveNameDefined = FALSE;
HACCEL hAccel = NULL;

int iSetVoices;

TCHAR PatchName[SMALLBUFFER];
TCHAR PatchPath[BUFSIZE];




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	(lpCmdLine);

	MSG msg;
	BOOL ret;
	
	iSetVoices = 0;
//get config items
	SetUpConfig();
	LoadSettings();
	InitCommonControls();
	//our accelerators
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DX9PATCHMANAGER));
	hMainWindow = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), 0, DialogProc, 0);
	ShowWindow(hMainWindow, nCmdShow);

	while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
		if (ret == -1)
			return -1;
		
		if (!TranslateAccelerator(hMainWindow, hAccel, &msg))
		{
			if (!IsDialogMessage(hMainWindow, &msg)) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	
	return 0;
}

void InitSaveName()
{
	wcscpy_s(PatchName,SMALLBUFFER, L"Untitled");
	wcscpy_s(PatchFileName, BUFSIZE, L"Untitled");
	SaveNameDefined = FALSE;
}

//Callback for the dialog
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD wmID = LOWORD(wParam);
	DWORD wmEvent = HIWORD(wParam);
	switch (uMsg)
	{
	case WM_DROPFILES:				
		FileDrop(hDlg, (HDROP)wParam);
		break;
	case WM_ACTIVATE:
		break;
	case WM_INITDIALOG:

//set our menu
		hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDC_DX9PATCHMANAGER));
		SetMenu(hDlg, hMenu);
		InitSaveName();
		EnableMenuItem(hMenu, IDM_CONVERT, MF_DISABLED|MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVE, MF_DISABLED | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVEAS, MF_DISABLED | MF_GRAYED);
		SetWindowTitle(hDlg);
		OpenLogFile();
//get our file list
		hFileList = GetDlgItem(hDlg, IDC_FILELIST);
		hPatchSet = GetDlgItem(hDlg, IDC_PATCHSET);
		hMIDIDevices = GetDlgItem(hDlg, IDC_MIDILIST);
//populate the MIDI device list
		PopulateMIDIList(hMIDIDevices);
		break;
		case WM_COMMAND:
		switch (wmID)
		{
		case IDC_MIDILIST:
			switch(wmEvent)
			{
//selecting the MIDI output device
			case CBN_SELCHANGE:
				selMIDIDevice = (int) SendMessage(hMIDIDevices, CB_GETCURSEL, 0, 0L);			
				SetListButtons();
				break;

			}
			break;
		case IDC_FILELIST:
			switch (wmEvent)
			{		
			case LBN_SELCHANGE:			
				SetListButtons();
				break;
			case LBN_DBLCLK:
				AddToPatchSet();				
				break;
			}
			break;
		case IDC_PATCHSET:
			switch (wmEvent)
			{
				case LBN_SELCHANGE:
					SetListButtons();
					break;
				case LBN_DBLCLK:
					RenamePatch(hDlg, hPatchSet);
					break;
			}
			break;
		case IDC_MIDI:
			SendPatches(hDlg, hFileList, DX9BulkPatch);
			break;
		case IDC_PATCHMIDI:
			SendPatches(hDlg, hPatchSet, DX9OutPatch);
			break;
		case IDM_EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		case IDM_OPEN:
		case IDC_BROWSEBUTTON:		
			GetFileName(hDlg);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
			break;
		case IDC_CLEARLIST:
			ClearList(hDlg);
			break;
		case IDC_SELECTALL:
			SelectAll(hDlg);
			break;
		case IDC_PATCH_SELECTALL:
			SelectAllSet(hDlg);
			break;
		case IDC_SELECTNONE:
			SelectNone(hDlg);
			break;
		case IDC_PATCH_SELECTNONE:
			SelectNoneSet(hDlg);
			break;

		case IDC_CLEAROUTPUT:
			ClearOutput();
			break;
		case IDC_REMOVE:
			RemoveItems();
			break;
		case IDC_MOVERIGHT:
			AddToPatchSet();
			break;
		case IDC_PATCHREMOVE:
			RemoveFromPatchSet();
			break;
		case IDC_PATCHCLEAR:
			if (ClearPatchList(hDlg))
				ResetWindowTitle(hDlg);
			break;
		case IDC_MOVEUP:
			MovePatchUp();
			break;
		case IDC_MOVEDOWN:
			MovePatchDown();
			break;
		case IDC_RENAMEPATCH:
			RenamePatch(hDlg,hPatchSet);
			break;
		case IDC_PLAYNOTE:
			PlayNotes(SendMIDIChannel);
			break;
		case IDM_SAVE:
			if(SaveDX9PatchFile(hDlg,hPatchSet,FALSE))
				UpdateWindowTitle(hDlg);
			break;
		case IDM_SAVEAS:
			if (SaveDX9PatchFile(hDlg, hPatchSet, TRUE))
				UpdateWindowTitle(hDlg);
			break;

		case IDM_NEW:
			if (ClearPatchList(hDlg))
				ResetWindowTitle(hDlg);

			break;
		case IDM_SETTINGS:
			ShowSettings(hDlg);
			break;
		}
		break;

	case WM_CLOSE:
		BOOL close;
		close = TRUE;
		if (FileNotSaved)
		{			
			int retVal = MessageBox(hDlg, TEXT("Save Changes to Patch File ?"), TEXT("DX9PatchManager"), MB_ICONQUESTION | MB_YESNOCANCEL);
			switch (retVal)
			{
				case IDYES:
					close=SaveDX9PatchFile(hDlg, hPatchSet, FALSE);
					break;
				case IDNO:
					break;
				case IDCANCEL:
					close = FALSE;
					break;
			}
		}

		if (close)
			DestroyWindow(hDlg);

		return TRUE;

	case WM_DESTROY:
		DestroyMenu(hMenu);
		CloseLogFile();
		SaveSettings();
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}



void ConsoleOut(TCHAR *output)
{
	HWND hOutput = GetDlgItem(hMainWindow, IDC_OUTPUT);
	
	DWORD noLines=(DWORD)SendMessage(hOutput, EM_GETLINECOUNT, 0, 0);
	if (noLines > MAX_CONSOLE_LINES)	
		SetDlgItemText(hMainWindow, IDC_OUTPUT, L"");

	
	if (bLogFile)
	{
		DWORD bytesWritten;
		int byteLen;
		byteLen = WideCharToMultiByte(CP_UTF8, 0, output, (int)_tcslen(output), workBuf, BUFSIZE, NULL, NULL);
		if (byteLen)
		{
			workBuf[byteLen] = 0;
			if (!WriteFile(hLogFile, workBuf, (DWORD)byteLen+1, &bytesWritten, NULL))
			{
				SendMessage(hOutput, EM_SETSEL, -1, -1);
				SendMessage(hOutput, EM_REPLACESEL, FALSE, (LPARAM)L"** Unable to write to log file");
			}
		}
	}
	SendMessage(hOutput, EM_SETSEL, -1, -1);
	SendMessage(hOutput, EM_REPLACESEL, FALSE, (LPARAM)output);
}

void ClearOutput()
{
	HWND hOutput = GetDlgItem(hMainWindow, IDC_OUTPUT);
	SetDlgItemText(hMainWindow, IDC_OUTPUT, L"");
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DX9PATCHMANAGER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
      return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void SelectAll(HWND hWnd)
{
	SendMessage(hFileList, LB_SELITEMRANGE, (WPARAM)TRUE, (LPARAM)(LOWORD(0) | HIWORD(-1)));
	SetListButtons();
}

void SelectAllSet(HWND hWnd)
{
	SendMessage(hPatchSet, LB_SELITEMRANGE, (WPARAM)TRUE, (LPARAM)(LOWORD(0) | HIWORD(-1)));
	SetListButtons();
}

void SelectNone(HWND hWnd)
{
	SendMessage(hFileList, LB_SELITEMRANGE, (WPARAM)FALSE, (LPARAM)(LOWORD(0)|HIWORD(-1)));
	SetListButtons();
}

void SelectNoneSet(HWND hWnd)
{
	SendMessage(hPatchSet, LB_SELITEMRANGE, (WPARAM)FALSE, (LPARAM)(LOWORD(0) | HIWORD(-1)));
	SetListButtons();
}

void ClearList(HWND hWnd)
{
	GenClearList(hWnd, hFileList);		
}

BOOL ClearPatchList(HWND hWnd)
{
//check if there are any patches (this Can be called from new)
//if the first item evaluates to true, the section item (GenClearList) won't be called
	if ((SendMessage(hPatchSet, LB_GETCOUNT, 0, 0) == 0) || GenClearList(hWnd, hPatchSet))
	{
//if we've cleared the list, no point saving it
		FileNotSaved = FALSE;
		return TRUE;
	}

	return FALSE;
}

BOOL GenClearList(HWND hWnd, HWND hList)
{

	if (!ConfirmClearList || MessageBox(hWnd, TEXT("Clear the list ?"), TEXT("Clear"),
		MB_ICONQUESTION | MB_YESNO) == IDYES)
	{
		SendMessage(hList, LB_RESETCONTENT, 0, 0);
		SetListButtons();
		return TRUE;
	}
	return FALSE;
}

void ConvertFiles(HWND hWnd)
{
	TCHAR *inBuffer = NULL;
	DWORD textLen;
	DWORD maxTextLen = 0;
	DWORD fileItems;
	int selItems[BUFSIZE];	
//get the array of selected items
	fileItems= (DWORD)SendMessage(hFileList, LB_GETSELITEMS, BUFSIZE, (LPARAM)selItems);
	
	if (fileItems > 0)
	{
		//loop through
		for (DWORD i = 0; i < fileItems; i++)
		{
			textLen = (DWORD)SendMessage(hFileList, LB_GETTEXTLEN, selItems[i], 0);
			if (textLen > maxTextLen)
			{
				inBuffer = new TCHAR[textLen + 1];
				maxTextLen = textLen;
			}
//get the selected item text
			SendMessage(hFileList, LB_GETTEXT, selItems[i], (LPARAM)inBuffer);
//unselect
			SendMessage(hFileList, LB_SETSEL, FALSE, (LPARAM)selItems[i]);
//convert the file
			DX9Convert(inBuffer, hWnd, hFileList);
			Yield();
		}
		//delete our buffer if we allocated anything	
		if (inBuffer)
			delete inBuffer;
	}
//set the button states
	SetListButtons();
	
}

//Put up a Save dialog for the file name
BOOL SaveFileName(HWND hWnd, TCHAR *pFileName,BOOL saveAs)
{
	BOOL retValue = FALSE;

//always copy the filename across if it's defined
	if (SaveNameDefined)
		wcscpy_s(pFileName, BUFSIZE, PatchFileNameFull);
//show the dialog if nothing is defiled or it's a "Save As"	
	if(!SaveNameDefined||saveAs)
		return SaveAsFileName(hWnd, pFileName);
	return TRUE;
}

BOOL SaveAsFileName(HWND hWnd, TCHAR *pFileName)
{
	BOOL retValue = FALSE;
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		IFileSaveDialog *pFileSave;
		TCHAR *pWork;
		TCHAR *pWork2;
	
	
		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
			IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

		if (SUCCEEDED(hr))
		{
//add the extension types
			const COMDLG_FILTERSPEC c_rgSaveTypes[] =
			{
				{ L"Sysex Files",       L"*.syx" },
				{ L"All Files",         L"*.*" }
			};


			pFileSave->SetFileTypes(2, c_rgSaveTypes);
//ad default to syx if use doesn't pick anything
			pFileSave->SetDefaultExtension(L"syx");

//check if we have a saved file name already
			if (SaveNameDefined)
//use the file name
				pFileSave->SetFileName(PatchFileName);
			else
//nothing yet, so default to blank with extension
				pFileSave->SetFileName(L"*.syx");
			
					
			hr = pFileSave->Show(NULL);
			if (hr == S_OK)
			{
				IShellItem *pSI;
				hr = pFileSave->GetResult(&pSI);

				if (SUCCEEDED(hr))
				{
					hr = pSI->GetDisplayName(SIGDN_FILESYSPATH, &pWork);

					
					if (SUCCEEDED(hr))					
					{
						//copy the name across
						wcscpy_s(pFileName, BUFSIZE, pWork);
						wcscpy_s(PatchFileNameFull, BUFSIZE, pWork);
						
						hr=pSI->GetDisplayName(SIGDN_NORMALDISPLAY, &pWork2);
						if (SUCCEEDED(hr))
						{
							wcscpy_s(PatchFileName, BUFSIZE, pWork2);
							CoTaskMemFree(pWork2);
						}						
						CoTaskMemFree(pWork);
						//go to here - so everything went as planned
						SaveNameDefined = TRUE;
						
						retValue = TRUE;
					}
					pSI->Release();
				}
			}
			pFileSave->Release();
		}
		CoUninitialize();
	}
	return retValue;
}


void GetFileName(HWND hWnd)
{	
	SetDlgItemText(hWnd, IDC_OUTPUT, L"");	
	
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog *pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			const COMDLG_FILTERSPEC c_rgSaveTypes[] =
			{
				{ L"Sysex Files",       L"*.syx" },
				{ L"All Files",         L"*.*" }
			};


			pFileOpen->SetFileTypes(2, c_rgSaveTypes);
			FILEOPENDIALOGOPTIONS fodo;
			pFileOpen->GetOptions(&fodo);
			pFileOpen->SetOptions(fodo | FOS_ALLOWMULTISELECT);

			hr = pFileOpen->Show(hWnd);
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);	
			if (SUCCEEDED(hr))
			{				
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
//Load in the DX9 patches
				if (SUCCEEDED(hr))
				{	
//Single filename
					DX9Convert(pszFilePath, hWnd, hFileList);
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
			else
//check for multiple selections
				AddSelections(pFileOpen,hWnd);
			pFileOpen->Release();
		}
		CoUninitialize();
	}
	SetListButtons();
}

HRESULT AddSelections(IFileOpenDialog *pfd,HWND hWnd)
{
	HRESULT hr;

	// To Allow for multiple selections the code called IFileOpenDialog::GetResults
	// which returns an IShellItemArray object when the 'Open' button is clicked

	IShellItemArray *psiaResult;

	hr = pfd->GetResults(&psiaResult);	
	if (SUCCEEDED(hr))
	{
		PWSTR pszFilePath = NULL;
		DWORD dwNumItems = 0; // number of items in multiple selection		

		hr = psiaResult->GetCount(&dwNumItems);  // get number of selected items		

// Loop through IShellItemArray and add the items
		for (DWORD i = 0; i < dwNumItems; i++)
		{
			IShellItem *psi = NULL;

			hr = psiaResult->GetItemAt(i, &psi); // get a selected item from the IShellItemArray

			if (SUCCEEDED(hr))
			{
				hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				if (SUCCEEDED(hr))
				{
					DX9Convert(pszFilePath, hWnd, hFileList);
					CoTaskMemFree(pszFilePath);
				}

				psi->Release();
			}
		}		
		psiaResult->Release();

	}

	return hr;
}

//set the enable/disable states of the buttons
void SetListButtons()
{
	int noSelItems;
	int noItems;
	int noPatchItems;
	int noPatchSelItems;
	int selPatchItems[BUFSIZE];

	HWND butSelectAll, butSelectNone, butRemove, butClear, butMoveRight, butPatchRemove, butMoveUp, butMoveDown, butPatchClear, butPatchMIDI, butMIDI, butRename;
	HWND butSelectAllPatch, butSelectNonePatch, butPlayNote;
//get all the buttons
	butSelectAll = GetDlgItem(hMainWindow, IDC_SELECTALL);
	butSelectNone = GetDlgItem(hMainWindow, IDC_SELECTNONE);
	butRemove= GetDlgItem(hMainWindow, IDC_REMOVE);
	butClear = GetDlgItem(hMainWindow, IDC_CLEARLIST);	
	butMoveRight = GetDlgItem(hMainWindow, IDC_MOVERIGHT);
	butPatchRemove = GetDlgItem(hMainWindow, IDC_PATCHREMOVE);
	butMoveUp = GetDlgItem(hMainWindow, IDC_MOVEUP);
	butMoveDown= GetDlgItem(hMainWindow, IDC_MOVEDOWN);
	butPatchClear = GetDlgItem(hMainWindow, IDC_PATCHCLEAR);
	butPatchMIDI= GetDlgItem(hMainWindow, IDC_PATCHMIDI);
	butMIDI = GetDlgItem(hMainWindow, IDC_MIDI);
	butRename = GetDlgItem(hMainWindow, IDC_RENAMEPATCH);
	butPlayNote= GetDlgItem(hMainWindow, IDC_PLAYNOTE);
	butSelectAllPatch = GetDlgItem(hMainWindow, IDC_PATCH_SELECTALL);
	butSelectNonePatch = GetDlgItem(hMainWindow, IDC_PATCH_SELECTNONE);
//get number of list items and number of selected items
	noItems = (int)SendMessage(hFileList, LB_GETCOUNT, 0, 0);
	noSelItems = (int)SendMessage(hFileList, LB_GETSELCOUNT, 0, 0);
	noPatchItems = (int)SendMessage(hPatchSet, LB_GETCOUNT, 0, 0);	

//get the count but also get the seleccted items - useful for move up/down
	noPatchSelItems = (int)SendMessage(hPatchSet, LB_GETSELITEMS, BUFSIZE, (LPARAM)selPatchItems);
	
//simple send midi note enable	
	if (selMIDIDevice != -1)
		EnableWindow(butPlayNote, TRUE);
	else
		EnableWindow(butPlayNote, FALSE);
	
//now, the number of items in the loaded files list	
	if (noItems > 0)
	{
//some items
		EnableWindow(butClear, TRUE);
		if (noSelItems > 0)
		{
			EnableWindow(butRemove,  TRUE);						
			EnableWindow(butMoveRight, TRUE);
			EnableMenuItem(hMenu, IDM_CONVERT, MF_ENABLED);
			if(selMIDIDevice!=-1)
				EnableWindow(butMIDI, TRUE);
			else
				EnableWindow(butMIDI, FALSE);

		}
		else
		{
			EnableWindow(butRemove, FALSE);
			EnableWindow(butMoveRight, FALSE);
			EnableWindow(butMIDI, FALSE);
			EnableMenuItem(hMenu, IDM_CONVERT, MF_DISABLED | MF_GRAYED);
		}
		if (noSelItems != noItems)
			EnableWindow(butSelectAll, TRUE);
		else
			EnableWindow(butSelectAll, FALSE);

		if (noSelItems == 0)
			EnableWindow(butSelectNone, FALSE);
		else
			EnableWindow(butSelectNone, TRUE);
	}
	else
	{
//no items
		EnableWindow(butSelectAll, FALSE);
		EnableWindow(butSelectNone,  FALSE);
		EnableWindow(butRemove, FALSE);
		EnableWindow(butClear, FALSE);
		EnableWindow(butMoveRight, FALSE);
		EnableWindow(butMIDI, FALSE);
		EnableMenuItem(hMenu, IDM_CONVERT, MF_DISABLED | MF_GRAYED);
	}
//patch list
	if (noPatchItems > 0)
	{
//enable save menu items
		EnableMenuItem(hMenu, IDM_SAVE, MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SAVEAS, MF_ENABLED);
		
		if (noPatchSelItems != noPatchItems)
			EnableWindow(butSelectAllPatch, TRUE);
		else
			EnableWindow(butSelectAllPatch, FALSE);


//check if we already have 20 items or more
		if(noPatchItems>19)
			EnableWindow(butMoveRight, FALSE);

		EnableWindow(butPatchClear, TRUE);
//any selected items?
		if (noPatchSelItems > 0)
		{
//some selected items, so enable remove and patch send
			EnableWindow(butPatchRemove, TRUE);
	
			if (selMIDIDevice != -1)
				EnableWindow(butPatchMIDI, TRUE);	
			else
				EnableWindow(butPatchMIDI, FALSE);
//1 item selected?
			if (noPatchSelItems == 1)
			{
//enable rename
				EnableWindow(butRename, TRUE);
//more than 1 patch item?
				if (noPatchItems > 1)
				{
//more than 1 patches with just 1 item selected - see where we are
					if (selPatchItems[0] > 0)
						EnableWindow(butMoveUp, TRUE);
					else
						EnableWindow(butMoveUp, FALSE);
					if (selPatchItems[0] != (noPatchItems - 1))
						EnableWindow(butMoveDown, TRUE);
					else
						EnableWindow(butMoveDown, FALSE);
				}
				else
				{
//only 1 patch, disable up and down				
					EnableWindow(butMoveUp, FALSE);
					EnableWindow(butMoveDown, FALSE);
				}
			}
			else
			{
//more than 1 item, disable rename and up and down
				EnableWindow(butRename, FALSE);
				EnableWindow(butMoveUp, FALSE);
				EnableWindow(butMoveDown, FALSE);
			}

			if (noPatchSelItems == 0)
				EnableWindow(butSelectNonePatch, FALSE);
			else
				EnableWindow(butSelectNonePatch, TRUE);
		}
		else
		{
//no selected items - so disable the buttons
			EnableWindow(butPatchRemove, FALSE);
			EnableWindow(butMoveUp, FALSE);
			EnableWindow(butMoveDown, FALSE);
			EnableWindow(butPatchMIDI, FALSE);
			EnableWindow(butRename, FALSE);
			EnableWindow(butSelectNonePatch, FALSE);

		}
	}
	else
	{
//no items - so disable the buttons
		EnableWindow(butPatchRemove, FALSE);
		EnableWindow(butPatchClear, FALSE);
		EnableWindow(butPatchMIDI, FALSE);
		EnableWindow(butRename, FALSE);
		EnableWindow(butMoveUp, FALSE);
		EnableWindow(butMoveDown, FALSE);
//and save menu items
		EnableMenuItem(hMenu, IDM_SAVE, MF_DISABLED | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVEAS, MF_DISABLED | MF_GRAYED);
		EnableWindow(butSelectAllPatch, FALSE);
		EnableWindow(butSelectNonePatch, FALSE);
	}

		
}

void AddToPatchSet(void)
{
//get the selected index
	DWORD fileItems;
	int selItems[BUFSIZE];
	int patchItems;
	int iNoItems = 0;
	int addIdx;
	DWORD textLen;
	TCHAR *inBuffer = NULL;
	DWORD maxTextLen = 0;
//get the array of selected items
	fileItems = (DWORD)SendMessage(hFileList, LB_GETSELITEMS, BUFSIZE, (LPARAM)selItems);

	if (fileItems > 0)
	{
		FileNotSaved=TRUE;		
//loop through
		for (DWORD i = 0; i < fileItems; i++)
		{
//check if we've already hit 20 items
			patchItems = (DWORD)SendMessage(hPatchSet, LB_GETCOUNT, 0, 0);
			if (patchItems == MAX_DX9_WORK_PATCHES)
			{
				switch (i)
				{
				case 1:
					wcscpy_s(outBuffer, BUFSIZE, L"Reached Maximum of 20 DX9 Patches\r\nAdded only 1 selected patch");					
					break;
				case 20:
					wcscpy_s(outBuffer, BUFSIZE, L"Reached Maximum of 20 DX9 Patches");
					break;
				default:
					wsprintf(outBuffer, L"Reached Maximum of 20 DX9 Patches\r\nAdded only %d selected patches", i);
				}
				MessageBox(hMainWindow, outBuffer, TEXT("Max DX9 Patches Reached"), MB_ICONWARNING | MB_OK);
				break;
			}

			textLen = (DWORD)SendMessage(hFileList, LB_GETTEXTLEN, selItems[i], 0);
			if (textLen > maxTextLen)
			{
				inBuffer = new TCHAR[textLen + 1];
				maxTextLen = textLen;
			}
//get the selected item text
			SendMessage(hFileList, LB_GETTEXT, selItems[i], (LPARAM)inBuffer);
//add to our patch list
			addIdx=(int)SendMessage(hPatchSet, LB_ADDSTRING, 0, (LPARAM)inBuffer);
//add the data item (index) - for the inital insert this will fairly straight forward where the index = data index
			if (addIdx != LB_ERR)			
//copy the data
				CopyDX9PatchItem(selItems[i] , addIdx);							
		}
	}

//set the buttons
	SetListButtons();
//cleanup
	delete inBuffer;
	return;
}

void RemoveFromPatchSet(void)
{
	GenRemoveItems(hPatchSet, DX9OutPatch);
	if (SendMessage(hPatchSet, LB_GETCOUNT, (WPARAM)0, (LPARAM)0) == 0)
		FileNotSaved = FALSE;
}

void RemoveItems()
{	
	GenRemoveItems(hFileList, DX9BulkPatch);
	SendMessage(hFileList, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);			
}

BOOL GenRemoveItems(HWND hList,lpFM_BULK_OLD_PATCH lpBulkList)
{
	int fileItems;
	int listCount;
	BOOL bRet = FALSE;
	int selItems[BUFSIZE];
//get the total number of items
	listCount = (DWORD)SendMessage(hList, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

	//get the array of selected items
	fileItems = (DWORD)SendMessage(hList, LB_GETSELITEMS, BUFSIZE, (LPARAM)selItems);

	if (fileItems > 0)
	{
		if (!ConfirmRemove || MessageBox(hMainWindow, TEXT("Remove Selected Item(s) from List ?"), TEXT("Remove Items"),
			MB_ICONQUESTION | MB_YESNO) == IDYES)
		{
//if there is at least 1 item left, return true (for the save before close flag)
			if(fileItems!=1)
				bRet = TRUE;
			//need to go through the list backwards			
			for (int i = fileItems - 1; i >= 0; i--)
			{
				//delete the item
				SendMessage(hList, LB_DELETESTRING, selItems[i], 0);
//move the array down
				if (listCount > 1)
				{
					for (int j = selItems[i]; j< listCount;j++)
						memcpy_s(&lpBulkList[j], sizeof(FM_BULK_OLD_PATCH), &lpBulkList[j + 1], sizeof(FM_BULK_OLD_PATCH));						
					listCount--;
				}
			}
		}
	}
	SetListButtons();
	return bRet;
}


void MovePatchUp(void)
{
	int noSelItems;
	int selPatchItems[SMALLBUFFER];
	int selIdx;
	int selIdx2;	
	TCHAR patchName2[SMALLBUFFER];
	FM_BULK_OLD_PATCH workPatch;
//get the selected item
	noSelItems = (int)SendMessage(hPatchSet, LB_GETSELITEMS, SMALLBUFFER, (LPARAM)selPatchItems);
	
	if (noSelItems == 1)
	{
//there should only be 1 selected item, so get that index
		selIdx = selPatchItems[0];
		selIdx2 = selIdx - 1;
//save details of the item above
		SendMessage(hPatchSet, LB_GETTEXT, selIdx2, (LPARAM)patchName2);		
//then delete it
		SendMessage(hPatchSet, LB_DELETESTRING, selIdx2, 0);
//then re-add that item in the selected index
		SendMessage(hPatchSet, LB_INSERTSTRING, selIdx, (LPARAM)patchName2);
//now switch the data items
//same deal, save the data for the item above
		memcpy_s(&workPatch, sizeof(FM_BULK_OLD_PATCH), &DX9OutPatch[selIdx2], sizeof(FM_BULK_OLD_PATCH));
//then replace it with the current patch
		memcpy_s(&DX9OutPatch[selIdx2], sizeof(FM_BULK_OLD_PATCH), &DX9OutPatch[selIdx], sizeof(FM_BULK_OLD_PATCH));
//then copy the saved date to the old idx
		memcpy_s(&DX9OutPatch[selIdx], sizeof(FM_BULK_OLD_PATCH), &workPatch, sizeof(FM_BULK_OLD_PATCH));
	}
	SetListButtons();
}

void MovePatchDown(void)
{
	int noSelItems;
	int selPatchItems[SMALLBUFFER];
	int selIdx;
	int selIdx2;	
	TCHAR patchName[SMALLBUFFER];
	FM_BULK_OLD_PATCH workPatch;

	//get the selected item
	noSelItems = (int)SendMessage(hPatchSet, LB_GETSELITEMS, SMALLBUFFER, (LPARAM)selPatchItems);
	if (noSelItems == 1)
	{
		//there should only be 1 selected item, so get that index
		selIdx = selPatchItems[0];
		selIdx2 = selIdx + 1;
		//save details of the item
		SendMessage(hPatchSet, LB_GETTEXT, selIdx, (LPARAM)patchName);		
		//then delete it
		SendMessage(hPatchSet, LB_DELETESTRING, selIdx, 0);
		//then re-add that item at the index below
		SendMessage(hPatchSet, LB_INSERTSTRING, selIdx2, (LPARAM)patchName);
//as we delete the selected item - we need to re-select it
		SendMessage(hPatchSet, LB_SETSEL, TRUE, selIdx2);

		//now switch the data items
		//same deal, save the data for the item 
		memcpy_s(&workPatch, sizeof(FM_BULK_OLD_PATCH), &DX9OutPatch[selIdx], sizeof(FM_BULK_OLD_PATCH));
		//then replace it with the item
		memcpy_s(&DX9OutPatch[selIdx], sizeof(FM_BULK_OLD_PATCH), &DX9OutPatch[selIdx2], sizeof(FM_BULK_OLD_PATCH));
		//then copy the saved date to the new idx
		memcpy_s(&DX9OutPatch[selIdx2], sizeof(FM_BULK_OLD_PATCH), &workPatch, sizeof(FM_BULK_OLD_PATCH));
	}
	SetListButtons();
}

void OpenLogFile()
{
	hLogFile = CreateFile(L"DX9PatchManager.log",                // name of the file
		GENERIC_WRITE,          // open for writing
		FILE_SHARE_READ,	// log can be read before it's closed
		NULL,                   // default security
		CREATE_ALWAYS,             // create file always, even if it already exists
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

	if (hLogFile != INVALID_HANDLE_VALUE)
	{
		GetSystemTime(&sysTime);
		sprintf_s(workBuf, BUFSIZE, "------- Log Opened  %02d/%02d/%04d %02d:%02d:%02d  ------- \r\n",		
			sysTime.wDay, sysTime.wMonth, sysTime.wYear,
			sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
		WriteFile(hLogFile, workBuf, (DWORD)strlen(workBuf) + 1, &bytesWritten, NULL);
		bLogFile = TRUE;
	}
}

void CloseLogFile()
{
	if (bLogFile)
	{
		GetSystemTime(&sysTime);
		sprintf_s(workBuf, BUFSIZE, "\r\n=========== Log Closed %02d/%02d/%04d %02d:%02d:%02d ===========\r\n",
			sysTime.wDay, sysTime.wMonth, sysTime.wYear,
			sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
		WriteFile(hLogFile, workBuf, (DWORD)strlen(workBuf) + 1, &bytesWritten, NULL);

		CloseHandle(hLogFile);
	}
	bLogFile = FALSE;
}

void ResetWindowTitle(HWND hDlg)
{
	InitSaveName();
	SetWindowTitle(hDlg);
}

void UpdateWindowTitle(HWND hDlg)
{
	wcscpy_s(PatchName, SMALLBUFFER, PatchFileName);
	SetWindowTitle(hDlg);
}
void SetWindowTitle(HWND hDlg)
{
	wsprintf(textBuffer,L"%s - DX9 Patch Manager", PatchName);
		
	SetWindowText(hDlg, textBuffer);

}

void FileDrop(HWND hDlg,HDROP hDrop)
{
	POINT point;	
	UINT noFiles,uiRet;	

//get the mosue poisition (may of some use later on if we have multiple drag controsl)
	if (DragQueryPoint(hDrop, &point))
	{
//get the number of files dropped
		noFiles = DragQueryFile(hDrop, 0xFFFFFFFF, PatchFileNameFull, BUFSIZE);

		if (noFiles == 1)
		{
//only 1 file to add
			uiRet=DragQueryFile(hDrop, 0, PatchFileNameFull, BUFSIZE);
			if (uiRet != 0)
				DX9Convert(PatchFileNameFull, hDlg, hFileList);
			else
			{
				wsprintf(outBuffer, L"Unable to get drag file details %d\r\n", uiRet);
				ConsoleOut(outBuffer);
			}
		}
		else
		{
//multiple files
			for (UINT i = 0; i < noFiles; i++)
			{
				uiRet = DragQueryFile(hDrop, i, PatchFileNameFull, BUFSIZE);
				if (uiRet != 0)
					DX9Convert(PatchFileNameFull, hDlg, hFileList);
				else
				{
					wsprintf(outBuffer, L"Unable to get drag file detailsfor item %d :: %d\r\n", i, uiRet);
					ConsoleOut(outBuffer);
				}
			}
		}
//close out the drag file and free anything allocated for it
		DragFinish(hDrop);
		
	}
	else
		ConsoleOut(L"*** - Failed to get mouse position at time of file drop\r\n");
	//set the button states
	SetListButtons();
}