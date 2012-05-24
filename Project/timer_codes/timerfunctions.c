#include "timer.h"

tlist* create_tlist()
{
	tlist *list = NULL;

	list = (tlist *) calloc(1,sizeof(tlist));
	if (list != NULL) {
		list->numtnodes= 0;
		list->head = NULL;
		list->tail = NULL;
		printf("Timer list created\n");
	}
	else {
		printf("Error: Memory allocation failed!\n");
	}
	return list;
}

void remove_tlist(tlist *list)
{
	tnode *tnode_ptr;
	/*delete list if not empty*/
	if (list != NULL) {
		for (tnode_ptr = list->head; tnode_ptr != NULL; tnode_ptr = list->head) {
		    list->head = list->head->next;
		    remove_tnode(tnode_ptr);
		}
		free(list);
	}
}

void print_tlist(tlist* list) {
	tnode* temp;
	temp = list->head;
	/* If the timer list is not empty */
	if (list->head != NULL) {
		printf ("\n--------------------START LIST--------------------\n");
		while(temp != NULL) {
			printf("Sequence Number:%d\t\tTime Out:%lf microseconds\n",temp->seqnum,temp->timeout);
			temp = temp->next;
		}
		printf ("\n--------------------END LIST--------------------\n");
	}
}

tnode* create_tnode(int seqnum, double timeout)
{
	tnode *tnode_ptr = NULL;
	/* Allocate memory for node */
	if ((tnode_ptr = (tnode *) calloc(1,sizeof(tnode))) != NULL) {
		tnode_ptr->seqnum = seqnum;
		tnode_ptr->timeout = timeout;
		tnode_ptr->prev = NULL;
		tnode_ptr->next = NULL;
		if(tnode_ptr->timeout !=0) {
			printf("\nTimer Created for packet sequence no:%d with timeout :%f\n", tnode_ptr->seqnum, tnode_ptr->timeout);
		}
	}
	else {
		printf("Error: Memory allocation failed!\n");
	}
	return tnode_ptr;
}

void remove_tnode(tnode *tnode_ptr){
	/* Delete if not null */
	if (tnode_ptr != NULL) {
		tnode_ptr->prev = NULL;
		tnode_ptr->next = NULL;
		free(tnode_ptr);
	}
}

int addTotlist(tlist *list, tnode *tnode_ptr)
{
	tnode *temp, *prev_node;
	int tto = 0;
	// Insert the first node in the list 
	if (list->head == NULL && list->tail == NULL) {
		list->head = list->tail = tnode_ptr;
	}
	//If the timeout value of new node is less than the timeout value of head node than insert new node at the beginning of the delta list
	else if (tnode_ptr->timeout <= list->head->timeout) {
		list->head->timeout -= tnode_ptr->timeout;
		tnode_ptr->next = list->head;
		list->head->prev = tnode_ptr;
		list -> head = tnode_ptr;
		tnode_ptr->prev = NULL;
	}  
	// Insert the new node at some appropriate location in the delta list 
	else {
		temp = list->head;  
		tto=temp->timeout;
		while(tnode_ptr->timeout>tto && temp->next != NULL) {
		    prev_node=temp; 
		    temp=temp->next;
		    tto += temp->timeout;
		 }
		 if(temp->next!= NULL) {
		     tnode_ptr->timeout -= (tto - temp->timeout);
		     temp->timeout -= tnode_ptr->timeout; 
		     tnode_ptr->next=temp; 
		     prev_node->next=tnode_ptr;
		     tnode_ptr->prev=prev_node;
		     temp->prev=tnode_ptr; 
		 }    
		 else {
		     if(tnode_ptr->timeout < tto) {
		       tnode_ptr->timeout -= (tto-temp->timeout);

		       temp->timeout -= tnode_ptr->timeout;
		       prev_node->next = tnode_ptr;
		       tnode_ptr->next = temp;
		       temp->prev = tnode_ptr;
		       tnode_ptr->prev = prev_node;
		     } 
		     else {
		     tnode_ptr->timeout -= tto;
		     list->tail->next = tnode_ptr;
		     tnode_ptr->prev = list->tail;
		     tnode_ptr->next = NULL;
		     list->tail = tnode_ptr;
		     }
		 }
	} 
	list->numtnodes++;
	printf("new node successfully added to the delta list\n");
	//print_tlist(list);
	printf("Number of timer nodes:%d\n",list->numtnodes);
	return 1;
}

