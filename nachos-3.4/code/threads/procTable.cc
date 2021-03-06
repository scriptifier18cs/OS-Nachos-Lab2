#include "procTable.h"
#include "system.h"

PCB *ProcTable::GetPCB(int pid)
{
    if (IsExist(pid) == false)
        return NULL;
    return pcbTable[pid];
}

// Constructor
// Cap phat bitmap va bitmapSemaphore voi toi da 1 process
ProcTable::ProcTable(int size)
{
    bitmap = new BitMap(size);
    pcbTable = new PCB *[size];
    bitmapSema = new Semaphore("BitmapSemaphore", 1); // chi 1 process dung bitmap 1 thoi diem

    for (int i = 0; i < ProcTableSize; i++)
    {
        pcbTable[i] = NULL;
    }

    bitmap->Mark(0); //Có bit tại vị trí 0

    pcbTable[0] = new PCB(0);
    currentThread->pcb = pcbTable[0];
}

// Destructor
ProcTable::~ProcTable()
{
    delete bitmap;
    delete bitmapSema;
    for (int i = 0; i < ProcTableSize; i++)
    {
        if (pcbTable[i] != NULL)
            delete pcbTable[i];
    }
    delete pcbTable;
}

// Thuc thi process moi
int ProcTable::ExecUpdate(char *filename)
{
    //ngan nap dong thoi 2 process
    bitmapSema->P();

    int curPID = currentThread->pcb->GetID();

    // ? Sai ten tep !!
    if (fileSystem->Open(filename) == NULL)
    {
        bitmapSema->V();
        return -1;
    }

    // ? Thuc thi chinh no !!
    if (strcmp(filename, currentThread->getName()) == 0)
    {
        printf("Thuc thi chinh no !!\n");
        bitmapSema->V();
        return -1;
    }

    // Tim slot trong
    int freeslot = GetFreeSlot();

    // ? Het slot !!
    if (freeslot == -1)
    {
        printf("Het slot !!\n");
        bitmapSema->V();
        return -1;
    }

    pcbTable[freeslot] = new PCB(freeslot);
    bitmap->Mark(freeslot);

    // thuc thi
    int id = pcbTable[freeslot]->Exec(filename, freeslot);
    if (id != -1)
    {
        pcbTable[curPID]->IncNumWait();
    }

    bitmapSema->V();

    return id;
}

// Tat process voi exitcode
int ProcTable::ExitUpdate(int exitcode)
{
    int curPID = currentThread->pcb->GetID();

    procTable->JoinUpdate(curPID);
    procTable->JoinReleaseUpdate(curPID);

    // ? exit Main process !!
    if (curPID == 0)
    {
        interrupt->Halt(); // tat may!!
        return exitcode;
    }

    pcbTable[curPID]->SetExitCode(exitcode);

    pcbTable[curPID]->JoinRelease(); // bao process k cho join
    Remove(curPID);                  // xoa pcb khoi pcbTable

    return exitcode;
}

// Process hien tai joinwait process voi pid
int ProcTable::JoinUpdate(int pid)
{
    // ? pid khong hop le !!
    if (IsExist(pid) == false)
        return -1;

    pcbTable[pid]->JoinWait();

    int ec = pcbTable[pid]->GetExitCode();

    // trả về exit code
    return ec;
}

int ProcTable::JoinReleaseUpdate(int pid)
{
    // ? pid khong hop le !!
    if (IsExist(pid) == false)
        return -1;

    // ? Chua tat process con
    if (pcbTable[pid]->GetNumWait() != 0)
    {
        return -1;
    }

    // ? exit Main process !!
    if (pid == 0)
    {
        interrupt->Halt(); // tat may!!
        return -1;
    }

    int exitcode = pcbTable[pid]->GetExitCode();

    pcbTable[pid]->JoinRelease(); // bao process cha k cho join

    Remove(pid); // xoa pcb khoi pcbTable

    return exitcode;
}

// Tim cho trong
int ProcTable::GetFreeSlot()
{
    return bitmap->Find();
}

// ? pID co ton tai trong pcbTable
bool ProcTable::IsExist(int pid)
{
    if (pid < 0 || pid >= 10)
        return false;

    return bitmap->Test(pid);
}

// Ket thuc process va xoa bo nho cua pcb do
void ProcTable::Remove(int pid)
{
    if (IsExist(pid) == false)
        return;

    bitmap->Clear(pid);
    if (pcbTable[pid] != NULL)
        delete pcbTable[pid];
    pcbTable[pid] = NULL;
}

// Ten process voi pid
char *ProcTable::GetProcessName(int pid)
{
    if (IsExist(pid) == false)
        return NULL;

    return pcbTable[pid]->GetName();
}

