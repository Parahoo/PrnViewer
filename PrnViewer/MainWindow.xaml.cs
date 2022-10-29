using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace PrnViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            AppSetting.Default = AppSetting.Load();
            plTextBox.Text = AppSetting.Default.Pl.ToString();
            filepathTextBox.Text = AppSetting.Default.PrnFilePath;
            calcGrid.Visibility = Visibility.Collapsed;
        }

        PrnFileHeader header = new();
        Int64[] inkcounts = Array.Empty<Int64>();
        private void openButton_Click(object sender, RoutedEventArgs e)
        {
            string oldpath = filepathTextBox.Text;
            
            
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.InitialDirectory = oldpath;
            ofd.Filter = "Prn文件|*.prn";
            if(ofd.ShowDialog() == true)
            {
                filepathTextBox.Text = ofd.FileName;
                AppSetting.Default.PrnFilePath = filepathTextBox.Text;
                AppSetting.Default.Save();

                header = PrnFileReader.ReadHeader(ofd.FileName);
                if(!header.IsOk())
                {
                    MessageBox.Show(this, "Prn文件格式错误");
                    return;
                }
                img.Source = null;
                UpdatePrnInfo();

                Task.Run(() =>{
                    Dispatcher.Invoke(() =>
                    {
                        calcGrid.Visibility = Visibility.Visible;
                        calcProgressBar.Value = 0;
                        calcProgressBar.IsIndeterminate = true;
                    });

                    CPrnProcss.CalcBitmapAndInkDots(ofd.FileName,
                        (int pixelWidth, int pixelHeight, double dpiX, double dpiY, PixelFormat pixelFormat, Array pixels, int stride) =>
                        {
                            Dispatcher.BeginInvoke(() =>
                            {
                                if (img.Source == null)
                                {
                                    WriteableBitmap bitmap = new(WriteableBitmap.Create(pixelWidth, pixelHeight,
                                        dpiX, dpiY, pixelFormat, null,
                                        pixels, stride));
                                    img.Source = bitmap;
                                }
                                else
                                {
                                    WriteableBitmap bitmap = img.Source as WriteableBitmap;
                                    var rect = new Int32Rect(0, 0, pixelWidth, pixelHeight);
                                    bitmap.WritePixels(rect, pixels, stride, 0);
                                }
                            });
                        },
                        (Int64[] v) => {
                            inkcounts = v;
                            Dispatcher.BeginInvoke(() =>
                            {
                                UpdatePrnInfo();
                            });
                        },
                        (int percent) =>{
                            Dispatcher.BeginInvoke(() =>
                            {
                                calcProgressBar.Value = percent;
                            });
                        });
                    Dispatcher.Invoke(() =>{
                        calcGrid.Visibility = Visibility.Collapsed;
                        calcProgressBar.IsIndeterminate = false;

                    });
                });
            }
        }

        private void UpdatePrnInfo()
        {

            try
            {
                string info = "";
                if(header != null)
                    info += header.ToString() + "\r\n";

                if (inkcounts != null)
                {
                    string[] defaultinkname = { "K", "C", "M", "Y" };
                    for (int i = 0; i < inkcounts.Length; i++)
                    {
                        string inkname = ((i < 4) ? defaultinkname[i] : ("颜色" + (i + 1)));
                        double inkml = CaclInkml(inkcounts[i]);
                        info += string.Format("{0} : {1} dots => {2:F4} ml\r\n", inkname, inkcounts[i], inkml);
                    }
                }

                prninfoTextBlock.Text = info;
            }
            catch (Exception)
            {
            }
        }

        private double CaclInkml(Int64 v)
        {
            try
            {
                double pl = Convert.ToDouble(plTextBox.Text);
                return v * pl * 1e-9;
            }
            catch (Exception)
            {
                return 0;
            }
        }

        private void inkCalcButton_Click(object sender, RoutedEventArgs e)
        {
            UpdatePrnInfo();
        }

        private void plTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            try
            {

                AppSetting.Default.Pl = Convert.ToDouble(plTextBox.Text);
                AppSetting.Default.Save();
            }
            catch (Exception)
            {
            }
        }
    }
}
