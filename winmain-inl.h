#pragma once
#include <WinSock2.h>
#include <iostream>

namespace {
	struct OnApp {
		OnApp() {
#ifdef _WIN32
			// Initialize Winsock 2.2
			WSADATA wsaData;
			int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

			if (err) {
				std::cout << "WSAStartup() failed with error: %d" << err;
			}
#endif
		}
		~OnApp() {
#ifdef _WIN32
			system("pause");
			WSACleanup();
#endif
		}
	} __s_onexit_pause;
}