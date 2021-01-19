#ifndef __EXTERN_H
#define __EXTERN_H

extern	HINSTANCE hInst;
extern	TCHAR outBuffer[BUFSIZE];
extern	FM_BULK_OLD_PATCH	DX9OutPatch[MAX_DX9_WORK_PATCHES];
extern	TCHAR outBuffer[BUFSIZE];

extern BOOL FileNotSaved;

extern	FM_BULK_OLD_PATCH	DX9BulkPatch[MAX_DX9_BULK_PATCHES];
extern	FM_BULK_OLD_PATCH	DX9OutPatch[MAX_DX9_WORK_PATCHES];

extern int selMIDIDevice;


#endif

