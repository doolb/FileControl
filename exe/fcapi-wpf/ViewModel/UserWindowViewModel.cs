using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using fcapi_wpf.View;
using System.Windows.Navigation;
using System.Windows.Controls;

namespace fcapi_wpf.ViewModel {
    class UserWindowViewModel : MVVM.ViewModel {

        public Page page { get { return _page; } set { _page = value; RaisePropertyChanged (); } }
        private Page _page;

        Dictionary<Type, Page> pages = new Dictionary<Type, Page> ();

        public UserViewModel[] users { get { return _users; } set { _users = value; RaisePropertyChanged (); } }
        private UserViewModel[] _users;

        public UserWindowViewModel () {
            users = new UserViewModel[5];
            for (int i=0; i<users.Length; i++) {
                users[i] = new UserViewModel {
                    vm = this,
                    img = "https://2-im.guokr.com/FKrbvmmuROVn-mFSEv9NzeuIF3LmaiFUnUP8kRCvndCgAAAAoAAAAEpQ.jpg?imageView2/1/w/69/h/69",
                    name ="test"
                };
            }
            Show<UserLoginPage> (users[0]);
        }

        public void Show<T> ( MVVM.ViewModel viewModel ) where T : Page {
            if (!pages.ContainsKey (typeof (T)))
                pages.Add (typeof (T), Activator.CreateInstance<T> ());

            page = pages[typeof (T)];
            page.DataContext = viewModel;
        }

        public void ShowAllUser () {
            Show<UserSelectPage> (this);
        }


        public class UserViewModel : MVVM.ViewModel {

            public UserWindowViewModel vm;

            public string img { get { return _img; } set { _img = value; RaisePropertyChanged (); } }
            private string _img;

            public string name { get { return _name; } set { _name = value; RaisePropertyChanged (); } }
            private string _name;


            public Command loginCmd {
                get {
                    return _loginCmd ??(_loginCmd = new Command {
                        ExecuteDelegate = _ => {

                        }
                    });
                }
            }
            private Command _loginCmd;

            public Command showUserCmd {
                get {
                    return _showUserCmd ?? (_showUserCmd = new Command {
                        ExecuteDelegate = _ => vm.ShowAllUser ()
                    });
                }
            }
            private Command _showUserCmd;

            public Command selUserCmd {
                get {
                    return _selUserCmd ??(_selUserCmd = new Command {
                        ExecuteDelegate = _ => vm.Show<UserLoginPage> (this)
                    });
                }
            }
            private Command _selUserCmd;

        }
    }
}
