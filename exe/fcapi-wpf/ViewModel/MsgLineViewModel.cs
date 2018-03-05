using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
namespace fcapi_wpf.ViewModel {
    class MsgLineViewModel : MVVM.ViewModel {
        public string msg { get { return _msg; } set { _msg = value; RaisePropertyChanged (); } }
        private string _msg;
    }
}
