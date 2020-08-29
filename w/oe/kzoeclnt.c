//
// MODULE NAME:  KZOECLNT  -  Object services Test program
// DESCRIPTION:  This is a Windows program which initializes the
//               Object services DLL and creates the list window
//               for all Object services tracing.
// ************************************************************************
// * Copyright © 1993 - 2016 QuinSoft, Inc. All rights reserved.          *
// * Confidential and Proprietary material subject to license -           *
// * do not reproduce or disclose. This material is an unpublished        *
// * work and is considered a trade secret belonging to the               *
// * copyright holder.                                                    *
// ************************************************************************
//  AUTHOR:  Gig Kirk
//  DATE:    3/31/92
//  API:     Windows 3.x
//  ENVIRONMENT: DOS 5.0
//

#include <windows.h>
#include <stdio.h>
#include <sys\stat.h>

#define  KZSYSSVC_INCL
#include <kzoengaa.h>   // Object Services
#include <zdecimal.h>   // Object Services

#define NUM_THREADS 1
// int zPASCAL WinMain( HANDLE, HANDLE, LPSTR, int );
HWND       InitInstance( HANDLE hInstance, int  nCmdShow, LPSTR lpCmdLine );
BOOL       InitApplication( HANDLE hInstance );

long zPASCAL
MainWndProc( HWND, unsigned, WPARAM, LPARAM );
long zPASCAL
AboutDlgProc( HWND, unsigned, WPARAM, LPARAM );
long zPASCAL
LogonDlgProc( HWND, unsigned, WPARAM, LPARAM );
void
fnProcessOENG_Message( HWND, unsigned, WPARAM, LPARAM );

#define TRACE( n, sz ) fnTrace( n, sz )
//#define TRACE( sz ) SendMessage( hListWnd, LB_ADDSTRING, 0, (zLONG) sz )

HANDLE hInst;                       // current instance
HWND   hListWnd;
HWND   hMainWnd;
char   szClassName[ ] = "KZOECLNTClass";
WORD   wClientMessage;              // allocate message
int    nNoUnregister;
zVIEW  vSubtask;
zBOOL  g_bRunningTest = FALSE;
zULONG g_lTestCount = 0;

#include <process.h>

zBOOL  g_bContinueThreadTest = 0;
zBOOL  g_bTestThreadStarted = FALSE;
zULONG g_hTestThread = 0;
zULONG g_dwTestThreadID = 0;

#define DECLARE_TLS __declspec( thread )
DECLARE_TLS zBOOL bThreadRegistered = FALSE;

void
fnTrace( zSHORT nThreadNumber, char *pchMsg )
{
   WORD wItemCount;
   char szMsg[ 2000 ];

   sprintf_s( szMsg, zsizeof( szMsg ), "%d: %s", nThreadNumber, pchMsg );
   wItemCount = (WORD) SendMessage( hListWnd, LB_ADDSTRING, 0, (zLONG) szMsg );
   if ( wItemCount > 4000 )
   {
      SendMessage( hListWnd, LB_RESETCONTENT, 0, 0 );
      wItemCount = 0;
   }

   SendMessage( hListWnd, LB_SETCURSEL, wItemCount, 0L );
}

