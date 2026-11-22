using AstralWorkshop;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
var root=Path.Combine(Path.GetTempPath(),"astral-tests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);int passed=0;
string Hash(string s)=>Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(s)));
void Check(bool value,string name){if(!value)throw new Exception(name);Console.WriteLine("PASS "+name);passed++;}
void Reject(Action action,string name){try{action();}catch(InvalidDataException){Check(true,name);return;}throw new Exception("Expected rejection: "+name);}
try{
 foreach(var p in new[]{"../escape","/absolute","C:/escape","a/../x","a\\b","a./x","a//b"})Reject(()=>Updater.SafePath(root,p),"reject "+p);
 var live=Path.Combine(root,"live");Directory.CreateDirectory(Path.Combine(live,"d2rloader","config"));File.WriteAllText(Path.Combine(live,"d2rloader","config","user.toml"),"custom");File.WriteAllText(Path.Combine(live,"user-note.txt"),"keep");
 var zip=Path.Combine(root,"mod.zip");var files=new[]{new ReleaseFile("ReimaginedWorkshop.mpq/data/test.txt",Hash("new-data")),new ReleaseFile("d2rloader/config/user.toml",Hash("default"))};
 using(var z=ZipFile.Open(zip,ZipArchiveMode.Create)){using(var w=new StreamWriter(z.CreateEntry(files[0].Path).Open()))w.Write("new-data");using(var w=new StreamWriter(z.CreateEntry(files[1].Path).Open()))w.Write("default");}
 var release=new ModRelease("test","workshop-mod.zip",new string('0',64),files);var stage=Path.Combine(root,"stage");Updater.Prepare(live,stage,zip,release,JsonSerializer.Serialize(release),default);
 Check(File.ReadAllText(Path.Combine(stage,"ReimaginedWorkshop.mpq/data/test.txt"))=="new-data","new payload staged");Check(File.ReadAllText(Path.Combine(stage,"d2rloader/config/user.toml"))=="custom","user configuration preserved");Check(File.ReadAllText(Path.Combine(stage,"user-note.txt"))=="keep","unmanaged files preserved");Check(!Directory.Exists(Path.Combine(live,"ReimaginedWorkshop.mpq")),"live installation untouched during staging");
 var wrong=release with{Files=[files[0] with{Sha256=new string('f',64)},files[1]]};Reject(()=>Updater.Prepare(live,Path.Combine(root,"bad"),zip,wrong,JsonSerializer.Serialize(wrong),default),"corrupt payload rejected");
 var incomplete=release with{Files=[..files,new("ReimaginedWorkshop.mpq/data/missing",Hash("x"))]};Reject(()=>Updater.Prepare(live,Path.Combine(root,"missing"),zip,incomplete,JsonSerializer.Serialize(incomplete),default),"missing file rejected");
 var duplicate=release with{Files=[files[0],files[0]]};Reject(()=>Updater.Validate(duplicate,root),"duplicate manifest paths rejected");
 using var cancel=new CancellationTokenSource();cancel.Cancel();try{Updater.Prepare(live,Path.Combine(root,"cancel"),zip,release,"",cancel.Token);throw new Exception("Cancellation ignored");}catch(OperationCanceledException){Check(true,"cancel before commit");}
 var old=release with{Files=[new("ReimaginedWorkshop.mpq/data/obsolete.txt",Hash("old"))]};Directory.CreateDirectory(Path.Combine(live,"ReimaginedWorkshop.mpq/data"));File.WriteAllText(Path.Combine(live,"ReimaginedWorkshop.mpq/data/obsolete.txt"),"old");File.WriteAllText(Path.Combine(live,".astral-manifest.json"),JsonSerializer.Serialize(old));var stage2=Path.Combine(root,"stage2");Updater.Prepare(live,stage2,zip,release,JsonSerializer.Serialize(release),default);Check(!File.Exists(Path.Combine(stage2,"ReimaginedWorkshop.mpq/data/obsolete.txt")),"obsolete managed file removed");Check(File.Exists(Path.Combine(live,"ReimaginedWorkshop.mpq/data/obsolete.txt")),"old installation retained");
 var metadata=Path.Combine(root,"modinfo.json");
 File.WriteAllText(metadata,"{\"savepath\":\"ReimaginedWorkshop-v2/\"}");
 Check(SavePaths.Resolve(Path.Combine(root,"redirected-saves"),metadata)==Path.Combine(root,"redirected-saves","Diablo II Resurrected","mods","ReimaginedWorkshop-v2"),"redirected Saved Games and versioned mod profile");
 File.WriteAllText(metadata,"{\"savepath\":\"../other/\"}");
 Reject(()=>SavePaths.Resolve(root,metadata),"savepath traversal rejected");
 Check(Directory.Exists(SavePaths.SavedGames()),"Windows Saved Games known folder resolves");
 Console.WriteLine($"{passed} checks passed");
}finally{Directory.Delete(root,true);}
