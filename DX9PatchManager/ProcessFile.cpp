//where most of the file processing happens
#include "stdafx.h"
#include "DX9PatchManager.h"
#include "FileConstants.h"
#include "SysExConstants.h"
#include "FMStructs.h"
#include "FMConversion.h"
#include "ProcessFile.h"
#include "DXDisplay.h"
#include "SendPatch.h"
#include "SettingValues.h"
#include <Shlwapi.h>
#include <math.h> 
#include <stdio.h>
#include <mmeapi.h>
OVERLAPPED ol = { 0 };
TCHAR genOut[BUFSIZE];
char textBuffer[BUFSIZE];


FM_BULK_OLD_PATCH	DX9BulkPatch[MAX_DX9_BULK_PATCHES];
FM_BULK_OLD_PATCH	DX9OutPatch[MAX_DX9_WORK_PATCHES];

int idxDX9SinglePatch = 0;
int idxDX9OutPatch = 0;

FM_BULK_OLD_PATCH	DX9INITPATCH;
FM_BULK_OLD_PATCH	DX9INITPATCH2;
FM_SINGLE_OLD_PATCH	DX9INITSINGLEPATCH;

UCHAR DX9INITPATCHNAME[FM_PATCH_NAME_SIZE] = { 'I','N','I','T',' ','V','O','I','C','E' };

BOOL ProcessDX9Patches(HWND hPatchSet, UCHAR *pWriteBuffer, int midiChannel, TCHAR *pFileName)
{
	TCHAR patchFileName[BUFSIZE];
	char workBuffer[SMALLBUFFER];
	char workBuffer2[SMALLBUFFER];

	DWORD items;

	int byteLen;


	TCHAR *pPatchName;
	TCHAR *pSysExName;
	

//create a name for the patch file
	GetPatchNameFile(pFileName, patchFileName);
	wsprintf(genOut, L"Patch Filename: %s\r\n", patchFileName);
	ConsoleOut(genOut);
	pPatchName = PathFindFileName(patchFileName);
	pSysExName = PathFindFileName(pFileName);
	byteLen = WideCharToMultiByte(CP_UTF8, 0, pPatchName, (int)_tcslen(pPatchName), workBuffer, SMALLBUFFER, NULL, NULL);
	workBuffer[byteLen] = 0;
	byteLen = WideCharToMultiByte(CP_UTF8, 0, pSysExName, (int)_tcslen(pSysExName), workBuffer2, SMALLBUFFER, NULL, NULL);
	workBuffer2[byteLen] = 0;

	items = (DWORD)SendMessage(hPatchSet, LB_GETCOUNT, 0, 0);
	if (items)
	{
//header for patch name file
		sprintf_s(textBuffer, BUFSIZE, "-------------\r\n  Sysex File : %s\r\n  Patch File : %s\r\n-------------\r\n", workBuffer2, workBuffer);
		strcat_s(textBuffer, BUFSIZE, "\r\n     Patches\r\n=================\r\n");
//fill the output buffer and create the name file
		fillPatchesBuffer(hPatchSet, midiChannel, pWriteBuffer, items, NULL, DX9OutPatch);
		
		fillNamesBuffer(hPatchSet);

//write out the file
		WriteDX9BulkFile(pFileName, pWriteBuffer, SYSLEN);
		WriteDX9PatchNames(patchFileName, textBuffer);
		return TRUE;
	}
	return FALSE;
}


void fillNamesBuffer(HWND hPatchSet)
{
	TCHAR tcharBuffer[BUFSIZE];
	TCHAR patchName[BUFSIZE];
	char workBuffer[SMALLBUFFER];
	char workBuffer3[SMALLBUFFER];

	DWORD items;
	LRESULT lRet;
	int byteLen;
	items = (DWORD)SendMessage(hPatchSet, LB_GETCOUNT, 0, 0);
	if (items > 0)
	{
//loop through the patches in the list box
		for (DWORD i = 0; i < items; i++)
		{
			//get a nice version of the name
			PreProcessName(hPatchSet, i, patchName);
			//and an 8bit version of that
			byteLen = WideCharToMultiByte(CP_UTF8, 0, patchName, (int)_tcslen(patchName), workBuffer3, SMALLBUFFER, NULL, NULL);
			workBuffer3[byteLen] = 0;

			lRet = SendMessage(hPatchSet, LB_GETTEXT, i, (LPARAM)tcharBuffer);
			sprintf_s(workBuffer, SMALLBUFFER, "%02d - %s\r\n", (i + 1), workBuffer3);
			strcat_s(textBuffer, BUFSIZE, workBuffer);
		}
	}
}

int fillPatchesBuffer(HWND hPatchSet,int midiChannel,UCHAR *pWriteBuffer,DWORD noItems,DWORD *pItems,lpFM_BULK_OLD_PATCH lpPatches)
{
	DWORD outBufferPos = 0;
	int noFillers, byteLen;
	DWORD idx, i;	
	TCHAR patchName[SMALLBUFFER];
	char workString[SMALLBUFFER];
	char workBuffer3[SMALLBUFFER];
	lpFM_BULK_OLD_PATCH lpOldPatch;
	FM_SINGLE_OLD_PATCH	oldSinglePatch;


	if (noItems > 0)
	{

		//some items - 1 or many ?
		if (noItems > 1)
		{
			//write out the header
			memcpy(pWriteBuffer, FMDX9_BULK_HEADER, 6);
			outBufferPos = 6;
			//set the midi channel
			pWriteBuffer[MIDI_CHANNEL_POS] = (midiChannel - 1);

			//loop through the patches in the list box
			for (i = 0; i < noItems; i++)
			{
				//pull index from array if it exists
				idx = (pItems == NULL) ? i : pItems[i];

				//get a nice version of the name
				PreProcessName(hPatchSet, idx, patchName);
				//and an 8bit version of that
				byteLen = WideCharToMultiByte(CP_UTF8, 0, patchName, (int)_tcslen(patchName), workBuffer3, SMALLBUFFER, NULL, NULL);
				workBuffer3[byteLen] = 0;

				//get the index of this item into our master list of patches

//copy the patch to our output buffer
				memcpy(&pWriteBuffer[outBufferPos], &lpPatches[idx], sizeof(FM_BULK_OLD_PATCH));
				//correct the DX9 name
				lpOldPatch = (lpFM_BULK_OLD_PATCH)&pWriteBuffer[outBufferPos];				
				//create the standard DX9 patch name
				sprintf_s(workString, SMALLBUFFER, "DX9.%*d     ", 2, i + 1);		

				memcpy(lpOldPatch->PatchName, workString, FM_PATCH_NAME_SIZE);
				outBufferPos += sizeof(FM_BULK_OLD_PATCH);
			}
			//see how many 'fillers' we need
			noFillers = 32 - noItems;
			//fill out the remainder of the outut buffer
			memset(&pWriteBuffer[outBufferPos], DX9FILLER, (sizeof(FM_BULK_OLD_PATCH)*noFillers));
			//add the checksum
			pWriteBuffer[SYSCHKSUM] = CalcChecksum(pWriteBuffer, SYSDATAPOS, SYSDATABYTES);
			//add EOF
			pWriteBuffer[SYSEOF] = SYSEND;
			return SYSLEN;
		}
		else
		{
//single patch - different format
//write out the header
			memcpy(pWriteBuffer, FMDX9_SINGLE_HEADER, 6);
			outBufferPos = 6;
//set the midi channel
			pWriteBuffer[MIDI_CHANNEL_POS] = (midiChannel - 1);
//get the 'bulk' patch
			i = 0;
			idx = (pItems == NULL) ? i : pItems[i];
//get the index of this item into our master list of patches
						
//convert the structure
			ConvertBulkToSingle(&lpPatches[idx], &oldSinglePatch);


/* test to see what happens if you send DX9 other alogrithm numbers
it turns out it reports no error but ignores patches which do not have the supported alogrithm numbers:
1(1),14(2),8(3),7(4),5(5),22,(6),31(7),32(8)
			static int rollAlgo = 0;
			wsprintf(genOut, L"Setting Algorithm to %d (%d)\n", rollAlgo, rollAlgo + 1);
			ConsoleOut(genOut);
			oldSinglePatch.Algorithm.Algorithm = rollAlgo++;
			if (rollAlgo == 32)
				rollAlgo = 0;
*/
//The patch name will oddly appear on the DX9 display (however fleetingly) if we do this, so we might as well
//get a nice version of the name
			PreProcessName(hPatchSet, idx, patchName);
			//and an 8bit version of that
			byteLen = WideCharToMultiByte(CP_UTF8, 0, patchName, (int)_tcslen(patchName), workBuffer3, SMALLBUFFER, NULL, NULL);
			memset(oldSinglePatch.PatchName, 32, FM_PATCH_NAME_SIZE);
			if (byteLen > FM_PATCH_NAME_SIZE)
				byteLen = FM_PATCH_NAME_SIZE;
			memcpy(oldSinglePatch.PatchName, workBuffer3, byteLen);

//copy the patch
			memcpy(&pWriteBuffer[outBufferPos], &oldSinglePatch, sizeof(oldSinglePatch));
//
//add the checksum
			pWriteBuffer[SYSCHKSUM_SINGLE] = CalcChecksum(pWriteBuffer, SYSDATAPOS, SYSDATABYTES_SINGLE);
//add EOF
			pWriteBuffer[SYSEOF_SINGLE] = SYSEND;			
			return SYSLEN_SINGLE;
		}		
	}
	return -1;

}


