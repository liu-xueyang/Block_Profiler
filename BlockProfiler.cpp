/*
 * Copyright (C) 2007-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::endl;
using std::string;
using std::ios;
using std::setw;
using std::dec;
using std::hex;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 insCount    = 0; //number of dynamically executed instructions
UINT64 bblCount    = 0; //number of dynamically executed basic blocks
UINT64 threadCount = 0; //total number of threads, including main thread

int debug = 0;
int num_region = 0;

std::ostream* out = &cerr;
std::ofstream TraceFile;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "profile.out", "specify file name for BlockProfiler output");

KNOB< string > KnobTraceFile(KNOB_MODE_WRITEONCE, "pintool", "t", "trace.out", "specify file name for BlockProfiler trace output");

KNOB< BOOL > KnobCount(KNOB_MODE_WRITEONCE, "pintool", "count", "1",
                       "count instructions, basic blocks and threads in the application");

KNOB< BOOL > KnobValues(KNOB_MODE_WRITEONCE, "pintool", "values", "1", "Output memory values reads and written");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool prints out the number of dynamically executed " << endl
         << "instructions, basic blocks and threads in the application." << endl
         << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

static VOID EmitMem(VOID* ea, INT32 size)
{
    if (!KnobValues) return;

    switch (size)
    {
        case 0:
            TraceFile << setw(1);
            break;

        case 1:
            TraceFile << static_cast< UINT32 >(*static_cast< UINT8* >(ea));
            break;

        case 2:
            TraceFile << *static_cast< UINT16* >(ea);
            break;

        case 4:
            TraceFile << *static_cast< UINT32* >(ea);
            break;

        case 8:
            TraceFile << *static_cast< UINT64* >(ea);
            break;

        default:
            TraceFile.unsetf(ios::showbase);
            TraceFile << setw(1) << "0x";
            for (INT32 i = 0; i < size; i++)
            {
                TraceFile << static_cast< UINT32 >(static_cast< UINT8* >(ea)[i]);
            }
            TraceFile.setf(ios::showbase);
            break;
    }
}

static VOID RecordMem(VOID* ip, CHAR r, VOID* addr, INT32 size, BOOL isPrefetch)
{
    TraceFile << ip << ": " << r << " " << setw(2 + 2 * sizeof(ADDRINT)) << addr << " " << dec << setw(2) << size << " " << hex
              << setw(2 + 2 * sizeof(ADDRINT));
    if (!isPrefetch) EmitMem(addr, size);
    TraceFile << endl;
}

static VOID* WriteAddr;
static INT32 WriteSize;

static VOID RecordWriteAddrSize(VOID* addr, INT32 size)
{
    WriteAddr = addr;
    WriteSize = size;
}

static VOID RecordMemWrite(VOID* ip) { RecordMem(ip, 'W', WriteAddr, WriteSize, false); }

/*!
 * Increase counter of the executed basic blocks and instructions.
 * This function is called for every basic block when it is about to be executed.
 * @param[in]   numInstInBbl    number of instructions in the basic block
 * @note use atomic operations for multi-threaded applications
 */
VOID CountBbl(UINT32 numInstInBbl)
{
    bblCount++;
    insCount += numInstInBbl;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

/*!
 * Insert call to the CountBbl() analysis routine before every basic block 
 * of the trace.
 * This function is called every time a new trace is encountered.
 * @param[in]   trace    trace to be instrumented
 * @param[in]   v        value specified by the tool in the TRACE_AddInstrumentFunction
 *                       function call
 */
VOID Trace(TRACE trace, VOID* v)
{
    // Visit every basic block in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Insert a call to CountBbl() before every basic bloc, passing the number of instructions
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)CountBbl, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
    }
   
}

VOID PIN_FAST_ANALYSIS_CALL ProfilingBreakpoint(char *name, void* siteObj)
{
  cerr << "Encountered profiling breakpoint\n" << endl;
}

VOID Image(IMG img, VOID *v)
{
    string startSiteName("ANNOTATE_SITE_BEGIN_WKR");

    if (debug) cerr << "Looking for Func " << startSiteName  << endl;
    AFUNPTR afunptr(0);
    afunptr = (AFUNPTR)ProfilingBreakpoint;
    RTN rtn = RTN_FindByName(img, startSiteName.c_str());;
    if (!RTN_Valid(rtn)) {
        if (debug) cerr << "ANNOTATE_SITE_BEGIN_WKR is an invalid function" << endl;
        return;
    }
    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, afunptr,
            IARG_FAST_ANALYSIS_CALL,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
    RTN_Close(rtn);    
}

VOID Instruction(INS ins, VOID* v)
{
    // instruments loads using a predicated call, i.e.
    // the call happens iff the load will be actually executed

    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_INST_PTR, IARG_UINT32, 'R', IARG_MEMORYREAD_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_BOOL, INS_IsPrefetch(ins), IARG_END);
    }

    if (INS_HasMemoryRead2(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_INST_PTR, IARG_UINT32, 'R', IARG_MEMORYREAD2_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_BOOL, INS_IsPrefetch(ins), IARG_END);
    }

    // instruments stores using a predicated call, i.e.
    // the call happens iff the store will be actually executed
    if (INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteAddrSize, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
                                 IARG_END);

        if (INS_IsValidForIpointAfter(ins))
        {
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_END);
        }
        if (INS_IsValidForIpointTakenBranch(ins))
        {
            INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_END);
        }
    }
}

/*!
 * Increase counter of threads in the application.
 * This function is called for every thread created by the application when it is
 * about to start running (including the root thread).
 * @param[in]   threadIndex     ID assigned by PIN to the new thread
 * @param[in]   ctxt            initial register state for the new thread
 * @param[in]   flags           thread creation flags (OS specific)
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddThreadStartFunction function call
 */
VOID ThreadStart(THREADID threadIndex, CONTEXT* ctxt, INT32 flags, VOID* v) { threadCount++; }

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID* v)
{
    *out << "===============================================" << endl;
    *out << "BlockProfiler analysis results: " << endl;
    *out << "Number of instructions: " << insCount << endl;
    *out << "Number of basic blocks: " << bblCount << endl;
    *out << "Number of threads: " << threadCount << endl;
    *out << "===============================================" << endl;

    TraceFile << "#eof" << endl;
    TraceFile.close();
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    // configure profiling output
    string fileName = KnobOutputFile.Value();
    if (!fileName.empty())
    {
        out = new std::ofstream(fileName.c_str());
    }

    // configure trace output 
    string trace_header = string("#\n"
                                 "# Memory Access Trace Generated By Pin\n"
                                 "#\n");
    TraceFile.open(KnobTraceFile.Value().c_str());
    TraceFile.write(trace_header.c_str(), trace_header.size());
    TraceFile.setf(ios::showbase);


    if (KnobCount)
    {
        PIN_InitSymbols();

        // Register function to be called to instrument traces
        TRACE_AddInstrumentFunction(Trace, 0);
        IMG_AddInstrumentFunction(Image, 0);
        INS_AddInstrumentFunction(Instruction, 0);

        // Register function to be called for every thread before it starts running
        PIN_AddThreadStartFunction(ThreadStart, 0);

        // Register function to be called when the application exits
        PIN_AddFiniFunction(Fini, 0);
    }

    cerr << "===============================================" << endl;
    cerr << "This application is instrumented by BlockProfiler" << endl;
    if (!KnobOutputFile.Value().empty())
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
