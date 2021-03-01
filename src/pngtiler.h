#pragma once

#include <string>
#include <vector>
#include <emscripten/val.h>

//std::vector<unsigned char> convertSvgToPng(std::string svgWithImageTiles);

void svg2png(std::string svgString, emscripten::val cb);