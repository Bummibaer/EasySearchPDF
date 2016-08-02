using System;
using System.Collections.Generic;
using System.Text;

using System.Management;

using System.IO;
using System.Diagnostics;
using System.Windows.Forms;

using CommandLine.Utility;

namespace SD_Reader
{
    class Program
    {
        static void Main(string[] args)
        {

            int debug = 0;
            int iCount = 1;
            bool bWriteFile = false;
            bool bVerify = false;
            string name = "MICRON";

            Arguments arg = new Arguments(args);

            Stopwatch sw = new Stopwatch();
            Trace.Listeners.Add(new TextWriterTraceListener(Console.Out));
            Trace.AutoFlush = true;

            PhysicalDrive pd = new PhysicalDrive();
            /*
            pd.TestRAID();
            Environment.Exit(0);
            */

            arg.ParseCommandLine();
            if (arg["h"] != null)
            {
                Console.WriteLine("Usage : lwg");
                Environment.Exit(0);
            }
            if (arg["d"] != null)
            {
                debug = int.Parse(arg["d"]);
                Trace.WriteLine("Debug = " + debug);
            }
            if (arg["n"] != null)
            {
                name = arg["n"];
            }
            if (arg["v"] != null)
            {
                bVerify = true;
            }
            if (arg["c"] != null)
            {
                iCount = int.Parse(arg["c"]);
            }
            if (arg["f"] != null)
            {
                bWriteFile = true;
            }
            if ( arg["w"] != null )  {
                iTestBench tb = new UnmanagedTestbench();
                tb.WriteFiles();
            }
            if (arg["t"] != null)
            {
                //iTestBench tb = new UnmanagedFileLoader();
                iTestBench tb = new UnmanagedTestbench();
                Trace.WriteLine("tb initialized");
                //tb.AddWrite(AddWrite);
                //tb.AddRead(AddRead);
                tb.Debug = debug;
                Trace.WriteLine("-------------  Unmanaged Test -------------------------");
                tb.FileSize = 2 << 21;
                tb.Start(name ,bWriteFile, bVerify, iCount);
                Trace.WriteLine("End", "BGW");
                Environment.Exit(0);
            }
            int offset = 0;
            if (arg["o"] != null)
            {
                 offset = int.Parse(arg["o"]);
            }
                if (arg["r"] != null)
            {
                int id = int.Parse(arg["r"]);
                pd.ReadFirstBlock(id , offset);
                Environment.Exit(0);
            }

            if (arg["l"] != null)
            {
                pd.GetAllVolumeDeviceIDs(true);
                pd.GetAllDiskDriveDeviceIDs(true);
                pd.Close();
            }

            if (arg["g"] != null)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new Form1());

            }

        }

    }
}
