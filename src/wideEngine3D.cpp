#include <iostream>
#include "../include/wideConsoleEngine.h"

class GameEngine : public wideConsoleEngine
{
    public:
        GameEngine() {};

        virtual bool onUserCreate() {

            return true;
        }

        virtual bool onUserUpdate(float frameCounter) {
            return true;
        }

};


int main()
{
    GameEngine ge;

    ge.constructConsole(80, 40);
    ge.Start();

    return 0;
}