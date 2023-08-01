// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you are using the "stub" file system, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

int Kernel::usedPhysicalPages[NumPhysPages]={0};
// initialized the static array usedPhysicalPages in the addrspace.cc

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
#ifdef RDATA
    noffH->readonlyData.size = WordToHost(noffH->readonlyData.size);
    noffH->readonlyData.virtualAddr = 
           WordToHost(noffH->readonlyData.virtualAddr);
    noffH->readonlyData.inFileAddr = 
           WordToHost(noffH->readonlyData.inFileAddr);
#endif 
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);

#ifdef RDATA
    DEBUG(dbgAddr, "code = " << noffH->code.size <<  
                   " readonly = " << noffH->readonlyData.size <<
                   " init = " << noffH->initData.size <<
                   " uninit = " << noffH->uninitData.size << "\n");
#endif
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
    /*
    pageTable = new TranslationEntry[NumPhysPages];
    for (int i = 0; i < NumPhysPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virt page # = phys page #
	pageTable[i].physicalPage = i;
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  
    }
    
    // zero out the entire address space
    bzero(kernel->machine->mainMemory, MemorySize);
    */
    // we don't use the original 1 to 1 (virtual to physical)page table
    // we'll establish another page table in Load()  
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   // When the thread is finished, 
   // make sure to release the address space and restore physical page status.
   for(int i=0;i< numPages;i++)
   {
        kernel->usedPhysicalPages[pageTable[i].physicalPage]=0;
        // every thread own their pagetable, 
        // so use the numPages(virtual) to use pageTable[i].physicalPage 
        // to find corresponding usedPhysicalPages,
        // usedPhysicalPages[pageTable[i].physicalPage]=0 is to release the PhysicalPages which thread uses
   }

   delete []pageTable;
   // delete the thread's pagetable
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

#ifdef RDATA
// how big is address space?
    size = noffH.code.size + noffH.readonlyData.size + noffH.initData.size +
           noffH.uninitData.size + UserStackSize;	
                                                // we need to increase the size
						// to leave room for the stack
#else
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
#endif
    
    numPages = divRoundUp(size, PageSize);
    // numPages: need how much virtual memory pages
    size = numPages * PageSize;

    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

	pageTable = new TranslationEntry[numPages];
	for(unsigned int i = 0; i < numPages; i++){
		pageTable[i].virtualPage = i;
		int index = 0;
		while(index < NumPhysPages && kernel->usedPhysicalPages[index] == 1)index++;
        // find the usedPhysicalPages
        if(index == NumPhysPages) ExceptionHandler(MemoryLimitException);
        // if there is no usedPhysicalPage,then it means the insufficent memory,call exception
		pageTable[i].physicalPage = index;
		kernel->usedPhysicalPages[index] = 1;
		pageTable[i].valid = true;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
	}

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

// then, copy in the code and data segments into memory
// Note: this code assumes that virtual address = physical address
    if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	    DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        
        int page_number = noffH.code.virtualAddr / PageSize;
        // calculate the page_number
        int page_offset = noffH.code.virtualAddr % PageSize;
        // calculate the page_offset
        int remaining_Data_Size = noffH.code.size;
        // record the remaing_Data_Size
        int inFileAddr = noffH.code.inFileAddr;
        // record the inFileAddr
        
        while (remaining_Data_Size > 0) {
            int Data_to_Load_Size = std::min(PageSize-page_offset, remaining_Data_Size);
            // calculate the the data size can be loaded this time
            // first time would be start from page_offset to the end of the page
            // last time would be the remaining data size which is less than a page size
            
            executable->ReadAt(
                &(kernel->machine->mainMemory[pageTable[page_number].physicalPage * PageSize + page_offset]),
                Data_to_Load_Size, inFileAddr);
                // load data from the inFileAddrr(which is from the disk) to the mainMemory
                // and copy the Data_to_Load_Size from the inFileAddr to mainMemory
                // the physical address in the mainMemory would be 
                // the the the index of physical Page * PageSize + page_offset

            inFileAddr += Data_to_Load_Size;
            // change the start point from the inFileAddr
            remaining_Data_Size -= Data_to_Load_Size;
            // update the remaining_Data_Size
            page_offset = 0;
            // from the start of the page
            page_number++;
            // page_number+1, move to next virtual page number
        }
    }
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
        DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);

        int page_number = noffH.initData.virtualAddr / PageSize;
        // calculate the page_number
        int page_offset = noffH.initData.virtualAddr % PageSize;
        // calculate the page_offset
        int remaining_Data_Size = noffH.initData.size;
        // record the remaing_Data_Size
        int inFileAddr = noffH.initData.inFileAddr;
        // record the inFileAddr
        while (remaining_Data_Size > 0) {
            int Data_to_Load_Size = std::min(PageSize-page_offset, remaining_Data_Size);
            // calculate the the data size can be loaded this time
            // first time would be start from page_offset to the end of the page
            // last time would be the remaining data size which is less than a page size
            executable->ReadAt(
                &(kernel->machine->mainMemory[pageTable[page_number].physicalPage * PageSize + page_offset]),
                Data_to_Load_Size, inFileAddr);
                // load data from the inFileAddrr(which is from the disk) to the mainMemory
                // and copy the Data_to_Load_Size from the inFileAddr to mainMemory
                // the physical address in the mainMemory would be 
                // the the the index of physical Page * PageSize + page_offset
            inFileAddr += Data_to_Load_Size;
            // change the start point from the inFileAddr
            remaining_Data_Size -= Data_to_Load_Size;
            // update the remaining_Data_Size
            page_offset = 0;
            // from the start of the page
            page_number++;
            // page_number+1, move to next virtual page
        }
    }

