
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <string>

using namespace std;

#define NUM_INSTR_DESTINATIONS 2
#define NUM_INSTR_DESTINATIONS_SPARC 4
#define NUM_INSTR_SOURCES 4
#define ATOMIC_INCREMENT(x) __sync_fetch_and_add(&(x), 1)
#define ATOMIC_DECREMENT(x) __sync_sub_and_fetch(&(x), 1)
#define BUF_SIZE 100000
#define MAX_THREADS 4

typedef struct trace_instr_format {
    uint64_t ip;  // instruction pointer (program counter) value

    uint8_t is_branch;    // is this branch
    uint8_t branch_taken; // if so, is this taken

    uint8_t destination_registers[NUM_INSTR_DESTINATIONS_SPARC]; // output registers
    uint8_t source_registers[NUM_INSTR_SOURCES];           // input registers

    uint64_t destination_memory[NUM_INSTR_DESTINATIONS_SPARC]; // output memory
    uint64_t source_memory[NUM_INSTR_SOURCES];           // input memory

    uint8_t asid[2];
    
    trace_instr_format()
    {
	asid[0] = 4;
	asid[1] = 16;
    }
} trace_instr_format_t;

/* ================================================================== */
// Global variables 
/* ================================================================== */

UINT64 instrCount = 0;
UINT64 iCount[MAX_THREADS] = {0};
PIN_LOCK lock;

FILE* out[MAX_THREADS];
string fileName;

bool output_file_closed[MAX_THREADS] = {0};
bool tracing_on = false;

trace_instr_format_t curr_instr[MAX_THREADS];

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool", "o", "champsim.trace", 
        "specify file name for Champsim tracer output");

KNOB<UINT64> KnobSkipInstructions(KNOB_MODE_WRITEONCE, "pintool", "s", "0", 
        "How many instructions to skip before tracing begins");

KNOB<UINT64> KnobTraceInstructions(KNOB_MODE_WRITEONCE, "pintool", "t", "1000000", 
        "How many instructions to trace");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool creates a register and memory access trace" << endl 
        << "Specify the output trace file with -o" << endl 
        << "Specify the number of instructions to skip before tracing with -s" << endl
        << "Specify the number of instructions to trace with -t" << endl << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

void BeginInstruction(VOID *ip, UINT32 op_code, THREADID threadid)
{
    instrCount++;
    //printf("[%p %u %s ", ip, op_code, (char*)opstring);

    if(instrCount > KnobSkipInstructions.Value() &&
	 instrCount < (KnobTraceInstructions.Value()+KnobSkipInstructions.Value())) 
    {
	//assert(threadid < MAX_THREADS);
        tracing_on = true;
    }

    if(!tracing_on)
	return;

    // reset the current instruction
    curr_instr[threadid].ip = (uint64_t)ip;
    iCount[threadid] = instrCount;

    curr_instr[threadid].is_branch = 0;
    curr_instr[threadid].branch_taken = 0;

    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++) 
    {
        curr_instr[threadid].destination_registers[i] = 0;
        curr_instr[threadid].destination_memory[i] = 0;
    }

    for(int i=0; i<NUM_INSTR_SOURCES; i++) 
    {
        curr_instr[threadid].source_registers[i] = 0;
        curr_instr[threadid].source_memory[i] = 0;
    }
}

void EndInstruction(THREADID threadid)
{

    //printf("\n");

    if(iCount[threadid] > KnobSkipInstructions.Value())
    {
        tracing_on = true;

        if(iCount[threadid] <= (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
        {
            // keep tracing
            fwrite(&curr_instr[threadid], sizeof(trace_instr_format_t), 1, out[threadid]);
        }
        else
        {
            tracing_on = false;
            // close down the file, we're done tracing
            if(!output_file_closed[threadid])
            {
                fclose(out[threadid]);
                output_file_closed[threadid] = true;
            }
            exit(0);
        }
    }
}

void BranchOrNot(UINT32 taken, THREADID threadid)
{
    //printf("[%d] ", taken);

    curr_instr[threadid].is_branch = 1;
    if(taken != 0)
    {
        curr_instr[threadid].branch_taken = 1;
    }
}

void RegRead(UINT32 i, UINT32 index, THREADID threadid)
{
    //if(!tracing_on) return;
    if(!tracing_on) {
	return;
    }

    REG r = (REG)i;

    /*
       if(r == 26)
       {
    // 26 is the IP, which is read and written by branches
    return;
    }
    */

    //cout << r << " " << REG_StringShort((REG)r) << " " ;
    //cout << REG_StringShort((REG)r) << " " ;

    //printf("%d ", (int)r);

    // check to see if this register is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_SOURCES; i++)
    {
        if(curr_instr[threadid].source_registers[i] == ((uint8_t)r))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_SOURCES; i++)
        {
            if(curr_instr[threadid].source_registers[i] == 0)
            {
                curr_instr[threadid].source_registers[i] = (uint8_t)r;
                break;
            }
        }
    }
}

void RegWrite(REG i, UINT32 index, THREADID threadid)
{
    //if(!tracing_on) return;
    if(!tracing_on) {
	return;
    }

    REG r = (REG)i;

    /*
       if(r == 26)
       {
    // 26 is the IP, which is read and written by branches
    return;
    }
    */

    //cout << "<" << r << " " << REG_StringShort((REG)r) << "> ";
    //cout << "<" << REG_StringShort((REG)r) << "> ";

    //printf("<%d> ", (int)r);

    int already_found = 0;
    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
    {
        if(curr_instr[threadid].destination_registers[i] == ((uint8_t)r))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
        {
            if(curr_instr[threadid].destination_registers[i] == 0)
            {
                curr_instr[threadid].destination_registers[i] = (uint8_t)r;
                break;
            }
        }
    }
    /*
       if(index==0)
       {
       curr_instr.destination_register = (unsigned long long int)r;
       }
       */
}

void MemoryRead(VOID* addr, UINT32 index, UINT32 read_size, THREADID threadid)
{
    //if(!tracing_on) return;
    if(!tracing_on) {
	return;
    }

    //printf("0x%llx,%u ", (unsigned long long int)addr, read_size);

    // check to see if this memory read location is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_SOURCES; i++)
    {
        if(curr_instr[threadid].source_memory[i] == ((uint64_t)addr))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_SOURCES; i++)
        {
            if(curr_instr[threadid].source_memory[i] == 0)
            {
                curr_instr[threadid].source_memory[i] = (uint64_t)addr;
                break;
            }
        }
    }
}

