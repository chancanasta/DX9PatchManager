//where most of the file processing happens
#include "stdafx.h"
#include <mmeapi.h>
#include "DX9PatchManager.h"
#include "FileConstants.h"
#include "FMStructs.h"
#include "SendPatch.h"
#include "FMConversion.h"
#include "ProcessFile.h"
#include "SettingValues.h"
#include "Extern.h"


TCHAR WorkingPatchName[SMALLBUFFER];
TCHAR SaveName[SMALLBUFFER];
TCHAR SavePrefix[SMALLBUFFER];
UCHAR OutBuffer[BUFSIZE];
HWND theFileList;
int theIndex;
//Midi
HMIDIOUT    midiHandle = NULL;

static UCHAR RefMinNote = 3*12;
static UCHAR RefMaxNote = 6*12;
static UCHAR RefNoteStep = 12;

static UCHAR RefNote = RefMinNote;

static UCHAR MIDINoteON = 0x90;
static UCHAR MIDINoteOFF = 0x80;

int NoMIDIDevices;

void CALLBACK MIDIOutCallBack(HMIDIOUT hmo, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	PMIDIHDR     pMidiHdr;
	MMRESULT mmRet;
	switch (wMsg)
	{
	case MOM_CLOSE:
#ifdef _DX9_DEBUG		
		ConsoleOut(L"Close\r\n");
#endif
		break;
	case MOM_DONE:
#ifdef _DX9_DEBUG
		ConsoleOut(L"Stream completed\r\n");		
#endif
		if (midiHandle)
		{
#ifdef _DX9_DEBUG
			ConsoleOut(L"Unprepare header\r\n");
#endif
			pMidiHdr = (PMIDIHDR)dwParam1;
			mmRet = midiOutUnprepareHeader(midiHandle, pMidiHdr, sizeof(MIDIHDR));
			if (mmRet == MMSYSERR_NOERROR)
			{
#ifdef _DX9_DEBUG
				ConsoleOut(L"Header successfully unprepared\r\n");
#endif
			}
			else
			{
				//failed to unprepare header MIDI output device
				ConsoleOut(L"Failed to unprepare header ");
				switch (mmRet)
				{
				case MIDIERR_STILLPLAYING:
					ConsoleOut(L"The buffer pointed to by lpMidiOutHdr is still in the queue.\r\n");
					break;
				case MMSYSERR_INVALHANDLE:
					ConsoleOut(L"The specified device handle is invalid.\r\n");
					break;
				case MMSYSERR_INVALPARAM:
					ConsoleOut(L"The specified pointer or structure is invalid.\r\n");
					break;
				default:
					wsprintf(outBuffer, L"Error %d\r\n", mmRet);
					break;
				}
			}
			//close the device
#ifdef _DX9_DEBUG
			ConsoleOut(L"Close device\r\n");
#endif
			midiOutClose(midiHandle);
			midiHandle = NULL;
		}
		break;
	case MOM_OPEN:
#ifdef _DX9_DEBUG
		ConsoleOut(L"Open\r\n");
#endif
		break;
	}
}

void PopulateMIDIList(HWND hMIDIList)
{
	int i;
	MIDIOUTCAPS midiOutCaps;
	int addIdx;

//get the number of MIDI devices
	NoMIDIDevices = midiOutGetNumDevs();
#ifdef _DX9_DEBUG
	wsprintf(outBuffer, L"Midi Devices : %d\r\n", NoMIDIDevices);
	ConsoleOut(outBuffer);
#endif
	for (i = 0; i < NoMIDIDevices; i++)
	{
		midiOutGetDevCaps(i, &midiOutCaps, sizeof(MIDIOUTCAPS));
#ifdef _DX9_DEBUG
		wsprintf(outBuffer, L"Device %d , %s\r\n", i, midiOutCaps.szPname);
		ConsoleOut(outBuffer);
#endif
		addIdx = (int)SendMessage(hMIDIList, CB_ADDSTRING, 0, (LPARAM)midiOutCaps.szPname);
		//add the data item (index)
		if (addIdx != LB_ERR)
			SendMessage(hMIDIList, CB_SETITEMDATA, addIdx, i);		
	}
}

