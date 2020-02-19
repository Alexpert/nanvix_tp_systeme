#include <sys/sem.h>



struct semaphore {
 	unsigned key;
	unsigned int id;
	unsigned int value;
	unsigned int nb_proc;
	struct process **processes;

};

 static struct semaphore sem_list[SEM_MAX];
 static unsigned int nb_sem = 0;

PUBLIC int sys_semget(unsigned key) {
  for (int i = 0; i < SEM_MAX; i ++) {
    if (sem_list[i].key == key) {
      return sem_list[i].id;
    }
  }

  sem_list[nb_sem].key = key;
  sem_list[nb_sem].id = nb_sem;

  nb_sem ++;
  return sem_list[nb_sem-1].id;

}

PUBLIC int sys_semctl(int semid, int cmd, int val) {
  if (val < 0)
    return -1;
  if (semid < 0 || semid > SEM_MAX - 1)
    return -1;
  if (cmd != GETVAL && cmd != SETVAL && cmd != IPC_RMID)
     return -1;

  unsigned i = 0;
  while (i < nb_sem && sem_list[i].id != i)
    i++;

  if (i == nb_sem)
    return -1;

  int ret = 0;
  switch (cmd) {
    case GETVAL:
      ret = sem_list[i].value;
      break;
    case SETVAL:
      sem_list[i].value = val;
      break;
    case IPC_RMID:
      while (i < nb_sem - 1) {
        sem_list[i].key = sem_list[i + 1].key;
        sem_list[i].id = sem_list[i + 1].id;
        sem_list[i].value = sem_list[i + 1].value;
        sem_list[i].nb_proc = sem_list[i + 1].nb_proc;
        sem_list[i].processes = sem_list[i + 1].processes;
        i++;
      }
      nb_sem --;
      break;
    default:
      ret = -1;
  }

  return ret;
}

PUBLIC int sys_semop(int semid, int op) {
  if (semid < 0 || semid > SEM_MAX - 1)
    return -1;

  unsigned i = 0;
  while (i < nb_sem && sem_list[i].id != i)
    i++;

  if (i == nb_sem)
    return -1;

  struct semaphore *current_sem = &sem_list[i];
  if (op < 0) {
    if (current_sem->value > 0) {
      current_sem->value--;
    } else {
      current_sem->value++;
      sleep(current_sem->processes, 0);
    }
  } else {
    if (current_sem->value == 0 && current_sem->nb_proc > 0) {
      struct process *next = *current_sem->processes;
      while (next->next != NULL && next->next->next != NULL)
        next = (next)->next;
      struct process *to_wake = next->next;
      next->next = NULL;
      wakeup(&to_wake);
      current_sem->nb_proc--;
    } else {
      current_sem->value++;
    }
  }
  return -1;
}
