using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using System.Text;
namespace AstralWorkshop;
public static class DensityModifier {
 internal static readonly string[] Paths=["ReimaginedWorkshop.mpq/data/global/excel/levels.txt","ReimaginedWorkshop.mpq/data/global/excel/base/levels.txt"];
 static readonly UTF8Encoding Utf8=new(false,true);
 internal static int Normalize(int value)=>value is >=1 and <=5?value:1;
 static string Hash(byte[] data)=>Convert.ToHexString(SHA256.HashData(data));
 static string Cache(string root,int index)=>Path.Combine(root,".astral-density",$"levels-{index}.txt");
 static string Selection(string root)=>Path.Combine(root,".astral-density","multiplier.txt");
 internal static byte[] Transform(byte[] baseline,int multiplier){
  if(multiplier<1||multiplier>5)throw new InvalidDataException("Monster density must be between x1 and x5.");
  if(multiplier==1)return baseline.ToArray();
  bool bom=baseline.AsSpan().StartsWith(new byte[]{239,187,191});
  string text=Utf8.GetString(baseline,bom?3:0,baseline.Length-(bom?3:0));
  string newline=text.Contains("\r\n")?"\r\n":"\n";
  var lines=text.Split(newline);var header=lines[0].Split('\t');
  var columns=new[]{"MonDen","MonDen(N)","MonDen(H)"}.Select(name=>Array.IndexOf(header,name)).ToArray();
  if(columns.Any(i=>i<0))throw new InvalidDataException("Monster density columns are missing from levels.txt.");
  for(int row=1;row<lines.Length;row++){
   if(string.IsNullOrWhiteSpace(lines[row]))continue;
   var cells=lines[row].Split('\t');
   foreach(int column in columns){
    if(column>=cells.Length)throw new InvalidDataException("Incomplete monster density row.");
    if(cells[column].Length==0)continue;
    if(!int.TryParse(cells[column],NumberStyles.None,CultureInfo.InvariantCulture,out int value)||value>10000/multiplier)throw new InvalidDataException("Monster density exceeds the supported 10000 limit.");
    cells[column]=(value*multiplier).ToString(CultureInfo.InvariantCulture);
   }
   lines[row]=string.Join('\t',cells);
  }
  byte[] data=Utf8.GetBytes(string.Join(newline,lines));return bom?[239,187,191,..data]:data;
 }
 // Recognize only an exact derivation of the release checksum, not arbitrary local edits.
 internal static bool Matches(string root,ReleaseFile file){
  int index=Array.IndexOf(Paths,file.Path);if(index<0)return false;
  try{
   if(!int.TryParse(File.ReadAllText(Selection(root)),out int multiplier)||multiplier<1||multiplier>5)return false;
   var baseline=File.ReadAllBytes(Cache(root,index));
   return Hash(baseline).Equals(file.Sha256,StringComparison.OrdinalIgnoreCase)&&File.ReadAllBytes(Updater.SafePath(root,file.Path)).AsSpan().SequenceEqual(Transform(baseline,multiplier));
  }catch(Exception e) when(e is IOException or UnauthorizedAccessException or DecoderFallbackException){return false;}
 }
 static void Write(string path,byte[] bytes){
  Directory.CreateDirectory(Path.GetDirectoryName(path)!);var temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
  try{File.WriteAllBytes(temp,bytes);File.Move(temp,path,true);}finally{if(File.Exists(temp))File.Delete(temp);}
 }
 internal static void Apply(string root,ModRelease release,int multiplier){
  if(multiplier<1||multiplier>5)throw new InvalidDataException("Monster density must be between x1 and x5.");
  var plan=new List<(string Path,string Cache,byte[] Baseline,byte[] Result)>();
  for(int i=0;i<Paths.Length;i++){
   var file=release.Files.SingleOrDefault(f=>f.Path==Paths[i])??throw new InvalidDataException("This mod release does not include the monster density tables.");
   var path=Updater.SafePath(root,file.Path);var baseline=File.ReadAllBytes(path);
   if(!Hash(baseline).Equals(file.Sha256,StringComparison.OrdinalIgnoreCase)){
    if(!Matches(root,file))throw new InvalidDataException("Monster density table needs repair before starting.");
    baseline=File.ReadAllBytes(Cache(root,i));
   }
   plan.Add((path,Cache(root,i),baseline,Transform(baseline,multiplier)));
  }
  foreach(var item in plan){Write(item.Cache,item.Baseline);Write(item.Path,item.Result);}
  Write(Selection(root),Encoding.ASCII.GetBytes(multiplier.ToString(CultureInfo.InvariantCulture)));
 }
}
