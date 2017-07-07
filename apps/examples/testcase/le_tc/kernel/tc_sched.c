/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/// @file sched.c
/// @brief Test Case Example for Sched API

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "../../../../../os/kernel/sched/sched.h"
#include "tc_internal.h"

#define USECVAL         1000000
#define SCHED_PRIORITY  13
#define SLEEPVAL        10000
#define SEC_2           2
#define LOOPCOUNT       2
#define ARRLEN          2
#define VAL_3           3
#define VAL_5           5

pthread_t thread1, thread2;

pid_t g_task_pid;
bool g_callback = false;
bool g_pthread_callback = true;

/**
* @fn                   :sched_foreach_callback
* @description          :Function for tc_sched_sched_foreach
* @return               :void
*/
static void sched_foreach_callback(struct tcb_s *tcb, void *arg)
{
	/* it enumerate every task tcb, that means tcb created for current process will also be enumerated */
	if (tcb->pid == g_task_pid) {
		g_callback = true;
	}
}

/**
* @fn                   :function_wait
* @description          :Function for tc_sched_wait
* @return               :int
*/
static int function_wait(int argc, char *argv[])
{
	usleep(USECVAL);
	task_delete(0);
	return 0;
}

#if defined(CONFIG_SCHED_WAITPID) && defined(CONFIG_SCHED_HAVE_PARENT)
/**
* @fn                   :function_waitlong
* @description          :Function for tc_sched_wait
* @return               :int
*/
static int function_waitlong(int argc, char *argv[])
{
	sleep(SEC_2);
	task_delete(0);
	return 0;
}

/**
* @fn                   :function_waitid
* @description          :Function for tc_sched_waitid
* @return               :int
*/
static int function_waitid(int argc, char *argv[])
{
	usleep(SLEEPVAL);
	task_delete(0);
	return 0;
}
#endif

/**
* @fn                   :threadfunc_callback
* @description          :Function for tc_sched_sched_yield
* @return               :void*
*/
static void *threadfunc_callback(void *param)
{
	g_pthread_callback = true;
	sleep(VAL_3);
	sched_yield();
	pthread_exit((pthread_addr_t)1);
	/* yield to another thread, g_pthread_callback will remain true in main process */
	g_pthread_callback = false;
	return NULL;
}

