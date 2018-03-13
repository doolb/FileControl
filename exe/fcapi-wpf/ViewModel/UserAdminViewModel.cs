using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using MVVM;
using System.Windows;
using FCApi;
using System.IO;
using System.Xml.Serialization;
using Newtonsoft.Json;
using System.Collections.ObjectModel;

namespace fcapi_wpf.ViewModel {
    class UserAdminViewModel : MVVM.ViewModel {

        /// <summary>
        /// exit
        /// </summary>
        public Command exitCmd { get { return _exitCmd ?? (_exitCmd = new Command { ExecuteDelegate = _ => Application.Current.Shutdown () }); } }
        private Command _exitCmd;

        /// <summary>
        /// driver work root
        /// </summary>
        public string workRoot { get { return _workRoot; } set { _workRoot = value; RaisePropertyChanged (); } }
        private string _workRoot;


        /// <summary>
        /// load driver
        /// </summary>
        public Command loadCmd { get { return _loadCmd ?? (_loadCmd = new Command { ExecuteDelegate = _ => { FC.Load (); refresh (); }, CanExecuteDelegate = _ => FC.installed && !FC.loaded }); } }
        private Command _loadCmd;

        /// <summary>
        /// unload driver
        /// </summary>
        public Command unloadCmd { get { return _unloadCmd ?? (_unloadCmd = new Command { ExecuteDelegate = _ => { FC.Unload (); refresh (); }, CanExecuteDelegate = _ => FC.installed && FC.loaded }); } }
        private Command _unloadCmd;

        public string workLetter { get { return _workLetter; } set { _workLetter = value; RaisePropertyChanged (); } }
        private string _workLetter;

        /// <summary>
        /// the volume can be set as work root
        /// </summary>
        public ObservableCollection<Volume> volumes { get { return _volumes; } set { _volumes = value; RaisePropertyChanged (); } }
        private ObservableCollection<Volume> _volumes = new ObservableCollection<Volume> ();
        public int selVolume { get { return _selVolume; } set { _selVolume = value; RaisePropertyChanged (); } }
        private int _selVolume;
        /// <summary>
        /// the volume dont has user key or work root
        /// </summary>
        public ObservableCollection<Volume> nomVolumes { get { return _nomVolumes; } set { _nomVolumes = value; RaisePropertyChanged (); } }
        private ObservableCollection<Volume> _nomVolumes = new ObservableCollection<Volume> ();

        /// <summary>
        /// the volume has user key
        /// </summary>
        public ObservableCollection<Volume> userVolumes { get { return _userVolumes; } set { _userVolumes = value; RaisePropertyChanged (); } }
        private ObservableCollection<Volume> _userVolumes = new ObservableCollection<Volume> ();

