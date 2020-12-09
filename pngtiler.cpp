#include <string>
#include <vector>
#include <memory>

#include "base64.h"
#include "lodepng.h"

#include <fstream>
#include <rapidxml/rapidxml.hpp>

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


std::vector<unsigned char> convertSvgToPng(std::string svgWithImageTiles)
{
    using namespace rapidxml;
    const auto getIntAttr = [](const xml_node<> * node, const char * attrName) {
        return std::stoi(node->first_attribute(attrName)->value());
    };

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



