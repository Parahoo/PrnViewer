using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace PrnViewer
{
    [Serializable]
    public class AppSetting
    {
        public double Pl { get; set; } = 12;

        public string PrnFilePath { get; set; } = "";

        public static AppSetting Default { get; set; } = new AppSetting();

        public static AppSetting Load()
        {
            try
            {
                using (StreamReader fs = new StreamReader("appsetting.xml", Encoding.UTF8))
                {
                    XmlSerializer xmlSerializer = new XmlSerializer(typeof(AppSetting));
                    return xmlSerializer.Deserialize(fs) as AppSetting;
                }
            }
            catch
            {
                return new AppSetting();
            }
        }
        public void Save()
        {
            lock (this)
            {
                try
                {
                    using (StreamWriter fs = new StreamWriter("appsetting.xml", false, Encoding.UTF8))
                    {
                        XmlSerializer xmlSerializer = new XmlSerializer(typeof(AppSetting));
                        xmlSerializer.Serialize(fs, this);
                    }
                }
                catch
                {

                }
            }
        }
    }
}
