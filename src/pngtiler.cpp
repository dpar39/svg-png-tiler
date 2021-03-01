#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base64.h"
#include "lodepng.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include <emscripten.h>

#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>

using namespace std;
class PngTile;
using PngTilePtr = std::shared_ptr<PngTile>;

class PngTile
{
public:
    PngTile(const int x, const int y, const int w, int h, const char * encodedData, const size_t encodedLen)
    : _x(x)
    , _y(y)
    , _w(w)
    , _h(h)
    , _encodedData(encodedData)
    , _encodedLen(encodedLen)
    {
    }

    int getX() const
    {
        return _x;
    }
    int getY() const
    {
        return _y;
    }

    int getWidth() const
    {
        return _w;
    }
    int getHeight() const
    {
        return _h;
    }

    const unsigned char * getData() const
    {
        return &_image[0];
    }

    bool loadTile(const LodePNGColorType colorType, const unsigned bitDepth)
    {
        static const auto PREFIX = std::string("data:image/png;base64,");
        const auto l = PREFIX.length();
        auto png = base64Decode(_encodedData + l, _encodedLen - l);
        unsigned width, height;
        const auto err = lodepng::decode(_image, width, height, png, colorType, bitDepth);
        return err == 0 && width == static_cast<unsigned>(_w) && height == static_cast<unsigned>(_h);
    }

    void unloadTile()
    {
        _image.clear();
        _image.shrink_to_fit();
    }

private:
    int _x, _y;
    int _w, _h;
    const char * _encodedData;
    const size_t _encodedLen;
    std::vector<unsigned char> _image;
};

int getIntAttr(const rapidxml::xml_node<> * node, const char * attrName)
{
    return std::stoi(node->first_attribute(attrName)->value());
}

std::vector<unsigned char> convertSvgToPng(std::string svgWithImageTiles)
{
    using namespace rapidxml;

    auto doc = rapidxml::xml_document<>();
    auto svgContent = svgWithImageTiles;
    doc.parse<0>(&svgContent.front());
    auto * svg = doc.first_node("svg");
    if (!svg)
        return {};

    const auto width = getIntAttr(svg, "width");
    const auto height = getIntAttr(svg, "height");

    std::vector<PngTilePtr> tiles;
    for (auto * img = svg->first_node("image"); img; img = img->next_sibling("image"))
    {
        auto x = getIntAttr(img, "x");
        auto y = getIntAttr(img, "y");
        auto w = getIntAttr(img, "width");
        auto h = getIntAttr(img, "height");

        auto * const urlAttr = img->first_attribute("xlink:href");
        auto * data = urlAttr->value();
        auto dataLen = urlAttr->value_size();
        auto tile = make_shared<PngTile>(x, y, w, h, data, dataLen);
        tiles.push_back(tile);
    }

    if (tiles.empty())
        return {};

    const auto colorType = LCT_RGBA;
    const auto bytesPerPixel = 4;
    const auto bitDepth = 8u;

    std::vector<unsigned char> scanLines;

    const lodepng::ScanLineLoader loader = [&](unsigned y) {
        int numScanLines = -1;
        const bool loadOneRowTileAtATime = true;
        if (!loadOneRowTileAtATime)
        {
            const auto numBytes = width * height * bytesPerPixel;
            scanLines.resize(numBytes, 0);
            numScanLines = height;
        }

        const auto FAIL = std::make_pair(static_cast<unsigned char *>(nullptr), 0u);
        for (const auto & tile : tiles)
        {
            if (loadOneRowTileAtATime)
            {
                if (tile->getY() == static_cast<int>(y))
                {
                    if (!tile->loadTile(colorType, bitDepth))
                        return FAIL;

                    if (numScanLines < 0)
                    {
                        numScanLines = tile->getHeight();
                        const auto numBytes = width * numScanLines * bytesPerPixel;
                        scanLines.resize(numBytes, 0);
                    }
                    else if (numScanLines != tile->getHeight())
                    {
                        tile->unloadTile();
                        assert(false && "All tiles should have the same number of scan lines");
                        return FAIL;
                    }

                    const auto * src = tile->getData();
                    const auto tileScanBytes = tile->getWidth() * bytesPerPixel;
                    for (auto row = 0; row < numScanLines; ++row)
                    {
                        auto * const dst = &scanLines[0] + (row * width + tile->getX()) * bytesPerPixel;
                        memcpy(dst, src, tileScanBytes);
                        src += tileScanBytes;
                    }
                    tile->unloadTile();
                }
            }
            else
            {
                if (!tile->loadTile(colorType, bitDepth))
                    return FAIL;

                const auto * src = tile->getData();
                const auto tileScanBytes = tile->getWidth() * bytesPerPixel;
                for (auto row = 0; row < tile->getHeight(); ++row)
                {
                    auto * const dst = &scanLines[0] + ((row + tile->getY()) * width + tile->getX()) * bytesPerPixel;
                    memcpy(dst, src, tileScanBytes);
                    src += tileScanBytes;
                }
                tile->unloadTile();
            }
        }
        return std::make_pair(&scanLines[0], static_cast<unsigned>(numScanLines));
    };

    std::vector<unsigned char> png;
    if (lodepng::encode(png, loader, width, height, colorType, bitDepth))
        return {};

    scanLines.clear();
    scanLines.shrink_to_fit();

    std::ofstream fs("C:\\Users\\dapard\\Desktop\\outTiled-stream.png", std::ios::binary);
    fs.write(reinterpret_cast<char *>(&png[0]), png.size());
    return png;
}
#ifdef __EMSCRIPTEN__

