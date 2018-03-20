﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using Tool;

namespace FCApi {
    #region driver struct

    [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct User {
        /// <summary>
        /// user name
        /// </summary>
        [MarshalAs (UnmanagedType.ByValTStr, SizeConst=FC.NameMax)]
        public String user;

        /// <summary>
        /// group name
        /// </summary>
        [MarshalAs (UnmanagedType.ByValTStr, SizeConst=FC.NameMax)]
        public String group;

        /// <summary>
        /// user id
        /// </summary>
        public Guid uid;

        /// <summary>
        /// group id
        /// </summary>
        public Guid gid;

        public override string ToString () {
            return string.Format ("{0} @ \n {1} \n\n {2} @ \n {3}", user, uid, group, gid);
        }

        public static bool operator == ( User a, User b ) {
            return (a.uid == b.uid && a.gid == b.gid);
        }
        public static bool operator != ( User a, User b ) {
            return (a.uid != b.uid || a.gid != b.gid);
        }

        public override bool Equals ( object obj ) {
            return base.Equals (obj);
        }

        public override int GetHashCode () {
            return base.GetHashCode ();
        }
    }

    [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct Msg_User_Registry {
        public User user;

        /// <summary>
        /// password
        /// </summary>
        [MarshalAs (UnmanagedType.ByValTStr, SizeConst=FC.NameMax)]
        public String password;

        /// <summary>
        /// volume letter
        /// </summary>
        public char letter;
    }

    [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct Msg_User_Login {
        public User user;

        /// <summary>
        /// password
        /// </summary>
        [MarshalAs (UnmanagedType.ByValTStr, SizeConst=FC.NameMax)]
        public String password;
    }

    [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct Msg_File {
        [MarshalAs (UnmanagedType.LPWStr)]
        public String path;
        public User user;					// the user which whole the file
        public PermissionCode pmCode;		// permission code

        public override string ToString () {
            return string.Format ("{0}:{1}\n{2}", path, pmCode.ToString (), user.ToString ());
        }
    }

    [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct Msg_Listen {
        uint ReplyLength;
        ulong MessageId;
        public MsgCode msg;
    }

    public enum MsgCode {
        Null, // null define , for daemon use

        // user
        User_Query,
        User_Login,
        User_Login_Get,
        User_Registry,
        User_Logout,
        User_Delete,

        // volume
        Volume_Query,

        // file
        Permission_Get,
        Permission_Set,

        // work root
        WorkRoot_Get,
        WorkRoot_Set,

        Admin_Init,
        Admin_Check,
        Admin_Exit,

        Max
    }

    public enum PermissionCode {
        Invalid = 0,
        User_Read = 0x00000001,
        User_Write = 0x00000002,
        Group_Read = 0x00000004,
        Group_Write = 0x00000008,
        Other_Read = 0x00000010,
        Other_Write = 0x00000020,

        Default = User_Read | User_Write | Group_Read
    }
    #endregion



    public partial class FC {

        /// <summary>
        /// the max lenght of name
        /// </summary>
        public const int NameMax = 32;

        public const string PortName = "\\fc";
        public const string FilterName = "fc";
    }

    public static class FC_Ext {
        public static bool valid ( this IntPtr handle ) {
            return handle != IntPtr.Zero && handle != (IntPtr)(-1);
        }
    }

    /// <summary>
    /// tools
    /// </summary>
    public partial class FC {
        protected static void MarshalUnmananagedArray2Struct<T> ( IntPtr unmanagedArray, int length, out T[] mangagedArray ) {
            var size = Marshal.SizeOf (typeof (T));
            mangagedArray = new T[length];

            for (int i = 0; i < length; i++) {
                IntPtr ins = new IntPtr (unmanagedArray.ToInt64 () + i * size);
                mangagedArray[i] = (T)Marshal.PtrToStructure (ins, typeof (T));
            }
        }

        protected static bool Check ( uint retCode ) {
            Debug.WriteLine (new Win32Exception ((int)retCode).Message);
            if (retCode == 0x80070006) { // The handle is invalid. (Exception from HRESULT: 0x80070006 (E_HANDLE))
                Port = IntPtr.Zero;
                //throw new Win32Exception ((int)retCode);
            }

            if (retCode == 0)
                return true;
            return false;
        }

        /// <summary>
        /// check is driver install
        /// </summary>
        protected static bool isInstall ( string driverName ) {
            System.Management.SelectQuery query = new System.Management.SelectQuery ("Win32_SystemDriver");
            query.Condition = string.Format ("Name = '{0}'", driverName);
            System.Management.ManagementObjectSearcher searcher = new System.Management.ManagementObjectSearcher (query);
            var drivers = searcher.Get ();

            if (drivers.Count > 0)
                return true;
            else
                return false;
        }

    }

    /// <summary>
    /// dll import
    /// </summary>
    public partial class FC {


        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterConnectCommunicationPort ( string lpPortName,
            int dwOptions, IntPtr lpContext, int dwSizeOfContext, IntPtr lpSecurityAttributes,
            ref IntPtr hPort );

        [DllImport ("Kernel32.dll", SetLastError=true)]
        static extern bool CloseHandle ( IntPtr handle );

        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterSendMessage ( IntPtr hPort, ref MsgCode lpInBuffer, int dwInBufferSize, IntPtr lpOutBuffer, int dwOutBufferSize, ref int lpBytesReturned );

        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterLoad ( string name );
        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterUnload ( string name );
    }

    /// <summary>
    /// open and close
    /// </summary>
    public partial class FC {

        static FC () {
            refresh ();
        }

        public static bool installed;


        static IntPtr Port;

        public static bool Open () {
            if (isopen) { return true; }
            uint ret = FilterConnectCommunicationPort (PortName, 0, IntPtr.Zero, 0, IntPtr.Zero, ref Port);
            if (Check (ret)) { Listen (); }

            return isopen;
        }

        public static void Close () {
            CloseHandle (Port);
            Port = IntPtr.Zero;
        }

        /// <summary>
        /// is open commiunicate with driver
        /// </summary>
        public static bool isopen { get { return Port != IntPtr.Zero && Port != (IntPtr)(-1); } }

        /// <summary>
        /// refresh data
        /// </summary>
        public static void refresh () {

            // check driver install
            installed = isInstall ("fc");
            Open ();

            // check driver load
            workRoot = null;
            workRootLetter = '\0';
        }

        public static void Load () { Process.Start (new ProcessStartInfo { FileName = "fltmc", Arguments = "load fc", UseShellExecute = false, CreateNoWindow = true }); refresh (); }
        public static void Unload () { Process.Start (new ProcessStartInfo { FileName = "fltmc", Arguments = "unload fc", UseShellExecute = false, CreateNoWindow = true }); refresh (); }
    }

    /// <summary>
    /// query user
    /// </summary>
    public partial class FC {

        public static User[] QueryUser () {
            if (!isopen) { return null; }


            int retlen = 0;
            object r = Send<User> (MsgCode.User_Query, default (User), ref retlen, 25);
            if (r is User[])
                return (User[])r;
            else
                return null;
        }
        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterSendMessage ( IntPtr hPort, ref MsgCode lpInBuffer, int dwInBufferSize, ref Msg_User_Login reg, int dwOutBufferSize, ref int lpBytesReturned );

        public static bool Login ( User user, string password ) {
            if (!isopen) { return false; }
            Msg_User_Login login = new Msg_User_Login ();
            login.user = user;
            login.password = password;
            var retlen = 0;
            MsgCode msg = MsgCode.User_Login;
            uint ret = FilterSendMessage (Port, ref msg, 4, ref login, Marshal.SizeOf (typeof (Msg_User_Login)), ref retlen);
            return Check (ret);
        }

        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterSendMessage ( IntPtr hPort, ref MsgCode lpInBuffer, int dwInBufferSize, ref Msg_User_Registry reg, int dwOutBufferSize, ref int lpBytesReturned );

        public static bool addUser ( Msg_User_Registry reg ) {
            if (!isopen) { return false; }
            int retlen = 0;
            MsgCode msg = MsgCode.User_Registry;

            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), ref reg, Marshal.SizeOf (typeof (Msg_User_Registry)), ref retlen);
            if (Check (ret)) { return true; }
            else { return false; }
        }

        public static bool delUser ( string volume ) {
            if (!isopen) { return false; }

            int retlen = 0;
            MsgCode msg = MsgCode.User_Delete;
            StringBuilder sbd = new StringBuilder (volume);
            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), sbd, 2, ref retlen);
            return Check (ret);
        }
    }

    /// <summary>
    /// send message to driver
    /// </summary>
    public partial class FC {

        /// <summary>
        /// Send msg and data to driver
        /// </summary>
        /// <typeparam name="T">Data Type</typeparam>
        /// <param name="msg">MsgCode</param>
        /// <param name="data"></param>
        /// <param name="retlen"></param>
        /// <param name="count"></param>
        /// <returns></returns>
        public static object Send<T> ( MsgCode msg, T data, ref int retlen, int count = 0 ) {

            if (!isopen) { return 0; }

            var size = Marshal.SizeOf (typeof (T));             // the size of one struct
            var totalSize = count == 0 ? size : count * size;   // total size
            var buff = Marshal.AllocHGlobal (totalSize);        // memory ptr of buffer

            if (count == 0) {
                Marshal.StructureToPtr (data, buff, false);
            }
            else {
                for (int i=0; i<count; i++) {
                    Marshal.StructureToPtr (data, buff + i * size, false);
                }
            }

            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), buff, totalSize, ref retlen);
            Check (ret);

            // is success
            if (ret != 0) { return default (T); }

            // is has result
            if (retlen == 0) { return default (T); }

            T obj = default (T);
            T[] objs= null;
            if (count == 0) {
                obj = (T)Marshal.PtrToStructure (buff, typeof (T));
            }
            else {
                MarshalUnmananagedArray2Struct<T> (buff, retlen / size, out objs);
            }

            Marshal.FreeHGlobal (buff);

            return count == 0 ? (object)obj : objs;
        }

