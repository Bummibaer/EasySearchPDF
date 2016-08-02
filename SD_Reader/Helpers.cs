using System;
using System.Threading;
using System.Text;

using System.Diagnostics;

using System.Net;
using System.Net.Mail;
using System.Net.Mime;
using System.ComponentModel;

using System.IO;

namespace de.fraunhofer.ipms.SNDiverses
{
    /// <summary>
    /// General class containing helper methods and stuff like that
    /// </summary>

    static class Helpers
    {
        private struct TimeStamp
        {
            public UInt64 seconds;
            public UInt64 ms;
        };
        /// <summary>
        /// Writes the colored Message to Console.
        /// </summary>
        /// <param name="msg">The message(without NL).</param>
        /// <param name="bg">The background color.</param>
        public static void WriteColored(String msg, ConsoleColor bg)
        {
            ConsoleColor oldBg = System.Console.BackgroundColor;
            ConsoleColor oldFg = System.Console.ForegroundColor;
            System.Console.BackgroundColor = bg;
            System.Console.ForegroundColor = ConsoleColor.White;
            System.Console.Write(msg);
            System.Console.BackgroundColor = oldBg;
            System.Console.ForegroundColor = oldFg;
            System.Console.Write("\n");

        }
        /// <summary>
        /// Writes the colored Message to Console.
        /// </summary>
        /// <param name="msg">The message (without NL).</param>
        /// <param name="bg">background color.</param>
        /// <param name="fg">foreground color.</param>
        public static void WriteColored(String msg, ConsoleColor bg, ConsoleColor fg)
        {
            ConsoleColor oldFg = System.Console.ForegroundColor;
            ConsoleColor oldBg = System.Console.BackgroundColor;
            System.Console.ForegroundColor = fg;
            System.Console.BackgroundColor = bg;
            System.Console.Write(msg);
            System.Console.BackgroundColor = oldBg;
            System.Console.ForegroundColor = oldFg;
            System.Console.Write("\n");

        }
        /// <summary>
        /// returns  count '#' .
        /// </summary>
        /// <param name="count">count.</param>
        public static string GetHashes(long count)
        {
            StringBuilder hashes = new StringBuilder();
            for (int i = 0; i < count; i++) hashes.Append('#');
            return hashes.ToString();
        }
        /// <summary>
        /// Wait till any key is pressed and continue with program execution
        /// </summary>
        /// <returns></returns>
        public static bool WaitTillAnyKeyIsPressed()
        {
            while (!System.Console.KeyAvailable)
            {
                Thread.Sleep(1);
            }
            System.Console.ReadKey();
            return true;
        }

        /// <summary>
        /// Converts a string into a byte array
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static byte[] StrToByte(string str)
        {
            byte[] temp = new byte[str.Length];
            int x;
            for (x = 0; x < str.Length; ++x)
            {
                temp[x] = (byte)str[x];
            }
            return temp;
        }

        /// <summary>
        /// Trys to finds all occurances of pattern in array 
        /// </summary>
        /// <param name="array"></param>
        /// <param name="pattern"></param>
        /// <returns>0 or less if not found, 1 or more elements</returns>
        public static int ArrayFindAllPatterns(byte[] array, byte[] pattern)
        {
            Object LockFunc = new Object();
            int founds = 0;
            bool found = false;
            lock (LockFunc)
            {
                if (array == null || pattern == null)
                {
                    return 0;
                }
                else if (array.Length == 0)
                {
                    return 0;
                }
                else if (pattern.Length == 0)
                {
                    return 0;
                }
                else if (array.Length < pattern.Length)
                {
                    return 0;
                }
                else
                {
                    for (int i = 0; i < array.Length; i++)
                    {
                        if (array[i] == pattern[0])
                        {
                            found = true;
                            for (int j = 1; j < pattern.Length; j++)
                            {
                                if ((j + i) >= array.Length)
                                {
                                    found = false;
                                    break;
                                }
                                if ((pattern[j] != array[j + i]))
                                {
                                    found = false;
                                    break;
                                }
                            }
                            if (found) { founds++; }
                        }
                    }
                    return founds;
                }
            }
        }

