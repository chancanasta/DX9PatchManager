#ifndef __SENDPATCH_H
#define __SENDPATCH_H

BOOL SendPatches(HWND hDlg, HWND hList, lpFM_BULK_OLD_PATCH lpPatchList);
BOOL SendPatchSet(HWND hList, DWORD noItems, DWORD *pItems, lpFM_BULK_OLD_PATCH lpPatchList);

void RenamePatch(HWND hDlg,HWND hList);
BOOL PreProcessName(HWND hList, int selItem, TCHAR *pBuffer);
void SetNewName(HWND hDlg);

//MIDI
void PopulateMIDIList(HWND hMIDIList);
BOOL sendMIDI(BYTE *midiData, int noBytes);
void PlayNotes(int midiChannel);


//dialog
INT_PTR CALLBACK DialogRename(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif