using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using FCApi;

namespace fcapi_wpf.ViewModel {
    public class TestViewModel : MVVM.ViewModel {

        private Command _openCmd;
        /// <summary>
        /// open connect
        /// </summary>
        public Command openCmd {
            get {
                return _openCmd ?? (_openCmd = new Command {
                    ExecuteDelegate = _ => {
                        FC.Open (true);
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
        /// close connect
        /// </summary>
        public Command queryUserCmd {
            get {
                return _queryUserCmd ?? (_queryUserCmd = new Command {
                    ExecuteDelegate = _ => {
                        users = FC.QueryUser ();
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _queryUserCmd;
        private User[] users;

        /// <summary>
        /// close connect
        /// </summary>
        public Command listenCmd {
            get {
                return _listenCmd ?? (_listenCmd = new Command {
                    ExecuteDelegate = async _ => {
                        var msg = await FC.Listen ();
                        if (msg == MsgCode.User_Login) {
                            var users = FC.QueryUser ();
                            var retlen= 0;
                            if (users.Length == 1) {
                                Msg_User_Login login = new Msg_User_Login ();
                                login.user = users[0];
                                login.password = "password";
                                FC.Send<Msg_User_Login> (MsgCode.User_Login, login, ref retlen);
                            }
                        }
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _listenCmd;
    }
}
