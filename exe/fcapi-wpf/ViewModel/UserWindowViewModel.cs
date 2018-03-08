﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using fcapi_wpf.View;
using System.Windows.Navigation;
using System.Windows.Controls;

namespace fcapi_wpf.ViewModel {
    class UserWindowViewModel : MVVM.ViewModel {

        public Page page { get { return _page; } set { _page = value; RaisePropertyChanged (); } }
        private Page _page;

        Dictionary<Type, Page> pages = new Dictionary<Type, Page> ();

        public UserViewModel[] users { get { return _users; } set { _users = value; RaisePropertyChanged (); } }
        private UserViewModel[] _users;

        /// <summary>
        /// is no user in system
        /// </summary>
        public bool nouser { get { return _nouser; } set { _nouser = value; RaisePropertyChanged (); } }
        private bool _nouser;

        public UserWindowViewModel () {

            users = new UserViewModel[5];
            for (int i=0; i<users.Length; i++) {
                users[i] = new UserViewModel {
                    vm = this,
                    img = "https://2-im.guokr.com/FKrbvmmuROVn-mFSEv9NzeuIF3LmaiFUnUP8kRCvndCgAAAAoAAAAEpQ.jpg?imageView2/1/w/69/h/69",
                    name ="test",
                    group="group"
                };
            }
            //Show<UserLoginPage> (users[0]);
            Show<UserViewPage> (users[0]);
        }

        public void Show<T> ( MVVM.ViewModel viewModel ) where T : Page {
            if (!pages.ContainsKey (typeof (T)))
                pages.Add (typeof (T), Activator.CreateInstance<T> ());

            page = pages[typeof (T)];
            page.DataContext = viewModel;
        }

        public void ShowAllUser () {
            Show<UserSelectPage> (this);
        }

        public class UserViewModel : MVVM.ViewModel {

            public UserWindowViewModel vm;

            public FCApi.User data;

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
                            if (!FCApi.FC.Login (data, pass)) { logFail = true; msgFail = Language ("login_fail"); return; }
                        }
                    });
                }
            }
            private Command _loginCmd;

            public Command showUserCmd {
                get {
                    return _showUserCmd ?? (_showUserCmd = new Command {
                        ExecuteDelegate = _ => vm.ShowAllUser ()
                    });
                }
            }
            private Command _showUserCmd;

            public Command selUserCmd {
                get {
                    return _selUserCmd ??(_selUserCmd = new Command {
                        ExecuteDelegate = _ => vm.Show<UserLoginPage> (this)
                    });
                }
            }
            private Command _selUserCmd;

        }
    }
}
