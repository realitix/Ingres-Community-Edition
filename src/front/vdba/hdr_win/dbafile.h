/********************************************************************
//
//  Copyright (C) 2005-2011 Actian Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : dbafile.h
//    save/load environment
//
//    13-Dec-2010 (drivi01)
//      Port the solution to x64. Clean up the warnings.
//      Clean up datatypes. Port function calls to x64.
********************************************************************/

typedef FILE * FILEIDENT;

#define ARCHIVEBUFSIZE       4096
#define ENDOFFILEEMPTYCHARS (ARCHIVEBUFSIZE + 1024)

void InitLLFile4Save();
void InitLLFile4Read();
int  LLDBATerminateFile   (FILEIDENT fident);
int  LLDBAAppend4Save     (FILEIDENT fident, void *buf, UINT cb);
int  LLDBAAppend4SaveNC   (FILEIDENT fident, void *buf, UINT cb);
int  LLDBAAppendChars4Save(FILEIDENT fident, char c   , UINT cb);
int  LLDBAReadData        (FILEIDENT fident, void *bufresu, size_t cb, size_t *pActualCb);
long LLDBASeekRel         (FILEIDENT fident,long cb);
BOOL LLDBAisCmprsActive   (void);

int DBAFileOpen4Save    (LPUCHAR lpfilename, FILEIDENT * fident);
int DBAAppend4Save      (FILEIDENT fident, void *buf, UINT cb);
int DBAAppend4SaveNC    (FILEIDENT fident, void *buf, UINT cb);
int DBAAppendChars4Save (FILEIDENT fident, char c, UINT cb);

int DBAFileOpen4Read(LPUCHAR lpfilename, FILEIDENT * fident);
int DBAReadData     (FILEIDENT fident, void *bufresu, UINT cb);

int DBACloseFile    (FILEIDENT fident);


int DOMSaveDisp     (FILEIDENT fident);
int DOMLoadDisp     (FILEIDENT fident);
int DOMSaveCache    (FILEIDENT fident);
int DOMLoadCache    (FILEIDENT fident);
int DOMSaveSettings (FILEIDENT fident);
int DOMLoadSettings (FILEIDENT fident);
int PropsSaveCache  (FILEIDENT fident);
int PropsLoadCache  (FILEIDENT fident);

// function in dom.c, put here since uses FILEIDENT
HWND MakeNewDomMDIChild(INT_PTR domCreateMode, HWND hwndMdiRef, FILEIDENT idFile, DWORD_PTR dwData);


