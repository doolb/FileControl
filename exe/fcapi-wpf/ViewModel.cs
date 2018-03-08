using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

using p = fcapi_wpf.Properties;

namespace MVVM {
    public class ViewModel : INotifyPropertyChanged {

        #region interface INotifyPropertyChanged
        public event PropertyChangedEventHandler PropertyChanged;

        /// <summary>
        /// Raises the PropertyChanged event if needed.
        /// </summary>
        /// <param name="propertyName">The name of the property that changed. 
        /// this function also using CallerMemberName, so you could not use param</param>
        protected virtual void RaisePropertyChanged ( [CallerMemberName] string propertyName = null ) {
            if (PropertyChanged != null) {
                PropertyChanged (this, new PropertyChangedEventArgs (propertyName));
            }
        }

        #endregion

        #region 字段


        #endregion

        #region 命令


        #endregion

        #region 函数
        public static string Language ( string key ) {
            return p.Resources.ResourceManager.GetString (key) ?? key;
        }

        #endregion
    }
}
