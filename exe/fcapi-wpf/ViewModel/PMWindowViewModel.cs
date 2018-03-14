using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using FCApi;
using MVVM;
using p = fcapi_wpf.Properties;
namespace fcapi_wpf.ViewModel {
    class PMWindowViewModel : MVVM.ViewModel {
        public View.MsgLine msg;
        public string file { get { return _file; } set { _file = value; RaisePropertyChanged (); } }
        private string _file;

        public string userName { get { return _userName; } set { _userName = value; RaisePropertyChanged (); } }
        private string _userName;

        public bool hasPM { get { return _hasPM; } set { _hasPM = value; RaisePropertyChanged (); } }
        private bool _hasPM;

        public PMItem[] pmItems { get { return _pmItems; } set { _pmItems = value; RaisePropertyChanged (); } }
        private PMItem[] _pmItems;

        private Msg_File fileInfo;
        private User user;

        public string status { get { return _status; } set { _status = value; RaisePropertyChanged (); } }
        private string _status;

        public Command updateCmd {
            get {
                return _updateCmd ??(_updateCmd = new Command {
                    ExecuteDelegate = _ => {
                        msg.Show (FC.setFilePM (fileInfo)? "success" : "fail" );
                    },
                    CanExecuteDelegate = _ => FC.isopen && hasPM
                });
            }
        }
        private Command _updateCmd;

        /// <summary>
        /// open file for read or write permission
        /// </summary>
        /// <param name="path"></param>
        /// <returns></returns>
        public bool open ( string path ) {
            file = path;

            if (!IsAdministrator ()) { status = Language ("admin"); return false; }
            // is driver installed
            if (!FC.installed) { status = Language ("driver_no_install"); return false; }
            // is driver load
            if (!FC.isopen) { status = Language ("driver_no_run"); return false; }

            if (FC.WorkRoot == null) { status = Language ("no_work_dir"); return false; }

            // get file info
            fileInfo = FC.getFilePM (path);
            if (fileInfo.Equals (default (Msg_File)))
                return false;

            userName = fileInfo.user.user + "  " + fileInfo.user.group;

            // get current user
            user = FC.Get<User> (MsgCode.User_Login_Get);

            hasPM = user == fileInfo.user;

            pmItems = new PMItem[6];
            uint code = 1;
            for (int i=0; i<pmItems.Length; i++, code <<= 1) {
                pmItems[i] = new PMItem {
                    vm = this,
                    name = p.Resources.ResourceManager.GetString ("pm_name_"+ i.ToString ()),
                    enable = (fileInfo.pmCode & (PermissionCode)code)!= PermissionCode.Invalid,
                    code = (PermissionCode)(code),
                };
            }

            status = "";
            return true;
        }

        public class PMItem : MVVM.ViewModel {
            public string name { get; set; }
            public bool enable { get { return _enable; } set { _enable = value; RaisePropertyChanged (); } }
            private bool _enable;
            public PermissionCode code { get; set; }

            public PMWindowViewModel vm;

            public Command updateCmd {
                get {
                    return _updateCmd??(_updateCmd = new Command {
                        ExecuteDelegate = _ => {
                            if ((vm.fileInfo.pmCode & code) != PermissionCode.Invalid) { vm.fileInfo.pmCode &= ~code; }
                            else { vm.fileInfo.pmCode |= code; }

                            enable = (vm.fileInfo.pmCode & code)!= PermissionCode.Invalid;
                        },
                        CanExecuteDelegate = _ => FC.isopen && vm.hasPM
                    });
                }
            }
            private Command _updateCmd;
        }
    }
}
