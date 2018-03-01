using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace FCApi {


    public class FC {

        [DllImport ("fcapi.dll")]
        static extern bool fc_open ( bool isdaemon );
        public static bool Open ( bool isdaemon ) {
            _isopen = fc_open (isdaemon);
            return isopen;
        }

        [DllImport ("fcapi.dll")]
        static extern void fc_close ();
        public static void Close () {
            fc_close ();
        }

        private static bool _isopen;
        /// <summary>
        /// is open commiunicate with driver
        /// </summary>
        public static bool isopen { get { return _isopen; } }
    }

}
