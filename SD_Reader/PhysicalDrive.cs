using de.fraunhofer.ipms.SNDiverses;
using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace SD_Reader
{
    class PhysicalDrive
    {

        //function import
        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        static extern SafeFileHandle CreateFile(
            string fileName,
            [MarshalAs(UnmanagedType.U4)] FileAccess fileAccess,
            [MarshalAs(UnmanagedType.U4)] FileShare fileShare,
            IntPtr securityAttributes,
            [MarshalAs(UnmanagedType.U4)] FileMode creationDisposition,
            int flags,
            IntPtr template);

        internal void Close()
        {
            // throw new NotImplementedException();
        }

        internal struct sDevice
        {
            internal string name;
            internal string model;

            internal UInt64 size;
            internal int partitions;
            internal int BytesperSector;
            internal FileStream fs;

            public FileStream Fs(FileMode fa)
            {
                fs = new FileStream(name, fa);
                return fs;
            }


        }
        int debug = 0;
        private Dictionary<string, Dictionary<string, string>> dVolumes;
        private Dictionary<string, sDevice> dDevices;

        public object Integer { get; private set; }

        public Dictionary<string, Dictionary<string, string>> DVolumes
        {
            get
            {
                return dVolumes;
            }

            set
            {
                dVolumes = value;
            }
        }

        internal Dictionary<string, sDevice> DDevices
        {
            get
            {
                return dDevices;
            }

            set
            {
                dDevices = value;
            }
        }

        public PhysicalDrive()
        {
            DDevices = new Dictionary<string, sDevice>();
            dVolumes = new Dictionary<string, Dictionary<string, string>>();
            GetAllDiskDriveDeviceIDs(false);
            GetAllVolumeDeviceIDs(false);
        }

        public String GetSafePhysicalDevice(String name)
        {

            foreach (string drives in DDevices.Keys)
            {
                if (DDevices[drives].model.IndexOf(name) >= 0)
                {
                    Trace.WriteLine("Found : Drive" + drives + DDevices[drives].model, "GetSafePhysicalDevice");
                    int partitions = DDevices[drives].partitions;
                    if (partitions > 0)
                    {
                        throw new AccessViolationException("DiskDrive has " + partitions + " Partitions");
                    }
                    return DDevices[drives].name;
                }
            }
            Trace.WriteLine("No Device found", "GetSafePhysicalDevice");
            return null;
        }
        public String[] GetSafePhysicalDevices(String name)
        {
            List<String> lStrings = new List<string>();
            foreach (string drives in DDevices.Keys)
            {
                if (DDevices[drives].model.IndexOf(name) >= 0)
                {
                    Trace.WriteLine("Found : Drive" + drives + DDevices[drives].model, "OpenSafePhysicalDevice");
                    int partitions = DDevices[drives].partitions;
                    if (partitions > 0)
                    {
                        throw new AccessViolationException("DiskDrive has " + partitions + " Partitions");
                    }
                    lStrings.Add(DDevices[drives].name);
                }
            }
            return lStrings.ToArray();
        }

        public FileStream OpenPhysicalDrive(string iDevice, FileAccess access)
        {
            Console.WriteLine(@"Try to open \\{0}", iDevice);
            SafeFileHandle drive = CreateFile(fileName: iDevice,
                         fileAccess: access,
                         fileShare: FileShare.Write | FileShare.Read | FileShare.Delete,
                         securityAttributes: IntPtr.Zero,
                         creationDisposition: FileMode.Open,
                         flags: 0, //with this also an enum can be used. (as described above as EFileAttributes)
                         template: IntPtr.Zero);
            string errorMessage = new Win32Exception(Marshal.GetLastWin32Error()).Message;
            Console.WriteLine("MESSAGE:" + errorMessage);

            if (drive.IsInvalid)
            {
                Trace.WriteLine("Unable to access drive. Win32 Error Code " + Marshal.GetLastWin32Error());
                if (Marshal.GetLastWin32Error() == 5)
                {
                    Trace.WriteLine(">>>> Try with administration rights !! >>>>>>>>>>>>>>>>>>>>>>>>", "INFO");
                }
                Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());

            }

            return new FileStream(drive, access);
        }

        //function to write
        private void writeToDisk(string lpFileName, byte[] dataToWrite)
        {
            if (dataToWrite == null) throw new System.ArgumentException("dataToWrite parameter cannot be null!");

            SafeFileHandle drive = CreateFile(fileName: lpFileName,
                             fileAccess: FileAccess.Write,
                             fileShare: FileShare.Write | FileShare.Read | FileShare.Delete,
                             securityAttributes: IntPtr.Zero,
                             creationDisposition: FileMode.Open,
                             flags: 4, //with this also an enum can be used. (as described above as EFileAttributes)
                             template: IntPtr.Zero);

            FileStream diskStreamToWrite = new FileStream(drive, FileAccess.Write);
            diskStreamToWrite.Write(dataToWrite, 0, dataToWrite.Length);
            try
            {
                diskStreamToWrite.Close();
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace, "FATAL");

            }
            try
            {
                drive.Close();
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace, "FATAL");
            }


        }

        public string GetPhyicalName(int iDevice)
        {
            return @"\\.\PHYSICALDRIVE" + iDevice;
        }
        public void ReadFirstBlock(int iDevice, long offset)
        {
            String sDevice = GetPhyicalName(iDevice);
            Trace.WriteLine("---------------- Open " + sDevice + "------------------------------------", "INFO");

            using (FileStream fs = OpenPhysicalDrive(GetPhyicalName(iDevice), FileAccess.Read))
            {
                BinaryReader br = null;
                try
                {
                    br = new BinaryReader(fs);
                    fs.Seek(offset, SeekOrigin.Begin);
                    Trace.WriteLine("Position :" + fs.Position, "FILESTREAM");
                    for (int i = 0; i < 3; i++)
                    {
                        byte[] bytes = br.ReadBytes(512);
                        Trace.WriteLine("Read " + bytes.Length);
                        Helpers.DumpBytes(bytes);
                    }
                    br.Close();
                }
                catch (Exception e)
                {
                    Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace);

                }
                finally
                {
                    if (br != null)
                    {
                        br.Close();
                    }
                }
            }

            Trace.WriteLine("");
            Trace.WriteLine("Done!");
        }

        enum eDriveType
        {

            Unknown = 0,
            NoRootDirectory = 1,
            RemovableDisk = 2,
            LocalDisk = 3,
            NetworkDrive = 4,
            CompactDisk = 5,
            RAMDisk = 6
        }
        public void GetAllVolumeDeviceIDs(bool bWrite)
        {
            dVolumes = new Dictionary<string, Dictionary<string, string>>();

            String MOUNTPOINT_DIRS_KEY = "MountPoint: ";


            // retrieve information from Win32_Volume
            try
            {
                using (ManagementClass volClass = new ManagementClass("Win32_Volume"))
                {
                    using (ManagementObjectCollection mocVols = volClass.GetInstances())
                    {
                        // iterate over every volume
                        foreach (ManagementObject moVol in mocVols)
                        {

                            // get the volume's device ID (will be key into our dictionary)                            
                            string devId = moVol.GetPropertyValue("DeviceID").ToString();
                            // ReadFirstBlock(devId);

                            dVolumes.Add(devId, new Dictionary<string, string>());

                            if (debug > 0) Trace.WriteLine(String.Format("Vol: {0}", devId));

                            // for each non-null property on the Volume, add it to our Dictionary<string,string>
                            foreach (PropertyData p in moVol.Properties)
                            {
                                if (debug > 0) Trace.Write("\"" + p.Name + "\";");
                            }
                            if (debug > 0) Trace.WriteLine("");
                            foreach (PropertyData p in moVol.Properties)
                            {
                                String txt;
                                if (p.Value == null)
                                {
                                    txt = String.Format("\"{0," + p.Name.Length + "}\";", " ");
                                }
                                else
                                {
                                    if (p.IsArray)
                                    {
                                        Type[] type = Type.GetTypeArray(new object[] { p.Value });
                                        StringBuilder v = new StringBuilder();
                                        switch (p.Type)
                                        {
                                            case CimType.UInt16:
                                                foreach (UInt16 value in (UInt16[])p.Value)
                                                {
                                                    v.Append(value.ToString() + ',');
                                                }
                                                break;
                                            case CimType.String:
                                                foreach (String value in (String[])p.Value)
                                                {
                                                    v.Append(value.ToString() + ',');
                                                }
                                                break;
                                            default:
                                                v.Append(p.Type.ToString());
                                                break;
                                        }

                                        // Object[] values = p.Value;
                                        dVolumes[devId].Add(p.Name, v.ToString());

                                        txt = "\"" + CenterString(v.ToString(), p.Name.Length) + "\";";
                                    }
                                    else
                                    {
                                        dVolumes[devId].Add(p.Name, p.Value.ToString());
                                        txt = "\"" + CenterString(p.Value.ToString(), p.Name.Length) + "\";";
                                    }
                                }
                                if (debug > 0) Trace.Write(txt);
                            }
                            if (debug > 0) Trace.WriteLine("");

                            // find the mountpoints of this volume
                            using (ManagementObjectCollection mocMPs = moVol.GetRelationships("Win32_Volume"))
                            {
                                foreach (ManagementObject moMP in mocMPs)
                                {
                                    // only care about adding directory
                                    // Directory prop will be something like "Win32_Directory.Name=\"C:\\\\\""
                                    string dir = moMP["Directory"].ToString();

                                    // find opening/closing quotes in order to get the substring we want
                                    int first = dir.IndexOf('"') + 1;
                                    int last = dir.LastIndexOf('"');
                                    string dirSubstr = dir.Substring(first, last - first);

                                    // use GetFullPath to normalize/unescape any extra backslashes
                                    string fullpath = Path.GetFullPath(dirSubstr);
                                    if (debug > 0) Trace.WriteLine(dir + "\t" + fullpath);
                                    dVolumes[devId].Add(MOUNTPOINT_DIRS_KEY, fullpath);

                                }
                            }
                        }
                        if (bWrite) WriteWMI("WIN32_Volume", dVolumes);
                    }
                }
            }
            catch (Exception ex)
            {
                Trace.WriteLine(String.Format("Problem retrieving Volume information from WMI. {0} - \n{1}", ex.Message, ex.StackTrace));
            }

            if (bWrite)
            {
                foreach (string device in new List<string>(dVolumes.Keys))
                {
                    int ei = int.Parse(dVolumes[device]["DriveType"]);
                    string e = Enum.GetName(typeof(eDriveType), ei);

                    Trace.WriteLine(String.Format("{0}\t{1}\tFilesystem :{2}\tBootvolume? {3}\tBlockSize : {4}\tCapacity : {5,15} bytes\t{6}",
                    device,
                    dVolumes[device].ContainsKey("DriveLetter") ? dVolumes[device]["DriveLetter"] : "none",
                    dVolumes[device].ContainsKey("FileSystem") ? dVolumes[device]["FileSystem"] : "none",
                    dVolumes[device].ContainsKey("BootVolume") ? dVolumes[device]["BootVolume"] : " none ",
                     dVolumes[device].ContainsKey("BlockSize") ? dVolumes[device]["BlockSize"] : "none",
                     dVolumes[device].ContainsKey("Capacity") ? dVolumes[device]["Capacity"] : "none",
                      e
                    ));
                }
            }

        }

        public void GetAllDiskDriveDeviceIDs(bool list)
        {
            Dictionary<string, Dictionary<string, string>> dDiskDrives = new Dictionary<string, Dictionary<string, string>>();

            // retrieve information from Win32_Volume
            try
            {
                using (ManagementClass volClass = new ManagementClass("Win32_Diskdrive"))
                {
                    using (ManagementObjectCollection mocVols = volClass.GetInstances())
                    {
                        // iterate over every volume
                        foreach (ManagementObject moVol in mocVols)
                        {

                            string devId = moVol.GetPropertyValue("DeviceID").ToString();
                            int Id = int.Parse(moVol.GetPropertyValue("Index").ToString());

                            if (!list)
                            {
                                sDevice device = new sDevice();
                                device.name = moVol.GetPropertyValue("Name").ToString();
                                device.model = moVol.GetPropertyValue("Model").ToString();
                                if (!int.TryParse(moVol.GetPropertyValue("Partitions").ToString(), out device.partitions))
                                {
                                    throw new ArgumentException("Could not Get Partitions");
                                }

                                if (!ulong.TryParse(moVol.GetPropertyValue("Size").ToString(), out device.size))
                                {
                                    throw new ArgumentException("Could not Get Size");
                                }
                                if (!int.TryParse(moVol.GetPropertyValue("BytesPerSector").ToString(), out device.BytesperSector))
                                {
                                    throw new ArgumentException("Could not Get Size");
                                }

                                dDevices.Add(devId, device);
                            }
                            // get the volume's device ID (will be key into our dictionary)                            
                            // ReadFirstBlock(devId);

                            dDiskDrives.Add(devId, new Dictionary<string, string>());

                            if (debug > 1) Trace.WriteLine(String.Format("Vol: {0}", devId));

                            // for each non-null property on the Volume, add it to our Dictionary<string,string>
                            foreach (PropertyData p in moVol.Properties)
                            {
                                if (debug > 1) Trace.Write("\"" + p.Name + "\";");
                            }
                            if (debug > 1) Trace.WriteLine("");
                            foreach (PropertyData p in moVol.Properties)
                            {
                                String txt;
                                if (p.Value == null)
                                {
                                    txt = String.Format("\"{0," + p.Name.Length + "}\";", " ");
                                }
                                else
                                {
                                    if (p.IsArray)
                                    {
                                        Type[] type = Type.GetTypeArray(new object[] { p.Value });
                                        StringBuilder v = new StringBuilder();
                                        switch (p.Type)
                                        {
                                            case CimType.UInt16:
                                                foreach (UInt16 value in (UInt16[])p.Value)
                                                {
                                                    v.Append(value.ToString() + ',');
                                                }
                                                break;
                                            case CimType.String:
                                                foreach (String value in (String[])p.Value)
                                                {
                                                    v.Append(value.ToString() + ',');
                                                }
                                                break;
                                            default:
                                                v.Append(p.Type.ToString());
                                                break;
                                        }

                                        // Object[] values = p.Value;
                                        dDiskDrives[GetPhyicalName(Id)].Add(p.Name, v.ToString());

                                        txt = "\"" + CenterString(v.ToString(), p.Name.Length) + "\";";
                                    }
                                    else
                                    {
                                        dDiskDrives[GetPhyicalName(Id)].Add(p.Name, p.Value.ToString());
                                        txt = "\"" + CenterString(p.Value.ToString(), p.Name.Length) + "\";";
                                    }
                                }
                                if (debug > 1) Trace.Write(txt);
                            }
                            if (debug > 1) Trace.WriteLine("");

                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Trace.WriteLine(String.Format("Problem retrieving Volume information from WMI. {0} - \n{1}", ex.Message, ex.StackTrace));
            }


            WriteWMI("WIN32_DiskDrive", dDiskDrives);
            if (list)
            {
                List<string> devices = new List<string>(dDiskDrives.Keys);
                devices.Sort();
                foreach (string device in devices)
                {
                    Trace.WriteLine(String.Format("{0}\tPartitions :{1}\t{2,35}\t{3,20}\t{4} Bytes\t{5}",
                    dDiskDrives[device].ContainsKey("Name") ? dDiskDrives[device]["Name"] : "none",
                    dDiskDrives[device].ContainsKey("Partitions") ? dDiskDrives[device]["Partitions"] : "none",
                    dDiskDrives[device].ContainsKey("Model") ? dDiskDrives[device]["Model"] : " none ",
                    dDiskDrives[device].ContainsKey("MediaType") ? dDiskDrives[device]["MediaType"] : "none",
                    dDiskDrives[device].ContainsKey("Size") ? dDiskDrives[device]["Size"] : "Unknown",
                    dDiskDrives[device].ContainsKey("CapabilityDescriptions") ? dDiskDrives[device]["CapabilityDescriptions"] : "none"
                    ));
                }
            }


        }

        private void WriteWMI(String kind, Dictionary<string, Dictionary<string, string>> ret)
        {
            List<string> devices = new List<string>(ret.Keys);
            StringBuilder[] text = new StringBuilder[ret[devices[0]].Keys.Count + 1];
            List<string> properties = new List<string>(ret[devices[0]].Keys);
            devices.Sort();
            text[0] = new StringBuilder(";");
            TextWriter tw = File.CreateText(kind + ".csv");
            foreach (string device in devices)
            {
                text[0].Append("\"" + device + "\";");
                if (device == devices[devices.Count - 1])
                {
                    tw.WriteLine(text[0]);
                }
                for (int index = 0; index < properties.Count; index++)
                {
                    if (device == devices[0]) text[index + 1] = new StringBuilder("\"" + properties[index] + "\";");
                    if (ret[device].ContainsKey(properties[index]))
                    {
                        text[index + 1].Append("\"" + ret[device][properties[index]] + "\";");
                    }
                    else
                    {
                        text[index + 1].Append("\"" + "-" + "\";");
                    }
                    if (device == devices[devices.Count - 1])
                    {
                        tw.WriteLine(text[index + 1]);
                        //Trace.WriteLine(text[index + 1]);
                    }
                }
            }
            tw.Close();
        }

        public void TestStructureSSD()
        {
            String files = GetSafePhysicalDevice("MICRON");
            ASCIIEncoding ascii = new ASCIIEncoding();
            Random r = new Random();

            Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
            Trace.WriteLine("Culture " + Thread.CurrentThread.CurrentCulture.TwoLetterISOLanguageName);
            using ( FileStream fs = OpenPhysicalDrive(files,FileAccess.ReadWrite))
            {
                Trace.WriteLine("CanSeek : " + fs.CanSeek + "\tCanWrite " + fs.CanWrite);
                
                try {
                    long seek = 0;
                    int rand = 0;
                    Byte[] bytes = ascii.GetBytes("Write to " + seek.ToString("X"));
                    Trace.WriteLine("Write to " + seek.ToString("X") + " " + rand);
                    fs.Write(bytes, 0, bytes.Length);
                    fs.Flush();
                    seek = 512;
                    rand = r.Next(256);
                    bytes = ascii.GetBytes("Write to " + seek.ToString("X") + " " + rand);
                    Trace.WriteLine("Write to " + seek.ToString("X") + " " + rand + "\t" + (seek+rand));
                    fs.Seek(seek + rand, SeekOrigin.Current);
                    fs.Write(bytes, 0, bytes.Length);

                    seek = 1024;
                    rand = r.Next(256);
                    bytes = ascii.GetBytes("Write to " + seek.ToString("X") + " " + rand);
                    Trace.WriteLine("Write to " + seek.ToString("X") + " " + rand);
                    fs.Seek(seek + rand, SeekOrigin.Begin);
                    fs.Write(bytes, 0, bytes.Length);

                    seek = (long)1 << 16;
                    rand = r.Next(256);
                    bytes = ascii.GetBytes("Write to " + seek.ToString("X") + " " + rand);
                    Trace.WriteLine("Write to " + seek.ToString("X") + " " + rand);
                    fs.Seek(seek + rand, SeekOrigin.Begin);
                    fs.Write(bytes, 0, bytes.Length);

                    seek = 0x1BE919FFFF0;
                    rand = 0;
                    bytes = ascii.GetBytes("Write to " + seek.ToString("X") + " " + rand);
                    Trace.WriteLine("Write to " + seek.ToString("X") + " " + rand);
                    fs.Seek(seek + rand, SeekOrigin.Begin);
                    fs.Write(bytes, 0, bytes.Length);
                } catch ( IOException ie)
                {
                    Trace.WriteLine(ie.Message + Environment.NewLine + ie.StackTrace);
                    throw ie;
                } catch ( Exception e )
                {
                    Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace);
                    throw e;
                }

            }
        }

        public void TestRAID()
        {
            String[] files = GetSafePhysicalDevices("MICRON");
            FileStream[] fs = { dDevices[files[0]].fs, dDevices[files[0]].fs };
            int count = 0;
            UInt32 oldValue = 0;
            foreach (FileStream f in fs)
            {
                Trace.WriteLine("Read " + f.Name);
                using (BinaryReader br = new BinaryReader(f))
                {
                    byte[] bytes = new byte[512];
                    int block = 0;
                    while ((count = br.Read(bytes, 0, 512)) > 0)
                    {
                        UInt32 values = (UInt32)(bytes[0] << 24) |
                                        (UInt32)(bytes[1] << 16) |
                                        (UInt32)(bytes[2] << 8) |
                                        (UInt32)(bytes[3]);
                        if (oldValue != values)
                        {
                            Trace.WriteLine("Block " + block + "\t" + oldValue + "\t->\t" + values);
                        }
                        oldValue = values + 1;
                        block++;
                        if (values == 0xFFFFFFFF)
                        {
                            break;
                        }
                    }
                }
            }
        }


        private String CenterString(String txt, int length)
        {
            double dTextHalf = txt.Length / 2.0;
            double dWidthHalf = length / 2.0;
            double dPad = dWidthHalf - dTextHalf;
            if (txt.Length > length) dTextHalf = length / 2.0;
            return txt.PadLeft((int)Math.Floor(dPad) + txt.Length).PadRight(length);
        }



    }
}