int removeFromtlist(tlist *list, tnode *tnode_ptr,int ack)
{
	tnode *nxt_node, *prev_node;
	int ispresent = 0;
	/*Check if list is empty*/
	if (list->numtnodes == 0) {
		return 0;
	}
	prev_node = NULL;
	nxt_node = list->head;
	while (nxt_node != NULL) {
		printf("In while\n");
		/* found it */
		if (nxt_node->seqnum == tnode_ptr->seqnum) {
			printf("Found!\n");
			ispresent = 1;
			/* delete head */
			if (prev_node == NULL) 
			{
				/*if only one node in the list*/
				printf("Only one node in list!\n");
				list->head = nxt_node->next;
				if (list->head == NULL) {
				    list->tail = NULL;
//				    break;
				}
				else {
				/*Update the list of head if the head node is deleted*/
					if (ack==1) {
						nxt_node->next->timeout += nxt_node->timeout;
					}
					nxt_node->next->prev = NULL;
				}
			}
			/* delete tail */
			else if (nxt_node->next == NULL) {
				list->tail = prev_node;
				prev_node->next = NULL;
			}
			/* delete a node in the middle */
			else {
				if (ack==1) {
					nxt_node->next->timeout += nxt_node->timeout;
				}
				prev_node->next = nxt_node->next;
				nxt_node->next->prev = prev_node;
			}
			remove_tnode(nxt_node);
			list->numtnodes--;
			printf("Number of timer nodes: %d\n",list->numtnodes);
			break;
		}
		prev_node = nxt_node;
		nxt_node = nxt_node->next;
	}
	return ispresent;
}

void tlist_clock(tlist *list, double decrement_value) {
	tnode *nxt_node, *temp;
	int insideWhile = 0;
	struct sockaddr_in tcpd_client_addr;
	
	//  struct hostent *hp, *gethostbyname();
	unsigned int sockout;	

	tcpd_client_addr.sin_family = AF_INET;           	     
	tcpd_client_addr.sin_port = htons(TCPD_TIMER_PORT);
	tcpd_client_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	/*list empty*/
	if (list->head == NULL) {
		printf("Delta list is empty\n");
	} 
	else {
		/*update the time in the list*/
		nxt_node = list->head;
		nxt_node->timeout -= decrement_value;		// decrement timeout by decrement_value
		printf("timeout value: %lf\n",nxt_node->timeout);

		/*Check for nodes with timeout value less than zero*/
		while(nxt_node != NULL && nxt_node->timeout <=0) {
			printf("Inside while in tlist_clock\n"); 
			insideWhile = 1;

			/* Inform client the packet's seqnum for which timeout has occured*/
			sockout = socket(AF_INET, SOCK_DGRAM, 0); 	
			if(sendto(sockout, &(nxt_node->seqnum), sizeof(nxt_node->seqnum), 0, (struct sockaddr*)&tcpd_client_addr, sizeof(tcpd_client_addr)) < 0) {
				printf("Error: Unable to send timeout event to tcpd_client\n");
			}
			else {
				printf("\nMessage sent to tcpd_client to retransmit packet no %d\n",nxt_node->seqnum);
			}
			/*delete the nodes for which timeout has occurred*/
			temp = nxt_node;
			nxt_node = nxt_node->next;
			if(removeFromtlist(list, temp, 0)) {		// 3rd argument is 0 because of timeout event
				printf ("Timer node deleted from delta list\n");
			}
			else {
				printf("Failed to delete timer node from delta list\n");
			}
		}
		if (insideWhile && nxt_node != temp) 
		list -> head = nxt_node;
	}
}
