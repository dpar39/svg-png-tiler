#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "lodepng.h"

#include <emscripten.h>
#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>

int getIntAttr(const rapidxml::xml_node<> * node, const char * attrName)
{
    return std::stoi(node->first_attribute(attrName)->value());
}

#ifdef __EMSCRIPTEN__

// void renderTile (const char * svgString, int w, int h, unsigned char *rawData);

// clang-format off

EM_JS(void, renderTile, (const char * svgStr, int w, int h, unsigned char * pixData), {
    Asyncify.handleSleep(wakeUp => {
        let canvas = document.createElement("canvas");
        canvas.width = w;
        canvas.height = h;
        let ctx = canvas.getContext("2d");
        var img = new Image();
        img.width = "" + w + "px";
        img.height = "" + h + "px";
        img.onload = () => {
            ctx.drawImage(img, 0, 0);
            URL.revokeObjectURL(img.src);
            const imageData = ctx.getImageData(0, 0, w, h);
            const numBytes = imageData.data.length;
            const heapBytes = HEAPU8.subarray(pixData, pixData + numBytes);
            heapBytes.set(imageData.data);
            wakeUp();
        };
        const svgString = UTF8ToString(svgStr);
        const svgBlob = new Blob([svgString], {type: "image/svg+xml;charset=utf-8"});
        img.src = URL.createObjectURL(svgBlob);
    });
});
// clang-format on
#endif

void renderSvgTile(int x,
                   int y,
                   int tileSizeX,
                   int tileSizeY,
                   rapidxml::xml_document<> & doc,
                   std::vector<unsigned char> & tileData)
{
    using namespace rapidxml;
    const auto setAttr = [&](const char * attrName, const std::string & value) {
        auto * svg = doc.first_node("svg");
        auto * attr = svg->first_attribute(attrName);
        if (!attr)
        {
            attr = doc.allocate_attribute(attrName, value.c_str());
            svg->append_attribute(attr);
        }
        else
        {
            attr->value(value.c_str());
        }
    };

    using namespace std;
    const auto viewBox = to_string(x) + " " + to_string(y) + " " + to_string(tileSizeX) + " " + to_string(tileSizeY);
    const auto wStr = to_string(tileSizeX);
    const auto hStr = to_string(tileSizeY);
    setAttr("viewBox", viewBox);
    setAttr("width", wStr);
    setAttr("height", hStr);
    // Render svg as as a raster tile
    std::string s;
    print(std::back_inserter(s), doc, 0);
    renderTile(s.c_str(), tileSizeX, tileSizeY, &tileData[0]);
}

void svg2png(std::string svgString, emscripten::val options, emscripten::val cb)
{
    std::vector<std::string> store;
    using namespace rapidxml;
    auto doc = xml_document<>();
    auto svgContent = svgString;
    doc.parse<0>(&svgContent.front());
    auto * svg = doc.first_node("svg");
    if (!svg)
        cb();

    constexpr const auto OPT_MAX_TILE_PIXES = "maxTilePixels";
    constexpr const auto OPT_PROGRESS_CB = "onProgress";

    const auto colorType = LCT_RGBA;
    const auto bytesPerPixel = 4;
    const auto bitDepth = 8u;

    int ya = 0, xa = 0;

    const auto width = getIntAttr(svg, "width");
    const auto height = getIntAttr(svg, "height");

    const auto Dim16K = 1 << 14;
    const auto Area8kImage = options.hasOwnProperty(OPT_MAX_TILE_PIXES) ? options[OPT_MAX_TILE_PIXES].as<int>()
                                                                       : 7680 * 4320;

    const auto tileSizeX = std::min(Dim16K, width); // 16K max
    const auto tileSizeY = std::min(Dim16K, std::min((int)std::floor(Area8kImage / width), height)); // 16K max

    const auto numExpectedTiles = std::ceil((double)width / tileSizeX) * std::ceil((double)height / tileSizeY);

    int numProcessedTiles = 0;
    std::vector<unsigned char> scanLines;
    std::vector<unsigned char> tileData;
    const lodepng::ScanLineLoader loader = [&](unsigned y) {
        int sizeY = std::min(height - (int)y, tileSizeY);
        const auto stripNumBytes = width * sizeY * bytesPerPixel;
        scanLines.resize(stripNumBytes, 0);

        for (auto x = 0; x < width; x += tileSizeX)
        {
            const auto sizeX = std::min(width - x, tileSizeX);
            const auto tileScanBytes = sizeX * bytesPerPixel;
            // compute the bytes for this tile
            tileData.resize(tileScanBytes * sizeY);
            renderSvgTile(x, y, sizeX, sizeY, doc, tileData);
            auto * src = tileData.data();
            for (auto row = 0; row < sizeY; ++row)
            {
                auto * const dst = &scanLines[0] + (row * width + x) * bytesPerPixel;
                memcpy(dst, src, tileScanBytes);
                src += tileScanBytes;
            }

            if (options.hasOwnProperty(OPT_PROGRESS_CB))
            {
                const auto progressPct = 100.0 * (++numProcessedTiles) / numExpectedTiles;
                options[OPT_PROGRESS_CB](val(progressPct));
            }
        }
        return std::make_pair(&scanLines[0], static_cast<unsigned>(sizeY));
    };
    static std::vector<unsigned char> png;
    if (lodepng::encode(png, loader, width, height, colorType, bitDepth))
        cb();
    cb(val(typed_memory_view(png.size(), png.data())));
}
