// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <fstream>

struct PrnFileHeader
{
    int SizeHeader = 0;//指定文件头的字节数；
    int Colors = 0;//指定除白色和光油外的颜色个数；
    int UseWhite = 0;//1代表使用白色，0代表不使用白色
    int UseVarnish = 0;//1代表使用光油，0代表不使用光油
    int Width = 0;//图象象素宽度；
    int Height = 0;//图象象素高度；
    int BitPerPixel = 0;//指定每个象素有几位数据来表达；
    int GrayLevel = 0;//象素灰度等级；
    int ResolutionX = 0; //指定图象X方向精度
    int ResolutionY = 0; //指定图象Y方向精度
    int SizeImageLow = 0;  //指定图象所有字节数的低32位
    int SizeImageHigh = 0; //指定图象所有字节数的高32位
    int BytesPerLine = 0;//指定每行每个颜色字节数；
    int BiDirection = 0;//1代表双向，0代表单向
    int Pass = 0;//指定此时文件的PASS数，1，2，3，4，6，8，16，32；
    int Version = 0;//软件版本号
    int Manufacturer = 0;//制造商

    int rev[13] = { 0 };

    bool IsOk()
    {
        return SizeHeader == 116;
    }
};

constexpr size_t prnheadersize = sizeof(PrnFileHeader);

static inline uint32_t BitCount4(uint32_t n)
{
    n = (n & 0x55555555) + ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f);
    n = (n & 0x00ff00ff) + ((n >> 8) & 0x00ff00ff);
    n = (n & 0x0000ffff) + ((n >> 16) & 0x0000ffff);

    return n;
}

static inline int BitCount2(short v)
{
    int n = v;
    n = (n & 0x5555) + ((n >> 1) & 0x5555);
    n = (n & 0x3333) + ((n >> 2) & 0x3333);
    n = (n & 0x0f0f) + ((n >> 4) & 0x0f0f);
    n = (n & 0x00ff) + ((n >> 8) & 0x00ff);
    return n;
}

static const int ones[] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

static int CalcDots(uint8_t* linedata, size_t size)
{
    int dots = 0;
    size_t count = size / 4;
    uint32_t* pdata = (uint32_t*)linedata;
    for (int i = 0; i < count; i++)
    {
        dots += BitCount4(pdata[i]);
    }
    return dots;
}

typedef void(_stdcall* updatefunc)();
extern "C" __declspec(dllexport) void _stdcall ReadPrnInkCount(const char *pfilename, uint64_t *pinkcounts, updatefunc update)
{
    std::ifstream fs(pfilename, std::ios::binary | std::ios::in);
    if (fs.good())
    //FILE* pFile = fopen(pfilename, "rb");
    //if(pFile)
    {
        PrnFileHeader prnFileHeader;
        fs.read((char *) & prnFileHeader, prnheadersize);
        //fread((char *) & prnFileHeader, 1, prnheadersize, pFile);
        if (!prnFileHeader.IsOk())
            return ;

        if (1)
        {
            auto linesize = prnFileHeader.BytesPerLine;
            std::unique_ptr<uint8_t[]> buf(new uint8_t[linesize]);
            auto linedata = buf.get();
            for (int i = 0; i < prnFileHeader.Height; i++)
            {
                for (int j = 0; j < prnFileHeader.Colors; j++)
                {
                    fs.read((char *)linedata, linesize);
                    //fread((char*)linedata, 1, linesize, pFile);
                    int dots = CalcDots(linedata, linesize);
                    pinkcounts[j] += dots;
                }
                if ((i & 0x3FF) == 0x3FF)
                {
                    if (update)
                        update();
                }
            }
        }
        fs.close();
        //fclose(pFile);

        if(update)
            update();
    }
}


