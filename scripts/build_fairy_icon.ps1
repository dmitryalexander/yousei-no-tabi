# Same drawing palette, 256-unit grid and 16-unit stroke as the user's app family.
param([string]$Output = (Join-Path $PSScriptRoot '..\soh\yousei-lighting.ico'), [string]$Preview)
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -eq 'Core') {
    $arguments = @('-NoProfile','-ExecutionPolicy','Bypass','-File',$PSCommandPath,'-Output',$Output)
    if ($Preview) { $arguments += @('-Preview',$Preview) }
    & "$env:WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe" @arguments
    exit $LASTEXITCODE
}
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
public static class YouseiFairyIcon {
    static byte[] Render(int size) {
        using (var bitmap = new Bitmap(size, size)) using (var g = Graphics.FromImage(bitmap)) {
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.ScaleTransform(size / 256f, size / 256f);
            g.Clear(Color.Transparent);
            using (var fill = new SolidBrush(Color.FromArgb(28,29,29)))
            using (var amber = new SolidBrush(Color.FromArgb(233,179,68)))
            using (var pen = new Pen(Color.FromArgb(233,179,68),16)) {
                pen.LineJoin = LineJoin.Round; pen.StartCap = pen.EndCap = LineCap.Round;
                PointF[][] wings = {
                    new PointF[] {new PointF(108,119),new PointF(48,16),new PointF(16,112),new PointF(100,141)},
                    new PointF[] {new PointF(148,119),new PointF(208,16),new PointF(240,112),new PointF(156,141)},
                    new PointF[] {new PointF(101,173),new PointF(24,191),new PointF(62,240),new PointF(106,187)},
                    new PointF[] {new PointF(155,173),new PointF(232,191),new PointF(194,240),new PointF(150,187)}
                };
                foreach (var points in wings) using (var path = new GraphicsPath()) {
                    path.AddLines(points); path.CloseFigure(); g.FillPath(fill,path); g.DrawPath(pen,path);
                }
                g.FillEllipse(amber,117,144,22,22);
            }
            using (var stream = new MemoryStream()) { bitmap.Save(stream,ImageFormat.Png); return stream.ToArray(); }
        }
    }
    public static void Write(string path, string preview) {
        int[] sizes = {16,20,24,32,40,48,64,128,256};
        byte[][] images = new byte[sizes.Length][];
        for (int i=0;i<sizes.Length;i++) images[i]=Render(sizes[i]);
        using (var writer = new BinaryWriter(File.Create(path))) {
            writer.Write((ushort)0);writer.Write((ushort)1);writer.Write((ushort)sizes.Length);
            int offset=6+16*sizes.Length;
            for(int i=0;i<sizes.Length;i++) {
                writer.Write((byte)(sizes[i]==256?0:sizes[i]));writer.Write((byte)(sizes[i]==256?0:sizes[i]));
                writer.Write((byte)0);writer.Write((byte)0);writer.Write((ushort)1);writer.Write((ushort)32);
                writer.Write(images[i].Length);writer.Write(offset);offset+=images[i].Length;
            }
            foreach(var image in images) writer.Write(image);
        }
        if (!String.IsNullOrEmpty(preview)) File.WriteAllBytes(preview,images[images.Length-1]);
    }
}
'@
[YouseiFairyIcon]::Write([IO.Path]::GetFullPath($Output), $(if($Preview){[IO.Path]::GetFullPath($Preview)}else{''}))
