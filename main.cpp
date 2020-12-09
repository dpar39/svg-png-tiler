
#include "pngtiler.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(my_module) {
    function("convertSvgToPng", &convertSvgToPng);
}

#else
#include <iostream>
int main(int argc, char **argv)
{
    std::cout << "First CMAKE with VSCode" << std::endl;
}
#endif