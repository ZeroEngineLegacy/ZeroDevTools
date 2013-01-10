using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace ZeroCrashHandler
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(String[] arguments)
        {
            ICSharpCode.SharpZipLib.Zip.
            // Enable visuals
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            // If the arguments 
            if (arguments.Length >= 1)
            {
                // The rest of the arguments are all files
                List<String> files = arguments.ToList();
                files.RemoveAt(0);

                // Store the engine version (the first argument
                String version = arguments[0];

                // Run the application
                Application.Run(new Main(version, files));
            }
        }
    }
}
