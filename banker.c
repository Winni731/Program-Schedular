/**
 *  banker.c
 *
 *  Full Name: Ying Zhang
 *  EECS3221 Section: M 
 *  Description of the program: the program takes an input file and schedule the process by using FIFO algorithm
 * 								and banker's algorithm to simulate deadlock prevention. When it executed, input task files will generate output for FIFO and banker's 
 *                manager results for each tasks.
 */

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <math.h>

#define MAX_TASK 100 // max numbers of tasks

#define MAX_ACT 1000 // max numbers of activities

#define MAX_RES 100 // max numbers of resource type

typedef struct activity_struct {
  char * title; // activity name 
  int pid; // task id
  int res_type; // resource type 
  int res_num; // resource number
}
activity;

typedef struct task { // representation of a task 
  activity * acts[MAX_ACT]; // an array of activities
  int state; // state of the task: <0 initial>, <1 run>, <2 wait>, <10 abort>, <999 terminate>
  int resource_hold[MAX_RES]; // resource holded by task
  int run; // time is running
  int wait; // time is waiting
  int compute; // compute time tracker
  int claim[MAX_RES]; // resource claim
}
Task;

//helper method adds a task to a block queue 
void add_block_q(int * block_q, int index) {
  int size = 0;

  while (1) {
    if (block_q[size] == -1) {
      break;
    } else {
      size = size + 1;
    }
  }

  block_q[size] = index;
}

//helper method remove a task from a blocked queue
void remove_block_q(int * block_q, int index) {
  int item_index; // index of the item to be removed
  int size = 0; // size of the block_q

  while (1) {
    if (block_q[size] == -1) {
      break;
    } else {
      if (block_q[size] == index) {
        item_index = size;
      }
      size = size + 1;
    }
  }
  for (int i = item_index; i < size - 1; i++) {
    block_q[i] = block_q[i + 1];
  }

  block_q[size - 1] = -1;
}

//helper method remove a task from a blocked queue
void remove_check_q(int * check_q, int index) {
  int item_index; // index of the item to be removed
  int size = 0; // size of the check_q

  while (1) {
    if (check_q[size] == -1) {
      break;
    } else {
      if (check_q[size] == index) {
        item_index = size;
      }
      size = size + 1;
    }
  }

  for (int i = 0; i < size; i++) {
    if (check_q[i] == index) {
      item_index = i;
    }
  }

  for (int i = item_index; i < size - 1; i++) {
    check_q[i] = check_q[i + 1];
  }

  check_q[size - 1] = -1;
}

//helper method to determine if block_q contains aTask
int contains(int * block_q, int pid) {
  int result = 0; // bool indicate if contains or not
  int size = 0;

  while (1) {
    if (block_q[size] == -1) {
      break;
    } else {
      size = size + 1;
    }
  }
  for (int i = 0; i < size; i++) {
    if (block_q[i] == pid) {
      result = 1;
    }
  }
  return result;
}

//helper method to shif the task array right several cycles
Task block(Task aTask, int current, int right) {
  Task result;
  int size = 0;

  while (1) {
    if (strcmp(aTask.acts[size] -> title, "terminate") == 0) {
      size = size + 1;
      break;
    } else {
      size = size + 1;
    }
  }

  for (int i = 0; i < (size + right); i++) {
    result.acts[i] = (activity * ) malloc(sizeof(activity * ));
    result.acts[i] -> title = (char * ) malloc(sizeof(char * ));
    if (i < current) { // copy to the current index
      strcpy(result.acts[i] -> title, aTask.acts[i] -> title);
      result.acts[i] -> pid = aTask.acts[i] -> pid;
      result.acts[i] -> res_type = aTask.acts[i] -> res_type;
      result.acts[i] -> res_num = aTask.acts[i] -> res_num;
    } else if (i >= current && i <= (current + right)) { // copy to the right several cycles
      strcpy(result.acts[i] -> title, aTask.acts[current] -> title);
      result.acts[i] -> pid = aTask.acts[current] -> pid;
      result.acts[i] -> res_type = aTask.acts[current] -> res_type;
      result.acts[i] -> res_num = aTask.acts[current] -> res_num;
    } else { // copy the rest 
      strcpy(result.acts[i] -> title, aTask.acts[i - right] -> title);
      result.acts[i] -> pid = aTask.acts[i - right] -> pid;
      result.acts[i] -> res_type = aTask.acts[i - right] -> res_type;
      result.acts[i] -> res_num = aTask.acts[i - right] -> res_num;
    }
  }
  result.state = aTask.state;
  for (int i = 0; i < MAX_RES; i++) {
    result.resource_hold[i] = aTask.resource_hold[i];
    result.claim[i] = aTask.claim[i];
  }
  result.run = aTask.run;
  result.wait = aTask.wait;
  result.compute = aTask.compute;

  return result;
}

