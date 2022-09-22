/* wideConsoleEngine.h
* 
* Used to create console games.
* 
* Glyph -> UNICODE: 16-bit (65536)
*   wchar_t or tchar (compiler dependant)
* 
* Construct Console
* -> returns true or false
* -> Start
* -> Create/Load Resources
* 
* Game Loop ->
*       Update Timing (elapsed time)
*       -> Update Input
*       -> Update Game State & Update Screen ->(true) Game Loop
*
*
*/
#pragma once
#pragma comment(lib, "winmm.lib")

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#include <iostream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>

class wideConsoleEngine {
public:
    wideConsoleEngine() {
        screenWidth_  = 80;
        screenHeight_ = 30;

        consoleOutput_ = GetStdHandle(STD_OUTPUT_HANDLE);
        consoleInput_  = GetStdHandle(STD_INPUT_HANDLE);

        mousePosX_ = 0;
        mousePosY_ = 0;
    }

    int constructConsole(int width, int height) {
        // TODO error checking.
        screenWidth_  = width;
        screenHeight_ = height;
        
        // Declaring window upper left and lower right coordinates to pass in.
        rectWindow_ = { 0, 0, 1, 1 };

        // Sets the current size and position of a console screen buffer's window.
        if (!SetConsoleWindowInfo(consoleOutput_, TRUE, &rectWindow_))
            return Error(L"SetConsoleWindowInfo 1st");

        // Get console info for triple checking everything is setup properly.
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(consoleOutput_, &consoleInfo);
        if (width > consoleInfo.dwMaximumWindowSize.X || height > consoleInfo.dwMaximumWindowSize.Y) {
            std::cout << "Max width: " << consoleInfo.dwMaximumWindowSize.X << std::endl;
            std::cout << "Max height: " << consoleInfo.dwMaximumWindowSize.Y << std::endl;
            rectWindow_ = { 0, 0, 80, 40 };
            SetConsoleWindowInfo(consoleOutput_, TRUE, &rectWindow_);
            return Error(L"Screen width or height are too big.");
        }

        // Setting the size of the screen buffer.
        //COORD coord = { (short)screenWidth_, (short)screenHeight_ };
        COORD coord;
        coord.X = screenWidth_;
        coord.Y = screenHeight_;
        if (!SetConsoleScreenBufferSize(consoleOutput_, coord))
            Error(L"SetConsoleScreenBufferSize");

        // Assign screen buffer to the console.
        if (!SetConsoleActiveScreenBuffer(consoleOutput_))
            return Error(L"SetConsoleActiveScreenBuffer");
        
        // Set the physical console window size.
        rectWindow_.Top = 0;
        rectWindow_.Left = 0;
        rectWindow_.Bottom = height - 1;
        rectWindow_.Right = width - 1;
        SetConsoleWindowInfo(consoleOutput_, TRUE, &rectWindow_);
        
        // Set flags to allow mouse input
        if (!SetConsoleMode(consoleInput_, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
            return Error(L"SetConsoleMode");
        
        // Allocate memory for the screen buffer.
        //bufScreen_ = new CHAR_INFO[screenWidth_ * screenHeight_];
        //memset(bufScreen_, 0, sizeof(CHAR_INFO) * screenWidth_ * screenHeight_);

        return 1;
    }

    ~wideConsoleEngine() {
        SetConsoleActiveScreenBuffer(originalConsole_);
        //delete[] bufScreen_;
    }

    protected:
        int Error(const wchar_t* msg) {
            wchar_t buf[256];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
            SetConsoleActiveScreenBuffer(originalConsole_);
            wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
            return 0;
        }

    protected:
        int screenWidth_;
        int screenHeight_;
        int mousePosX_;
        int mousePosY_;
        CHAR_INFO *bufScreen_;
        HANDLE originalConsole_;
        HANDLE consoleOutput_;
        HANDLE consoleInput_;
        SMALL_RECT rectWindow_;
};
