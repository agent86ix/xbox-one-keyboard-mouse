#include "stdafx.h"
#include "atlstr.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <tchar.h>
#include "resource.h"
#include <interception.h>

#include "AsyncSerial.h"

#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")

#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "interception.lib")

HWND hDlg;
CallbackAsyncSerial *serial;

bool keepRunning;
bool mouseMoveWatchdog;
InterceptionContext context;
InterceptionDevice device;

typedef enum {
	BUTTON_INVALID=-1,
	BUTTON_A=0,
	BUTTON_B=1,
	BUTTON_X=2,
	BUTTON_Y=3,
	BUTTON_LB=4,
	BUTTON_RB=5,
	BUTTON_L3=6,
	BUTTON_R3=7,
	BUTTON_BACK=8,
	BUTTON_START=9,
	BUTTON_LS_X=10,
	BUTTON_LS_Y=11,
	BUTTON_RS_X=12,
	BUTTON_RS_Y=13,
	BUTTON_T_L=14,
	BUTTON_T_R=15,
} buttonAddress;


void sendControllerCommand(buttonAddress address, int flags, int value)
{
	unsigned char serialData[2];
	serialData[0] = address << 3 | (flags & 0x7);
	serialData[1] = (unsigned char)value;
	serial->write((const char*)serialData, 2);
}

void resetController()
{
	unsigned char serialData[2];
	serialData[0] = 0xff;
	serialData[1] = 0xff;
	serial->write((const char*)serialData, 2);
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	unsigned char serialData[2];
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTONW:
			sendControllerCommand(BUTTON_LS_Y, 0, 0);
			return TRUE;
		case IDC_BUTTONA:
			sendControllerCommand(BUTTON_LS_X, 0, 0);
			return TRUE;
		case IDC_BUTTONS:
			sendControllerCommand(BUTTON_LS_Y, 0, 255);
			return TRUE;
		case IDC_BUTTOND:
			sendControllerCommand(BUTTON_LS_X, 0, 255);
			return TRUE;
		case IDC_BUTTONLS:
			sendControllerCommand(BUTTON_T_L, 0, 0);
			return TRUE;
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		if (MessageBox(hDlg, TEXT("Close the program?"), TEXT("Close"),
			MB_ICONQUESTION | MB_YESNO) == IDYES)
		{
			DestroyWindow(hDlg);
		}
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

void AppendText(const HWND &hwnd, TCHAR *newText)
{
	// get edit control from dialog
	HWND hwndOutput = GetDlgItem(hwnd, IDC_OUTPUT);

	// get the current selection
	DWORD StartPos, EndPos;
	SendMessage(hwndOutput, EM_GETSEL, reinterpret_cast<WPARAM>(&StartPos), reinterpret_cast<WPARAM>(&EndPos));

	// move the caret to the end of the text
	int outLength = GetWindowTextLength(hwndOutput);
	SendMessage(hwndOutput, EM_SETSEL, outLength, outLength);

	// insert the text at the new caret position
	SendMessage(hwndOutput, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(newText));

	// restore the previous selection
	SendMessage(hwndOutput, EM_SETSEL, StartPos, EndPos);
}

void serialReadCallback(const char *data, unsigned int len)
{
	CA2T dataT(data);
	CA2T lineEnd("\r\n");
	dataT[len] = 0;
	AppendText(hDlg, dataT);

	AppendText(hDlg, lineEnd);
}

DWORD WINAPI interceptionThread(LPVOID lpParam) 
{
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionStroke stroke;
	TCHAR debugStr[256];
	bool blockStrokes = false;
	bool strokeUsed = false;
	

	context = interception_create_context();

	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);
	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL);

	while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
	{
		if (!keepRunning) break;
		if (interception_is_mouse(device))
		{
			InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *)&stroke;
			
			
			// mstroke.state is or'ed INTERCEPTION_MOUSE_BUTTON_#_UP, _DOWN
			// mstroke.flags can be INTERCEPTION_MOUSE_MOVE_RELATIVE or _MOVE_ABSOLUTE
			// mstroke.x and mstroke.y are x/y values that depend on RELATIVE or ABSOLUTE
			if (mstroke.flags == INTERCEPTION_MOUSE_MOVE_RELATIVE)
			{
				_stprintf_s(debugStr, (size_t)256, TEXT("%d"), mstroke.x);
				SetDlgItemText(hDlg, IDC_X_MOUSE, debugStr);
				_stprintf_s(debugStr, (size_t)256, TEXT("%d"), mstroke.y);
				SetDlgItemText(hDlg, IDC_Y_MOUSE, debugStr);


				int x_map[7] = { 0, 32, 48, 64, 80, 96, 127 };
				int y_map[7] = { 0, 64, 96, 127, 127, 127, 127 };

				int x, y;
				int sign_y, sign_x;
				sign_y = sign_x = 1;

				/* Get sign */
				if (mstroke.x < 0) sign_x = -1;
				if (mstroke.y < 0) sign_y = -1;

				/* Get magnitude */
				x = abs(mstroke.x);
				y = abs(mstroke.y);

				/* translate magnitude */
				if (x > 6) x = 127;
				else x = x_map[x];

				if (y > 6) y = 127;
				else y = y_map[y];

				/* Compute output */
				x = sign_x*x + 127;
				y = sign_y*y + 127;

				/* clamp */
				if (x > 255) x = 255;
				if (x < 0) x = 0;
				if (y > 255) y = 255;
				if (y < 0) y = 0;

				mouseMoveWatchdog = true;
				sendControllerCommand(BUTTON_RS_X, 0, x);
				sendControllerCommand(BUTTON_RS_Y, 0, y);
			}

			if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_1_DOWN)
			{
				sendControllerCommand(BUTTON_T_R,0,0);
			}
			else if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_1_UP)
			{
				sendControllerCommand(BUTTON_T_R, 0, 255);
			}

			if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_2_DOWN)
			{
				sendControllerCommand(BUTTON_T_L, 0, 0);
			}
			else if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_2_UP)
			{
				sendControllerCommand(BUTTON_T_L, 0, 255);
			}

			if (mstroke.state)
			{
				_stprintf_s(debugStr, (size_t)256, TEXT("state: 0x%x\n"), mstroke.state);
				AppendText(hDlg, debugStr);
			}

			//_stprintf_s(debugStr, (size_t)256, TEXT("x: %d y: %d flag: %d\n"), mstroke.x, mstroke.y, mstroke.flags);
			//AppendText(hDlg, debugStr);
			if(!blockStrokes) interception_send(context, device, &stroke, 1);
		}

		if (interception_is_keyboard(device))
		{
			InterceptionKeyStroke &kstroke = *(InterceptionKeyStroke *)&stroke;
			strokeUsed = true;
			// kstroke.state can be INTERCEPTION_KEY_UP or INTERCEPTION_KEY_DOWN
			// kstroke.code is the raw keycode from the device
			int val = 0;
			buttonAddress addr = BUTTON_INVALID;
			if (kstroke.state == INTERCEPTION_KEY_DOWN) val = 1;
			
			switch (kstroke.code) {
				case 0x44: // f10
					if(kstroke.state == INTERCEPTION_KEY_DOWN) blockStrokes = !blockStrokes; 
					break;
				case 0x11: // w
					addr = BUTTON_LS_Y;
					val = kstroke.state == INTERCEPTION_KEY_DOWN ? 0 : 127;
					break;
				case 0x1f: // s
					addr = BUTTON_LS_Y;
					val = kstroke.state == INTERCEPTION_KEY_DOWN ? 255 : 127;
					break;
				case 0x1e: // a
					addr = BUTTON_LS_X;
					val = kstroke.state == INTERCEPTION_KEY_DOWN ? 0 : 127;
					break;
				case 0x20: // d
					addr = BUTTON_LS_X;
					val = kstroke.state == INTERCEPTION_KEY_DOWN ? 255 : 127;
					break;
				case 0x10: // q -> y
					addr = BUTTON_Y;
					break;
				case 0x13: // r -> x
					addr = BUTTON_X;
					break;
				case 0x39: // space -> a
					addr = BUTTON_A;
					break;
				case 0x1D: // lctrl -> b
					addr = BUTTON_B;
					break;
				case 0x2a: // lshift -> L3
					addr = BUTTON_L3;
					break;
				case 0x29: // tilde -> start
					addr = BUTTON_START;
					break;
				case 0xf: // tab -> back
					addr = BUTTON_BACK;
					break;
				case 0x22: // g -> LB
					addr = BUTTON_LB;
					break;
				case 0x12: // e -> rB
					addr = BUTTON_RB;
					break;
				default:
					strokeUsed = false;
					break;

			}
			
			if (addr != BUTTON_INVALID)
			{
				sendControllerCommand(addr, 0, val);
				if (!blockStrokes) interception_send(context, device, &stroke, 1);
			}
			else 
			{
				_stprintf_s(debugStr, (size_t)256, TEXT("keycode: 0x%x\n"), kstroke.code);
				AppendText(hDlg, debugStr);
				interception_send(context, device, &stroke, 1);
			}
			
		}
		
		
	}

	interception_destroy_context(context);

	return 0;
}