//helper method to determine safe sate
int safe(Task aTask, int res_type_num, int * num_resources, int request_num, int request_type) {
  int result = 0; // bool to indicate safe state
  int count = 0;

  for (int i = 1; i <= res_type_num; i++) { // safe if total resource greater than claim or 
    //   current resource and resource on hold is greater than claim
    if (num_resources[i] >= aTask.claim[i] || num_resources[i] + aTask.resource_hold[i] >= aTask.claim[i]) {
      count = count + 1;
    }
  }
  if ((count == res_type_num) && (request_num + aTask.resource_hold[request_type]) <= aTask.claim[request_type]) { // request and resource on hold must less than claim
    result = 1;
  } else {
    result = 0;
  }

  return result;
}

// fifo manager grants tasks in first come first serve mannar, deadlock may occur
Task * fifo_manager(int num_of_tasks, int * num_activities, int resource_type_num, int * num_resources, Task * queue) {
  int num_check = num_of_tasks; // number of tasks in the check_q
  int time = 0; // time cycle
  int num_wait = 0; // number of tasks waiting for resource
  int take_time = 0; // increase time bool
  int block_q[MAX_TASK]; // block task queue
  int de_check = 0; // number to decrease the num_check
  int check_q[MAX_TASK]; // checking task queue
  int res_temp[resource_type_num]; // resource on hold array

  for (int i = 1; i <= resource_type_num; i++) { // initialize resource on hold array
    res_temp[i] = 0;
  }

  for (int i = 0; i < MAX_TASK; i++) { // initialize block queue
    check_q[i] = -1;
  }

  Task * checker = (Task * ) malloc(num_of_tasks * sizeof(Task));
  for (int i = 0; i < num_of_tasks; i++) { // read input file data to queue
    for (int j = 0; j < num_activities[i]; j++) {
      checker[i].acts[j] = (activity * ) malloc(sizeof(activity * ));
      checker[i].acts[j] -> title = (char * ) malloc(sizeof(char * ));
      strcpy(checker[i].acts[j] -> title, queue[i].acts[j] -> title);
      checker[i].acts[j] -> pid = queue[i].acts[j] -> pid;
      checker[i].acts[j] -> res_type = queue[i].acts[j] -> res_type;
      checker[i].acts[j] -> res_num = queue[i].acts[j] -> res_num;
    }
    checker[i].state = queue[i].state;
    checker[i].run = queue[i].run;
    checker[i].wait = queue[i].wait;
    checker[i].compute = queue[i].compute;
    for (int k = 1; k <= resource_type_num; k++) { // initialize holding resources to 0
      checker[i].resource_hold[k] = 0; // resource on hold for task 
      check_q[i] = checker[i].acts[0] -> pid; // add task to checking queue 
    }
  }

  for (int i = 0; i < MAX_TASK; i++) { // initialize block queue
    block_q[i] = -1;
  }
  for (int j = 0; j < MAX_ACT; j++) {
    if (num_check == 0) { // break when check_q is empty 
      break;
    } else { // start checking
      for (int i = 1; i <= resource_type_num; i++) { // total resource available from beginning cycle
        num_resources[i] = num_resources[i] + res_temp[i];
        res_temp[i] = 0;
      }

      if (num_wait != 0) { // there's task waiting, check block_q first 
        if (num_wait == num_check) { // all the tasks are waiting
          if (num_wait == 1) { // only one task is waiting 
            if (checker[block_q[0] - 1].acts[j] -> res_num <= num_resources[checker[block_q[0] - 1].acts[j] -> res_type]) { // can grant, grant the waiting task
              num_resources[checker[block_q[0] - 1].acts[j] -> res_type] = num_resources[checker[block_q[0] - 1].acts[j] -> res_type] - checker[block_q[0] - 1].acts[j] -> res_num;
              checker[block_q[0] - 1].resource_hold[checker[block_q[0] - 1].acts[j] -> res_type] = checker[block_q[0] - 1].resource_hold[checker[block_q[0] - 1].acts[j] -> res_type] + checker[block_q[0] - 1].acts[j] -> res_num;
              num_wait = num_wait - 1; // waiting task decrease by 1
              take_time = 1;
              checker[block_q[0] - 1].state = 1; // tasks runs     
              checker[block_q[0] - 1].run = checker[block_q[0] - 1].run + 1; // increase run time        
            } else { // cannot grant, abort the only task 
              checker[check_q[0] - 1].state = 10; // set to abort state
              for (int m = 1; m <= resource_type_num; m++) { // resource available next cycle, sent to temp
                res_temp[m] = res_temp[m] + checker[check_q[0] - 1].resource_hold[m];
              }
              remove_block_q(block_q, check_q[0]); // remove the task from block_q
              num_wait = num_wait - 1;
              remove_check_q(check_q, check_q[0]); // remove the task from check_q
              num_check = num_check - 1;
            }
          } else { // abort as many tasks as possible to grant the next task
            de_check = 0;
            for (int i = 0; i < num_check; i++) {
              if ((res_temp[checker[check_q[0] - 1].acts[j] -> res_type] + num_resources[checker[check_q[0] - 1].acts[j] -> res_type]) >= checker[block_q[0] - 1].acts[j] -> res_num) {
                break; // break if total resource after abort grants the first task in block_q
              } else { // remove the first task (lowest ID task) from check_q 
                res_temp[checker[check_q[0] - 1].acts[j] -> res_type] = res_temp[checker[check_q[0] - 1].acts[j] -> res_type] + checker[check_q[0] - 1].resource_hold[checker[check_q[0] - 1].acts[j] -> res_type];
                remove_block_q(block_q, check_q[0]);
                num_wait = num_wait - 1;
                checker[check_q[0] - 1].state = 10; // set to abort state
                remove_check_q(check_q, check_q[0]);
                de_check = de_check + 1;
              }
            }
            num_check = num_check - de_check;
            de_check = 0;
          }
        } // end of num_wait == num_check
        else { // num_wait < num_check && num_wait != 0 : part of tasks are blocked  
          for (int n = 0; n < num_wait; n++) { // check tasks in block_q first
            if (checker[block_q[n] - 1].acts[j] -> res_num <= num_resources[checker[block_q[n] - 1].acts[j] -> res_type]) { // block tasks but can be granted
              num_resources[checker[block_q[n] - 1].acts[j] -> res_type] = num_resources[checker[block_q[n] - 1].acts[j] -> res_type] - checker[block_q[n] - 1].acts[j] -> res_num;
              checker[block_q[n] - 1].resource_hold[checker[block_q[n] - 1].acts[j] -> res_type] = checker[block_q[n] - 1].resource_hold[checker[block_q[n] - 1].acts[j] -> res_type] + checker[block_q[n] - 1].acts[j] -> res_num;
              take_time = 1;
              checker[block_q[n] - 1].state = 1; // tasks runs     
              checker[block_q[n] - 1].run = checker[block_q[n] - 1].run + 1;
            } else { // block tasks and cannot be granted
              checker[block_q[n] - 1] = block(checker[block_q[n] - 1], j, 1); // shift task activities right by 1 
              take_time = 1;
              checker[block_q[n] - 1].state = 2; // still in block state
              checker[block_q[n] - 1].wait = checker[block_q[n] - 1].wait + 1; // waiting time increased by 1
            }
          } // end of tasks in block_q 
          de_check = 0;
          for (int i = 0; i < num_check; i++) { // check tasks not in block_q
            if (checker[check_q[i] - 1].state != 10 && checker[check_q[i] - 1].state != 999 && !contains(block_q, checker[check_q[i] - 1].acts[j] -> pid)) {
              if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "request") == 0) { // request activity
                if (checker[check_q[i] - 1].acts[j] -> res_num <= num_resources[checker[check_q[i] - 1].acts[j] -> res_type]) { //tasks can be granted
                  num_resources[checker[check_q[i] - 1].acts[j] -> res_type] = num_resources[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
                  checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
                  take_time = 1;
                  checker[check_q[i] - 1].state = 1; // task is being granted  
                  checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1; // run time increased by 1
                } else { //tasks cannot be granted 
                  take_time = 1;
                  checker[check_q[i] - 1].state = 2; // set to block state
                  checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task activities right by 1 
                  num_wait = num_wait + 1;
                  add_block_q(block_q, checker[check_q[i] - 1].acts[0] -> pid); // add to block_q
                  checker[check_q[i] - 1].wait = checker[check_q[i] - 1].wait + 1;
                }
              } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "compute") == 0) { // everthing blocked during computing cycle
                take_time = 1;
                checker[check_q[i] - 1].state = 1; // running state
                checker[check_q[i] - 1].acts[j] -> res_type = checker[check_q[i] - 1].acts[j] -> res_type - 1;
                checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
                if (checker[check_q[i] - 1].acts[j] -> res_type != 0) {
                  checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task right by 1
                }
              } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "release") == 0) { // release activity: resource should available next cycle
                take_time = 1;
                checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
                res_temp[checker[check_q[i] - 1].acts[j] -> res_type] = res_temp[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
                checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
              } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "terminate") == 0) { // terminate activity
                checker[check_q[i] - 1].state = 999; // set terminate state
                de_check = de_check + 1; // increase de_check for num_check modification
                take_time = 1;
              }
            }
          }

          for (int m = 0; m < num_check; m++) { // modify block_q
            if (checker[check_q[m] - 1].state != 2 && contains(block_q, checker[check_q[m] - 1].acts[0] -> pid)) {
              int to_remove = checker[check_q[m] - 1].acts[0] -> pid;
              remove_block_q(block_q, to_remove);
              num_wait = num_wait - 1;
            }
          }
          for (int z = 0; z < num_of_tasks; z++) { // modify check_q
            if ((checker[z].state == 999 || checker[z].state == 10) && contains(check_q, checker[z].acts[0] -> pid)) {
              remove_check_q(check_q, checker[z].acts[0] -> pid);
            }
          }
          num_check = num_check - de_check;
          de_check = 0;
          //------------------------------------------------------------------end of blocking task check---------------
          if (num_wait == num_check) { // abort task again
            if (num_wait == 1) { // abort the only task
              checker[check_q[0] - 1].state = 10; // set abort task to abort state
              for (int m = 1; m <= resource_type_num; m++) { // released resources available next cycle
                res_temp[m] = res_temp[m] + checker[check_q[0] - 1].resource_hold[m];
              }
              remove_block_q(block_q, check_q[0]); // remove abort task from block_q
              num_wait = num_wait - 1;
              remove_check_q(check_q, check_q[0]); // remove abort task from check_q
              num_check = num_check - 1;
            } else { // abort as many tasks as possible to grant the next task
              de_check = 0;
              for (int i = 0; i < num_check; i++) { // abort task until the first task in block_q can be granted
                if ((res_temp[checker[check_q[0] - 1].acts[j] -> res_type] + num_resources[checker[check_q[0] - 1].acts[j] -> res_type]) >= checker[block_q[0] - 1].acts[j] -> res_num) {
                  break;
                }
                res_temp[checker[check_q[0] - 1].acts[j] -> res_type] = res_temp[checker[check_q[0] - 1].acts[j] -> res_type] + checker[check_q[0] - 1].resource_hold[checker[check_q[0] - 1].acts[j] -> res_type];
                remove_block_q(block_q, check_q[0]);
                num_wait = num_wait - 1;
                checker[check_q[0] - 1].state = 10;
                remove_check_q(check_q, check_q[0]);
                de_check = de_check + 1;
              }
              num_check = num_check - de_check;
              de_check = 0;
            }
          }
        }
      } // end of num_check !=0 part 
      else { //no waiting process regular check: num_wait == 0
        de_check = 0;
        for (int i = 0; i < num_check; i++) {
          if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "initiate") == 0) { // initiate activity 
            take_time = 1;
            checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            checker[check_q[i] - 1].state = 1; // running state
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "request") == 0) { // request activity
            if (checker[check_q[i] - 1].state != 2 && checker[check_q[i] - 1].acts[j] -> res_num <= num_resources[checker[check_q[i] - 1].acts[j] -> res_type]) { // can grant
              num_resources[checker[check_q[i] - 1].acts[j] -> res_type] = num_resources[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
              checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
              take_time = 1;
              checker[check_q[i] - 1].state = 1; // task is being granted 
              checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            } else { // cannot grant
              take_time = 1;
              checker[check_q[i] - 1].state = 2; // block state = 2
              num_wait = num_wait + 1; // number of waiting task increased by 1
              checker[check_q[i] - 1].wait = checker[check_q[i] - 1].wait + 1;
              add_block_q(block_q, checker[check_q[i] - 1].acts[j] -> pid);
              checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task activities right by 1 
            }
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "compute") == 0) { // everthing blocked during computing cycle
            take_time = 1;
            checker[check_q[i] - 1].state = 1;
            checker[check_q[i] - 1].acts[j] -> res_type = checker[check_q[i] - 1].acts[j] -> res_type - 1;
            checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            if (checker[check_q[i] - 1].acts[j] -> res_type != 0) {
              checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task right by 1
            }
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "release") == 0) { // resource available next cycle
            take_time = 1;
            checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
            res_temp[checker[check_q[i] - 1].acts[j] -> res_type] = res_temp[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
            checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            checker[check_q[i] - 1].state = 1;
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "terminate") == 0) {
            checker[check_q[i] - 1].state = 999; // set terminate state
            de_check = de_check + 1;
            take_time = 1;
          }
        }
        num_check = num_check - de_check;
        de_check = 0;

        if (num_wait == num_check) { // process need to abort to continue
          if (num_wait == 1) { // abort the only task
            checker[check_q[0] - 1].state = 10; // set to abort state 
            for (int m = 1; m <= resource_type_num; m++) { // released resources available next cycle
              res_temp[m] = res_temp[m] + checker[check_q[0] - 1].resource_hold[m];
            }
            remove_block_q(block_q, check_q[0]);
            num_wait = num_wait - 1;
            remove_check_q(check_q, check_q[0]);
            num_check = num_check - 1;
          } else { // abort as many tasks as possible to grant the next task
            de_check = 0;
            for (int i = 0; i < num_check; i++) { // abort tasks from check_q until the first task of block_q can be granted
              if ((res_temp[checker[check_q[0] - 1].acts[j] -> res_type] + num_resources[checker[check_q[0] - 1].acts[j] -> res_type]) >= checker[block_q[0] - 1].acts[j] -> res_num) {
                break;
              } else {
                res_temp[checker[check_q[0] - 1].acts[j] -> res_type] = res_temp[checker[check_q[0] - 1].acts[j] -> res_type] + checker[check_q[0] - 1].resource_hold[checker[check_q[0] - 1].acts[j] -> res_type];
                remove_block_q(block_q, check_q[0]);
                num_wait = num_wait - 1;
                checker[check_q[0] - 1].state = 10; // abort state
                remove_check_q(check_q, check_q[0]);
                de_check = de_check + 1;
              }
            }
            num_check = num_check - de_check;
            de_check = 0;
          }

        }
      } // end num_wait = 0 checking process
      if (take_time == 1) {
        time = time + 1;
      }
    }
  }
  return checker;
}

