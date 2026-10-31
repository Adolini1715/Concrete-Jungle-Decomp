// Writes Ghidra pseudocode for each function in a todo list, one file per function, so a run
// can stop and resume at any point.
// Usage (postScript): PcjDecomp.java <todo.txt> <outdir>   lines: "<hex va> <obj> <mangled name>"
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;

public class PcjDecomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File out = new File(args[1]);
        out.mkdirs();
        DecompInterface ifc = new DecompInterface();
        ifc.setOptions(new DecompileOptions());
        ifc.openProgram(currentProgram);
        int done = 0, skipped = 0, failed = 0;
        try (BufferedReader r = new BufferedReader(new FileReader(args[0]))) {
            String line;
            while ((line = r.readLine()) != null) {
                if (monitor.isCancelled()) break;
                String[] p = line.trim().split(" ", 3);
                if (p.length < 3) continue;
                File dir = new File(out, p[1].replace(".obj", ""));
                File f = new File(dir, p[0] + ".c");
                if (f.exists()) {
                    skipped++;
                    continue;
                }
                dir.mkdirs();
                Function fn = getFunctionAt(toAddr(Long.parseLong(p[0], 16)));
                String text;
                if (fn == null) {
                    text = "// no function at " + p[0] + "\n";
                    failed++;
                } else {
                    DecompileResults res = ifc.decompileFunction(fn, 60, monitor);
                    if (res != null && res.decompileCompleted() && res.getDecompiledFunction() != null) {
                        text = res.getDecompiledFunction().getC();
                    } else {
                        text = "// decompile failed: " + (res == null ? "null" : res.getErrorMessage()) + "\n";
                        failed++;
                    }
                }
                try (FileWriter w = new FileWriter(f)) {
                    w.write("// " + p[0] + " " + p[2] + "\n");
                    w.write(text);
                }
                done++;
                if (done % 100 == 0) println("decompiled " + done);
            }
        }
        ifc.dispose();
        println("done " + done + " skipped " + skipped + " failed " + failed);
    }
}