        /// <summary>
        /// Removes every occurance "pattern" from array and returns resized array
        /// Caution: There is a bug when byte is 0xFF
        /// </summary>
        /// <returns></returns>
        public static byte[] ArrayRemoveArrayPattern(byte[] array, byte[] pattern)
        {
            string haystack = System.Text.Encoding.Default.GetString(array);
            string needle = System.Text.Encoding.Default.GetString(pattern);
            int index = 0;
            while (haystack.Contains(needle))
            {
                index = haystack.IndexOf(needle);
                haystack = haystack.Remove(index, pattern.Length);
            }
            byte[] arrayout = System.Text.Encoding.Default.GetBytes(haystack);
            if (arrayout == null)
            {
                return new byte[] { 0 };
            }
            else
            {
                return arrayout;
            }
        }

        /// <summary>
        /// try to find a needle in an array haystack, needle is smaller than haystack
        /// </summary>
        /// <param name="arrayhaystack"></param>
        /// <param name="needle"></param>
        /// <returns></returns>
        public static bool FindArrayPatternInArray(byte[] arrayhaystack, byte[] needle)
        {
            Object LockFunc = new Object();
            lock (LockFunc)
            {
                if (arrayhaystack == null || needle == null)
                {
                    return false;
                }
                if (arrayhaystack.Length == 0)
                {
                    return false;
                }
                else if (needle.Length == 0)
                {
                    return false;
                }
                else if (arrayhaystack.Length < needle.Length)
                {
                    return false;
                }
                else
                {
                    for (int i = 0; i < arrayhaystack.Length; i++)
                    {
                        if (arrayhaystack[i] == needle[0])
                        {
                            bool found = true;
                            for (int j = 1; j < needle.Length; j++)
                            {
                                if ((j + i) >= arrayhaystack.Length)
                                {
                                    return false;
                                }
                                if ((needle[j] != arrayhaystack[j + i]))
                                {
                                    found = false;
                                    break;
                                }
                            }
                            if (found) { return true; }
                        }
                    }
                    return false;
                }
            }
        }

        /// <summary>
        /// Cuts a Part from out of an array and returns the cutted array
        /// </summary>
        /// <param name="array"></param>
        /// <param name="startpos"></param>
        /// <param name="stoppos"></param>
        /// <returns></returns>
        public static byte[] ArrayCut(byte[] array, int startpos, int endpos)
        {
            byte[] tmp = new byte[endpos - startpos + 1];
            int j = 0;
            for (int i = startpos; i <= endpos; i++)
            {
                tmp[j] = array[i];
                j++;
            }

            return tmp;
        }
        /// <summary>
        /// removes given fields from array
        /// </summary>
        /// <param name="array"></param>
        /// <param name="startpos"></param>
        /// <param name="stoppos"></param>
        /// <returns></returns>
        public static byte[] ArrayRemoveAPart(byte[] array, int startpos, int endpos)
        {
            byte[] tmplow = new byte[startpos];
            byte[] tmphigh = new byte[array.Length - endpos];
            for (int i = 0; i < startpos; i++)
            {
                tmplow[i] = array[i];
            }
            int j = 0;
            for (int i = endpos + 1; i < array.Length; i++)
            {
                tmphigh[j] = array[i];
                j++;
            }
            return Helpers.MergeArrays(tmplow, tmphigh);
        }

        public static string TimeStr_min_s_ms()
        {
            return "Time: " + System.DateTime.Now.Minute + "min" +
                    " " + System.DateTime.Now.Second + "sec" +
                    " " + System.DateTime.Now.Millisecond + "ms";
        }

        /// <summary>
        /// Wait 
        /// </summary>
        /// <param name="key"> The Key to be pressed, this function shouldn't be</param>
        /// <param name="sleeptime">Sleeptime in ms</param>
        public static void WaitTillKeyPressed(char key, int sleeptime)
        {
            //System.Console.WriteLine("Abort by pressing:" + key);
            while ((char)System.Console.Read() != key)
            {
                Thread.Sleep(System.TimeSpan.FromMilliseconds(sleeptime));//Main should sleep since it is not needed
            }
            System.Console.WriteLine("Pressed: " + key + " >>>End Waiting Till Key pressed");
        }

