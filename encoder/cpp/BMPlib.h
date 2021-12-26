/*
    Author: Leon Etienne
    Copyright (c) 2021, Leon Etienne
    https://github.com/Leonetienne
    https://github.com/Leonetienne/BMPlib

    License:
    Don't Be a Jerk: The Open Source Software License. Last Update: Jan, 7, 2021
    This software is free and open source.
    Please read the full license: https://github.com/Leonetienne/BMPlib/blob/master/license.txt

    #define BMPLIB_SILENT
    if you want bmplib to stop writing exceptions to stderr
*/

#pragma once
#include <sstream>
#include <fstream>
#include <string.h>

#define BMPLIB_VERSION 0.602

namespace BMPlib
{
    typedef unsigned char  byte;
    typedef unsigned short byte2;
    typedef unsigned int   byte4;

    using bytestring    = std::basic_string<byte>;
    using bytestream    = std::basic_stringstream<byte>;

    template<typename T>
    // Will convert (int)15 to 0F 00 00 00 and NOT TO 00 00 00 0F
    bytestring ToBytes(T t)
    {
        bytestream toret;
        long long sizeofT = (long long)sizeof(T); // Let's make it a signed value to keep the compiler happy with the comparison
        byte* bPtr = (byte*)&t;
        for (long long i = 0; i < sizeofT; i++)
            toret << *(bPtr + i);

        return toret.str();
    }

    template<typename ST, typename SO, typename T>
    // Will convert 0F 00 00 00 to 15 and NOT TO 251658240
    std::basic_istream<ST, SO>& FromBytes(std::basic_istream<ST, SO>& is, T& b)
    {
        const std::size_t sizeofT = sizeof(T);
        b = 0x0;
        byte buf;

        for (std::size_t i = 0; i < sizeofT; i++)
        {
            is.read((ST*)&buf, 1);
            T bbuf = buf << (i * 8);
            b |= bbuf;
        }

        return is;
    }

    class BMP
    {
    public:
        enum class COLOR_MODE
        {
            BW, // This is a special case, since BMP doesn't support 1 channel. It is just RGB, but with a 1-channel pixel buffer
            RGB,
            RGBA
        };

        BMP() noexcept
        {
            width = 0;
            height = 0;
            colorMode = COLOR_MODE::BW;
            sizeofPxlbfr = 0;
            numChannelsFile = 0;
            numChannelsPXBF = 0;
            pixelbfr = nullptr;
            isInitialized = false;
            return;
        }

        explicit BMP(const std::size_t& width, const std::size_t& height, const BMP::COLOR_MODE& colorMode = BMP::COLOR_MODE::RGB)
            : isInitialized{false}
        {
            ReInitialize(width, height, colorMode);
            return;
        }

