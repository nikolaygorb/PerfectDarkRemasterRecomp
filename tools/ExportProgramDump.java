// Ghidra script: full-program dump exporter.
//
// Exports everything useful that Ghidra's analysis extracted from the
// currently loaded program (default.xex) to a set of files saved in
// "xex_dump/" next to wherever THIS script file itself lives (so if you
// copy this into C:\Users\<you>\ghidra_scripts, the dump lands there too -
// no repo-relative paths this time, per request).
//
// Output files (all in <script_dir>/xex_dump/):
//   manifest.json     - program name/language/image base/counts/timestamp
//   memory_map.json   - every memory block: name, start/end, size, perms
//   symbols.jsonl      - every symbol in the symbol table (name, address, type)
//   functions.jsonl    - one pretty-printed JSON object PER FUNCTION, back to
//                        back (NOT a single JSON array - see note below),
//                        each with: address, name, size, full disassembly
//                        (address/bytes/mnemonic/operands per instruction),
//                        and (if ENABLE_DECOMPILE) the decompiled C.
//
// IMPORTANT about functions.jsonl format: this is a stream of standalone
// JSON objects separated by blank lines, NOT valid as a single JSON
// document and NOT strict one-object-per-line JSONL either (functions with
// many instructions span many lines, pretty-printed, so they stay readable
// and diffable/greppable). Parse it by reading one `{ ... }` object at a
// time (e.g. Python's json.JSONDecoder().raw_decode in a loop), not
// json.load() on the whole file.
//
// Config (edit below before running):
//   ENABLE_DECOMPILE - also run the decompiler per function (MUCH slower -
//                       expect this to take a long time across the whole
//                       program; leave off for a fast disassembly-only pass
//                       first). Default: false.
//   MIN_ADDR/MAX_ADDR - restrict the dump to this address range (hex
//                       strings, no "0x"), leave both "" to dump the whole
//                       program. Example: dump only the game's own code
//                       range by setting MIN_ADDR="82170000",
//                       MAX_ADDR="832C2A3C".
//   FUNCTION_LIMIT    - stop after this many functions (-1 = no limit).
//                       Useful for a quick test run before committing to a
//                       full multi-hour dump.
//   TARGET_ADDRESSES  - addresses (hex, no "0x") of functions to decompile
//                       even when ENABLE_DECOMPILE is false. Useful for
//                       debugging a specific function without paying the
//                       cost of decompiling the whole program.
//
// Usage: Window -> Script Manager -> run this script while default.xex is
// open and analyzed. Progress is printed to the Console every 500
// functions. You can Cancel from the task dialog at any time - files
// written so far remain valid/readable.

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSetView;
import ghidra.program.model.lang.Language;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.util.exception.CancelledException;

