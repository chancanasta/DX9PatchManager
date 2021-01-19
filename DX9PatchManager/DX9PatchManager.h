#ifndef __DX9PATCHMANAGER_H
#define __DX9PATCHMANAGER_H

#include <shobjidl.h> 

#include "resource.h"
#include "FMStructs.h"

#define MAX_CONSOLE_LINES	512

//uncomment for more debug output
//#define _DX9_DEBUG

void GetFileName(HWND hWnd);
void ConvertFiles(HWND hWnd);

HRESULT AddSelections(IFileOpenDialog *pfd, HWND hWnd);

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//'console' output
void ClearOutput();
void ConsoleOut(TCHAR *output);

void AddToPatchSet(void);
void RemoveFromPatchSet(void);
BOOL GenRemoveItems(HWND hList,lpFM_BULK_OLD_PATCH lpBulkList);

BOOL ClearPatchList(HWND hWnd);
BOOL GenClearList(HWND hWnd, HWND hList);
void MovePatchUp(void);
void MovePatchDown(void);


//file list functions
void ClearList(HWND hWnd);
void SelectAll(HWND hWnd);
void SelectNone(HWND hWnd);
void SetListButtons();
void RemoveItems();

//patch list
void SelectAllSet(HWND hWnd);
void SelectNoneSet(HWND hWnd);

//log file

void OpenLogFile();
void CloseLogFile();
BOOL SaveFileName(HWND hWnd,TCHAR *pFileName,BOOL saveAs);
BOOL SaveAsFileName(HWND hWnd, TCHAR *pFileName);

void ResetWindowTitle(HWND hDlg);
void UpdateWindowTitle(HWND hDlg);
void SetWindowTitle(HWND hDlg);
void InitSaveName();
void FileDrop(HWND hDlg, HDROP hDrop);

#endif