// void renderTile (const char * svgString, int w, int h, unsigned char *rawData);

// clang-format off
extern "C" {
EM_JS(void, renderTile, (const char * svgStr, int w, int h, unsigned char * pixData), {
    Asyncify.handleSleep(wakeUp => {
        console.log("Starting renderTile");
        let canvas = document.createElement("canvas");
        canvas.width = w;
        canvas.height = h;
        let ctx = canvas.getContext("2d");
        var img = new Image();
        img.onload = () => {
            ctx.drawImage(img, 0, 0);
            URL.revokeObjectURL(img.src);
            const imageData = ctx.getImageData(0, 0, w, h);
            const numBytes = imageData.data.length;
            const heapBytes = HEAPU8.subarray(pixData, pixData + numBytes);
            heapBytes.set(imageData.data);
            console.log("Tile Renderered");
            wakeUp();
            console.log("WakeUp");

        };

        const svgString = UTF8ToString(svgStr);
        const svgBlob = new Blob([svgString], {type: "image/svg+xml;charset=utf-8"});
        img.src = URL.createObjectURL(svgBlob);
    });
});
}
// clang-format on

#endif

void renderSvgTile(int x,
                   int y,
                   int tileSizeX,
                   int tileSizeY,
                   rapidxml::xml_document<> & doc,
                   std::vector<unsigned char> & tileData)
{

    using namespace std;
    const auto viewBox = to_string(x) + " " + to_string(y) + " " + to_string(tileSizeX) + " " + to_string(tileSizeY);
    auto * svg = doc.first_node("svg");
    auto * viewBoxAttr = svg->first_attribute("viewBox");
    if (!viewBoxAttr)
    {
        viewBoxAttr = doc.allocate_attribute("viewBox", viewBox.c_str());
        svg->append_attribute(viewBoxAttr);
    }
    else
    {
        viewBoxAttr->value(viewBox.c_str());
    }
    // The rendering part

    std::string s;
    print(std::back_inserter(s), doc, 0);
    std::cout << "Started rendering" << std::endl;
    renderTile(s.c_str(), tileSizeX, tileSizeY, &tileData[0]);
    std::cout << "Finished rendering" << std::endl;
}

void svg2png(std::string svgString, emscripten::val cb)
{
    using namespace rapidxml;
    auto doc = xml_document<>();
    auto svgContent = svgString;
    doc.parse<0>(&svgContent.front());
    auto * svg = doc.first_node("svg");
    if (!svg)
        cb();

    const auto colorType = LCT_RGBA;
    const auto bytesPerPixel = 4;
    const auto bitDepth = 8u;

    int ya = 0, xa = 0;

    const auto width = getIntAttr(svg, "width");
    const auto height = getIntAttr(svg, "height");

    const auto Dim16K = 1 << 14;
    const auto Area8kImage = 7680 * 4320;
    const auto tileSizeX = std::min(Dim16K, width); // 16K max
    const auto tileSizeY = std::min(Dim16K, std::min((int)std::floor(Area8kImage / width), height)); // 16K max

    std::vector<unsigned char> scanLines;
    std::vector<unsigned char> tileData;
    const lodepng::ScanLineLoader loader = [&](unsigned y) {
        int numScanLines = -1;
        if (numScanLines < 0)
        {
            numScanLines = tileSizeY;
            const auto numBytes = width * numScanLines * bytesPerPixel;
            scanLines.resize(numBytes, 0);
        }

        for (auto x = 0; x < width; x += tileSizeX)
        {
            const auto tileScanBytes = std::min(width - x, tileSizeX) * bytesPerPixel;
            // compute the bytes for this tile
            tileData.resize(tileScanBytes * numScanLines);
            renderSvgTile(x, y, tileSizeX, tileSizeY, doc, tileData);
            auto * src = tileData.data();
            for (auto row = 0; row < numScanLines; ++row)
            {
                auto * const dst = &scanLines[0] + (row * width + x) * bytesPerPixel;
                memcpy(dst, src, tileScanBytes);
                src += tileScanBytes;
            }
        }
        return std::make_pair(&scanLines[0], static_cast<unsigned>(numScanLines));
    };
    printf("We are at the end of this method");
    static std::vector<unsigned char> png;
    if (lodepng::encode(png, loader, width, height, colorType, bitDepth))
        cb();

    printf("We are at the end of this method");
    cb(val(typed_memory_view(png.size(), png.data())));
}