        public static void Quit()
        {
            System.Console.WriteLine("Quit with q ...");
            while (true)
            {
                if ('q' == (char)System.Console.Read())
                {
                    System.Environment.Exit(-1);
                }
            }
        }

        /// <summary>
        /// Compares two arrays with each other, must be excatly the same (no. of elements, element values)
        /// </summary>
        /// <param name="a">first byte[] array</param>
        /// <param name="TestValue">second byte[] array</param>
        /// <returns>true if the same / false if not the same</returns>
        public static bool ArrayCompare(byte[] a, byte[] b)
        {
            if (a.Length != b.Length)
            {
                return false;
            }
            for (uint i = 0; i < a.Length; i++)
            {
                if (a[i] != b[i])
                {
                    return false;
                }
            }
            return true;//must be the same
        }

        //Merges to arrays, first array goes into first array
        public static byte[] MergeArrays(byte[] a, byte[] b)		//Merge two arrays
        {
            if (a == null && b == null)
            {
                return null;
            }
            else if (a == null)
            {
                return b;
            }
            else if (b == null)
            {
                return a;
            }

            else if (a.Length == 0)
            {
                return b;
            }
            else if (b.Length == 0)
            {
                return a;
            }
            else if ((a.Length == 0) && (b.Length == 0))
            {
                return new byte[] { };
            }
            else
            {
                byte[] c = new byte[a.Length + b.Length];
                Buffer.BlockCopy(a, 0, c, 0, a.Length);
                Buffer.BlockCopy(b, 0, c, a.Length, b.Length);
                return c;
            }
        }
        // returns a byte array of length 4 > ToDo: pruefen
        public static byte intToByte(int i)						//!!!we just use the low part
        {
            byte[] dword = new byte[4];
            dword[0] = (byte)(i & 0x000000FF);
            dword[1] = (byte)((i >> 8) & 0x000000FF);//not used
            dword[2] = (byte)((i >> 16) & 0x000000FF);//not used
            dword[3] = (byte)((i >> 24) & 0x000000FF);//not used
            return dword[0];
        }
        // returns a byte array of length 4 > ToDo: pruefen
        public static byte[] intToByteArray(int i)						//!!!we just use the low part
        {
            byte[] dword = new byte[4];
            dword[0] = (byte)(i & 0x000000FF);
            dword[1] = (byte)((i >> 8) & 0x000000FF);//not used
            dword[2] = (byte)((i >> 16) & 0x000000FF);//not used
            dword[3] = (byte)((i >> 24) & 0x000000FF);//not used
            return dword;
        }

        /// <summary>
        /// Convert Byte Array into Hex String
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static string ToHexStr(byte[] arr)				//outputs a Hex String
        {
            string tmp = "";
            int wrap = 0;
            if (arr == null)
            {
                tmp = "";
            }
            else
            {
                foreach (byte b in arr)
                {
                    tmp = tmp + b.ToString("X2") + " ";				//a byte has maximal 2 hexa digits
                    if ((wrap++ % 16) == 15) tmp += "\r\n";
                }
            }
            return tmp;

        }
        /// <summary>
        /// Convert Byte Array into Hex String
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static string ToHexStr(byte[] arr, int length)				//outputs a Hex String
        {
            string tmp = "";
            if (arr == null)
            {
                tmp = "";
            }
            else
            {
                for (int i = 0; i < length; i++)
                {
                    tmp = tmp + arr[i].ToString("X2") + " ";				//a byte has maximal 2 hexa digits
                }
            }
            return tmp;

        }

