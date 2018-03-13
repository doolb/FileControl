using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using fcapi_wpf.View;
using System.Windows.Navigation;
using System.Windows.Controls;
using FCApi;

namespace fcapi_wpf.ViewModel {
    class UserWindowViewModel : MVVM.ViewModel {

        public Page page { get { return _page; } set { _page = value; RaisePropertyChanged (); } }
        private Page _page;

        Dictionary<Type, Page> pages = new Dictionary<Type, Page> ();

        public UserViewModel[] userVms { get { return _userVms; } set { _userVms = value; RaisePropertyChanged (); } }
        private UserViewModel[] _userVms;

        private User[] users;

        /// <summary>
        /// is no user in system
        /// </summary>
        public string status { get { return _status; } set { _status = value; RaisePropertyChanged (); } }
        private string _status;

        private Page lastPage;
        public Command adminCmd {
            get {
                return _adminCmd ?? (_adminCmd = new Command {
                    ExecuteDelegate = _ => {
                        if (!inAdmin) { lastPage = page; Show<UserAdminPage> (null); inAdmin = true; status = ""; }
                        else { inAdmin = false; page = lastPage; refresh (); }
                    }
                });
            }
        }
        private Command _adminCmd;

        public bool inAdmin { get { return _inAdmin; } set { _inAdmin = value; RaisePropertyChanged (); } }
        private bool _inAdmin;

        public UserWindowViewModel () {
            refresh ();
        }

        public void refresh () {
            // is driver installed
            if (!FC.installed) { status = Language ("driver_no_install"); this.page = null; return; }

            // is driver load
            if (!FC.loaded) { status = Language ("driver_no_run"); this.page = null; return; }

            // open driver
            if (!FC.isopen) {
                FC.Open (null);
            }

            if (FC.WorkRoot == null) { status = Language ("no_work_dir"); this.page = null; return; }

            // is user already login
            User user = FC.Get<User> (MsgCode.User_Login_Get);
            if (!user.Equals (default (User))) {
                Show<UserViewPage> (new UserViewModel (this, user));
                return;
            }

            // query all user
            users = FC.QueryUser ();
            if (users != null) {
                userVms = new UserViewModel[users.Length];
                for (int i=0; i<users.Length; i++) { userVms[i] = new UserViewModel (this, users[i]); }

                // is only one user
                if (users.Length == 1)
                    Show<UserLoginPage> (userVms[0]);
                else
                    ShowAllUser ();
                return;
            }

            this.page = null;
            status = Language ("no_user");
        }

        public void Show<T> ( MVVM.ViewModel viewModel ) where T : Page {
            if (!pages.ContainsKey (typeof (T)))
                pages.Add (typeof (T), Activator.CreateInstance<T> ());

            page = pages[typeof (T)];
            if (viewModel != null) { page.DataContext = viewModel; }
        }

        public void ShowAllUser () {
            Show<UserSelectPage> (this);
            RaisePropertyChanged ("userVms");
        }

        public class UserViewModel : MVVM.ViewModel {

            private UserWindowViewModel vm;
            private User user;

            public UserViewModel ( UserWindowViewModel vm, User user ) {
                this.vm = vm;
                this.user = user;

                name = user.user;
                group = user.group;
                uid = user.uid;
                gid = user.gid;
                img = "http://127.0.0.1/IMG/a.png";
            }

            /// <summary>
            /// user image
            /// </summary>
            public string img { get { return _img; } set { _img = value; RaisePropertyChanged (); } }
            private string _img;

            /// <summary>
            /// user name
            /// </summary>
            public string name { get { return _name; } set { _name = value; RaisePropertyChanged (); } }
            private string _name;

            /// <summary>
            /// group name
            /// </summary>
            public string group { get { return _group; } set { _group = value; RaisePropertyChanged (); } }
            private string _group;

            /// <summary>
            /// user guid
            /// </summary>
            public Guid uid { get { return _uid; } set { _uid = value; RaisePropertyChanged (); } }
            private Guid _uid;

            /// <summary>
            /// group guid
            /// </summary>
            public Guid gid { get { return _gid; } set { _gid = value; RaisePropertyChanged (); } }
            private Guid _gid;

            public bool logFail { get { return _logFail; } set { _logFail = value; RaisePropertyChanged (); } }
            private bool _logFail;

            public string msgFail { get { return _msgFail; } set { _msgFail = value; RaisePropertyChanged (); } }
            private string _msgFail;

            public Command loginCmd {
                get {
                    return _loginCmd ??(_loginCmd = new Command {
                        ExecuteDelegate = p => {
                            logFail = false;
                            string pass = (p as PasswordBox).Password;
                            if (string.IsNullOrEmpty (pass)) { logFail = true; msgFail = Language ("null_password"); return; }
                            if (!FCApi.FC.Login (user, pass)) { logFail = true; msgFail = Language ("login_fail"); return; }
                            vm.Show<UserViewPage> (this);
                        }
                    });
                }
            }
            private Command _loginCmd;

            public Command showUserCmd {
                get {
                    return _showUserCmd ?? (_showUserCmd = new Command {
                        ExecuteDelegate = _ => vm.ShowAllUser (),
                        CanExecuteDelegate = _ => vm.users.Length > 1
                    });
                }
            }
            private Command _showUserCmd;

            public Command selUserCmd {
                get {
                    return _selUserCmd ??(_selUserCmd = new Command {
                        ExecuteDelegate = _ => {
                            vm.Show<UserLoginPage> (this);
                        }
                    });
                }
            }
            private Command _selUserCmd;

        }
    }
}
