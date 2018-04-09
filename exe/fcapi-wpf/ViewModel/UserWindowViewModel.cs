﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using fcapi_wpf.View;
using System.Windows.Navigation;
using System.Windows.Controls;
using FCApi;
using System.Collections.ObjectModel;
using System.Windows.Input;

namespace fcapi_wpf.ViewModel {
    class UserWindowViewModel : MVVM.ViewModel {
        public View.MsgLine msg { get { return _msg; } set { _msg = value; RaisePropertyChanged (); } }
        private View.MsgLine _msg;
        public Page page { get { return _page; } set { _page = value; RaisePropertyChanged (); } }
        private Page _page;

        Dictionary<Type, Page> pages = new Dictionary<Type, Page> ();

        public ObservableCollection<UserViewModel> userVms { get { return _userVms; } set { _userVms = value; RaisePropertyChanged (); } }
        private ObservableCollection<UserViewModel> _userVms = new ObservableCollection<UserViewModel> ();


        private User[] users;
        /// <summary>
        /// 当前登录的用户
        /// </summary>
        public User loginUser;

        public bool islogin { get { return _islogin; } set { _islogin = value; RaisePropertyChanged (); } }
        private bool _islogin;

        public ICommand exitCmd {
            get {
                return _exitCmd ?? (_exitCmd = new Command {

                    ExecuteDelegate = _ => {
                        FC.LoginOut (loginUser);
                        refresh ();
                    },
                    CanExecuteDelegate = _ => islogin
                });
            }
        }
        private ICommand _exitCmd;

        /// <summary>
        /// is no user in system
        /// </summary>
        public string status { get { return _status; } set { _status = value; RaisePropertyChanged (); } }
        private string _status;

        public UserWindowViewModel () {
            this.msg = new MsgLine ();
            refresh ();
        }

        public void refresh () {
            status = "";
            islogin = false;
            bool need_admin = false;

            if (!IsAdministrator ()) { status = Language ("admin"); return; }

            // is driver installed
            if (!FC.installed) { status = Language ("driver_no_install"); this.page = null; return; }

            // is driver load
            if (!FC.isopen) { status = Language ("driver_no_run"); this.page = null; return; }

            //if (FC.onMsging == null) { FC.onMsging += }

            if (!FC.hasAdmin) { Show<AdminCreate> (new AdminCreateViewModel () { vm = this }); return; }

            if (FC.WorkRoot == null) { status = Language ("no_work_dir"); need_admin = true; }

            // is user already login
            User user = FC.Get<User> (MsgCode.User_Login_Get);
            if (!user.Equals (default (User))) {
                loginUser = user;
                islogin = true;
                Show<UserViewPage> (new UserViewModel (this, user));
                return;
            }

            // query all user
            userVms.Clear ();
            users = FC.QueryUser ();
            if (users != null) {
                for (int i=0; i<users.Length; i++) {
                    if (!need_admin)
                        userVms.Add (new UserViewModel (this, users[i]));
                    else if (users[i].isAdmin ())
                        userVms.Add (new UserViewModel (this, users[i]));
                }

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

                isAdmin = user.isAdmin ();
            }

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

            public bool isAdmin { get { return _isAdmin; } set { _isAdmin = value; RaisePropertyChanged (); } }
            private bool _isAdmin;
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
                            if (user.isAdmin ())
                                vm.Show<UserAdminPage> (new UserAdminViewModel () { msg = vm.msg });
                            else
                                vm.Show<UserViewPage> (this);
                            vm.islogin = true;
                            vm.loginUser = user;
                        }
                    });
                }
            }
            private Command _loginCmd;

            public Command showUserCmd {
                get {
                    return _showUserCmd ?? (_showUserCmd = new Command {
                        ExecuteDelegate = _ => vm.ShowAllUser (),
                        CanExecuteDelegate = _ => vm.users != null && vm.users.Length > 1
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