        /// <summary>
        /// Convert Short Array into Hex String
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static string ToHexStr(short[] arr, int length)				//outputs a Hex String
        {
            string tmp = "";
            if (arr == null)
            {
                tmp = "";
            }
            else
            {
                for (int i = 0; i < length; i++)
                {
                    tmp = tmp + arr[i].ToString("X2") + " ";				//a byte has maximal 2 hexa digits
                }
            }
            return tmp;

        }

        /// <summary>
        /// Convert a Byte into a HexaNo. and return the string
        /// </summary>
        /// <param name="val"></param>
        /// <returns></returns>
        public static string ToHexStr(byte val)					//convert a byte into a hexa string
        {
            return val.ToString("X");
        }

        /// <summary>
        /// Convert a byte array into an ASCII String
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static string ToAsciiStr(byte[] arr)				//outputs an ASCII String
        {
            return System.Text.Encoding.ASCII.GetString(arr);
        }

        public static string ToPseudoAsciiStr(byte[] arr)				//outputs an ASCII String
        {
            StringBuilder tmp = new StringBuilder("");
            if (arr != null)
                foreach (byte b in arr)
                {
                    if ((b >= 0x20) && (b < 0x80))
                    {
                        tmp.AppendFormat("{0}", (char)b);				//a byte has maximal 2 hexa digits
                    }
                    else
                    {
                        tmp.AppendFormat("|{0:x2}|", b);			//a byte has maximal 2 hexa digits
                    }
                }
            return tmp.ToString();
        }
        public static string ToPseudoAsciiStr(byte[] arr, int length)				//outputs an ASCII String
        {
            StringBuilder tmp = new StringBuilder("");
            if (length != 0)
                for (int i = 0; i < length; i++)
                {
                    byte b = arr[i];
                    if ((b >= 0x20) && (b < 0x80))
                    {
                        tmp.AppendFormat("{0}", (char)b);				//a byte has maximal 2 hexa digits
                    }
                    else
                    {
                        tmp.AppendFormat("|{0:X2}|", b);			//a byte has maximal 2 hexa digits
                    }
                }
            return tmp.ToString();
        }

        public static UInt64 ArrayToUInt(byte[] b, int from, int length)
        {
            int to = from + length - 1;
            UInt64 rc = 0;
            if ((from < 0) || (from > to) || (to > b.Length))
            {
                return 0;
            }
            for (int i = from; i <= to; i++)
            {
                rc += (UInt64)b[i] << ((length - 1) * 8);
                length--;
            }
            return rc;
        }
        /// <summary>
        /// Output a Byte Array as Hexadecimal No.
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static string ToDecStr(byte[] arr)				//outputs a Decimal String
        {
            string tmp = "";
            foreach (byte b in arr)
            {
                tmp = tmp + b.ToString("D") + " ";			//a byte has maximal 3 decimal digits
            }
            return tmp;
        }

        /// <summary>
        /// Output a Byte  as a binary No.
        /// </summary>
        /// <param name="arr"></param>
        /// <returns></returns>
        public static string ToBinString(byte b)				//outputs a binary String
        {
            StringBuilder tmp = new StringBuilder("");
            for (int i = 0; i < 8; i++)
            {
                tmp = tmp.Append((b & 0x80) == 0x80 ? '1' : '0');			//a byte has maximal 3 decimal digits
                b = (byte)(b << 1);
            }
            String s = tmp.ToString();
            return tmp.ToString();
        }

        public static string ToBinString(ushort b)				//outputs a binary String
        {
            StringBuilder tmp = new StringBuilder("");
            for (int i = 0; i < 15; i++)
            {
                tmp = tmp.Append((b & 0x8000) == 0x8000 ? '1' : '0');			//a byte has maximal 3 decimal digits
                b = (ushort)(b << 1);
            }
            String s = tmp.ToString();
            return tmp.ToString();
        }

        static private DateTime UnixEpoch = new DateTime(1970, 1, 1);

