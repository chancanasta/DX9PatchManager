#ifndef __DX9CONVERTER_H
#define __DX9CONVERTER_H

BOOL AllocateBuffers(VOID);
VOID FreeBuffers(VOID);


int DX9Convert(TCHAR *inFile, HWND hWnd, HWND hFileList);
BOOL SaveDX9PatchFile(HWND hDlg, HWND hPatchSet,BOOL saveAs);


#endif
