#include "../../sr_module.h"    /* param_export_t, proc_export_t */
#include "../../script_var.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "server_group.h"
#include <time.h> 
#include "mod_curl.h"

int max_response_size = 0;
int connection_timeout = 0;

/* command */
static int w_get_f(struct sip_msg* msg, char* url);
/* parse pseudo variable parameter */
static int param_fixup_get(void** param, int param_no);
/* CURL callback */
// static size_t curl_receive_chunk(void *ptr, size_t size, size_t nmemb, struct string *response);
/* CURL send get */
static int curl_send_get(char *url);

int add_host(modparam_t type, void *val);

/* exported parameters */
static param_export_t params[] = {
	{"max_response_size", INT_PARAM , &max_response_size},
	{"connection_timeout", INT_PARAM , &connection_timeout},
	{"add_host", STR_PARAM|USE_FUNC_PARAM, (void *)&add_host},
	{ 0,0,0 }
};

/* Extra process */
static proc_export_t mod_procs[] = {
        {0,0,0,0,0,0}
};

/* Module exported function */
static cmd_export_t cmds[]={
        {"curl_get",(cmd_function)w_get_f,1,param_fixup_get,0,REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
   	{0,0,0,0,0,0}
};

/* Server Group */
struct server_group {
	str host;
	int status;	
	time_t last_error;
	struct server_group *next;
};

int add_host(modparam_t type, void *val)
{
	char *param=(char*)val;
	if(!param)
		return E_UNSPEC;
	if(STR_PARAM & (type != STR_PARAM)){
		LM_ERR("Expected string type parameter: hostname or IP addresse.\n");
		return E_CFG;
        }

	time_t t;  
    	time(&t);  
    	printf ("Nombre de seconde écoulée depuis l'Epoch : %d\n", (int)t); 
 
	add_server(val);
	return 0;
}

struct module_exports exports = {
        "curl",                     /* module's name */
        MODULE_VERSION,             /* module version */
        DEFAULT_DLFLAGS,            /* dlopen flags */
        cmds,                       /* exported functions */
        params,                     /* module parameters */
        0,                          /* exported statistics */
        0,                          /* exported MI functions */
        0,                          /* exported pseudo-variables */
        mod_procs,                  /* extra processes */
        curl_mod_init,              /* module initialization function */
        0,                          /* response function*/
        0,                          /* destroy function */
        child_init                  /* per-child init function */
};

static int curl_mod_init(void){
 	LM_DBG("curl init succeded\n");


	time_t t;  
    	time(&t);  
    	printf ("Nombre de seconde écoulée depuis l'Epoch : %d\n", (int)t); 

	show_server_list();

        return 0;
}

static int child_init(int rank){
    	return 0;
}

struct string {
	char *ptr;
	size_t len;
};

// CURL function pointer - callback function
static size_t curl_receive_chunk(void *ptr, size_t size, size_t nmemb, void *response){

 	struct string * buff = (struct string*) response;
	int chunk_size = nmemb*size;
	int new_size = buff->len + chunk_size;
	if(new_size >= max_response_size && max_response_size > 0){
		new_size = max_response_size;
		chunk_size = max_response_size - buff->len;
	}

	buff->ptr = (char*) pkg_realloc(buff->ptr, new_size + 1);	
        if (buff->ptr == NULL) {
                LM_ERR("no pkg memory left\n");
                return -1;
	}
	memcpy(buff->ptr + buff->len, ptr, chunk_size);
	buff->len = new_size;
	buff->ptr[buff->len] = '\0';
	

	return nmemb*size; 
}

static int curl_send_get(char *url){
	
	struct string response;
	response.len = 0;
	response.ptr = (char*) pkg_malloc(1);
        if (response.ptr == NULL) {
        	LM_ERR("no pkg memory left\n");
        	return -1;
	}
        response.ptr[0] = '\0';
	char curl_error[CURL_ERROR_SIZE + 1]; curl_error[CURL_ERROR_SIZE] = '\0';
 	CURL *curl_handle;
 	curl_global_init(CURL_GLOBAL_ALL);
 	curl_handle = curl_easy_init();
	if(curl_handle){
 		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
 		curl_easy_setopt(curl_handle, CURLOPT_HEADER , 0);
 		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
		if(connection_timeout > 0)
 			curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, connection_timeout);
 		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_receive_chunk);
 		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);
 		curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, curl_error);
 		if(curl_easy_perform(curl_handle) != 0){
			LM_ERR("%s\n",curl_error);
		}
 		curl_easy_cleanup(curl_handle);
	}

	// add result to an Opensips var
 	int flags = VAR_VAL_STR;
        int_str vname;
        vname.s.len = 8;
        vname.s.s = "curl_res";
        
	
        int_str val;
	val.s.len = response.len;
	val.s.s = (char*) response.ptr;         
	script_var_t* curl_res_var = add_var(&vname.s);
        if(!set_var_value(curl_res_var, &val, flags)){
        	LM_ERR("cannot set svar\n");
		pkg_free(response.ptr);
                return -1;
        }
	
        LM_INFO("max:%d size:%d timeout:%d\n",max_response_size,response.len,connection_timeout);
	pkg_free(response.ptr);
	return 1;
}

static int w_get_f(struct sip_msg *msg, char *url)
{
	pv_spec_t *sp;
	sp = (pv_spec_t *)url;
	pv_value_t pv_val;

        if (sp && (pv_get_spec_value(msg, sp, &pv_val) == 0)) {
            if (!(pv_val.flags & PV_VAL_STR)) {
                LM_ERR("curl: script variable value is not string\n");
                return -1;
            }
        } else {
            LM_ERR("curl: cannot get script variable value\n");
            return -1;
        }

	if (curl_send_get(pv_val.rs.s) < 0){
		return -1;
	}
	return 1;
}

static int param_fixup_get(void** param, int param_no)
{
        pv_spec_t *sp;
        str s;

	if (param_no > 1) {
		LM_ERR("get function as only 1 parameter !\n");
                return -1;
	}

	if (param_no == 1) { /* pseudo variable */

            sp = (pv_spec_t*)pkg_malloc(sizeof(pv_spec_t));
            if (sp == 0) {
                LM_ERR("no pkg memory left\n");
                return -1;
            }
            s.s = (char*)*param; s.len = strlen(s.s);
            if (pv_parse_spec(&s, sp) == 0) {
                LM_ERR("parsing of pseudo variable %s failed!\n", (char*)*param);
                pkg_free(sp);
                return -1;
            }

            if (sp->type == PVT_NULL) {
                LM_ERR("bad pseudo variable\n");
                pkg_free(sp);
                return -1;
            }

	   // LM_INFO("parameter:%s:%p\n", (char*)*param,(void);
            *param = (void*)sp;
            return 0;
        }

        *param = (void *)0;
        return 0;
}

