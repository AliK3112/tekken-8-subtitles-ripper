#pragma warning(disable : 4996)
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <conio.h>
#include <thread>
#include <string>
#include <list>
#include <iomanip>
#include <vector>
#include <set>

HANDLE processHandle = NULL;	 // Variable to Handle the process
uintptr_t gameBaseAddress = 0; // Base Address of the Game
uintptr_t START_ADDRESS = 0;
uintptr_t END_ADDRESS = 0;

std::wstring charToWstring(const char *narrowStr);
uintptr_t findProcessByName(const std::wstring &processName);
uintptr_t GetModuleBaseAddress(TCHAR *lpszModuleName, uintptr_t pID);
void getOffsetsList();
bool prepare();
bool isProcessRunning(HANDLE processHandle);
void printHex(std::string prompt, uintptr_t value);
std::string readString(uintptr_t address, int length);
std::string readUTF8String(uintptr_t address, int length);
int readInt(uintptr_t address);
short readShort(uintptr_t address);
int64_t readLong(uintptr_t address);
uintptr_t readULong(uintptr_t address);
void writeInt(uintptr_t address, int value);

int main()
{
	int c = 1;
	uintptr_t pID = 10708;
	while (1)
	{
		pID = findProcessByName(L"Polaris-Win64-Shipping.exe");
		if (pID == 0)
		{
			if (c == 1)
			{
				std::cout << "Game window not found.\nPlease Run TEKKEN 7\nWaiting...";
				c++;
			}
			Sleep(1000);
		}
		else
			break;
	}
	std::cout << "Window Found!\n";

	processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pID);
	if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
	{
		std::cout << "Unable to attach to process\nClosing Program...";
		Sleep(2500);
		return 0;
	}

	std::cout << "Script Attached\n";
	TCHAR proc_name[] = TEXT("Polaris-Win64-Shipping.exe");
	gameBaseAddress = GetModuleBaseAddress(proc_name, pID);
	printHex("current_address", gameBaseAddress);
	bool okay = prepare();
	if (okay)
		getOffsetsList();
	printf("Press any key to close...");
	getch();
	return 0;
}

bool prepare()
{
	std::ifstream file;
	file.open("addresses.ini");
	if (!file.is_open())
	{
		printf("Unable to open file 'addresses.ini'\n");
		return false;
	}
	std::string line;
	try
	{
		while (std::getline(file, line))
		{
			std::istringstream iss(line);
			std::string key, value;
			if (std::getline(iss, key, '=') && std::getline(iss, value))
			{
				if (key == "start_address")
				{
					START_ADDRESS = std::stoull(value, nullptr, 16);
				}
				else if (key == "end_address")
				{
					END_ADDRESS = std::stoull(value, nullptr, 16);
				}
			}
		}
	}
	catch (const std::invalid_argument &e)
	{
		printf("Invalid values in the text file\n");
		return false;
	}
	catch (const std::out_of_range &e)
	{
		printf("values out of range\n");
		return false;
	}
	file.close();
	return true;
}

void getOffsetsList()
{
	uintptr_t startAddress = START_ADDRESS;
	uintptr_t endAddress = END_ADDRESS;
	uintptr_t addr = startAddress;
	// List to store everything
	std::list<std::string> formattedStrings;

	while (addr <= endAddress)
	{
		std::string str = readString(addr, 4);
		if (str.compare("text") != 0)
		{
			printf("Invalid Text\n");
			break;
		}
		int _0x4 = readInt(addr + 4);	 // length of this text entity's header?
		int _0x8 = readInt(addr + 8);	 // max length of file name?
		int _0xC = readInt(addr + 12); // max length of text? min is 8

		std::string fileName = readString(addr + 8 + _0x4, _0x8);
		std::string subtitle = readString(addr + 8 + _0x4 + _0x8, _0xC);

		addr = addr + _0x4 + _0x8 + _0xC + 8;

		// Create a stringstream for formatting
		std::ostringstream oss;

		// Format the string
		oss << std::left << std::setw(50) << fileName << " | " << subtitle;

		formattedStrings.push_back(oss.str());

		printf("%-50s | %s\n", fileName.c_str(), subtitle.c_str());
	}

	// Logic to print them into a file
	std::ofstream write("output.txt");
	if (!write.is_open())
	{
		std::cerr << "Failed to open output file." << std::endl;
		return;
	}
	// Write each formatted string to the file
	for (const auto &str : formattedStrings)
	{
		write << str << std::endl;
	}
	write.close();
	printf("Contents written to output.txt\n");
}

