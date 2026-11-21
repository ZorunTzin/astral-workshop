using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using Microsoft.Win32;
namespace AstralWorkshopSetup;
public static class Program {
 static readonly string Target=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs","AstralWorkshop");
 const string RegistryPath=@"Software\Microsoft\Windows\CurrentVersion\Uninstall\AstralWorkshop";
 [STAThread] public static void Main(string[] args){
  if(args.Length==2&&args[0]=="--remove"&&int.TryParse(args[1],out int pid)){try{Process.GetProcessById(pid).WaitForExit(30000);}catch(ArgumentException){}try{if(Directory.Exists(Target)){AssertNoLinks(Target);Directory.Delete(Target,true);}Registry.CurrentUser.DeleteSubKeyTree(RegistryPath,false);foreach(var p in ShortcutPaths())if(File.Exists(p))File.Delete(p);MessageBox.Show("Astral Workshop Launcher removed. Your mod and saved characters were kept.");}catch(Exception e){MessageBox.Show(e.Message,"Uninstall failed");}return;}
  if(args.Contains("--uninstall")){if(MessageBox.Show("Remove Astral Workshop Launcher? The mod and your saved characters will be kept.","Uninstall",MessageBoxButton.YesNo)!=MessageBoxResult.Yes)return;var temp=Path.Combine(Path.GetTempPath(),"AstralWorkshopUninstall-"+Guid.NewGuid().ToString("N")+".exe");File.Copy(Environment.ProcessPath!,temp);Process.Start(new ProcessStartInfo(temp,$"--remove {Environment.ProcessId}"){UseShellExecute=true});return;}
  var app=new Application();var window=new Window{Title="Astral Workshop · Setup",Width=630,Height=420,ResizeMode=ResizeMode.NoResize,WindowStartupLocation=WindowStartupLocation.CenterScreen,Background=new SolidColorBrush(Color.FromRgb(19,14,31)),Foreground=Brushes.AntiqueWhite};var panel=new StackPanel{Margin=new Thickness(36)};window.Content=panel;
  panel.Children.Add(new TextBlock{Text="ASTRAL WORKSHOP",FontFamily=new FontFamily("Georgia"),FontSize=29,Foreground=Brushes.PaleGoldenrod,Margin=new Thickness(0,0,0,20)});
  panel.Children.Add(new TextBlock{Text="Install the launcher for automatic mod updates before you play.\nDiablo II: Resurrected and D2RLoader are required separately.",TextWrapping=TextWrapping.Wrap,FontSize=15,Margin=new Thickness(0,0,0,20)});
  panel.Children.Add(new TextBlock{Text=Target,TextWrapping=TextWrapping.Wrap,Margin=new Thickness(0,0,0,18)});var desktop=new CheckBox{Content="Create desktop shortcut",IsChecked=true,Foreground=Brushes.AntiqueWhite};panel.Children.Add(desktop);var status=new TextBlock{TextWrapping=TextWrapping.Wrap,Margin=new Thickness(0,18,0,5)};panel.Children.Add(status);
  var button=new Button{Content="Install launcher",Padding=new Thickness(20,12,20,12),HorizontalAlignment=HorizontalAlignment.Right};panel.Children.Add(button);
  bool installed=false;button.Click+=async(_,_)=>{if(installed){window.Close();return;}button.IsEnabled=false;status.Text="Installing…";bool shortcut=desktop.IsChecked==true;try{await Task.Run(()=>Install(shortcut));status.Text="Installed. Open Astral Workshop from your Start menu.";installed=true;button.Content="Close";button.IsEnabled=true;}catch(Exception e){status.Text="Installation failed: "+e.Message;button.IsEnabled=true;}};app.Run(window);
 }
 static IEnumerable<string> ShortcutPaths(){yield return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs),"Astral Workshop.lnk");yield return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),"Astral Workshop.lnk");}
 static void AssertNoLinks(string root){if((File.GetAttributes(root)&FileAttributes.ReparsePoint)!=0)throw new IOException("Installation path contains a link.");foreach(var p in Directory.EnumerateFileSystemEntries(root)){if((File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Installation contains a link.");if(Directory.Exists(p))AssertNoLinks(p);}}
 static void Install(bool desktop){if(Process.GetProcessesByName("AstralWorkshop").Length>0)throw new IOException("Close the launcher before installing.");var parent=Path.GetDirectoryName(Target)!;Directory.CreateDirectory(parent);if((File.GetAttributes(parent)&FileAttributes.ReparsePoint)!=0)throw new IOException("Installation parent is a directory link.");if(Directory.Exists(Target))AssertNoLinks(Target);var stage=Target+"-stage-"+Guid.NewGuid().ToString("N");var backup=Target+"-backup-"+Guid.NewGuid().ToString("N");Directory.CreateDirectory(stage);
  try{using var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("launcher.zip")??throw new IOException("Installer payload missing.");using(var zip=new ZipArchive(stream)){foreach(var entry in zip.Entries){var path=Path.GetFullPath(Path.Combine(stage,entry.FullName));if(!path.StartsWith(stage+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new IOException("Invalid installer payload.");if(entry.FullName.EndsWith('/'))continue;Directory.CreateDirectory(Path.GetDirectoryName(path)!);entry.ExtractToFile(path);}}File.Copy(Environment.ProcessPath!,Path.Combine(stage,"Uninstall.exe"));if(Directory.Exists(Target))Directory.Move(Target,backup);try{Directory.Move(stage,Target);}catch{if(Directory.Exists(backup))Directory.Move(backup,Target);throw;}
   using(var key=Registry.CurrentUser.CreateSubKey(RegistryPath)){key.SetValue("DisplayName","Astral Workshop Launcher");key.SetValue("DisplayVersion","0.1.0");key.SetValue("Publisher","Astral Workshop");key.SetValue("InstallLocation",Target);key.SetValue("UninstallString","\""+Path.Combine(Target,"Uninstall.exe")+"\" --uninstall");key.SetValue("NoModify",1);key.SetValue("NoRepair",1);}
   Type type=Type.GetTypeFromProgID("WScript.Shell")!;dynamic shell=Activator.CreateInstance(type)!;foreach(var link in ShortcutPaths().Take(desktop?2:1)){dynamic item=shell.CreateShortcut(link);item.TargetPath=Path.Combine(Target,"AstralWorkshop.exe");item.WorkingDirectory=Target;item.Description="Astral Workshop Launcher";item.Save();}if(Directory.Exists(backup))Directory.Delete(backup,true);
  }finally{if(Directory.Exists(stage))Directory.Delete(stage,true);}
 }
}