        void ReInitialize(const std::size_t& width, const std::size_t& height, const BMP::COLOR_MODE& colorMode = BMP::COLOR_MODE::RGB)
        {
            if ((!width) || (!height)) ThrowException("Bad image dimensions!");

            // Initialize bunch of stuff
            this->width = width;
            this->height = height;
            this->colorMode = colorMode;

            switch (colorMode)
            {
            case COLOR_MODE::BW:
                numChannelsFile = 3;
                numChannelsPXBF = 1;
                break;
            case COLOR_MODE::RGB:
                numChannelsFile = 3;
                numChannelsPXBF = 3;
                break;
            case COLOR_MODE::RGBA:
                numChannelsFile = 4;
                numChannelsPXBF = 4;
                break;
            }

            // Delete pixelbuffer if already exists
            if (isInitialized) delete[] pixelbfr;
            sizeofPxlbfr = sizeof(byte) * width * height * numChannelsPXBF;

            // Try to allocate memory for the pixelbuffer
            try
            {
                pixelbfr = new byte[sizeofPxlbfr];
            }
            catch (std::bad_alloc& e)
            {
                // too bad!
                ThrowException(std::string("Can't allocate memory for pixelbuffer!") + e.what());
            }

            // Make image black
            memset(pixelbfr, 0, sizeofPxlbfr);

            isInitialized = true;
            return;
        }

#ifdef __linux__
#pragma GCC diagnostic push
// g++ doesn't see the parameters, numPx, and curPx getting used inside the switch... we don't want these false warnings
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
        // Will convert between color modes. Set isNonColorData to true to treat pixeldata as raw values rather than color
        void ConvertTo(const BMP::COLOR_MODE& convto, bool isNonColorData = false)
        {
            // Damn this method is one hell of a mess

            if (!isInitialized)
                ThrowException("Not initialized!");

            byte* tmp_pxlbfr;
            try
            {
                tmp_pxlbfr = new byte[sizeofPxlbfr];
            }
            catch (std::bad_alloc&e )
            {
                // too bad!
                ThrowException(std::string("Can't allocate memory for temporary conversion pixelbuffer!") + e.what());
                return; // This won't ever be reached but it satisfies the compiler, soooo...
            }
            const std::size_t numPx = width * height;
            const std::size_t oldPxlbfrSize = sizeofPxlbfr;
            const byte* curPx; // Usage may vary on the conversion in question. It's just a pixel cache for a small performance improvement
            memcpy(tmp_pxlbfr, pixelbfr, sizeofPxlbfr);

#ifdef __linux__
#pragma GCC diagnostic pop // Let's enable these warnings again
#endif

            switch (colorMode)
            {
            case COLOR_MODE::BW:
#ifndef __linux__
#pragma region CONVERT_FROM_BW
#endif
                switch (convto)
                {
                case COLOR_MODE::RGB:
                    // BW -> RGB

                    ReInitialize(width, height, COLOR_MODE::RGB);
                    for (std::size_t i = 0; i < numPx; i++)
                    {
                        curPx = tmp_pxlbfr + i;
                        pixelbfr[i * 3 + 0] = *curPx;
                        pixelbfr[i * 3 + 1] = *curPx;
                        pixelbfr[i * 3 + 2] = *curPx;
                    }

                    break;
                case COLOR_MODE::RGBA:
                    // BW -> RGBA

                    ReInitialize(width, height, COLOR_MODE::RGBA);
                    for (std::size_t i = 0; i < numPx; i++)
                    {
                        curPx = tmp_pxlbfr + i;
                        pixelbfr[i * 4 + 0] = *curPx;
                        pixelbfr[i * 4 + 1] = *curPx;
                        pixelbfr[i * 4 + 2] = *curPx;
                        pixelbfr[i * 4 + 3] = 0xFF;
                    }
                    break;

                default:
                    break;
                }
#ifndef __linux__
#pragma endregion
#endif
                break;

            case COLOR_MODE::RGB:
#ifndef __linux__
#pragma region CONVERT_FROM_RGB
#endif
                switch (convto)
                {
                case COLOR_MODE::BW:
                    // RGB -> BW

                    ReInitialize(width, height, COLOR_MODE::BW);
                    for (std::size_t i = 0; i < numPx; i++)
                    {
                        curPx = tmp_pxlbfr + i * 3;

                        // Don't ask me why but the compiler hates dereferencing tmp_pxlbfr/curPx via [] and throws false warnings...
                        if (isNonColorData)
                            pixelbfr[i] = (byte)((
                                *(curPx + 0) + 
                                *(curPx + 1) + 
                                *(curPx + 2)) * 0.33333333);
                        else
                            pixelbfr[i] = (byte)(
                                *(curPx + 0) * 0.3 +
                                *(curPx + 1) * 0.59 +
                                *(curPx + 2) * 0.11);
                    }
                    break;

                case COLOR_MODE::RGBA:
                    // RGB -> RGBA

                    ReInitialize(width, height, COLOR_MODE::RGBA);
                    for (std::size_t i = 0; i < numPx; i++)
                    {
                        curPx = tmp_pxlbfr + i * 3;

                        pixelbfr[i * 4 + 0] = *(curPx + 0);
                        pixelbfr[i * 4 + 1] = *(curPx + 1);
                        pixelbfr[i * 4 + 2] = *(curPx + 2);
                        pixelbfr[i * 4 + 3] = 0xFF;
                    }
                    break;

                default:
                    break;
                }
#ifndef __linux__
#pragma endregion
#endif
                break;
            
            case COLOR_MODE::RGBA:
#ifndef __linux__
#pragma region CONVERT_FROM_RGBA
#endif
                switch (convto)
                {
                case COLOR_MODE::BW:
                    // RGBA -> BW

                    ReInitialize(width, height, COLOR_MODE::BW);
                    for (std::size_t i = 0; i < numPx; i++)
                    {
                        curPx = tmp_pxlbfr + i * 4;

                        if (isNonColorData)
                            pixelbfr[i] = (byte)((
                                *(curPx + 0) + 
                                *(curPx + 1) +
                                *(curPx + 2)) * 0.33333333);
                        else
                            pixelbfr[i] = (byte)(
                                *(curPx + 0) * 0.3 +
                                *(curPx + 1) * 0.59 +
                                *(curPx + 2) * 0.11);
                    }
                    break;

                case COLOR_MODE::RGB:
                    // RGBA -> RGB

                    ReInitialize(width, height, COLOR_MODE::RGB);
                    for (std::size_t i = 0; i < numPx; i++)
                    {
                        curPx = tmp_pxlbfr + i * 4;

                        pixelbfr[i * 3 + 0] = *(curPx + 0);
                        pixelbfr[i * 3 + 1] = *(curPx + 1);
                        pixelbfr[i * 3 + 2] = *(curPx + 2);
                    }
                    break;

                default:
                    break;
                }
#ifndef __linux__
#pragma endregion
#endif
                break;
            }

            delete[] tmp_pxlbfr;
            return;
        }

