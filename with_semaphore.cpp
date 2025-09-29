/* *************************************************** *
 * This is a test program                              *
 *    With semaphore & shared memory                   *
 * *************************************************** */
#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <unistd.h>  

// the followings are for semaphores ----- 
#include <sys/sem.h>
#include <sys/ipc.h>

// the followings are for shared memory ----
#include <sys/shm.h>
#include <sys/ipc.h>

#define NUM_REPEAT    100000     // number of loops for testing 
#define SEM_KEY       8712       // the semaphore key
#define SHM_KEY       5512       // the shared memory key 

int main (void)
{
   pid_t  process_id;  
   int    i;                     // external loop counter  
   int    j;                     // internal loop counter  
   int    k = 0;                 // dumy integer  

   int    sem_id;                // the semaphore ID  
   struct sembuf operations[1];  // Define semaphore operations 
   int    ret_val;               // system-call return value    

   int    shm_id;                // the shared memory ID 
   int    shm_size;              // the size of the shared memoy  
   struct my_mem * p_shm;        // pointer to the attached shared memory 

   // Semaphore control data structure ----
   union semun {
        int    val;  
        struct semid_ds  *buf;  
        ushort * arry;
   } argument; 
   argument.val = 1;   // the initial value of the semaphore   

   // shared memory definition ----   
   struct my_mem {
          long int counter;
          int      parent;
          int      child;  
   };    

   // find the shared memory size in bytes ----
   shm_size = sizeof(my_mem);   
   if (shm_size <= 0)
   {  
      fprintf(stderr, "sizeof error in acquiring the shared memory size. Terminating ..\n");
      exit(0); 
   }    
   
   // create a new semaphore -----
   sem_id = semget(SEM_KEY, 1, 0666 | IPC_CREAT); 
   if (sem_id < 0)
   {
      fprintf(stderr, "Failed to create a new semaphire. Terminating ..\n"); 
      exit(0);
   }

   // initialzie the new semapahore by 1 (zero) ----
   if (semctl(sem_id, 0, SETVAL, argument) < 0)
   {
      fprintf(stderr, "Failed to initialize the semapgore by 1. Terminating ..\n"); 
      exit(0);  
   }

   // create a shared memory ----
   shm_id = shmget(SHM_KEY, shm_size, 0666 | IPC_CREAT);         
   if (shm_id < 0) 
   {
      fprintf(stderr, "Failed to create the shared memory. Terminating ..\n");  
      exit(0);  
   } 

   // attach the new shared memory ----
   p_shm = (struct my_mem *)shmat(shm_id, NULL, 0);     
   if (p_shm == (struct my_mem*) -1)
   {
      fprintf(stderr, "Failed to attach the shared memory.  Terminating ..\n"); 
      exit(0);   
   }   

   // initialize the shared memory ----
   p_shm->counter  = 0;  
   p_shm->parent   = 0;   
   p_shm->child    = 0;
 
   // spawn a child process ----
   process_id = fork();

   // The child process ----------------------------- //
   if (process_id == 0)
   {
        printf("I am the child process ...\n");

        // initialize child process ----    
        p_shm->child = 1; // child started .. 
        
        // wait for the parent to get ready ----
        while (p_shm->parent != 1)     
        { k = k + 1; }
        k = 0;  
 
        for (i = 0; i < NUM_REPEAT; i++)  
        { 
            // WAIT on the semaphore ----
            operations[0].sem_num = 0;
            operations[0].sem_op = -1; // "wait" on the semaphore   
            operations[0].sem_flg = 0;  
            ret_val = semop(sem_id, operations, 1);         
            if (ret_val != 0)  
            {   
                fprintf(stderr, "P-OP(wait) failed (child) ....\a\n");   
            }

            // subtract 1 from the shared memory ----
            p_shm->counter = p_shm->counter - 1;  
            printf("C: %d\n", p_shm->counter);   

            for (j = 0; j < 10000; j++)
            {  k = k + 1; }
        
            // SIGNAL on the semaphore ----
            operations[0].sem_num = 0;  
            operations[0].sem_op  = 1;  
            operations[0].sem_flg = 0;  
            ret_val = semop(sem_id, operations, 1);   
            if (ret_val != 0)
            {
                fprintf(stderr, "V-OP (wait) failed (child) ....\a\n");    
            }  
        }

        // declare completion ----
        p_shm->child = 0;  

        // wait for the parent to complete ---
        k = 0;
        while (p_shm->parent != 0)
        {  k = k + 1; }
 
        exit(0); 
   }

   // The parent process ----------------------------- //   
   else 
   {     
        printf("I am the parent process ...\n");  

        // initialize the parent process ----
        p_shm->parent = 1;  // the parent process started ..

        // wait for the child pocess to get ready ----
        while (p_shm->child != 1)
        {  k = k + 1; }
        k = 0;   
        
        for (i = 0; i < NUM_REPEAT; i++)   
        {
            // WAIT on the semaphore -----
            operations[0].sem_num = 0;  // the first semapahore
            operations[0].sem_op  = -1; // "wait" on the semaphore
            operations[0].sem_flg = 0;  // make sure to block    
            ret_val = semop(sem_id, operations, 1); 
            if (ret_val != 0)
            {
                 fprintf(stderr, "P-OP (wait) failed ....\a\n"); 
            } 
 
            // THE CRITICAL SECTION ============= // 
            p_shm->counter = p_shm->counter + 1;  
            printf("P: %d\n", p_shm->counter);  

            for (j = 0; j < 10000; j++)
            {  k = k + 1; }

            // SIGNAL on the semapahore ----
            operations[0].sem_num = 0;  
            operations[0].sem_op  = 1; // SIGNAL   
            operations[0].sem_flg = 0;
            ret_val = semop(sem_id, operations, 1);   
            if (ret_val != 0)  
            {
                fprintf(stderr, "V-OP (signal) failed ....\a\n");  
            }
        }

        // declare completion ----
        p_shm->parent = 0;  

        // wait for the child process to complete ---- 
        k = 0;
        while (p_shm->child != 0)  
        {  k = k + 1;  }   

        printf("Shared Memory Counter: %d\n", p_shm->counter);  

        // detach the shared memory ---
        ret_val = shmdt(p_shm);  
        if (ret_val != 0) 
        {  printf ("shared memory detach failed ....\n"); }

        ret_val = shmctl(shm_id, IPC_RMID, 0); 
        if (ret_val != 0)
        {  printf("shared memory ID remove ID failed ... \n"); } 

        ret_val = semctl(sem_id, IPC_RMID, 0);  
        if (ret_val != 0)
        {  printf("semaphore remove ID failed ... \n"); }
 
        exit(0);  
   }
}