        /// <summary>
        /// send invalid buffer to query the size we need
        /// </summary>
        /// <param name="msg">MsgCode</param>
        /// <returns>The size of buffer,which drive needing</returns>
        public static int Send ( MsgCode msg ) {
            if (!isopen) { return 0; }

            int len = 0;

            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), IntPtr.Zero, 0, ref len);
            Check (ret);
            if (ret != 0x8007007a)
                return 0;

            return len;
        }

        /// <summary>
        /// get data from driver
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="msg"></param>
        /// <returns></returns>
        public static T Get<T> ( MsgCode msg ) {
            if (!isopen) { return default (T); }

            var size = Marshal.SizeOf (typeof (T));             // the size of one struct
            var buff = Marshal.AllocHGlobal (size);        // memory ptr of buffer
            var retlen = 0;
            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), buff, size, ref retlen);
            Check (ret);
            if (ret != 0 || retlen == 0) { return default (T); }

            return (T)Marshal.PtrToStructure (buff, typeof (T));
        }
    }

    public partial class FC {
        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterSendMessage ( IntPtr hPort, ref MsgCode lpInBuffer, int dwInBufferSize, ref MsgCode lpOutBuffer, int dwOutBufferSize, ref int lpBytesReturned );

        public static event Action<MsgCode> onMsging;
        protected static void Listen () {
            Task.Run (() => {
                int retlen = 0;
                MsgCode send = MsgCode.Null;
                MsgCode recv = MsgCode.Null;
                while (isopen) {
                    uint ret = FilterSendMessage (Port, ref send, 4, ref recv, 4, ref retlen);
                    if (Check (ret) && recv != MsgCode.Null && onMsging != null) {
                        System.Windows.Application.Current.Dispatcher.BeginInvoke (new Action (() => onMsging (recv)));
                    }
                    Thread.Sleep (1000);
                }
            });
        }
    }

    /// <summary>
    /// work root
    /// </summary>
    public partial class FC {
        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterSendMessage ( IntPtr hPort, ref MsgCode lpInBuffer, int dwInBufferSize, [MarshalAs (UnmanagedType.LPTStr)]StringBuilder lpOutBuffer, int dwOutBufferSize, ref int lpBytesReturned );

        public static string WorkRoot {
            get { if (workRoot == null) { getWorkRoot (); } return workRoot; }
        }
        private static string workRoot;
        public static char WorkRootLetter {
            get { if (workRootLetter == 0) { getWorkRoot (); } return workRootLetter; }
        }
        private static char workRootLetter;
        static void getWorkRoot () {
            if (!Port.valid ()) { return; }

            StringBuilder sbd = new StringBuilder (1024);
            MsgCode msg = MsgCode.WorkRoot_Get;
            int retlen = 0;
            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), sbd, sbd.Capacity, ref retlen);
            Check (ret);
            if (sbd.Length == 0) { return; }

            workRootLetter = sbd[0];
            workRoot = sbd.ToString ().Substring (1, sbd.Length - 1);
        }
        public static bool setWorkRoot ( char letter ) {
            if (!Port.valid ()) { return false; }

            int retlen = 0;
            MsgCode msg = MsgCode.WorkRoot_Set;
            StringBuilder sbd = new StringBuilder (letter.ToString ());
            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), sbd, 2, ref retlen);

            if (Check (ret))
                return true;
            return false;
        }

        public static string getVolumes () {
            if (!Port.valid ()) { return null; }

            StringBuilder sbd = new StringBuilder (1024);
            MsgCode msg = MsgCode.Volume_Query;
            int retlen = 0;
            uint ret = FilterSendMessage (Port, ref msg, sizeof (MsgCode), sbd, sbd.Capacity, ref retlen);
            Check (ret);

            return sbd.ToString ();
        }
    }

    /// <summary>
    /// file permission
    /// </summary>
    public partial class FC {
        public static Msg_File getFilePM ( string path ) {
            if (char.ToUpper (path[0]) != WorkRootLetter || !isopen)
                return default (Msg_File);

            int retlen = 0;

            Msg_File file = new Msg_File ();
            file.path = WorkRoot + path.Substring (2, path.Length - 2);

            file = (Msg_File)Send<Msg_File> (MsgCode.Permission_Get, file, ref retlen);
            return file;
        }

        public static bool setFilePM ( Msg_File mf ) {
            if (!isopen) { return false; }

            var retlen = 0;
            return (bool)Send<Msg_File> (MsgCode.Permission_Set, mf, ref retlen);
        }
    }

    /// <summary>
    /// admin 
    /// </summary>
    public partial class FC {
        public static bool CheckAdmin () {
            int retlen = 0;
            RSA rsa = new RSA ();
            rsa.genKeys ("res/primes.txt");
            Send<RSA.Key> (MsgCode.Admin_Init, rsa.key, ref retlen);
            return true;
        }
    }
}
