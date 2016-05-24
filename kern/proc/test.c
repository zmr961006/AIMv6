
#include <console.h>
#include <proc.h>
#include <sched.h>

/*
 * Temporary test since I haven't implemented scheduler and timer yet.
 */

void kthread(void *arg)
{
	int j;
	int id = (int)arg;
	kprintf("KTHREAD%d: congratulations!\n", id);
	for (;;) {
		/* There is no unified timer interface so I implemented
		 * suspension as busy-wait.  Change the limit if things
		 * happen too fast for you. */
		for (j = 0; j < 100000; ++j)
			/* nothing */;
		kprintf("KTHREAD%d: running here\n", id);
		schedule();
	}
}

void proc_test(void)
{
	struct proc *kthreads[5];
	int i;

	for (i = 0; i < 5; ++i) {
		kthreads[i] = proc_new(NULL);
		kthreads[i]->mm = mm_new();	/* should be kernel_mm */
		proc_ksetup(kthreads[i], kthread, kthreads[i]->kstack,
		    (void *)i);
		kthreads[i]->state = PS_RUNNABLE;
		proc_add(kthreads[i]);
	}

	schedule();

	/*
	 * Correct output:
	 *
	 * THREAD0: congratulations!
	 * KTHREAD0: running here
	 * KTHREAD1: congratulations!
	 * KTHREAD1: running here
	 * KTHREAD2: congratulations!
	 * KTHREAD2: running here
	 * KTHREAD3: congratulations!
	 * KTHREAD3: running here
	 * KTHREAD4: congratulations!
	 * KTHREAD4: running here
	 * KTHREAD0: running here
	 * KTHREAD1: running here
	 * KTHREAD2: running here
	 * KTHREAD3: running here
	 * KTHREAD4: running here
	 * KTHREAD0: running here
	 * KTHREAD1: running here
	 * KTHREAD2: running here
	 * KTHREAD3: running here
	 * KTHREAD4: running here
	 * ...
	 */
}