//main file processing funtion
BOOL ProcessFile(HANDLE hFile, PHANDLE phInputBuffer, PHANDLE phOutputBuffer, PHANDLE phNameBuffer,int midiChannel,TCHAR *fileName,HWND hFileList)
{
	DWORD dwBytesRead = 0;	
//buffers
	UCHAR *readBuffer;
	UCHAR *writeBuffer;		
					
	WORD dumpFormat;
//try to read the file in	
	if (!ReadFile(hFile, phInputBuffer, INPUTBUFFERSIZE, &dwBytesRead, &ol))
	{
		wsprintf(genOut, L"Failed to read file - error %d\n", GetLastError());
		ConsoleOut(genOut);		
		return FALSE;
	}

//create a comparison init patch	
	memset(&DX9INITPATCH, 0x2e, sizeof(FM_BULK_OLD_PATCH));
	memset(&DX9INITSINGLEPATCH, 0x2e, sizeof(FM_SINGLE_OLD_PATCH));
	readBuffer = (UCHAR *)phInputBuffer;
	writeBuffer = (UCHAR *)phOutputBuffer;
//check it's a valid SYSEX file
	if (!ValidDXSysEx(readBuffer, dwBytesRead))
		return FALSE;
		
//figure out which format it is
	dumpFormat = GetDumpFormat(readBuffer);
	
//so. we either add DX9 patches "as is", or convert other 4op files
	switch (dumpFormat)
	{
		case SINGLE_VOICE:
			ConsoleOut(L"Single voice data dump (DX11/21/27/100)\r\nComverting and adding\r\n");
			AddSingleVoice(hFile, readBuffer, hFileList, TRUE);			
			break;
		case BULK_VOICE:			
			ConsoleOut(L"Bulk voice data dump (DX11/21/27/100)\r\nComverting and adding\r\n");
			AddBulkVoices(hFile, readBuffer, hFileList, TRUE);		
			break;
		case OLD_SINGLE_VOICE:			
			ConsoleOut(L"Original DX single voice dump (DX7/DX9)\r\nAdding Patch\r\n");
			AddSingleVoice(hFile, readBuffer, hFileList);
			break;
		case OLD_BULK_VOICE:
			ConsoleOut(L"Origingal DX bulk voice dump (DX7/9)\r\nAdding Patches\r\n");			
			AddBulkVoices(hFile,readBuffer,hFileList);
			break;
		case ADDITIONAL_VOICE:
			ConsoleOut(L"2GEN Single voice (DX11/TX81Z)\r\nComverting and adding\r\n");
			AddSingleVoice(hFile, readBuffer, hFileList, TRUE, TRUE,dwBytesRead);			
			break;
		default:
			ConsoleOut(L"Invalid or Unsupported Voice Dump format\r\n");
			return FALSE;
	}
			
	return TRUE;
}


void GetPatchNameFile(TCHAR *tIn, TCHAR *tOut)
{
	TCHAR tWork[BUFSIZE];
	int xx, y;
	//add the prefix
	wcscpy_s(tWork, BUFSIZE, tIn);
	xx = (int)_tcslen(tIn);
	for (y = xx; y > 0; y--)
	{
		if (tWork[y] == '.')
			tWork[y] = 0;
		if (tWork[y] == '\\')
			break;
	}
	swprintf(tOut, BUFSIZE, TEXT("%s.txt"), tWork);

}

void AddSingleVoiceStruct(HANDLE hFile, lpFM_SINGLE_OLD_PATCH lpOldSinglePatch, HWND hFileList, char *workBuffer)
{
	FM_BULK_OLD_PATCH oldBulkPatch;
	DWORD dwBytesRead = 0;
	TCHAR patchName[SMALLBUFFER];	
	TCHAR tWork[BUFSIZE]; 
	TCHAR tBuffer[BUFSIZE];
	int addIdx;
	DWORD dwRet;
	char work[16];
	char txtPatchName[SMALLBUFFER];
	int listCount;

	listCount=(int)SendMessage(hFileList, LB_GETCOUNT, (WPARAM)0, (LPARAM)patchName);
	if (listCount >= MAX_DX9_BULK_PATCHES)
	{
		ConsoleOut(L"Max number of DX9 bulk patches loaded\r\n");
		return;
	}

	work[FM_PATCH_NAME_SIZE] = 0;
	dwRet = GetFinalPathNameByHandle(hFile, tWork, BUFSIZE, VOLUME_NAME_DOS);
	GetPatchNameFile(tWork, tBuffer);
	ConsoleOut(tBuffer);
	ConsoleOut(L"\r\n");

	//if this is a real patch, add it
	if (!isInitSinglePatch(lpOldSinglePatch))
	{
		//valid patch, so create a 'bulk' patch from it
		ConvertSingleToBulk(lpOldSinglePatch, &oldBulkPatch);
//deal with the name
		memcpy(work, oldBulkPatch.PatchName, FM_PATCH_NAME_SIZE);
		//check for a name in the text file
		if (getTextPatchName(0, workBuffer, dwBytesRead, txtPatchName))
		{
			wsprintf(patchName, L"[%03d] %S", (listCount+1), txtPatchName);
			memcpy(oldBulkPatch.PatchName, txtPatchName, FM_PATCH_NAME_SIZE);
		}
		else
			wsprintf(patchName, L"[%03d] %S", (listCount+1), work);
		addIdx = (int)SendMessage(hFileList, LB_ADDSTRING, 0, (LPARAM)patchName);
		//add the index into our structure list as a data item
		if (addIdx != LB_ERR)
		{
			//add the data
			if (AutoSelectLoaded)
			{
				//de-select everything
				SendMessage(hFileList, LB_SELITEMRANGE, (WPARAM)FALSE, (LPARAM)(LOWORD(0) | HIWORD(-1)));
				//select the addition
				SendMessage(hFileList, LB_SETSEL, (WPARAM)TRUE, (LPARAM)addIdx);
			}
//add the data
			memcpy(&DX9BulkPatch[addIdx], &oldBulkPatch, sizeof(FM_BULK_OLD_PATCH));

			SetListButtons();
		}
	}
}