DWORD WINAPI mouseMoveWatchdogThread(LPVOID lpParam)
{
	bool controllerCentered = true;
	int timeoutCount = 0;

	while (keepRunning)
	{
		Sleep(50);

		if (!mouseMoveWatchdog) 
		{
			timeoutCount++;
			/* No mouse interrupt in the last sleep period */
			if ((timeoutCount > 2) && !controllerCentered)
			{
				/* Center the controller */
				sendControllerCommand(BUTTON_RS_X, 0, 127);
				sendControllerCommand(BUTTON_RS_Y, 0, 127);
				controllerCentered = true;
			}			
		}
		else
		{
			timeoutCount = 0;
			controllerCentered = false;
		}
		mouseMoveWatchdog = false;
	}
	return 0;
}


int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow)
{
	
	MSG msg;
	BOOL ret;
	HANDLE interceptionThreadHndl, watchdogThreadHndl;

	/* Init window */
	InitCommonControls();
	hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);
	ShowWindow(hDlg, nCmdShow);

	keepRunning = true;

	
	
	/* Init serial */
	serial = new CallbackAsyncSerial("COM3", 115200);
	serial->setCallback(boost::bind(serialReadCallback, _1, _2));
	//serial->writeString(" ");

	resetController();

	/* Init kb/mouse interception */
	interceptionThreadHndl = CreateThread(NULL, 0, interceptionThread, NULL, 0, NULL);
	watchdogThreadHndl = CreateThread(NULL, 0, mouseMoveWatchdogThread, NULL, 0, NULL);

	/* Main loop */
	while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
		if (ret == -1)
			return -1;

		if (!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}