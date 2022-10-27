using MaterialDesignThemes.Wpf;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Ink;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;

namespace PrnViewer
{
    internal class PrnFileHeader
    {
        public int SizeHeader = 0;//指定文件头的字节数；
        public int Colors = 0;//指定除白色和光油外的颜色个数；
        public int UseWhite = 0;//1代表使用白色，0代表不使用白色
        public int UseVarnish = 0;//1代表使用光油，0代表不使用光油
        public int Width = 0;//图象象素宽度；
        public int Height = 0;//图象象素高度；
        public int BitPerPixel = 0;//指定每个象素有几位数据来表达；
        public int GrayLevel = 0;//象素灰度等级；
        public int ResolutionX = 0; //指定图象X方向精度
        public int ResolutionY = 0; //指定图象Y方向精度
        public int SizeImageLow = 0;  //指定图象所有字节数的低32位
        public int SizeImageHigh = 0; //指定图象所有字节数的高32位
        public int BytesPerLine = 0;//指定每行每个颜色字节数；
        public int BiDirection = 0;//1代表双向，0代表单向
        public int Pass = 0;//指定此时文件的PASS数，1，2，3，4，6，8，16，32；
        public int Version = 0;//软件版本号
        public int Manufacturer = 0;//制造商

        override public string ToString()
        {
            double w_inch = Width * 1.0 / ResolutionX;
            double h_inch = Height * 1.0 / ResolutionY;
            return String.Format("颜色数:{0}\r\n宽:{1:F3}mm({2:F1}inch)\r\n高:{3:F3}mm({4:F1}inch)\r\nXDpi:{5}\r\nYDpi:{6}\r\n", Colors,
                w_inch*25.4, w_inch,
                h_inch*25.4, h_inch,
                ResolutionX, ResolutionY);
        }

        public void InitFromBytes(byte[] data)
        {
            if(data == null)
                return;
            if (data.Length <= 68)
                return;
            SizeHeader = BitConverter.ToInt32(data, 0);
            Colors = BitConverter.ToInt32(data, 4*1);
            UseWhite = BitConverter.ToInt32(data, 4*2);
            UseVarnish = BitConverter.ToInt32(data, 4 * 3);
            Width = BitConverter.ToInt32(data, 4 *4);
            Height = BitConverter.ToInt32(data, 4 *5);
            BitPerPixel = BitConverter.ToInt32(data, 4 *6);
            GrayLevel = BitConverter.ToInt32(data, 4 *7);
            ResolutionX = BitConverter.ToInt32(data, 4 *8);
            ResolutionY = BitConverter.ToInt32(data, 4 *9);
            SizeImageLow = BitConverter.ToInt32(data, 4 *10);
            SizeImageHigh = BitConverter.ToInt32(data, 4 * 11);
            BytesPerLine = BitConverter.ToInt32(data, 4 * 12);
            BiDirection = BitConverter.ToInt32(data, 4 * 13);
            Pass = BitConverter.ToInt32(data, 4 * 14);
            Version = BitConverter.ToInt32(data, 4 * 15);
            Manufacturer = BitConverter.ToInt32(data, 4 * 16);
        }

        internal bool IsOk()
        {
            return SizeHeader == 116;
        }
    }

    internal class PrnFileReader
    {
        static public PrnFileHeader ReadHeader(string filename)
        {
            PrnFileHeader prnFileHeader = new PrnFileHeader();
            using (FileStream fileStream = new FileStream(filename, FileMode.Open))
            {
                BinaryReader br = new BinaryReader(fileStream);
                var headerdata = br.ReadBytes(120);
                prnFileHeader.InitFromBytes(headerdata);
            }
            return prnFileHeader;
        }
        static public void ReadInkCount(string filename, Action<Int64[]> update)
        {
            using (FileStream fileStream = new FileStream(filename, FileMode.Open))
            {
                PrnFileHeader prnFileHeader = new PrnFileHeader();
                BinaryReader br = new BinaryReader(fileStream);
                var headerdata = br.ReadBytes(120);
                prnFileHeader.InitFromBytes(headerdata);
                if (!prnFileHeader.IsOk())
                    return ;

                Int64[] inkcounts = new Int64[prnFileHeader.Colors];

                for(int i = 0; i < prnFileHeader.Height; i++)
                {
                    for(int j = 0; j < prnFileHeader.Colors; j++)
                    {
                        var linedata = br.ReadBytes(prnFileHeader.BytesPerLine);
                        int dots = CalcDots(linedata);
                        inkcounts[j] += dots;
                    }
                    if((i & 0x3ff) == 0x3ff)
                    {
                        update?.Invoke(inkcounts);
                    }
                }
                update?.Invoke(inkcounts);
            }
        }

        private static int CalcDots(byte[] linedata)
        {
            int dots = 0;
            int count = (linedata.Length) / 4;
            for(int i = 0; i < count; i++)
            {
                int v = BitConverter.ToInt32(linedata, i*4);
                dots += BitCount4(v);
            }
            return dots;
        }

