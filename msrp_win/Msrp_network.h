#ifndef _MSRP_NETWORK_H
#define _MSRP_NETWORK_H

#include "msrp.h"
#include "msrp_utils.h"


/*
	Network (recv/send) threads and related stuff
*/
void *msrp_recv_thread(void *data);
pthread_t recv_thread;		/* The receiving thread */
pthread_mutex_t recv_lock;	/* A lock to access the file descriptors */
fd_set recv_fds;		/* File descriptors set for reading */
int recv_fdmax;			/* Highest file descriptor */
int recv_pair[2];		/* A socket pair to unblock select, when needed */
void msrp_recv_add_fd(int fd);
void msrp_recv_del_fd(int fd);

#ifdef USEFORWIN
	void winsocket_init();
#endif


#endif