        byte* GetPixelBuffer() noexcept
        {
            return pixelbfr;
        }

        const byte* GetPixelBuffer() const noexcept
        {
            return pixelbfr;
        }

        std::size_t GetWidth() const noexcept
        {
            return width;
        }

        std::size_t GetHeight() const noexcept
        {
            return height;
        }

        COLOR_MODE GetColorMode() const noexcept
        {
            return colorMode;
        }

        bool IsInitialized() const noexcept
        {
            return isInitialized;
        }

        std::size_t CalculatePixelIndex(const std::size_t& x, const std::size_t& y) const
        {
            if ((x >= width) || (y >= height)) ThrowException("Pixel coordinates out of range!");

            return numChannelsPXBF * ((y * width) + x);
        }

        byte* GetPixel(const std::size_t& x, const std::size_t& y)
        {
            return pixelbfr + CalculatePixelIndex(x, y);
        }

        const byte* GetPixel(const std::size_t& x, const std::size_t& y) const
        {
            return pixelbfr + CalculatePixelIndex(x, y);
        }

        // Sets a pixels color
        // If using RGBA, use all
        // If using RGB, a gets ignored
        // If using BW, use only r
        void SetPixel(const std::size_t& x, const std::size_t& y, const byte& r, const byte& g = 0, const byte& b = 0, const byte& a = 0)
        {
            byte* px = GetPixel(x, y);

            switch (colorMode)
            {
            case COLOR_MODE::BW:
                px[0] = r;
                break;

            case COLOR_MODE::RGB:
                px[0] = r;
                px[1] = g;
                px[2] = b;
                break;

            case COLOR_MODE::RGBA:
                px[0] = r;
                px[1] = g;
                px[2] = b;
                px[3] = a;
                break;
            }

            return;
        }

