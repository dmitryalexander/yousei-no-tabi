// Windows companion UI. The pinned local worker performs transactional installation.
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.Collections.Generic;

public sealed class VisualSetup : Form {
    const string PythonUrl = "https://www.python.org/ftp/python/3.14.8/python-3.14.8-embed-amd64.zip";
    const string PythonHash = "a93abe456ab01bd96d7a085b3cdb6566b3063f4241360d114142fbdb07f0a310";
    readonly string root, source;
    readonly Label status = new Label();
    readonly Button install = new Button(), undo = new Button(), cancel = new Button(), launch = new Button();
    readonly ProgressBar progress = new ProgressBar();
    readonly TextBox history = new TextBox();
    CancellationTokenSource cancellation;
    Process worker;
    bool busy;
    static readonly UTF8Encoding Utf8 = new UTF8Encoding(false);

    [STAThread]
    public static int Main(string[] args) {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        string root = AppDomain.CurrentDomain.BaseDirectory;
        for (int i = 0; i < args.Length; i++) {
            if (args[i] == "--root" && i + 1 < args.Length) root = args[++i];
            else { MessageBox.Show("Unknown setup argument."); return 1; }
        }
        try { Application.Run(new VisualSetup(Path.GetFullPath(root))); return 0; }
        catch (Exception error) { MessageBox.Show(error.Message, "Visual setup"); return 1; }
    }

