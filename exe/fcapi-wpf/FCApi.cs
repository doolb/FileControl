using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

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
        public String   path;
        public User     user;					// the user which whole the file
        public PermissionCode pmCode;		// permission code
    }

    [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct Msg_Listen {
        uint ReplyLength;
        ulong MessageId;
        public MsgCode msg;
    }

    public enum MsgCode {
        Null=0, // null define , for daemon use

        // user
        User_Query      = 1,
        User_Login      = 2,
        User_Registry   = 4,
        User_Logout     = 8,

        // volume
        Volume_Query = 16,

        // file
        Permission_Get = 32,
        Permission_Set = 64,
    }

    public enum PermissionCode {
        PC_Invalid = 0,
        PC_User_Read = 0x00000001,
        PC_User_Write = 0x00000002,
        PC_Group_Read = 0x00000004,
        PC_Group_Write = 0x00000008,
        PC_Other_Read = 0x00000010,
        PC_Other_Write = 0x00000020,

        PC_Default = PC_User_Read | PC_User_Write | PC_Group_Read
    }
    #endregion



    public partial class FC {

        /// <summary>
        /// the max lenght of name
        /// </summary>
        public const int NameMax = 32;

        public const string DaemonPort = "\\fc-d";
        public const string NormalPort = "\\fc";
        public const string FilterName = "fc";
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

        protected static void Check ( uint retCode ) {
            Debug.WriteLine (new Win32Exception ((int)retCode).Message);
            if (retCode == 0x80070006) { // The handle is invalid. (Exception from HRESULT: 0x80070006 (E_HANDLE))
                Port = IntPtr.Zero;
                //throw new Win32Exception ((int)retCode);
            }
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
        static extern uint FilterGetMessage ( IntPtr hPort, ref Msg_Listen lpMessageBuffer, int dwMessageBufferSize, IntPtr lpOverlapped );


        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterLoad ( string name );
        [DllImport ("FltLib.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern uint FilterUnload ( string name );
    }

    /// <summary>
    /// open and close
    /// </summary>
    public partial class FC {

        static IntPtr Port;

        public static bool Open ( Action<MsgCode> onmsg = null ) {
            uint ret = FilterConnectCommunicationPort (onmsg != null ? DaemonPort : NormalPort, 0, IntPtr.Zero, 0, IntPtr.Zero, ref Port);
            Check (ret);

            if (ret == 0 && onmsg != null) { Listen (onmsg); }

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


        public static void Load () { Process.Start (new ProcessStartInfo { FileName = "fltmc", Arguments = "load fc", UseShellExecute = false, CreateNoWindow = true }); }
        public static void Unload () { Process.Start (new ProcessStartInfo { FileName = "fltmc", Arguments = "unload fc", UseShellExecute = false, CreateNoWindow = true }); }
    }

    /// <summary>
    /// query user
    /// </summary>
    public partial class FC {

        public static User[] QueryUser () {
            if (!isopen) { return null; }


            int retlen = Send (MsgCode.User_Query);
            if (retlen == 0)
                return null;

            User[] users = (User[])Send<User> (MsgCode.User_Query, default (User), ref retlen, retlen / Marshal.SizeOf (typeof (User)));

            return users;
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

            if (retlen == 0 && ret > 0) {
                return true;
            }

            T obj = default (T);
            T[] objs= null;
            if (count == 0) {
                obj = (T)Marshal.PtrToStructure (buff, typeof (T));
            }
            else {
                MarshalUnmananagedArray2Struct<T> (buff, count, out objs);
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
    }

    public partial class FC {
        protected static void Listen ( Action<MsgCode> onmsg ) {
            if (onmsg == null) { return; }

            Task.Run (() => {

                Msg_Listen lis = new Msg_Listen ();

                while (isopen) {
                    // get message
                    uint ret = FilterGetMessage (Port, ref lis, Marshal.SizeOf (typeof (Msg_Listen)), IntPtr.Zero);
                    Check (ret);

                    // handle message
                    onmsg (lis.msg);
                }
            });
        }
    }
}
