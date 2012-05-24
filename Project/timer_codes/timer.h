#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define TIMER_PORT 6666			// Port on which timer is listening
#define TCPD_TIMER_PORT 5555		// Port that the TCPD_CLIENT uses to communicate with the timer
#define LOCALHOST "127.0.0.1"
#define DECREMENT_VALUE 50000

//timer_node
typedef struct timer_node{
     int seqnum;		// sequence number of the packet
     double timeout;		// timeout value in microseconds
     struct timer_node *next;	// next node of the timer
     struct timer_node *prev;	// previous node of the timer
}tnode;

//timer_list
typedef struct{
tnode *head,*tail;		// head and tail of the Delta list
int numtnodes;			// number of send and unack packets
}tlist;


typedef struct{
  int seqnum;
  double timeout;
} timerPacket;

// timer functions
tlist* create_tlist();
void delete_tlist(tlist *list);
void print_tlist(tlist* list);
tnode* create_tnode(int seqnum,double timeout);
void remove_tnode(tnode *tnode_ptr);
int addTotlist(tlist *list, tnode *tnode_ptr);
int removeFromtlist(tlist *list, tnode *tnode_ptr,int ack);
void tlist_clock(tlist *list,double select_to);