unsigned __stdcall
fnThreadProc2( zPVOID p )
{
   zVIEW  vSubtask;
   zLONG  nCount = 0;
   zVIEW  vView;
   char   szMsg[ 200 ];
   zSHORT nRC;
   zSHORT nThreadNumber = (zSHORT) p;

   TRACE( nThreadNumber, "Starting thread test" );

   while ( g_bContinueThreadTest )
   {
      zCHAR szDateTime[ 30 ];

      if ( !bThreadRegistered )
      {
         if ( RegisterZeidonApplication( &vSubtask,
                                         (zLONG) hInst,
                                         (zLONG) hMainWnd,
                                         (zLONG) wClientMessage,
                                         "\\\\Zencas\\", 0, 0 ) == zCALL_ERROR )
         {
            g_dwTestThreadID = 0;
            return( 0 );
         }

         bThreadRegistered = TRUE;
         TRACE( nThreadNumber, "Completed thread register" );
      }

   // if ( nThreadNumber == 2 )
   //    Sleep( 500 );
      Sleep( 100 * ((nThreadNumber % 5) + 1) );

   // SysMutexLock( "KZOECLNT/1", 0, 0 );
   // TRACE( nThreadNumber, "Mutex locked" );

      SysGetDateTime( szDateTime, zsizeof( szDateTime ) );
      TRACE( nThreadNumber, szDateTime );

      nRC = ActivateObjectInstance( &vView, "mUser", vSubtask, 0, zSINGLE );
      if ( nRC >= 0 )
         DropView( vView );
      else
         TRACE( nThreadNumber, "----- ERROR activating mUser --------- " );

      sprintf_s( szMsg, zsizeof( szMsg ), "Thread test #%d, rc = %d", nCount++, nRC );
      TRACE( nThreadNumber, szMsg );

   // SysMutexUnlock( "KZOECLNT/1" );
   // TRACE( nThreadNumber, "Mutex unlocked" );

      // Unregister every third time or so.
      if ( nCount % 3 == 0 )
      {
         UnregisterZeidonApplication( vSubtask );
         vSubtask = 0;
         bThreadRegistered = FALSE;
         TRACE( nThreadNumber, "Completed thread UN(!)register" );
      }
   }

   if ( bThreadRegistered )
   {
      UnregisterZeidonApplication( vSubtask );
      vSubtask = 0;
      bThreadRegistered = FALSE;
      TRACE( nThreadNumber, "Completed thread UN(!)register" );
   }

   g_dwTestThreadID = 0;
   TRACE( nThreadNumber, "Completed thread test" );

   return( 0 );
}

/////////////////////////////////////////////////////////////////////////////
//
//  ENTRY:     WinMain
//  PURPOSE:   Establish message queue and create object
//             services window
//
/////////////////////////////////////////////////////////////////////////////

int zPASCAL
WinMain( HINSTANCE hInstance,     // current instance
         HINSTANCE hPrevInstance, // previous instance
         LPSTR lpCmdLine,         // command line
         int  nCmdShow )          // show-window type (open/icon)
{
   MSG msg;                       // message
   HWND hWnd;                     // Main window handle

   if (!hPrevInstance)                  // Other instances of app running?
   {
      if (!InitApplication( hInstance )) // Initialize shared things
         return( FALSE );               // Exits if unable to initialize
   }

   // Perform initializations that apply to a specific instance

   if ( !(hWnd = InitInstance( hInstance, nCmdShow, lpCmdLine )) )
      return( FALSE );

   // Set nNoUnregister to 0
   nNoUnregister = 0;

   // Acquire and dispatch messages until a WM_QUIT message is received.

   while ( GetMessage( &msg,    // message structure
                       0,       // handle of window receiving the message
                       0,       // lowest message to examine
                       0 ) )    // highest message to examine
   {
      TranslateMessage( &msg ); // Translate message
      DispatchMessage( &msg );  // Dispatches message to window
   }

   return( msg.wParam );        // Returns the value from PostQuitMessage
}

VOID CALLBACK
TimerProc( HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
   zVIEW  vView;
   zVIEW  vLocalSubtask;
   zCHAR  szMsg[ 100 ];

   if ( g_lTestCount % 50 == 0 )
      SendMessage( hListWnd, LB_RESETCONTENT, 0, 0 );

   SfCreateSubtask( &vLocalSubtask, vSubtask, "Zencas" );
   if ( vSubtask )
   {
      zCHAR szDateTime[ 30 ];

      SysGetDateTime( szDateTime, zsizeof( szDateTime ) );
      TRACE( 0, szDateTime );

      ActivateObjectInstance( &vView, "mMedCode", vSubtask, 0, zSINGLE );
      DropView( vView );
      SfDropSubtask( vSubtask, 0 );
   }


   sprintf_s( szMsg, zsizeof( szMsg ), "Done with test %ld", g_lTestCount++ );
   TRACE( 0, szMsg );

   if ( !g_bRunningTest )
      KillTimer( hWnd, idEvent );
}