uintptr_t GetModuleBaseAddress(TCHAR *lpszModuleName, uintptr_t pID)
{
	uintptr_t dwModuleBaseAddress = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, (DWORD)pID); // make snapshot of all modules within process
	MODULEENTRY32 ModuleEntry32 = {0};
	ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
	if (Module32First(hSnapshot, &ModuleEntry32)) // store first Module in ModuleEntry32
	{
		do
		{
			if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0) // if Found Module matches Module we look for -> done!
			{
				dwModuleBaseAddress = (uintptr_t)ModuleEntry32.modBaseAddr;
				break;
			}
		} while (Module32Next(hSnapshot, &ModuleEntry32)); // go through Module entries in Snapshot and store in ModuleEntry32
	}
	CloseHandle(hSnapshot);
	return dwModuleBaseAddress;
}

std::wstring charToWstring(const char *narrowStr)
{
	std::wstring wideStr;
	if (narrowStr != nullptr)
	{
		size_t len = std::mbstowcs(nullptr, narrowStr, 0);
		if (len != static_cast<size_t>(-1))
		{
			wideStr.resize(len);
			std::mbstowcs(&wideStr[0], narrowStr, len + 1);
		}
	}
	return wideStr;
}

uintptr_t findProcessByName(const std::wstring &processName)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(snapshot, &entry))
		{
			do
			{
				std::wstring currentProcessName = charToWstring(entry.szExeFile);
				if (currentProcessName == processName)
				{
					return static_cast<uintptr_t>(entry.th32ProcessID);
				}
			} while (Process32Next(snapshot, &entry));
		}

		CloseHandle(snapshot);
	}

	return 0;
}

bool isProcessRunning(HANDLE processHandle)
{
	DWORD exitCode;
	if (GetExitCodeProcess(processHandle, &exitCode) && exitCode == STILL_ACTIVE)
	{
		// The process is still running
		return true;
	}

	// The process has been closed
	return false;
}

void printHex(std::string prompt, uintptr_t value)
{
	printf("%s = 0x%.16llx\n", prompt.c_str(), value);
}

std::string readString(uintptr_t address, int length = 256)
{
	char buffer[256];
	if (!ReadProcessMemory(processHandle, (LPVOID)(address), buffer, sizeof(buffer), NULL))
		return 0;
	std::string s = buffer;
	return s.substr(0, length);
}

std::string readUTF8String(uintptr_t address, int length = 256)
{
    // Create a buffer to read the memory
    std::vector<char> buffer(length);
    if (!ReadProcessMemory(processHandle, (LPVOID)(address), buffer.data(), buffer.size(), NULL))
        return "";

    // Ensure the buffer is null-terminated
    buffer.push_back('\0');

    // Convert the buffer to a UTF-8 encoded string
    std::string utf8Str(buffer.data());
    
    return utf8Str;
}

int readInt(uintptr_t address)
{
	int value = 0;
	if (!ReadProcessMemory(processHandle, (LPVOID)(address), &value, sizeof(value), NULL))
		return 0;
	return value;
}

short readShort(uintptr_t address)
{
	short value = 0;
	if (!ReadProcessMemory(processHandle, (LPVOID)(address), &value, sizeof(value), NULL))
		return 0;
	return value;
}

int64_t readLong(uintptr_t address)
{
	int64_t value = 0;
	if (!ReadProcessMemory(processHandle, (LPVOID)(address), &value, sizeof(value), NULL))
		return 0;
	return value;
}

uintptr_t readULong(uintptr_t address)
{
	uintptr_t value = 0;
	if (!ReadProcessMemory(processHandle, (LPVOID)(address), &value, sizeof(value), NULL))
		return 0;
	return value;
}

void writeInt(uintptr_t address, int value)
{
	WriteProcessMemory(processHandle, (LPVOID)(address), &value, sizeof(value), NULL);
}
