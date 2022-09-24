#ifdef _WIN32
#include <windows.h>
#else
#include <uinstd.h>
#endif

#include <iostream>
#include <string>
#include <algorithm>

#include "../include/wideConsoleEngine.h"

class GameEngine : public wideConsoleEngine
{
    public:
        GameEngine() { appName_ = L"Mech Fighers 3000"; };

        virtual bool onUserCreate() {

            return true;
        }

        virtual bool onUserUpdate(float frameCounter) {

            if (counter > 1000) {
                Draw(10, 10, unicodeCharacter);
                unicodeCharacter++;
                counter = 0;
            }

            counter++;

            return true;
        }

        // Overriden to handle mech drawing routines
        virtual void Draw(int x, int y, wchar_t c = 0x2588, short col = 0x000F) {
            wideConsoleEngine::Draw(x, y, c, col);
        }

    public:
        wchar_t unicodeCharacter = 0x0000;
        int counter = 0;

};


int main()
{
    GameEngine ge;

    ge.constructConsole(80, 40);
    ge.Start();



    return 0;
}