// Applies the decomp's class layouts to the Ghidra program so the decompiler prints this->m_Field instead
// of *(int *)(this + 0x2c). Input: build/ghidra_types.txt from tools/ghidra_types.py.
// Usage (postScript): PcjTypes.java <ghidra_types.txt>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.GhidraClass;
import ghidra.program.model.listing.VariableUtilities;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.SymbolTable;
import ghidra.app.util.NamespaceUtils;
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.*;

public class PcjTypes extends GhidraScript {
    private final Map<String, Structure> structs = new HashMap<>();

    private GhidraClass classFor(String name) throws Exception {
        SymbolTable st = currentProgram.getSymbolTable();
        Namespace g = currentProgram.getGlobalNamespace();
        Namespace ns = st.getNamespace(name, g);
        if (ns == null) return st.createClass(g, name, SourceType.IMPORTED);
        if (ns instanceof GhidraClass) return (GhidraClass) ns;
        return NamespaceUtils.convertNamespaceToClass(ns);
    }

    private DataType builtin(String t) {
        switch (t) {
            case "char": return CharDataType.dataType;
            case "uchar": return UnsignedCharDataType.dataType;
            case "short": return ShortDataType.dataType;
            case "ushort": return UnsignedShortDataType.dataType;
            case "int": return IntegerDataType.dataType;
            case "uint": return UnsignedIntegerDataType.dataType;
            case "float": return FloatDataType.dataType;
            case "double": return DoubleDataType.dataType;
        }
        return UnsignedCharDataType.dataType;
    }

    private DataType type(String kind, String t) {
        if (kind.equals("b")) return builtin(t);
        if (kind.equals("pp")) {
            Structure s = structs.get(t);
            return new PointerDataType(new PointerDataType(s != null ? s : VoidDataType.dataType, 4), 4);
        }
        if (kind.equals("p")) {
            Structure s = structs.get(t);
            return new PointerDataType(s != null ? s : VoidDataType.dataType, 4);
        }
        Structure s = structs.get(t);
        return s != null ? s : null;
    }

    @Override
    public void run() throws Exception {
        List<String[]> lines = new ArrayList<>();
        try (BufferedReader r = new BufferedReader(new FileReader(getScriptArgs()[0]))) {
            String l;
            while ((l = r.readLine()) != null) lines.add(l.trim().split(" "));
        }
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        int nc = 0, nf = 0, bad = 0, nm = 0, ng = 0;
        // 1. structures with their sizes
        for (String[] p : lines) {
            if (!p[0].equals("C")) continue;
            try {
                GhidraClass c = classFor(p[1]);
                Structure s = (Structure) dtm.resolve(VariableUtilities.findOrCreateClassStruct(c, dtm), DataTypeConflictHandler.KEEP_HANDLER);
                int size = Integer.parseInt(p[2]);
                if (s.getLength() < size) s.growStructure(size - (s.isZeroLength() ? 0 : s.getLength()));
                structs.put(p[1], s);
                nc++;
            } catch (Exception e) {
                bad++;
            }
        }
        // 2. fields
        for (String[] p : lines) {
            if (!p[0].equals("F")) continue;
            Structure s = structs.get(p[1]);
            if (s == null) continue;
            int off = Integer.parseInt(p[2]), count = Integer.parseInt(p[3]);
            DataType dt = type(p[4], p[5]);
            if (dt == null || dt.getLength() <= 0) { bad++; continue; }
            if (count > 1) dt = new ArrayDataType(dt, count, dt.getLength());
            try {
                if (off + dt.getLength() > s.getLength()) { bad++; continue; }
                s.replaceAtOffset(off, dt, dt.getLength(), p[6], null);
                nf++;
            } catch (Exception e) {
                bad++;
            }
        }
        // 3. methods: thiscall in their class namespace, so the auto `this` is Class *
        for (String[] p : lines) {
            if (!p[0].equals("M")) continue;
            Function f = getFunctionAt(toAddr(Long.parseLong(p[1], 16)));
            if (f == null) continue;
            try {
                GhidraClass c = classFor(p[2]);
                if (!f.getParentNamespace().equals(c)) f.setParentNamespace(c);
                f.setCallingConvention("__thiscall");
                nm++;
            } catch (Exception e) {
                bad++;
            }
        }
        // 4. globals
        for (String[] p : lines) {
            if (!p[0].equals("G")) continue;
            DataType dt = type(p[2], p[3]);
            if (dt == null) continue;
            if (p[2].equals("pp")) {   // T *g[N]: an array of pointers
                int n = p.length > 5 ? Integer.parseInt(p[5]) : 1;
                DataType el = ((Pointer) dt).getDataType();
                dt = n > 1 ? new ArrayDataType(el, n, 4) : el;
            }
            Address a = toAddr(Long.parseLong(p[1], 16));
            try {
                clearListing(a, a.add(dt.getLength() - 1));
                createData(a, dt);
                ng++;
            } catch (Exception e) {
                bad++;
            }
        }
        println("structs " + nc + " fields " + nf + " methods " + nm + " globals " + ng + " failed " + bad);
    }
}
