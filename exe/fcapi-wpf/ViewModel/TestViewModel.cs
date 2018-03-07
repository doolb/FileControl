using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using FCApi;
using fcapi_wpf.View;
using Microsoft.Win32;

namespace fcapi_wpf.ViewModel {
    public class TestViewModel : MVVM.ViewModel {
        #region Command

        private Command _openCmd;
        /// <summary>
        /// open connect
        /// </summary>
        public Command openCmd {
            get {
                return _openCmd ?? (_openCmd = new Command {
                    ExecuteDelegate = _ => {
                        MsgLine.Show (FC.Open (onmsg) ? "open_success" : "open_fail");
                    },
                    CanExecuteDelegate = _ => !FC.isopen
                });
            }
        }

        private Command _closeCmd;
        /// <summary>
        /// close connect
        /// </summary>
        public Command closeCmd {
            get {
                return _closeCmd ?? (_closeCmd = new Command {
                    ExecuteDelegate = _ => {
                        FC.Close ();
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }

        /// <summary>
        /// query User
        /// </summary>
        public Command queryUserCmd {
            get {
                return _queryUserCmd ?? (_queryUserCmd = new Command {
                    ExecuteDelegate = _ => {
                        users = FC.QueryUser ();
                        for (int i=0; i<users.Length; i++)
                            data = users[i].ToString () + '\n';
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _queryUserCmd;
        private User[] users;

        public Command loadCmd { get { return _loadCmd??(_loadCmd = new Command { ExecuteDelegate = _ => FC.Load () }); } }
        private Command _loadCmd;

        public Command unloadCmd { get { return _unloadCmd??(_unloadCmd = new Command { ExecuteDelegate = _ => FC.Unload () }); } }
        private Command _unloadCmd;

        public Command getworkRootCmd {
            get {
                return _getworkRootCmd ?? (_getworkRootCmd = new Command {
                    ExecuteDelegate = _ => data = FC.WorkRoot,
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _getworkRootCmd;

        public Command getfilePMCmd {
            get {
                return _getfilePMCmd ?? (_getfilePMCmd = new Command {
                    ExecuteDelegate = _ => {
                        OpenFileDialog ofd = new OpenFileDialog ();
                        if (ofd.ShowDialog () == true) {
                            data = FC.getFilePM (ofd.FileName).ToString ();
                        }
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _getfilePMCmd;

        public Command setfilePMCmd {
            get {
                return _setfilePMCmd ?? (_setfilePMCmd = new Command {
                    ExecuteDelegate = _ => {
                        OpenFileDialog ofd = new OpenFileDialog ();
                        if (ofd.ShowDialog () == true) {
                            new PMWindow (ofd.FileName).ShowDialog ();
                        }
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _setfilePMCmd;

        public Command nameWinCmd {
            get {
                return _nameWinCmd ?? (_nameWinCmd = new Command {
                    ExecuteDelegate = _ => new UserWindow ().ShowDialog (),
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _nameWinCmd;
        #endregion

        public string data { get { return _data; } set { _data = value; RaisePropertyChanged (); } }
        private string _data;


        void onmsg ( MsgCode msg ) {
            if (msg == MsgCode.Null) { return; }


            if (msg == MsgCode.User_Login) {
                // is user logined
                var user = FC.Get<User> (MsgCode.User_Login_Get);
                if (user != default (User))
                    return;

                var users = FC.QueryUser ();
                var retlen= 0;
                if (users.Length == 1) {
                    Msg_User_Login login = new Msg_User_Login ();
                    login.user = users[0];
                    login.password = "password";
                    FC.Send<Msg_User_Login> (MsgCode.User_Login, login, ref retlen);
                }
            }

            MsgLine.Show (msg.ToString ());
        }
    }
}
