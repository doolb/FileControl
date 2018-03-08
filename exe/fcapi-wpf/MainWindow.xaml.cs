using MahApps.Metro.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using WinForms = System.Windows.Forms;

namespace fcapi_wpf {
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {
        public MainWindow () {
            InitializeComponent ();
            icon ();
            this.Closing+=MainWindow_Closing;
        }

        void MainWindow_Closing ( object sender, System.ComponentModel.CancelEventArgs e ) {
            e.Cancel = true;
            WindowState = WindowState.Minimized;
            this.ShowInTaskbar = false;
        }

        private WinForms.NotifyIcon notifyIcon;

        private void icon () {

            this.notifyIcon = new WinForms.NotifyIcon ();

            this.notifyIcon.BalloonTipText = "Hello, 文件监视器"; //设置程序启动时显示的文本

            this.notifyIcon.Text = "文件监视器";//最小化到托盘时，鼠标点击时显示的文本

            this.notifyIcon.Icon = new System.Drawing.Icon ("res/icon-32.ico");//程序图标

            this.notifyIcon.Visible = true;

            notifyIcon.MouseDoubleClick += notifier_MouseDown;

            this.notifyIcon.ShowBalloonTip (1000);

        }


        void notifier_MouseDown ( object sender, WinForms.MouseEventArgs e ) {
            WindowState = WindowState.Normal;
            this.ShowInTaskbar = true;
            this.Activate ();
            this.Topmost = true;
        }


        private void HamburgerMenu_ItemClick ( object sender, ItemClickEventArgs e ) {
            HamburgerMenu hm = sender as HamburgerMenu;

            hm.Content = e.ClickedItem;
            hm.IsPaneOpen = false;
        }
    }
}