void AddSingleVoice(HANDLE hFile, UCHAR *readBuffer, HWND hFileList, BOOL newPatch,BOOL additional,DWORD dwBytesRead)
{
	DWORD inBufferPos = 6;
	TCHAR patchName[SMALLBUFFER];
	TCHAR tWork[BUFSIZE];
	TCHAR tBuffer[BUFSIZE];
	char txtPatchName[SMALLBUFFER];
	DWORD dwTextBytesRead = 0;
	char workBuffer[BUFSIZE];
	FM_BULK_OLD_PATCH oldBulkPatch;
	FM_SINGLE_OLD_PATCH oldSinglePatch;
	lpFM_SINGLE_OLD_PATCH lpOldSinglePatch;	
	int chkLen = sizeof(FM_SINGLE_ADD_HEADER);
	int listCount;

	DWORD dwRet;
	char work[16];
	HANDLE hTxtFile;
	
	int addIdx;

	work[FM_PATCH_NAME_SIZE] = 0;
	dwRet = GetFinalPathNameByHandle(hFile, tWork, BUFSIZE, VOLUME_NAME_DOS);
	GetPatchNameFile(tWork, tBuffer);
	ConsoleOut(tBuffer);
	ConsoleOut(L"\r\n");

	hTxtFile = CreateFile(tBuffer,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (hTxtFile != INVALID_HANDLE_VALUE)
	{

		//try to read the file in	
		if (!ReadFile(hTxtFile, workBuffer, BUFSIZE, &dwTextBytesRead, &ol))
		{
			wsprintf(genOut, L"Failed to read file - error %d\n", GetLastError());
			ConsoleOut(genOut);
			dwTextBytesRead = 0;
		}
		else
			ConsoleOut(L"Sucessfully read patch name file\r\n");

	}
	//get the single patch
	lpOldSinglePatch = (lpFM_SINGLE_OLD_PATCH)&readBuffer[inBufferPos];

//"new", so convert
	if (newPatch)
	{
		lpOldSinglePatch = &oldSinglePatch;
//2nd gen patch ?
		if (additional)
//get the additional params
			ConvertAdditionalVoice(readBuffer, lpOldSinglePatch, SendMIDIChannel, dwBytesRead);
		else
			ConvertSingleVoice(readBuffer, lpOldSinglePatch, SendMIDIChannel);
	}
	
//get the current list count
	listCount = (int)SendMessage(hFileList, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

	if (listCount >= MAX_DX9_BULK_PATCHES)
	{
		ConsoleOut(L"Max number of DX9 bulk patches loaded\r\n");
		return;
	}



	//if this is a real patch, add it
	if (!isInitSinglePatch(lpOldSinglePatch))
	{
//valid patch, so create a 'bulk' patch from it
		ConvertSingleToBulk(lpOldSinglePatch, &oldBulkPatch);
//then add it
		memcpy(&DX9BulkPatch[listCount], &oldBulkPatch, sizeof(FM_BULK_OLD_PATCH));
		
		memcpy(work, oldBulkPatch.PatchName, FM_PATCH_NAME_SIZE);
		//check for a name in the text file
		if (getTextPatchName(0, workBuffer, dwTextBytesRead, txtPatchName))
		{
			wsprintf(patchName, L"[%03d] %S", (listCount+1), txtPatchName);
			memcpy(oldBulkPatch.PatchName, txtPatchName, FM_PATCH_NAME_SIZE);
		}
		else
			wsprintf(patchName, L"[%03d] %S", (listCount + 1), work);
		addIdx = (int)SendMessage(hFileList, LB_ADDSTRING, 0, (LPARAM)patchName);
		//add the index into our structure list as a data item
		if (addIdx != LB_ERR)
		{

			if (AutoSelectLoaded)
			{
				//de-select everything
				SendMessage(hFileList, LB_SELITEMRANGE, (WPARAM)FALSE, (LPARAM)(LOWORD(0) | HIWORD(-1)));
				//select the addition
				SendMessage(hFileList, LB_SETSEL, (WPARAM)TRUE, (LPARAM)addIdx);
			}
			SetListButtons();
		}
	}
	if (hTxtFile != NULL)
		CloseHandle(hTxtFile);
}

void AddBulkVoices(HANDLE hFile,UCHAR *readBuffer,HWND hFileList,BOOL newPatch)
{
	DWORD inBufferPos = 6;
	TCHAR patchName[SMALLBUFFER];
	TCHAR tWork[BUFSIZE];
	TCHAR tBuffer[BUFSIZE];
	char txtPatchName[SMALLBUFFER];
	DWORD dwBytesRead = 0;
	char workBuffer[BUFSIZE];
	int minIdx = 32000;
	int maxIdx = -1;
	int listCount;
	DWORD dwRet;
	char work[16];
	HANDLE hTxtFile;
	//'old' format file - so just add the patches
	lpFM_BULK_OLD_PATCH lpOldPatch;
	lpFM_BULK_NEW_PATCH lpNewPatch;
	FM_BULK_OLD_PATCH oldPatch;
	//dump the voice data details 
	work[FM_PATCH_NAME_SIZE] = 0;
	int addIdx;


	dwRet = GetFinalPathNameByHandle(hFile, tWork, BUFSIZE, VOLUME_NAME_DOS);
	GetPatchNameFile(tWork, tBuffer);
	ConsoleOut(tBuffer);
	ConsoleOut(L"\r\n");

	hTxtFile = CreateFile(tBuffer,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (hTxtFile != INVALID_HANDLE_VALUE)
	{		
	
		//try to read the file in	
		if (!ReadFile(hTxtFile, workBuffer, BUFSIZE, &dwBytesRead, &ol))
		{
			wsprintf(genOut, L"Failed to read file - error %d\r\n", GetLastError());
			ConsoleOut(genOut);
			dwBytesRead = 0;

		}
		else	
			ConsoleOut(L"Sucessfully read patch name file\r\n");
	
	}

	listCount = (int)SendMessage(hFileList, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

	for (int i = 0; i < 32; i++, inBufferPos += FMDX9_PATCH_DATA_SIZE)
	{
		if (listCount >= MAX_DX9_BULK_PATCHES)
		{
			ConsoleOut(L"Max number of DX9 bulk patches loaded\r\n");
			return;
		}

		if (newPatch)
		{
			lpNewPatch=(lpFM_BULK_NEW_PATCH)&readBuffer[inBufferPos];
//comvert the patch
			lpOldPatch = &oldPatch;
			ConvertVoice(lpNewPatch, i, lpOldPatch);					
		}
		else
			lpOldPatch = (lpFM_BULK_OLD_PATCH)&readBuffer[inBufferPos];
//if this is a real patch, add it
		if (!isInitPatch(i,lpOldPatch,listCount))
		{					
		
			memcpy(work, lpOldPatch->PatchName, FM_PATCH_NAME_SIZE);
			//check for a name in the text file
			if (getTextPatchName(i, workBuffer, dwBytesRead, txtPatchName))
			{
				wsprintf(patchName, L"[%03d] %S", (listCount+1), txtPatchName);
				memcpy(lpOldPatch->PatchName, txtPatchName, FM_PATCH_NAME_SIZE);
			}
			else
				wsprintf(patchName, L"[%03d] %S", (listCount+1), work);
				
			addIdx=(int)SendMessage(hFileList, LB_ADDSTRING, 0, (LPARAM)patchName);
			
			
//add the index into our structure list as a data item
			if (addIdx != LB_ERR)
			{
				if (addIdx < minIdx)
					minIdx = addIdx;
				if (addIdx > maxIdx)
					maxIdx = addIdx;
				memcpy(&DX9BulkPatch[addIdx], lpOldPatch, sizeof(FM_BULK_OLD_PATCH));
				listCount++;
			}
		}		
	}

	if (AutoSelectLoaded)
	{
		//de-select everything
		SendMessage(hFileList, LB_SELITEMRANGE, (WPARAM)FALSE, (LPARAM)(LOWORD(0) | HIWORD(-1)));
		//select the additions
		SendMessage(hFileList, LB_SELITEMRANGE, (WPARAM)TRUE, MAKELPARAM(minIdx, maxIdx));
	}
	if (hTxtFile != NULL)
		CloseHandle(hTxtFile);
}


BOOL isInitSinglePatch(lpFM_SINGLE_OLD_PATCH lpOldSinglePatch)
{
	//check if this in an initialised patch
	if (!memcmp(lpOldSinglePatch, &DX9INITSINGLEPATCH, sizeof(FM_SINGLE_OLD_PATCH)))
		return TRUE;
	//or if it has "INIT PATCH" as a name
	if (!memcmp(lpOldSinglePatch->PatchName, &DX9INITPATCHNAME, FM_PATCH_NAME_SIZE))
		return TRUE;
	return FALSE;
}

BOOL isInitPatch(int patchNo,lpFM_BULK_OLD_PATCH lpOldPatch,int lastIdx)
{
	//check if this in an initialised patch
	if (!memcmp(lpOldPatch, &DX9INITPATCH, sizeof(FM_BULK_OLD_PATCH)))
		return TRUE;
//or if it has "INIT PATCH" as a name
	if (!memcmp(lpOldPatch->PatchName, &DX9INITPATCHNAME, FM_PATCH_NAME_SIZE))
		return TRUE;
//slightly more complicated - the DX9 will repeat the last patch (patch 20) 12 times in the sysex file under certain circumstances
	if (patchNo>19 && !memcmp(lpOldPatch, &DX9BulkPatch[lastIdx], 118))
		return TRUE;
	return FALSE;
}

//check for a patch name in the text file
BOOL getTextPatchName(int index, char *txtWork,DWORD dwBytesRead,char *patchName)
{
	char *pWork;
	char *pEnd;
	char work[SMALLBUFFER];
	if (dwBytesRead > 0)
	{
		sprintf_s(work, 16,"%02d - ", index+1);
		pWork = strstr(txtWork, work);
		if (pWork)
		{
//Found something like...
//01 - Name
//so skip to text			
			pWork += 5;

//try to find the EOL
			pEnd = strstr(pWork, "\r");
//this will force len to be 10 if we don't find the CR	
			int len;
			if (pEnd)
			{
				len = (int)(pEnd - pWork);
				if (len > FM_PATCH_NAME_SIZE)
					len = FM_PATCH_NAME_SIZE;
			}
			else
				len = FM_PATCH_NAME_SIZE;
			strncpy_s(patchName, SMALLBUFFER,pWork, len);
			return TRUE;
		}	
	}
	return FALSE;
}
//get the data dump format
WORD GetDumpFormat(PUCHAR readBuffer)
{
	switch (readBuffer[3])
	{
		case 0x00:
			return OLD_SINGLE_VOICE;
		case 0x03:
			return SINGLE_VOICE;
		case 0x04:
			return BULK_VOICE;		
		case 0x09:
			return OLD_BULK_VOICE;
		case 0x7e:
			return ADDITIONAL_VOICE;

	}
	return FORMAT_ERROR;
}

//Check that it's a valid SysEx File
BOOL ValidDXSysEx(PUCHAR readBuffer,DWORD  dwBytesRead)
{
//check the easy bytes
//start
	if (readBuffer[0] != START_BYTE)
	{
		ConsoleOut(L"**** Not a MIDI Sysex file\r\n");
		return FALSE;
	}
//manufacturer's ID
	if (readBuffer[1] != ID_BYTE)
	{		
		ConsoleOut(L"**** Not a Yamaha SysEx File\r\n");

	}
//MIDI channel
	if (readBuffer[2] > 15)
	{
		ConsoleOut(L"**** Invalid Midi channel in Sysex file\r\n");
		return false;
	}
	else
	{
		swprintf(genOut, BUFSIZE, TEXT("SysEx MIDI Channel value (%d)\r\n"), (int)readBuffer[2] + 1);
		ConsoleOut(genOut);		
	}


//End byte
	if (readBuffer[dwBytesRead - 1] != END_BYTE)
	{
		ConsoleOut(L"Invalid Sysex File (incorrect last byte)\r\n");
		return FALSE;
	}
	
	return TRUE;
}

void SingleOperatorConvert(lpFM_SINGLE_NEW_PATCH lpNewPatch, lpFM_SINGLE_OLD_PATCH lpOldPatch,lpFM_SINGLE_ADD_PARAMS lpFMAdditional)
{
	lpFM_SINGLE_OPERATOR_NEW lpNewOperator;
	lpFM_SINGLE_OPERATOR_OLD lpOldOperator;
	lpFM_SINGLE_ADD_OP	lpAddOperator = NULL;

	UCHAR algo;
	FM_DX9_CARRIERS OpsMap;
	

	/*
	loop through the 4 operators
	Note that the operators oddly and helpfully go 4,2,3,1
	*/

	//get the algorithm, as we may need to remap the operators (algorithm 3 (7))
	algo = lpNewPatch->Algorithm.Algorithm;
	//get the map
	OpsMap = OPERATORMAP[algo];
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT("Algo :%d \r\n"), algo);
	ConsoleOut(genOut);
#endif
	for (int i = 0; i < 4; i++)
	{

		//get pointers to the operators
		int mappedOp = OpsMap.Operators[i];
		lpNewOperator = &lpNewPatch->FMOp[mappedOp];
		if (lpFMAdditional)
			lpAddOperator = &lpFMAdditional->FMOp[mappedOp];

		lpOldOperator = &lpOldPatch->FMOp[i];
			
		//Envlopes
#ifdef _DX9_DEBUG
		swprintf(genOut, BUFSIZE, TEXT("Ops i -%d from %d, EG %d - "), i, OpsMap.Operators[i], (i + 1));
		ConsoleOut(genOut);
#endif
		EGConvert(&lpNewOperator->EG, &lpOldOperator->EG);
		//Frequency
		SingleFreqConvert(lpNewOperator, lpOldOperator,lpAddOperator);
		//General Operator values
		//output level
		lpOldOperator->OutputLevel = FM_DX9_OUTLEVEL[lpNewOperator->OutputLevel];
		//DX9 has simple keyboard scaling compared to DX7, just rate and level
		lpOldOperator->KeyRateScaling = SCALINGRATECONVERT[lpNewOperator->KeyScalingRate];
		//scaling level
		//DX9 uses the ScaleRightDepth parameter for level scaling
		lpOldOperator->ScaleRightDepth = lpNewOperator->KeyScalingLevel;
			
		//default the other scaling parameters
		lpOldOperator->BreakPoint = 0x0F;
		lpOldOperator->ScaleLeftDepth = 0x00;
		lpOldOperator->ScaleRightCurve = 0x01;
		lpOldOperator->ScaleLeftCurve = 0x00;
		//key velocity is always zero,
		lpOldOperator->KeyVelSens = 0;
		
		//amplitute mod sensitvity
		int amod = lpNewPatch->LFOParams.AModSens;
		//Amplitute mod enable is on/off per operator, so get flag		
		//then set amp mod for this operator		
		lpOldOperator->AmpModSens = amod*lpNewOperator->AmpModEnable;;			
	}

}

void OperatorConvert(lpFM_BULK_NEW_PATCH lpNewPatch, lpFM_BULK_OLD_PATCH lpOldPatch)
{
	lpFM_BULK_OPERATOR_NEW lpNewOperator;
	lpFM_BULK_OPERATOR_OLD lpOldOperator;
	lpFM_BULK_ADD_OPERATOR lpAddOperator;
	UCHAR work;
	UCHAR algo;
	FM_DX9_CARRIERS OpsMap;

/*
loop through the 4 operators
Note that the operators oddly and helpfully go 4,2,3,1
*/

//get the algorithm, as we may need to remap the operators (algorithm 3 (7))
	algo = (lpNewPatch->LFOParams.SyncFBackAlgo)&0x07;
//get the map
	OpsMap = OPERATORMAP[algo];
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT("Algo :%d \r\n"), algo);
	ConsoleOut(genOut);
#endif
	for (int i = 0; i < 4; i++)
	{
		int mappedOp = OpsMap.Operators[i];
//get pointers to the operators
		lpNewOperator = &lpNewPatch->FMOp[mappedOp];
		lpAddOperator = &lpNewPatch->AddOp[mappedOp];
		lpOldOperator = &lpOldPatch->FMOp[i];
		
//Envlopes
#ifdef _DX9_DEBUG
		swprintf(genOut, BUFSIZE, TEXT("Ops i -%d from %d, EG %d - "), i, OpsMap.Operators[i], (i + 1));
		ConsoleOut(genOut);
#endif
		EGConvert(&lpNewOperator->EG, &lpOldOperator->EG);
//Frequency
		FreqConvert(lpNewOperator, lpOldOperator,lpAddOperator);
//General Operator values
//output level
		lpOldOperator->OutputLevel = FM_DX9_OUTLEVEL[lpNewOperator->OutputLevel];
//DX9 has simple keyboard scaling compared to DX7, just rate and level
//scaling rate
//breakdown RateScalingDetune
//b4 3 - Keyboard Scaling rate (0-3)
//b2 1 0 - Detune (0-6, centre is 3)
		work = (lpNewOperator->RateScalingDetune>>3)&0x03;
//rate scaling in bottom 3 bits
		lpOldOperator->OscDetuneRateScale |= SCALINGRATECONVERT[work];
//scaling level
//DX9 uses the ScaleRightDepth parameter for level scaling
		lpOldOperator->ScaleRightDepth = lpNewOperator->LevelScaling;
		
//default the other scaling parameters
		lpOldOperator->BreakPoint = 0x0F;
		lpOldOperator->ScaleLeftDepth = 0x00;
		lpOldOperator->ScaleLRCurve = 0x04;
				
//key velocity is always zero, just need amplitute mod sensitvity (comes from performance parameters)
		int amod = (lpNewPatch->PerfParameters.PmsAmsLFOWave >> 2) & 3;
//Amplitute mod enable is on/off per operator, so get flag
		int ame = (lpNewOperator->AmeEGBiasVel >> 6) & 1;
//then set amp mod for this operator		
		lpOldOperator->KeyVelSensAModSens = amod*ame;
	}
}

//calculate the coarse, fine and detune frequencies for the DX9 new style operator
void CalcFreqDetune(UCHAR freqIn,UCHAR fineIn,UCHAR detuneIn, UCHAR *pCoarseOut, UCHAR *pFineOut, UCHAR *pDetuneOut)
{
	UCHAR coarse = 0;
	UCHAR fine = 0;
	UCHAR work = 0;
	UCHAR oldDetune;
	double newFreqVal;
	double oldFreqVal;
	double freqFine;
	double fineStep;
	double detuneStep;
	double fineWork;
	double calcFreq;
	int i;
// the frequency (0-63) from new 4 op
//get a double that represents that ratio
//	newFreqVal = DX21FREQ[freqIn];
//support 2gen DX11 / TX81Z
	fineIn &= 15;
	freqIn &= 63;
	newFreqVal = TX81FREQ[freqIn][fineIn];
	//derive coarse and fine values
	i = 0;
	//find the nearest coarse freq that is less than or equal to the 4op freq
	while (i<32 && DX9COARSEFREQ[i] <= newFreqVal)
		i++;
	if (i > 0)
		i--;

	//the index now points to our coarse setting
	oldFreqVal = DX9COARSEFREQ[i];
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT("In freq (%d) %d (%f) -> %d (%f) "), lpOperatorNew->OSCFreq, newFreq, newFreqVal, i, oldFreqVal);
	ConsoleOut(genOut);
#endif
	coarse = i;
	//given this coarse freq, get the fine steps
	fineStep = oldFreqVal / 100.0F;
	//see if there is any fine needed i.e any difference between the DX9 coarse value and the newer 4op ratio value
	freqFine = newFreqVal - oldFreqVal;
	if (freqFine > 0.0)
	{
		//there is a difference, so work out what that is in the available DX9 fine steps		
		//work out the number of steps
		fineWork = freqFine / fineStep;
		fine = (UCHAR)(int)fineWork;
		calcFreq = oldFreqVal + (fineStep*(double)fine);
#ifdef _DX9_DEBUG
		swprintf(genOut, BUFSIZE, TEXT("fine delta:%f , step %f, steps %d , calc %f"), freqFine, fineStep, fine, calcFreq);
		ConsoleOut(genOut);
#endif
	}
	//set the coarse (0-31);
	coarse &= 0x1f;
	*pCoarseOut = coarse;
	//then the fine (0-99)
	*pFineOut = fine;
	//detune
	//DX9 Detune breaks the current fine step into 15 smaller steps, centered around 7
	detuneStep = fineStep / 15.0F;
	//Later 4op detune does +/- 2 cents in 7 steps, centered around 3
	//interestingly 
	//The DX21 manual says it's 2 cents and the values are -7 to +7, though the bulk data format at least (VMEM) only allows for -3 to +3
	//DX100, 27,11 and tx81z all agree it's 2.6 cents -3 to +3
	//comething like:
	//    0       1        2     3     4     5       6
	//  -2.6    -1.73    -0.87   0    0.87  1.73    2.6
	//
	//we're doing a simple translation of the detune - just moving the centre to 7 rather than 3
	//
//	newDetune = lpOperatorNew->RateScalingDetune & 7;
//	newFreq = lpOperatorNew->OSCFreq;	
	/*
	Here's some maths to translate the cents into ratios - a WIP so sticking with the simple 'recentre' approach for now
	maxCents = 2.6F;
	centStep = maxCents / 3.0f;
	centWork = ((float)(newDetune-3.0f))*centStep;
	//form.ratioout.value = Math.round(1000000 * Math.pow(2, (centin / 100 / 12))) / 1000000;

	centRatio = ((1000000.0f * pow(2, (centWork/100.0f/12.0f))) / 1000000.0f)-1.0f;

	oldDetune = (UCHAR)(centRatio / detuneStep);
	printf("\n in det %d cent %f , rat %f, detStep %f, oDet %d", newDetune,centWork, centRatio,detuneStep,oldDetune);
	*/
	oldDetune = DETUNECONVERT[detuneIn];
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT(" Det in:%d out%d \r\n"), detuneIn, oldDetune);
	ConsoleOut(genOut);
#endif	
	*pDetuneOut = oldDetune;	
	return;
}