BOOL sendMIDI(BYTE *midiData,int noBytes)
{
	UINT noDevs;
	UINT err;
	MIDIOUTCAPS midiOutCaps;
	MMRESULT mmRet;
	TCHAR workBuff[256];
	MIDIHDR     midiHdr;
	BOOL bRet = FALSE;
/*
initial test...
	BYTE sysEx[] = {
		0xF0,	//Sysex
		0x43,	//Yamaha ID
		0x10,	//sub status (1) and channel (1 = 0)
		0x00,  //group + top 2 bits of param
		0x00,	//botom 6 bits of param (op 6, EG rate 1)
		0x20,	//value		
		0xF7 };
*/
	noDevs = midiOutGetNumDevs();
#ifdef _DX9_DEBUG
	wsprintf(outBuffer, L"Midi Devices : %d\r\n", noDevs);
	ConsoleOut(outBuffer);
#endif
	if (selMIDIDevice >=0 && selMIDIDevice < (int)noDevs)
	{
		midiOutGetDevCaps(selMIDIDevice , &midiOutCaps, sizeof(MIDIOUTCAPS));
		wsprintf(outBuffer, L"Device %d , %s\r\n", selMIDIDevice, midiOutCaps.szPname);
//Check that this is an actual physical MIDI out port
		if(midiOutCaps.wTechnology !=MOD_MIDIPORT)		
		{
			ConsoleOut(L"*** Selected MIDI out device is not a hardware port, cannot send MIDI data to DX9\r\n");
			MessageBox(NULL, TEXT("Selected MIDI Out device is not a hardware port, cannot send data to DX9"), TEXT("MIDI Port"),
				MB_ICONERROR | MB_OK);			
		}
		else
		{
			mmRet = midiOutOpen(&midiHandle, selMIDIDevice, (DWORD_PTR)&MIDIOutCallBack, 0, CALLBACK_FUNCTION);
			if (mmRet == MMSYSERR_NOERROR)
			{
#ifdef _DX9_DEBUG
				wsprintf(outBuffer, L"Opening Device %s\r\n", midiOutCaps.szPname);
				ConsoleOut(outBuffer);
#endif
				midiHdr.lpData = (LPSTR)midiData;
				midiHdr.dwFlags = 0;
				midiHdr.dwBufferLength = noBytes;
				midiHdr.dwBufferLength = midiHdr.dwBufferLength;

				err = midiOutPrepareHeader(midiHandle, &midiHdr, sizeof(MIDIHDR));
				if (err)
				{
					midiOutGetErrorText(err, workBuff, 256);
					wsprintf(outBuffer, L"Failed to prepare header %d , %s\r\n", selMIDIDevice, err, workBuff);
					ConsoleOut(outBuffer);					
				}
				else
				{
					//header ok
					err = midiOutLongMsg(midiHandle, &midiHdr, sizeof(MIDIHDR));
					if (err)
					{
						midiOutGetErrorText(err, workBuff, 256);
						wsprintf(outBuffer, L"Failed to Send MIDI Sysex %d , %s\r\n", selMIDIDevice, err, workBuff);
						ConsoleOut(outBuffer);
						//close the device
#ifdef _DX9_DEBUG
						ConsoleOut(L"Close device\r\n");
#endif
						midiOutClose(midiHandle);
						midiHandle = NULL;
					}
					else
//blimey - actually managed to send the MIDI bytes
						bRet = TRUE;
				}
			}
			else
			{
				//failed to open MIDI output device
				switch (mmRet)
				{
				case MIDIERR_NODEVICE:
					ConsoleOut(L"No MIDI port was found.\r\n");
					break;
				case MMSYSERR_ALLOCATED:
					ConsoleOut(L"The specified resource is already allocated.\r\n");
					break;
				case MMSYSERR_BADDEVICEID:
					ConsoleOut(L"The specified device identifier is out of range.\r\n");
					break;
				case MMSYSERR_INVALPARAM:
					ConsoleOut(L"The specified pointer or structure is invalid.\r\n");
					break;
				case MMSYSERR_NOMEM:
					ConsoleOut(L"The system is unable to allocate or lock memory.\r\n");
					break;
				default:
					wsprintf(outBuffer, L"Error opening out device %d\r\n", mmRet);
					break;
				}				
			}
		}
	}
	else
	{
		wsprintf(outBuffer, L"**Selected Midi Device (%d) does not exist\r\n", selMIDIDevice);
		ConsoleOut(outBuffer);
	}

	return bRet;
}


//send patch (or patches) from a given list
BOOL SendPatches(HWND hDlg, HWND hList, lpFM_BULK_OLD_PATCH lpPatchList)
{
	DWORD fileItems;
	DWORD selItems[BUFSIZE];
	//get the array of selected items
	fileItems = (DWORD)SendMessage(hList, LB_GETSELITEMS, BUFSIZE, (LPARAM)selItems);
	if (fileItems > 0)
	{

//check there aren't more than 20 patches
		if (fileItems > 20)
		{
			MessageBox(hDlg, TEXT("More than 20 Patches Selected, unable to send this many patches to DX9 "), TEXT("Too Many Patches"),MB_ICONEXCLAMATION | MB_OK);
			return FALSE;
		}
//for multiple patches, check before we send them as this will wipe all patches		
		if (fileItems==1 || !ConfirmSend || MessageBox(hDlg, TEXT("Send Patches to DX9 (will replace all current sounds) ?"), TEXT("Multiple Patches Selected"),
			MB_ICONQUESTION | MB_YESNO) == IDYES)
		{
			SendPatchSet(hList, fileItems, (DWORD *)selItems, lpPatchList);		
			return TRUE;
		}
	}
//unlikely we got here if true, but no selected items
	return FALSE;
	
}


