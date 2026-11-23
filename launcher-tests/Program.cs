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
 // Density is derived from verified release originals, never multiplied cumulatively.
 var densityRoot=Path.Combine(root,"density");
 string table="Name\tMonDen\tMonDen(N)\tMonDen(H)\tMonUMin\r\nTown\t0\t0\t0\t0\r\nField\t1800\t100\t200\t3\r\nTrial\t0\t\t0\t7\r\n";
 var densityFiles=DensityModifier.Paths.Select(path=>new ReleaseFile(path,Hash(table))).ToArray();
 var densityRelease=new ModRelease("density-test","workshop-mod.zip",new string('0',64),densityFiles);
 foreach(var f in densityFiles){var path=Updater.SafePath(densityRoot,f.Path);Directory.CreateDirectory(Path.GetDirectoryName(path)!);File.WriteAllText(path,table);}
 for(int multiplier=1;multiplier<=5;multiplier++){
  DensityModifier.Apply(densityRoot,densityRelease,multiplier);DensityModifier.Apply(densityRoot,densityRelease,multiplier);
  foreach(var f in densityFiles){var text=File.ReadAllText(Updater.SafePath(densityRoot,f.Path));Check(text.Contains($"Field\t{1800*multiplier}\t{100*multiplier}\t{200*multiplier}\t3\r\n")&&text.Contains("Trial\t0\t\t0\t7"),$"density x{multiplier}: all difficulties, fixed spawns untouched, no compounding");Check(DensityModifier.Matches(densityRoot,f),"modified density passes exact release-derived integrity check");}
 }
 DensityModifier.Apply(densityRoot,densityRelease,2);Check(File.ReadAllText(Updater.SafePath(densityRoot,densityFiles[0].Path)).Contains("Field\t3600\t200\t400"),"switch x5 to x2 uses original baseline");
 DensityModifier.Apply(densityRoot,densityRelease,1);Check(densityFiles.All(f=>File.ReadAllText(Updater.SafePath(densityRoot,f.Path))==table),"x1 restores exact original bytes");
 byte[] bom=[239,187,191,..Encoding.UTF8.GetBytes(table)];Check(DensityModifier.Transform(bom,2).AsSpan().StartsWith(new byte[]{239,187,191}),"UTF-8 BOM preserved");
 Reject(()=>DensityModifier.Transform(Encoding.UTF8.GetBytes(table),6),"unsupported density rejected");
 Reject(()=>DensityModifier.Transform(Encoding.UTF8.GetBytes("bad\nrow\n"),2),"invalid density table rejected");
 Reject(()=>DensityModifier.Transform(Encoding.UTF8.GetBytes(table.Replace("1800","3000")),5),"density overflow rejected");
 var first=Updater.SafePath(densityRoot,densityFiles[0].Path);File.AppendAllText(first,"corrupt");Check(!DensityModifier.Matches(densityRoot,densityFiles[0]),"modified table corruption triggers repair");
 Reject(()=>DensityModifier.Apply(densityRoot,densityRelease,2),"corrupt baseline is never reused");File.WriteAllText(first,table);
 var nextTable=table.Replace("1800","1500");var nextRelease=densityRelease with{Files=DensityModifier.Paths.Select(path=>new ReleaseFile(path,Hash(nextTable))).ToArray()};
 Check(!DensityModifier.Matches(densityRoot,nextRelease.Files[0]),"new release checksum invalidates old baseline");
 foreach(var f in nextRelease.Files)File.WriteAllText(Updater.SafePath(densityRoot,f.Path),nextTable);
 DensityModifier.Apply(densityRoot,nextRelease,3);Check(File.ReadAllText(first).Contains("Field\t4500\t300\t600"),"updated release uses its new baseline");
 var oldPreferences=JsonSerializer.Deserialize<Preferences>("{\"GameDirectory\":\"D2R\",\"ExtraArguments\":\"-w\"}")!;
 Check(oldPreferences.DensityMultiplier==1,"old settings default to x1");
 Check(JsonSerializer.Deserialize<Preferences>(JsonSerializer.Serialize(oldPreferences with{DensityMultiplier=5}))!.DensityMultiplier==5,"density preference persists");
 Check(DensityModifier.Normalize(0)==1&&DensityModifier.Normalize(6)==1,"invalid saved settings fall back to x1");
 var repo=Path.GetFullPath(Path.Combine(AppContext.BaseDirectory,"../../../../"));
 foreach(var rel in new[]{"data/global/excel/levels.txt","data/global/excel/base/levels.txt"}){
  var bytes=File.ReadAllBytes(Path.Combine(repo,rel));
  for(int multiplier=1;multiplier<=5;multiplier++)Check(DensityModifier.Transform(bytes,multiplier).Length>0,$"current {rel} supports x{multiplier}");
 }
 Console.WriteLine($"{passed} checks passed");
}finally{Directory.Delete(root,true);}
