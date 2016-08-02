using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Threading;
using System.Windows.Forms;

namespace SD_Reader
{
    public partial class Form1 : Form
    {
        iTestBench tb;
        public Form1()
        {
            InitializeComponent();
      
            bgwTestBench.RunWorkerAsync();
            bgwGetValues.RunWorkerAsync();

        }

        private delegate void dAddWrite(double value);
        private delegate void dAddRead(double value);

        private delegate void dStartTB(int size, bool file);


        public void AddWrite(double value)
        {
            if (this.chart1.InvokeRequired)
            {
                dAddWrite d = new dAddWrite(AddWrite);
                this.chart1.Invoke(d, value);
            }
            else
            {

                try
                {
                    chart1.Series["sWrite"].Points.AddY(value);

                }
                catch (Exception e)
                {
                    Trace.WriteLine(e.Message + Environment.NewLine + e.StackTrace);
                    throw e;
                }
            }
        }

        public void AddRead(double value)
        {
            if (this.chart1.InvokeRequired)
            {
                dAddRead d = new dAddRead(AddRead);
                this.chart1.Invoke(d, value);
            }
            else
            {
                chart1.Series["sRead"].Points.AddY(value);
            }
        }

        private void btnRun_Click(object sender, EventArgs e)
        {
            bgwTestBench.RunWorkerAsync();
        }


        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void bgwTestBench_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            Trace.WriteLine("RunWorkerCompleted : " + e.Cancelled + "\t Error :" + e.Error, "BGW");
        }

        private void bgwTestBench_DoWork(object sender, DoWorkEventArgs e)
        {
            Trace.WriteLine("DoWork", "BGW");
            tb = new UnmanagedTestbench();
            Trace.WriteLine("tb initialized");
            tb.AddWrite(AddWrite);
            tb.AddRead(AddRead);
            tb.BlockSize = 4096 * 2;
            tb.Start("XXXXX", false,true ,2);
            Trace.WriteLine("End", "BGW");
        }

        private void bgwGetValues_DoWork(object sender, DoWorkEventArgs e)
        {
            bool run = true;
            while (run)
            {
                lock (UnmanagedTestbench.qRead)
                if (UnmanagedTestbench.qRead.Count > 0)
                    {
                        AddRead(UnmanagedTestbench.qRead.Dequeue());
                    }
                lock (UnmanagedTestbench.qWrite)
                if (UnmanagedTestbench.qWrite.Count > 0)
                    {
                        AddWrite(UnmanagedTestbench.qWrite.Dequeue());
                    }
                Thread.Sleep(1);
            }
        }
    }
}