extern "C" __declspec(dllexport) void _stdcall CalcPrnImage(const char* pfilename, uint8_t * rawImage, updatefunc update)
{
    std::ifstream fs(pfilename, std::ios::binary | std::ios::in);
    if (fs.good())
        //FILE* pFile = fopen(pfilename, "rb");
        //if(pFile)
    {
        PrnFileHeader prnFileHeader;
        fs.read((char*)&prnFileHeader, prnheadersize);
        //fread((char *) & prnFileHeader, 1, prnheadersize, pFile);
        if (!prnFileHeader.IsOk())
            return;

        int width = (prnFileHeader.Width + 15) / 16;
        int height = (prnFileHeader.Height + 15) / 16;
        int XDpi = prnFileHeader.ResolutionX / 16;
        int YDpi = prnFileHeader.ResolutionY / 16;

        auto rawStride = (width * 32 + 7) / 8;

        std::unique_ptr<uint8_t[]> k(new uint8_t[width]);
        std::unique_ptr<uint8_t[]> c(new uint8_t[width]);
        std::unique_ptr<uint8_t[]> m(new uint8_t[width]);
        std::unique_ptr<uint8_t[]> y(new uint8_t[width]);
        uint8_t *pixvalue[4] = { k.get(), c.get(), m.get(), y.get()};

        auto linesize = prnFileHeader.BytesPerLine;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[linesize]);
        auto linedata = buf.get();
        uint16_t *linedata_16 = (uint16_t*)linedata;

        int rawpos = 0;
        for (int i = 0; i < prnFileHeader.Height; i++)
        {
            if ((i & 0xf) == 0)
            {
                for (int j = 0; j < 4; j++)
                {
                    for (int k = 0; k < width; k++)
                        pixvalue[j][k] = 0;
                }
            }

            for (int j = 0; j < prnFileHeader.Colors; j++)
            {
                fs.read((char*)linedata, linesize);
                if (j < 4)
                {
                    for (int m = 0; m < width; m++)
                    {
                        int sum = pixvalue[j][m] + BitCount2(linedata_16[m]);
                        pixvalue[j][m] = sum > 255 ? 255 : sum;
                    }
                }
            }

            if (((i & 0xf) == 0xf) || (i == (prnFileHeader.Height - 1)))
            {
                for (int k = 0; k < width; k++)
                {
                    rawImage[rawpos + k * 4 + 0] = pixvalue[1][k];
                    rawImage[rawpos + k * 4 + 1] = pixvalue[2][k];
                    rawImage[rawpos + k * 4 + 2] = pixvalue[3][k];
                    rawImage[rawpos + k * 4 + 3] = pixvalue[0][k];
                }
                rawpos += rawStride;
            }

            if ((i & 0x3fff) == 0x3fff)
            {
                if(update)
                    update();
            }
        }
        if(update)
            update();
    }
}

