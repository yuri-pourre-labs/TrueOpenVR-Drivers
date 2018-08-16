#include "stdafx.h"
#include <thread> 
#include <winsock2.h>
#pragma comment (lib, "WSock32.Lib")
#include <vector>
#include <math.h>
#include <sstream>
#pragma warning(disable:4996) 

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
	WORD	Buttons;
	BYTE	Trigger;
	SHORT	ThumbX;
	SHORT	ThumbY;
} TController, *PController;

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *myHMD);
DLLEXPORT DWORD __stdcall GetControllersData(__out TController *MyController, __out TController *MyController2);
DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in WORD	MotorSpeed);
DLLEXPORT DWORD __stdcall SetCentering(__in int dwIndex);

struct TControllerPacket {
	BYTE Index;
	double Yaw;
	double Pitch;
	double Roll;
	double JoystickX;
	double JoystickY;
	WORD Buttons;
	BYTE Trigger;
};

SOCKET socketS, socketS2;
int bytes_read;
struct sockaddr_in from;
int fromlen;
bool SocketsActivated = false, ControllersInit = true;
bool bKeepReading = false;

double Ctrl1Offset[3], Ctrl2Offset[3];
TControllerPacket Ctrl1Packet, Ctrl2Packet;

std::thread *pCtrlsReadThread = NULL;

char *sp;
char DataBuff[1024];
std::string item;
std::vector<std::string> parsed;

double MyOffset(double f, double f2) {
	return fmod(f - f2, 180);
}

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *myHMD)
{
	myHMD->X = 0;
	myHMD->Y = 0;
	myHMD->Z = 0;
	myHMD->Yaw = 0;
	myHMD->Pitch = 0;
	myHMD->Roll = 0;

	return 0;
}

void ReadControllersData() {
	memset(&Ctrl1Packet, 0, sizeof(Ctrl1Packet));
	memset(&Ctrl2Packet, 0, sizeof(Ctrl2Packet));
	memset(&DataBuff, 0, sizeof(DataBuff));
	memset(&parsed, 0, sizeof(parsed));

	while (SocketsActivated) {

			//Controller 1
			bKeepReading = true;
			while (bKeepReading) {

				bytes_read = recvfrom(socketS, DataBuff, sizeof(DataBuff), 0, (sockaddr*)&from, &fromlen);

				if (bytes_read > 0) {

					sp = strtok(DataBuff, ";");
					while (sp) {
						item = std::string(sp);
						parsed.push_back(item);
						sp = strtok(NULL, ";");
					}

					Ctrl1Packet.Yaw = std::stof(parsed[5]);
					Ctrl1Packet.Pitch = std::stof(parsed[4]) * -1;
					Ctrl1Packet.Roll = std::stof(parsed[3]) * -1;

					Ctrl1Packet.Buttons = 0;
					Ctrl1Packet.Trigger = 0;

					if (parsed[14] == "1") Ctrl1Packet.Buttons = 1;
					if (parsed[14] == "2") Ctrl1Packet.Buttons = 8;
					if (parsed[14] == "4") Ctrl1Packet.Buttons = 4;
					if (parsed[14] == "8") Ctrl1Packet.Buttons = 2;
					if (parsed[14] == "16") Ctrl1Packet.Trigger = 255;

					Ctrl1Packet.JoystickX = 0;
					Ctrl1Packet.JoystickY = 0;
					Ctrl1Packet.JoystickX = std::stof(parsed[17]);
					Ctrl1Packet.JoystickY = std::stof(parsed[18]) * -1;

					if (parsed[0] == "L#TMST") {
						Ctrl1Packet.Index = 1; 
					} else { 
						Ctrl1Packet.Index = 2; 
					}

					if (parsed[12] == "1")
						SetCentering(1);

					parsed.clear();
				}
				else {
					bKeepReading = false;
				}
			}

			//Controller 2
			bKeepReading = true;
			while (bKeepReading) {

				bytes_read = recvfrom(socketS2, DataBuff, sizeof(DataBuff), 0, (sockaddr*)&from, &fromlen);

				if (bytes_read > 0) {

					sp = strtok(DataBuff, ";");
					while (sp) {
						item = std::string(sp);
						parsed.push_back(item);
						sp = strtok(NULL, ";");
					}

					Ctrl2Packet.Yaw = std::stof(parsed[5]);
					Ctrl2Packet.Pitch = std::stof(parsed[4]) * -1;
					Ctrl2Packet.Roll = std::stof(parsed[3]) * -1;

					Ctrl2Packet.Buttons = 0;
					Ctrl2Packet.Trigger = 0;

					if (parsed[14] == "1") Ctrl2Packet.Buttons = 1;
					if (parsed[14] == "2") Ctrl2Packet.Buttons = 8;
					if (parsed[14] == "4") Ctrl2Packet.Buttons = 4;
					if (parsed[14] == "8") Ctrl2Packet.Buttons = 2;
					if (parsed[14] == "16") Ctrl2Packet.Trigger = 255;

					Ctrl2Packet.JoystickX = 0;
					Ctrl2Packet.JoystickY = 0;
					Ctrl2Packet.JoystickX = std::stof(parsed[17]);
					Ctrl2Packet.JoystickY = std::stof(parsed[18]) * -1;

					if (parsed[0] == "L#TMST") {
						Ctrl2Packet.Index = 1;
					}
					else {
						Ctrl2Packet.Index = 2;
					}

					if (parsed[12] == "1")
						SetCentering(2);

					parsed.clear();
				}
				else {
					bKeepReading = false;
				}
			}
	}
}

