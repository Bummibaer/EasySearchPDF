using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace SD_Reader
{

    class UnmanagedTestbench : iTestBench
    {
        public const short FILE_ATTRIBUTE_NORMAL = 0x80;
        public const short INVALID_HANDLE_VALUE = -1;
        public const uint GENERIC_READ = 0x80000000;
        public const uint GENERIC_WRITE = 0x40000000;
        public const uint CREATE_NEW = 1;
        public const uint CREATE_ALWAYS = 2;
        public const uint OPEN_EXISTING = 3;

        const int cMaxFiles = 16;

        [StructLayout(LayoutKind.Sequential)]
        internal struct sTiming
        {
            internal double dReadElapsed;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = cMaxFiles)]
            internal double[] dWriteElapsed;
        };

        const String dll = @"D:\Netz\VSProjects\SSD_TEST\x64\Debug\CopySpecialFiles.dll";

        [DllImport(dll, SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern void RegisterCallbacks(
            MyCallback write_callback,
            MyCallback read_callback
          );

        [DllImport(dll)]
        public static extern void setBlockSize(UInt32 value);

        [DllImport(dll)]
        public static extern void setFileSize(Int64 value);

        [DllImport(dll)]
        public static extern void setDebug(int value);

        [DllImport(dll)]
        public static extern void setRawFile(bool value);

        [DllImport(dll)]
        public static extern void setNegate(bool value);

        [DllImport(dll)]
        public static extern void setOverride(bool value);

        [DllImport(dll)]
        public static extern void testSSDRAID();

        [DllImport(dll, SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool copyFiletoRAW(ref sTiming timing,
                    int anz,
                   String[] fromPath
                   );


        [DllImport(dll, SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool VerifyFile(string filename);


        public delegate void MyCallback(double time);

        private Action<double> aWrite, aRead;

        public static Queue<double> qWrite = new Queue<double>();
        public static Queue<double> qRead = new Queue<double>();

        private static Boolean bGraphic = false;
        public static void addWrite(double time)
        {
            if (bGraphic)
                lock (qWrite)
                {
                    qWrite.Enqueue(time);
                }
        }
        public static void addRead(double time)
        {
            if (bGraphic)
                lock (qRead)
                {
                    qRead.Enqueue(time);
                }
        }

        static MyCallback cbWrite = new MyCallback(addWrite);
        static MyCallback cbRead = new MyCallback(addRead);
        public static void RegisterEvent()
        {
            RegisterCallbacks(
              cbWrite,
               cbRead
              );

        }

        private int debug = 0;

        UInt32 blockSize = 4096;
        Int64 fileSize = (Int64)((Int64)2 << 30);
        public UInt32 BlockSize
        {
            get
            {
                return blockSize;
            }
            set
            {
                blockSize = value;
                setBlockSize(value);
            }
        }
        public Int64 FileSize
        {
            get
            {
                return fileSize;
            }
            set
            {
                fileSize = value;
            }
        }

        public int Debug
        {
            get
            {
                return debug;
            }

            set
            {
                debug = value;
                setDebug(value);
            }
        }

        public void AddWrite(Action<double> addWrite)
        {
            aWrite = addWrite;
        }

        public void AddRead(Action<double> addRead)
        {
            aRead = addRead;
        }

        public String GetFile(Int64 fileSize)
        {

            Microsoft.VisualBasic.Devices.ComputerInfo ci = new Microsoft.VisualBasic.Devices.ComputerInfo();
            Trace.WriteLine(
                 ci.AvailablePhysicalMemory + "\t" +
                 ci.AvailableVirtualMemory + "\t" +
                 ci.TotalPhysicalMemory + "\t" +
                 ci.TotalVirtualMemory
                 );
            ulong ulAvailablePhysicalMemory = ci.AvailablePhysicalMemory;

            byte[] bytes = new byte[2 << 16];


            if (!File.Exists("test" + FileSize + ".bin"))
            {
                FileStream fs = new FileStream("test" + fileSize + ".bin", FileMode.Create);
                UInt64 value = 0;
                Int64 bytesWritten = 0;
                do
                {
                    for (long i = 0; i < BlockSize; i += 8)
                    {
                        byte[] ibytes = BitConverter.GetBytes(value);
                        ibytes.CopyTo(bytes, i);
                        value++;
                    }
                    fs.Write(bytes, 0, bytes.Length);
                    bytesWritten += bytes.Length;
                } while (bytesWritten < fileSize);

                fs.Close();
                Trace.WriteLine("File written");
            }
            return "test" + fileSize + ".bin";
        }

        public void WriteFiles()
        {
            testSSDRAID();
        }
        public void Start()
        {
            sTiming timing = new sTiming();
            int fileAnz = 0;
            string[] files = new string[2];
            copyFiletoRAW(ref timing, fileAnz, files);
        }
        public void Start(string fileToWrite, bool bFile, bool bVerify, int fileAnz)
        {
            String[] files;
            Stopwatch sw = new Stopwatch();
            Microsoft.VisualBasic.Devices.ComputerInfo ci = new Microsoft.VisualBasic.Devices.ComputerInfo();
            ulong ulAvailablePhysicalMemory = ci.AvailablePhysicalMemory;
            using (FileStream fs = new FileStream("test.log", FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                TextWriter tw = new StreamWriter(fs);

                if (bFile)
                {
                    files = new String[cMaxFiles];
                    for (int i = 0; i < fileAnz; i++)
                    {
                        files[i] = "test" + i + ".bin";
                    }
                }
                else

                {
                    PhysicalDrive pd = new PhysicalDrive();
                    files = pd.GetSafePhysicalDevices(fileToWrite, FileAccess.ReadWrite);
                    if (files.Length == 0)
                    {
                        Trace.WriteLine("Could not find any Device, or it is not safe : " + fileToWrite);
                        Environment.Exit(4);
                    }
                    foreach (String file in files) Trace.WriteLine("Got " + file + " with FileSize : "  + pd.DDevices[file].size);
                }

                RegisterEvent();
                UInt64 fileSize = (UInt64)((UInt64)1 << 34); // (long)((long)1 << 20); // 960194511360;  // Hardcoded could come from pd !
                              // 1917995466240 RAID of two
                setFileSize(FileSize);
                setRawFile(!bFile);
                //setOverride(true);

                int bestexp = (int)Math.Log((double)fileSize, 2.0);
                UInt64 from64 = (UInt64)((UInt64)1 << (bestexp - 4));
                UInt64 to64 = (UInt64)((UInt64)1 << (bestexp - 1));
                UInt32 to,from; 

                Trace.WriteLine("AvailablePhysicalMemory : " + ulAvailablePhysicalMemory.ToString("X"));
                Trace.WriteLine("UIN32.max/2 = " +  (UInt32.MaxValue / 2).ToString("X"));
                from = (UInt32)from64;
                if (from64 > ulAvailablePhysicalMemory / 2)
                {
                    from64 = (uint)(ulAvailablePhysicalMemory / 2);
                    if (from64 >= UInt32.MaxValue / 2)
                    {
                        from = UInt32.MaxValue / 2;
                    }
                }
                to = (UInt32)to64;
                if (to64 > ulAvailablePhysicalMemory / 2)
                {
                    to64 = (uint)(ulAvailablePhysicalMemory / 2);
                    if (to64 >= UInt32.MaxValue / 2)
                    {
                        to = UInt32.MaxValue / 2;
                    }
 
                }
                if ( to < from )
                {
                    Trace.WriteLine("to < from !");
                    from = to;
                }
                Trace.WriteLine("-------------  Unmanaged Test -------------------------");
                Trace.WriteLine("Loop BlockSize from " + from.ToString("X") + "\tto : " + to.ToString("X") );
                Trace.WriteLine("FileSize " + ((double)FileSize / (double)1E9));


                tw.WriteLine("-------------  Unmanaged Test -------------------------");
                tw.WriteLine("Loop BlockSize from " + from + "\tto : " + to + "\tto64 : " + to64);
                tw.WriteLine("FileSize " + ((double)FileSize / (double)1E9));
                tw.WriteLine("----------------------------------------------------------");
                tw.WriteLine("Bytes written GB | BlockSize Bytes | Time sec | Speed GB/s");
                //                            123456789012345678901234567890123456789012345678901234567890
                //                                     1         2         3         4         5
                Trace.WriteLine("Bytes written GB | BlockSize Bytes | Time sec | Speed GB/s");
                Trace.WriteLine("----------------------------------------------------------");
                for (UInt32 bs = from; bs <= to; bs <<= 1)

                {
                    if (debug > 0) Trace.WriteLine("Start with fileSize =" + fileSize + "\tblockSize = " + bs.ToString("X"), "UNMANAGED TB");
                    setBlockSize(bs);
                    sTiming timing = new sTiming();
                    sw.Reset();
                    sw.Start();
                    bool rc = copyFiletoRAW(ref timing, fileAnz, files);
                    sw.Stop();
                     Trace.WriteLine("Run " + sw.ElapsedMilliseconds + "ms");
                    if (!rc)
                    {
                        Trace.WriteLine("Could not open Files !");
                        Environment.Exit(2);
                    }
                    StringBuilder result = new StringBuilder(String.Format(
                        "{0,17}|{1,17}",
                        FileSize / 1E9,
                        bs));
                    for (int i = 0; i < fileAnz; i++)
                    {
                        result.Append(string.Format("|{0,8:#0.00000}|{1,10:#0.0000000}",
                        timing.dWriteElapsed[i],
                        FileSize / 1E9 / timing.dWriteElapsed[i]
                        ));
                    }
                    Trace.WriteLine(result);
                    tw.WriteLine(result);
                    if (bVerify)
                    {
                        for (int i = 0; i < fileAnz; i++)
                        {
                            VerifyFile(files[i]);
                        }
                    }
                    if (debug > 0) Trace.WriteLine("Next Round", "UNMANAGED TB");
                }
                tw.Close();
            }

        }
    }
}