#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <format>
#include <comdef.h>
#include "dll.h"
#include "dllHex.h"

void ShowMessageBox(const char* szTitle, const char* szMessage) {

	HWND hwnd = NULL;
	UINT style = MB_OK | MB_ICONINFORMATION;
	MessageBoxA(hwnd, szMessage, szTitle, style);
}

bool InjectDLL(DWORD procID, const char* dllPath)
{
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
	if (handle == NULL) {

		// Handle process open failure
		//ShowMessageBox("Failed", "Failed to get processhandle");
		return false;
	}

	LPVOID pDllPath = VirtualAllocEx(handle, 0, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (pDllPath == NULL) {

		// Handle memory allocation failure
		CloseHandle(handle);
		//ShowMessageBox("Failed", "Failed to get DLL path");
		return false;
	}

	WriteProcessMemory(handle, pDllPath, (LPVOID)dllPath, strlen(dllPath) + 1, 0);

	HANDLE hLoadThread = CreateRemoteThread(handle, 0, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"),
		pDllPath, 0, 0);

	WaitForSingleObject(hLoadThread, INFINITE);

	VirtualFreeEx(handle, pDllPath, strlen(dllPath) + 1, MEM_RELEASE);

	CloseHandle(handle);

	//ShowMessageBox("Success", "DLL injected successfully");
	return true;
}

bool IsProcessDllLoaded(DWORD processId, const char* dllName) {
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
	if (hModuleSnap == INVALID_HANDLE_VALUE) {
		return false;
	}

	me32.dwSize = sizeof(MODULEENTRY32);

	// Retrieve information about the first module.
	if (!Module32First(hModuleSnap, &me32)) {
		CloseHandle(hModuleSnap);
		return false;
	}

	// Check if the specified DLL is loaded in the process.
	do {
		if (_stricmp(_bstr_t(me32.szModule), dllName) == 0) {
			CloseHandle(hModuleSnap);
			return true;
		}
	} while (Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);
	return false;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	static DWORD backupProcessID = 0;
	static bool bLoaded = false;

	Anticheat::Handle == Anticheat::Lib.LoadFromMemory(hexByte, sizeof(hexByte));
	// DLLDetection

	while (true) {

		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE) {

			//ShowMessageBox("Error", "Failed to get the process snapshot.");
			return 1;
		}

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);

		bool bIsMinecraftOpen = false;
		// Iterate through the processes and check for the DLL.
		if (Process32First(hProcessSnap, &pe32)) {
			do {
				std::wstring ws(pe32.szExeFile);
				std::string processName(ws.begin(), ws.end());
				if (processName == "javaw.exe") {
					if (!bLoaded) {
						//ShowMessageBox("Alert", "Started loading");
						bLoaded = InjectDLL(pe32.th32ProcessID, "C:\\AntiCheat.dll");
					}
					backupProcessID = pe32.th32ProcessID;
					bIsMinecraftOpen = true;
				}
				if (processName.find("cheatengine") != std::string::npos) {
					//ShowMessageBox("Cheat engine found", processName.c_str());
				}
				if (IsProcessDllLoaded(pe32.th32ProcessID, "speedhack-x86_64.dll")) {
					//ShowMessageBox("speedhack-x86_64.dll", std::format("ProcessID: {}, ProcessName: {}", pe32.th32ProcessID, processName).c_str());
				}
				if (IsProcessDllLoaded(pe32.th32ProcessID, "winhook-x86_64.dll")) {
					//ShowMessageBox("winhook-x86_64.dll", std::format("ProcessID: {}, ProcessName: {}", pe32.th32ProcessID, processName).c_str());
				}
			} while (Process32Next(hProcessSnap, &pe32));
		}

		if (!bIsMinecraftOpen && bLoaded)
			break;
	}
}