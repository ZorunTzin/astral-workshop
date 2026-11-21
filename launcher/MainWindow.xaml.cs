using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Windows;
using Microsoft.Win32;
namespace AstralWorkshop;
public sealed record Preferences(string GameDirectory = "", string ExtraArguments = "");
public partial class MainWindow : Window {
 internal const string Repository = "https://github.com/ZorunTzin/astral-workshop";
 static readonly string SettingsDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AstralWorkshop");
 static readonly string SettingsFile = Path.Combine(SettingsDir, "settings.json");
 CancellationTokenSource? cancellation;
 public MainWindow() { InitializeComponent(); try { if(File.Exists(SettingsFile)) { var p=JsonSerializer.Deserialize<Preferences>(File.ReadAllText(SettingsFile)); GamePath.Text=p?.GameDirectory??""; Arguments.Text=p?.ExtraArguments??""; } } catch(Exception e) { Status.Text="Settings could not be loaded: "+e.Message; } RefreshVersion(); Closing += (_,e)=>{ if(cancellation!=null) { cancellation.Cancel(); e.Cancel=true; Status.Text="Cancelling update. Please wait before closing."; } else Save(); }; }
 void Save() { Directory.CreateDirectory(SettingsDir); var temp=SettingsFile+".tmp"; File.WriteAllText(temp,JsonSerializer.Serialize(new Preferences(GamePath.Text,Arguments.Text))); File.Move(temp,SettingsFile,true); }
 void RefreshVersion() { var file=Path.Combine(GamePath.Text,"Mods","ReimaginedWorkshop",".astral-version"); VersionLabel.Text=File.Exists(file)?"Installed mod: "+File.ReadAllText(file):"Mod not installed by this launcher yet"; if(!string.IsNullOrEmpty(GamePath.Text)) Status.Text="Ready. Play checks for updates before starting the game."; }
 static void Open(string path) { Process.Start(new ProcessStartInfo(path){UseShellExecute=true}); }
 async Task Update(bool launch) {
  if(cancellation!=null)return;
  try {
   if(!File.Exists(Path.Combine(GamePath.Text,"D2R.exe"))||!File.Exists(Path.Combine(GamePath.Text,"D2RLoader.exe"))) throw new InvalidOperationException("Select a D2R folder containing D2R.exe and D2RLoader.exe in Settings.");
   if(Process.GetProcessesByName("D2R").Length>0)throw new InvalidOperationException("Close Diablo II: Resurrected before updating.");
   Save(); SettingsPanel.Visibility=Visibility.Collapsed; PlayButton.IsEnabled=false; CancelButton.Visibility=Visibility.Visible; cancellation=new();
   var reporter=new Progress<(string Message,double Percent)>(p=>{Status.Text=p.Message;Progress.Value=p.Percent;});
   await Updater.Run(GamePath.Text,reporter,cancellation.Token);
   RefreshVersion();
   if(launch) { cancellation.Token.ThrowIfCancellationRequested(); Process.Start(new ProcessStartInfo(Path.Combine(GamePath.Text,"D2RLoader.exe")){WorkingDirectory=GamePath.Text,Arguments="-mod ReimaginedWorkshop -txt "+Arguments.Text,UseShellExecute=true}); Status.Text="Game started."; } else Status.Text="Mod files verified and up to date.";
  } catch(OperationCanceledException) {Status.Text="Update cancelled. The game was not started.";}
    catch(Exception e) {Status.Text=e.Message;}
  finally {cancellation?.Dispose();cancellation=null;PlayButton.IsEnabled=true;CancelButton.Visibility=Visibility.Collapsed;}
 }
 async void Play_Click(object s,RoutedEventArgs e)=>await Update(true);
 async void Repair_Click(object s,RoutedEventArgs e)=>await Update(false);
 void Cancel_Click(object s,RoutedEventArgs e)=>cancellation?.Cancel();
 void Browse_Click(object s,RoutedEventArgs e) { var d=new OpenFolderDialog{Title="Select Diablo II: Resurrected installation folder"}; if(d.ShowDialog()==true) {GamePath.Text=d.FolderName;Save();RefreshVersion();} }
 void Settings_Click(object s,RoutedEventArgs e) {if(cancellation==null)SettingsPanel.Visibility=Visibility.Visible;}
 void CloseSettings_Click(object s,RoutedEventArgs e) {Save();SettingsPanel.Visibility=Visibility.Collapsed;}
 void Page(int p) {PreferencesPage.Visibility=p==0?Visibility.Visible:Visibility.Collapsed;GamePage.Visibility=p==1?Visibility.Visible:Visibility.Collapsed;AboutPage.Visibility=p==2?Visibility.Visible:Visibility.Collapsed;}
 void Preferences_Click(object s,RoutedEventArgs e)=>Page(0);
 void Game_Click(object s,RoutedEventArgs e)=>Page(1);
 void About_Click(object s,RoutedEventArgs e)=>Page(2);
 void Compendium_Click(object s,RoutedEventArgs e)=>Open("https://astral-workshop-compendium.chattyactor2.chatgpt.site");
 void Github_Click(object s,RoutedEventArgs e)=>Open(Repository);
 void Releases_Click(object s,RoutedEventArgs e)=>Open(Repository+"/releases");
 void OpenGame_Click(object s,RoutedEventArgs e) {if(Directory.Exists(GamePath.Text))Open(GamePath.Text);}
 void OpenSettings_Click(object s,RoutedEventArgs e) {Directory.CreateDirectory(SettingsDir);Open(SettingsDir);}
 void OpenSaves_Click(object s,RoutedEventArgs e) {
  var profile="ReimaginedWorkshop";
  var metadata=Path.Combine(GamePath.Text,"Mods","ReimaginedWorkshop","ReimaginedWorkshop.mpq","modinfo.json");
  if(File.Exists(metadata)) {
   using var doc=JsonDocument.Parse(File.ReadAllText(metadata));
   if(doc.RootElement.TryGetProperty("savepath",out var value)) {
    var candidate=(value.GetString()??"").TrimEnd('/','\\');
    if(candidate.Length>0 && candidate.All(c=>char.IsLetterOrDigit(c)||c=='-'||c=='_'))profile=candidate;
   }
  }
  var p=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),"Saved Games","Diablo II Resurrected","mods",profile);
  if(Directory.Exists(p))Open(p);else Status.Text="No saved characters folder exists yet.";
 }
}
