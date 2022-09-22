#include <iostream>
#include "../include/wideConsoleEngine.h"


int main()
{
    wideConsoleEngine wce;

    std::cout << wce.constructConsole(50, 24) << std::endl;
    return 0;
}