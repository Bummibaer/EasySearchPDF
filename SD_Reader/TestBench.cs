using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SD_Reader
{
    public class TestBench : iTestBench
    {

        private enum eIO
        {
            WRITE,
            READ
        };

        private double swFrequency = (double)Stopwatch.Frequency;
        private double write_elapsed_time = 0.0;
        private double read_elapsed_time = 0.0;

        private double write_bps_average = 0.0;
 
        private int write_lock = -1;

        private const double cMBScale = 1E6;
        private const int cBuffers = 2;

        private Byte[][] bytes = new byte[cBuffers][];
        private Int64 offset = 0;


        private FileStream mfsPhysicalDevice;

        private AutoResetEvent[] eDataAvailable;
        private AutoResetEvent[] eDataWritten;

        private Action<double> aWrite, aRead;

        private TextWriter tw;

        private int debug = 0;

        private bool writeCSV = false;

        private int iFileAnz;

        public void AddWrite(Action<double> addWrite)
        {
            aWrite = addWrite;
        }

        public void AddRead(Action<double> addRead)
        {
            aRead = addRead;
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
        UInt32 u32BlockSize;
        public UInt32 BlockSize
        {
            get
            {
                return u32BlockSize;
            }

            set
            {

                System.Diagnostics.Debug.Assert((value > 0) && (value < int.MaxValue), "TESTBENCH: Caution ! Filesize may not greater than " + int.MaxValue);
                u32BlockSize = value;
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
            }
        }

        public bool WriteCSV
        {
            get
            {
                return writeCSV;
            }

            set
            {
                writeCSV = value;
            }
        }

        Int64 fileSize = (int)1E7;
  

        public TestBench()
        {
            eDataAvailable = new AutoResetEvent[2];
            eDataWritten = new AutoResetEvent[2];

            if (writeCSV)
            {
                try
                {
                    tw = File.CreateText("write.csv");
                    //  elapsed_ticks,write_time, write_elapsed_time, byte_write_time, byte_write_average_time
                    tw.WriteLine("\"elapsed_time\",\"write_elapsed_time\",\"bytes_per_second\",\"bytes_per_second_average\"");
                }
                catch (Exception e)
                {
                    Trace.WriteLine("Could not open write.csv, open yet?");
                    Trace.WriteLine(e.Message);
                    Environment.Exit(1);
                }
            }
            for (int i = 0; i < cBuffers; i++)
            {
                eDataAvailable[i] = new AutoResetEvent(false);
                eDataWritten[i] = new AutoResetEvent(true);     // Inititialize buffers
            }
        }

        public FileStream GetFile(long size)
        {

            if (!File.Exists("test" + FileSize + ".bin"))
            {
                FileStream fs = new FileStream("test" + size + ".bin", FileMode.Create);
                for (int i = 0; i < size; i++)
                {
                    fs.WriteByte((byte)i);
                }
                fs.Close();
                Trace.WriteLine("File written");
            }
            return new FileStream("test" + size + ".bin", FileMode.Open);
        }

        public void Start()
        {

        }
        public void Start(string fileToWrite, bool bFile, bool bVerify, int FileAnz)
        {
            iFileAnz = FileAnz;
            for (int i=0;i<cBuffers;i++) bytes[i] = new Byte[u32BlockSize];

            if (bFile)
                mfsPhysicalDevice = File.Create("test0.bin");
            else
            {
                PhysicalDrive pd = new PhysicalDrive();
                string[] physycaldevices = pd.GetSafePhysicalDevices(fileToWrite, FileAccess.ReadWrite);

                mfsPhysicalDevice = pd.DDevices[physycaldevices[0]].Fs(FileMode.Create) ;
                if ( mfsPhysicalDevice == null )
                {
                    Trace.WriteLine("Could not find any Device, or it is not safe : " + fileToWrite);
                    Environment.Exit(4);
                }
            }

            read_elapsed_time = 0.0;
            write_elapsed_time = 0.0;
            offset = 0;
            using (mfsPhysicalDevice)
            {

                // mfsPhysicalDevice = new FileStream("test_write.txt", FileMode.Create);

/*  // for Reading Files, not yet
        if (bFile)
                {
                    Thread rt = new Thread(ReadThread);
                    Thread wt = new Thread(WriteThread);
                    rt.Name = "Read Thread";
                    rt.Priority = ThreadPriority.Highest;
                    wt.Name = "Write Thread";
                    wt.Priority = ThreadPriority.Highest;
                    rt.Start();
                    wt.Start();
                }
                else
                {
                */
                    Thread wt = new Thread(WriteByteThread);
                    wt.Name = "Write Thread";
                    wt.Priority = ThreadPriority.Highest;
                    wt.Start();

                    while (wt.IsAlive)
                    {
                        Thread.Sleep(100);
                    }

                    if (tw != null) tw.Close();
               //  }
            }
            Trace.WriteLine("Testbench Ended");
        }

        private void WriteByteThread()
        {
            Task<double> t;
            int value = 0;
            int read_buffer = 0;
            Stopwatch sw = new Stopwatch();
            long start, stop;
            double elapsed_in_sec;

            for (long i = 0; i < u32BlockSize; i += 4)
            {
                bytes[0][i] = (byte)((value >> 24) & 0xFF);
                bytes[0][i + 1] = (byte)((value >> 16) & 0xFF);
                bytes[0][i + 2] = (byte)((value >> 8) & 0xFF);
                bytes[0][i + 3] = (byte)((value) & 0xFF);
                value++;
            }

            long last = 0;
            while (last < FileSize )
            {
                sw.Start();
                start = sw.ElapsedTicks;
                if (debug > 1) Trace.WriteLine("read_bufer = "+ read_buffer);
                t = DoWrite(bytes[read_buffer]);
                for (long i = 0; i < u32BlockSize; i += 4)
                {
                    bytes[read_buffer][i] = (byte)((value >> 24) & 0xFF);
                    bytes[read_buffer][i + 1] = (byte)((value >> 16) & 0xFF);
                    bytes[read_buffer][i + 2] = (byte)((value >> 8) & 0xFF);
                    bytes[read_buffer][i + 3] = (byte)((value) & 0xFF);
                    
                    value++;
                }
                last += u32BlockSize;
                 t.Wait();
                stop = sw.ElapsedTicks ;
                sw.Stop();
                elapsed_in_sec = (double)(stop - start) / swFrequency;

                CalcTiming(elapsed_in_sec, read_buffer);
                read_buffer++;
                if (read_buffer == (cBuffers)) read_buffer = 0;
            }

            StringBuilder result = new StringBuilder(String.Format(
                 "{0,17}|{1,17}",
                 FileSize / 1E9,
                 u32BlockSize));
            for (int i = 0; i < iFileAnz; i++)
            {
                result.Append(string.Format("|{0,12:#0.000000000}|{1,10:##0.0000000000}",
                write_elapsed_time, write_bps_average / cMBScale
                ));
            }
            Trace.WriteLine(result);
        }

        private void WriteThread()
        {
            Task<double> t;
            int write_buffer = 0;
            bool rc = false;
            rc = eDataAvailable[write_buffer].WaitOne(1000);
            while (true)
            {
                if (rc)
                {
                    int actual_write_buffer = write_buffer;
                    t = DoWrite((byte[])bytes[write_buffer]);
                    write_buffer++;
                    if (write_buffer == cBuffers)
                    {
                        write_buffer = 0;
                    }
                    rc = eDataAvailable[write_buffer].WaitOne(1000);
                    t.Wait();
                    CalcTiming(t.Result, actual_write_buffer);
                }
                else
                {
                    Trace.WriteLine("End of Write !", "WRITE");
                    break;
                }
            }

            Trace.WriteLine("End of Thread", "WRITE");
        }

        public void WriteFiles()
        {

        }
        private async Task<double> DoWrite(byte[] bytes)
        {
            try
            {
                if (debug > 1) Trace.WriteLine("Write bl=" + bytes.Length + "\t bs = " + u32BlockSize, "DOWRITE");
                await mfsPhysicalDevice.WriteAsync(bytes, 0, (int)u32BlockSize);
            }
            catch (Exception e)
            {

                Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace) ;
                throw e;
            }
            return 0.0;
        }

        private void CalcTiming(double time_in_sec, int buffer)
        {

            write_elapsed_time += time_in_sec;
            offset += u32BlockSize;

            double bytes_per_second = u32BlockSize / time_in_sec;
            write_bps_average = offset / write_elapsed_time;

            if (debug > 0)
                Trace.WriteLine(
                 "Write from " + buffer +
                 "\t Time Last Write: " + time_in_sec * 1000.0 +
                 "ms\t Last Write: " + bytes_per_second / cMBScale +
                 "MByte/sec\t Average  : " + write_bps_average / cMBScale +
                 "MByte/sec", "WRITE"
            );
           //  aWrite(bytes_per_second / cMBScale);
            if (writeCSV)
            {
                tw.WriteLine("\"{0}\",\"{1}\",\"{2}\",\"{3}\"", time_in_sec, write_elapsed_time, bytes_per_second, write_bps_average);
                tw.Flush();
            }

        }
        private async void ReadThread()
        {
            Stopwatch sw = new Stopwatch();
            double elapsed = 0;
            double time = 0;
            double elapsed_speed = 0.0;

            int read_buffer = 0;
            bool rc = true;
            bool run = true;
            sw.Start();

            using (FileStream fs = GetFile(fileSize))
            {
                try
                {
                    while (run && (rc = eDataWritten[read_buffer].WaitOne(1000)))
                    {
                        write_lock = read_buffer;
                        long start = sw.ElapsedTicks;
                        int x = await fs.ReadAsync(bytes[read_buffer], 0, (int)u32BlockSize);
                        long stop = sw.ElapsedTicks;

                        write_lock = -1;
                        long elapsed_ticks = stop - start;
                        elapsed = elapsed_ticks / swFrequency;
                        eDataAvailable[read_buffer].Set();
                        time += elapsed;
                        elapsed_speed = u32BlockSize / elapsed * 1000.0;
                        read_elapsed_time = offset / time * 1000.0;
                        Trace.WriteLine("Read to  " + read_buffer + "\t Last Read: " + elapsed_speed + "ms\t Elapsed: " + read_elapsed_time + "ms", "READ");
                        aRead(elapsed_speed);
                        if (x < u32BlockSize)
                        {
                            Trace.WriteLine("End of Read !", "Read");
                            run = false;
                            break;
                        }
                        offset += u32BlockSize;
                        read_buffer++;
                        if (read_buffer == cBuffers)
                        {
                            read_buffer = 0;
                        }
                    }
                }
                catch (Exception e)
                {
                    Trace.WriteLine("Read_Buffer = " + read_buffer);
                    Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace);
                    throw e;
                }
            }
            Trace.WriteLine("End of Thread", "READ");

        }

    }
}