        // Will write a bmp image
        bool Write(const std::string& filename)
        {
            if (!isInitialized)
                return false;

            std::size_t paddingSize = (4 - ((width * numChannelsFile) % 4)) % 4; // number of padding bytes per scanline
            byte paddingData[4] = { 0x69,0x69,0x69,0x69 }; // dummy-data for padding

            bytestream data;
            data
                // BMP Header
                << ToBytes(byte2(0x4D42)) // signature
                << ToBytes(byte4(0x36 + sizeofPxlbfr + paddingSize * height)) // size of the bmp file (all bytes)
                << ToBytes(byte2(0))      // unused
                << ToBytes(byte2(0))      // unused
                << ToBytes(byte4(0x36))   // Offset where the pixel array begins (size of both headers)

                // DIB Header
                << ToBytes(byte4(0x28))   // Number of bytes in DIB header (without this field)
                << ToBytes(byte4(width))  // width
                << ToBytes(byte4(height)) // height
                << ToBytes(byte2(1))      // number of planes used
                << ToBytes(byte2(numChannelsFile * 8)) // bit-depth
                << ToBytes(byte4(0))      // no compression
                << ToBytes(byte4(sizeofPxlbfr + paddingSize * height)) // Size of raw bitmap data (including padding)
                << ToBytes(byte4(0xB13))  // print resolution pixels/meter X
                << ToBytes(byte4(0xB13))  // print resolution pixels/meter Y
                << ToBytes(byte4(0))      // 0 colors in the color palette
                << ToBytes(byte4(0));     // 0 means all colors are important

            // Dumbass unusual pixel order of bmp made me do this...
            for (long long y = height - 1; y >= 0; y--)
            {
                for (std::size_t x = 0; x < width; x++)
                {
                    std::size_t idx = CalculatePixelIndex(x, (std::size_t)y); // No precision lost here. I just need a type that can get as large as std::size_t but is also signed (because for y >= 0)

                    switch (colorMode)
                    {
                    case COLOR_MODE::BW:
                        // pixelbfr ==> R-G-B ==> B-G-R ==> bmp format
                        data.write((pixelbfr + idx), 1); // B
                        data.write((pixelbfr + idx), 1); // G
                        data.write((pixelbfr + idx), 1); // R

                        break;

                    case COLOR_MODE::RGB:
                        // pixelbfr ==> R-G-B ==> B-G-R ==> bmp format
                        data.write((pixelbfr + idx + 2), 1); // B
                        data.write((pixelbfr + idx + 1), 1); // G
                        data.write((pixelbfr + idx + 0), 1); // R

                        break;

                    case COLOR_MODE::RGBA:
                        // pixelbfr ==> R-G-B-A ==> B-G-R-A ==> bmp format
                        data.write((pixelbfr + idx + 2), 1); // B
                        data.write((pixelbfr + idx + 1), 1); // G
                        data.write((pixelbfr + idx + 0), 1); // R
                        data.write((pixelbfr + idx + 3), 1); // A

                        break;
                    }
                }

                if ((colorMode == COLOR_MODE::BW) || (colorMode == COLOR_MODE::RGB))
                    if (paddingSize > 0)
                        data.write(paddingData, paddingSize);
            }



            // write file
            std::ofstream bs;
            bs.open(filename, std::ofstream::binary);
            if (!bs.good())
                return false;

            bytestring bytes = data.str();
            bs.write((const char*)bytes.c_str(), bytes.length());
            bs.flush();
            bs.close();

            return true;
        }

