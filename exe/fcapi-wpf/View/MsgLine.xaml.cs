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
            if (instance == null) {
                instance = this;
            }
            else {
                this.IsEnabled = false;
                return;
            }
            InitializeComponent ();
            vm = this.DataContext as ViewModel.MsgLineViewModel;
        }

        private ViewModel.MsgLineViewModel vm;

        private static MsgLine instance;

        public static void Show ( string msg ) {
            Application.Current.Dispatcher.BeginInvoke (new Action (() => {

                if (MsgLine.instance == null) { return; }

                (MsgLine.instance.Resources["sb_show"] as Storyboard).Seek (TimeSpan.Zero);
                (MsgLine.instance.Resources["sb_show"] as Storyboard).Begin ();
                string format = Properties.Resources.ResourceManager.GetString (msg);
                MsgLine.instance.vm.msg = format ?? msg;
            }));
        }
    }
}
