using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

using WinForms = System.Windows.Forms;

namespace Tool {
    public class NotifyTool {

        public Window win { get { return Application.Current.MainWindow; } }


        public NotifyTool () {
            icon ();

            win.Closing +=win_Closing;
        }

        void win_Closing ( object sender, System.ComponentModel.CancelEventArgs e ) {
            e.Cancel = true;
            Application.Current.MainWindow.WindowState = WindowState.Minimized;
            Application.Current.MainWindow.ShowInTaskbar = false;
        }

        private WinForms.NotifyIcon notifyIcon;


        void notifier_MouseDown ( object sender, WinForms.MouseEventArgs e ) {
            Application.Current.MainWindow.WindowState = WindowState.Normal;
            Application.Current.MainWindow.ShowInTaskbar = true;
            Application.Current.MainWindow.Activate ();
            Application.Current.MainWindow.Topmost = true;
        }

        private void icon () {

            this.notifyIcon = new WinForms.NotifyIcon ();

            this.notifyIcon.BalloonTipText = "Hello, 文件监视器"; //设置程序启动时显示的文本

            this.notifyIcon.Text = "文件监视器";//最小化到托盘时，鼠标点击时显示的文本

            this.notifyIcon.Icon = new System.Drawing.Icon ("./res/icon-32.ico");//程序图标

            this.notifyIcon.Visible = true;

            notifyIcon.MouseDoubleClick += notifier_MouseDown;

            this.notifyIcon.ShowBalloonTip (1000);

        }
    }
}