/////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: MainWndProc(HWND, unsigned, WPARAM, LPARAM )
//
//  PURPOSE:  Processes messages for Object Services
//
//  MESSAGES:
//
//  COMMENTS:
//
/////////////////////////////////////////////////////////////////////////////
long zPASCAL
MainWndProc( HWND hWnd,                // window handle
             unsigned message,         // type of message
             WPARAM wParam,            // additional information
             LPARAM lParam )           // additional information
{
   switch ( message )
   {
      case WM_CLOSE:
         // Check to see if the network is up.
         if ( g_bRunningTest )
         {
            SysMessageBox( 0, "Test In Progress", "Close running test", -1 );
            return( 0 );
         }

         if ( vSubtask && NetStatus( vSubtask, "zWinSock" ) != 0 )
            NetClose( vSubtask, "zWinSock" );

         // It's OK to leave, Shutdown Object Services
         if ( nNoUnregister != 99 && vSubtask )
            UnregisterZeidonApplication( vSubtask );

         return( DefWindowProc( hWnd, message, wParam, lParam ));


      case WM_DESTROY:        // message: window being destroyed
         PostQuitMessage( 0 );
         break;

      case WM_COMMAND:
         if ( wParam == 110 )
         {
            SysMessageBox( 0, "?", "Where yourself", -1 );
         }
         else
         if ( wParam == 111 )
         {
            RegisterZeidonApplication( &vSubtask,
                                       (zLONG) hInst,
                                       (zLONG) hMainWnd,
                                       (zLONG) wClientMessage,
                                       "\\\\Zencas\\", 0, 0 );
         }
         else
         if ( wParam == 112 )
         {
            UnregisterZeidonApplication( vSubtask );
            vSubtask = 0;
         }
         else
         if ( wParam == 113 )
         {
            nNoUnregister = 99;
            PostMessage( hWnd, WM_CLOSE, 0, 0L );
         }
         else
         if ( wParam == 114 )
         {
            PostMessage( hWnd, WM_CLOSE, 0, 0L );
         }
         else
         if ( wParam == 115 )  // ID_TEST
         {
               int i;

               /* Display 10 numbers. */
               for( i = 0;   i < 10;i++ )
                  TraceLineI( "  Random =", SysGetRandomLong() );

             zVIEW vLOD_Meta;
             zCHAR szName[300] = "c:\\10d\\a\\bin\\sys\\TZTEUPDD.PWD";
             zCHAR szTempFile[300] = "c:\\temp\\meta.LOD";
             int nRC = ActivateOI_FromFile( &vLOD_Meta, "TZWDLGSO", vSubtask, szName, zACTIVATE_ROOTONLY );
             nRC = CommitOI_ToFile( vLOD_Meta, szTempFile, zINCREMENTAL );
             nRC = ActivateOI_FromFile( &vLOD_Meta, "TZWDLGSO", vSubtask, szTempFile, zSINGLE );
             nRC = CommitOI_ToFile( vLOD_Meta, "c:\\temp\\meta-original.LOD", zUSE_OLD_LINK_CARDS );
         }

         break;

      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
         if ( wParam == VK_F3 )
            PostMessage( hWnd, WM_CLOSE, 0, 0L );

         break;

    default:
      if ( message == wClientMessage )
         ProcessZeidonMessage( wParam, lParam );

      break;
   }

   return( DefWindowProc( hWnd, message, wParam, lParam ));
}

/////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:  InitApplication( HANDLE hInatsnce )
//
//  PURPOSE:  Saves instance handle and creates main window
//
/////////////////////////////////////////////////////////////////////////////
BOOL
InitApplication( HANDLE hInstance )     // current instance
{
   WNDCLASS  wc;

   // Fill in window class structure with parameters that describe the
   // main window.

   wc.style = 0;                       // Class style(s).
   wc.lpfnWndProc = MainWndProc;       // Function to retrieve messages for
                                       // windows of this class.
   wc.cbClsExtra = 0;                  // No per-class extra data.
   wc.cbWndExtra = 0;                  // No per-window extra data.
   wc.hInstance = hInstance;           // Application that owns the class.
   wc.hIcon = LoadIcon( hInstance, "KZOECLNT" );
   wc.hCursor = LoadCursor( 0, IDC_ARROW );
   wc.hbrBackground = GetStockObject( WHITE_BRUSH );
   wc.lpszMenuName =  0;               // Name of menu resource in .RC file.
   wc.lpszClassName = szClassName;     // Name used in call to CreateWindow.

   // Register the window class and return success/failure code.
   return( RegisterClass( &wc ) );
}

