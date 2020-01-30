/*
 * Copyright(C) 2011-2016 Pedro H. Penna   <pedrohenriquepenna@gmail.com>
 *              2015-2016 Davidson Francis <davidsondfgl@hotmail.com>
 *
 * This file is part of Nanvix.
 *
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <nanvix/clock.h>
#include <nanvix/const.h>
#include <nanvix/hal.h>
#include <nanvix/pm.h>
#include <signal.h>

struct multiqueue {
	struct process *super_user_proc[PROC_MAX];
	struct process *user_proc[PROC_MAX];
	unsigned nb_su_proc;
	unsigned nb_u_proc;
};


/**
 * @brief Schedules a process to execution.
 * 
 * @param proc Process to be scheduled.
 */
PUBLIC void sched(struct process *proc)
{
	proc->state = PROC_READY;
	proc->counter = 0;
}

/**
 * @brief Stops the current running process.
 */
PUBLIC void stop(void)
{
	curr_proc->state = PROC_STOPPED;
	sndsig(curr_proc->father, SIGCHLD);
	yield();
}

/**
 * @brief Resumes a process.
 * 
 * @param proc Process to be resumed.
 * 
 * @note The process must stopped to be resumed.
 */
PUBLIC void resume(struct process *proc)
{	
	/* Resume only if process has stopped. */
	if (proc->state == PROC_STOPPED)
		sched(proc);
}

/**
 * @brief Yields the processor.
 */
PUBLIC void yield(void)
{
	struct process *p;    /* Working process.     */
	struct process *next; /* Next process to run. */

	/* Re-schedule process for execution. */
	if (curr_proc->state == PROC_RUNNING)
		sched(curr_proc);

	/* Remember this process. */
	last_proc = curr_proc;

	/* Check alarm. */
	for (p = FIRST_PROC; p <= LAST_PROC; p++)
	{
		/* Skip invalid processes. */
		if (!IS_VALID(p))
			continue;
		
		/* Alarm has expired. */
		if ((p->alarm) && (p->alarm < ticks))
			p->alarm = 0, sndsig(p, SIGALRM);
	}

	/* Choose a process to run next. */
	// next = IDLE;
	// for (p = FIRST_PROC; p <= LAST_PROC; p++)
	// {
	// 	/* Skip non-ready process. */
	// 	if (p->state != PROC_READY)
	// 		continue;
		
	// 	/*
	// 	 * Process with lowest
	// 	 * priority + niceness found
	// 	 * if equal higest waiting time
	// 	 * is choosen
	// 	 */
	// 	if (p->counter > next->counter)   
	// 	{
	// 		next->counter++;
	// 		next = p;
	// 	}
			
	// 	/*
	// 	 * Increment waiting
	// 	 * time of process.
	// 	 */
	// 	else
	// 		p->counter++;
	// }

	struct multiqueue multiqueue;
	multiqueue.nb_su_proc = 0;
	multiqueue.nb_u_proc = 0;


	for (p = FIRST_PROC; p <= LAST_PROC; p++) {
		if(p->state != PROC_READY)
			continue;

		if (IS_SUPERUSER(p)) {
			multiqueue.super_user_proc[multiqueue.nb_su_proc] = p;
			multiqueue.nb_su_proc++;
		} else {
			multiqueue.user_proc[multiqueue.nb_u_proc] = p;
			multiqueue.nb_u_proc++;
		}
	}

	next = IDLE;

	struct process **proc_list;
	unsigned proc_list_size;

	if (multiqueue.nb_su_proc != 0) {
		proc_list = multiqueue.super_user_proc;
		proc_list_size = multiqueue.nb_su_proc;
	} else {
		proc_list = multiqueue.user_proc;
		proc_list_size = multiqueue.nb_u_proc;
	}

	for (unsigned i = 0; i < proc_list_size; i++) {
		if (proc_list[i]->counter > next->counter) {
			next->counter++;
			next = proc_list[i];
		} else {
			proc_list[i]->counter++;
		}

	}

	
	/* Switch to next process. */
	next->priority = PRIO_USER;
	next->state = PROC_RUNNING;
	next->counter = PROC_QUANTUM;
	if (curr_proc != next)
		switch_to(next);
}
