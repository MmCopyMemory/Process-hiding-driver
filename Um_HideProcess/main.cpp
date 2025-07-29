#define _CRT_SECURE_NO_WARNINGS
#include "driver.h"
#include <iostream>
#include <string>
#include <cctype>

bool numonly(const std::string& str) {
	for (char c : str) {
		if (!std::isdigit(c)) return false;
	}
	return !str.empty();
}

void main() {
	std::cout << "Setting up handle to driver..." << std::endl;
	if (hideprocess_km::Initialize()) {
		std::cout << "Success!" << std::endl;
	}
	else {
		std::cout << "Unsuccessful!" << std::endl;
		Sleep(1000);
		exit(-1);
	}
	std::string procname;
	while (true) {
		std::cout << "Enter processname to hide or PID to restore -> " << std::endl;
		std::getline(std::cin, procname);
		if (numonly(procname)) {
			hideprocess_km::ShowProcess(std::stoi(procname));
		}
		else {
			size_t len = procname.length();
			wchar_t* wbuffer = new wchar_t[len + 1];
			mbstowcs(wbuffer, procname.c_str(), len + 1); //why no .w_str()?
			int pid = hideprocess_km::GetProcessId(wbuffer);
			if (pid) {
				hideprocess_km::HideProcess(pid);
				std::cout << "Hid " << pid << std::endl;
			}
			else {
				std::cout << "Couldnt find process" << std::endl;
			}
		}
	}
}