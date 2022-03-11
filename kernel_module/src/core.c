//////////////////////////////////////////////////////////////////////
//                     University of California, Riverside
//
//
//
//                             Copyright 2022
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng, Yu-Chia Liu
//
//   Description:
//     Core of Kernel Module for CSE202's Resource Container
//
////////////////////////////////////////////////////////////////////////

#include "blockmma.h"
#include <asm/segment.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include "core.h"

extern struct miscdevice blockmma_dev;
/**
 * Enqueue a task for the caller/accelerator to perform computation.
 */
 
int getTask = 0;
int syncTask = 0;
int sendTask = 0;
int compTask = 0;
struct tasks{
    struct blockmma_cmd cmd;
    float *a;
    float *b;
    float *c;
    int count;
    struct tasks *next_task;
};
struct tasks *task_l;
int tile_v = 0;

long blockmma_send_task(struct blockmma_cmd __user *user_cmd)
{
    printk(KERN_INFO "----------------blockMMA----------------");
    printk(KERN_INFO "Send Task!!!");
    if(sendTask == 0){
         task_l = kmalloc(sizeof(struct tasks), GFP_KERNEL);  
    }
    struct tasks *temp_l;
    temp_l = kmalloc(sizeof(struct tasks), GFP_KERNEL);;
    temp_l->count = sendTask;
    printk(KERN_INFO "Send Task1!!!");
    printk(KERN_INFO "Send Task2!!!");
    temp_l->a = kmalloc(128*128*sizeof(float *),GFP_KERNEL);
    temp_l->b = kmalloc(128*128*sizeof(float *), GFP_KERNEL);
    temp_l->c = kmalloc(128*128*sizeof(float *), GFP_KERNEL);
    printk(KERN_INFO "Send Task3!!!");
    copy_from_user(temp_l->a, (void *)user_cmd->a,128*128*sizeof(float *));
    copy_from_user(temp_l->b, (void *)user_cmd->b,128*128*sizeof(float *));
    if (copy_from_user(&temp_l->cmd, user_cmd, sizeof(task_l->cmd)))
    {
        return -1;
    }
    if(sendTask == 0){
    	task_l = temp_l;
    }
    else{
        struct tasks *temp_ll;
        temp_ll = task_l;
    	while(temp_ll->next_task){
             temp_ll = temp_ll->next_task;
    	}
    	temp_ll->next_task = temp_l;
    }
    sendTask++;
    struct task_struct *p = current;
    return p->pid;
}


/**
 * Return the task id of a task to the caller/accelerator to perform computation.
 */
int blockmma_get_task(struct blockmma_hardware_cmd __user *user_cmd)
{
    if(getTask<sendTask){
        printk(KERN_INFO "Entered Get Task!!!");
        struct tasks *temp_l = task_l;
        while(temp_l->count != getTask){
        	temp_l = temp_l->next_task;
        }
    	copy_to_user((void *)user_cmd->a, temp_l->a, 128*128*sizeof(float *));
    	copy_to_user((void *)user_cmd->b, temp_l->b, 128*128*sizeof(float *));
    	copy_to_user((void *)user_cmd->c, temp_l->c, 128*128*sizeof(float *));
    	getTask++;
    	struct task_struct *p = current;
    	return p->pid;
    }
    return -1;
}


/**
 * Return until the task specified in the command is done.
 */
int blockmma_comp(struct blockmma_hardware_cmd __user *user_cmd)
{
	printk(KERN_INFO "Entered Comp Task!!!");
	struct tasks *temp_l;
	temp_l = task_l;
        while(temp_l->count != compTask){
        	temp_l = temp_l->next_task;
        }
        temp_l->c = kmalloc(128*128*sizeof(float *), GFP_KERNEL);
	if (copy_from_user(temp_l->c, (void *)user_cmd->c, 128*128*sizeof(float *)))
    	{
        	return -1;
    	}
    	compTask++;
    	struct task_struct *p = current;
    	printk(KERN_INFO "%d\n", p->pid);
    	return p->pid;
}


/**
 * Return until all outstanding tasks for the calling process are done
 */
int blockmma_sync(struct blockmma_cmd __user *user_cmd)
{    
    if(sendTask == compTask){
	printk(KERN_INFO "Check Sync Task!!!");
//	while(task_l->next_task){
	//	printk(KERN_INFO "Final: %d\n", task_l->count);
	//	task_l = task_l->next_task;
	//}
	copy_to_user((void *)task_l->cmd.c, task_l->c, 256*256*sizeof(float *));
    	struct task_struct *p = current;
    	printk(KERN_INFO "%d\n", p->pid);
    	return 0;
    }
    return -1;
}


/*
 * Tell us who wrote the module
 */
int blockmma_author(struct blockmma_hardware_cmd __user *user_cmd)
{
    struct blockmma_hardware_cmd cmd;
    char auth[] = "Huy Dinh Tran (htran197), 862325308 and Suraj Thalari (sthal001), 862325121";
    if (copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        return -1;
    }
    copy_to_user((void *)cmd.a, (void *)auth, sizeof(auth));
    return 0;
}

int blockmma_init(void)
{
    int ret =0;
    if ((ret = misc_register(&blockmma_dev)))
    {
        printk(KERN_ERR "Unable to register \"blockmma\" misc device\n");
        return ret;
    }
    printk("BlockMMA kernel module installed\n");
    return ret;
}

void blockmma_exit(void)
{
    printk("BlockMMA removed\n");
    misc_deregister(&blockmma_dev);
}