/////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:  InitInstance( HANDLE, int, LPSTR )
//
//  PURPOSE:  Saves instance handle and creates main window
//
//  COMMENTS:
//
//      This function is called at initialization time for every instance of
//      this application.  This function performs initialization tasks that
//      cannot be shared by multiple instances.
//
//      In this case, we save the instance handle in a static variable and
//      create and display the main program window.
//
/////////////////////////////////////////////////////////////////////////////
HWND
InitInstance( HANDLE hInstance,     // Current instance identifier.
              int  nCmdShow,        // Param for first ShowWindow( ) call
              LPSTR lpCmdLine )     // Command line
{
   int            nHeight, nPosY;
   int            nWidth, nPosX;
   RECT           rect;

   // Save the instance handle in static variable, which will be used in
   // many subsequence calls from this application to Windows.

   hInst = hInstance;

   // Get the dimensions of the screen
   nWidth = GetSystemMetrics( SM_CXSCREEN );
   nHeight = GetSystemMetrics( SM_CYSCREEN );
   nWidth /= 2;
   nHeight /= 2;
   nWidth -= 40;
   nHeight -= 20;
   nPosX = nWidth / 2;
   nPosY = nHeight / 2;
   nPosX -= 8; // So it doesn't occupy same space as object services
   nPosY -= 8;

   // Create a main window for this application instance.
   hMainWnd = CreateWindow( szClassName,                    // See RegisterClass( ) call.
                            "Test Client",                  // Text for window title bar.
                            WS_OVERLAPPEDWINDOW,            // Window style.
                            nPosX,                          // Default horizontal position.
                            nPosY,                          // Default vertical position.
                            nWidth,                         // Default width.
                            nHeight,                        // Default height.
                            0,                              // Overlapped windows have no parent.
                            LoadMenu( hInstance, "KZOECLNT" ), // Menu for trace window
                         // 0,                              // Use default menu
                            hInstance,                      // This instance owns this window.
                            0 );                            // Pointer not needed.

   // If window could not be created, return "failure"

   if ( !hMainWnd )
      return( FALSE );

   GetClientRect( hMainWnd, &rect );
   hListWnd = CreateWindow( "ListBox",                      // See RegisterClass( ) call.
                            "Trace Window",                 // Text for window title bar.
                            WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL |
                            WS_THICKFRAME | WS_CAPTION,
                            rect.left,                      // Default horizontal position.
                            rect.top,                       // Default vertical position.
                            (rect.right - rect.left),       // Default width.
                            (rect.bottom - rect.top),       // Default height.
                            hMainWnd,                       // Parent Window
                            0,                              // Use the window class menu.
                            hInstance,                      // This instance owns this window.
                            0 );                            // Pointer not needed.

   SendMessage( hListWnd, WM_SETFONT,
                (WPARAM) GetStockObject( SYSTEM_FIXED_FONT ), 0 );

   wClientMessage = RegisterWindowMessage( "KZOECLNT" );

   if ( RegisterZeidonApplication( &vSubtask, (zLONG) hInst,
                                   (zLONG) hMainWnd,
                                   (zLONG) wClientMessage,
                                   "\\\\Zeidon_Tools", 0, 0 ) != 0 )
                                   //lpCmdLine, 0, 0 ) != 0 )
   {
      DestroyWindow( hMainWnd );
      return( FALSE );
   }

   ShowWindow( hMainWnd, SW_SHOW );  // Show the window
   {
      zPCHAR psz;
      zLONG  h = SysAllocMemory( &psz, 100, 0, zCOREMEM_ALLOC, 0 );
      HANDLE ph = GetProcessHeap( );
      LPVOID pv;
      LPVOID pv2;
      DWORD  dw;

      pv = HeapAlloc( ph, 0, 100 );
      dw = GetLastError( );
      pv2 = HeapAlloc( ph, 0, 100 );
      dw = GetLastError( );

      HeapFree( ph, 0, pv );
      dw = GetLastError( );
      HeapFree( ph, 0, pv2 );
      dw = GetLastError( );

      strcpy_s( psz, 100, "laskjdflkasjdflkjasdlfkjasdlkfj" );
      SysFreeMemory( h );
   }

   TRACE( 0, "Done registering" );

   // Make the window visible; update its client area; and return "success"
   return( hMainWnd );            // Returns the value from PostQuitMessage
}
