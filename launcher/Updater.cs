using System.IO;
using System.IO.Compression;
using System.Net;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;
namespace AstralWorkshop;
public sealed record ReleaseFile(string Path,string Sha256);
public sealed record ModRelease(string Version,string Archive,string Sha256,ReleaseFile[] Files);
public static class Updater {
 static readonly HttpClient Http = new(){Timeout=TimeSpan.FromMinutes(30)};
 const string Base="https://github.com/ZorunTzin/astral-workshop/releases/latest/download/";
 static Updater(){Http.DefaultRequestHeaders.UserAgent.ParseAdd("AstralWorkshop/0.1.0");}
 internal static string SafePath(string root,string name) {
  if(string.IsNullOrWhiteSpace(name)||name.Contains('\\')||name.Contains(':')||name.StartsWith('/')||name.Split('/').Any(p=>p is "" or "." or ".."||p.EndsWith('.')||p.EndsWith(' ')||p.IndexOfAny(Path.GetInvalidFileNameChars())>=0))throw new InvalidDataException("Invalid update path: "+name);
  var result=Path.GetFullPath(Path.Combine(root,name));
  if(!result.StartsWith(Path.GetFullPath(root)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Update path escapes mod directory.");
  return result;
 }
 static bool Preserved(string p)=>p.StartsWith("d2rloader/config/",StringComparison.OrdinalIgnoreCase);
 static string Hash(string file) {using var stream=File.OpenRead(file);return Convert.ToHexString(SHA256.HashData(stream));}
 static void NoLinks(string root) {if(!Directory.Exists(root))return;foreach(var p in Directory.EnumerateFileSystemEntries(root)){if((File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked files are not supported in the mod folder: "+p);if(Directory.Exists(p))NoLinks(p);}}
 static void CopyTree(string from,string to) {Directory.CreateDirectory(to);foreach(var file in Directory.EnumerateFiles(from))File.Copy(file,Path.Combine(to,Path.GetFileName(file)),true);foreach(var dir in Directory.EnumerateDirectories(from))CopyTree(dir,Path.Combine(to,Path.GetFileName(dir)));}
 internal static void Validate(ModRelease release,string root) {
  if(release.Files.Length==0||release.Files.Length>200000||string.IsNullOrWhiteSpace(release.Version)||release.Version.Length>100||release.Version.Any(char.IsControl)||release.Sha256.Length!=64||!release.Sha256.All(Uri.IsHexDigit)||release.Archive!="workshop-mod.zip")throw new InvalidDataException("Invalid release manifest.");
  var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
  foreach(var f in release.Files){SafePath(root,f.Path);if(!names.Add(f.Path)||f.Sha256.Length!=64||!f.Sha256.All(Uri.IsHexDigit)||!(f.Path.StartsWith("ReimaginedWorkshop.mpq/",StringComparison.Ordinal)||f.Path.StartsWith("d2rloader/",StringComparison.Ordinal)||f.Path=="bank.txt"))throw new InvalidDataException("Invalid release file: "+f.Path);}
 }
 public static async Task Run(string game,IProgress<(string,double)> progress,CancellationToken token) {
  var mods=Path.Combine(game,"Mods");Directory.CreateDirectory(mods);
  if((File.GetAttributes(mods)&FileAttributes.ReparsePoint)!=0)throw new IOException("The Mods folder must not be a directory link.");
  using var guard=new FileStream(Path.Combine(mods,".astral-update.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
  var live=Path.Combine(mods,"ReimaginedWorkshop");
  if(Directory.Exists(live)&&(File.GetAttributes(live)&FileAttributes.ReparsePoint)!=0)throw new IOException("The mod folder must not be a directory link.");
  NoLinks(live);
  progress.Report(("Checking the latest mod release…",2));
  using var response=await Http.GetAsync(Base+"workshop-manifest.json",token);
  if(response.StatusCode==HttpStatusCode.NotFound)throw new IOException("No mod update release has been published yet. The game was not started.");
  response.EnsureSuccessStatusCode();
  var json=await response.Content.ReadAsStringAsync(token);
  var release=JsonSerializer.Deserialize<ModRelease>(json)??throw new InvalidDataException("Empty release manifest.");Validate(release,live);
  var valid=await Task.Run(()=>release.Files.All(f=>{token.ThrowIfCancellationRequested();var p=SafePath(live,f.Path);return File.Exists(p)&&(Preserved(f.Path)||Hash(p).Equals(f.Sha256,StringComparison.OrdinalIgnoreCase));}),token);
  if(valid&&File.Exists(Path.Combine(live,".astral-version"))&&File.ReadAllText(Path.Combine(live,".astral-version"))==release.Version){progress.Report(("Mod is up to date.",100));return;}
  var id=Guid.NewGuid().ToString("N");var stage=Path.Combine(mods,".astral-stage-"+id);var zip=Path.Combine(mods,".astral-download-"+id+".zip");var backup=Path.Combine(mods,".astral-backup-"+id);
  try {
   using(var download=await Http.GetAsync(Base+release.Archive,HttpCompletionOption.ResponseHeadersRead,token)) {
    download.EnsureSuccessStatusCode();await using var input=await download.Content.ReadAsStreamAsync(token);await using var output=File.Create(zip);var buffer=new byte[1024*1024];long total=0;int count;
    while((count=await input.ReadAsync(buffer,token))>0){await output.WriteAsync(buffer.AsMemory(0,count),token);total+=count;var length=download.Content.Headers.ContentLength;progress.Report(($"Downloading mod update: {total/1048576} MB",length>0?5+50.0*total/length.Value:10));}
   }
   progress.Report(("Verifying downloaded update…",58));
   await Task.Run(()=>{if(!Hash(zip).Equals(release.Sha256,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Download checksum failed. Existing installation was not changed.");},token);
   await Task.Run(()=>Prepare(live,stage,zip,release,json,token),token);
   token.ThrowIfCancellationRequested();
   if(System.Diagnostics.Process.GetProcessesByName("D2R").Length>0)throw new IOException("D2R started during the update. Close the game and try again.");
   progress.Report(("Installing verified update…",95));
   if(Directory.Exists(live))Directory.Move(live,backup);
   try{Directory.Move(stage,live);}catch{if(Directory.Exists(backup))Directory.Move(backup,live);throw;}
   // Retain the previous complete installation as a recovery backup.
   progress.Report(("Update installed.",100));
  } finally {if(File.Exists(zip))File.Delete(zip);if(Directory.Exists(stage))Directory.Delete(stage,true);}
 }
 internal static void Prepare(string live,string stage,string zip,ModRelease release,string json,CancellationToken token) {
  Validate(release,stage);Directory.CreateDirectory(stage);if(Directory.Exists(live))CopyTree(live,stage);
  var prior=Path.Combine(stage,".astral-manifest.json");
  if(File.Exists(prior)){var old=JsonSerializer.Deserialize<ModRelease>(File.ReadAllText(prior))??throw new InvalidDataException("Invalid installed manifest.");Validate(old,stage);var current=release.Files.Select(f=>f.Path).ToHashSet(StringComparer.OrdinalIgnoreCase);foreach(var f in old.Files){if(!current.Contains(f.Path)&&!Preserved(f.Path))File.Delete(SafePath(stage,f.Path));}}
  using var archive=ZipFile.OpenRead(zip);var expected=release.Files.ToDictionary(f=>f.Path,StringComparer.OrdinalIgnoreCase);var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);long expanded=0;
  foreach(var entry in archive.Entries){token.ThrowIfCancellationRequested();if(entry.FullName.EndsWith('/'))continue;var destination=SafePath(stage,entry.FullName);if(!expected.TryGetValue(entry.FullName,out var spec)||!seen.Add(entry.FullName)||((entry.ExternalAttributes>>16)&0xF000)==0xA000)throw new InvalidDataException("Unexpected archive entry.");expanded+=entry.Length;if(expanded>32L*1024*1024*1024)throw new InvalidDataException("Update is too large.");var temp=destination+".astral-new";Directory.CreateDirectory(Path.GetDirectoryName(destination)!);entry.ExtractToFile(temp,true);if(!Hash(temp).Equals(spec.Sha256,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("File checksum failed: "+entry.FullName);if(Preserved(entry.FullName)&&File.Exists(destination))File.Delete(temp);else File.Move(temp,destination,true);}
  if(seen.Count!=expected.Count)throw new InvalidDataException("Update archive is incomplete.");
  File.WriteAllText(prior,json);File.WriteAllText(Path.Combine(stage,".astral-version"),release.Version);
 }
}