// banker's algorithm manager grants tasks if they are in safe state
Task * banker_manager(int num_of_tasks, int * num_activities, int resource_type_num, int * num_resources, Task * queue) {
  int num_check = num_of_tasks; // number of tasks in check_q
  int time = 0; // time cycle
  int num_wait = 0; // number of tasks waiting for resource
  int take_time = 0; // increase time bool
  int block_q[MAX_TASK]; // block task queue
  int de_check = 0; // num_check decrease counter
  int check_q[MAX_TASK]; // checking task queue
  int res_temp[resource_type_num]; // resource on hold array

  for (int i = 1; i <= resource_type_num; i++) { // initialize resource on hold array 
    res_temp[i] = 0;
  }

  for (int i = 0; i < MAX_TASK; i++) { // initialize block queue
    check_q[i] = -1;
  }

  Task * checker = (Task * ) malloc(num_of_tasks * sizeof(Task));
  for (int i = 0; i < num_of_tasks; i++) { // read input file data to queue
    for (int j = 0; j < num_activities[i]; j++) {
      checker[i].acts[j] = (activity * ) malloc(sizeof(activity * ));
      checker[i].acts[j] -> title = (char * ) malloc(sizeof(char * ));
      strcpy(checker[i].acts[j] -> title, queue[i].acts[j] -> title);
      checker[i].acts[j] -> pid = queue[i].acts[j] -> pid;
      checker[i].acts[j] -> res_type = queue[i].acts[j] -> res_type;
      checker[i].acts[j] -> res_num = queue[i].acts[j] -> res_num;
    }
    checker[i].state = queue[i].state;
    checker[i].run = queue[i].run;
    checker[i].wait = queue[i].wait;
    checker[i].compute = queue[i].compute;
    for (int k = 1; k <= resource_type_num; k++) { // initialize holding resources & claim to 0
      checker[i].resource_hold[k] = 0;
      checker[i].claim[k] = 0;
      check_q[i] = checker[i].acts[0] -> pid; // add task to checking queue 
    }
  }

  for (int i = 0; i < MAX_TASK; i++) { // initialize block queue
    block_q[i] = -1;
  }
  for (int j = 0; j < MAX_ACT; j++) {
    if (num_check == 0) {
      break;
    } else { // start checking
      for (int i = 1; i <= resource_type_num; i++) { // total resource available from beginning cycle
        num_resources[i] = num_resources[i] + res_temp[i];
        res_temp[i] = 0;
      }
      if (num_wait != 0) { // part of tasks are blocked, need to check the process in the block_q first
        for (int n = 0; n < num_wait; n++) {
          if (checker[block_q[n] - 1].acts[j] -> res_num <= num_resources[checker[block_q[n] - 1].acts[j] -> res_type] &&
            safe(checker[block_q[n] - 1], resource_type_num, num_resources, checker[block_q[n] - 1].acts[j] -> res_num, checker[block_q[n] - 1].acts[j] -> res_type)) { // block but can grant
            num_resources[checker[block_q[n] - 1].acts[j] -> res_type] = num_resources[checker[block_q[n] - 1].acts[j] -> res_type] - checker[block_q[n] - 1].acts[j] -> res_num;
            checker[block_q[n] - 1].resource_hold[checker[block_q[n] - 1].acts[j] -> res_type] = checker[block_q[n] - 1].resource_hold[checker[block_q[n] - 1].acts[j] -> res_type] + checker[block_q[n] - 1].acts[j] -> res_num;
            take_time = 1;
            checker[block_q[n] - 1].state = 1; // tasks runs     
            checker[block_q[n] - 1].run = checker[block_q[n] - 1].run + 1;
          } else { // block and cannot grant      
            checker[block_q[n] - 1] = block(checker[block_q[n] - 1], j, 1); // shift task activities right by 1 
            take_time = 1;
            checker[block_q[n] - 1].state = 2; // still in block state
            checker[block_q[n] - 1].wait = checker[block_q[n] - 1].wait + 1;
          }
        } //---------------------------------end of tasks in block_q------------------------------
        de_check = 0;
        for (int i = 0; i < num_check; i++) {
          if (checker[check_q[i] - 1].state != 10 && checker[check_q[i] - 1].state != 999 && !contains(block_q, checker[check_q[i] - 1].acts[0] -> pid)) {
            if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "request") == 0) { // request activity                 
              if (checker[check_q[i] - 1].acts[j] -> res_num > checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type] ||
                checker[check_q[i] - 1].acts[j] -> res_num + checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] > checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type]) { // request exceed claim, abord
                checker[check_q[i] - 1].state = 10;
                // remove_check_q(check_q, checker[check_q[i]-1].acts[0]->pid);
                de_check = de_check + 1;
                printf("During cycle %d-%d of Banker's algorithm\n", time, time + 1);
                printf("     Task %d's request exceeds its claim; aborted;\n", checker[check_q[i] - 1].acts[0] -> pid); // need to adjust here
                for (int m = 1; m <= resource_type_num; m++) { // free holding resource; available next cycle
                  res_temp[m] = res_temp[m] + checker[check_q[i] - 1].resource_hold[m];
                  printf("              %d units of resource[%d] available next cycle.\n", res_temp[m], m);
                }
              } else if (checker[check_q[i] - 1].acts[j] -> res_num <= num_resources[checker[check_q[i] - 1].acts[j] -> res_type] &&
                safe(checker[check_q[i] - 1], resource_type_num, num_resources, checker[check_q[i] - 1].acts[j] -> res_num, checker[check_q[i] - 1].acts[j] -> res_type)) { // can grant
                num_resources[checker[check_q[i] - 1].acts[j] -> res_type] = num_resources[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
                checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
                take_time = 1;
                checker[check_q[i] - 1].state = 1; // task is being granted  
                checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
              } else { // cannot grant
                take_time = 1;
                checker[check_q[i] - 1].state = 2;
                checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task activities right by 1 
                num_wait = num_wait + 1;
                add_block_q(block_q, checker[check_q[i] - 1].acts[0] -> pid);
                checker[check_q[i] - 1].wait = checker[check_q[i] - 1].wait + 1;
              }
            } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "compute") == 0) { // everthing blocked during computing cycle
              checker[check_q[i] - 1].acts[j] -> res_type = checker[check_q[i] - 1].acts[j] -> res_type - 1;
              checker[check_q[i] - 1].state = 1;
              checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
              if (checker[check_q[i] - 1].acts[j] -> res_type != 0) {
                take_time = 1;
                checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task right by 1
              }
            } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "release") == 0) { // release activity ---> resource should available next cycle
              take_time = 1;
              checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
              res_temp[checker[check_q[i] - 1].acts[j] -> res_type] = res_temp[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
              checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "terminate") == 0) { // terminate activity
              checker[check_q[i] - 1].state = 999;
              de_check = de_check + 1;
              take_time = 1;
            }
          }
        }

        for (int m = 0; m < num_check; m++) { // adjust block_q
          if (checker[check_q[m] - 1].state != 2 && contains(block_q, checker[check_q[m] - 1].acts[0] -> pid)) {
            int to_remove = checker[check_q[m] - 1].acts[0] -> pid;
            remove_block_q(block_q, to_remove);
            num_wait = num_wait - 1;
          }
        }
        for (int z = 0; z < num_of_tasks; z++) {  // adjust check_q
          if ((checker[z].state == 999 || checker[z].state == 10) && contains(check_q, checker[z].acts[0] -> pid)) {
            remove_check_q(check_q, checker[z].acts[0] -> pid);
          }
        }
        num_check = num_check - de_check;
        de_check = 0;
      } // end of num_check !=0 part
      else { //no waiting process regular check: num_wait == 0
        de_check = 0;
        for (int i = 0; i < num_check; i++) { // check by task input order
          if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "initiate") == 0) {  // initiate activity 
            checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].acts[j] -> res_num;
            if (checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type] > num_resources[checker[check_q[i] - 1].acts[j] -> res_type]) { // claim > resource: abord
              printf("Banker aborts task %d before run begins:\n", checker[check_q[i] - 1].acts[0] -> pid);
              printf("     claim for resource %d (%d) exceeds number of units present (%d)\n",
                checker[check_q[i] - 1].acts[j] -> res_type, checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type], num_resources[checker[check_q[i] - 1].acts[j] -> res_type]);
              checker[check_q[i] - 1].state = 10; // abort state
              de_check = de_check + 1;
            } else { // run the task activity 
              take_time = 1;
              checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
              checker[check_q[i] - 1].state = 1; // running state
            }
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "request") == 0) { // check request activity && need to abord if exceeds claim
            if (checker[check_q[i] - 1].acts[j] -> res_num > checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type] ||
              checker[check_q[i] - 1].acts[j] -> res_num + checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] > checker[check_q[i] - 1].claim[checker[check_q[i] - 1].acts[j] -> res_type]) { // request exceeds claim, abord task
              checker[check_q[i] - 1].state = 10; // abort state
              de_check = de_check + 1;
              printf("During cycle %d-%d of Banker's algorithm\n", time, time + 1);
              printf("     Task %d's request exceeds its claim; aborted;\n", checker[check_q[i] - 1].acts[0] -> pid); 
              for (int m = 1; m <= resource_type_num; m++) { // free holding resource; available next cycle
                res_temp[m] = res_temp[m] + checker[check_q[i] - 1].resource_hold[m];
                printf("              %d units of resource[%d] available next cycle.\n", res_temp[m], m);
              }
            } else if (checker[check_q[i] - 1].state != 2 && checker[check_q[i] - 1].acts[j] -> res_num <= num_resources[checker[check_q[i] - 1].acts[j] -> res_type] &&
              safe(checker[check_q[i] - 1], resource_type_num, num_resources, checker[check_q[i] - 1].acts[j] -> res_num, checker[check_q[i] - 1].acts[j] -> res_type)) { // can grant
              num_resources[checker[check_q[i] - 1].acts[j] -> res_type] = num_resources[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
              checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
              take_time = 1;
              checker[check_q[i] - 1].state = 1; // task is being granted 
              checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            } else { // cannot grant
              take_time = 1;
              checker[check_q[i] - 1].state = 2; // block state = 2
              num_wait = num_wait + 1; // number of waiting task increased by 1
              checker[check_q[i] - 1].wait = checker[check_q[i] - 1].wait + 1;
              add_block_q(block_q, checker[check_q[i] - 1].acts[0] -> pid);
              checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task activities right by 1 
            }
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "compute") == 0) { // everthing blocked during computing cycle
            take_time = 1;
            checker[check_q[i] - 1].state = 1;  // running state
            checker[check_q[i] - 1].acts[j] -> res_type = checker[check_q[i] - 1].acts[j] -> res_type - 1;
            checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            if (checker[check_q[i] - 1].acts[j] -> res_type != 0) {
              checker[check_q[i] - 1] = block(checker[check_q[i] - 1], j, 1); // shift task right by 1
            }
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "release") == 0) { // release activity: resource available next cycle
            take_time = 1;
            checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] = checker[check_q[i] - 1].resource_hold[checker[check_q[i] - 1].acts[j] -> res_type] - checker[check_q[i] - 1].acts[j] -> res_num;
            res_temp[checker[check_q[i] - 1].acts[j] -> res_type] = res_temp[checker[check_q[i] - 1].acts[j] -> res_type] + checker[check_q[i] - 1].acts[j] -> res_num;
            checker[check_q[i] - 1].run = checker[check_q[i] - 1].run + 1;
            checker[check_q[i] - 1].state = 1;
          } else if (strcmp(checker[check_q[i] - 1].acts[j] -> title, "terminate") == 0) {  // terminate activity
            checker[check_q[i] - 1].state = 999;  // terminate state
            de_check = de_check + 1;
            take_time = 1;
          }
        }
        for (int i = 0; i < num_of_tasks; i++) {  // adjust check_q
          if ((checker[i].state == 10 || checker[i].state == 999) && contains(check_q, checker[i].acts[0] -> pid)) {
            remove_check_q(check_q, checker[i].acts[0] -> pid);
          }
        }
        num_check = num_check - de_check;
        de_check = 0;
      } // end num_wait = 0 checking process
      if (take_time == 1) {
        time = time + 1;
      }
    }
  }
  return checker;
}