        // Ticks are 100ns long 
        static public ulong DateTimeToUTC(DateTime dtCurTime)
        {
            DateTime dtEpochStartTime = UnixEpoch;
            TimeSpan ts = dtCurTime.Subtract(dtEpochStartTime);

            DateTime utc = dtCurTime.ToUniversalTime();

            ulong rc;
            rc = ((ulong)ts.TotalSeconds << 32) | ((ulong)ts.Milliseconds << 16);//

            return rc;
        }
        static public UInt64 NowToMSDTime()
        {
            DateTime dt = DateTime.Now.ToUniversalTime();
            Int32 UnixTime = ToUnixTime(dt);
            return ((UInt64)UnixTime << 32);
        }
        private static byte[] NowToUTCTime()
        {
            UInt64 nowUTC = Helpers.NowToMSDTime();
            Byte[] bnow = new Byte[]
            {
                (byte)((nowUTC >> 56 ) & 0xff),
                (byte)((nowUTC >> 48 ) & 0xff),
                (byte)((nowUTC >> 40 ) & 0xff),
                (byte)((nowUTC >> 32 ) & 0xff),
                (byte)((nowUTC >> 24 ) & 0xff),
                (byte)((nowUTC >> 16 ) & 0xff),
                (byte)((nowUTC >> 8 ) & 0xff),
                (byte)((nowUTC ) & 0xff)
            };
            return bnow;
        }


        /// <summary>
        /// MSDs the time to string.
        /// </summary>
        /// <param name="MSDUTC">The MSDUTC.  Byte 7:4 in Seconds, Byte 3:2 in fractions of Seconds, Byte 1:0 === Zeros </param>
        /// // Warning: In spec is specified a LSB for Year 2038 problem !
        /// <returns></returns>
        static public String KSTTimeToString(UInt64 KSTUTC)
        {
            TimeStamp ts = new TimeStamp();
            ts = KSTTimeToTimeStamp(KSTUTC);
            DateTime UTC = FromUnixTime(ts.seconds);

            return String.Format("{0}-{1}.{2}", UTC.ToShortDateString(), UTC.ToLongTimeString(), ts.ms.ToString());
        }
        /// <summary>
        /// MSDs the time to string.
        /// </summary>
        /// <param name="MSDUTC">The MSDUTC.  Byte 7:4 in Seconds, Byte 3:2 in fractions of Seconds, Byte 1:0 === Zeros </param>
        /// // Warning: In spec is specified a LSB for Year 2038 problem !
        /// <returns></returns>
        static public String MSDTimeToString(UInt64 MSDUTC)
        {
            TimeStamp ts = new TimeStamp();
            ts = MSDTimeToTimeStamp(MSDUTC);
            DateTime UTC = FromUnixTime(ts.seconds);

            return String.Format("{0}-{1}.{2}", UTC.ToShortDateString(), UTC.ToLongTimeString(), ts.ms.ToString());
        }
        /// <summary>
        /// MSDs the time to TimeStamp.
        /// </summary>
        /// <param name="MSDUTC">The MSDUTC.  Byte 7:4 in Seconds, Byte 3:2 in fractions of Seconds, Byte 1:0 === Zeros</param>
        /// <returns></returns>
        /// // Warning: In spec is specified a LSB for Year 2038 problem !
        static private TimeStamp MSDTimeToTimeStamp(UInt64 MSDUTC)
        {
            /*
             * MSDUTC:
             *  Byte 7 6 5 4 3 2 1 0
             * 7..4 UTC Sekunden since 1970
             * 3..2 RTC @ 32xxxkHz
             * 1..0 =0
             * */
            TimeStamp ts = new TimeStamp();
            const double frac2ms = 1000.0 / (UInt16.MaxValue / 2);
            UInt32 frac;
            ts.seconds = (UInt32)(MSDUTC >> 32); // Byte 7..4 -> Sekunden
            frac = (UInt32)(MSDUTC >> 16) & 0x7fff; // Byte 3..2 Fraction in 1/2**15 * 2 ( Bit15..1 + Bit0 = 'b0)
            ts.ms = (UInt16)(frac * frac2ms);

            return ts;
        }
        /// <summary>
        /// KoronarSportTime the time to TimeStamp.
        /// </summary>
        /// <param name="MSDUTC">The MSDUTC.  Byte 7:4 in Seconds, Byte 3:2 in fractions of Seconds, Byte 1:0 === Zeros</param>
        /// <returns></returns>
        /// // Warning: In spec is specified a LSB for Year 2038 problem !
        static private TimeStamp KSTTimeToTimeStamp(UInt64 MSDUTC)
        {
            /*
             * MSDUTC:
             *  Byte 7 6 5 4 3 2 1 0
             * 7..4 UTC Sekunden since 1970
             * 3..2 RTC @ 32xxxkHz
             * 1..0 =0
             * */
            TimeStamp ts = new TimeStamp();
            UInt32 frac;
            ts.seconds = (UInt32)(MSDUTC >> 32); // Byte 7..4 -> Sekunden
            frac = (UInt32)(MSDUTC >> 16) & 0x7fff; // Byte 3..2 Fraction in 1/2**15 * 2 ( Bit15..1 + Bit0 = 'b0)
            ts.ms = (UInt16)(frac);
            return ts;
        }

