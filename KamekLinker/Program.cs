using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Kamek
{
    class Program
    {
        struct CombinedHeader
        {
            public byte[] ToBytes()
            {
                byte[] ret = new byte[4 * 4];
                Buffer.BlockCopy(sizes, 0, ret, 0, 4 * 4);
                return ret;
            }
            public CombinedHeader() { }
            public int[] sizes = new int[4];
        }
        public static string _curVer;
        static async Task Main(string[] args)
        {
            Console.WriteLine("Kamek 2.0 by Ninji/Ash Wolf - https://github.com/Treeki/Kamek");
            Console.WriteLine();

            // Parse the command line arguments and do cool things!
            var modules = new List<Elf>();
            uint? baseAddress = null;
            string outputKamekPath = null, outputRiivPath = null, outputDolphinPath = null, outputGeckoPath = null, outputARPath = null, outputCodePath = null, outputCombinedPath = null;
            string inputDolPath = null, outputDolPath = null;
            string outputMapPath = null;
            List<byte> combined = new List<byte>();
            CombinedHeader header = new CombinedHeader();
            var externals = new Dictionary<string, uint>();
            VersionInfo versions = null;
            Debug debug = null;
            var selectedVersions = new List<String>();

            uint debugStartAddr = 0;
            string debugMapPath = "";
            string debugReadElfPath = "";
            foreach (string arg in args)
            {
                if (arg.StartsWith("-map=")) debugMapPath = arg.Substring(5);
                if (arg.StartsWith("-readelf=")) debugReadElfPath = arg.Substring(9);
                if (arg.StartsWith("-debug=")) debugStartAddr = Convert.ToUInt32(arg.Substring(7), 16);
            }
            if (debugStartAddr != 0 && debugMapPath != null && debugReadElfPath != null)
            {
                debug = new Debug(debugMapPath, debugReadElfPath, debugStartAddr);
            }

            foreach (string arg in args)
            {

                if (arg.StartsWith("-"))
                {
                    if (arg == "-h" || arg == "-help" || arg == "--help")
                    {
                        ShowHelp();
                        return;
                    }

                    if (arg == "-dynamic")
                        baseAddress = null;
                    else if (arg.StartsWith("-debug") || arg.StartsWith("-map=") || arg.StartsWith("-readelf=")) { }
                    else if (arg.StartsWith("-static=0x"))
                        baseAddress = uint.Parse(arg.Substring(10), System.Globalization.NumberStyles.HexNumber);
                    else if (arg.StartsWith("-output-kamek="))
                        outputKamekPath = arg.Substring(14);
                    else if (arg.StartsWith("-output-riiv="))
                        outputRiivPath = arg.Substring(13);
                    else if (arg.StartsWith("-output-dolphin="))
                        outputDolphinPath = arg.Substring(16);
                    else if (arg.StartsWith("-output-gecko="))
                        outputGeckoPath = arg.Substring(14);
                    else if (arg.StartsWith("-output-ar="))
                        outputARPath = arg.Substring(11);
                    else if (arg.StartsWith("-output-code="))
                        outputCodePath = arg.Substring(13);
                    else if (arg.StartsWith("-output-combined="))
                    {
                        outputCombinedPath = arg.Substring(17);
                    }
                    else if (arg.StartsWith("-input-dol="))
                        inputDolPath = arg.Substring(11);
                    else if (arg.StartsWith("-output-dol="))
                        outputDolPath = arg.Substring(12);
                    else if (arg.StartsWith("-output-map="))
                        outputMapPath = arg.Substring(12);
                    else if (arg.StartsWith("-externals="))
                        ReadExternals(externals, arg.Substring(11));
                    else if (arg.StartsWith("-versions="))
                        versions = new VersionInfo(arg.Substring(10));
                    else if (arg.StartsWith("-select-version="))
                        selectedVersions.Add(arg.Substring(16));
                    else
                        Console.WriteLine("warning: unrecognised argument: {0}", arg);
                }
                else
                {
                    Console.WriteLine("adding {0} as object..", arg);

                    if (debug != null)
                    {
                        debug.AnalyzeFile(arg, modules.Count);
                    }
                    using (var stream = new FileStream(arg, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
                    {
                        modules.Add(new Elf(stream));
                    }

                }
            }


            // We need a default VersionList for the loop later
            if (versions == null)
                versions = new VersionInfo();


            // Can we build a thing?
            if (modules.Count == 0)
            {
                Console.WriteLine("no input files specified");
                return;
            }
            if (outputKamekPath == null && outputRiivPath == null && outputDolphinPath == null && outputGeckoPath == null && outputARPath == null
                && outputCodePath == null && outputDolPath == null && outputCombinedPath == null)
            {
                Console.WriteLine("no output path(s) specified");
                return;
            }
            if (outputDolPath != null && inputDolPath == null)
            {
                Console.WriteLine("input dol path not specified");
                return;
            }


            // Do safety checks
            if (versions.Mappers.Count > 1 && selectedVersions.Count != 1)
            {
                bool ambiguousOutputPath = false;
                ambiguousOutputPath |= (outputKamekPath != null && !outputKamekPath.Contains("$KV$"));
                ambiguousOutputPath |= (outputRiivPath != null && !outputRiivPath.Contains("$KV$"));
                ambiguousOutputPath |= (outputDolphinPath != null && !outputDolphinPath.Contains("$KV$"));
                ambiguousOutputPath |= (outputGeckoPath != null && !outputGeckoPath.Contains("$KV$"));
                ambiguousOutputPath |= (outputARPath != null && !outputARPath.Contains("$KV$"));
                ambiguousOutputPath |= (outputCodePath != null && !outputCodePath.Contains("$KV$"));
                ambiguousOutputPath |= (outputDolPath != null && !outputDolPath.Contains("$KV$"));
                if (ambiguousOutputPath)
                {
                    Console.WriteLine("ERROR: this configuration builds for multiple game versions, and some of the outputs will be overwritten");
                    Console.WriteLine("add the $KV$ placeholder to your output paths, or use -select-version=.. to only build one version");
                    return;
                }

            }



            foreach (var version in versions.Mappers)
            {
                if (selectedVersions.Count > 0 && !selectedVersions.Contains(version.Key))
                {
                    Console.WriteLine("(skipping version {0} as it's not selected)", version.Key);
                    continue;
                }
                Console.WriteLine("linking version {0}...", version.Key);
                _curVer = version.Key;
                var linker = new Linker(version.Value);
                foreach (var module in modules)
                    linker.AddModule(module);

                if (baseAddress.HasValue)
                    linker.LinkStatic(baseAddress.Value, externals);
                else
                    linker.LinkDynamic(externals);

                var kf = new KamekFile();
                kf.LoadFromLinker(linker);
                if (outputKamekPath != null)
                    File.WriteAllBytes(outputKamekPath.Replace("$KV$", version.Key), kf.Pack());
                if (outputRiivPath != null)
                    File.WriteAllText(outputRiivPath.Replace("$KV$", version.Key), kf.PackRiivolution());
                if (outputDolphinPath != null)
                    File.WriteAllText(outputDolphinPath.Replace("$KV$", version.Key), kf.PackDolphin());
                if (outputGeckoPath != null)
                    File.WriteAllText(outputGeckoPath.Replace("$KV$", version.Key), kf.PackGeckoCodes());
                if (outputARPath != null)
                    File.WriteAllText(outputARPath.Replace("$KV$", version.Key), kf.PackActionReplayCodes());
                if (outputCodePath != null)
                    File.WriteAllBytes(outputCodePath.Replace("$KV$", version.Key), kf.CodeBlob);
                if (outputCombinedPath != null)
                {
                    uint index = 0;
                    switch (version.Key)
                    {
                        case "P": index = 0; break;
                        case "E": index = 1; break;
                        case "J": index = 2; break;
                        case "K": index = 3; break;
                    }
                    byte[] packed = kf.Pack();
                    header.sizes[index] = BinaryPrimitives.ReverseEndianness(packed.Length);
                    combined.AddRange(kf.Pack());
                }



                if (outputDolPath != null)
                {
                    var dol = new Dol(new FileStream(inputDolPath.Replace("$KV$", version.Key), FileMode.Open, FileAccess.ReadWrite, FileShare.None));
                    kf.InjectIntoDol(dol);

                    var outpath = outputDolPath.Replace("$KV$", version.Key);
                    using (var outStream = new FileStream(outpath, FileMode.Create))
                    {
                        dol.Write(outStream);
                    }
                }

                if (outputMapPath != null)
                {
                    linker.WriteSymbolMap(outputMapPath.Replace("$KV$", version.Key));
                }
            }

            if (outputCombinedPath != null)
            {
                combined.InsertRange(0, header.ToBytes());
                File.WriteAllBytes(outputCombinedPath, combined.ToArray());
            }
            if (debug != null)
            {
                debug.Save();
            }

        }

        private static void ReadExternals(Dictionary<string, uint> dict, string path)
        {
            var commentRegex = new Regex(@"^\s*#");
            var emptyLineRegex = new Regex(@"^\s*$");
            var assignmentRegex = new Regex(@"^\s*([a-zA-Z0-9_<>@,-\\$]+)\s*=\s*0x([a-fA-F0-9]+)\s*(#.*)?$");

            foreach (var line in File.ReadAllLines(path))
            {
                if (emptyLineRegex.IsMatch(line))
                    continue;
                if (commentRegex.IsMatch(line))
                    continue;

                var match = assignmentRegex.Match(line);
                if (match.Success)
                {
                    dict[match.Groups[1].Value] = uint.Parse(match.Groups[2].Value, System.Globalization.NumberStyles.HexNumber);
                }
                else
                {
                    Console.WriteLine("unrecognised line in externals file: {0}", line);
                }
            }
        }

        private static void ShowHelp()
        {
            Console.WriteLine("Syntax:");
            Console.WriteLine("  Kamek file1.o [file2.o...] [options]");
            Console.WriteLine();
            Console.WriteLine("Options:");
            Console.WriteLine("  Build Mode (select one; defaults to -dynamic):");
            Console.WriteLine("    -dynamic");
            Console.WriteLine("      generate a dynamically linked Kamek binary for use with the loader");
            Console.WriteLine("    -static=0x80001900");
            Console.WriteLine("      generate a blob of code which must be loaded at the specified Wii RAM address");
            Console.WriteLine();
            Console.WriteLine("  Game Configuration:");
            Console.WriteLine("    -externals=file.txt");
            Console.WriteLine("      specify the addresses of external symbols that exist in the target game");
            Console.WriteLine("    -versions=file.txt");
            Console.WriteLine("      specify the different executable versions that Kamek can link binaries for");
            Console.WriteLine("    -select-version=key");
            Console.WriteLine("      build only one version from the versions file, and ignore the rest");
            Console.WriteLine("      (can be specified multiple times)");
            Console.WriteLine();
            Console.WriteLine("  Outputs (at least one is required; $KV$ will be replaced with the version name):");
            Console.WriteLine("    -output-kamek=file.$KV$.bin");
            Console.WriteLine("      write a Kamek binary to for use with the loader (-dynamic only)");
            Console.WriteLine("    -output-riiv=file.$KV$.xml");
            Console.WriteLine("      write a Riivolution XML fragment (-static only)");
            Console.WriteLine("    -output-dolphin=file.$KV$.ini");
            Console.WriteLine("      write a Dolphin INI fragment (-static only)");
            Console.WriteLine("    -output-gecko=file.$KV$.xml");
            Console.WriteLine("      write a list of Gecko codes (-static only)");
            Console.WriteLine("    -output-ar=file.$KV$.xml");
            Console.WriteLine("      write a list of Action Replay codes (-static only)");
            Console.WriteLine("    -input-dol=file.$KV$.dol -output-dol=file2.$KV$.dol");
            Console.WriteLine("      apply these patches and generate a modified DOL (-static only)");
            Console.WriteLine("    -output-code=file.$KV$.bin");
            Console.WriteLine("      write the combined code+data segment to file.bin (for manual injection or debugging)");
            Console.WriteLine("    -output-map=file.$KV$.map");
            Console.WriteLine("      write a list of symbols and their relative offsets (for debugging)");
        }
    }
}
