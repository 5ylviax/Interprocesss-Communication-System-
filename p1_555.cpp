// Header comment:
// Silvia Saavedra
// ssaaved@siue.edu
// 800-726-555
// CS 340-001
// Date 4/04/2025
// Project #1 
#include <cstdio>

#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <ctime>
#include <sys/shm.h>
#include <sys/wait.h>


// the followings are for shared memory ----
#include <sys/ipc.h>
#include <sys/shm.h> 

// Required Const Labels
#define ONE_SECOND     1000    //    1 second
#define MSG_KEY           73157319     // (unique) message queue key

#define NUM_REPEATS        200    // number of loops for high-priority processes
#define NUM_CHILD            4   // number of the child processes

#define BUFFER_SIZE       1024    // max. message queue size

#define THREE_SECONDS  3    //    3 seconds  

#define NUM_REPEAT    80000      // number of loops for testing 
#define SHM_KEY       73157319       // the shared memory key 

// function prototype ----------------------------------------------
void millisleep(unsigned milli_seconds);     // for random sleep time


// definition of message -------------------------------------------
struct message{
         long mtype;
         unsigned int mnum;
};


// shared memory definition ---- 
     struct my_mem {
	  unsigned int Go_Flag;
          unsigned int Done_Flag[NUM_CHILD];
          int Individual_Sum[NUM_CHILD];
    };

// function prototype ----------------------------------------------
unsigned int uniform_rand(void);  // a random number generator