import java.io.File;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ExportProgramDump extends GhidraScript {

    // ---- Config ---------------------------------------------------------
    static final boolean ENABLE_DECOMPILE = false;
    static final String MIN_ADDR = ""; // e.g. "82170000", or "" for whole program
    static final String MAX_ADDR = ""; // e.g. "832C2A3C", or "" for whole program
    static final int FUNCTION_LIMIT = -1; // -1 = no limit
    static final int PROGRESS_EVERY = 500;
    // Addresses (hex, no "0x") of functions to decompile even when ENABLE_DECOMPILE is false.
    // Useful for debugging: e.g. decompile only the functions you're investigating.
    static final String[] TARGET_ADDRESSES = {
        // "82C77E00",  // sub_82C77E00
        // "82C8D7D8",  // sub_82C8D7D8
        // "8310C340",  // sub_8310C340
        // "8310C5A0",  // sub_8310C5A0
    };
    
    // Auto-detect suspicious functions (thunks, register saves) and export context.
    // This makes the script reusable across different .xex files.
    static final boolean ENABLE_AUTO_DETECT_SUSPICIOUS = true;
    static final int SUSPICIOUS_MAX_SIZE = 32; // Functions smaller than this are suspicious
    static final int CONTEXT_BYTES_BEFORE = 256; // Bytes of context before function
    static final int CONTEXT_BYTES_AFTER = 256; // Bytes of context after function
    // ----------------------------------------------------------------------

    // PowerPC general-purpose register names (r0-r31)
    static final String[] PPC_REGS = {
        "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10",
        "r11","r12","r13","r14","r15","r16","r17","r18","r19","r20",
        "r21","r22","r23","r24","r25","r26","r27","r28","r29","r30","r31"
    };

    @Override
    public void run() throws Exception {
        File outDir = new File(getSourceFile().getFile(false).getParentFile(), "xex_dump");
        outDir.mkdirs();

        writeManifest(outDir);
        writeMemoryMap(outDir);
        writeSymbols(outDir);
        writeFunctions(outDir);
        writeXrefs(outDir);
        writeCallGraph(outDir);
        writeRegisterAnalysis(outDir);
        writeRawDisassembly(outDir);

        println("Done. Output in: " + outDir.getAbsolutePath());
    }

    void writeManifest(File outDir) throws Exception {
        try (PrintWriter out = new PrintWriter(new File(outDir, "manifest.json"), "UTF-8")) {
            Language lang = currentProgram.getLanguage();
            out.println("{");
            out.println("  \"programName\": \"" + esc(currentProgram.getName()) + "\",");
            out.println("  \"language\": \"" + esc(lang.getLanguageID().getIdAsString()) + "\",");
            out.println("  \"imageBase\": \"0x" + currentProgram.getImageBase().toString() + "\",");
            out.println("  \"minAddress\": \"0x" + currentProgram.getMinAddress().toString() + "\",");
            out.println("  \"maxAddress\": \"0x" + currentProgram.getMaxAddress().toString() + "\",");
            out.println("  \"functionCount\": " + currentProgram.getFunctionManager().getFunctionCount() + ",");
            out.println("  \"exportedAt\": \"" + new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss").format(new Date()) + "\",");
            out.println("  \"decompileEnabled\": " + ENABLE_DECOMPILE + ",");
            out.println("  \"minAddrFilter\": \"" + esc(MIN_ADDR) + "\",");
            out.println("  \"maxAddrFilter\": \"" + esc(MAX_ADDR) + "\"");
            out.println("}");
        }
        println("Wrote manifest.json");
    }

    void writeMemoryMap(File outDir) throws Exception {
        try (PrintWriter out = new PrintWriter(new File(outDir, "memory_map.json"), "UTF-8")) {
            out.println("[");
            MemoryBlock[] blocks = currentProgram.getMemory().getBlocks();
            for (int i = 0; i < blocks.length; i++) {
                MemoryBlock b = blocks[i];
                out.println("  {");
                out.println("    \"name\": \"" + esc(b.getName()) + "\",");
                out.println("    \"start\": \"0x" + b.getStart().toString() + "\",");
                out.println("    \"end\": \"0x" + b.getEnd().toString() + "\",");
                out.println("    \"size\": " + b.getSize() + ",");
                out.println("    \"read\": " + b.isRead() + ",");
                out.println("    \"write\": " + b.isWrite() + ",");
                out.println("    \"execute\": " + b.isExecute() + ",");
                out.println("    \"initialized\": " + b.isInitialized());
                out.println("  }" + (i + 1 < blocks.length ? "," : ""));
            }
            out.println("]");
        }
        println("Wrote memory_map.json");
    }

    void writeSymbols(File outDir) throws Exception {
        try (PrintWriter out = new PrintWriter(new File(outDir, "symbols.jsonl"), "UTF-8")) {
            SymbolIterator it = currentProgram.getSymbolTable().getAllSymbols(true);
            int count = 0;
            while (it.hasNext()) {
                monitor.checkCancelled();
                Symbol s = it.next();
                out.println("{\"address\":\"0x" + s.getAddress().toString() + "\",\"name\":\"" + esc(s.getName())
                        + "\",\"type\":\"" + esc(s.getSymbolType().toString()) + "\",\"namespace\":\""
                        + esc(s.getParentNamespace().getName()) + "\"}");
                count++;
            }
            println("Wrote symbols.jsonl (" + count + " symbols)");
        } catch (CancelledException e) {
            println("Symbol export cancelled.");
        }
    }

    void writeFunctions(File outDir) throws Exception {
        // Decompile if enabled globally OR if we have target addresses to decompile.
        boolean needDecomp = ENABLE_DECOMPILE || TARGET_ADDRESSES.length > 0;
        DecompInterface decomp = null;
        if (needDecomp) {
            decomp = new DecompInterface();
            decomp.openProgram(currentProgram);
        }

        AddressSetView scope = null;
        if (!MIN_ADDR.isEmpty() && !MAX_ADDR.isEmpty()) {
            Address min = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(MIN_ADDR);
            Address max = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(MAX_ADDR);
            scope = currentProgram.getAddressFactory().getAddressSet(min, max);
        }

        try (PrintWriter out = new PrintWriter(new File(outDir, "functions.jsonl"), "UTF-8")) {
            FunctionIterator it = (scope != null)
                    ? currentProgram.getFunctionManager().getFunctions(scope, true)
                    : currentProgram.getFunctionManager().getFunctions(true);

            int count = 0;
            try {
                while (it.hasNext()) {
                    monitor.checkCancelled();
                    if (FUNCTION_LIMIT >= 0 && count >= FUNCTION_LIMIT) break;

                    Function fn = it.next();
                    writeOneFunction(out, decomp, fn);
                    count++;

                    if (count % PROGRESS_EVERY == 0) {
                        println("... " + count + " functions dumped (last: " + fn.getName() + " @ 0x"
                                + fn.getEntryPoint() + ")");
                    }
                }
            } catch (CancelledException e) {
                println("Function export cancelled after " + count + " functions.");
            }
            println("Wrote functions.jsonl (" + count + " functions)");
        } finally {
            if (decomp != null) decomp.dispose();
        }
    }

    void writeOneFunction(PrintWriter out, DecompInterface decomp, Function fn) {
        out.println("{");
        out.println("  \"address\": \"0x" + fn.getEntryPoint().toString() + "\",");
        out.println("  \"name\": \"" + esc(fn.getName()) + "\",");
        out.println("  \"size\": " + fn.getBody().getNumAddresses() + ",");

        out.println("  \"instructions\": [");
        Instruction instr = getInstructionAt(fn.getEntryPoint());
        boolean first = true;
        Address end = fn.getBody().getMaxAddress();
        while (instr != null && instr.getAddress().compareTo(end) <= 0) {
            if (!first) out.println(",");
            first = false;

            StringBuilder ops = new StringBuilder();
            for (int i = 0; i < instr.getNumOperands(); i++) {
                if (i > 0) ops.append(", ");
                ops.append(instr.getDefaultOperandRepresentation(i));
            }

            byte[] bytes = new byte[0];
            try {
                bytes = instr.getBytes();
            } catch (Exception ignored) {
            }
            StringBuilder hex = new StringBuilder();
            for (byte b : bytes) hex.append(String.format("%02x", b));

            out.print("    {\"addr\": \"0x" + instr.getAddress() + "\", \"bytes\": \"" + hex
                    + "\", \"mnemonic\": \"" + esc(instr.getMnemonicString()) + "\", \"operands\": \""
                    + esc(ops.toString()) + "\"}");

            instr = instr.getNext();
        }
        out.println();
        out.println("  ],");

        // Decide whether to decompile this function:
        //   - decomp is non-null AND (ENABLE_DECOMPILE is true OR this function's address is in TARGET_ADDRESSES)
        boolean shouldDecomp = decomp != null && (ENABLE_DECOMPILE || isTargetAddress(fn.getEntryPoint()));
        if (shouldDecomp) {
            String c = null;
            try {
                DecompileResults res = decomp.decompileFunction(fn, 15, monitor);
                if (res != null && res.decompileCompleted() && res.getDecompiledFunction() != null) {
                    c = res.getDecompiledFunction().getC();
                }
            } catch (Exception ignored) {
            }
            out.println("  \"decompiled\": " + (c != null ? "\"" + esc(c) + "\"" : "null"));
        } else {
            out.println("  \"decompiled\": null");
        }
        out.println("}");
        out.println();
    }

    static String esc(String s) {
        if (s == null) return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:
                    if (c < 0x20) {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        return sb.toString();
    }

    // ----------------------------------------------------------------------
    // New exports: xrefs, call graph, register analysis
    // ----------------------------------------------------------------------

    boolean isTargetAddress(Address addr) {
        String hex = addr.toString();
        for (String t : TARGET_ADDRESSES) {
            if (t.equalsIgnoreCase(hex)) return true;
        }
        return false;
    }

    /**
     * Write raw_disassembly.jsonl: raw disassembly for suspicious functions.
     * 
     * Auto-detects suspicious functions (small functions that only save/restore registers)
     * and exports the surrounding context. This helps investigate functions that Ghidra
     * has misidentified as separate functions when they're actually parts of larger functions.
     */
    void writeRawDisassembly(File outDir) throws Exception {
        if (!ENABLE_AUTO_DETECT_SUSPICIOUS) return;
        
        try (PrintWriter out = new PrintWriter(new File(outDir, "raw_disassembly.jsonl"), "UTF-8")) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            int suspiciousCount = 0;
            
            try {
                while (it.hasNext()) {
                    monitor.checkCancelled();
                    Function fn = it.next();
                    
                    // Check if this function is suspicious (small and only saves/restores registers)
                    if (!isSuspiciousFunction(fn)) continue;
                    
                    // Calculate context range
                    Address start = fn.getEntryPoint();
                    Address end = fn.getBody().getMaxAddress();
                    
                    // Extend range to include context
                    Address contextStart = start.subtract(CONTEXT_BYTES_BEFORE);
                    Address contextEnd = end.add(CONTEXT_BYTES_AFTER);
                    
                    // Clamp to valid memory range
                    if (contextStart.compareTo(currentProgram.getMinAddress()) < 0) {
                        contextStart = currentProgram.getMinAddress();
                    }
                    if (contextEnd.compareTo(currentProgram.getMaxAddress()) > 0) {
                        contextEnd = currentProgram.getMaxAddress();
                    }
                    
                    writeContextRange(out, fn, contextStart, contextEnd);
                    suspiciousCount++;
                    
                    if (suspiciousCount % 100 == 0) {
                        println("... " + suspiciousCount + " suspicious functions dumped");
                    }
                }
            } catch (CancelledException e) {
                println("Raw disassembly export cancelled after " + suspiciousCount + " functions.");
            }
            
            println("Wrote raw_disassembly.jsonl (" + suspiciousCount + " suspicious functions)");
        }
    }
    
    /**
     * Check if a function is suspicious (small and only contains register saves/restores).
     * These are likely parts of larger functions that Ghidra misidentified.
     */
    boolean isSuspiciousFunction(Function fn) {
        // Skip if function is too large
        if (fn.getBody().getNumAddresses() > SUSPICIOUS_MAX_SIZE) return false;
        
        // Check if function only contains register saves/restores
        int saveCount = 0;
        int restoreCount = 0;
        int otherCount = 0;
        
        Instruction instr = getInstructionAt(fn.getEntryPoint());
        Address end = fn.getBody().getMaxAddress();
        while (instr != null && instr.getAddress().compareTo(end) <= 0) {
            String mnemonic = instr.getMnemonicString();
            if (mnemonic.startsWith("st")) {
                saveCount++;
            } else if (mnemonic.equals("lwz") || mnemonic.equals("ld")) {
                restoreCount++;
            } else if (mnemonic.equals("blr") || mnemonic.equals("bl") || mnemonic.equals("b")) {
                // Branches are expected in thunks
            } else {
                otherCount++;
            }
            instr = instr.getNext();
        }
        
        // Suspicious if mostly saves/restores with no other instructions
        return otherCount == 0 && (saveCount > 0 || restoreCount > 0);
    }
    
    /**
     * Write a context range to the output file.
     */
    void writeContextRange(PrintWriter out, Function fn, Address start, Address end) {
        out.println("{");
        out.println("  \"function\": \"" + esc(fn.getName()) + "\",");
        out.println("  \"functionAddress\": \"0x" + fn.getEntryPoint().toString() + "\",");
        out.println("  \"functionSize\": " + fn.getBody().getNumAddresses() + ",");
        out.println("  \"contextStart\": \"0x" + start.toString() + "\",");
        out.println("  \"contextEnd\": \"0x" + end.toString() + "\",");
        out.println("  \"instructions\": [");
        
        Instruction instr = getInstructionAt(start);
        boolean first = true;
        while (instr != null && instr.getAddress().compareTo(end) <= 0) {
            if (!first) out.println(",");
            first = false;
            
            StringBuilder ops = new StringBuilder();
            for (int i = 0; i < instr.getNumOperands(); i++) {
                if (i > 0) ops.append(", ");
                ops.append(instr.getDefaultOperandRepresentation(i));
            }
            
            byte[] bytes = new byte[0];
            try {
                bytes = instr.getBytes();
            } catch (Exception ignored) {
            }
            StringBuilder hex = new StringBuilder();
            for (byte b : bytes) hex.append(String.format("%02x", b));
            
            // Find which function (if any) this instruction belongs to
            Function containingFn = currentProgram.getFunctionManager().getFunctionContaining(instr.getAddress());
            String fnName = containingFn != null ? containingFn.getName() : "null";
            
            // Mark if this instruction is inside the target function
            boolean inTarget = containingFn != null && containingFn.getEntryPoint().equals(fn.getEntryPoint());
            
            out.print("    {\"addr\": \"0x" + instr.getAddress() + "\", \"bytes\": \"" + hex
                    + "\", \"mnemonic\": \"" + esc(instr.getMnemonicString()) + "\", \"operands\": \""
                    + esc(ops.toString()) + "\", \"function\": \"" + esc(fnName) + "\", \"inTarget\": "
                    + inTarget + "}");
            
            instr = instr.getNext();
        }
        out.println();
        out.println("  ]");
        out.println("}");
        out.println();
    }

    /**
     * Write xrefs.jsonl: one JSON object per call/reference instruction.
     * Each line: {"from":"0x...","to":"0x...","type":"direct|indirect|reference","mnemonic":"bl|bctrl|...","function":"sub_..."}
     */
    void writeXrefs(File outDir) throws Exception {
        try (PrintWriter out = new PrintWriter(new File(outDir, "xrefs.jsonl"), "UTF-8")) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            int count = 0;
            try {
                while (it.hasNext()) {
                    monitor.checkCancelled();
                    Function fn = it.next();
                    Instruction instr = getInstructionAt(fn.getEntryPoint());
                    Address end = fn.getBody().getMaxAddress();
                    while (instr != null && instr.getAddress().compareTo(end) <= 0) {
                        // Direct call: bl to known address
                        if (instr.getMnemonicString().equals("bl") || instr.getMnemonicString().equals("b")) {
                            Address target = getBranchTarget(instr);
                            if (target != null) {
                                Function callee = currentProgram.getFunctionManager().getFunctionContaining(target);
                                out.println("{\"from\":\"0x" + instr.getAddress() + "\",\"to\":\"0x" + target
                                        + "\",\"type\":\"direct\",\"mnemonic\":\"" + esc(instr.getMnemonicString())
                                        + "\",\"caller\":\"" + esc(fn.getName()) + "\",\"callee\":"
                                        + (callee != null ? "\"" + esc(callee.getName()) + "\"" : "null") + "}");
                                count++;
                            }
                        }
                        // Indirect call: bctrl
                        else if (instr.getMnemonicString().equals("bctrl")) {
                            out.println("{\"from\":\"0x" + instr.getAddress() + "\",\"to\":null"
                                    + ",\"type\":\"indirect\",\"mnemonic\":\"bctrl\",\"caller\":\""
                                    + esc(fn.getName()) + "\",\"callee\":null}");
                            count++;
                        }
                        instr = instr.getNext();
                    }
                }
            } catch (CancelledException e) {
                println("Xref export cancelled after " + count + " xrefs.");
            }
            println("Wrote xrefs.jsonl (" + count + " xrefs)");
        }
    }

    /**
     * Write call_graph.jsonl: aggregated caller -> callee edges.
     * Each line: {"caller":"sub_...","callee":"sub_...","count":N}
     * Indirect calls are listed as callee=null.
     */
    void writeCallGraph(File outDir) throws Exception {
        // Use a map to aggregate edges: caller_name -> (callee_name -> count)
        java.util.Map<String, java.util.Map<String, Integer>> edges = new java.util.HashMap<>();
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        try {
            while (it.hasNext()) {
                monitor.checkCancelled();
                Function fn = it.next();
                Instruction instr = getInstructionAt(fn.getEntryPoint());
                Address end = fn.getBody().getMaxAddress();
                while (instr != null && instr.getAddress().compareTo(end) <= 0) {
                    String calleeName = null;
                    if (instr.getMnemonicString().equals("bl")) {
                        Address target = getBranchTarget(instr);
                        if (target != null) {
                            Function callee = currentProgram.getFunctionManager().getFunctionContaining(target);
                            if (callee != null) calleeName = callee.getName();
                        }
                    } else if (instr.getMnemonicString().equals("bctrl")) {
                        calleeName = "<indirect>";
                    }
                    if (calleeName != null) {
                        java.util.Map<String, Integer> inner = edges.computeIfAbsent(fn.getName(), k -> new java.util.HashMap<>());
                        inner.merge(calleeName, 1, Integer::sum);
                    }
                    instr = instr.getNext();
                }
            }
        } catch (CancelledException e) {
            println("Call graph export cancelled.");
        }

        try (PrintWriter out = new PrintWriter(new File(outDir, "call_graph.jsonl"), "UTF-8")) {
            int count = 0;
            for (java.util.Map.Entry<String, java.util.Map<String, Integer>> e : edges.entrySet()) {
                for (java.util.Map.Entry<String, Integer> inner : e.getValue().entrySet()) {
                    out.println("{\"caller\":\"" + esc(e.getKey()) + "\",\"callee\":\"" + esc(inner.getKey())
                            + "\",\"count\":" + inner.getValue() + "}");
                    count++;
                }
            }
            println("Wrote call_graph.jsonl (" + count + " edges)");
        }
    }

    /**
     * Write register_analysis.jsonl: for each function, which registers are
     * saved/restored (prologue/epilogue) and which are read/written.
     * This helps find functions that might corrupt callee-saved registers.
     */
    void writeRegisterAnalysis(File outDir) throws Exception {
        try (PrintWriter out = new PrintWriter(new File(outDir, "register_analysis.jsonl"), "UTF-8")) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            int count = 0;
            try {
                while (it.hasNext()) {
                    monitor.checkCancelled();
                    Function fn = it.next();
                    writeOneFunctionRegisterAnalysis(out, fn);
                    count++;
                    if (count % PROGRESS_EVERY == 0) {
                        println("... " + count + " functions analyzed (last: " + fn.getName() + ")");
                    }
                }
            } catch (CancelledException e) {
                println("Register analysis cancelled after " + count + " functions.");
            }
            println("Wrote register_analysis.jsonl (" + count + " functions)");
        }
    }

    void writeOneFunctionRegisterAnalysis(PrintWriter out, Function fn) {
        // Track registers written and read
        java.util.Set<String> written = new java.util.HashSet<>();
        java.util.Set<String> read = new java.util.HashSet<>();
        java.util.Set<String> saved = new java.util.HashSet<>(); // saved to stack in prologue
        java.util.Set<String> restored = new java.util.HashSet<>(); // restored from stack in epilogue

        Instruction instr = getInstructionAt(fn.getEntryPoint());
        Address end = fn.getBody().getMaxAddress();
        while (instr != null && instr.getAddress().compareTo(end) <= 0) {
            String mnemonic = instr.getMnemonicString();
            int numOperands = instr.getNumOperands();
            String[] operands = new String[numOperands];
            for (int i = 0; i < numOperands; i++) {
                operands[i] = instr.getDefaultOperandRepresentation(i);
            }

            // stw/stw/r31,-N(r1) = save register to stack
            if (mnemonic.startsWith("st") && operands.length >= 2) {
                String reg = operands[0].split(",")[0].trim();
                if (isGpr(reg)) {
                    // Check if target is r1 (stack) with negative offset
                    String target = operands[1];
                    if (target.contains("r1") && (target.startsWith("-") || target.contains("("))) {
                        saved.add(reg);
                    }
                }
            }
            // lwz r31,-N(r1) = restore register from stack
            else if (mnemonic.equals("lwz") && operands.length >= 2) {
                String reg = operands[0].split(",")[0].trim();
                String target = operands[1];
                if (isGpr(reg) && target.contains("r1")) {
                    restored.add(reg);
                }
            }
            // General read/write tracking
            for (String op : operands) {
                String reg = op.split(",")[0].trim();
                if (isGpr(reg)) {
                    // If it's a store instruction, the first operand is written
                    if (mnemonic.startsWith("st")) {
                        written.add(reg);
                    } else {
                        read.add(reg);
                    }
                }
            }
            instr = instr.getNext();
        }

        // Registers that are saved but not restored (potential bug)
        java.util.Set<String> notRestored = new java.util.HashSet<>(saved);
        notRestored.removeAll(restored);

        out.println("{\"address\":\"0x" + fn.getEntryPoint() + "\",\"name\":\"" + esc(fn.getName())
                + "\",\"saved\":" + jsonArray(saved) + ",\"restored\":" + jsonArray(restored)
                + ",\"notRestored\":" + jsonArray(notRestored) + ",\"written\":" + jsonArray(written)
                + ",\"read\":" + jsonArray(read) + "}");
    }

    boolean isGpr(String s) {
        if (s == null) return false;
        s = s.trim();
        if (s.startsWith("r") && s.length() > 1) {
            try {
                int n = Integer.parseInt(s.substring(1));
                return n >= 0 && n <= 31;
            } catch (NumberFormatException e) {
                return false;
            }
        }
        return false;
    }

    /**
     * Get the target address of a branch instruction (bl, b).
     * Returns null if the instruction is not a branch or the target cannot be determined.
     */
    Address getBranchTarget(Instruction instr) {
        // Parse the operand string to extract the target address
        if (instr.getNumOperands() > 0) {
            String operand = instr.getDefaultOperandRepresentation(0);
            if (operand != null && operand.startsWith("0x")) {
                try {
                    return currentProgram.getAddressFactory().getDefaultAddressSpace()
                            .getAddress(operand.substring(2));
                } catch (Exception e) {
                    return null;
                }
            }
        }
        
        return null;
    }

    String jsonArray(java.util.Set<String> set) {
        StringBuilder sb = new StringBuilder("[");
        boolean first = true;
        for (String s : set) {
            if (!first) sb.append(",");
            sb.append("\"").append(esc(s)).append("\"");
            first = false;
        }
        sb.append("]");
        return sb.toString();
    }
}