        /// <summary>
        /// MSDs the time to string.
        /// </summary>
        /// <param name="MSDUTC">The MSDUTC.  Byte 7:4 in Seconds, Byte 3:2 in fractions of Seconds, Byte 1:0 === Zeros </param>
        /// // Warning: In spec is specified a LSB for Year 2038 problem !
        /// <returns></returns>
        static public UInt64 MSDTimeToMS(UInt64 MSDUTC)
        {
            TimeStamp ts = new TimeStamp();
            ts = MSDTimeToTimeStamp(MSDUTC);
            return ts.seconds * 1000L + ts.ms;
        }

        static public UInt64 UTCTimetoMS(UInt64 UTCTime)
        {
            return ((UTCTime >> 32) & 0xffffffff) * 1000 + ((UTCTime >> 16) & 0xffff);
        }
        static public DateTime MSDTimeDateTime(UInt64 MSDUTC)
        {
            DateTime dt = UnixEpoch;
            TimeStamp ts = new TimeStamp();
            ts = MSDTimeToTimeStamp(MSDUTC);
            dt = dt.AddSeconds((double)ts.seconds);
            dt = dt.AddMilliseconds((double)ts.ms);
            return dt;
        }
        static public DateTime UTCtoDateTime(int seconds, int ms)
        {
            DateTime dt = UnixEpoch;
            dt = dt.AddSeconds(seconds);
            dt = dt.AddMilliseconds(ms);
            return dt;
        }

        public static DateTime FromUnixTime(UInt64 unixTime)
        {
            return UnixEpoch.AddSeconds((double)unixTime);
        }

        public static Int32 ToUnixTime(DateTime dateTime)
        {
            TimeSpan timeSpan = dateTime - UnixEpoch;
            return (Int32)timeSpan.TotalSeconds;
        }

        public static void DumpBytes(string name, byte[] bytes)
        {
            Trace.Write("Dump " + name);
            DumpBytes(bytes);
        }
        public static void DumpBytes(byte[] bytes)
        {
            int adr = 0;
            {
                foreach (byte b in bytes)
                {
                    if ((adr % 16) == 0)
                    {
                        Trace.Write(String.Format("{0}{1:x4}: ", System.Environment.NewLine, adr));
                    }
                    Trace.Write(String.Format("{0:x2}|{1} ", b, Char.IsControl((char)b) ? '_' : (char)b));
                    adr++;
                }
            }
            if ((adr % 16) != 15) Trace.WriteLine(System.Environment.NewLine);
        }

        public static void DumpBytes(byte[] bytes, int start, int end)
        {
            for (int adr = start; adr < end; adr++)
            {
                if ((adr % 16) == 0)
                {
                    Trace.Write(String.Format("{0}{1:x4}: ", System.Environment.NewLine, adr));
                }
                Trace.Write(String.Format("{0:x2}|{1} ", bytes[adr],
                    Char.IsControl((char)bytes[adr]) ? '_' : (char)bytes[adr])
                    );

            }
            Trace.WriteLine(System.Environment.NewLine);
        }

