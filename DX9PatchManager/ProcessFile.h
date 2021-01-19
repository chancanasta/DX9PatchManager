#ifndef __PROCESSFILE_H
#define __PROCESSFILE_H

//fairly lazy, the data structures are small, so just define some oversized arrays
#define MAX_DX9_BULK_PATCHES	1024
#define MAX_DX9_WORK_PATCHES	20

//main processing
BOOL ProcessFile(HANDLE hFile, PHANDLE phInputBuffer, PHANDLE phOutputBuffer, PHANDLE phNameBuffer, int midiChannel, TCHAR *fileName, HWND hFileList);
//format of data dump
WORD GetDumpFormat(PUCHAR readBuffer);
//initial validation checks
BOOL ValidDXSysEx(PUCHAR readBuffer, DWORD  dwBytesRead);
//convert patch
BOOL ConvertVoice(lpFM_BULK_NEW_PATCH lpNewPatch, int PatchNo, lpFM_BULK_OLD_PATCH lpOldPatch);


//Which calls these...
//convert operator
void OperatorConvert(lpFM_BULK_NEW_PATCH lpNewPatch, lpFM_BULK_OLD_PATCH lpOldPatch);
void SingleOperatorConvert(lpFM_SINGLE_NEW_PATCH lpNewPatch, lpFM_SINGLE_OLD_PATCH lpOldPatch, lpFM_SINGLE_ADD_PARAMS lpFMAdditional = NULL);

//convert from 'new' envelope to 'old' one
void EGConvert(lpFM_EG_NEW lpEgNew, lpFM_EG_OLD lpEgOld);
//convert the frequency params
void FreqConvert(lpFM_BULK_OPERATOR_NEW lpOperatorNew, lpFM_BULK_OPERATOR_OLD lpOperatorOld, lpFM_BULK_ADD_OPERATOR lpAddOperator);
void SingleFreqConvert(lpFM_SINGLE_OPERATOR_NEW lpOperatorNew, lpFM_SINGLE_OPERATOR_OLD lpOperatorOld, lpFM_SINGLE_ADD_OP lpFMAddOperator = NULL);

//convert the alogrithm params
void AlgorithmConvert(lpFM_BULK_NEW_PATCH lpNewPatch, lpFM_BULK_OLD_PATCH lpOldPatch);
//convert the LFO params
void LFOConvert(lpFM_BULK_NEW_PATCH lpNewPatch, lpFM_BULK_OLD_PATCH lpOldPatch);
void SingleLFOConvert(lpFM_SINGLE_NEW_PATCH lpNewPatch, lpFM_SINGLE_OLD_PATCH lpOldPatch);
//default out all the easy stuff in the patch
void SetDefaults(lpFM_BULK_OLD_PATCH lpOldPatch, int PatchNo);
void SetSingleDefaults(lpFM_SINGLE_OLD_PATCH lpOldPatch,char *dispName);
//the checksum
UCHAR CalcChecksum(UCHAR *writeBuffer, int startPos, int dataLen);
//calculate the coarse, fine and detune frequencies for the DX9 new style operator
void CalcFreqDetune(UCHAR freqIn, UCHAR fineIn, UCHAR detuneIn, UCHAR *pCoarseOut, UCHAR *pFineOut, UCHAR *pDetuneOut);



//write out the buffers for the 2 files (sysex and text file of patch names)
BOOL WriteDX9BulkFile(LPCWSTR fileName, UCHAR *outBuffer, int outLen);
BOOL WriteDX9PatchNames(LPCWSTR fileName, LPCSTR outBuffer);

//convert bulk voices
void ConvertBulkVoices(HANDLE hFile, UCHAR *readBuffer, UCHAR *writeBuffer,int midiChannel);
void ConvertSingleVoice(UCHAR *readBuffer, lpFM_SINGLE_OLD_PATCH lpOldPatch, int midiChannel, lpFM_SINGLE_ADD_PARAMS lpFMAdditional=NULL);
void ConvertAdditionalVoice(UCHAR *readBuffer, lpFM_SINGLE_OLD_PATCH lpOldPatch, int midiChannel, DWORD dwBytesRead);

//handle the patch names
void getPatchName(char *outName, char *patchName);
void getStrName(lpFM_BULK_NEW_PATCH lpNewPatch, char *dispName);
void getStrSingleName(lpFM_SINGLE_NEW_PATCH lpNewPatch, char *dispName);

void AddBulkVoices(HANDLE hFile, UCHAR *readBuffer, HWND hFileList, BOOL newPatch=FALSE);
BOOL isInitSinglePatch(lpFM_SINGLE_OLD_PATCH lpOldSinglePatch);
BOOL isInitPatch(int patchNo, lpFM_BULK_OLD_PATCH lpOldPatch, int lastIdx);
BOOL getTextPatchName(int index, char *txtWork, DWORD dwBytesRead, char *patchName);

void AddSingleVoice(HANDLE hFile, UCHAR *readBuffer, HWND hFileList, BOOL newPatch = FALSE, BOOL additional = FALSE,DWORD dwBytesRead=0);
void AddSingleVoiceStruct(HANDLE hFile, lpFM_SINGLE_OLD_PATCH lpOldSinglePatch, HWND hFileList, char *workBuffer);

BOOL ProcessDX9Patches(HWND hPatchSet, UCHAR *pWriteBuffer, int midiChannel, TCHAR *pFileName);
//split out so we can do MIDI sends as well as file saves
int fillPatchesBuffer(HWND hPatchSet, int midiChannel, UCHAR *pWriteBuffer, DWORD noItems, DWORD *pItems, lpFM_BULK_OLD_PATCH lpPatches);
void fillNamesBuffer(HWND hPatchSet);

void GetPatchNameFile(TCHAR *tIn, TCHAR *tOut);
BOOL ConvertBulkToSingle(lpFM_BULK_OLD_PATCH lpBulkPatch, lpFM_SINGLE_OLD_PATCH lpSinglePatch);
BOOL ConvertSingleToBulk(lpFM_SINGLE_OLD_PATCH lpSinglePatch,lpFM_BULK_OLD_PATCH lpBulkPatch);

void CopyDX9PatchItem(int masterIdx, int patchIdx);

#endif

