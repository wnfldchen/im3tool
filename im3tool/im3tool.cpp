// im3tool.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <string>
#include "im3tool.h"
#include "FileOpenDialog.h"
#include "BitmapFile.h"
#include "Painter.h"
#include "Codec.h"
#include "IM3File.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_IM3TOOL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IM3TOOL));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	static const COLORREF BLACK = RGB(0, 0, 0);
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IM3TOOL));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(BLACK);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_IM3TOOL);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static const WCHAR IM3_FILE_EXTENSION[] = L".im3";
	static BitmapFile* bitmapFile = NULL;
	static IM3File* im3File = NULL;
	static Codec codec;
	static std::wstring openedFileName;
    switch (message) {
	case WM_CREATE: {
		bitmapFile = FileOpenDialog::OpenBitmapFile(hWnd);
		if (bitmapFile) {
			openedFileName = FileOpenDialog::getFileName();
			im3File = codec.compress(bitmapFile);
			std::wstring saveFileName = openedFileName.append(IM3_FILE_EXTENSION);
			HANDLE saveFileHandle = FileOpenDialog::OverwriteFileFromName(saveFileName);
			im3File->Save(saveFileHandle);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	}
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
		case IDM_ABOUT: {
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		}
		case ID_FILE_OPEN: {
			SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			if (bitmapFile) {
				delete bitmapFile;
				bitmapFile = NULL;
			}
			if (im3File) {
				delete im3File;
				im3File = NULL;
			}
			bitmapFile = FileOpenDialog::OpenBitmapFile(hWnd);
			if (bitmapFile) {
				openedFileName = FileOpenDialog::getFileName();
				im3File = codec.compress(bitmapFile);
				std::wstring saveFileName = openedFileName.append(IM3_FILE_EXTENSION);
				HANDLE saveFileHandle = FileOpenDialog::OverwriteFileFromName(saveFileName);
				im3File->Save(saveFileHandle);
			}
			SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case IDM_EXIT: {
			DestroyWindow(hWnd);
			break;
		}
		default: {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
        }
		break;
    }
    case WM_PAINT: {
		if (bitmapFile) {
			Painter painter(hWnd, bitmapFile);
		} else {
			DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
    }
	case WM_DESTROY: {
		if (bitmapFile) {
			delete bitmapFile;
			bitmapFile = NULL;
		}
		if (im3File) {
			delete im3File;
			im3File = NULL;
		}
		PostQuitMessage(0);
		break;
	}
	default: {
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