//convert the frequency settings of the operator
void FreqConvert(lpFM_BULK_OPERATOR_NEW lpOperatorNew, lpFM_BULK_OPERATOR_OLD lpOperatorOld, lpFM_BULK_ADD_OPERATOR lpAddOperator)
{
	UCHAR coarse;
	UCHAR fine;
	UCHAR detune;
	UCHAR inFreq = lpOperatorNew->OSCFreq;
	UCHAR inDetune = lpOperatorNew->RateScalingDetune & 7;
//fine - padded to 0 in DX21/27/100 sysex files, part of additional parameters for DX11/TX81Z
	UCHAR inFine = lpAddOperator->WaveFine & 0x0f;
//calculate/convert the coarse, fine and detune settings for this 'new style' operator
	CalcFreqDetune(inFreq,inFine,inDetune,&coarse, &fine, &detune);
//then set those into the DX9 operator
	lpOperatorOld->FreqCMode = (coarse << 1);
	lpOperatorOld->FreqFine = fine;
	lpOperatorOld->OscDetuneRateScale = detune << 3;	
	return;
}

//convert the frequency settings of the operator
void SingleFreqConvert(lpFM_SINGLE_OPERATOR_NEW lpOperatorNew, lpFM_SINGLE_OPERATOR_OLD lpOperatorOld, lpFM_SINGLE_ADD_OP lpFMAddOperator)
{
	UCHAR coarse;
	UCHAR fine;
	UCHAR detune;
	UCHAR inFreq = lpOperatorNew->OSCFreq;
	UCHAR inDetune = lpOperatorNew->Detune;
	UCHAR inFine = 0;

	if (lpFMAddOperator)
		inFine = lpFMAddOperator->FreqRangeFine;

	//calculate/convert the coarse, fine and detune settings for this 'new style' operator
	CalcFreqDetune(inFreq, inFine, inDetune, &coarse, &fine, &detune);
	//then set those into the DX9 operator
	lpOperatorOld->FreqCoarse = coarse;
	lpOperatorOld->FreqFine = fine;
	lpOperatorOld->OscMode = 1;
	lpOperatorOld->OscDetune = detune;
	return;
}