        /// <summary>
        /// the volume can add new user
        /// </summary>
        public ObservableCollection<Volume> remVolumes { get { return _remVolumes; } set { _remVolumes = value; RaisePropertyChanged (); } }
        private ObservableCollection<Volume> _remVolumes = new ObservableCollection<Volume> ();
        public Command setWorkRootCmd {
            get {
                return _setWorkRootCmd??(_setWorkRootCmd = new Command {
                    ExecuteDelegate = _ => { if (FC.setWorkRoot (volumes[selVolume].letter[0])) refresh (); },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _setWorkRootCmd;

        public Command delWorkRootCmd {
            get {
                return _delWorkRootCmd??(_delWorkRootCmd = new Command {
                    ExecuteDelegate = _ => { if (FC.setWorkRoot ('\0')) refresh (); },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _delWorkRootCmd;


        #region group
        public ObservableCollection<Group> groups { get { return _groups; } set { _groups = value; RaisePropertyChanged (); } }
        private ObservableCollection<Group> _groups = new ObservableCollection<Group> ();

        public int selGroup { get { return _selGroup; } set { _selGroup = value; RaisePropertyChanged (); } }
        private int _selGroup;
        public Command addGroupCmd {
            get {
                return _addGroupCmd ?? (_addGroupCmd = new Command {
                    ExecuteDelegate = n => {
                        string nm = n as string;
                        if (string.IsNullOrEmpty (nm))
                            return;
                        if (!groups.Any (g => g.name == nm)) {
                            groups.Add (new Group (n as string));
                            if (groups.Count == 1) { selVolume = 0; }
                            RaisePropertyChanged ("groups");
                            saveGroup ();
                        }
                        else
                            MessageBox.Show (Language ("group_existed"));
                    },
                    CanExecuteDelegate = _ => FC.isopen
                });
            }
        }
        private Command _addGroupCmd;
        public Command delGroupCmd {
            get {
                return _delGroupCmd ?? (_delGroupCmd = new Command {
                    ExecuteDelegate = n => {
                        if (MessageBox.Show (Language ("del_group_ok?"), "ok?", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                            groups.RemoveAt (selGroup);
                        RaisePropertyChanged ("groups");
                    },
                    CanExecuteDelegate = _ => FC.isopen && groups.Count > 0
                });
            }
        }
        private Command _delGroupCmd;
        #endregion

        #region user

        public string newUserName { get { return _newUserName; } set { _newUserName = value; RaisePropertyChanged (); } }
        private string _newUserName;
        public string newUserPassword { get { return _newUserPassword; } set { _newUserPassword = value; RaisePropertyChanged (); } }
        private string _newUserPassword;

        public int newUserGroup { get { return _newUserGroup; } set { _newUserGroup = value; RaisePropertyChanged (); } }
        private int _newUserGroup;

        public int newUserVolume { get { return _newUserVolume; } set { _newUserVolume = value; RaisePropertyChanged (); } }
        private int _newUserVolume;


        public Command newUserCmd {
            get {
                return _newUserCmd ??(_newUserCmd = new Command {
                    CanExecuteDelegate = _ => FC.isopen && nomVolumes.Count > 0
                });
            }
        }
        private Command _newUserCmd;

        public Command delUserCmd {
            get {
                return _delUserCmd ??(_delUserCmd = new Command {
                    CanExecuteDelegate = _ => FC.isopen && userVolumes.Count > 0
                });
            }
        }
        private Command _delUserCmd;

        #endregion


        public UserAdminViewModel () {
            refresh ();
        }

        void refresh () {
            //workRoot = FC.WorkRootLetter != 0 ? FC.WorkRootLetter.ToString () : Language ("empty");

            if (!FC.installed) { return; }
            bool myopen = false;
            if (!FC.loaded) { FC.Open (); myopen = true; }

            queryVolume ();
            loadGroup ();
            RaisePropertyChanged ("groups");



            if (myopen)
                FC.Close ();
        }

        private Volume[] allVolume;

        void queryVolume () {
            workLetter = Language ("empty");

            string v = FC.getVolumes ();
            if (v == null) {
                return;
            }

            allVolume = new Volume[v.Length / 2];
            for (int i=0; i < v.Length / 2; i++) {
                allVolume[i] = new Volume ();
                allVolume[i].letter = v[i * 2].ToString ();

                char state = v[i * 2 +1];
                if (state == 'W') { allVolume[i].isWorkRoot = true; continue; }
                else if (state == 'K') { allVolume[i].isKeyRoot = true; continue; }
                else if (state == 'U') { allVolume[i].isHasUser = true; continue; }
                else if (state == 'R') { allVolume[i].isRemote = true; continue; }
            }

            volumes.Clear ();
            remVolumes.Clear ();
            nomVolumes.Clear ();
            userVolumes.Clear ();
            for (int i=0; i<allVolume.Length; i++) {
                if (allVolume[i].isWorkRoot) { volumes.Add (allVolume[i]); workLetter = allVolume[i].letter; }
                else if (allVolume[i].isHasUser || allVolume[i].isKeyRoot) {
                    userVolumes.Add (allVolume[i]);
                }
                else if (allVolume[i].isRemote) {
                    remVolumes.Add (allVolume[i]);
                }
                else {
                    nomVolumes.Add (allVolume[i]);
                    volumes.Add (allVolume[i]);
                }
            }
        }

        void loadGroup () {
            try {
                JsonSerializer js = new JsonSerializer ();
                js.NullValueHandling = NullValueHandling.Ignore;

                using (StreamReader sr = new StreamReader ("../group.json", Encoding.UTF8))
                using (JsonTextReader tr = new JsonTextReader (sr)) {
                    groups = js.Deserialize<ObservableCollection<Group>> (tr);
                    if (groups == null) { groups = new ObservableCollection<Group> (); }
                }
            }
            catch (Exception _e) { }
        }

        void saveGroup () {
            try {
                JsonSerializer js = new JsonSerializer ();
                js.NullValueHandling = NullValueHandling.Ignore;

                using (StreamWriter sw = new StreamWriter ("../group.json", false, Encoding.UTF8))
                using (JsonTextWriter tw = new JsonTextWriter (sw)) {
                    js.Serialize (tw, groups);
                }
            }
            catch (Exception _e) { }
        }

        public class Volume {
            public string letter { get; set; }
            public bool isKeyRoot { get; set; }
            public bool isHasUser { get; set; }
            public bool isWorkRoot { get; set; }
            public bool isRemote { get; set; }
        }

        public class Group {
            public string name { get; set; }
            public Guid guid { get; set; }

            public Group ( string name ) {
                this.name = name;
                this.guid = Guid.NewGuid ();
            }
        }
    }
}
