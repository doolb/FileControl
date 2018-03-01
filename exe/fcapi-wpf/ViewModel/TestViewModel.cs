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

    }
}
