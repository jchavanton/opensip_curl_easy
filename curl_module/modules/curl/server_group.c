
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"

#ifndef NULL
#define NULL ((void *)0)
#endif  /* NULL */

/* Server Group */
struct server_group {
        str host;
        int status;
	int last_error;
        struct server_group *next;
};

/* Shared server group */
struct server_group *hosts = NULL; 
struct server_group *head = NULL; 
int host_count = 0;

int add_server(char * hostname){

	/* adding host to a linked list */	
	if(head == NULL){
		head = (struct server_group *) pkg_malloc(sizeof(struct server_group));
		hosts = head;
	}
	else{
		LM_INFO("searching the last host \n");
		hosts = head;
		while(hosts->next != NULL){
			LM_INFO("another host found...\n");
			hosts = hosts->next; 
		}
		LM_INFO("adding host at the end of the linked list\n");
		hosts->next = (struct server_group *) pkg_malloc(sizeof(struct server_group));
		hosts = hosts->next;
	}
	hosts->next = NULL;
	hosts->last_error = 0;
	hosts->host.len = strlen(hostname);
	hosts->host.s = (char*) pkg_malloc(hosts->host.len+1);
	memcpy(hosts->host.s,hostname,hosts->host.len);
	hosts->host.s[hosts->host.len]='\0';
	hosts->status = 0;

	host_count++;	
	LM_INFO("adding hosts[%d][%d][%s]\n",host_count,hosts->host.len,hosts->host.s);
	return 0;

}

int show_server_list(void){
	hosts = head;
	while(hosts != NULL){
		LM_INFO("hosts[%d][%s]\n",hosts->host.len,hosts->host.s);
		hosts = hosts->next;
	}
	return 0;
}