#ifdef RDATA
    if (noffH.readonlyData.size > 0) {
        DEBUG(dbgAddr, "Initializing read only data segment.");
        DEBUG(dbgAddr, noffH.readonlyData.virtualAddr << ", " << noffH.readonlyData.size);

        int page_number = noffH.readonlyData.virtualAddr / PageSize;
        // calculate the page_number
        int page_offset = noffH.readonlyData.virtualAddr % PageSize;
        // calculate the page_offset
        int remaining_Data_Size = noffH.readonlyData.size;
        // record the remaing_Data_Size
        int inFileAddr = noffH.readonlyData.inFileAddr;
        // record the inFileAddr

        while (remaining_Data_Size > 0) {
            int Data_to_Load_Size = std::min(PageSize-page_offset, remaining_Data_Size);
            // calculate the the data size can be loaded this time
            // first time would be start from page_offset to the end of the page
            // last time would be the remaining data size which is less than a page size
            executable->ReadAt(
                &(kernel->machine->mainMemory[pageTable[page_number].physicalPage * PageSize + page_offset]),
                Data_to_Load_Size, inFileAddr);
                // load data from the inFileAddrr(which is from the disk) to the mainMemory
                // and copy the Data_to_Load_Size from the inFileAddr to mainMemory
                // the physical address in the mainMemory would be 
                // the the the index of physical Page * PageSize + page_offset
            inFileAddr += Data_to_Load_Size;
            // change the start point from the inFileAddr
            remaining_Data_Size -= Data_to_Load_Size;
            // update the remaining_Data_Size
            page_offset = 0;
            // from the start of the page
            page_number++;
            // page_number+1, move to next virtual page
        }
    }
#endif

    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program using the current thread
//
//      The program is assumed to have already been loaded into
//      the address space
//
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char* fileName) 
{

    kernel->currentThread->space = this;

    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start", which
    //  is assumed to be virtual address zero
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    // Since instructions occupy four bytes each, the next instruction
    // after start will be at virtual address four.
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}


//----------------------------------------------------------------------
// AddrSpace::Translate
//  Translate the virtual address in _vaddr_ to a physical address
//  and store the physical address in _paddr_.
//  The flag _isReadWrite_ is false (0) for read-only access; true (1)
//  for read-write access.
//  Return any exceptions caused by the address translation.
//----------------------------------------------------------------------
ExceptionType
AddrSpace::Translate(unsigned int vaddr, unsigned int *paddr, int isReadWrite)
{
    TranslationEntry *pte;
    int               pfn;
    unsigned int      vpn    = vaddr / PageSize;
    unsigned int      offset = vaddr % PageSize;

    if(vpn >= numPages) {
        return AddressErrorException;
    }

    pte = &pageTable[vpn];

    if(isReadWrite && pte->readOnly) {
        return ReadOnlyException;
    }

    pfn = pte->physicalPage;

    // if the pageFrame is too big, there is something really wrong!
    // An invalid translation was loaded into the page table or TLB.
    if (pfn >= NumPhysPages) {
        DEBUG(dbgAddr, "Illegal physical page " << pfn);
        return BusErrorException;
    }

    pte->use = TRUE;          // set the use, dirty bits

    if(isReadWrite)
        pte->dirty = TRUE;

    *paddr = pfn*PageSize + offset;

    ASSERT((*paddr < MemorySize));
    
    //cerr << " -- AddrSpace::Translate(): vaddr: " << vaddr <<
    //  ", paddr: " << *paddr << "\n";

    return NoException;
}




