using fcapi_wpf.Tool;
using fcapi_wpf.View;
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading;
using System.Windows;

namespace fcapi_wpf {
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application {

        const string Name = "fcapi_wpf";

        NamedPipeManager pipe;


        private void Application_Startup ( object sender, StartupEventArgs e ) {
            pipe = new NamedPipeManager (Name);
            if (!pipe.SingleServer ()) {
                if (e.Args.Length == 1)
                    pipe.Write (e.Args[0]);
                Environment.Exit (0);
            }

            pipe.ReceiveString +=pipe_ReceiveString;
            pipe.StartServer ();

            var win = new UserWindow ();
            win.Closed+=win_Closed;
            win.Show ();
        }

        void win_Closed ( object sender, EventArgs e ) {
            pipe.StopServer ();
        }

        void pipe_ReceiveString ( string filesToOpen ) {
            Dispatcher.Invoke (() => {
                if (!string.IsNullOrEmpty (filesToOpen)) {
                    var win = new View.PMWindow (filesToOpen);
                    win.Topmost = true;
                    win.Activate ();
                    win.Show ();
                }
            });
        }

    }
}