//convert from 'new' Envelopes to old ones
void EGConvert(lpFM_EG_NEW lpEgNew, lpFM_EG_OLD lpEgOld)
{

/*
EG notes
Converting from the later 4op 'Enhanced ADSR' to the older (and more flexible) DX9 4 levels and rates
L1 is 99
R1 is Attack Rate

L2 is Decay 1 Level
R2 is Decay 1 Rate

If Decay 2 Rate is 0
  L3 is Decay 1 Level (sustain)
else
  L3 is 0
R3 is Decay 2 rate

L4 is 0
R4 is release rate
*/

//level 1 is fixed at 99
	lpEgOld->Level1= 99;
//level 2 is Decay1Level
	lpEgOld->Level2 = SustainLevel[lpEgNew->Decay1Level];
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT("L1:%d , L2:%d(%d) , "), lpEgOld->Level1, lpEgOld->Level2, lpEgNew->Decay1Level);
	ConsoleOut(genOut);
#endif
//level 3 is the sustain level if D2R is 0
	if (lpEgNew->DecayRate2 == 0)
		lpEgOld->Level3 = SustainLevel[lpEgNew->Decay1Level];
	else
//otherwise level 3 is 0
		lpEgOld->Level3 = 0;
//level 4 is fixed - 0
	lpEgOld->Level4 = 0;
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT("L3:%d , L4:%d , "), lpEgOld->Level3, lpOEgOld->Level4);
	ConsoleOut(genOut);
#endif
//now the rates
//convert the attack rate
	lpEgOld->Rate1 = AttackRate[lpEgNew->AttackRate];
//the 2 decay rates
	lpEgOld->Rate2 = DecayRate[lpEgNew->DecayRate1];
	lpEgOld->Rate3 = DecayRate[lpEgNew->DecayRate2];
//then the release rate
	lpEgOld->Rate4 = ReleaseRate[lpEgNew->ReleaseRate];
#ifdef _DX9_DEBUG
	swprintf(genOut, BUFSIZE, TEXT("R1:%d(%d) , R2:%d(%d) ,  R3:%d(%d) , R4:%d(%d) "), lpEgOld->Rate1, lpEgNew->AttackRate,
		lpEgOld->Rate2, lpEgNew->DecayRate1, lpEgOld->Rate3, lpEgNew->DecayRate2, lpEgOld->Rate4, lpEgNew->ReleaseRate);				
	ConsoleOut(genOut);
#endif
	return;
}

//convert algorithm
void AlgorithmConvert(lpFM_BULK_NEW_PATCH lpNewPatch, lpFM_BULK_OLD_PATCH lpOldPatch)
{
//for compound values
	UCHAR outByte;

	//algorithm is in bottom 3 bits
	lpOldPatch->Algorithm.Algorithm = MAPALGO[(lpNewPatch->LFOParams.SyncFBackAlgo & 0x07)];
//there is an LFO sync in the 'new' 4ops, which is not on the DX9
//also there is a OSC sync on the DX9 which is no present in the newer 4ops
//so default the OSC sync to on or off ?
	outByte = OSC_BULK_SYNC_ON;
	//add the feedback (value in bits 5,4,3 move to 2,1,0)
	outByte |= ((lpNewPatch->LFOParams.SyncFBackAlgo & 0x38) >> 3);
	lpOldPatch->Algorithm.OscKeySyncFeedback = outByte;
}

void SingleAlgorithmConvert(lpFM_SINGLE_NEW_PATCH lpNewPatch, lpFM_SINGLE_OLD_PATCH lpOldPatch)
{
	lpOldPatch->Algorithm.Algorithm = MAPALGO[lpNewPatch->Algorithm.Algorithm];
	lpOldPatch->Algorithm.Feedback = lpNewPatch->Algorithm.Feedback;	
	lpOldPatch->Algorithm.OscSync = 1;
}