int main(int argc, char * argv[]) {
  FILE * fp;  // input file
  int num_of_tasks; // total number of input tasks
  int resource_type_num; // resource type numbers
  char act_name[10]; // activity name
  Task * fifo_checker;  // task checker for fifo manager
  Task * banker_checker;  // task checker for banker manager
  Task * read = (Task * ) malloc(sizeof(Task)); // read in task

  fp = fopen(argv[1], "r");
  fscanf(fp, "%d", & num_of_tasks);
  fscanf(fp, "%d", & resource_type_num);

  int num_resources[resource_type_num]; // number of total resouces
  int num_activities[num_of_tasks]; // number of activities 

  for (int i = 0; i < num_of_tasks; i++) { // initialize num_activities 
    num_activities[i] = 0;
  }

  for (int i = 1; i <= resource_type_num; i++) { // read resoures numbers into resource array
    fscanf(fp, "%d", & num_resources[i]);
  }

  int c = getc(fp);
  int total_input = 0;

  for (int j = 0; j < MAX_ACT; j++) { // read all the activities first
    read[0].acts[j] = (activity * ) malloc(sizeof(activity * ));
    fscanf(fp, "%s", act_name);
    if (c == EOF) {
      break;
    } else {
      read[0].acts[j] -> title = (char * ) malloc(sizeof(char * ));
      strcpy(read[0].acts[j] -> title, act_name);
      fscanf(fp, "%d %d %d", & read[0].acts[j] -> pid, &
        read[0].acts[j] -> res_type, &
        read[0].acts[j] -> res_num);
      total_input = total_input + 1;
      c = getc(fp);
    }
  }

  for (int i = 0; i < total_input; i++) { // find out number of activities for each task
    for (int j = 0; j < num_of_tasks; j++) {
      if (read[0].acts[i] -> pid == j + 1) {
        num_activities[j] = num_activities[j] + 1;
      }
    }
  }

  if (num_of_tasks > 0) {
    Task * queue = (Task * ) malloc(num_of_tasks * sizeof(Task));

    for (int i = 0; i < num_of_tasks; i++) { // copy the correct order of activities for each task
      int j = 0;
      for (int m = 0; m < total_input; m++) {
        if (read[0].acts[m] -> pid == i + 1) {
          queue[i].acts[j] = (activity * ) malloc(sizeof(activity * ));
          queue[i].acts[j] = (activity * ) malloc(sizeof(activity * ));
          queue[i].acts[j] -> title = (char * ) malloc(sizeof(char * ));
          strcpy(queue[i].acts[j] -> title, read[0].acts[m] -> title);
          queue[i].acts[j] -> pid = read[0].acts[m] -> pid;
          queue[i].acts[j] -> res_type = read[0].acts[m] -> res_type;
          queue[i].acts[j] -> res_num = read[0].acts[m] -> res_num;
          j = j + 1;
        }
      }
      queue[i].state = 0; // initialize state
      queue[i].run = 0; // initialize running time
      queue[i].wait = 0;  // initialize waiting time
      queue[i].compute = 0; // initialize compute tracker

      for (int k = 1; k <= resource_type_num; k++) { // initialize holding resources and claim to 0
        queue[i].resource_hold[k] = 0;
        queue[i].claim[k] = 0;
      }
    }

    fifo_checker = fifo_manager(num_of_tasks, num_activities, resource_type_num, num_resources, queue);
    banker_checker = banker_manager(num_of_tasks, num_activities, resource_type_num, num_resources, queue);
    printf("\n");
    printf("                      FIFO                         BANKER'S\n");
    int total_run = 0;  // total fifo running time 
    int total_wait = 0; // total fifo waiting time

    int banker_run = 0; // total banker running time
    int banker_wait = 0;  // total banker waiting time
    for (int i = 0; i < num_of_tasks; i++) {
      if (fifo_checker[i].state == 10 && banker_checker[i].state == 999) {  // fifo abort && banker terminate
        banker_run = banker_run + banker_checker[i].run;
        banker_wait = banker_wait + banker_checker[i].wait;
        printf("     Task %d        aborted                Task %d        %d    %d    %.0f%%\n", fifo_checker[i].acts[0] -> pid,
          banker_checker[i].acts[0] -> pid, banker_checker[i].run + banker_checker[i].wait,
          banker_checker[i].wait, (double) banker_checker[i].wait / (banker_checker[i].run + banker_checker[i].wait) * 100);
      } else if (fifo_checker[i].state == 999 && banker_checker[i].state == 10) { // fifo terminate && banker abort
        total_run = total_run + fifo_checker[i].run;
        total_wait = total_wait + fifo_checker[i].wait;
        printf("     Task %d        %d    %d    %.0f%%          Task %d        aborted\n", fifo_checker[i].acts[0] -> pid, fifo_checker[i].run + fifo_checker[i].wait,
          fifo_checker[i].wait, (double) fifo_checker[i].wait / (fifo_checker[i].run + fifo_checker[i].wait) * 100,
          banker_checker[i].acts[0] -> pid);

      } else if (fifo_checker[i].state == 10 && banker_checker[i].state == 10) {  // both fifo and banker abort
        printf("     Task %d        aborted                Task %d        aborted\n", fifo_checker[i].acts[0] -> pid, banker_checker[i].acts[0] -> pid);
      } else {  // both fifo and banker terminate
        total_run = total_run + fifo_checker[i].run;
        total_wait = total_wait + fifo_checker[i].wait;
        banker_run = banker_run + banker_checker[i].run;
        banker_wait = banker_wait + banker_checker[i].wait;
        printf("     Task %d        %d    %d    %.0f%%          Task %d        %d    %d    %.0f%%\n", fifo_checker[i].acts[0] -> pid, fifo_checker[i].run + fifo_checker[i].wait,
          fifo_checker[i].wait, (double) fifo_checker[i].wait / (fifo_checker[i].run + fifo_checker[i].wait) * 100,
          banker_checker[i].acts[0] -> pid, banker_checker[i].run + banker_checker[i].wait,
          banker_checker[i].wait, (double) banker_checker[i].wait / (banker_checker[i].run + banker_checker[i].wait) * 100);
      }
    }
    printf("     Total         %d    %d    %.0f%%          Total         %d    %d    %.0f%%\n", total_run + total_wait, total_wait, (double) total_wait / (total_run + total_wait) * 100,
      banker_run + banker_wait, banker_wait, (double) banker_wait / (banker_run + banker_wait) * 100);
    free(read);
    free(queue);
    free(fifo_checker);
    free(banker_checker);
  }
  fclose(fp);
  return 0;
}