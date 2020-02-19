#include <sys/sem.h>
#include <nanvix/pm.h>
	
#define SEM_MAX  128

struct sem
{
	unsigned sem_key;
	int sem_id;
	unsigned sem_val;
	struct process **sem_list_procs;
	unsigned sem_nb_waiting;
};

static unsigned sem_list_length;
static struct sem *sem_list[SEM_MAX];

int create_sem(unsigned key) {
    if (sem_list_length == SEM_MAX)
        return -1;

    int idx = sem_list_length;
    sem_list_length++;
    sem_list[idx]->sem_key = key;
    sem_list[idx]->sem_id = key;
    sem_list[idx]->sem_val = 1;
    sem_list[idx]->sem_list_procs = NULL;
    sem_list[idx]->sem_nb_waiting = 0;

    return idx;
}

int destruct_sem(unsigned idx) {
    if (idx >= SEM_MAX)
        return -1;

    while (idx < sem_list_length - 1) {
        sem_list[idx] = sem_list[idx + 1];
        idx++;
    }

    sem_list_length--;
    return 0;
}

PUBLIC int sys_semget(unsigned key) 
{
    unsigned i = 0;
    while (i < sem_list_length && sem_list[i]->sem_key != key)
        i++;
    
    if (i != sem_list_length)
        return sem_list[i]->sem_id;
    
    else
        return sem_list_length;
}

PUBLIC int sys_semctl(int semid, int cmd, int val)
{
    if (cmd != GETVAL && cmd != SETVAL && cmd != IPC_RMID)
        return -1;
    
    if (semid < 0 || semid > SEM_MAX - 1)
        return -2;

    unsigned i = 0;
    while (i < sem_list_length && sem_list[i]->sem_id != semid)
        i++;

    if (i == sem_list_length)
        return -3;

    int ret = 0;
    switch (cmd)
    {
        case GETVAL:
            ret = sem_list[i]->sem_val;
            break;
        case SETVAL:
            sem_list[i]->sem_val = val;
            break;
        case IPC_RMID:
            ret = destruct_sem(i);
            break;
        default:
            ret = -4;
            break;
    }

    return ret;
}

PUBLIC int sys_semop(int semid, int op) {
    if (semid < 0 || semid > SEM_MAX - 1)
        return -1;

    unsigned i = 0;
    while (i < sem_list_length && sem_list[i]->sem_id != semid)
        i++;

    if (i == sem_list_length)
        return -1;

    struct sem *current_sem = sem_list[i];

    if (op < 0) {
        disable_interupts();
        if (current_sem->sem_val > 0) {
            current_sem->sem_val--;
        } else {
            current_sem->sem_nb_waiting++;
            sleep(current_sem->sem_list_procs, 0);
        }
        enable_interupts();
    } else {
        disable_interupts();
        if (current_sem->sem_val == 0 && current_sem->sem_nb_waiting > 0) {
            struct process *next = *current_sem->sem_list_procs;
            while (next->next != NULL && next->next->next != NULL)
                next = (next)->next;
            struct process *to_wake = next->next;
            next->next = NULL;
            wakeup(&to_wake);
            current_sem->sem_nb_waiting--;
        } else {
            current_sem->sem_val++;
        }
        enable_interupts();
    }

    return 0;
}
