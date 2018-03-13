using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace fcapi_wpf.Tool {
    /// <summary>
    /// A very simple Named Pipe Server implementation that makes it 
    /// easy to pass string messages between two applications.
    /// ref:https://weblog.west-wind.com/posts/2016/May/13/Creating-Single-Instance-WPF-Applications-that-open-multiple-Files#WPFandSingletonapps
    /// </summary>
    public class NamedPipeManager {
        public string NamedPipeName = "MarkdownMonster";
        public event Action<string> ReceiveString;

        private const string EXIT_STRING = "__EXIT__";
        private bool _isRunning = false;
        private Thread Thread;
        private Mutex mutex;

        public NamedPipeManager ( string name ) {
            NamedPipeName = name;
        }

        /// <summary>
        /// Starts a new Pipe server on a new thread
        /// </summary>
        public void StartServer () {
            Thread = new Thread (( pipeName ) => {
                _isRunning = true;

                while (true) {
                    string text;
                    using (var server = new NamedPipeServerStream (pipeName as string)) {
                        server.WaitForConnection ();

                        using (StreamReader reader = new StreamReader (server)) {
                            text = reader.ReadToEnd ();
                        }
                    }

                    if (text == EXIT_STRING)
                        break;

                    OnReceiveString (text);

                    if (_isRunning == false)
                        break;
                }
            });
            Thread.Start (NamedPipeName);
        }

        public bool SingleServer () {
            bool onlyOneInstance;
            mutex = new Mutex (true, NamedPipeName, out onlyOneInstance);
            return onlyOneInstance;
        }

        /// <summary>
        /// Called when data is received.
        /// </summary>
        /// <param name="text"></param>
        protected virtual void OnReceiveString ( string text ) { if (ReceiveString != null) ReceiveString (text); }


        /// <summary>
        /// Shuts down the pipe server
        /// </summary>
        public void StopServer () {
            _isRunning = false;
            Write (EXIT_STRING);
            Thread.Sleep (30); // give time for thread shutdown
        }

        /// <summary>
        /// Write a client message to the pipe
        /// </summary>
        /// <param name="text"></param>
        /// <param name="connectTimeout"></param>
        public bool Write ( string text, int connectTimeout = 300 ) {
            using (var client = new NamedPipeClientStream (NamedPipeName)) {
                try {
                    client.Connect (connectTimeout);
                }
                catch {
                    return false;
                }

                if (!client.IsConnected)
                    return false;

                using (StreamWriter writer = new StreamWriter (client)) {
                    writer.Write (text);
                    writer.Flush ();
                }
            }
            return true;
        }
    }
}
