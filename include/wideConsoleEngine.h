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
#include <mutex>

class wideConsoleEngine {
public:
    wideConsoleEngine()
    {
        screenWidth_  = 80;
        screenHeight_ = 30;

        consoleOutput_ = GetStdHandle(STD_OUTPUT_HANDLE);
        consoleInput_  = GetStdHandle(STD_INPUT_HANDLE);

        std::memset(keyNewState_, 0, 256 * sizeof(short));
        std::memset(keyOldState_, 0, 256 * sizeof(short));
        std::memset(keys_, 0, 256 * sizeof(KeyState));

        mousePosX_ = 0;
        mousePosY_ = 0;
    }

    int constructConsole(int width, int height)
    {
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
        bufScreen_ = new CHAR_INFO[screenWidth_ * screenHeight_];
        memset(bufScreen_, 0, sizeof(CHAR_INFO) * screenWidth_ * screenHeight_);

        return 1;
    }

    virtual void Draw(int x, int y, short c = 0x2588, short col = 0x00F)
    {
        if (x >= 0 && x < screenWidth_ && y >= 0 && y < screenHeight_) {
            bufScreen_[y * screenWidth_ + x].Char.UnicodeChar = c;
            bufScreen_[y * screenWidth_ + x].Attributes = col;
        }
    }

    ~wideConsoleEngine() {
        SetConsoleActiveScreenBuffer(originalConsole_);
        //delete[] bufScreen_;
    }

    void Start()
    {
        // Start the thread
        atomicActive_ = true;
        std::thread game_thread = std::thread(&wideConsoleEngine::gameThread, this);

        game_thread.join();
    }

    private:
        void gameThread()
        {
            if (!onUserCreate())
                atomicActive_ = false;

            auto frameStart = std::chrono::system_clock::now();
            auto frameEnd = std::chrono::system_clock::now();

            while (atomicActive_) {
                // Generate counter for consistent framerate
                frameEnd = std::chrono::system_clock::now();
                std::chrono::duration<float> elapsedTime = frameEnd - frameStart;
                frameStart = frameEnd;
                float frameCounter = elapsedTime.count();
                
                // Handle Keyboard inputs
                for (int i = 0; i < 256; i++) {
                    keyNewState_[i] = GetAsyncKeyState(i);

                    keys_[i].pressed = false;
                    keys_[i].released = false;

                    if (keyNewState_[i] != keyOldState_[i]) {
                        if (keyNewState_[i] & 0x8000) {
                            keys_[i].pressed = keys_[i].held;
                            keys_[i].held = true;
                        } else {
                            keys_[i].released = true;
                            keys_[i].held = false;
                        }
                    }
                    keyOldState_[i] = keyNewState_[i];
                }

                // Handle mouse input | check for window events.
                INPUT_RECORD inputBuf[32];
                DWORD events = 0;
                GetNumberOfConsoleInputEvents(consoleInput_, &events);
                if (events > 0)
                    ReadConsoleInput(consoleInput_, inputBuf, events, &events);

                // Handle when mouse and movement key input events.
                for (DWORD i = 0; i < events; i++) {
                    switch (inputBuf[i].EventType) {
                        case FOCUS_EVENT:
                            consoleInFocus_ = inputBuf[i].Event.FocusEvent.bSetFocus;
                            break;
                        case MOUSE_EVENT:
                            switch (inputBuf[i].Event.MouseEvent.dwEventFlags) {
                                case MOUSE_MOVED:
                                    mousePosX_ = inputBuf[i].Event.MouseEvent.dwMousePosition.X;
                                    mousePosY_ = inputBuf[i].Event.MouseEvent.dwMousePosition.Y;
                                    break;
                                case 0:
                                    for (int m = 0; m < 5; m++) {
                                        mouseNewState_[m] = (inputBuf[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;
                                    }
                                    break;
                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                }

                for (int m = 0; m < 5; m++) {
                    mouse_[m].pressed = false;
                    mouse_[m].released = false;

                    if (mouseNewState_[m] != mouseOldState_[m]) {
                        if (mouseNewState_[m]) {
                            mouse_[m].pressed = true;
                            mouse_[m].held = true;
                        } else {
                            mouse_[m].released = true;
                            mouse_[m].held = false;
                        }
                    }

                    mouseOldState_[m] = mouseNewState_[m];
                }

                // Handle Frame Update
                if (!onUserUpdate(frameCounter))
                    atomicActive_ = false;

                // Update title and present screen buffer
                wchar_t s[256];
                swprintf_s(s, 256, L"Wide Game Engine - %s - FPS: %3.2f", appName_.c_str(), 1.0f / frameCounter);
                SetConsoleTitle(s);
                WriteConsoleOutput(consoleOutput_, bufScreen_, { (short)screenWidth_, (short)screenHeight_ }, { 0,0 }, &rectWindow_);
            }
        }

    public: // User needs to override these!
        virtual bool onUserUpdate(float frameCounter) = 0;
        virtual bool onUserCreate() = 0;

    protected:
        int Error(const wchar_t* msg)
        {
            wchar_t buf[256];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
            SetConsoleActiveScreenBuffer(originalConsole_);
            wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
            return 0;
        }

    protected:
        // Screen and terminal
        int screenWidth_;
        int screenHeight_;
        int mousePosX_;
        int mousePosY_;
        CHAR_INFO *bufScreen_;
        HANDLE originalConsole_;
        HANDLE consoleOutput_;
        HANDLE consoleInput_;
        SMALL_RECT rectWindow_;
        bool consoleInFocus_ = true;
        std::wstring appName_;

        // Keyboard and mouse
        short keyOldState_[256] = { 0 };
        short keyNewState_[256] = { 0 };
        bool mouseOldState_[5] = { 0 };
        bool mouseNewState_[5] = { 0 };

        struct KeyState
        {
            bool pressed;
            bool released;
            bool held;
        } keys_[256], mouse_[5];

        // Threading
        // These need to be static because of the OnDestroy call the OS may make.
        // spawns a special thread just for that.
        static std::atomic<bool> atomicActive_;
        static std::mutex mutexGame_;
};

// Defining static variables
std::atomic<bool> wideConsoleEngine::atomicActive_(false);
std::mutex wideConsoleEngine::mutexGame_;
