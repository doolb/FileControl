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

namespace fcapi_wpf {
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {
        public MainWindow () {
            InitializeComponent ();
        }

        private void HamburgerMenu_ItemClick ( object sender, ItemClickEventArgs e ) {
            HamburgerMenu hm = sender as HamburgerMenu;

            hm.Content = e.ClickedItem;
            hm.IsPaneOpen = false;
        }
    }
}
