// DX9Converter.cpp : From the original console version
//

#include "stdafx.h"
#include "DX9Converter.h"
#include "DX9PatchManager.h"
#include "FileConstants.h"
#include "FMStructs.h"
#include "ProcessFile.h"
#include "Extern.h"


DWORD g_BytesTransferred = 0;

HANDLE hFile;
HANDLE hProcessHeap;
PHANDLE phInputBuffer;
PHANDLE phOutputBuffer;
PHANDLE phNameBuffer;
HWND mainWindow=NULL;
TCHAR outBuffer[BUFSIZE];


//Save the DX9 20 patch file
BOOL SaveDX9PatchFile(HWND hDlg, HWND hPatchSet,BOOL saveAs)
{
	BOOL bRetVal;
	TCHAR fileName[BUFSIZE];
	int midiChannel = 1;
	if (SaveFileName(hDlg, fileName,saveAs))
	{
		wsprintf(outBuffer, L"Save File : %s \r\n", fileName);
		ConsoleOut(outBuffer);
		bRetVal= ProcessDX9Patches(hPatchSet, (UCHAR *)phOutputBuffer, midiChannel, fileName);
		FileNotSaved = !bRetVal;
		return bRetVal;
	}
	return FALSE;
}

int DX9Convert(TCHAR *inFile,HWND hWnd,HWND hFileList)
{	
	int MidiChannel;
	TCHAR *lpOutFile = NULL;
	TCHAR outBuffer[1024];


	mainWindow = hWnd;

	ConsoleOut(L"\r\n*************************\r\nImport - Start \r\n");
	
	wsprintf(outBuffer, TEXT("Opening File \'%s\'\r\n"), inFile);
	ConsoleOut(outBuffer);

//allocate buffers	
	if (!AllocateBuffers())
		return -3;
	
//open the input file
	hFile = CreateFile(inFile,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		wsprintf(outBuffer,TEXT("\n***ERROR - Unable to open file \'%s\'\r\n"), inFile);
		ConsoleOut(outBuffer);		
//free the buffers
		FreeBuffers();
		return -3;
	}

	MidiChannel = 1;
	ProcessFile(hFile, phInputBuffer, phOutputBuffer, phNameBuffer, MidiChannel, inFile, hFileList);
//close the input file
	CloseHandle(hFile);
	
//free the buffers
	FreeBuffers();

    return 0;
}

BOOL AllocateBuffers(VOID)
{
	//get heap
	hProcessHeap = GetProcessHeap();
	if (hProcessHeap == NULL)
	{
		ConsoleOut(L"*** Unable to get process heap handle\r\n");
		return FALSE;
	}
	//allocate input buffer
	phInputBuffer = (PHANDLE)HeapAlloc(hProcessHeap, 0, INPUTBUFFERSIZE);
	if (phInputBuffer == NULL)
	{
		ConsoleOut(L"*** Unable to allocate input buffer\r\n");
		return FALSE;
	}
#ifdef _DX9_DEBUG
	else
	{
		swprintf_s(outBuffer, 1024, L"Allocated input buffer (%d bytes)\r\n", INPUTBUFFERSIZE);
		ConsoleOut(outBuffer);
	}
#endif

	//allcoate output buffer
	phOutputBuffer = (PHANDLE)HeapAlloc(hProcessHeap, 0, OUTPUTBUFFERSIZE);
	if (phOutputBuffer == NULL)
	{
		ConsoleOut(L"*** Unable to allocate output buffer\r\n");
		return FALSE;
	}
#ifdef _DX9_DEBUG
	else
	{
		swprintf_s(outBuffer, 1024, L"Allocated output buffer (%d bytes)\r\n", OUTPUTBUFFERSIZE);
		ConsoleOut(outBuffer);
	}

#endif

	//allocate input buffer
	phNameBuffer = (PHANDLE)HeapAlloc(hProcessHeap, 0, NAMEBUFFERSIZE);
	if (phInputBuffer == NULL)
	{
		ConsoleOut(L"*** Unable to allocate Name buffer\r\n");
		return FALSE;
	}
#ifdef _DX9_DEBUG
	else
	{
		swprintf_s(outBuffer, 1024, L"Allocated input buffer (%d bytes)\r\n", INPUTBUFFERSIZE);
		ConsoleOut(outBuffer);
	}
#endif
	return TRUE;
}

VOID FreeBuffers(VOID)
{
	if (HeapFree(hProcessHeap, 0, phInputBuffer) == NULL)
		ConsoleOut(L"*** Failed to free Input Buffer\r\n");

	if (HeapFree(hProcessHeap, 0, phOutputBuffer) == NULL)	
		ConsoleOut(L"*** Failed to free outptu buffer\r\n");

	return;
}