        static int BitCount4(int n)
        {
            n = (n & 0x55555555) + ((n >> 1) & 0x55555555);
            n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
            n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f);
            n = (n & 0x00ff00ff) + ((n >> 8) & 0x00ff00ff);
            n = (n & 0x0000ffff) + ((n >> 16) & 0x0000ffff);

            return n;
        }
        static int BitCount2(short v)
        {
            int n = v;
            n = (n & 0x5555) + ((n >> 1) & 0x5555);
            n = (n & 0x3333) + ((n >> 2) & 0x3333);
            n = (n & 0x0f0f) + ((n >> 4) & 0x0f0f);
            n = (n & 0x00ff) + ((n >> 8) & 0x00ff);
            return n;
        }

        internal static void CalcBitmap(string filename, Action<int , int , double , double , PixelFormat , Array , int > update)
        {
            using (FileStream fileStream = new FileStream(filename, FileMode.Open))
            {
                PrnFileHeader prnFileHeader = new PrnFileHeader();
                BinaryReader br = new BinaryReader(fileStream);
                var headerdata = br.ReadBytes(120);
                prnFileHeader.InitFromBytes(headerdata);

                if(!prnFileHeader.IsOk())
                    return;

                int width = (prnFileHeader.Width +15)/ 16;
                int height = (prnFileHeader.Height + 15) / 16;
                int XDpi = prnFileHeader.ResolutionX / 16;
                int YDpi = prnFileHeader.ResolutionY / 16;

                var pf = PixelFormats.Cmyk32;
                var rawStride = (width * pf.BitsPerPixel + 7) / 8;
                var rawImage = new byte[rawStride * height];

                byte[] []pixvalue = { new byte[width], new byte[width], new byte[width], new byte[width] };

                int rawpos = 0;
                for (int i = 0; i < prnFileHeader.Height; i++)
                {
                    if((i & 0xf) == 0)
                    {
                        for(int j = 0; j < 4; j++)
                        {
                            for (int k = 0; k < width; k++)
                                pixvalue[j][k] = 0;
                        }
                    }

                    for (int j = 0; j < prnFileHeader.Colors; j++)
                    {
                        var curline = br.ReadBytes(prnFileHeader.BytesPerLine);
                        if (j < 4)
                        {
                            for (int m = 0; m < width; m++)
                            {
                                int sum = pixvalue[j][m] + BitCount2(BitConverter.ToInt16(curline, m * 2));
                                pixvalue[j][m] = sum > 255 ? (byte)255 : (byte)sum;
                            }
                        }
                    }

                    if (((i & 0xf) == 0xf) || (i == (prnFileHeader.Height-1)))
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

                    if((i & 0x3fff) == 0x3fff)
                    {
                        update(width, height,
                                XDpi, YDpi, pf,
                                rawImage, rawStride);
                    }
                }
                update(width, height,
                        XDpi, YDpi, pf,
                        rawImage, rawStride);
            }
        }
    }

    internal class CPrnProcss
    {
        [DllImport("PrnDataProcess.dll", CallingConvention= CallingConvention.Cdecl)]
        static extern void ReadPrnInkCount(string filename, IntPtr inkcounts, Action action);

        static public void ReadInkCount(string filename, Action<Int64[]> update)
        {
            var header = PrnFileReader.ReadHeader(filename);
            if (!header.IsOk())
                return ;

            Int64[] inkcounts = new Int64[header.Colors];
            for (int i = 0; i < inkcounts.Length; i++)
                inkcounts[i] = 0;

            var pinkcounts = Marshal.AllocHGlobal(inkcounts.Length*8);
            Marshal.Copy(inkcounts, 0,  pinkcounts, inkcounts.Length);
            ReadPrnInkCount(filename, pinkcounts, () => {
                Marshal.Copy(pinkcounts, inkcounts, 0, inkcounts.Length);
                update?.Invoke(inkcounts);
            });
        }


        [DllImport("PrnDataProcess.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern void CalcPrnImage(string filename, IntPtr rawImage, Action action);

        internal static void CalcBitmap(string filename, Action<int, int, double, double, PixelFormat, Array, int> update)
        {
            var prnFileHeader = PrnFileReader.ReadHeader(filename);
            if (!prnFileHeader.IsOk())
                return ;

            int width = (prnFileHeader.Width + 15) / 16;
            int height = (prnFileHeader.Height + 15) / 16;
            int XDpi = prnFileHeader.ResolutionX / 16;
            int YDpi = prnFileHeader.ResolutionY / 16;

            var pf = PixelFormats.Cmyk32;
            var rawStride = (width * pf.BitsPerPixel + 7) / 8;
            var rawImage = new byte[rawStride * height];

            var ptr = Marshal.AllocHGlobal(rawStride * height);
            CalcPrnImage(filename, ptr, () => { 

            Marshal.Copy(ptr, rawImage, 0, rawStride * height);
            update?.Invoke(width, height,
                    XDpi, YDpi, pf,
                    rawImage, rawStride);
            });
        }
    }
}