void ControllersStart() {
	Ctrl1Offset[0] = 0;
	Ctrl1Offset[1] = 0;
	Ctrl1Offset[2] = 0;

	Ctrl2Offset[0] = 0;
	Ctrl2Offset[1] = 0;
	Ctrl2Offset[2] = 0;

	//Controller 1
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult == 0) {
		struct sockaddr_in local;
		fromlen = sizeof(from);
		local.sin_family = AF_INET;
		local.sin_port = htons(5555);
		local.sin_addr.s_addr = INADDR_ANY;

		socketS = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		u_long nonblocking_enabled = TRUE;
		ioctlsocket(socketS, FIONBIO, &nonblocking_enabled);

		if (socketS != INVALID_SOCKET) {

			iResult = bind(socketS, (sockaddr*)&local, sizeof(local));

			if (iResult != SOCKET_ERROR) {
				SocketsActivated = true;
				//
			}
			else {
				WSACleanup();
				SocketsActivated = false;
			}

		}
		else {
			WSACleanup();
			SocketsActivated = false;
		}
	}
	else
	{
		WSACleanup();
		SocketsActivated = false;
	}

	//Controller 2
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult == 0) {
		struct sockaddr_in local;
		fromlen = sizeof(from);
		local.sin_family = AF_INET;
		local.sin_port = htons(5556);
		local.sin_addr.s_addr = INADDR_ANY;

		socketS2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		u_long nonblocking_enabled = TRUE;
		ioctlsocket(socketS2, FIONBIO, &nonblocking_enabled);

		if (socketS2 != INVALID_SOCKET) {

			iResult = bind(socketS2, (sockaddr*)&local, sizeof(local));

			if (iResult != SOCKET_ERROR) {
				SocketsActivated = true;
				//
			}
			else {
				WSACleanup();
				SocketsActivated = false;
			}

		}
		else {
			WSACleanup();
			SocketsActivated = false;
		}
	}
	else
	{
		WSACleanup();
		SocketsActivated = false;
	}


	if (SocketsActivated) {
		pCtrlsReadThread = new std::thread(ReadControllersData);
	}
}