/**
* @fn                   :tc_sched_sched_setget_scheduler_param
* @brief                :set and get scheduler policies for the named process
* @scenario             :set and get scheduler policies for the named process, sched_getscheduler should return scheduler set
* API's covered         :sched_setscheduler, sched_getscheduler
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_setget_scheduler_param(void)
{
	int ret_chk = ERROR;
	struct sched_param st_setparam;
	struct sched_param st_getparam;
	int loop_cnt = LOOPCOUNT;
	int arr_idx = 0;
	int sched_arr[ARRLEN] = { SCHED_RR, SCHED_FIFO };

	while (arr_idx < loop_cnt) {
		st_setparam.sched_priority = SCHED_PRIORITY;
		ret_chk = sched_setparam(getpid(), &st_setparam);
		TC_ASSERT_EQ("sched_setparam", ret_chk, OK);

		ret_chk = sched_setscheduler(getpid(), sched_arr[arr_idx], &st_setparam);
		TC_ASSERT_NEQ("sched_setscheduler", ret_chk, ERROR);

		/* ret_chk should be SCHED set */
		ret_chk = sched_getscheduler(getpid());
		TC_ASSERT_EQ("sched_getscheduler", ret_chk, sched_arr[arr_idx]);

		ret_chk = sched_getparam(getpid(), &st_getparam);
		TC_ASSERT_EQ("sched_getparam", ret_chk, OK);
		TC_ASSERT_EQ("sched_getparam", st_setparam.sched_priority, st_getparam.sched_priority);
		arr_idx++;
	}
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_rr_get_interval
* @brief                :get  the  SCHED_RR  interval for the named process
* @scenario             :get the SCHED_RR interval for the named process
* API's covered         :sched_rr_get_interval
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_rr_get_interval(void)
{
	int ret_chk;
	struct timespec st_timespec1;
	struct timespec st_timespec2;
	st_timespec1.tv_sec = 0;
	st_timespec1.tv_nsec = -1;

	st_timespec2.tv_sec = 0;
	st_timespec2.tv_nsec = -1;
	/* Values are filled in st_timespec structure to differentiate them with values overwritten by rr_interval */
	ret_chk = sched_rr_get_interval(0, &st_timespec1);
	TC_ASSERT_NEQ("sched_rr_get_interval", ret_chk, ERROR);
	TC_ASSERT_GEQ("sched_rr_get_interval", st_timespec1.tv_nsec, 0);
	TC_ASSERT_LT("sched_rr_get_interval", st_timespec1.tv_nsec, 1000000000);

	ret_chk = sched_rr_get_interval(getpid(), &st_timespec2);
	TC_ASSERT_NEQ("sched_rr_get_interval", ret_chk, ERROR);
	TC_ASSERT_GEQ("sched_rr_get_interval", st_timespec2.tv_nsec, 0);
	TC_ASSERT_LT("sched_rr_get_interval", st_timespec2.tv_nsec, 1000000000);

	/* after sched_rr_get_interval() call, st_timespec structure should be overwritten with rr_interval values */
	TC_ASSERT_EQ("sched_rr_get_interval", st_timespec1.tv_sec, st_timespec2.tv_sec);
	TC_ASSERT_EQ("sched_rr_get_interval", st_timespec1.tv_nsec, st_timespec2.tv_nsec);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_yield
* @brief                :sched_yield() causes the calling thread to relinquish the CPU.
* @scenario             :sched_yield() causes the calling thread to relinquish the CPU.  The thread is moved
*                        to the end of the queue for its static priority and a new thread gets to run.
* API's covered         :sched_yield
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_yield(void)
{
	int ret_chk = 0;

	ret_chk = pthread_create(&thread1, NULL, threadfunc_callback, NULL);
	TC_ASSERT_EQ("pthread_create", ret_chk, OK);
	TC_ASSERT("sched_yield", g_pthread_callback);

	ret_chk = pthread_create(&thread2, NULL, threadfunc_callback, NULL);
	TC_ASSERT_EQ("pthread_create", ret_chk, OK);
	TC_ASSERT("sched_yield", g_pthread_callback);

	/* wait for threads to exit */
	pthread_join(thread1, 0);
	pthread_join(thread2, 0);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_wait
* @brief                :Suspends execution of the current process until one of its children terminates
* @scenario             :wait for state changes in a child of the calling process, and obtain information about
*                        the child whose state has changed. A state change is considered to be: the child
*                        terminated; the child was stopped by a signal; or the child was resumed by a signal
* API's covered         :wait
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
#ifdef CONFIG_SCHED_WAITPID
#ifdef CONFIG_SCHED_HAVE_PARENT
static void tc_sched_wait(void)
{
	int ret_chk;
	pid_t child1_pid;
	pid_t child2_pid;
	int status;

	/* creating new process */
	child1_pid = task_create("sched1", SCHED_PRIORITY_DEFAULT, CONFIG_USERMAIN_STACKSIZE, function_wait, (char * const *)NULL);
	TC_ASSERT_GT("task_create", child1_pid, 0);

	child2_pid = task_create("sched2", SCHED_PRIORITY_DEFAULT, CONFIG_USERMAIN_STACKSIZE, function_waitlong, (char * const *)NULL);
	TC_ASSERT_GT("task_create", child2_pid, 0);

	/* child which exits first is handled by wait, here child1_pid exits earlier. */
	usleep(SLEEPVAL);

	/* wait for child to exit, and store child's exit status */
	ret_chk = wait(&status);
	TC_ASSERT_NEQ("wait", ret_chk, ERROR);

	TC_ASSERT("wait", child1_pid == (pid_t)ret_chk || child2_pid == (pid_t)ret_chk);

	/* wait for second child to exit */
	sleep(SEC_2);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_waitid
* @brief                :Suspends execution of the current process until one of its children changes state
* @scenario             :provides more precise control over which child state changes to wait for.
* API's covered         :waitid
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/

static void tc_sched_waitid(void)
{
	int ret_chk;
	pid_t child_pid;
	siginfo_t info;

	child_pid = task_create("tc_waitid", SCHED_PRIORITY_DEFAULT, CONFIG_USERMAIN_STACKSIZE, function_waitid, (char * const *)NULL);
	TC_ASSERT_GT("task_create", child_pid, 0);

	ret_chk = waitid(P_PID, child_pid, &info, WEXITED);
	TC_ASSERT_NEQ("waitid", ret_chk, ERROR);
	TC_ASSERT_EQ("waitid", info.si_pid, child_pid);

	TC_SUCCESS_RESULT();
}
#endif

/**
* @fn                   :tc_sched_waitpid
* @brief                :Suspends the calling process until a specified process terminates
* @scenario             :The waitpid() system call suspends execution of the calling process until a child
*                        specified by pid argument has changed state. By default, waitpid() waits only for
*                        terminated children
* API's covered         :waitpid
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/

static void tc_sched_waitpid(void)
{
	int ret_chk;
	pid_t child_pid;
	int *status = (int *)malloc(sizeof(int));

	child_pid = task_create("tc_waitpid", SCHED_PRIORITY_DEFAULT, CONFIG_USERMAIN_STACKSIZE, function_wait, (char * const *)NULL);
	TC_ASSERT_GT("task_create", child_pid, 0);

	ret_chk = waitpid(child_pid, status, 0);
	TC_ASSERT_NEQ_CLEANUP("waitpid", ret_chk, ERROR, errno, TC_FREE_MEMORY(status));
	TC_ASSERT_EQ_CLEANUP("waitpid", ret_chk, child_pid, errno, free(status));

	free(status);
	TC_SUCCESS_RESULT();
}
#endif

/**
* @fn                   :tc_sched_sched_gettcb
* @brief                :Given a tsk id, this function will return pointer to corresponding TCB or null if no such task id
* @scenario             :return pointer to corresponding TCB or null for given task id
* API's covered         :gettcb
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_gettcb(void)
{
	struct tcb_s *tcb;
	pid_t pid;
	int stat_loc;

	pid = task_create("tc_gettcb", SCHED_PRIORITY_DEFAULT, CONFIG_USERMAIN_STACKSIZE, function_wait, (char * const *)NULL);
	TC_ASSERT_NEQ("task_create", pid, ERROR);

	tcb = sched_gettcb(pid);
	TC_ASSERT_NOT_NULL("sched_gettcb", tcb);
	TC_ASSERT_EQ("sched_gettcb", tcb->pid, pid);

	waitpid(pid, &stat_loc, 0);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_lock_unlock
* @brief                :sched_lock disables context switching by disabling addition of new tasks,
*                        to the task list and increment lock count, sched_unlock decrements preemption lock count
* @scenario             :sched_lock increments lock count, sched_unlock decrements preemption lock count
* API's covered         :sched_lock, sched_unlock
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/

static void tc_sched_sched_lock_unlock(void)
{
	int ret_chk = ERROR;
	int cntlock;
	struct tcb_s *st_tcb = NULL;

	st_tcb = sched_self();
	TC_ASSERT_NOT_NULL("sched_self", st_tcb);

	cntlock = st_tcb->lockcount;

	ret_chk = sched_lock();
	TC_ASSERT_NEQ("sched_lock", ret_chk, ERROR);

	/* after sched_lock, lock count gets incremented */
	ret_chk = cntlock;
	cntlock = st_tcb->lockcount;
	TC_ASSERT_EQ("sched_lock", ret_chk, cntlock - 1);

	ret_chk = sched_lock();
	TC_ASSERT_NEQ("sched_lock", ret_chk, ERROR);

	/* after sched_lock, lock count gets incremented */
	ret_chk = cntlock;
	cntlock = st_tcb->lockcount;
	TC_ASSERT_EQ("sched_lock", ret_chk, cntlock - 1);

	ret_chk = sched_unlock();
	TC_ASSERT_NEQ("sched_unlock", ret_chk, ERROR);

	/* after sched_unlock, lock count gets decremented */
	ret_chk = cntlock;
	cntlock = st_tcb->lockcount;
	TC_ASSERT_EQ("sched_unlock", ret_chk, cntlock + 1);

	ret_chk = sched_unlock();
	TC_ASSERT_NEQ("sched_unlock", ret_chk, ERROR);

	/* after sched_unlock, lock count gets decremented */
	ret_chk = cntlock;
	cntlock = st_tcb->lockcount;
	TC_ASSERT_EQ("sched_unlock", ret_chk, cntlock + 1);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_self
* @brief                :Return current thread tcb
* @scenario             :Return current thread tcb structure, verified by getting sched_gettcb(getpid)
* API's covered         :sched_self
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_self(void)
{
	struct tcb_s *st_tcbself;
	struct tcb_s *st_tcbpid;
	/* get process id */

	st_tcbpid = sched_self();
	TC_ASSERT_NOT_NULL("sched_self", st_tcbpid);

	/* should return tcb for current process */
	st_tcbself = sched_self();
	TC_ASSERT_NOT_NULL("sched_self", st_tcbself);
	TC_ASSERT_EQ("sched_self", st_tcbself->pid, st_tcbpid->pid);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_verifytcb
* @brief                :Returns true if tcb refers to active task, false if it is state tcb handle
* @scenario             :sched_verifytcb returns true if tcb refers to active task, false if it is state tcb handle
* API's covered         :sched_verifytcb,sched_gettcb
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_verifytcb(void)
{
	bool ret_chk = false;
	struct tcb_s *st_tcb;
	st_tcb = sched_self();
	ret_chk = sched_verifytcb(st_tcb);
	TC_ASSERT("verfiytcb fail", ret_chk);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_foreach
* @brief                :Enumerate over each task and provide the TCB of each task to a user callback functions.
* @scenario             :provides TCB to user callback function "sched_foreach_callback"
* API's covered         :sched_foreach
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/
static void tc_sched_sched_foreach(void)
{
	g_callback = false;
	struct tcb_s *st_tcb;
	st_tcb = sched_self();
	g_task_pid = st_tcb->pid;

	/* provides TCB to user callback function "sched_foreach_callback" */
	sched_foreach(sched_foreach_callback, NULL);
	TC_ASSERT("sched_foreach", g_callback);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_lockcount
* @brief                :sched_lockcount returns the lock count
* @scenario             :after sched_lock and sched_unlock, check the lockcount
* API's covered         :sched_lockcount
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/

static void tc_sched_sched_lockcount(void)
{
	int ret_chk = ERROR;
	int prev_cnt;
	int cur_cnt;
	struct tcb_s *st_tcb = NULL;

	st_tcb = sched_self();
	TC_ASSERT_NOT_NULL("sched_self", st_tcb);

	prev_cnt = sched_lockcount();
	ret_chk = sched_lock();
	TC_ASSERT_NEQ("sched_lock", ret_chk, ERROR);

	/* after sched_lock, lock count gets incremented */
	cur_cnt = sched_lockcount();
	TC_ASSERT_EQ("sched_lockcount", prev_cnt, cur_cnt - 1);

	prev_cnt = cur_cnt;

	ret_chk = sched_unlock();
	TC_ASSERT_NEQ("sched_unlock", ret_chk, ERROR);

	/* after sched_unlock, lock count gets decremented */
	cur_cnt = sched_lockcount();
	TC_ASSERT_EQ("sched_lockcount", prev_cnt, cur_cnt + 1);

	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_sched_sched_getstreams
* @brief                :return a pointer to the streams list for this thread
* @scenario             :check the streams list for current thread
* API's covered         :sched_getstreams
* Preconditions         :none
* Postconditions        :none
* @return               :void
*/

static void tc_sched_sched_getstreams(void)
{
	struct streamlist *stream;

	stream = sched_getstreams();
	TC_ASSERT_NOT_NULL("sched_getstreams", stream);

	TC_SUCCESS_RESULT();
}

/****************************************************************************
 * Name: sched
 ****************************************************************************/
int sched_main(void)
{
#ifdef CONFIG_SCHED_WAITPID
#ifdef CONFIG_SCHED_HAVE_PARENT
	tc_sched_wait();
	tc_sched_waitid();
#endif
	tc_sched_waitpid();
#endif
	tc_sched_sched_setget_scheduler_param();
	tc_sched_sched_rr_get_interval();
	tc_sched_sched_yield();
	tc_sched_sched_gettcb();
	tc_sched_sched_lock_unlock();
	tc_sched_sched_self();
	tc_sched_sched_verifytcb();
	tc_sched_sched_foreach();
	tc_sched_sched_lockcount();
	tc_sched_sched_getstreams();

	return 0;
}
