#include <sys/sem.h>

#define TRUE 1
#define FALSE 0

struct semaphore {
	int id;
 	unsigned key;
	unsigned value;
	unsigned nb_proc;
	struct process **processes;
};

static struct semaphore sem_list[SEM_MAX];
static int nb_sem = 0;

void init_sem(unsigned key, int idx) {
  // int id;
  // int id_taken = TRUE;

  // //find a unique id
  // for (int i = 0; i < SEM_MAX && id_taken; i++) { //for each possible id
  //   for (int j = 0; j < nb_sem; j++) { // for each sem
  //     if (i == sem_list[j].id)
  //       id_taken = FALSE;
  //   }
  //   if (!id_taken)
  //     id = i;
  // }

  //init structure
  sem_list[idx].id = (int) key;
  sem_list[idx].key = key;
  sem_list[idx].value = 0;
  sem_list[idx].nb_proc = 0;
  sem_list[idx].processes = NULL;
}

PUBLIC int sys_semget(unsigned key) {
  int i = 0;
  int id;
  //looking for an existing sem
  while (i < nb_sem && sem_list[i].key != key)
    i++;

  if (i == nb_sem) { //sem not found, creating a new one
    if (nb_sem == SEM_MAX)
      return -1; //no room for a new sem

    init_sem(key, i);
    nb_sem++;
  }
  id = sem_list[i].id;
  

  return id;
}

PUBLIC int sys_semctl(int semid, int cmd, int val) {
  int i, ret;
  if (cmd != GETVAL && cmd != SETVAL && cmd != IPC_RMID)
    return -1; //error not a command
  if (cmd == SETVAL && val < 1)
    return -1; //cannot set the sem to a negative number or 0
  
  i = 0;
  while (i < nb_sem && sem_list[i].id != semid)
    i++;

  if (i == nb_sem)
    return -1; //sem not found

  ret = 0;
  switch (cmd) {
  case GETVAL:
    ret = sem_list[i].value;
    break;
  case SETVAL:
    sem_list[i].value = val;
    break;
  case IPC_RMID:
    if (i != nb_sem - 1) {
      sem_list[i].id = sem_list[nb_sem - 1].id;
      sem_list[i].key = sem_list[nb_sem - 1].key;
      sem_list[i].value = sem_list[nb_sem - 1].value;
      sem_list[i].nb_proc = sem_list[nb_sem - 1].nb_proc;
      sem_list[i].processes = sem_list[nb_sem - 1].processes;
    }
    nb_sem--;
    break;
  }

  return ret;
}

PUBLIC int sys_semop(int semid, int op) {
  return semid + op;
}
