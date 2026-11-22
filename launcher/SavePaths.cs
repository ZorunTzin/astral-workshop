using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
namespace AstralWorkshop;
internal static class SavePaths {
 [DllImport("shell32.dll")] static extern int SHGetKnownFolderPath([In] ref Guid id,uint flags,IntPtr token,out IntPtr path);
 internal static string SavedGames() {
  var id=new Guid("4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4");
  int result=SHGetKnownFolderPath(ref id,0,IntPtr.Zero,out var pointer);
  try {Marshal.ThrowExceptionForHR(result);return Marshal.PtrToStringUni(pointer)??throw new IOException("Saved Games folder unavailable.");}finally{Marshal.FreeCoTaskMem(pointer);}
 }
 internal static string Resolve(string savedGames,string metadata) {
  var profile="ReimaginedWorkshop";
  if(File.Exists(metadata)) {
   using var doc=JsonDocument.Parse(File.ReadAllText(metadata));
   if(doc.RootElement.TryGetProperty("savepath",out var value))profile=(value.GetString()??"").TrimEnd('/','\\');
  }
  if(string.IsNullOrWhiteSpace(profile)||profile is "." or ".."||profile.IndexOfAny(Path.GetInvalidFileNameChars())>=0||profile.Contains('/')||profile.Contains('\\'))throw new InvalidDataException("Invalid mod savepath.");
  return Path.Combine(savedGames,"Diablo II Resurrected","mods",profile);
 }
}
