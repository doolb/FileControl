using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using System.Windows;
using FCApi;

namespace fcapi_wpf.ViewModel {
    class UserAdminViewModel : MVVM.ViewModel {

        /// <summary>
        /// exit
        /// </summary>
        public Command exitCmd { get { return _exitCmd ?? (_exitCmd = new Command { ExecuteDelegate = _ => Application.Current.Shutdown () }); } }
        private Command _exitCmd;

        /// <summary>
        /// driver work root
        /// </summary>
        public string workRoot { get { return _workRoot; } set { _workRoot = value; RaisePropertyChanged (); } }
        private string _workRoot;


        /// <summary>
        /// load driver
        /// </summary>
        public Command loadCmd { get { return _loadCmd ?? (_loadCmd = new Command { ExecuteDelegate = _ => { FC.Load (); refresh (); }, CanExecuteDelegate = _ => FC.installed && !FC.loaded }); } }
        private Command _loadCmd;

        /// <summary>
        /// unload driver
        /// </summary>
        public Command unloadCmd { get { return _unloadCmd ?? (_unloadCmd = new Command { ExecuteDelegate = _ => { FC.Unload (); refresh (); }, CanExecuteDelegate = _ => FC.installed && FC.loaded }); } }
        private Command _unloadCmd;


        public UserAdminViewModel () {
            refresh ();
        }

        void refresh () {
            workRoot = FC.WorkRootLetter != 0 ? FC.WorkRootLetter.ToString () : Language ("empty");
        }
    }
}
