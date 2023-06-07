#pragma once
#include <thread>
#include <string>
#include <Windows.h>
#include <format>

using namespace std::literals::chrono_literals;
void DLLDetection();
void SpeedHackDetection();
void ShowMessageBox(const char* szTitle, const char* szMessage);

/* Windows API dll entry */
int __stdcall DllMain(HINSTANCE hinstDLL, unsigned long fdwReason, void* lpReserved) {

	/* At successfull dll injection */
	if (fdwReason == DLL_PROCESS_ATTACH) {

		ShowMessageBox("Injected", "AntiCheat.dll was successfully injected");

		/* Disable other reason calls */
		DisableThreadLibraryCalls(hinstDLL);

		/* CreateThreads for the new functions, and close their handle to not memoryleak */
		if (HANDLE thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(DLLDetection), nullptr, 0, nullptr))
			CloseHandle(thread);

		if (HANDLE thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(SpeedHackDetection), nullptr, 0, nullptr))
			CloseHandle(thread);
	}

	return true;
}

void DLLDetection() {

	while (true) {

		/*
			///Cheat Engine anticheat\\\
			speedhack-x86_64:	hooks modified GetTickCount64() with detouring
			winhook-x86_64:		hooks windows debug funtions to search inside app memory

			those dll checks fix obvious hacks such as:
			{reach; fly; speedhacking; etc..}
		*/

		HMODULE cheatEngineSpeedHackHooks = GetModuleHandleA("speedhack-x86_64.dll");
		HMODULE cheatEngineWindowsHooks = GetModuleHandleA("winhook-x86_64.dll");

		/* FreeLibraryAndExitThread closes the handle that is used by the dlls ("uninject") */
		if (cheatEngineSpeedHackHooks) {
			ShowMessageBox("Detection!", std::format("speedhack-x86_64.dll was detected at memory location: {}", (DWORD64)&cheatEngineSpeedHackHooks).c_str());
			FreeLibrary(cheatEngineSpeedHackHooks);
		}

		if (cheatEngineWindowsHooks) {
			ShowMessageBox("Detection!", std::format("winhook-x86_64.dll was detected at memory location: {}", (DWORD64)&cheatEngineWindowsHooks).c_str());
			FreeLibrary(cheatEngineWindowsHooks);
		}

		/* Don't need to check every tick */
		std::this_thread::sleep_for(5s);
	}
}

void SpeedHackDetection() {

	while (true) {

		std::this_thread::sleep_for(1ms);

		/* Detection & tickcount record variables */
		static int detectionChances = 50;
		static ULONGLONG oldTickCount = 0;
		static int LastDifference = 0;
		static int FalseAlarms = 0;

		/*
			GetTickCount64 intervals typically in the range of 10 milliseconds to 16 milliseconds
			if it's greater than 16, user is manipulating game time (speed hacking)
		*/
		if (oldTickCount != GetTickCount64()) {

			/* Initialize the value before false alarming */
			if (oldTickCount == 0)
				oldTickCount = GetTickCount64();

			/* 20 chances for the player (approx: 0.2 second of speedhacking) */
			if (ULONGLONG difference = GetTickCount64() - oldTickCount; difference > 16) {

				/* Only alert when difference is constant */
				if (LastDifference == difference) {
					
					FalseAlarms++;
					if (FalseAlarms > 5) {
						detectionChances--;
						ShowMessageBox("Unusual speed detected!!", std::format("Game was sped up by {}%\nDetection chances left: {}", difference / 16.f, detectionChances).c_str());
					}
				}
				else 
					FalseAlarms = 0;
				
				LastDifference = difference;
			}

			/* Running out of chanced causes a force crash by dereferencing a nullptr */
			if (detectionChances < 0)
				*((unsigned int*)0) = 0xDEAD;

			/* Backing up tickcount to calculate interval between them */
			oldTickCount = GetTickCount64();
		}
	}
}

void ShowMessageBox(const char* szTitle, const char* szMessage) {

	HWND hwnd = NULL;
	UINT style = MB_OK | MB_ICONINFORMATION;
	MessageBoxA(hwnd, szMessage, szTitle, style);
}