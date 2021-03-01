
#include "pngtiler.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(my_module) {
    function("svg2png", &svg2png);
}
#else
#include <iostream>
int main(int argc, char **argv)
{
    std::cout << "This program is intended to target WebAssembly only." << std::endl;
}
#endif