    public VisualSetup(string selectedRoot) {
        root = selectedRoot; source = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "visual-setup");
        Text = "Yousei no Tabi - Recommended visuals";
        using (var stream = typeof(VisualSetup).Assembly.GetManifestResourceStream("YouseiFairy")) {
            if (stream != null) using (var icon = new Icon(stream)) Icon = (Icon)icon.Clone();
        }
        ClientSize = new Size(720, 624); MinimumSize = new Size(730, 654);
        StartPosition = FormStartPosition.CenterScreen; Font = new Font("Segoe UI", 10);
        AutoScaleMode = AutoScaleMode.Dpi;
        var title = new Label { Text = "Set up recommended visuals", Font = new Font(Font.FontFamily, 19, FontStyle.Bold),
                                Location = new Point(24, 20), AutoSize = true };
        var description = new Label { Location = new Point(24, 70), Size = new Size(670, 120), Text =
            "HD textures, selected 3DS backgrounds and objects, repaired town scenery, and the lighting profile.\r\n\r\n" +
            "Downloads about 1.3 GB from the authors' official sources. Allow 12 GB of free space. " +
            "Your existing mods are moved to a backup so this profile has a consistent load order. Saves and controls are preserved. " +
            "Save and close the game before installing." };
        Controls.Add(title); Controls.Add(description);
        AddLink("OoT Reloaded / GhostlyDark", "https://evilgames.eu/texture-packs/oot-reloaded.htm", 24);
        AddLink("Djipi's 3DS Experience", "https://gamebanana.com/mods/477979", 290);
        var details = new Label { Location = new Point(24, 229), Size = new Size(670, 45), Text =
            "Keeps Link and the original music. Private character, equipment and NPC-overhaul mods are separate. " +
            "If a download changes, setup stops safely; download it on the author page and retry with a compatible release." };
        Controls.Add(details);
        install.Text = "Download and set up"; install.SetBounds(24, 289, 218, 38);
        undo.Text = "Undo last setup"; undo.SetBounds(254, 289, 180, 38);
        launch.Text = "Start game"; launch.SetBounds(446, 289, 130, 38);
        cancel.Text = "Cancel"; cancel.SetBounds(588, 289, 106, 38); cancel.Enabled = false;
        install.Click += async (s, e) => await Run(false);
        undo.Click += async (s, e) => await Run(true);
        cancel.Click += (s, e) => { cancellation.Cancel(); TryKill(); };
        launch.Click += (s, e) => {
            string executable = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "yousei-no-tabi.exe");
            if (!File.Exists(executable)) { MessageBox.Show("Game executable is missing beside this setup utility."); return; }
            var info = new ProcessStartInfo(executable) { WorkingDirectory = root, UseShellExecute = false };
            info.EnvironmentVariables["SHIP_HOME"] = root; Process.Start(info); Close();
        };
        Controls.AddRange(new Control[] { install, undo, cancel, launch });
        var import = new Button { Text = "Use downloaded packs...", Left = 24, Top = 339, Width = 218, Height = 32 };
        import.Click += async (s, e) => {
            if (busy) return;
            using (var picker = new OpenFileDialog { Title = "Select the original author downloads", Multiselect = true,
                Filter = "Original packs (*.7z;*.zip;*.o2r)|*.7z;*.zip;*.o2r" }) {
                if (picker.ShowDialog(this) != DialogResult.OK) return;
                busy = true; install.Enabled = undo.Enabled = launch.Enabled = false;
                FileStream importLock = null;
                try {
                    Directory.CreateDirectory(Path.Combine(root, ".visual-setup"));
                    importLock = new FileStream(Path.Combine(root, ".visual-setup", "wizard.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
                    var recipe = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(File.ReadAllText(Path.Combine(source, "manifest.json")));
                    var expected = ((System.Collections.IEnumerable)recipe["packs"]).Cast<Dictionary<string, object>>().ToList();
                    foreach (var pack in expected.ToArray())
                        if ((string)pack["format"] == "7z") foreach (Dictionary<string, object> output in (System.Collections.IEnumerable)pack["outputs"])
                            expected.Add(new Dictionary<string, object> { { "filename", output["name"] }, { "sha256", output["sha256"] } });
                    expected.Add(new Dictionary<string, object> { { "filename", "python-3.14.8-embed-amd64.zip" }, { "sha256", PythonHash } });
                    foreach (string file in picker.FileNames) {
                        var item = expected.SingleOrDefault(x => (string)x["filename"] == Path.GetFileName(file));
                        if (item == null || await Task.Run(() => Hash(file)) != (string)item["sha256"])
                            throw new IOException("Unrecognized version or checksum: " + Path.GetFileName(file));
                    }
                    foreach (string file in picker.FileNames) {
                        string folder = Path.Combine(root, ".visual-setup", Path.GetFileName(file).StartsWith("python-") ? "" : "downloads");
                        Directory.CreateDirectory(folder);
                        string target = Path.Combine(folder, Path.GetFileName(file));
                        if (!string.Equals(Path.GetFullPath(file), target, StringComparison.OrdinalIgnoreCase)) await Task.Run(() => File.Copy(file, target, true));
                    }
                    Log("Imported verified downloads. Choose Download and set up to install; cached files will be reused.");
                } catch (Exception error) { Log(error.Message); }
                finally {
                    if (importLock != null) importLock.Dispose(); busy = false;
                    install.Enabled = undo.Enabled = true;
                    launch.Enabled = !File.Exists(Path.Combine(root, ".visual-setup", "pending.json"));
                }
            }
        };
        Controls.Add(import);
        launch.Enabled = !File.Exists(Path.Combine(root, ".visual-setup", "pending.json"));
        progress.SetBounds(254, 348, 440, 12); progress.Style = ProgressBarStyle.Marquee; progress.Visible = false;
        status.SetBounds(24, 385, 670, 46); status.Text = "Ready. No downloads or settings changes until you choose setup.";
        history.SetBounds(24, 439, 670, 161); history.Multiline = true; history.ReadOnly = true;
        history.ScrollBars = ScrollBars.Vertical; history.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
        Controls.Add(progress); Controls.Add(status); Controls.Add(history);
        FormClosing += (s, e) => {
            if (busy) {
                e.Cancel = true;
                if (cancellation != null) { cancellation.Cancel(); TryKill(); Log("Cancelling. Wait for setup to stop before closing."); }
                else Log("Wait for download import to finish before closing.");
            }
        };
    }

    void AddLink(string title, string url, int x) {
        var link = new LinkLabel { Text = title, AutoSize = true, Location = new Point(x, 200) };
        link.LinkClicked += (s, e) => Process.Start(new ProcessStartInfo(url) { UseShellExecute = true }); Controls.Add(link);
    }

    void Log(string message) {
        if (InvokeRequired) { BeginInvoke(new Action<string>(Log), message); return; }
        status.Text = message; history.AppendText(message + Environment.NewLine);
    }

    static string Hash(string file) {
        using (var sha = SHA256.Create()) using (var stream = File.OpenRead(file))
            return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
    }

    // Quotes a single Windows command-line argument; never invokes a shell.
    public static string Quote(string value) {
        var b = new StringBuilder("\""); int slashes = 0;
        foreach (char c in value) {
            if (c == '\\') slashes++;
            else if (c == '"') { b.Append('\\', slashes * 2 + 1); b.Append(c); slashes = 0; }
            else { b.Append('\\', slashes); slashes = 0; b.Append(c); }
        }
        b.Append('\\', slashes * 2); b.Append('"'); return b.ToString();
    }

    void TryKill() {
        try {
            if (worker != null && !worker.HasExited) {
                using (var kill = Process.Start(new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "taskkill.exe"),
                    "/PID " + worker.Id + " /T /F") { UseShellExecute = false, CreateNoWindow = true })) kill.WaitForExit();
            }
        } catch (InvalidOperationException) {}
    }

    async Task<string> PreparePython(CancellationToken token) {
        string work = Path.Combine(root, ".visual-setup"); Directory.CreateDirectory(work);
        string archive = Path.Combine(work, "python-3.14.8-embed-amd64.zip");
        if (!File.Exists(archive) || await Task.Run(() => Hash(archive)) != PythonHash) {
            Log("Downloading the isolated setup runtime (Python Software Foundation)");
            string part = archive + ".partial";
            try {
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                using (var handler = new HttpClientHandler { AllowAutoRedirect = false })
                using (var client = new HttpClient(handler) { Timeout = TimeSpan.FromMinutes(5) }) {
                    client.DefaultRequestHeaders.UserAgent.ParseAdd("Yousei-no-Tabi-Visual-Setup/1.0");
                    using (var response = await client.GetAsync(PythonUrl, HttpCompletionOption.ResponseHeadersRead, token)) {
                        response.EnsureSuccessStatusCode();
                        using (var input = await response.Content.ReadAsStreamAsync())
                        using (var output = File.Create(part)) {
                            byte[] buffer = new byte[131072]; long total = 0; int n;
                            while ((n = await input.ReadAsync(buffer, 0, buffer.Length, token)) != 0) {
                                total += n; if (total > 25 * 1024 * 1024) throw new IOException("Runtime download size changed.");
                                await output.WriteAsync(buffer, 0, n, token);
                            }
                        }
                    }
                }
                if (await Task.Run(() => Hash(part)) != PythonHash) throw new IOException("Runtime checksum changed; no changes installed.");
                if (File.Exists(archive)) File.Delete(archive); File.Move(part, archive);
            } finally { if (File.Exists(part)) File.Delete(part); }
        }
        token.ThrowIfCancellationRequested();
        // Fresh extraction avoids executing a stale/modified cache runtime.
        string runtime = Path.Combine(work, "runtime-" + Guid.NewGuid().ToString("N"));
        await Task.Run(() => ZipFile.ExtractToDirectory(archive, runtime));
        return runtime;
    }

    async Task Run(bool restore) {
        if (Process.GetProcessesByName("yousei-no-tabi").Length != 0) {
            MessageBox.Show("Save and close Yousei no Tabi, then click this button again. The setup window can stay open.", "Close game first"); return;
        }
        if (!Directory.Exists(source)) { MessageBox.Show("Keep the visual-setup folder beside this utility."); return; }
        // Refuse redirected paths before the companion creates caches or executes anything there.
        for (var dir = new DirectoryInfo(root); dir != null; dir = dir.Parent)
            if ((dir.Attributes & FileAttributes.ReparsePoint) != 0) { MessageBox.Show("Use a game folder without junctions or symbolic links."); return; }
        string work = Path.Combine(root, ".visual-setup");
        if (Directory.Exists(work) && (File.GetAttributes(work) & FileAttributes.ReparsePoint) != 0) {
            MessageBox.Show("The setup cache must be a normal folder."); return;
        }
        busy = true; install.Enabled = undo.Enabled = launch.Enabled = false; cancel.Enabled = true; progress.Visible = true;
        cancellation = new CancellationTokenSource(); string runtime = null; FileStream wizardLock = null;
        try {
            Directory.CreateDirectory(work);
            wizardLock = new FileStream(Path.Combine(work, "wizard.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
            runtime = await PreparePython(cancellation.Token); cancellation.Token.ThrowIfCancellationRequested();
            var info = new ProcessStartInfo(Path.Combine(runtime, "python.exe"),
                Quote(Path.Combine(source, "setup.py")) + " --root " + Quote(root) + (restore ? " --undo" : "")) {
                UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true,
                RedirectStandardError = true, StandardOutputEncoding = Utf8, StandardErrorEncoding = Utf8, WorkingDirectory = root };
            worker = new Process { StartInfo = info };
            worker.OutputDataReceived += (s, e) => {
                if (e.Data == null) return;
                try { var row = new JavaScriptSerializer().Deserialize<Dictionary<string, string>>(e.Data); Log(row["message"]); }
                catch { Log(e.Data); }
            };
            worker.ErrorDataReceived += (s, e) => { if (e.Data != null) Log(e.Data); };
            worker.Start(); worker.BeginOutputReadLine(); worker.BeginErrorReadLine();
            await Task.Run(() => worker.WaitForExit()); cancellation.Token.ThrowIfCancellationRequested();
            if (worker.ExitCode != 0) throw new IOException("Setup stopped. See the message above; your prior files are preserved. Retry to recover an interrupted install.");
        } catch (OperationCanceledException) { Log("Cancelled. Retry or undo before starting the game if installation was interrupted."); }
          catch (Exception error) { Log(error.Message + " Author-page links above provide a manual download fallback."); }
        finally {
            if (worker != null) { worker.Dispose(); worker = null; }
            // This is a newly generated runtime directory, always below .visual-setup.
            try { if (runtime != null && Directory.Exists(runtime)) Directory.Delete(runtime, true); }
            catch (IOException) { Log("The temporary setup runtime is still in use; it can be removed after closing setup."); }
            if (wizardLock != null) wizardLock.Dispose();
            busy = false; install.Enabled = undo.Enabled = true;
            launch.Enabled = !File.Exists(Path.Combine(root, ".visual-setup", "pending.json"));
            cancel.Enabled = false; progress.Visible = false;
            cancellation.Dispose(); cancellation = null;
        }
    }
}
