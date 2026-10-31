// Applies the Predator.map symbols to the flat image and creates every known function.
// Usage (postScript): PcjSetup.java <symbols.txt>   lines: "<hex va> <f|d> <mangled name>"
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.regex.Pattern;

public class PcjSetup extends GhidraScript {
    // Non-static member functions: Q/U/I/A/M/E access codes followed by A/B (const) and E (thiscall).
    private static final Pattern THISCALL = Pattern.compile("@@[QIAUMESC][AB]E");

    @Override
    public void run() throws Exception {
        String path = getScriptArgs()[0];
        int funcs = 0, labels = 0, failed = 0;
        try (BufferedReader r = new BufferedReader(new FileReader(path))) {
            String line;
            while ((line = r.readLine()) != null) {
                String[] p = line.trim().split(" ", 3);
                if (p.length < 3) continue;
                Address a = toAddr(Long.parseLong(p[0], 16));
                try {
                    createLabel(a, p[2], true, SourceType.IMPORTED);
                    labels++;
                } catch (Exception e) {
                    // duplicate or invalid name: keep going
                }
                if (!p[1].equals("f")) continue;
                if (getFunctionAt(a) != null) continue;
                disassemble(a);
                Function f = createFunction(a, p[2]);
                if (f == null) {
                    failed++;
                    continue;
                }
                funcs++;
                if (THISCALL.matcher(p[2]).find()) {
                    try {
                        f.setCallingConvention("__thiscall");
                    } catch (Exception e) {
                    }
                }
                if ((funcs & 1023) == 0) println("functions " + funcs);
            }
        }
        println("labels " + labels + " functions " + funcs + " failed " + failed);
    }
}