int main (void)
{
   pid_t  process_id;  
   int    i;                     // external loop counter  
   int    j;                     // internal loop counter  
   int    k = 0;                 // dumy integer  

   int    ret_val;               // system-call return value    
   int    sem_id;                // 
   int    shm_id;                // the shared memory ID 
   int    shm_size;              // the size of the shared memoy  
   struct my_mem * p_shm;        // pointer to the attached shared memory
 
   int     msqid_01;      // message queue ID (#1)
   key_t  msgkey_01;      // message-queue key (#1)
   struct message buf_01;
   struct message buf_02;
   struct message buf_03;
   struct message buf_04;

   msgkey_01 = MSG_KEY;     // the messge-que ID key
  
   // find the shared memory size in bytes ----
   shm_size = sizeof(my_mem);   
   if (shm_size <= 0)
   {  
      fprintf(stderr, "sizeof error in acquiring the shared memory size. Terminating ..\n");
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
   p_shm = (struct my_mem*) shmat(shm_id, NULL, 0);     
   if (p_shm == (void*) -1)
   {
      fprintf(stderr, "Failed to attach the shared memory.  Terminating ..\n"); 
      exit(0);   
   }

   p_shm->Go_Flag = 1;
   for(i = 0; i < NUM_CHILD; i++)
   {
    p_shm->Done_Flag[i] = 0;
    p_shm->Individual_Sum[i] = 0;
   }
   printf("========== THE MASTER PROCESS STARTS ========\n");
   // create a new message queue -----------------------------------
   msqid_01 = msgget(msgkey_01, 0666 | IPC_CREAT);
   if (msqid_01 == -1) {
      perror("msgget");
      exit(1);
   }
   // Loop from 0 to 3, calling process() on each iteration 
   for(i = 0; i < NUM_CHILD; i++)
   {
     process_id = fork(); // create a child process. Now we have 2 processes - the parent process and the child process
     // Unable to create child process
     if (process_id < 0) 
     {
      perror("fork error\n");
      exit(1);
     }
     //Child process
     if(process_id == 0)
     {
	while(p_shm->Go_Flag == 0){}; // wait for master to start
	      if(i == 0){ process_C1(p_shm, msqid_01);}
	else if(i == 1){ process_C2(p_shm, msqid_01);}
	else if(i == 2) { process_P1(p_shm, msqid_01);}
	else if(i == 3) {process_P2(p_shm, msqid_01);}
     }
   }
   
   p_shm->Go_Flag = 1;
   printf("---------------------- the master process waits for children to terminate ... \n\n");

// step master process wasit ( by a "spin- wait")
while ((p_shm->Done_Flag[0] == 0) || (p_shm->Done_Flag[1] == 0) || (p_shm->Done_Flag[2] == 0) || (p_shm->Done_Flag[3] == 0))
    {
      // Just wait here until all Done_Flag's are set to 1
    }
	// extra 
     printf("Master Process Report ***********************************************\n");
     // Print the Individual sum of each process
     for (int i = 0; i < 4; i++) {
        printf("         C%d Checksum: %d\n", i + 1, p_shm->Individual_Sum[i]);
     }    
    
    printf("         SEND-CHECKSUM: %d\n", p_shm->Individual_Sum[2] + p_shm->Individual_Sum[3]);
    printf("         RECEIVED-CHECKSUM: %d\n" ,  p_shm->Individual_Sum[0] + p_shm->Individual_Sum[1]);
    printf("------------------------ the master process is terminating...");


   // step 9 
   //Detach the process from the already attached shared memory
    if(shmdt(p_shm) != 0)
   {
	perror("shmdt error");
   }
   // IPC_RMID--> marks the segment to be destroyed. The segment is destroyed only after the last process has detached it
   if(shmctl(shm_id, IPC_RMID, NULL) < 0)
   {
	perror("shmctl error");
   }
   // Remove the message queue to free system resources
   if(msgctl(msqid_01, IPC_RMID, NULL) < 0 ) 
   {
	perror("msgctl error");
   }
  }

/* Process C1 ============================================================= */
void process_C1(struct my_mem *p_shm, int msqid)
{
   int          i;            // the loop counter
   int          status;       // result status code // ?? what is use for ?            
   unsigned int my_rand;      // a randon number
   unsigned int checksum = 0; // the local checksum
   struct message buf_01;

   // REQUIRED output #1 -------------------------------------------
   // NOTE: C1 can not make any output before this output
   printf("    Child Process #1 is created ....\n");
   printf("    I am the first consumer ....\n\n");
   // REQUIRED: shuffle the seed for random generator --------------
   srand(time(0));
   while(p_shm->Go_Flag == 0){} // some how this fixed it  

  for(i = 0; i < NUM_REPEATS; ++i)
  {
	// msgrcv --> used for communicating with different messages types
	if(msgrcv(msqid, (struct msgbuf *)&buf_01, sizeof(buf_01.mnum), 1, 0) < 0)
        {
		perror("msgrcv error");
	}
        else 
        {

	checksum += buf_01.mnum;
        }
  }
 p_shm->Individual_Sum[0] = checksum;
	

// REQUIRED 3 second wait ---------------------------------------
   millisleep (THREE_SECONDS);

   // REQUIRED output #2 -------------------------------------------
   // NOTE: after the following output, C1 can not make any output
   printf("    Child Process #1 is terminating (checksum: %d) ....\n\n", checksum);
   // step 8 
   // raise my "Done_Flag" -----------------------------------------
   p_shm->Done_Flag[0] = 1;  // I m done!
   _Exit(3); // would return the control back to the kernel immediately
}
// Child #2 (Consumer )
void process_C2(struct my_mem *p_shm, int msqid)
{
   int          i;            // the loop counter
   int          status;       // result status code           
   unsigned int my_rand;      // a randon number
   unsigned int checksum = 0; // the local checksum
   struct message buf_02;


   // REQUIRED output #1 -------------------------------------------
   // NOTE: C1 can not make any output before this output

   printf("    Child Process #2 is created ....\n");
   printf("    I am the second consumer ....\n\n"); 

   // REQUIRED: shuffle the seed for random generator --------------
   srand(time(0));

  for(i = 0; i < NUM_REPEATS; ++i)
  {
	// msgrcv --> used for communicating with different messages types
   if(msgrcv(msqid, &buf_02, sizeof(buf_02.mnum), 2, 0) < 0)
      {
		perror("msgrcv error");
	}
   else 
   {
	checksum += buf_02.mnum;
    }
  }
 p_shm->Individual_Sum[1] = checksum;

 // REQUIRED 3 second wait ---------------------------------------
   millisleep (THREE_SECONDS);

   // REQUIRED output #2 -------------------------------------------
   // NOTE: after the following output, C1 can not make any output
   printf("    Child Process #2 is terminating (checksum: %d) ....\n\n", checksum);
   // step 8 
   // raise my "Done_Flag" -----------------------------------------
   p_shm->Done_Flag[1] = 1;  // I m done!
   _Exit(3);
}

//Producer
// 1. Two Producer processes randomly generates a random number 
// 2. enters a random generated r
void process_P1(struct my_mem* p_shm, int msqid)
{
   int          i;            // the loop counter
   int          status;       // result status code           
       // a randon number
   unsigned int checksum = 0;     // the local checksum
   struct message buf_03; 

   // REQUIRED output #1 -------------------------------------------
   // NOTE: C1 can not make any output before this output
   printf("    Child Process #3 is created ....\n");
   printf("    I am the first producer ....\n\n");
   // REQUIRED: shuffle the seed for random generator --------------
   srand(time(0));

	
   for(i = 0; i < NUM_REPEATS; ++i)
   {
	// Generating a new random number inside the loop ensures that each iteration 
	// gets a fresh, unique value. 
	unsigned int my_rand = uniform_rand();
	
	buf_03.mtype = 1;
	buf_03.mnum = my_rand;

	if(msgsnd(msqid, (struct msgbuf *)&buf_03, sizeof(buf_03.mnum), 0) < 0) 
        {
		perror("msgsnd"); // message queues 
	}
	else 
	{
	checksum += my_rand;
	}
     } 
   p_shm->Individual_Sum[2] = checksum;   
   // REQUIRED 3 second wait ---------------------------------------
   millisleep (THREE_SECONDS);

   // REQUIRED output #2 -------------------------------------------
   // NOTE: after the following output, C1 can not make any output
   printf("    Child Process #3 is terminating (checksum: %d) ....\n\n", checksum);
   // step 8
   // raise my "Done_Flag" -----------------------------------------
   p_shm->Done_Flag[2] = 1;  // I m done!
   _Exit(3);
}
void process_P2(struct my_mem* p_shm, int msqid)
{
   int          i;            // the loop counter
   int          status;       // result status code           
   unsigned int my_rand;      // a randon number
   unsigned int checksum = 0;     // the local checksum
   struct message buf_04;

   // REQUIRED output #1 -------------------------------------------
   // NOTE: C1 can not make any output before this output
   printf("    Child Process #4 is created ....\n");
   printf("    I am the second producer ....\n\n");
   // REQUIRED: shuffle the seed for random generator --------------
   srand(time(0) + getpid());

   for(i = 0; i < NUM_REPEATS; ++i)
   {
	unsigned int my_rand= uniform_rand();

	
	buf_04.mtype = 2;
	buf_04.mnum = my_rand;

	if(msgsnd(msqid, (struct msgbuf *)&buf_04, sizeof(buf_04.mnum), 0) < 0) 
        {
		perror("msgsnd"); // message queues 
	}
	else 
	{
		checksum += my_rand;
	}
   } 
   p_shm->Individual_Sum[3] = checksum;
   
   // REQUIRED 3 second wait ---------------------------------------
   millisleep (THREE_SECONDS);

   // REQUIRED output #2 -------------------------------------------
   // NOTE: after the following output, C1 can not make any output
   printf("    Child Process #4 is terminating (checksum: %d) ....\n\n", checksum);

   // raise my "Done_Flag" -----------------------------------------
   p_shm->Done_Flag[3] = 1;  // I m done!
  _Exit(3);
}



/* function "millisleep" ------------------------------------------ */
void millisleep(unsigned milli_seconds)
{ 
	usleep(milli_seconds * 1000); 
}

/* generate a random number 0 ~ 999 */
unsigned int uniform_rand(void)
{

    unsigned int my_rand;
    my_rand = rand() % 1000;
    return my_rand;
}