        // Will read a bmp image
        bool Read(std::string filename)
        {
            std::ifstream bs;
            bs.open(filename, std::ifstream::binary);
            if (!bs.good())
                return false;

            // Check BMP signature
            byte2 signature;
            FromBytes(bs, signature);
            if (signature != 0x4D42)
                return false;

            // Gather filesize
            byte4 fileLen;
            FromBytes(bs, fileLen);

            byte2 unused0;
            byte2 unused1;
            FromBytes(bs, unused0);
            FromBytes(bs, unused1);

            byte4 offsetPixelArray;
            FromBytes(bs, offsetPixelArray);

            byte4 dibHeadLen;
            FromBytes(bs, dibHeadLen);

            // Gather image dimensions
            byte4 imgWidth;
            byte4 imgHeight;
            FromBytes(bs, imgWidth);
            FromBytes(bs, imgHeight);

            byte2 numPlanes;
            FromBytes(bs, numPlanes);

            // Gather image bit-depth
            byte2 bitDepth;
            FromBytes(bs, bitDepth);
            switch (bitDepth)
            {
                // BW is not supported so we can't read a bw image
            case 24:
                colorMode = COLOR_MODE::RGB;
                break;
            case 32:
                colorMode = COLOR_MODE::RGBA;
            }

            byte4 compression;
            FromBytes(bs, compression);

            // Gather size of pixel buffer
            byte4 sizeofPixelBuffer;
            FromBytes(bs, sizeofPixelBuffer);

            byte4 printresX;
            byte4 printresY;
            FromBytes(bs, printresX);
            FromBytes(bs, printresY);

            byte4 colorsInPalette;
            byte4 importantColors;
            FromBytes(bs, colorsInPalette);
            FromBytes(bs, importantColors);

            // Go to the beginning of the pixel array
            bs.seekg(offsetPixelArray);

            // Initialize image
            ReInitialize(imgWidth, imgHeight, colorMode);

            // Calculate scanline padding size
            std::size_t paddingSize = (4 - ((width * numChannelsFile) % 4)) % 4;
            paddingSize = paddingSize < 4 ? paddingSize : 0;

            // Dumbass unusual pixel order of bmp made me do this...
            for (long long y = imgHeight - 1; y >= 0; y--)
            {
                std::size_t byteCounter = 0;
                for (std::size_t x = 0; x < imgWidth; x++)
                {
                    std::size_t idx = CalculatePixelIndex(x, (std::size_t)y); // No precision lost here. I just need a type that can get as large as std::size_t but is also signed (because for y >= 0)

                    switch (colorMode)
                    {
                    case COLOR_MODE::RGB:
                        // bmp format ==> B-G-R ==> R-G-B ==> pixelbfr
                        bs.read((char*)(pixelbfr + idx + 2), 1); // Read B byte
                        bs.read((char*)(pixelbfr + idx + 1), 1); // Read G byte
                        bs.read((char*)(pixelbfr + idx + 0), 1); // Read R byte
                        byteCounter += 3;
                        break;

                    case COLOR_MODE::RGBA:
                        // bmp format ==> B-G-R-A ==> R-G-B-A ==> pixelbfr
                        bs.read((char*)(pixelbfr + idx + 2), 1); // Read B byte
                        bs.read((char*)(pixelbfr + idx + 1), 1); // Read G byte
                        bs.read((char*)(pixelbfr + idx + 0), 1); // Read R byte
                        bs.read((char*)(pixelbfr + idx + 3), 1); // Read A byte
                        byteCounter += 4;
                        break;

                    default:
                        break;
                    }
                }

                if ((colorMode == COLOR_MODE::BW) || (colorMode == COLOR_MODE::RGB)) // RGBA will always be a multiple of 4 bytes
                   if (byteCounter % 4) // If this scan line was not a multiple of 4 bytes long, account for padding
                        bs.ignore(paddingSize);
            }

            bs.close();
            return true;
        }

        ~BMP()
        {
            if (isInitialized)
            {
                delete[] pixelbfr;
                pixelbfr = nullptr;
            }
            return;
        }

    private:
        void ThrowException(const std::string msg) const
        {
            #ifndef BMPLIB_SILENT
            #ifdef _IOSTREAM_
            std::cerr << "BMPlib exception: " << msg << std::endl;
            #endif
            #endif
            throw msg;
            return;
        }

        std::size_t width;
        std::size_t height;
        std::size_t numChannelsFile; // Num channels of the file format
        std::size_t numChannelsPXBF; // Num channels of the pixel buffer
        COLOR_MODE colorMode;
        bool isInitialized;
        byte* pixelbfr;
        std::size_t sizeofPxlbfr; // how many bytes the pixelbuffer is long
    };
}
