using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace fcapi_wpf.View {
    /// <summary>
    /// Interaction logic for UserAdminPage.xaml
    /// </summary>
    public partial class UserAdminPage : Page {
        public UserAdminPage () {
            InitializeComponent ();
            (this.DataContext as ViewModel.UserAdminViewModel).msg = this.msg;
        }
    }
}
