/*
* Arguments class: application arguments interpreter
*
* Authors:		R. LOPES
* Contributors:	R. LOPES
* Created:		25 October 2002
* Modified:		28 October 2002
*
* Version:		1.0
*/

/*
* Example Using:

     static void Main(string[] args)
        {
           Arguments arg = new Arguments(args);
           if (args.Length > 0)
           {
               arg.ParseCommandLine();
               if (arg["h"] != null)
               {
                    PrintHelp();
            }
        }
       static void PrintHelp()
        {
            Console.WriteLine("Usage: {0} -p com -g", System.Reflection.Assembly.GetExecutingAssembly().GetName().Name);
        }
 * 
 */
using System;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace CommandLine.Utility
{
    /// <summary>
    /// Arguments class
    /// </summary>
    public class Arguments
    {
        // Variables
        private Dictionary<String,String> Parameters;
        // private bool bDebug = false;
        // Constructor
        public Arguments(string[] Args)
        {
            Parameters = new Dictionary<String,String>();
            Regex Spliter = new Regex(@"^-{1,2}|^/|=|:", RegexOptions.IgnoreCase | RegexOptions.Compiled);
            Regex Remover = new Regex(@"^['""]?(.*?)['""]?$", RegexOptions.IgnoreCase | RegexOptions.Compiled);
            string Parameter = null;
            string[] Parts;

            // Valid parameters forms:
            // {-,/,--}param{ ,=,:}((",')value(",'))
            // Examples: -param1 value1 --param2 /param3:"Test-:-work" /param4=happy -param5 '--=nice=--'
            foreach (string Txt in Args)
            {
                // Look for new parameters (-,/ or --) and a possible enclosed value (=,:)
                Parts = Spliter.Split(Txt, 3);
                switch (Parts.Length)
                {
                    // Found a value (for the last parameter found (space separator))
                    case 1:
                        if (Parameter != null)
                        {
                            if (!Parameters.ContainsKey(Parameter))
                            {
                                Parts[0] = Remover.Replace(Parts[0], "$1");
                                Parameters.Add(Parameter, Parts[0]);
                            }
                            Parameter = null;
                        }
                        // else Error: no parameter waiting for a value (skipped)
                        break;
                    // Found just a parameter
                    case 2:
                        // The last parameter is still waiting. With no value, set it to true.
                        if (Parameter != null)
                        {
                            if (!Parameters.ContainsKey(Parameter)) Parameters.Add(Parameter, "true");
                        }
                        Parameter = Parts[1];
                        break;
                    // Parameter with enclosed value
                    case 3:
                        // The last parameter is still waiting. With no value, set it to true.
                        if (Parameter != null)
                        {
                            if (!Parameters.ContainsKey(Parameter)) Parameters.Add(Parameter, "true");
                        }
                        Parameter = Parts[1];
                        // Remove possible enclosing characters (",')
                        if (!Parameters.ContainsKey(Parameter))
                        {
                            Parts[2] = Remover.Replace(Parts[2], "$1");
                            Parameters.Add(Parameter, Parts[2]);
                        }
                        Parameter = null;
                        break;
                }
            }
            // In case a parameter is still waiting
            if (Parameter != null)
            {
                if (!Parameters.ContainsKey(Parameter)) Parameters.Add(Parameter, "true");
            }
        }

        // Retrieve a parameter value if it exists
        public string this[string Param]
        {
            get
            {
                if (Parameters.ContainsKey(Param))
                    return (Parameters[Param]);
                else
                    return null;
            }
        }
        private Regex Flag = new Regex(@"^-(.*)$", RegexOptions.IgnoreCase | RegexOptions.Compiled);
        private String flag;
        public void ParseCommandLine()
        {
            String[] CommandLineArgs = Environment.GetCommandLineArgs();
            //System.Console.WriteLine(Environment.CommandLine);
            Parameters = new Dictionary<String, String>();
            foreach (string arg in CommandLineArgs)
            {
                Match m = Flag.Match(arg);
                if (m.Success)
                {
//                  System.Console.WriteLine("ParseCommandLine: Found flag: {0}", m.Value);
                    Group group = m.Groups[1];
                    Capture cap = group.Captures[0];
                    flag = cap.Value;
                    if (Parameters.ContainsKey(flag))
                    {
                        System.Diagnostics.Trace.WriteLine(String.Format("Flag:{0} must'nt twice!", flag), "ERROR");
                        Environment.Exit(2);
                    }
                    Parameters.Add(flag, flag);
                }
                else
                {
                    if (flag != null)
                    {
                        //System.Console.WriteLine("ParseCommandLine: -{1} {0}", arg, flag);
                        Parameters[flag] = arg;
                    }


                }
            }
        }
    }
}
