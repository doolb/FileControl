﻿using System;
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

namespace fcapi_wpf.View {
    /// <summary>
    /// Interaction logic for NameWindow.xaml
    /// </summary>
    public partial class UserWindow : Window {
        public UserWindow () {
            InitializeComponent ();
            tool = new Tool.NotifyTool ();
        }
        Tool.NotifyTool tool;
    }
}