void MemoryWrite(VOID* addr, UINT32 index, THREADID threadid)
{
    //if(!tracing_on) return;
    if(!tracing_on) {
	return;
    }

    //printf("(0x%llx) ", (unsigned long long int) addr);

    // check to see if this memory write location is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
    {
        if(curr_instr[threadid].destination_memory[i] == ((uint64_t)addr))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
        {
            if(curr_instr[threadid].destination_memory[i] == 0)
            {
                curr_instr[threadid].destination_memory[i] = (uint64_t)addr;
                break;
            }
        }
    }
    /*
       if(index==0)
       {
       curr_instr.destination_memory = (long long int)addr;
       }
       */
}

// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    //assert(threadid < MAX_THREADS);
    PIN_GetLock(&lock, threadid+1);
    int id = threadid;
    stringstream s2;
    s2 << id;
    string tid = fileName + "_thread" + s2.str() + ".trace";
    out[threadid] = fopen(tid.c_str(), "ab");
    if (!out[threadid]) 
    {
       cout << "Couldn't open output trace file. Exiting." << endl;
        exit(1);
    }
    fprintf(stdout, "thread %d process id: %d\n", threadid, PIN_GetPid());
    fflush(stdout);
    PIN_ReleaseLock(&lock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&lock, threadid+1);
    fprintf(stdout, "thread end %d\n",threadid);
    fflush(stdout);
    fclose(out[threadid]);
    PIN_ReleaseLock(&lock);
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // begin each instruction with this function
    UINT32 opcode = INS_Opcode(ins);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BeginInstruction, IARG_INST_PTR, IARG_UINT32, opcode, IARG_THREAD_ID, IARG_END);

    // instrument branch instructions
    if(INS_IsBranch(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BranchOrNot, IARG_BRANCH_TAKEN, IARG_THREAD_ID, IARG_END);

    // instrument register reads
    UINT32 readRegCount = INS_MaxNumRRegs(ins);
    for(UINT32 i=0; i<readRegCount; i++) 
    {
        UINT32 regNum = INS_RegR(ins, i);

        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RegRead,
                IARG_UINT32, regNum, IARG_UINT32, i, IARG_THREAD_ID,
                IARG_END);
    }

    // instrument register writes
    UINT32 writeRegCount = INS_MaxNumWRegs(ins);
    for(UINT32 i=0; i<writeRegCount; i++) 
    {
        UINT32 regNum = INS_RegW(ins, i);

        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RegWrite,
                IARG_UINT32, regNum, IARG_UINT32, i, IARG_THREAD_ID,
                IARG_END);
    }

    // instrument memory reads and writes
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++) 
    {
        if (INS_MemoryOperandIsRead(ins, memOp)) 
        {
            UINT32 read_size = INS_MemoryReadSize(ins);

            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryRead,
                    IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_UINT32, read_size, IARG_THREAD_ID,
                    IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryWrite,
                    IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_THREAD_ID,
                    IARG_END);
        }
    }

    // finalize each instruction with this function
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)EndInstruction, IARG_THREAD_ID, IARG_END);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{
    // close the file if it hasn't already been closed
    //if(!output_file_closed[threadid]) 
    //{
        //fclose(out);
        //output_file_closed = true;
    //}
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    if( PIN_Init(argc,argv) )
        return Usage();

    // Initialize the pin lock
    PIN_InitLock(&lock);

    fileName = KnobOutputFile.Value();

    //out = fopen(fileName, "ab");
    //if (!out) 
    //{
    //   cout << "Couldn't open output trace file. Exiting." << endl;
    //    exit(1);
    //}

    // Register function to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Analysis routines to be called when a thread begins/ends
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register function to be called when the application exits
    //PIN_AddFiniFunction(Fini, 0);

    //cerr <<  "===============================================" << endl;
    //cerr <<  "This application is instrumented by the Champsim Trace Generator" << endl;
    //cerr <<  "Trace saved in " << KnobOutputFile.Value() << endl;
    //cerr <<  "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