/*
New 4op LFOs =
TX81z
Speed:
00.007Hz   (150 seconds for 1 cycle)
50.000Hz   (50 cycles a second)

Delay:
0  seconds
15 seconds

DX21
Speed
00.06Hz (16 seconds for 1 cycle)
50.00Hz (50 cycles a second)

Delay:
0  seconds
15 seconds

DX9
00.06Hz (16 seconds for 1 cycle)
47.2Hz  (47 cycles a second)

->LFO speed maps closely to DX21/27/100 but vary significantly at lower settings for TX81z/DX11
*/

void SingleLFOConvert(lpFM_SINGLE_NEW_PATCH lpNewPatch, lpFM_SINGLE_OLD_PATCH lpOldPatch)
{
	lpOldPatch->Lfo.LFOSpeed = lpNewPatch->LFOParams.LFOSpeed;
	lpOldPatch->Lfo.LFODelay = lpNewPatch->LFOParams.LFODelay;
	lpOldPatch->Lfo.Transpose = lpNewPatch->LFOParams.Transpose;
	lpOldPatch->Lfo.LFOAmpModDepth = lpNewPatch->LFOParams.AModDepth;
	lpOldPatch->Lfo.LFOPitchModDepth = lpNewPatch->LFOParams.PModDepth;
	lpOldPatch->Lfo.LFOSync = 0;
	lpOldPatch->Lfo.WaveForm = LFOCONVERT[lpNewPatch->LFOParams.LFOWave];
	lpOldPatch->Lfo.PitchModSens = lpNewPatch->LFOParams.PModSens;
}

//convert LFO params
void LFOConvert(lpFM_BULK_NEW_PATCH lpNewPatch, lpFM_BULK_OLD_PATCH lpOldPatch)
{
//for compound values
	UCHAR outByte;
	UCHAR workByte;
//some simple values	
	lpOldPatch->Lfo.LFOSpeed = lpNewPatch->LFOParams.LFOSpeed;
	lpOldPatch->Lfo.LFODelay = lpNewPatch->LFOParams.LFODelay;
	lpOldPatch->Lfo.Transpose = lpNewPatch->PerfParameters.Transpose;

//Amp and pitch mod depth
	lpOldPatch->Lfo.LFOAmpModDepth = lpNewPatch->PerfParameters.AModDepth;
	lpOldPatch->Lfo.LFOPitchModDepth = lpNewPatch->PerfParameters.PModDepth;

//PmsAmsLFOWave breakdown
//b6 5 4 - Pitch Modulation Sensitivity (0-7)
//b3 2 - Amplitude Modulation Sensitivity (0-3)
//b1 0 - LFO Wave (0-3)
//get the LFO wave
	workByte = lpNewPatch->PerfParameters.PmsAmsLFOWave & 0x03;
//no LFO sync on DX9 - so bit 0 is always iff and we can start with the wave in bits 3 2 1
	outByte = LFOCONVERT[workByte]<<1;	
	//get pitch mod sensitivity - bits 6 5 4 in both 'old' and 'new' structures
	outByte |= (lpNewPatch->PerfParameters.PmsAmsLFOWave & 0x70);

	//breakdown LFOPModWaveSync;
	//b6 5 4 - LFO Pitch Mod Sensitivity (0-7)
	//b3 2 1 - LFO Wave (0-5)
	//b0	 - Sync (0-1)
	lpOldPatch->Lfo.LFOPModWaveSync = outByte;
	
	return;
}

//default out all the unused parameters in the patch
void SetDefaults(lpFM_BULK_OLD_PATCH lpOldPatch,int PatchNo)
{
	char dispName[16];
	
//set the 2 unnused operators in the DX9 structure to 0's
	memset(&lpOldPatch->FMOp[4], 0, sizeof(FM_BULK_OPERATOR_OLD)*2);

//pitch envelope (fixed for DX9)
	lpOldPatch->PitchEnvelope.Level1 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Level2 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Level3 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Level4 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Rate1 = PEG_RATE;
	lpOldPatch->PitchEnvelope.Rate2 = PEG_RATE;
	lpOldPatch->PitchEnvelope.Rate3 = PEG_RATE;
	lpOldPatch->PitchEnvelope.Rate4 = PEG_RATE;
//set the DX9 name, 
//be lazy and pad more than the length of the string, then just copy over the first 10 chars so we don't get the NULL
	sprintf_s(dispName,16, "DX9.%*d     ", 2,PatchNo+1);
	memcpy(lpOldPatch->PatchName, dispName, FM_PATCH_NAME_SIZE);
	return;
}

//default out all the unused parameters in the patch
void SetSingleDefaults(lpFM_SINGLE_OLD_PATCH lpOldPatch,char *dispName)
{	

	//set the 2 unnused operators in the DX9 structure to 0's
	memset(&lpOldPatch->FMOp[4], 0, sizeof(FM_SINGLE_OPERATOR_OLD) * 2);

	//pitch envelope (fixed for DX9)
	lpOldPatch->PitchEnvelope.Level1 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Level2 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Level3 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Level4 = PEG_LEVEL;
	lpOldPatch->PitchEnvelope.Rate1 = PEG_RATE;
	lpOldPatch->PitchEnvelope.Rate2 = PEG_RATE;
	lpOldPatch->PitchEnvelope.Rate3 = PEG_RATE;
	lpOldPatch->PitchEnvelope.Rate4 = PEG_RATE;
	//set the DX9 name, 
	memcpy(lpOldPatch->PatchName, dispName, FM_PATCH_NAME_SIZE);
	return;
}





UCHAR CalcChecksum(UCHAR *writeBuffer, int startPos, int dataLen)
{
	UCHAR work = 0;
	UCHAR *workPointer = &writeBuffer[startPos];
	//total of all data bytes
	for (int i = 0; i < dataLen; i++, workPointer++)
		work += *workPointer;
	//generate the 2's complement checksum midi style (no bit 7)
	work = (~work + 1) & 0x7f;
	return work;
}


//Convert the new patch to 'old' style DX9 patch
BOOL ConvertVoice(lpFM_BULK_NEW_PATCH lpNewPatch,int PatchNo,lpFM_BULK_OLD_PATCH lpOldPatch)
{
	char dispName[12];
//Get the voice name
	dispName[FM_PATCH_NAME_SIZE] = 0;
	memcpy(dispName, lpNewPatch->PatchName, FM_PATCH_NAME_SIZE);
	swprintf(genOut, BUFSIZE, TEXT("Converting Voice (%d) : %S\r\n"), (PatchNo + 1), dispName);
	ConsoleOut(genOut);

//Now convert to the DX9
//set the easy stuff
	SetDefaults(lpOldPatch,PatchNo);
//Algorithm
	AlgorithmConvert(lpNewPatch, lpOldPatch);
//convert the operators
	OperatorConvert(lpNewPatch, lpOldPatch);
//LFO
	LFOConvert(lpNewPatch, lpOldPatch);
//lazy reset/copy of name
	memcpy(lpOldPatch->PatchName, dispName, FM_PATCH_NAME_SIZE);
	return TRUE;
}

