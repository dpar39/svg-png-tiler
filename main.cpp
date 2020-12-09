#include <ostream>
#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

#include <iostream>

int main(int argc, char **argv)
{
    std::cout << "First CMAKE with VSCode" << std::endl;
}