DLLEXPORT DWORD __stdcall GetControllersData(__out TController *myController, __out TController *myController2)
{
	if (ControllersInit) {
		ControllersInit = false;
		ControllersStart();
	}


	//Controller 1
	myController->X = -0.02;
	myController->Y = -0.05;
	myController->Z = -0.1;

	myController->Yaw = 0;
	myController->Pitch = 0;
	myController->Roll = 0;

	myController->Buttons = 0;
	myController->Trigger = 0;
	myController->ThumbX = 0;
	myController->ThumbY = 0;

	//Controller 2
	myController2->X = 0.02;
	myController2->Y = -0.05;
	myController2->Z = -0.1;

	myController2->Yaw = 0;
	myController2->Pitch = 0;
	myController2->Roll = 0;

	myController2->Buttons = 0;
	myController2->Trigger = 0;
	myController2->ThumbX = 0;
	myController2->ThumbY = 0;

	if (SocketsActivated) {
		//Controller 1

		if (Ctrl1Packet.Index == 1) {
			myController->Yaw = MyOffset(Ctrl1Packet.Yaw, Ctrl1Offset[0]);
			myController->Pitch = MyOffset(Ctrl1Packet.Pitch, Ctrl1Offset[1]);
			myController->Roll = MyOffset(Ctrl1Packet.Roll, Ctrl1Offset[2]);

			myController->Buttons = Ctrl1Packet.Buttons;
			myController->Trigger = Ctrl1Packet.Trigger;
			myController->ThumbX = round(Ctrl1Packet.JoystickX * 32767);
			myController->ThumbY = round(Ctrl1Packet.JoystickY * 32767);
		}
		if (Ctrl1Packet.Index == 2) {
			myController2->Yaw = MyOffset(Ctrl1Packet.Yaw, Ctrl1Offset[0]);
			myController2->Pitch = MyOffset(Ctrl1Packet.Pitch, Ctrl1Offset[1]);
			myController2->Roll = MyOffset(Ctrl1Packet.Roll, Ctrl1Offset[2]);

			myController2->Buttons = Ctrl1Packet.Buttons;
			myController2->Trigger = Ctrl1Packet.Trigger;
			myController2->ThumbX = round(Ctrl1Packet.JoystickX * 32767);
			myController2->ThumbY = round(Ctrl1Packet.JoystickY * 32767);
		}
		//if index == 0 then none using

		//Controller2
		if (Ctrl2Packet.Index == 1) {
			myController->Yaw = MyOffset(Ctrl2Packet.Yaw, Ctrl1Offset[0]);
			myController->Pitch = MyOffset(Ctrl2Packet.Pitch, Ctrl1Offset[1]);
			myController->Roll = MyOffset(Ctrl2Packet.Roll, Ctrl1Offset[2]);

			myController->Buttons = Ctrl2Packet.Buttons;
			myController->Trigger = Ctrl2Packet.Trigger;
			myController->ThumbX = round(Ctrl2Packet.JoystickX * 32767);
			myController->ThumbY = round(Ctrl2Packet.JoystickY * 32767);
		}
		if (Ctrl2Packet.Index == 2) {
			myController2->Yaw = MyOffset(Ctrl2Packet.Yaw, Ctrl2Offset[0]);
			myController2->Pitch = MyOffset(Ctrl2Packet.Pitch, Ctrl2Offset[1]);
			myController2->Roll = MyOffset(Ctrl2Packet.Roll, Ctrl2Offset[2]);

			myController2->Buttons = Ctrl2Packet.Buttons;
			myController2->Trigger = Ctrl2Packet.Trigger;
			myController2->ThumbX = round(Ctrl2Packet.JoystickX * 32767);
			myController2->ThumbY = round(Ctrl2Packet.JoystickY * 32767);
		}
	}

	if (SocketsActivated) {
		return 1;
	}else {
		return 0;
	}
	
}

DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in WORD	MotorSpeed)
{
	return 0;
}

DLLEXPORT DWORD __stdcall SetCentering(__in int dwIndex)
{
	if (SocketsActivated) {
		if (dwIndex == 1) {
			if (Ctrl1Packet.Index == 1) {
				Ctrl1Offset[0] = Ctrl1Packet.Yaw;
				Ctrl1Offset[1] = Ctrl1Packet.Pitch;
				Ctrl1Offset[2] = Ctrl1Packet.Roll;
			}
			else {
				Ctrl2Offset[0] = Ctrl1Packet.Yaw;
				Ctrl2Offset[1] = Ctrl1Packet.Pitch;
				Ctrl2Offset[2] = Ctrl1Packet.Roll;
			}
		}

		if (dwIndex == 2) {
			if (Ctrl2Packet.Index == 1) {
				Ctrl1Offset[0] = Ctrl2Packet.Yaw;
				Ctrl1Offset[1] = Ctrl2Packet.Pitch;
				Ctrl1Offset[2] = Ctrl2Packet.Roll;
			}
			else {
				Ctrl2Offset[0] = Ctrl2Packet.Yaw;
				Ctrl2Offset[1] = Ctrl2Packet.Pitch;
				Ctrl2Offset[2] = Ctrl2Packet.Roll;
			}
		}

		return 1;
	}
	else {
		return 0;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
){
	switch (ul_reason_for_call)
	{
		//case DLL_PROCESS_ATTACH:
		//case DLL_THREAD_ATTACH:
		//case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH: {
			if (SocketsActivated) {
				SocketsActivated = false;

				if (pCtrlsReadThread) {
					pCtrlsReadThread->join();
					delete pCtrlsReadThread;
					pCtrlsReadThread = nullptr;
				}

				closesocket(socketS);
				WSACleanup();
			}
			break;
		}
	}
		

	return TRUE;
}