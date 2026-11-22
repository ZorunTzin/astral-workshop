using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows;
namespace AstralWorkshop;
public partial class App : Application {
 Mutex? instance;
 [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr window);
 [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr window,int command);
 internal static void Log(Exception error) { try { var dir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"AstralWorkshop");Directory.CreateDirectory(dir);File.AppendAllText(Path.Combine(dir,"launcher.log"),DateTimeOffset.Now+" "+error+Environment.NewLine); } catch { } }
 protected override void OnStartup(StartupEventArgs e) {
  instance=new Mutex(true,@"Local\AstralWorkshop.Launcher",out bool first);
  if(!first) {
   foreach(var p in Process.GetProcessesByName("AstralWorkshop")) using(p) {try {if(p.Id!=Environment.ProcessId&&p.MainWindowHandle!=IntPtr.Zero){ShowWindow(p.MainWindowHandle,9);SetForegroundWindow(p.MainWindowHandle);break;}}catch {}}
   Shutdown();return;
  }
  DispatcherUnhandledException+=(_,args)=>{Log(args.Exception);args.Handled=true;MessageBox.Show("The operation failed: "+args.Exception.Message+"\nDetails: %LOCALAPPDATA%/AstralWorkshop/launcher.log","Astral Workshop");};
  AppDomain.CurrentDomain.UnhandledException+=(_,args)=>{if(args.ExceptionObject is Exception error)Log(error);};
  base.OnStartup(e);MainWindow=new MainWindow();MainWindow.Show();
 }
 protected override void OnExit(ExitEventArgs e) {instance?.Dispose();base.OnExit(e);}
}