        public static String DTtoHHMMSSMMM(DateTime dt)
        {
            return String.Format("{0:d2}.{1:d2}.{2:d2}:{3:d3}",
                dt.Hour, dt.Minute, dt.Second, dt.Millisecond);
        }

        private static void SendCompletedCallback(object sender, AsyncCompletedEventArgs e)
        {
            // Get the unique identifier for this asynchronous operation.
            String token = (string)e.UserState;

            if (e.Cancelled)
            {
                Trace.WriteLine("[{0}] Send canceled.", token);
            }
            if (e.Error != null)
            {
                Trace.WriteLine(String.Format("[{0}] {1}", token, e.Error.ToString()));
            }
            else
            {
                Trace.WriteLine("Message sent.");
            }

        }
        public static void SendMail(string args)
        {
            // Command line argument must the the SMTP host.
            SmtpClient client = new SmtpClient("smtp.ipms.fraunhofer.de");
            // Specify the e-mail sender.
            // Create a mailing address that includes a UTF8 character
            // in the display name.
            MailAddress from = new MailAddress("netz@ipms.fraunhofer.de",
               "Steffen " + (char)0xD8 + " Netz",
            System.Text.Encoding.UTF8);
            // Set destinations for the e-mail message.
            MailAddress to = new MailAddress("steffennetz@freenet.de");
            // Specify the message content.
            MailMessage message = new MailMessage(from, to);
            message.Body = "This is a test e-mail message sent by an application. ";
            // Include some non-ASCII characters in body and subject.
            string someArrows = new string(new char[] { '\u2190', '\u2191', '\u2192', '\u2193' });
            message.Body += Environment.NewLine + someArrows;
            message.BodyEncoding = System.Text.Encoding.UTF8;
            message.Subject = "test message 1" + someArrows;
            message.SubjectEncoding = System.Text.Encoding.UTF8;
            // Set the method that is called back when the send operation ends.
            client.SendCompleted += new
            SendCompletedEventHandler(SendCompletedCallback);
            // The userState can be any object that allows your callback 
            // method to identify this send operation.
            // For this example, the userToken is a string constant.
            string userState = "test message1";
            client.SendAsync(message, userState);
            //    Console.WriteLine("Sending message... press c to cancel mail. Press any other key to exit.");
            //    string answer = Console.ReadLine();
            //    // If the user canceled the send, and mail hasn't been sent yet,
            //    // then cancel the pending operation.
            //    if (answer.StartsWith("c") && mailSent == false)
            //    {
            //        client.SendAsyncCancel();
            //    }
            //    // Clean up.
            //    message.Dispose();
            //    Console.WriteLine("Goodbye.");
            //
        }
        /// <summary>
        /// Erzeugt einen Filenamen , der noch nicht existiert ( hängt eine fortlaufende Zahl an)
        /// </summary>
        /// <param name="text"></param>
        /// <returns>unique FileName</returns>
        public static string CreateLogFile(string text)
        {
            int i = 0;
            while (i < int.MaxValue)
            {
                String FileName = String.Format("{0}.log.{1}", text, i++);
                if (!File.Exists(FileName))
                {
                    return FileName;
                }
            }
            throw new System.Exception("Too many log files, abort");
        }
        /// <summary>
        /// Exit for graphical programs
        /// </summary>
 /*       public static void ApplicationExit(object sender, EventArgs e)
        {
            if (System.Windows.Forms.Application.MessageLoop)
            {
                // Use this since we are a WinForms app
                Console.WriteLine("Exit MessageLoop");
                System.Windows.Forms.Application.Exit();
            }
            else
            {
                // Use this since we are a console app
                Console.WriteLine("Exit Console");
                System.Environment.Exit(0);
            }
        }
*/
        public static Int64 DateTimeToMS(DateTime dtR)
        {

            Int64 ticks = (dtR.Ticks - UnixEpoch.Ticks) / TimeSpan.TicksPerMillisecond; // ( 100 ns)
            return ticks;
        }

