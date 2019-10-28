/******************************************************************************
**
**      Copyright (c) 2011 Actian Corporation
**
******************************************************************************/

#include        <compat.h>

/*
**  Name:   Dll_Entry - DLL Entry module
**
**  This function should be declared as the DLL entry (in the LINK
**  command as /ENTRY:Dll_Entry).
**
**  This function will be invoked by the OS when a process
**  loads/unloads the DLL or a new thread is created/destroyed
**  (and the DLL is already loaded).
**
**  Parameters:
**
**       hModule            - Handle to the DLL module.
**
**       ul_reason_for_call -
**              DLL_PROCESS_ATTACH - DLL being loaded into virtual memory.
**              DLL_PROCESS_DETACH - DLL being unloaded from virtuall memory.
**              DLL_THREAD_ATTACH  - Current process is creating a new thread.
**              DLL_THREAD_DETACH  - Thread is exiting cleanly.
**
**       lpReserved         -
**              [DLL_PROCESS_ATTACH] - NULL for dynamic loads (i.e. LoadLibrary)
**                                     non-NULL for static loads
**              [DLL_PROCESS_DETACH] - NULL if unload via FreeLibrary
**                                     non-NULL if process terminating.
**
**  Returns:
**       TRUE  - If succeeds
**       FALSE - Will cause the DLL to be unloaded.
**
**  History:
**       07-Jan-2011 (horda03) b124907
**          Created
*/
extern void PC_doexitfuncs();

GLOBALDEF i4 clf_dll_module = 0;

BOOL WINAPI
Dll_Entry( HMODULE hModule,
           DWORD  ul_reason_for_call,
           LPVOID lpReserved )
{
   /* This is the DLL initialisation routine. The routine is invoked when the
   ** DLL is loaded. If the DLL is loaded statically (i.e. as part of the
   ** application being executed) lpReserved with have a value. A lpReserved of
   ** 0 indicates that the DLL has been loaded by the application using
   ** LoadLibrary.
   **
   ** Tried making the use of atexit() if the DLL was statically loaded, but
   ** in testing "sql iidbdb" the error seen for this bug occured.
   */
   if (ul_reason_for_call == DLL_PROCESS_ATTACH)
   {
      /* This is the DLL load, so indicate such */
      clf_dll_module = 1;
   }
   else if (ul_reason_for_call == DLL_PROCESS_DETACH)
   {
      /* The DLL is being unloaded, so call all the exit functions. */

      PC_doexitfuncs();
   }

   return TRUE;
}
