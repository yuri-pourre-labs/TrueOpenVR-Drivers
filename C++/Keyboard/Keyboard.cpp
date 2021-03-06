#include "stdafx.h"
#include <windows.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

typedef struct _HMDData
{
	double	X;
	double	Y;
	double	Z;
	double	Yaw;
	double	Pitch;
	double	Roll;
} THMD, *PHMD;

typedef struct _Controller
{
	double	X;
	double	Y;
	double	Z;
	double	Yaw;
	double	Pitch;
	double	Roll;
	unsigned short	Buttons;
	float	Trigger;
	float	AxisX;
	float	AxisY;
} TController, *PController;

#define TOVR_SUCCESS 0
#define TOVR_FAILURE 1

#define GRIP_BTN	0x0001
#define THUMB_BTN	0x0002
#define A_BTN		0x0004
#define B_BTN		0x0008
#define MENU_BTN	0x0010
#define SYS_BTN		0x0020

#define StepPos 0.0033;
#define StepRot 0.4;
double HMDPos[3], HMDPitch;

double CtrlPos[3], CtrlYaw, CtrlRoll;

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *HMD)
{
	if ((GetAsyncKeyState(VK_NUMPAD8) & 0x8000) != 0) HMDPos[2] -= StepPos;
	if ((GetAsyncKeyState(VK_NUMPAD2) & 0x8000) != 0) HMDPos[2] += StepPos;

	if ((GetAsyncKeyState(VK_NUMPAD4) & 0x8000) != 0) HMDPos[0] -= StepPos;
	if ((GetAsyncKeyState(VK_NUMPAD6) & 0x8000) != 0) HMDPos[0] += StepPos;

	if ((GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0) HMDPos[1] += StepPos;
	if ((GetAsyncKeyState(VK_NEXT) & 0x8000) != 0) HMDPos[1] -=  StepPos;

	//Yaw fixing
	if ((GetAsyncKeyState(VK_NUMPAD1) & 0x8000) != 0) HMDPitch += StepRot;
	if ((GetAsyncKeyState(VK_NUMPAD3) & 0x8000) != 0) HMDPitch -= StepRot;

	HMD->X = HMDPos[0];
	HMD->Y = HMDPos[1];
	HMD->Z = HMDPos[2];

	HMD->Yaw = 0;
	HMD->Pitch = HMDPitch;
	HMD->Roll = 0;

	return TOVR_SUCCESS;
}

#define StepCtrlPos 0.005;

DLLEXPORT DWORD __stdcall GetControllersData(__out TController *FirstController, __out TController *SecondController)
{
	if ((GetAsyncKeyState(87) & 0x8000) != 0) CtrlPos[2] -= StepCtrlPos; //W
	if ((GetAsyncKeyState(83) & 0x8000) != 0) CtrlPos[2] += StepCtrlPos; //S

	if ((GetAsyncKeyState(65) & 0x8000) != 0) CtrlPos[0] -= StepCtrlPos; //A
	if ((GetAsyncKeyState(68) & 0x8000) != 0) CtrlPos[0] += StepCtrlPos; //D

	if ((GetAsyncKeyState(81) & 0x8000) != 0) CtrlPos[1] += StepCtrlPos; //Q
	if ((GetAsyncKeyState(69) & 0x8000) != 0) CtrlPos[1] -= StepCtrlPos; //E

	if ((GetAsyncKeyState(85) & 0x8000) != 0) CtrlRoll += StepRot; //U
	if ((GetAsyncKeyState(74) & 0x8000) != 0) CtrlRoll -= StepRot; //J

	if ((GetAsyncKeyState(72) & 0x8000) != 0) CtrlYaw += StepRot; //H
	if ((GetAsyncKeyState(75) & 0x8000) != 0) CtrlYaw -= StepRot; //K

	//Controller 1
	FirstController->X = CtrlPos[0] - 0.2;
	FirstController->Y = CtrlPos[1];
	FirstController->Z = CtrlPos[2] - 0.5;

	FirstController->Yaw = CtrlYaw;
	FirstController->Pitch = 0;
	FirstController->Roll = CtrlRoll;

	FirstController->Buttons = 0;
	if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) FirstController->Buttons += GRIP_BTN;
	if ((GetAsyncKeyState(49) & 0x8000) != 0) FirstController->Buttons += SYS_BTN; //1
	if ((GetAsyncKeyState(50) & 0x8000) != 0) FirstController->Buttons += THUMB_BTN; //2
	if ((GetAsyncKeyState(51) & 0x8000) != 0) FirstController->Buttons += MENU_BTN; //3
	if ((GetAsyncKeyState(52) & 0x8000) != 0) FirstController->Buttons += A_BTN; //4
	if ((GetAsyncKeyState(53) & 0x8000) != 0) FirstController->Buttons += B_BTN; //5

	FirstController->Trigger = 0;
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) FirstController->Trigger = 1;

	FirstController->AxisX = 0;
	FirstController->AxisY = 0;

	//Controller 2
	SecondController->X = CtrlPos[0] + 0.2;
	SecondController->Y = CtrlPos[1];
	SecondController->Z = CtrlPos[2] - 0.5;

	SecondController->Yaw = CtrlYaw;
	SecondController->Pitch = 0;
	SecondController->Roll = CtrlRoll;

	SecondController->Buttons = 0;
	if ((GetAsyncKeyState(90) & 0x8000) != 0) SecondController->Buttons += GRIP_BTN; //z
	if ((GetAsyncKeyState(54) & 0x8000) != 0) SecondController->Buttons += SYS_BTN; //6
	if ((GetAsyncKeyState(55) & 0x8000) != 0) SecondController->Buttons += THUMB_BTN; //7
	if ((GetAsyncKeyState(56) & 0x8000) != 0) SecondController->Buttons += MENU_BTN; //8
	if ((GetAsyncKeyState(57) & 0x8000) != 0) SecondController->Buttons += A_BTN; //9
	if ((GetAsyncKeyState(48) & 0x8000) != 0) SecondController->Buttons += B_BTN; //0

	SecondController->Trigger = 0;
	if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) SecondController->Trigger = 1;
	SecondController->AxisX = 0;
	SecondController->AxisY = 0;

	return TOVR_SUCCESS;
}

DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in unsigned char MotorSpeed)
{
	return TOVR_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	/*	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
	break;
	}*/

	return TRUE;
}