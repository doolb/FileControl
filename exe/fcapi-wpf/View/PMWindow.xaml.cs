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
using System.Windows.Shapes;

using fcapi_wpf.ViewModel;

namespace fcapi_wpf.View {
    /// <summary>
    /// Interaction logic for PMWindow.xaml
    /// </summary>
    public partial class PMWindow : Window {
        public PMWindow ( string path ) {
            InitializeComponent ();

            (this.DataContext as PMWindowViewModel).msg = this.msg;
            (this.DataContext as PMWindowViewModel).open (path);
        }
    }
}