//write out the DX9 voice dump structure
BOOL WriteDX9BulkFile(LPCWSTR fileName, UCHAR *outBuffer,int outLen)
{
	HANDLE hFile;
	DWORD bytesWritten;
	BOOL retValue = TRUE;

	hFile = CreateFile(fileName,                // name of the write
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,             // create file always, even if it already exists
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template
	
	if (hFile == INVALID_HANDLE_VALUE)
	{
		swprintf(genOut, BUFSIZE, TEXT("Unable to create output file %s\r\n"), fileName);
		ConsoleOut(genOut);
		return FALSE;
	}

	if (!WriteFile(hFile, outBuffer, outLen, &bytesWritten, NULL))
	{
		swprintf(genOut, BUFSIZE, TEXT("Failed to write to output file %s\r\n"), fileName);
		ConsoleOut(genOut);
		retValue = FALSE;
	}
	CloseHandle(hFile);

	return retValue;
}

//write out the DX9 20 voice dump structure
BOOL WriteDX9PatchNames(LPCWSTR fileName, LPCSTR outBuffer)
{
	HANDLE hFile;
	DWORD bytesWritten;
	BOOL retValue = TRUE;
	
	int fPos = 0;
	int xx = (int)_tcslen(fileName);
	for (fPos = xx; fPos > 0; fPos--)
		if (fileName[fPos] == '\\')
			break;

	hFile = CreateFile(fileName,                // name of the write
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,             // create file always, even if it already exists
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		swprintf(genOut, BUFSIZE, TEXT("**** Unable to create output file %ls\r\n"), fileName);		
		ConsoleOut(genOut);
		return FALSE;
	}


	if (!WriteFile(hFile, outBuffer, (DWORD)strlen((char *)outBuffer), &bytesWritten, NULL))
	{
		swprintf(genOut, BUFSIZE, TEXT("**** Failed to write to output file %ls\r\n"), fileName);
		ConsoleOut(genOut);
		retValue = FALSE;
	}
	CloseHandle(hFile);

	return retValue;
}

void getPatchName(char *outName, char *patchName)
{
	int i;
	int x;
//copy across the filename
	return;
	strcpy_s(patchName,1024, outName);
	return;
//read backwards till we find a '.'

	x = (int)strlen(patchName);
	for (i = x; i >=0; i--)
	{
		if (patchName[i] == '.')
		{
			patchName[i] = 0;
			strcat_s(patchName, BUFSIZE,".txt");
		}
	}
	swprintf(genOut, BUFSIZE, TEXT("Get patch name %S -> %S\r\n"), outName, patchName);
	ConsoleOut(genOut);
}

void getStrName(lpFM_BULK_NEW_PATCH lpNewPatch,char *dispName)
{
	dispName[FM_PATCH_NAME_SIZE] = 0;
	strncpy_s(dispName, 16, (char *)lpNewPatch->PatchName, FM_PATCH_NAME_SIZE);
}
	

void getStrSingleName(lpFM_SINGLE_NEW_PATCH lpNewPatch, char *dispName)
{
	dispName[FM_PATCH_NAME_SIZE] = 0;
	strncpy_s(dispName, 16, (char *)lpNewPatch->PatchName, FM_PATCH_NAME_SIZE);
}

//convert 2gen voice - (has additional params)
void ConvertAdditionalVoice(UCHAR *readBuffer, lpFM_SINGLE_OLD_PATCH lpOldPatch, int midiChannel, DWORD dwBytesRead)
{
	DWORD inBufferPos = 0;
	inBufferPos = 6;
	WORD dumpFormat;
	int chkLen = sizeof(FM_SINGLE_ADD_HEADER);	
	lpFM_SINGLE_ADD_PARAMS lpFMAdditional;
	//check this is not just the additional dump without the main voice
	if (dwBytesRead < FM_2GEN_SINGLE_SIZE)
	{
		swprintf(genOut, BUFSIZE, TEXT("**** File size (%d) is too small for 2Gen Additional + Standard Voice Params\r\n"), dwBytesRead);
		ConsoleOut(genOut);
		return;
	}
	//read in the additional voice details
//check the header
	if (memcmp(&readBuffer[inBufferPos], FM_SINGLE_ADD_HEADER, chkLen))
	{
		ConsoleOut(L"***** 2Gen voice has incorrect text header\r\n");
		return;
	}
	
	inBufferPos += chkLen;
	lpFMAdditional = (lpFM_SINGLE_ADD_PARAMS)&readBuffer[inBufferPos];
	//move onto the voice params
	readBuffer += FM_2GEN_ADD_SIZE;
	//and check the format again
	if (ValidDXSysEx(readBuffer, (dwBytesRead - FM_2GEN_ADD_SIZE)))
	{
		dumpFormat = GetDumpFormat(readBuffer);

		if (dumpFormat == SINGLE_VOICE)
		{
			ConsoleOut(L"Single voice data found in 2Gen dump - converting\r\n");		
			ConvertSingleVoice(readBuffer, lpOldPatch, midiChannel, lpFMAdditional);
		}
		else
			//anything is an error
			ConsoleOut(L"***** Format of 2Gen dump is incorrect, aborting \r\n");
	}
	else
		ConsoleOut(L"**** Invalid SYSEX Format in 2Gen dump , aborting \r\n");

}



//convert single voice data dump
void ConvertSingleVoice(UCHAR *readBuffer, lpFM_SINGLE_OLD_PATCH lpOldPatch, int midiChannel, lpFM_SINGLE_ADD_PARAMS lpFMAdditional)
{

	char dispName[16];

	DWORD inBufferPos = 0;
	DWORD outBufferPos = 0;
	lpFM_SINGLE_NEW_PATCH lpNewPatch;		
	inBufferPos = 6;

	lpNewPatch = (lpFM_SINGLE_NEW_PATCH)&readBuffer[inBufferPos];
	getStrSingleName(lpNewPatch, dispName);
	
	swprintf(genOut, BUFSIZE, TEXT("Converting Voice : %S\r\n"),  dispName);
	ConsoleOut(genOut);

//Now convert to the DX9
//set the easy stuff
	SetSingleDefaults(lpOldPatch,dispName);
//Algorithm
	SingleAlgorithmConvert(lpNewPatch, lpOldPatch);
//convert the operators
	SingleOperatorConvert(lpNewPatch, lpOldPatch,lpFMAdditional);
//LFO
	SingleLFOConvert(lpNewPatch, lpOldPatch);

	return;

}

//convert bulk data dump
void ConvertBulkVoices(HANDLE hFile,UCHAR *readBuffer,UCHAR *writeBuffer, int midiChannel)
{
	TCHAR tWork[BUFSIZE];
	TCHAR tPath[BUFSIZE];
	TCHAR tTxtFile[BUFSIZE];
	TCHAR tTxtName[BUFSIZE];
	TCHAR tOutName[BUFSIZE];
	
	
	char workBuffer[BUFSIZE];
	char dispName[16];

	DWORD inBufferPos = 0;
	DWORD outBufferPos = 0;
	int pass, xx, y, byteLen;
	DWORD dwRet;

	lpFM_BULK_NEW_PATCH lpNewPatch;
	FM_BULK_OLD_PATCH oldPatch;

//32 voice bulk dump in 'new' 4op format (DX21/27/100/11)
//go through the file twice, creating 2 DX9 files
//32 patches -> 2x20 patches with 8 patches repeated
	for (pass = 0; pass < 2; pass++)
	{
		dwRet = GetFinalPathNameByHandle(hFile, tWork, BUFSIZE, VOLUME_NAME_DOS);

		//add the prefix
		xx = (int)_tcslen(tWork);
		for (y = xx; y > 0; y--)
		{
			if (tWork[y] == '.')
				tWork[y] = 0;
			if (tWork[y] == '\\')
				break;
		}
		_tcsncpy_s(tPath, tWork, y + 1);

		swprintf(tTxtFile, BUFSIZE, TEXT("DX9_%s_%d"), &tWork[y + 1], pass + 1);
		swprintf(tOutName, BUFSIZE, TEXT("%s%s.syx"), tPath, tTxtFile);
		swprintf(tTxtName, BUFSIZE, TEXT("%s%s.txt"), tPath, tTxtFile);
//create UTF-8 version of name for text file		
		byteLen = WideCharToMultiByte(CP_UTF8, 0, tTxtFile, (int)_tcslen(tTxtFile), workBuffer, BUFSIZE, NULL, NULL);
		textBuffer[0] = 0;
		if (byteLen)
		{
			workBuffer[byteLen] = 0;
			sprintf_s(textBuffer, BUFSIZE, "-------------\r\n  Sysex File : %s.syx\r\n  Patch File : %s.txt\r\n-------------\r\n", workBuffer, workBuffer);
		}

//text buffer containing patch list
		strcat_s(textBuffer, BUFSIZE, "\r\n     Patches\r\n=================\r\n");

		ConsoleOut(L"\r\n");
		inBufferPos = 6 + (FMDX9_SKIP_PATCH_SIZE * pass);

//write out the header
		memcpy(writeBuffer, FMDX9_BULK_HEADER, 6);
		outBufferPos = 6;
//set MIDI channel
		writeBuffer[MIDI_CHANNEL_POS] = (midiChannel - 1);
//'new' (non-DX9) format, so convert
//loop through the patches in the file	
		for (int i = 0; i < 20; i++, inBufferPos += 128, outBufferPos += 128)
		{
//save the patch name into our text buffer
			lpNewPatch = (lpFM_BULK_NEW_PATCH)&readBuffer[inBufferPos];
			getStrName(lpNewPatch, dispName);
			sprintf_s(workBuffer, BUFSIZE, "%02d - %s\r\n", (i + 1), dispName);
			strcat_s(textBuffer, BUFSIZE, workBuffer);
			//convert the patch
			ConvertVoice(lpNewPatch, i, &oldPatch);
			//write the patch out
			memcpy(&writeBuffer[outBufferPos], &oldPatch, sizeof(FM_BULK_OLD_PATCH));
		}
		//fill out the remainder of the file
		memset(&writeBuffer[outBufferPos], DX9FILLER, (sizeof(FM_BULK_OLD_PATCH)*FILLERPATCHES));
		//add the checksum
		writeBuffer[SYSCHKSUM] = CalcChecksum(writeBuffer, SYSDATAPOS, SYSDATABYTES);		
		//add EOF
		writeBuffer[SYSEOF] = SYSEND;
		//write out the file
		swprintf(tWork, BUFSIZE, TEXT("DX9_%s_%d.syx"), &tWork[y + 1], pass + 1);
		ConsoleOut(L"Writing Files :\r\n\t");
		ConsoleOut(tWork);
		ConsoleOut(L"\r\n\t");
		swprintf(tWork, BUFSIZE, TEXT("DX9_%s_%d.txt"), &tWork[y + 1], pass + 1);
		ConsoleOut(tWork);
		ConsoleOut(L"\r\n");
		ConsoleOut(L"\r\n======================================================\r\n");
		WriteDX9BulkFile(tOutName, writeBuffer,SYSLEN);
		WriteDX9PatchNames(tTxtName, textBuffer);
	}
}

//convert an old 'bulk' voice structure to a 'single' voice strucutre
BOOL ConvertBulkToSingle(lpFM_BULK_OLD_PATCH lpBulkPatch,lpFM_SINGLE_OLD_PATCH lpSinglePatch)
{
	//the operators 
	lpFM_SINGLE_OPERATOR_OLD lpSingleOp;
	lpFM_BULK_OPERATOR_OLD lpBulkOp;
	
//easy stuff first, copy across pitch envelope, name and algorithm
	memcpy(&lpSinglePatch->PitchEnvelope, &lpBulkPatch->PitchEnvelope, sizeof(lpSinglePatch->PitchEnvelope));
	memcpy(lpSinglePatch->PatchName, lpBulkPatch->PatchName, FM_PATCH_NAME_SIZE);
//now the algorithm
	lpSinglePatch->Algorithm.Algorithm = lpBulkPatch->Algorithm.Algorithm;
	lpSinglePatch->Algorithm.Feedback = (lpBulkPatch->Algorithm.OscKeySyncFeedback) & 7;
	lpSinglePatch->Algorithm.OscSync = (lpBulkPatch->Algorithm.OscKeySyncFeedback >> 3) & 1;
//LFO
	lpSinglePatch->Lfo.LFOAmpModDepth = lpBulkPatch->Lfo.LFOAmpModDepth;
	lpSinglePatch->Lfo.LFODelay = lpBulkPatch->Lfo.LFODelay;
	lpSinglePatch->Lfo.LFOSpeed = lpBulkPatch->Lfo.LFOSpeed;
	lpSinglePatch->Lfo.LFOPitchModDepth = lpBulkPatch->Lfo.LFOPitchModDepth;
	lpSinglePatch->Lfo.Transpose = lpBulkPatch->Lfo.Transpose;
	lpSinglePatch->Lfo.LFOSync = (lpBulkPatch->Lfo.LFOPModWaveSync) & 1;
	lpSinglePatch->Lfo.WaveForm= (lpBulkPatch->Lfo.LFOPModWaveSync>>1) & 7;
	lpSinglePatch->Lfo.PitchModSens = (lpBulkPatch->Lfo.LFOPModWaveSync >> 4) & 7;

//go through the operators
	for (int i = 0; i < 6; i++)
	{
		lpSingleOp = &lpSinglePatch->FMOp[i];
		lpBulkOp = &lpBulkPatch->FMOp[i];

//Envelope
		memcpy(&lpSingleOp->EG, &lpBulkOp->EG, sizeof(lpBulkOp->EG));
//breakpoint
		lpSingleOp->BreakPoint = lpBulkOp->BreakPoint;
		lpSingleOp->ScaleLeftDepth = lpBulkOp->ScaleLeftDepth;
		lpSingleOp->ScaleRightDepth = lpBulkOp->ScaleRightDepth;
//output level and scale depth
		lpSingleOp->OutputLevel = lpBulkOp->OutputLevel;
		
//scale curves
		lpSingleOp->ScaleLeftCurve = (lpBulkOp->ScaleLRCurve) & 3;
		lpSingleOp->ScaleRightCurve = (lpBulkOp->ScaleLRCurve >> 2) & 3;
//key rate scaling
		lpSingleOp->KeyRateScaling = (lpBulkOp->OscDetuneRateScale) & 7;
//detune
		lpSingleOp->OscDetune = (lpBulkOp->OscDetuneRateScale >> 3) & 15;

//fine frequency
		lpSingleOp->FreqFine = lpBulkOp->FreqFine;
		
		lpSingleOp->AmpModSens = (lpBulkOp->KeyVelSensAModSens) & 3;
		lpSingleOp->KeyVelSens = (lpBulkOp->KeyVelSensAModSens >> 2) & 7;
		lpSingleOp->OscMode = (lpBulkOp->FreqCMode) & 1;
		lpSingleOp->FreqCoarse = (lpBulkOp->FreqCMode >> 1) & 31;
		
	}
	return TRUE;
}

//convert an old 'single' voice structure to a 'bulk' voice strucutre
BOOL ConvertSingleToBulk(lpFM_SINGLE_OLD_PATCH lpSinglePatch, lpFM_BULK_OLD_PATCH lpBulkPatch)
{
	
	//the operators
	lpFM_SINGLE_OPERATOR_OLD lpSingleOp;
	lpFM_BULK_OPERATOR_OLD lpBulkOp;

	//easy stuff first, copy across pitch envelope, name and algorithm
	memcpy(&lpBulkPatch->PitchEnvelope, &lpSinglePatch->PitchEnvelope, sizeof(lpSinglePatch->PitchEnvelope));
	memcpy(lpBulkPatch->PatchName, lpSinglePatch->PatchName, FM_PATCH_NAME_SIZE);
	//now the algorithm
	lpBulkPatch->Algorithm.Algorithm = lpSinglePatch->Algorithm.Algorithm;
//construct osckeysync/feedback field
	lpBulkPatch->Algorithm.OscKeySyncFeedback = ((lpSinglePatch->Algorithm.OscSync & 1) << 3) | (lpSinglePatch->Algorithm.Feedback & 7);


	//LFO
	lpBulkPatch->Lfo.LFOAmpModDepth = lpSinglePatch->Lfo.LFOAmpModDepth;
	lpBulkPatch->Lfo.LFODelay = lpSinglePatch->Lfo.LFODelay;
	lpBulkPatch->Lfo.LFOSpeed = lpSinglePatch->Lfo.LFOSpeed;
	lpBulkPatch->Lfo.LFOPitchModDepth = lpSinglePatch->Lfo.LFOPitchModDepth;
	lpBulkPatch->Lfo.Transpose = lpSinglePatch->Lfo.Transpose;
	
	
	lpBulkPatch->Lfo.LFOPModWaveSync = (lpSinglePatch->Lfo.LFOSync & 1) | ((lpSinglePatch->Lfo.WaveForm & 7) << 1) | ((lpSinglePatch->Lfo.PitchModSens & 7) << 4);


	//go through the operators
	for (int i = 0; i < 6; i++)
	{
		lpSingleOp = &lpSinglePatch->FMOp[i];
		lpBulkOp = &lpBulkPatch->FMOp[i];

		//Envelope
		memcpy(&lpBulkOp->EG, &lpSingleOp->EG, sizeof(lpBulkOp->EG));
		//breakpoint
		lpBulkOp->BreakPoint = lpSingleOp->BreakPoint;
		lpBulkOp->ScaleLeftDepth = lpSingleOp->ScaleLeftDepth;
		lpBulkOp->ScaleRightDepth = lpSingleOp->ScaleRightDepth;
		//output level and scale depth
		lpBulkOp->OutputLevel = lpSingleOp->OutputLevel;
		//fine frequency
		lpBulkOp->FreqFine = lpSingleOp->FreqFine;
		//scale curves
		lpBulkOp->ScaleLRCurve = (lpSingleOp->ScaleLeftCurve & 3) | ((lpSingleOp->ScaleRightCurve & 3) << 2);
		//key rate scaling + detune
		lpBulkOp->OscDetuneRateScale = (lpSingleOp->KeyRateScaling & 7) | ((lpSingleOp->OscDetune & 15) << 3);
		
//key vel, a mod sens
		lpBulkOp->KeyVelSensAModSens = (lpSingleOp->AmpModSens & 3) | ((lpSingleOp->KeyVelSens & 7) << 2);
//osc mode and coarse freq
		lpBulkOp->FreqCMode = (lpSingleOp->OscMode & 1) | ((lpSingleOp->FreqCoarse & 31) << 1);
	}
	return TRUE;
}

void CopyDX9PatchItem(int masterIdx, int patchIdx)
{
	memcpy_s(&DX9OutPatch[patchIdx], sizeof(FM_BULK_OLD_PATCH), &DX9BulkPatch[masterIdx], sizeof(FM_BULK_OLD_PATCH));
}
