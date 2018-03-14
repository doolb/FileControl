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
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using fcapi_wpf.Properties;
namespace fcapi_wpf.View {
    /// <summary>
    /// Interaction logic for MsgLine.xaml
    /// </summary>
    public partial class MsgLine : UserControl {
        public MsgLine () {
            InitializeComponent ();
            vm = this.DataContext as ViewModel.MsgLineViewModel;
        }

        private ViewModel.MsgLineViewModel vm;

        public void Show ( string msg ) {

            Application.Current.Dispatcher.BeginInvoke (new Action (() => {
                (this.Resources["sb_show"] as Storyboard).Seek (TimeSpan.Zero);
                (this.Resources["sb_show"] as Storyboard).Begin ();
                string format = Properties.Resources.ResourceManager.GetString (msg);
                this.vm.msg = format ?? msg;
            }));
        }
    }
}
