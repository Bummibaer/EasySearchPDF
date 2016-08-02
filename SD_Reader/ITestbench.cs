using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SD_Reader
{
    interface iTestBench
    {
        // Fields
        UInt32 BlockSize { get; set; }  // DWORD !!
        Int64 FileSize { get; set; }

        int Debug { get; set; }
        void AddWrite(Action<double> addWrite);
        void AddRead(Action<double> addRead);

        void Start(String filetoWrite, bool file, bool verify, int anz);

        void Start();
        void WriteFiles();
    }
}