        public static void WriteLineIf(bool b, string text, string category)
        {
            if (b)
            {
                Trace.WriteLine(text, category);
            }
        }
        public static void WriteLineIf(bool b, params string[] args)
        {
            if (b)
            {
                string[] sargs = new string[args.Length - 2];
                for (int i = 1; i < args.Length; i++) sargs[i] = args[i];
                Trace.WriteLine(String.Format(args[0], sargs, args[args.Length]));
            }
        }

        public static float TwoToZK(byte[] bytes, int from)
        {
            UInt16 u = 0;

            u = (UInt16)((UInt16)(bytes[from] << 8) + (UInt16)bytes[from + 1]);

            return ToZK(u);
        }
        public static float TwoToZK(byte[] bytes, int from, bool msb)
        {
            UInt16 u = 0;

            if (msb)
                u = (UInt16)((UInt16)(bytes[from] << 8) + (UInt16)bytes[from + 1]);
            else
                u = (UInt16)((UInt16)(bytes[from + 1] << 8) + (UInt16)bytes[from]);


            return ToZK(u);
        }

        public static float FourToZK(byte[] bytes, int from)
        {
            UInt32 u = 0;

            u = (UInt32)(
                 (bytes[from] << 24) +
                 (bytes[from + 1] << 16) +
                 (bytes[from + 2] << 8) +
                 (bytes[from + 3])
                 );

            return ToZK(u);
        }

        public static float ToZK(UInt16 u)
        {
            int vz = 1;
            if ((u & 0x8000) == 0x8000)
            {
                u = (UInt16)(~u + 1);
                vz = -1;
            }
            float f = vz * (float)u / (float)(0x8000);
            return f;
        }

        public static float ToZK(UInt32 u)
        {
            int vz = 1;
            if ((u & 0x80000000) == 0x80000000)
            {
                u = (UInt16)(~u + 1);
                vz = -1;
            }
            float f = vz * (float)u / (float)(0x80000000);
            return f;
        }

        public static UInt64 B2UInt64(byte[] bytes, int from)
        {
            UInt64 u = (UInt64)(
                        bytes[from + 7] +
                        ((UInt64)bytes[from + 6] << 8) +
                        ((UInt64)bytes[from + 5] << 16) +
                        ((UInt64)bytes[from + 4] << 24) +
                        ((UInt64)bytes[from + 3] << 32) +
                        ((UInt64)bytes[from + 2] << 40) +
                        ((UInt64)bytes[from + 1] << 48) +
                        ((UInt64)bytes[from] << 56)
                        );
            return u;
        }

        public static UInt16 B2UInt16(byte[] bytes, int from)
        {
            UInt16 u = (UInt16)(
                        (UInt16)bytes[from + 1] +
                        ((UInt16)bytes[from] << 8)
                        );
            return u;
        }



        public static UInt32 B2UInt32(byte[] bytes, int from)
        {
            UInt32 u = (UInt32)(
                        ((UInt32)bytes[from]) +
                        ((UInt32)bytes[from + 1] << 8) +
                        ((UInt32)bytes[from + 2] << 16) +
                        ((UInt32)bytes[from + 3] << 24)
                        );
            return u;
        }

        public static UInt64 SwapByteOrder(UInt64 value)
        {
            UInt64 swapped =
                 ((0x00000000000000FF) & (value >> 56)
                 | (0x000000000000FF00) & (value >> 40)
                 | (0x0000000000FF0000) & (value >> 24)
                 | (0x00000000FF000000) & (value >> 8)
                 | (0x000000FF00000000) & (value << 8)
                 | (0x0000FF0000000000) & (value << 24)
                 | (0x00FF000000000000) & (value << 40)
                 | (0xFF00000000000000) & (value << 56));
            return swapped;
        }
        public static UInt16 SwapByteOrder(UInt16 value)
        {
            UInt16 swapped = (UInt16)(
                               (0x00FF & (value >> 8)) |
                               (0xFF00 & (value << 8))
                             );
            return swapped;
        }
    }


}