//construct the buffer then send the patches
BOOL SendPatchSet(HWND hList, DWORD noItems, DWORD *pItems, lpFM_BULK_OLD_PATCH lpPatchList)
{
	int noBytes;
	UCHAR writeBuffer[SYSLEN];
		
	noBytes=fillPatchesBuffer(hList, SendMIDIChannel,writeBuffer,noItems,pItems,lpPatchList);
	if (noBytes != -1)
	{
#ifdef _DX9_DEBUG
		wsprintf(outBuffer, L"Sending %d bytes to MIDI\r\n", noBytes);
		ConsoleOut(outBuffer);
		for (int i = 0; i < noBytes; i++)
		{
			wsprintf(outBuffer, L"%.2X ", writeBuffer[i]);
			ConsoleOut(outBuffer);
		}
		ConsoleOut(L"\r\n");
#endif
		sendMIDI(writeBuffer, noBytes);
		return TRUE;
	}
	return FALSE;
}

void RenamePatch(HWND hDlg,HWND hList)
{
	int fileItems;
	int selItems[BUFSIZE];	
	theFileList = hList;
	//get the array of selected items
	fileItems = (DWORD)SendMessage(hList, LB_GETSELITEMS, BUFSIZE, (LPARAM)selItems);
//there should be 1 selected item in the list
	if (fileItems == 1)
	{
		theIndex = selItems[0];
//get hold of the name
		if (PreProcessName(hList, theIndex, WorkingPatchName))
			DialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hDlg, (DLGPROC)DialogRename);
		else
			MessageBox(hDlg, L"Unable to edit patch name", L"Invalid Name", MB_ICONERROR | MB_OK);
	}
	return;
}

INT_PTR CALLBACK DialogRename(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD wmID = LOWORD(wParam);
	DWORD wmEvent = HIWORD(wParam);	

	switch (uMsg)
	{
		case WM_INITDIALOG:
			//set the text input			
			SetDlgItemText(hDlg, IDC_PATCHNAME, WorkingPatchName);
			break;
		case WM_CLOSE:
			EndDialog(hDlg, wParam);
			return TRUE;
		case WM_COMMAND:
			switch (wmID)
			{
				case ID_RENAME_CANCEL:
					EndDialog(hDlg, wParam);
					break;
				case ID_RENAME_OK:
					SetNewName(hDlg);
					EndDialog(hDlg, wParam);
					break;
			}
	}

	return FALSE;

}

BOOL PreProcessName(HWND hList,int selItem,TCHAR *pBuffer)
{
	BOOL bRet = FALSE;
	TCHAR WorkName[BUFSIZE];
//get the item name	
	SendMessage(hList, LB_GETTEXT, selItem, (LPARAM)WorkName);
//split based on the ] token
	TCHAR delim[] = L"]";
	TCHAR *context;
	TCHAR *token;

	token=wcstok_s(WorkName, delim,&context);
	if (token)
	{
		wsprintf(SavePrefix, L"%s]", token);
		token = wcstok_s(NULL, delim, &context);
		if (token)
		{
			token++;
			wcscpy_s(pBuffer, SMALLBUFFER, token);
			
			bRet = TRUE;
		}
	}
	return bRet;

}

//update the name of the item
void SetNewName(HWND hDlg)
{
	TCHAR newName[SMALLBUFFER];
	TCHAR workName[SMALLBUFFER];
	char workBuffer[SMALLBUFFER];
	int byteLen, baseLen;

//get the entered text
	GetDlgItemText(hDlg, IDC_PATCHNAME, workName, FM_PATCH_NAME_SIZE);

	baseLen=(int)_tcslen(workName);
//construct the new name
	wsprintf(newName, L"%s %s", SavePrefix, workName);

//delete the existing item	
	SendMessage(theFileList, LB_DELETESTRING, theIndex, 0);
//add the updated item back at the same index
	SendMessage(theFileList, LB_INSERTSTRING, theIndex, (LPARAM)newName);
//select it
	SendMessage(theFileList, LB_SELITEMRANGE, TRUE, MAKELPARAM(theIndex, theIndex));
//update the name in the data structure
//simplest approach - zero out the existing name in the structure
	memset(DX9OutPatch[theIndex].PatchName, 0, FM_PATCH_NAME_SIZE);
//convert the Win32/64 string back to 8bit
	byteLen = WideCharToMultiByte(CP_UTF8, 0, workName, baseLen, workBuffer, SMALLBUFFER, NULL, NULL);
//copy across
	memcpy_s(DX9OutPatch[theIndex].PatchName, FM_PATCH_NAME_SIZE, workBuffer, byteLen);
}

void PlayNotes(int midiChannel)
{
	int noBytes;
	int workMIDI;
	UCHAR midiBuffer[SMALLBUFFER];

	workMIDI = midiChannel - 1;
//note on values
	midiBuffer[0] = MIDINoteON | workMIDI;
	midiBuffer[1] = RefNote;		//Note Value
	midiBuffer[2] = 100;	//Velocity;
	noBytes = 3;
//send note on	
	if (sendMIDI(midiBuffer, noBytes))
	{
		//if that worked, wait
		Sleep(300);
		//then send note off	
		midiBuffer[0] = MIDINoteOFF | workMIDI;
		midiBuffer[2] = 0;
		sendMIDI(midiBuffer, noBytes);
		//then go to next note
		RefNote += RefNoteStep;
		if (RefNote > RefMaxNote)
			RefNote = RefMinNote;
	}
	return;
}
