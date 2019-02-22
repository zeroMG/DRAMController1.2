/* 
 * File:   ERAsimStub.h
 * Author: saud
 *
 * Created on September 19, 2016, 6:58 AM
 */

#ifndef ERAsimStub_H
#define ERAsimStub_H

#ifdef  __cplusplus
extern "C" {
#endif
  
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

typedef struct{
  char* BufferID; /* This is the file path to the shared segment*/
  int BufferSize;
  int QueueSize;
  int ItemSize;
  int NO_Items;
  char WriteMode;
  
  char* MEM;
  
  char Write_Mode;
  int* Write_ptr;
  int* Read_ptr;
  int* Count;
  
  int fd;
} ERAsim_Queue_t;

typedef struct{
  int fd;
  char* MEM;
  int64_t* clock;
  int* SimulationFinished;
}ERAsim_GlobalState_t;

typedef struct {
   int Address;
   int type;
   int Size;
   int64_t AccessTime;
   int64_t FinishTime;
   int Data;
   int Requestor_Id;
   int ID;
 }ERAsim_Transaction_t;

 
typedef struct
{
  int64_t* clock;
  int* SimulationFinished;
  ERAsim_Queue_t* Requests;
  int Requests_fd;
  ERAsim_Queue_t* Responses;
  int Responses_fd;
  
  void (*wait)(int);
  int (*count)(void);
  ERAsim_Transaction_t* (*get)(void);
  void (*put)(ERAsim_Transaction_t*);
 } ERAsim_Stub_t;  

 
#define ERAsim_Read 0
#define ERAsim_Write 1

#define ERAsim_Finished *ERAsim.SimulationFinished
#define ERAsim_Clock *ERAsim.clock
 
#define F_ULOCK 0	/* Unlock a previously locked region.  */
#define F_LOCK  1	/* Lock a region for exclusive use.  */ 
  
 
static ERAsim_Stub_t ERAsim;

static void ERAsim_printTransaction(ERAsim_Transaction_t* TA)
{
  //printf("Transaction @0x%x:\n", (unsigned int)TA);
  printf("Transaction:\n");
  printf("\tAddress: %d\n", TA->Address);
  printf("\ttype: %d\n", TA->type);
  printf("\tSize: %d\n", TA->Size);
  printf("\tAccessTime: %lld\n", TA->AccessTime);
  printf("\tFinishTime: %lld\n", TA->FinishTime);
  printf("\tData: %d\n", TA->Data);
  printf("\tRequestor_Id: %d\n", TA->Requestor_Id);
  printf("\tID: %d\n", TA->ID);
}

static char* ERAsim_Transaction2str(const ERAsim_Transaction_t* TA)
{
  char* retV = (char*)malloc(ERAsim.Requests->ItemSize);
  *retV = '\0';
  
  int x = 0;
  x += sprintf(retV+x, "%d\n", TA->Address);
  x += sprintf(retV+x, "%d\n", TA->type);
  x += sprintf(retV+x, "%d\n", TA->Size);
  x += sprintf(retV+x, "%lld\n", TA->AccessTime);
  x += sprintf(retV+x, "%lld\n", TA->FinishTime);
  x += sprintf(retV+x, "%d\n", TA->Data);
  x += sprintf(retV+x, "%d\n", TA->Requestor_Id);
  x += sprintf(retV+x, "%d\n%s", TA->ID, "\0");
//  printf("Total string = %d\n", x);
//  printf("message = \n%s", retV);
  
  return retV;
   
}

static ERAsim_Transaction_t* ERAsim_str2Transaction(char* strV)
{  
  ERAsim_Transaction_t* TA = (ERAsim_Transaction_t*)malloc(sizeof(ERAsim_Transaction_t));    
  char* pch;
  pch = strtok (strV,"\n");
  TA->Address = atoi(pch);  
  
  pch = strtok (NULL,"\n");
  TA->type = atoi(pch);  
  
  pch = strtok (NULL,"\n");
  TA->Size = atoi(pch);  
  
  pch = strtok (NULL,"\n");
  TA->AccessTime = atol(pch);  
  
  pch = strtok (NULL,"\n");
  TA->FinishTime = atol(pch);  
  
  pch = strtok (NULL,"\n");
  TA->Data = atoi(pch);  
  
  pch = strtok (NULL,"\n");
  TA->Requestor_Id = atoi(pch);  
  
  pch = strtok (NULL,"\n");
  TA->ID = atoi(pch);  

  //  printf("message = \n%s", retV);
  
  return TA;  
}

static int SizeAlignment(int Size)
{
  int newSize = Size;
  while ((newSize & 3) != 0)
  {
      newSize +=1;
  }

  return newSize;
}
  
static ERAsim_GlobalState_t* ERAsim_init_GlobalState()
{
  ERAsim_GlobalState_t* GState = (ERAsim_GlobalState_t*)malloc(sizeof(ERAsim_GlobalState_t));
  
  char const *BufferID = "/tmp/ERAsim_GlobalShared_State";
  int BufferSize = 12;
  
  int fd = open(BufferID, O_RDWR);  
  if (fd == -1) {
    perror("CANNOT OPEN FILE???");
    perror("Error opening shared Buffer");
    exit(EXIT_FAILURE);
  }
    
  //GState->MEM = mmap(0, BufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  GState->MEM = static_cast<char*>(mmap(0, BufferSize, PROT_READ, MAP_SHARED, fd, 0)); //Read-only access
  if (GState->MEM == MAP_FAILED) {
    close(fd);
    perror("Error in mmapping the file");
    exit(EXIT_FAILURE);
  
  }    

  GState->fd = fd;

  GState->clock = (int64_t*)&(GState->MEM[0]);
  GState->SimulationFinished = (int*)&(GState->MEM[8]);
  return GState;
}

static ERAsim_Queue_t* ERAsim_init_Queue(const char* BufferID, int ItemSize, int NO_Items, char Mode, int* ret_fd){
  ERAsim_Queue_t* Queue = (ERAsim_Queue_t*)malloc(sizeof(ERAsim_Queue_t));
  Queue->BufferID = (char*)BufferID;
  Queue->QueueSize = ItemSize * NO_Items;
  Queue->Write_Mode = (tolower(Mode)=='w');
  Queue->BufferSize = SizeAlignment(ItemSize * NO_Items + 12);  // handle the extra metadata for write and read ptrs and count
  
  Queue->ItemSize = ItemSize;
  Queue->NO_Items = NO_Items;
  Queue->WriteMode = (tolower(Mode)=='w');
  
  int fd = 0;

  if (tolower(Mode)=='w')
  {
    fd = open(BufferID, O_CREAT | O_TRUNC | O_RDWR, 0600);
    if (fd == -1) {
      perror("Error opening shared Buffer");
      exit(EXIT_FAILURE);
    }
      char* tmpBuffer = (char*)calloc(Queue->BufferSize, sizeof(char));      
      int wretV = write(fd, tmpBuffer, Queue->BufferSize);
      free(tmpBuffer);

    Queue->MEM = (char*)mmap(0, Queue->BufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (Queue->MEM == MAP_FAILED) {
      close(fd);
      perror("Error in mmapping the file");
      exit(EXIT_FAILURE);
    }    
  }          
    
  else // Read Only
  {
    fd = open(BufferID, O_RDWR);
    if (fd == -1) {
      perror("Error opening shared Buffer");
      exit(EXIT_FAILURE);
    }
        
    
    Queue->MEM = (char*)mmap(0, Queue->BufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (Queue->MEM == MAP_FAILED) {
      close(fd);
      perror("Error in mmapping the file");
      exit(EXIT_FAILURE);
    }    
  }
  
  *ret_fd = fd;
  Queue->fd = fd;
  
  int eIndex = Queue->QueueSize;
  Queue->Write_ptr = (int*)&(Queue->MEM[eIndex]);
  Queue->Read_ptr = (int*)&(Queue->MEM[eIndex + 4]);
  Queue->Count = (int*)&(Queue->MEM[eIndex + 8]);
  
  
  if (tolower(Mode)=='w')
  {
    *(Queue->Write_ptr) = 0;
    *Queue->Read_ptr = 0;
    *Queue->Count = 0;   
  }


          
  return Queue;
}


static char* ERAsim_Queue_get(ERAsim_Queue_t* QObj)
{
  
  if (*QObj->Count == 0)
  {
//    printf("The Quesue is empty, cannot read");
    return (0);            
  }

  int sIndex = *QObj->Read_ptr;
  int eIndex = sIndex + QObj->ItemSize;
  char* str_Item =(char*)malloc(QObj->ItemSize);
  memcpy(str_Item, (char*)&QObj->MEM[*QObj->Read_ptr], QObj->ItemSize);
  *QObj->Read_ptr =  (*QObj->Read_ptr + QObj->ItemSize)% QObj->QueueSize;
  int lretV = lockf(QObj->fd,F_LOCK, (long int)NULL);
  *QObj->Count -= 1;
  lretV = lockf(QObj->fd,F_ULOCK, (long int)NULL);
  
  return str_Item;
}

static void ERAsim_Queue_put(ERAsim_Queue_t* QObj, const char* str_Item)
{
    int N = strlen(str_Item)+1; // to include the '\n' byte
    if (!QObj->WriteMode){
      printf("This is read-only queue, you can not write to it\n");
      return;
    }
      
    if(N > QObj->ItemSize){
      printf("The Item size exceeds the size limit allowed in this queue (%d)->(%d)",N,QObj->ItemSize);
      return;
    }

    if(*QObj->Count >= QObj->NO_Items) // is_full
    {
      //printf("The Queue is full, cannot write");
      return;
    }
    char* new_str_Item;  
    int K = 0;
    if(N < QObj->ItemSize){
      K = QObj->ItemSize - N;
      new_str_Item = (char*)malloc(N+K);      
    }else
    {
      new_str_Item = (char*)malloc(N);      
    }
    memcpy(new_str_Item, str_Item, N);
    
    N = N+K;

    if(N != QObj->ItemSize){
      printf("Error: item size do not match");
    }
     

    int sIndex = *QObj->Write_ptr;
    memcpy(&QObj->MEM[sIndex], new_str_Item, N);   
    *QObj->Write_ptr = (*QObj->Write_ptr + QObj->ItemSize)% QObj->QueueSize;
    
    
    int lretV = lockf(QObj->fd,F_LOCK, (long int)NULL);
    *QObj->Count += 1;
    lretV = lockf(QObj->fd,F_ULOCK, (long int)NULL);

    free(new_str_Item);
}

//void possible_free(void* ptr)
//{
//  if(ptr)
//  {
//    free(ptr);
//  }
//}


static char* concat(char* dest,  const char* src1, const char* src2)
{
  int size1 = strlen(src1);
  int size2 = strlen(src2);
  int size = size1 + size2;
  
  char* buff = (char*) malloc(size+1);
  buff[size] = '\0';
  
  memcpy(buff, src1 ,size1);
  memcpy(buff + size1, src2 ,size2);
  
//  possible_free(dest);
  dest = buff;
  return buff;  
}


static void ERAsim_wait(int myClock)
{
  while(myClock>ERAsim_Clock && !ERAsim.SimulationFinished);
}

static ERAsim_Transaction_t* ERAsim_get(void)
{
    
  char* str_Item = ERAsim_Queue_get(ERAsim.Requests);
  if(!str_Item)
  {    
    return 0;
  }
  
  ERAsim_Transaction_t* TA = ERAsim_str2Transaction(str_Item);
  free(str_Item);
  
  return TA;
}

static void ERAsim_put(ERAsim_Transaction_t* TA)
{
  char* str_Item = ERAsim_Transaction2str(TA);  
  ERAsim_Queue_put(ERAsim.Responses, str_Item);
  
  free(TA);
  free(str_Item);
}

static int ERAsim_count(void)
{
  return *ERAsim.Requests->Count;
}

static ERAsim_Stub_t ERAsim_init_Stub(const char* PIPE_Name, int QueueSize, int ItemSize)
{  
  
  char* RequestsBufferID = NULL;
  char* ResponsesBufferID = NULL;
  
  RequestsBufferID = concat(RequestsBufferID, PIPE_Name, "_Requests");
  ResponsesBufferID = concat(ResponsesBufferID, PIPE_Name, "_Responses");
  
//  ERAsim.Requests  = ERAsim_init_Queue(BufferID, ItemSize, NO_Items, 'R');
  printf("initiating Response Queue\n");
  int ret_fd = 0;
  ERAsim.Responses = ERAsim_init_Queue(ResponsesBufferID, ItemSize, QueueSize, 'W', &ret_fd);
  ERAsim.Responses_fd = ret_fd;
  
  printf("initiating Requests Queue\n");
  ERAsim.Requests  = ERAsim_init_Queue(RequestsBufferID, ItemSize, QueueSize, 'R', &ret_fd);
  ERAsim.Requests_fd= ret_fd;

  printf("initiating ERAsim Global state\n");
  ERAsim_GlobalState_t* Gstate = ERAsim_init_GlobalState();
  ERAsim.clock = Gstate->clock;
  ERAsim.SimulationFinished = Gstate->SimulationFinished;
  
  
  ERAsim.wait = ERAsim_wait;
  ERAsim.get = ERAsim_get;
  ERAsim.put = ERAsim_put;
  ERAsim.count = ERAsim_count;

  free(RequestsBufferID);
  free(ResponsesBufferID);
  free(Gstate);
  
  return ERAsim;
}


#ifdef  __cplusplus
}
#endif

#endif  /* ERAsimStub_H */

