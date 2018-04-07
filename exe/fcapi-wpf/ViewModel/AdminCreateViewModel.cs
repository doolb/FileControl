using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using System.Windows.Input;
using FCApi;
using System.Collections.ObjectModel;
using fcapi_wpf.View;
using System.Windows.Controls;

namespace fcapi_wpf.ViewModel {
    class AdminCreateViewModel : MVVM.ViewModel {

        private User user;
        private bool hasAdmin;
        private bool hasVolume;
        public UserWindowViewModel vm;
        public AdminCreateViewModel () {
            refresh ();
            FC.onMsging +=FC_onMsging;
        }

        void FC_onMsging ( MsgCode obj ) {
            if (obj == MsgCode.Volume_Query)
                refresh ();
        }

        void refresh () {
            // query admin
            hasAdmin = false;
            User[] alluser = FC.QueryUser ();
            if (alluser != null) {
                foreach (var u in alluser) {
                    if (u.isAdmin ()) {
                        user = u;
                        admin = u.user;
                        hasAdmin = true;
                        break;
                    }
                }
            }

            // query empty volume
            volumes.Clear ();
            hasVolume = false;
            string s = FC.getVolumes ();
            for (int i=0; i<s.Length; i+=2) {
                if (s[i + 1] == 'U') {
                    volumes.Add (s[i].ToString ());
                    hasVolume = true;
                    break;
                }
            }

            RaisePropertyChanged ("admin");
            RaisePropertyChanged ("volumes");
            RaisePropertyChanged ("selectCmd");
            RaisePropertyChanged ("newCmd");
        }

        public string admin { get { return _admin; } set { _admin = value; RaisePropertyChanged (); } }
        private string _admin;

        public string name { get { return _name; } set { _name = value; RaisePropertyChanged (); } }
        private string _name;

        /// <summary>
        /// the volume of admin can be installed on
        /// </summary>
        public ObservableCollection<string> volumes {
            get { return _volumes; }
            set { _volumes = value; RaisePropertyChanged (); }
        }
        private ObservableCollection<string> _volumes = new ObservableCollection<string> ();

        public string volume {
            get { return _volume; }
            set { _volume = value; RaisePropertyChanged (); }
        }
        private string _volume;

        /// <summary>
        /// select exisited admin
        /// </summary>
        public ICommand selectCmd {
            get {
                return _selectCmd ?? (_selectCmd = new Command {
                    ExecuteDelegate = _ => {
                        int retlen = 0;
                        FC.Send<User> (MsgCode.Admin_Set, user, ref retlen);
                        if (FC.exception.NativeErrorCode ==0) {
                            FC.refresh ();
                            vm.Show<UserLoginPage> (new UserWindowViewModel.UserViewModel (vm, user));
                        }
                    },
                    CanExecuteDelegate = _ => FC.isopen && hasAdmin
                });
            }
        }
        private ICommand _selectCmd;

        // create a new admin
        public ICommand newCmd {
            get {
                return _newCmd ?? (_newCmd = new Command {
                    ExecuteDelegate = p => {
                        if (FC.CreateAdmin (volume[0], name, (p as PasswordBox).Password)) {
                            vm.Show<UserAdminPage> (new UserAdminViewModel ());
                            return;
                        }
                    },
                    CanExecuteDelegate = _ => FC.isopen && hasVolume
                });
            }
        }
        private ICommand _newCmd;
    }
}
