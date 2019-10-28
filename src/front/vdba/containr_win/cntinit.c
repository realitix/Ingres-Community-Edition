/****************************************************************************
 * Copyright (C) 2005-2009 Actian Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTINIT.C
 *
 * Entry point and initialization code for the Container control DLL for
 * both Win16 and Win32 environments.
 *
 * Change History:
 *	07-May-2009 (whiro01)
 *	    Don't try to call _CRT_INIT anymore because the system init code
 *	    does all that for us.  This was leftover from Win32s days and
 *	    caused crashes with double free of the module heap.
 *
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>  // Need this for the memory macros
#include <memory.h>
#include <string.h>
#include "cntdll.h"

static BOOL bRegistered=FALSE;

#ifdef WIN16

BOOL NEAR RegisterCntControl( HINSTANCE hInstance );

/****************************************************************************
 * LibMain
 *
 * Purpose:
 *  DLL-specific entry point called from LibEntry.  Initializes the 
 *  DLL's heap and registers the Container custom control class.
 *
 * Parameters:
 *  HINSTANCE     hInstance   - instance of the DLL.
 *  UINT          wDataSeg    - segment selector of the DLL's data segment.
 *  UINT          wHeapSize   - byte count of the heap.
 *  LPSTR         lpCmdLine   - command line used to start the module.
 *
 * Return Value:
 *  int           1 if successful; else 0
 ****************************************************************************/

int WINAPI LibMain( HINSTANCE hInstance, UINT wDataSeg, UINT cbHeapSize, LPSTR lpCmdLine )
    {
    hInstCnt = NULL;

    // Register the container control class.
    if( RegisterCntControl( hInstance ) )
      {
      hInstCnt = hInstance;

      if( 0 != cbHeapSize )
        UnlockData(0);

      return 1;
      }

    return 0;
    }

/****************************************************************************
 * RegisterCntControl
 *
 * Purpose:
 *  Registers the Container control class, including CS_GLOBALCLASS
 *  to make the control available to all applications in the system.
 *
 * Parameters:
 *  HINSTANCE     hInstance   - Instance of the application or DLL that will
 *                              own this class.
 *
 * Return Value:
 *  BOOL          TRUE if the class is registered, FALSE otherwise.
 *                TRUE is also returned if the class was already registered.
 ****************************************************************************/

BOOL NEAR RegisterCntControl( HINSTANCE hInstance )
    {
    WNDCLASS wc;

    if( !bRegistered )
      {
      wc.style         = CS_DBLCLKS | CS_GLOBALCLASS;
      wc.lpfnWndProc   = FrameWndProc;
      wc.cbClsExtra    = LEN_CLASSEXTRA;
      wc.cbWndExtra    = LEN_WINDOWEXTRA;
      wc.hInstance     = hInstance;
      wc.hIcon         = LoadIcon( hInstance, "ContainerIcon" );
      wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
      wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
      wc.lpszMenuName  = NULL;
      wc.lpszClassName = CONTAINER_CLASS;

      if( RegisterClass( &wc ) )
        {
        wc.style         = CS_DBLCLKS | CS_GLOBALCLASS;
        wc.lpfnWndProc   = ContainerWndProc;
        wc.cbClsExtra    = LEN_CLASSEXTRA;
        wc.cbWndExtra    = LEN_WINDOWEXTRA;
        wc.hInstance     = hInstance;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = CNTCHILD_CLASS;

        bRegistered = RegisterClass( &wc );
        }
      }

    return bRegistered;
    }

#else



/****************************************************************************
 * DllMain
 *
 * Purpose:
 *  On DLL_PROCESS_ATTACH it registers the Container Control Class in Win32.
 *
 *  DLL initialization serialization is guaranteed within a process 
 *  (if multiple threads then DLL entry points are serialized), but is 
 *  not guaranteed across processes.
 *
 *  When synchronization objects are created, it is necesaary to check
 *  the return code of GetLastError even if the create call succeeded. 
 *  If the object existed, ERROR_ALREADY_EXISTED will be returned.
 *
 * Parameters:
 *  HANDLE        hDLL        - DLL module handle
 *  DWORD         dwReason    - reason being called (e.g. process attaching)
 *  LPVOID        lpReserved  - reserved
 *
 * Return Value:
 *  BOOL          TRUE if initialization passed; FALSE otherwise.
 ****************************************************************************/

BOOL WINAPI DllMain( HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved )
    {
    WNDCLASS wc;

    hInstCnt = hDLL;

    switch( dwReason )
      {
      case DLL_PROCESS_ATTACH:
        // We only need this bRegister flag for Win32s. In WinNT and Win95
        // the DLL gets its own data space for each process that attaches
        // to the DLL. Therefore bRegister will always be FALSE when any
        // any process attaches. Win32s, however, has only one data instance
        // per DLL (just like Win16) and will share data between processes.
        // NOTE: Under Win32s, if the same DLL is loaded from different
        //       directories by different apps they will NOT share data.
        // Furthermore, under Win32s RegisterClass will fail if the class 
        // has already been registered by another process (just like Win16). 
        // Also, in Win32s we must make sure we do not unregister the class 
        // until the last process has detached.
        if( !bRegistered )
          {
          wc.style         = CS_DBLCLKS | CS_GLOBALCLASS;
          wc.lpfnWndProc   = FrameWndProc;
          wc.cbClsExtra    = LEN_CLASSEXTRA;
          wc.cbWndExtra    = LEN_WINDOWEXTRA;
          wc.hInstance     = hDLL;
          wc.hIcon         = LoadIcon( hDLL, "ContainerIcon" );
          wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
          wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
          wc.lpszMenuName  = NULL;
          wc.lpszClassName = CONTAINER_CLASS;
          
          if( RegisterClass( &wc ) )
            {
            wc.style         = CS_DBLCLKS | CS_GLOBALCLASS;
            wc.lpfnWndProc   = ContainerWndProc;
            wc.cbClsExtra    = LEN_CLASSEXTRA;
            wc.cbWndExtra    = LEN_WINDOWEXTRA;
            wc.hInstance     = hDLL;
            wc.hIcon         = NULL;
            wc.hCursor       = NULL;
            wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
            wc.lpszMenuName  = NULL;
            wc.lpszClassName = CNTCHILD_CLASS;
          
            if( RegisterClass( &wc ) )
              bRegistered++;
            }

          if( !bRegistered )
            {
            char szText[LEN_ERRMSG], szTitle[LEN_ERRMSG];

            LoadString( hDLL, IDS_REGCLASS_ERR, szText, LEN_ERRMSG );
            LoadString( hDLL, IDS_DLL_ERR, szTitle, LEN_ERRMSG );
            MessageBox( NULL, szText, szTitle, MB_OK | MB_ICONEXCLAMATION );

            return FALSE;
            }
          }
        else
          {
          // Keep track of how many processes attach so we know when to
          // unregister the class. NOTE: This is only for Win32s support.
          bRegistered++;
          }
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        // Do anything required for detaching here.

        // In Win32s we must make sure we do not unregister the
        // class until the last process has detached.
        bRegistered--;
        if( !bRegistered )
          {
          UnregisterClass( CONTAINER_CLASS, hDLL );
          UnregisterClass( CNTCHILD_CLASS,  hDLL );
          }

        break;

      case DLL_THREAD_DETACH:
        // Do anything required for detaching here.

        break;
      }

    return TRUE;
    }

#endif  /* WIN16 */
