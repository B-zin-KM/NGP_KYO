#pragma once
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>   
#include <process.h> 
#include <windows.h> 
#include <stdbool.h> 
#include <time.h>     

#pragma comment(lib, "ws2_32.lib")

void err_quit(const char* msg);
void err_display(const char* msg);