extern "C" __declspec(dllexport) void _stdcall CalcPrnImageAndInkDots(const char* pfilename, int XScale, int YScale, uint8_t * rawImage, uint64_t * pinkcounts, int *percent, updatefunc update)
{
    std::ifstream fs(pfilename, std::ios::binary | std::ios::in);
    if (fs.good())
        //FILE* pFile = fopen(pfilename, "rb");
        //if(pFile)
    {
        PrnFileHeader prnFileHeader;
        fs.read((char*)&prnFileHeader, prnheadersize);
        //fread((char *) & prnFileHeader, 1, prnheadersize, pFile);
        if (!prnFileHeader.IsOk())
            return;
        int width = (prnFileHeader.Width + XScale-1) / XScale;
        int height = (prnFileHeader.Height + YScale-1) / YScale;

        auto rawStride = width * 4;

        std::unique_ptr<uint32_t[]> k(new uint32_t[width]);
        std::unique_ptr<uint32_t[]> c(new uint32_t[width]);
        std::unique_ptr<uint32_t[]> m(new uint32_t[width]);
        std::unique_ptr<uint32_t[]> y(new uint32_t[width]);
        uint32_t* pixvalue[4] = { k.get(), c.get(), m.get(), y.get() };

        auto linesize = prnFileHeader.BytesPerLine;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[linesize]);

        auto func8 = [&]() {
            auto linedata = buf.get();
            uint8_t* linedata_8 = (uint8_t*)linedata;

            int pixmaxv = XScale * YScale;

            int rawpos = 0;
            for (int i = 0; i < prnFileHeader.Height; i++)
            {
                if ((i & 0x07) == 0)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        for (int k = 0; k < width; k++)
                            pixvalue[j][k] = 0;
                    }
                }

                for (int j = 0; j < prnFileHeader.Colors; j++)
                {
                    fs.read((char*)linedata, linesize);
                    if (j < 4)
                    {
                        for (int m = 0; m < width; m++)
                        {
                            int dots = ones[linedata_8[m]];
                            pixvalue[j][m] += dots;
                            pinkcounts[j] += dots;
                        }
                    }
                    else
                    {
                        int dots = CalcDots(linedata, linesize);
                        pinkcounts[j] += dots;
                    }
                }

                if (((i % YScale) == (YScale - 1)) || (i == (prnFileHeader.Height - 1)))
                {
                    for (int k = 0; k < width; k++)
                    {
                        rawImage[rawpos + k * 4 + 0] = pixvalue[1][k] >= pixmaxv ? 255 : (pixvalue[1][k] << 2);
                        rawImage[rawpos + k * 4 + 1] = pixvalue[2][k] >= pixmaxv ? 255 : (pixvalue[2][k] << 2);
                        rawImage[rawpos + k * 4 + 2] = pixvalue[3][k] >= pixmaxv ? 255 : (pixvalue[3][k] << 2);
                        rawImage[rawpos + k * 4 + 3] = pixvalue[0][k] >= pixmaxv ? 255 : (pixvalue[0][k] << 2);
                    }
                    rawpos += rawStride;
                }

                if ((i & 0x3f) == 0x3f)
                {
                    if (percent)
                        *percent = i * 100 / prnFileHeader.Height;
                    if (update)
                        update();
                }
            }
        };

        auto func16 = [&]() {
            auto linedata = buf.get();
            uint16_t* linedata_16 = (uint16_t*)linedata;

            int pixmaxv = XScale * YScale;

            int rawpos = 0;
            for (int i = 0; i < prnFileHeader.Height; i++)
            {
                if ((i & 0x07) == 0)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        for (int k = 0; k < width; k++)
                            pixvalue[j][k] = 0;
                    }
                }

                for (int j = 0; j < prnFileHeader.Colors; j++)
                {
                    fs.read((char*)linedata, linesize);
                    if (j < 4)
                    {
                        for (int m = 0; m < width; m++)
                        {
                            int dots = BitCount2(linedata_16[m]);
                            pixvalue[j][m] += dots;
                            pinkcounts[j] += dots;
                        }
                    }
                    else
                    {
                        int dots = CalcDots(linedata, linesize);
                        pinkcounts[j] += dots;
                    }
                }

                if (((i % YScale) == (YScale - 1)) || (i == (prnFileHeader.Height - 1)))
                {
                    for (int k = 0; k < width; k++)
                    {
                        rawImage[rawpos + k * 4 + 0] = pixvalue[1][k] >= pixmaxv ? 255 : (pixvalue[1][k]);
                        rawImage[rawpos + k * 4 + 1] = pixvalue[2][k] >= pixmaxv ? 255 : (pixvalue[2][k]);
                        rawImage[rawpos + k * 4 + 2] = pixvalue[3][k] >= pixmaxv ? 255 : (pixvalue[3][k]);
                        rawImage[rawpos + k * 4 + 3] = pixvalue[0][k] >= pixmaxv ? 255 : (pixvalue[0][k]);
                    }
                    rawpos += rawStride;
                }

                if ((i & 0x3f) == 0x3f)
                {
                    if (percent)
                        *percent = i * 100 / prnFileHeader.Height;
                    if (update)
                        update();
                }
            }
        };

        auto func32 = [&]() {
            auto linedata = buf.get();
            uint32_t* linedata_32 = (uint32_t*)linedata;

            int pixmaxv = XScale * YScale;
            int pixmaxdiv = pixmaxv / 256;
            int xunit = XScale / 32;

            int rawpos = 0;
            for (int i = 0; i < prnFileHeader.Height; i++)
            {
                if ((i % YScale) == 0)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        for (int k = 0; k < width; k++)
                            pixvalue[j][k] = 0;
                    }
                }

                for (int j = 0; j < prnFileHeader.Colors; j++)
                {
                    fs.read((char*)linedata, linesize);
                    if (j < 4)
                    {
                        for (int m = 0; m < width; m++)
                        {
                            int dots = 0;
                            for (int k = 0; k < xunit; k++)
                                dots += BitCount4(linedata_32[m * xunit + k]);
                            pixvalue[j][m] += dots;
                            pinkcounts[j] += dots;
                        }
                    }
                    else
                    {
                        int dots = CalcDots(linedata, linesize);
                        pinkcounts[j] += dots;
                    }
                }

                if (((i % YScale) == (YScale - 1)) || (i == (prnFileHeader.Height - 1)))
                {
                    for (int k = 0; k < width; k++)
                    {
                        rawImage[rawpos + k * 4 + 0] = pixvalue[1][k] >= pixmaxv ? 255 : (pixvalue[1][k] / pixmaxdiv);
                        rawImage[rawpos + k * 4 + 1] = pixvalue[2][k] >= pixmaxv ? 255 : (pixvalue[2][k] / pixmaxdiv);
                        rawImage[rawpos + k * 4 + 2] = pixvalue[3][k] >= pixmaxv ? 255 : (pixvalue[3][k] / pixmaxdiv);
                        rawImage[rawpos + k * 4 + 3] = pixvalue[0][k] >= pixmaxv ? 255 : (pixvalue[0][k] / pixmaxdiv);
                    }
                    rawpos += rawStride;
                }

                if ((i & 0x3ff) == 0x3ff)
                {
                    if (percent)
                        *percent = i * 100 / prnFileHeader.Height;
                    if (update)
                        update();
                }
            }
        };

        if (XScale == 8)
            func8();
        else if (XScale == 16)
            func16();
        else
            func32();

        if (percent)
            *percent = 100;
        if (update)
            update();
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

