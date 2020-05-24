/*
 * Linux HA management library
 *
 * Author: Huang Zhen <zhenhltc@cn.ibm.com>
 * Copyright (c) 2005 International Business Machines
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <unistd.h>
#include <glib.h>
#include <regex.h>
#include <dirent.h>
#include <sys/wait.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>

#if HAVE_HB_CONFIG_H
#include <hb_config.h>
#endif

#if HAVE_GLUE_CONFIG_H
#include <glue_config.h>
#endif

#include <clplumbing/cl_log.h>
#include <clplumbing/cl_syslog.h>
#include <clplumbing/lsb_exitcodes.h>

#include <crm/cib.h>
#include <crm/msg_xml.h>
#include <crm/pengine/status.h>

/*neoshineha ming.liu add*/
#include <sys/utsname.h>
#include <errno.h>
#include <corosync/cfg.h>
#include <corosync/totem/totempg.h>
#include <stdlib.h>
#include <pwd_encrypt.h>
#include <sqlite3.h>
#include <mgmt/mgmt_client.h>

/*ssh use*/
/*#include "mgmt/libssh2_config.h"*/
#include <libssh2.h>

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
/*neoshineha ming.liu add end*/
/* Jiaming Liu */
#include <sys/stat.h>
#include <halicense.h>
/* Done */

/*
#ifdef SUPPORT_COROSYNC
#undef SUPPORT_COROSYNC
#endif
*/

#ifdef SUPPORT_HEARTBEAT
#undef SUPPORT_HEARTBEAT
#endif

#include <pygui_internal.h>
#if HAVE_PACEMAKER_CRM_COMMON_CLUSTER_H
#  include <crm/common/cluster.h>
#endif
#if HAVE_PACEMAKER_CRM_CLUSTER_H
#  include <crm/cluster.h>
#endif
#if HAVE_PACEMAKER_CRM_SERVICES_H
#  include <crm/services.h>
#endif

#include "mgmt_internal.h"

#if !HAVE_CRM_DATA_T
typedef xmlNode crm_data_t;
#endif

#if !HAVE_CRM_IPC_NEW
typedef IPC_Channel crm_ipc_t;
#endif

extern resource_t *group_find_child(resource_t *rsc, const char *id);
/*extern crm_data_t * do_calculations(
	pe_working_set_t *data_set, crm_data_t *xml_input, ha_time_t *now);*/

extern int *client_id;

cib_t*	cib_conn = NULL;
int in_shutdown = FALSE;
int init_crm(int cache_cib);
void final_crm(void);
int set_crm(void);

static GHashTable* cib_conns = NULL;
static GHashTable* cib_envs = NULL;

static void on_cib_diff(const char *event, crm_data_t *msg);

static char* on_active_cib(char* argv[], int argc);
static char* on_shutdown_cib(char* argv[], int argc);
static char* on_init_cib(char* argv[], int argc);
static char* on_switch_cib(char* argv[], int argc);
static char* on_get_shadows(char* argv[], int argc);
static char* on_crm_shadow(char* argv[], int argc);

static char* on_get_cluster_type(char* argv[], int argc);
static char* on_get_cib_version(char* argv[], int argc);
static char* on_get_crm_schema(char* argv[], int argc);
static char* on_get_crm_dtd(char* argv[], int argc);

static char* on_get_crm_metadata(char* argv[], int argc);
static char* on_crm_attribute(char* argv[], int argc);
static char* on_get_activenodes(char* argv[], int argc);
static char* on_get_crmnodes(char* argv[], int argc);
static char* on_get_dc(char* argv[], int argc);
/* by Liu Jiaming */
static char * on_lic_verid(char * argv[], int argc);
static char * on_lic_expire(char * argv[], int argc);
/*Done*/
static char* on_migrate_rsc(char* argv[], int argc);
static char* on_set_node_standby(char* argv[], int argc);
static char* on_get_node_config(char* argv[], int argc);
static char* on_get_running_rsc(char* argv[], int argc);

static char* on_cleanup_rsc(char* argv[], int argc);

static char* on_get_all_rsc(char* argv[], int argc);
static char* on_get_rsc_type(char* argv[], int argc);
static char* on_get_sub_rsc(char* argv[], int argc);
static char* on_get_rsc_running_on(char* argv[], int argc);
static char* on_get_rsc_status(char* argv[], int argc);
static char* on_op_status2str(char* argv[], int argc);

static char* on_crm_rsc_cmd(char* argv[], int argc);
static char* on_set_rsc_attr(char* argv[], int argc);
static char* on_get_rsc_attr(char* argv[], int argc);
static char* on_del_rsc_attr(char* argv[], int argc);

/*neoshineha ming.liu add*/
static char* on_get_rsc_constraints(char* argv[], int argc);
static char* on_system(char* argv[], int argc);
ssize_t readline(int fd, void *vptr, size_t maxlen);
static char* on_get_file_context(char* argv[], int argc);
static char* on_save_file_context(char* argv[], int argc);
static char* on_status_of_hblinks(char* argv[], int argc);
static char* on_get_coro_config(char* argv[], int argc);
static char* on_get_localnode(char* argv[], int argc);
static char* on_pwd_decode(char* argv[], int argc);
static char* on_pwd_encode(char* argv[], int argc);
static char* on_get_totem(char* argv[], int argc);
static char* on_get_sqldata(char* argv[], int argc);
static char* on_update_sqldata(char* argv[], int argc);
static char* on_ssh_node(char* argv[], int argc);
/*neoshineha ming.liu add end*/

/* new CRUD protocol */
static char* on_cib_create(char* argv[], int argc);
static char* on_cib_query(char* argv[], int argc);
static char* on_cib_update(char* argv[], int argc);
static char* on_cib_replace(char* argv[], int argc);
static char* on_cib_delete(char* argv[], int argc);
/* end new protocol */

static char* on_gen_cluster_report(char* argv[], int argc);
static char* on_get_pe_inputs(char* argv[], int argc);
static char* on_get_pe_summary(char* argv[], int argc);
static char* on_gen_pe_graph(char* argv[], int argc);
static char* on_gen_pe_info(char* argv[], int argc);

/*
static int delete_object(const char* type, const char* entry, const char* id, crm_data_t** output);
static GList* find_xml_node_list(crm_data_t *root, const char *search_path);
*/
static int refresh_lrm(crm_ipc_t *crmd_channel, const char *host_uname);
static int delete_lrm_rsc(crm_ipc_t *crmd_channel, const char *host_uname, const char *rsc_id);
static pe_working_set_t* get_data_set(void);
static void free_data_set(pe_working_set_t* data_set);
static void on_cib_connection_destroy(gpointer user_data);
static char* crm_failed_msg(crm_data_t* output, int rc);
static const char* uname2id(const char* node);
/*
static resource_t* get_parent(resource_t* child);
*/
int regex_match(const char *regex, const char *str);
pid_t popen2(const char *command, FILE **fp_in, FILE **fp_out);
int pclose2(FILE *fp_in, FILE *fp_out, pid_t pid);

pe_working_set_t* cib_cached = NULL;
int cib_cache_enable = FALSE;
#if !HAVE_PCMK_STRERROR
#  define pcmk_strerror(rc) cib_error2string(rc)
#endif

#if !HAVE_DECL_CRM_CONCAT
static char *
crm_concat(const char *prefix, const char *suffix, char join)
{
    int len = 0;
    char *new_str = NULL;

    CRM_ASSERT(prefix != NULL);
    CRM_ASSERT(suffix != NULL);
    len = strlen(prefix) + strlen(suffix) + 2;

    new_str = calloc(1, (len));
    sprintf(new_str, "%s%c%s", prefix, join, suffix);
    new_str[len - 1] = 0;
    return new_str;
}
#endif

#if !HAVE_DECL_CRM_INT_HELPER
static long long
crm_int_helper(const char *text, char **end_text)
{
    long long result = -1;
    char *local_end_text = NULL;
    int saved_errno = 0;

    errno = 0;

    if (text != NULL) {
#ifdef ANSI_ONLY
        if (end_text != NULL) {
            result = strtol(text, end_text, 10);
        } else {
            result = strtol(text, &local_end_text, 10);
        }
#else
        if (end_text != NULL) {
            result = strtoll(text, end_text, 10);
        } else {
            result = strtoll(text, &local_end_text, 10);
        }
#endif

        saved_errno = errno;
/*             CRM_CHECK(errno != EINVAL); */
        if (errno == EINVAL) {
            crm_err("Conversion of %s failed", text);
            result = -1;

        } else if (errno == ERANGE) {
            crm_err("Conversion of %s was clipped: %lld", text, result);

        } else if (errno != 0) {
            crm_perror(LOG_ERR, "Conversion of %s failed:", text);
        }

        if (local_end_text != NULL && local_end_text[0] != '\0') {
            crm_err("Characters left over after parsing '%s': '%s'", text, local_end_text);
        }

        errno = saved_errno;
    }
    return result;
}
#endif

#if !HAVE_DECL_GENERATE_SERIES_FILENAME
static char *
generate_series_filename(const char *directory, const char *series, int sequence, gboolean bzip)
{
    int len = 40;
    char *filename = NULL;
    const char *ext = "raw";

    CRM_CHECK(directory != NULL, return NULL);
    CRM_CHECK(series != NULL, return NULL);

    len += strlen(directory);
    len += strlen(series);
    filename = calloc(1, len);
    CRM_CHECK(filename != NULL, return NULL);

    if (bzip) {
        ext = "bz2";
    }
    sprintf(filename, "%s/%s-%d.%s", directory, series, sequence, ext);

    return filename;
}
#endif

#if !HAVE_DECL_GET_LAST_SEQUENCE
static int
get_last_sequence(const char *directory, const char *series)
{
    FILE *file_strm = NULL;
    int start = 0, length = 0, read_len = 0;
    char *series_file = NULL;
    char *buffer = NULL;
    int seq = 0;
    int len = 36;

    CRM_CHECK(directory != NULL, return 0);
    CRM_CHECK(series != NULL, return 0);

    len += strlen(directory);
    len += strlen(series);
    series_file = calloc(1, len);
    CRM_CHECK(series_file != NULL, return 0);
    sprintf(series_file, "%s/%s.last", directory, series);

    file_strm = fopen(series_file, "r");
    if (file_strm == NULL) {
        crm_debug("Series file %s does not exist", series_file);
        free(series_file);
        return 0;
    }

    /* see how big the file is */
    start = ftell(file_strm);
    fseek(file_strm, 0L, SEEK_END);
    length = ftell(file_strm);
    fseek(file_strm, 0L, start);

    CRM_ASSERT(length >= 0);
    CRM_ASSERT(start == ftell(file_strm));

    if (length <= 0) {
        crm_info("%s was not valid", series_file);
        free(buffer);
        buffer = NULL;

    } else {
        crm_trace("Reading %d bytes from file", length);
        buffer = calloc(1, (length + 1));
        read_len = fread(buffer, 1, length, file_strm);
        if (read_len != length) {
            crm_err("Calculated and read bytes differ: %d vs. %d", length, read_len);
            free(buffer);
            buffer = NULL;
        }
    }

    seq = crm_parse_int(buffer, "0");
    fclose(file_strm);

    crm_trace("Found %d in %s", seq, series_file);

    free(series_file);
    free(buffer);
    return seq;
}
#endif

#define CIB_CHECK() \
	if (cib_conn == NULL) { \
		mgmt_log(LOG_ERR, "No cib connection: client_id=%d", *client_id); \
		return strdup(MSG_FAIL"\nNo cib connection"); \
	}

#define GET_CIB_NAME(cib_name) \
	if (getenv("CIB_shadow") == NULL) { \
		strncpy(cib_name, "live", sizeof(cib_name)-1); \
	} else { \
		snprintf(cib_name, sizeof(cib_name),"shadow.%s", getenv("CIB_shadow")); \
	} 

#define GET_RESOURCE()	rsc = pe_find_resource(data_set->resources, argv[1]);	\
	if (rsc == NULL) {						\
		char *as_clone = crm_concat(argv[1], "0", ':');		\
		rsc = pe_find_resource(data_set->resources, as_clone);	\
		free(as_clone);					\
		if (rsc == NULL) {					\
			free_data_set(data_set);			\
			return strdup(MSG_FAIL"\nno such resource");	\
		}							\
	}

#define free_cib_cached()					\
	if (cib_cache_enable) {					\
		if (cib_cached != NULL) {			\
			cleanup_calculations(cib_cached);	\
			free(cib_cached);			\
			cib_cached = NULL;			\
		}						\
	}

#define append_str(msg, buf, str)				\
	if (strlen(buf)+strlen(str) >= sizeof(buf)) {		\
		msg = mgmt_msg_append(msg, buf);		\
		memset(buf, 0, sizeof(buf));			\
	}							\
	strncat(buf, str, sizeof(buf)-strlen(buf)-1);

#define gen_msg_from_fstream(fstream, msg, buf, str)		\
	memset(buf, 0, sizeof(buf));				\
	while (!feof(fstream)){					\
		if (fgets(str, sizeof(str), fstream) != NULL){	\
			append_str(msg, buf, str);		\
		}						\
		else{						\
			sleep(1);				\
		}						\
	}							\
	msg = mgmt_msg_append(msg, buf);			\
	if (msg[strlen(msg)-1] == '\n'){			\
		msg[strlen(msg)-1] = '\0';			\
	}
	

/* internal functions */
/*
GList* find_xml_node_list(crm_data_t *root, const char *child_name)
{
	GList* list = NULL;
	xml_child_iter_filter(root, child, child_name,
			      list = g_list_append(list, child));
	return list;
}

int
delete_object(const char* type, const char* entry, const char* id, crm_data_t** output) 
{
	int rc;
	crm_data_t* cib_object = NULL;
	char xml[MAX_STRLEN];

	snprintf(xml, MAX_STRLEN, "<%s id=\"%s\">", entry, id);

	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return -1;
	}
	
	mgmt_log(LOG_INFO, "(delete)xml:%s",xml);

	rc = cib_conn->cmds->delete(
			cib_conn, type, cib_object, cib_sync_call);
	free_xml(cib_object);
	return rc;
}
*/

pe_working_set_t*
get_data_set(void) 
{
	pe_working_set_t* data_set;
	xmlNode *current_cib = NULL;

	
	if (cib_cache_enable) {
		if (cib_cached != NULL) {
			return cib_cached;
		}
	}
	
	data_set = (pe_working_set_t*)malloc(sizeof(pe_working_set_t));
	if (data_set == NULL) {
		mgmt_log(LOG_ERR, "%s:Can't alloc memory for data set.",__FUNCTION__);
		return NULL;
	}
	set_working_set_defaults(data_set);
        cib_conn->cmds->query(cib_conn, NULL, &current_cib, cib_scope_local | cib_sync_call);  
        data_set->input = current_cib;

#if HAVE_NEW_HA_DATE
	data_set->now = new_ha_date(TRUE);
#else
	data_set->now = NULL;
#endif

	cluster_status(data_set);
	
	if (cib_cache_enable) {
		cib_cached = data_set;
	}
	return data_set;
}

void 
free_data_set(pe_working_set_t* data_set)
{
	/* we only release the cib when cib is not cached.
	   the cached cib will be released in on_cib_diff() */
	if (!cib_cache_enable) {
		cleanup_calculations(data_set);
		free(data_set);
	}
}	

char* 
crm_failed_msg(crm_data_t* output, int rc) 
{
	const char* reason = NULL;
	crm_data_t* failed_tag;
	char* ret;
	
	/* beekhof:
		you can pretend that the return code is success, 
		its an internal CIB thing*/
#if !HAVE_PCMK_STRERROR
	if (rc == cib_diff_resync) {
#else
        if (rc == -pcmk_err_diff_resync) {
#endif
		if (output != NULL) {
			free_xml(output);
		}
		return strdup(MSG_OK);
	}
	
	ret = strdup(MSG_FAIL);
	ret = mgmt_msg_append(ret, pcmk_strerror(rc));	

	if (output == NULL) {
		return ret;
	}
	
	failed_tag = find_entity(output, XML_FAIL_TAG_CIB, NULL);
	if (failed_tag != NULL) {
		reason = crm_element_value(failed_tag, XML_FAILCIB_ATTR_REASON);
		if (reason != NULL) {
			ret = mgmt_msg_append(ret, reason);
		}
	}
	free_xml(output);
	
	return ret;
}
const char*
uname2id(const char* uname)
{
	node_t* node;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (strncmp(uname,node->details->uname,MAX_STRLEN) == 0) {
			free_data_set(data_set);
			return node->details->id;
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return NULL;
}
/*
static resource_t* 
get_parent(resource_t* child)
{
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->resources;
	while (cur != NULL) {
		resource_t* rsc = (resource_t*)cur->data;
		if(is_not_set(rsc->flags, pe_rsc_orphan) || rsc->role != RSC_ROLE_STOPPED) {
			GList* child_list = rsc->children;
			if (g_list_find(child_list, child) != NULL) {
				free_data_set(data_set);
				return rsc;
			}
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return NULL;
}
*/

/* mgmtd functions */
int
init_crm(int cache_cib)
{
	int ret = 0;
	int i, max_try = 5;
	char cib_name[MAX_STRLEN];

	GET_CIB_NAME(cib_name)

	mgmt_log(LOG_INFO,"init_crm: client_id=%d cib_name=%s", *client_id, cib_name);
        crm_xml_init();
	crm_log_level = LOG_ERR;
	cib_conn = cib_new();
	in_shutdown = FALSE;
	
	cib_cache_enable = cache_cib?TRUE:FALSE;
	cib_cached = NULL;
	
	for (i = 1; i <= max_try ; i++) {
		ret = cib_conn->cmds->signon(cib_conn, client_name, cib_command);
		if (ret == 0) {
			break;
		}
		mgmt_log(LOG_INFO,"login to cib '%s': %d, ret=%d (%s)", cib_name, i, ret, pcmk_strerror(ret));
		sleep(1);
	}
	if (ret != 0) {
		mgmt_log(LOG_INFO,"login to cib '%s' failed: %s", cib_name, pcmk_strerror(ret));
		cib_conn = NULL;
		return ret;
	}

	ret = cib_conn->cmds->add_notify_callback(cib_conn, T_CIB_DIFF_NOTIFY
						  , on_cib_diff);
	ret = cib_conn->cmds->set_connection_dnotify(cib_conn
			, on_cib_connection_destroy);

	reg_msg(MSG_ACTIVE_CIB, on_active_cib);
	reg_msg(MSG_SHUTDOWN_CIB, on_shutdown_cib);
	reg_msg(MSG_INIT_CIB, on_init_cib);
	reg_msg(MSG_SWITCH_CIB, on_switch_cib);
	reg_msg(MSG_GET_SHADOWS, on_get_shadows);
	reg_msg(MSG_CRM_SHADOW, on_crm_shadow);

	reg_msg(MSG_CLUSTER_TYPE, on_get_cluster_type);
	reg_msg(MSG_CIB_VERSION, on_get_cib_version);
	reg_msg(MSG_CRM_SCHEMA, on_get_crm_schema);
	reg_msg(MSG_CRM_DTD, on_get_crm_dtd);
	reg_msg(MSG_CRM_METADATA, on_get_crm_metadata);
	reg_msg(MSG_CRM_ATTRIBUTE, on_crm_attribute);
	
	reg_msg(MSG_DC, on_get_dc);
	reg_msg(MSG_ACTIVENODES, on_get_activenodes);
	reg_msg(MSG_CRMNODES, on_get_crmnodes);
	reg_msg(MSG_NODE_CONFIG, on_get_node_config);
	reg_msg(MSG_RUNNING_RSC, on_get_running_rsc);

	reg_msg(MSG_MIGRATE, on_migrate_rsc);
	reg_msg(MSG_STANDBY, on_set_node_standby);
	
	reg_msg(MSG_CLEANUP_RSC, on_cleanup_rsc);
	
	reg_msg(MSG_ALL_RSC, on_get_all_rsc);
	reg_msg(MSG_SUB_RSC, on_get_sub_rsc);
	reg_msg(MSG_RSC_RUNNING_ON, on_get_rsc_running_on);
	reg_msg(MSG_RSC_STATUS, on_get_rsc_status);
	reg_msg(MSG_RSC_TYPE, on_get_rsc_type);
	reg_msg(MSG_OP_STATUS2STR, on_op_status2str);

	reg_msg(MSG_CRM_RSC_CMD, on_crm_rsc_cmd);
	reg_msg(MSG_SET_RSC_ATTR, on_set_rsc_attr);
	reg_msg(MSG_GET_RSC_ATTR, on_get_rsc_attr);
	reg_msg(MSG_DEL_RSC_ATTR, on_del_rsc_attr);
		
        /*neoshineha ming.liu add*/
        reg_msg(MSG_GET_RSC_CONST, on_get_rsc_constraints);
        reg_msg(MSG_SYSTEM, on_system);
        reg_msg(MSG_GET_FILE_CONTEXT, on_get_file_context);
        reg_msg(MSG_SAVE_FILE_CONTEXT, on_save_file_context);
        reg_msg(MSG_STATUS_HBLINKS, on_status_of_hblinks);
	reg_msg(MSG_GET_CORO_CONFIG, on_get_coro_config);
	reg_msg(MSG_GET_LOCALNODE, on_get_localnode);
	reg_msg(MSG_PWD_DECODE, on_pwd_decode);
	reg_msg(MSG_PWD_ENCODE, on_pwd_encode);
	reg_msg(MSG_GET_TOTEM, on_get_totem);
	reg_msg(MSG_GET_SQLDATA, on_get_sqldata);
	reg_msg(MSG_UPDATE_SQLDATA, on_update_sqldata);
	reg_msg(MSG_SSH_NODE, on_ssh_node);
        /*neoshineha ming.liu add end*/
    /* Liu Jiaming */
    reg_msg(MSG_LIC_VERID, on_lic_verid);
    reg_msg(MSG_LIC_EXPIRE, on_lic_expire);
    /* Done */
	reg_msg(MSG_GEN_CLUSTER_REPORT, on_gen_cluster_report);
	reg_msg(MSG_GET_PE_INPUTS, on_get_pe_inputs);
	reg_msg(MSG_GET_PE_SUMMARY, on_get_pe_summary);
	reg_msg(MSG_GEN_PE_GRAPH, on_gen_pe_graph);
	reg_msg(MSG_GEN_PE_INFO, on_gen_pe_info);
	
	reg_msg(MSG_CIB_CREATE, on_cib_create);
	reg_msg(MSG_CIB_QUERY, on_cib_query);
	reg_msg(MSG_CIB_UPDATE, on_cib_update);
	reg_msg(MSG_CIB_REPLACE, on_cib_replace);
	reg_msg(MSG_CIB_DELETE, on_cib_delete);

	if (cib_conns == NULL) {
		cib_conns = g_hash_table_new(g_int_hash, g_int_equal);
	}
	g_hash_table_insert(cib_conns, client_id, cib_conn);

	if (cib_envs == NULL) {
		cib_envs = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
	}
	g_hash_table_insert(cib_envs, cib_conn, getenv("CIB_shadow")?strdup(getenv("CIB_shadow")):NULL);

	return 0;
}	
void
final_crm(void)
{
	char cib_name[MAX_STRLEN];
	
	GET_CIB_NAME(cib_name)

	mgmt_log(LOG_INFO,"final_crm: client_id=%d cib_name=%s", *client_id, cib_name);
	if(cib_conn != NULL) {
		in_shutdown = TRUE;
		cib_conn->cmds->signoff(cib_conn);
		cib_delete(cib_conn);

		g_hash_table_remove(cib_conns, client_id);
		g_hash_table_remove(cib_envs, cib_conn);
		cib_conn = NULL;

		if (g_hash_table_size(cib_conns) == 0) {
			g_hash_table_destroy(cib_conns);
			cib_conns = NULL;
		}

		if (g_hash_table_size(cib_envs) == 0) {
			g_hash_table_destroy(cib_envs);
			cib_envs = NULL;
		}
	}

	free_cib_cached();
}

int
set_crm(void)
{
	free_cib_cached();

	cib_conn = g_hash_table_lookup(cib_conns, client_id);
	if (cib_conn != NULL) {
		const char *env = g_hash_table_lookup(cib_envs, cib_conn);
		if (env == NULL) {
			unsetenv("CIB_shadow");
		} else {
			setenv("CIB_shadow", env, 1);
		}
		mgmt_debug(LOG_DEBUG, "set_crm: client_id=%d cib_conn=%p cib_name=%s", *client_id, cib_conn, env);

		return 0;
	}

	mgmt_log(LOG_WARNING, "No cib connection recorded for set_crm: client_id=%d", *client_id);
	return -1;
}

/* event handler */
void
on_cib_diff(const char *event, crm_data_t *msg)
{
	if (debug_level) {
		mgmt_debug(LOG_DEBUG,"update cib finished");
	}

	free_cib_cached();
	
	fire_event(EVT_CIB_CHANGED);
}

void
on_cib_connection_destroy(gpointer user_data)
{
	if (!in_shutdown) {
		fire_event(EVT_DISCONNECTED);
		cib_conn = NULL;
	}
	return;
}

char*
on_active_cib(char* argv[], int argc)
{
	char* ret = strdup(MSG_OK);
	const char *active_cib = getenv("CIB_shadow");

	if (active_cib != NULL) {
		ret = mgmt_msg_append(ret, active_cib);
	} else {
		ret = mgmt_msg_append(ret, "");
	}
	return ret;
}

char*
on_shutdown_cib(char* argv[], int argc)
{
	final_crm();
	return strdup(MSG_OK);
}

char*
on_init_cib(char* argv[], int argc)
{
	char buf[MAX_STRLEN];
	char* ret = NULL;
	char cib_name[MAX_STRLEN];
	int rc = 0;

	ARGC_CHECK(2);

	if (strnlen(argv[1], MAX_STRLEN) == 0) {
		unsetenv("CIB_shadow");
		strncpy(cib_name, "live", sizeof(cib_name)-1);
	} else {
		setenv("CIB_shadow", argv[1], 1);
		snprintf(cib_name, sizeof(cib_name), "shadow.%s", argv[1]);
	}

	if ((rc = init_crm(TRUE)) != 0) {
		ret = strdup(MSG_FAIL);
		snprintf(buf, sizeof(buf), "Cannot initiate CIB '%s': %s", cib_name, pcmk_strerror(rc));
		ret = mgmt_msg_append(ret, buf);
	} else {
		ret = strdup(MSG_OK);
	}

	return ret;
}

char*
on_switch_cib(char* argv[], int argc)
{
	char cib_name[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char* ret = NULL;
	const char* saved_env = getenv("CIB_shadow");
	int rc = 0;

	ARGC_CHECK(2)

	final_crm();

	if (strnlen(argv[1], MAX_STRLEN) == 0) {
		unsetenv("CIB_shadow");
		strncpy(cib_name, "live", sizeof(cib_name)-1);
	} else {
		setenv("CIB_shadow", argv[1], 1);
		snprintf(cib_name, sizeof(cib_name), "shadow.%s", argv[1]);
	}

	mgmt_log(LOG_INFO, "Switch to the CIB '%s'", cib_name);

	if ((rc = init_crm(TRUE)) != 0) {
		mgmt_log(LOG_ERR, "Cannot switch to the CIB '%s': %s", cib_name, pcmk_strerror(rc));
		ret = strdup(MSG_FAIL);
		snprintf(buf, sizeof(buf), "Cannot switch to the CIB '%s': %s", cib_name, pcmk_strerror(rc));
		ret = mgmt_msg_append(ret, buf);

		if (saved_env == NULL) {
			unsetenv("CIB_shadow");
			strncpy(cib_name, "live", sizeof(cib_name)-1);
		} else {
			setenv("CIB_shadow", saved_env, 1);
			snprintf(cib_name, sizeof(cib_name), "shadow.%s", saved_env);
		}
		if ((rc = init_crm(TRUE)) != 0) {
			mgmt_log(LOG_ERR, "Cannot switch back to the previous CIB '%s': %s", cib_name, pcmk_strerror(rc));
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "Cannot switch back to the previous CIB '%s': %s", cib_name, pcmk_strerror(rc));
			ret = mgmt_msg_append(ret, buf);

			if (saved_env) {
				unsetenv("CIB_shadow");
				strncpy(cib_name, "live", sizeof(cib_name)-1);

				if ((rc = init_crm(TRUE)) != 0) {
					mgmt_log(LOG_ERR, "Cannot recover to the CIB '%s': %s", cib_name, pcmk_strerror(rc));
					memset(buf, 0, sizeof(buf));
					snprintf(buf, sizeof(buf), "Cannot recover to the CIB '%s': %s", cib_name, pcmk_strerror(rc));
					ret = mgmt_msg_append(ret, buf);
				}
			}
		}
	} else {
		ret = strdup(MSG_OK);
	}

	return ret;
}

char*
on_get_shadows(char* argv[], int argc)
{
	char *ret = NULL;
	char *test_file = NULL;
	const char *shadow_dir = NULL;
	struct dirent *dirp;
	DIR *dp;
	char fullpath[MAX_STRLEN];
	struct stat statbuf;

	if ((test_file = get_shadow_file("test"))) {
		shadow_dir = dirname(test_file);
	} else {
		shadow_dir = CRM_CONFIG_DIR;
	}

	if ((dp = opendir(shadow_dir)) == NULL){
		mgmt_log(LOG_ERR, "error on opendir \"%s\": %s", shadow_dir, strerror(errno));
		if (test_file) {
			free(test_file);
		}
		return strdup(MSG_FAIL"\nCannot open the CIB shadow directory");
	}

	ret = strdup(MSG_OK);
	while ((dirp = readdir(dp)) != NULL) {
		if (strstr(dirp->d_name, "shadow.") == dirp->d_name) {
			snprintf(fullpath, sizeof(fullpath), "%s/%s", shadow_dir, dirp->d_name);

			if (stat(fullpath, &statbuf) < 0){
				mgmt_log(LOG_WARNING, "Cannot stat the file \"%s\": %s", fullpath, strerror(errno));
				continue;
			}
			if (S_ISREG(statbuf.st_mode)){
				ret = mgmt_msg_append(ret, dirp->d_name);
			}
		}
	}

	if (closedir(dp) < 0){
		mgmt_log(LOG_WARNING, "failed to closedir \"%s\": %s", shadow_dir, strerror(errno) );
	}

	if (test_file) {
		free(test_file);
	}
	return ret;
}
char*
on_crm_shadow(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char str[MAX_STRLEN];
	char* ret = NULL;
	int require_name = 0;
	const char* name = "";
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;
	pid_t childpid = 0;
	int stat;
	

	ARGC_CHECK(4);

	strncpy(cmd, "/usr/bin/xargs -0 crm_shadow 2>&1", sizeof(cmd)-1);

	if (STRNCMP_CONST(argv[3], "true") == 0) {
		strncat(cmd, " --force", sizeof(cmd)-strlen(cmd)-1);
	}

	if (STRNCMP_CONST(argv[1], "create") == 0) {
		strncat(cmd, " -b -c", sizeof(cmd)-strlen(cmd)-1);
		require_name = 1; 
	}
	else if (STRNCMP_CONST(argv[1], "create-empty") == 0) {
		strncat(cmd, " -b --create-empty", sizeof(cmd)-strlen(cmd)-1);
		require_name = 1; 
	}
	else if (STRNCMP_CONST(argv[1], "delete") == 0) {
		strncat(cmd, " -D", sizeof(cmd)-strlen(cmd)-1);
		require_name = 1; 
	}
	else if (STRNCMP_CONST(argv[1], "reset") == 0) {
		strncat(cmd, " -r", sizeof(cmd)-strlen(cmd)-1);
		require_name = 1; 
	}
	else if (STRNCMP_CONST(argv[1], "commit") == 0) {
		strncat(cmd, " -C", sizeof(cmd)-strlen(cmd)-1);
		require_name = 1; 
	}
	else if (STRNCMP_CONST(argv[1], "diff") == 0) {
		strncat(cmd, " -d", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid arguments specified: \"%s\"", argv[1]);
		return strdup(MSG_FAIL"\nInvalid arguments");
	}

	if (require_name) {
		if (strnlen(argv[2], MAX_STRLEN) != 0) {
			name = argv[2];
		} else if (getenv("CIB_shadow") != NULL) {
			name = getenv("CIB_shadow");
		}

		/*strncat(cmd, " ", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, name, sizeof(cmd)-strlen(cmd)-1);*/
	}

	if ((childpid = popen2(cmd, &fp_in, &fp_out)) < 0){
		mgmt_log(LOG_ERR, "error on popen2 \"%s\": %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nInvoke crm_shadow failed");
	}

	if (fputs(name, fp_in) == EOF) {
		mgmt_log(LOG_ERR, "error on fputs arguments to \"%s\": %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nPut arguments to crm_shadow failed");
	}

	if (fclose(fp_in) == EOF) {
		mgmt_log(LOG_WARNING, "failed to close input pipe");
	}

	ret = strdup(MSG_FAIL);
	gen_msg_from_fstream(fp_out, ret, buf, str);

	/*if (pclose2(fp_in, fp_out, childpid) == -1) {*/
	if ((stat = pclose2(NULL, fp_out, childpid)) == -1) {
		mgmt_log(LOG_WARNING, "failed to close pipe");
	/*} else if (WIFEXITED(stat) && WEXITSTATUS(stat) == 0) {
		ret[0] = CHR_OK;*/
	}

	return ret;
}

/* cluster  functions */
char* 
on_get_cluster_type(char* argv[], int argc)
{
	char* ret = NULL;

	if (is_openais_cluster()) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, "openais");
	}
	else if (is_heartbeat_cluster()) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, "heartbeat");
	}
	else {
		ret = strdup(MSG_FAIL);
	}
	return ret;
}

char* 
on_get_cib_version(char* argv[], int argc)
{
	const char* version = NULL;
	pe_working_set_t* data_set;
	char* ret;
	
	data_set = get_data_set();
	version = crm_element_value(data_set->input, "num_updates");
	if (version != NULL) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, version);
	}
	else {
		ret = strdup(MSG_FAIL);
	}	
	free_data_set(data_set);
	return ret;
}

static char*
on_get_crm_schema(char* argv[], int argc)
{
	const char *schema_file = NULL;
	const char *validate_type = NULL;
	const char *file_name = NULL;
	char buf[MAX_STRLEN];	
	char str[MAX_STRLEN];
	char* ret = NULL;
	FILE *fstream = NULL;

	ARGC_CHECK(3);
	validate_type = argv[1];
	file_name = argv[2];

	if (STRNCMP_CONST(validate_type, "") == 0){
		schema_file = HA_NOARCHDATAHBDIR"/crm.dtd";
	}
	else if (STRNCMP_CONST(validate_type, "pacemaker-0.6") == 0){
		schema_file = DTD_DIRECTORY"/crm.dtd";
	}
	else if (STRNCMP_CONST(validate_type, "transitional-0.6") == 0){
		schema_file = DTD_DIRECTORY"/crm-transitional.dtd";
	}
	else{
		if (STRNCMP_CONST(file_name, "") == 0){
			snprintf(buf, sizeof(buf), DTD_DIRECTORY"/%s.rng", validate_type);
			schema_file = buf;
		}
		else{
			snprintf(buf, sizeof(buf), DTD_DIRECTORY"/%s", file_name);
			schema_file = buf;
		}
	}

	if ((fstream = fopen(schema_file, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on fopen %s: %s",
			 schema_file, strerror(errno));
		return strdup(MSG_FAIL);
	}

	ret = strdup(MSG_OK);
	gen_msg_from_fstream(fstream, ret, buf, str);
	
	if (fclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to fclose stream");

	return ret;
}

static char*
on_get_crm_dtd(char* argv[], int argc)
{
	const char *dtd_file = HA_NOARCHDATAHBDIR"/crm.dtd";
	char buf[MAX_STRLEN];	
	char str[MAX_STRLEN];	
	char* ret = NULL;
	FILE *fstream = NULL;

	if ((fstream = fopen(dtd_file, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on fopen %s: %s",
			 dtd_file, strerror(errno));
		return strdup(MSG_FAIL);
	}

	ret = strdup(MSG_OK);
	gen_msg_from_fstream(fstream, ret, buf, str);

	if (fclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to fclose stream");

	return ret;
}

static char*
on_crm_attribute(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(8);

	snprintf(cmd, sizeof(cmd), "crm_attribute -t %s", argv[1]);

	if (regex_match(nv_regex, argv[3])){
		if (STRNCMP_CONST(argv[2], "get") == 0){
			strncat(cmd, " -Q -G", sizeof(cmd)-strlen(cmd)-1);
		}
		else if (STRNCMP_CONST(argv[2], "del") == 0){
			strncat(cmd, " -D", sizeof(cmd)-strlen(cmd)-1);
		}
		strncat(cmd, " -n \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	if (STRNCMP_CONST(argv[2], "set") == 0){
		if (regex_match(nv_regex, argv[4])){
			strncat(cmd, " -v \"", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[4], sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
		}
		else {
			mgmt_log(LOG_ERR, "invalid attribute value specified: \"%s\"", argv[4]);
			return strdup(MSG_FAIL"\nInvalid attribute value");
		}
	}

	if (STRNCMP_CONST(argv[5], "") != 0){
		if (uname2id(argv[5]) == NULL) {
			return strdup(MSG_FAIL"\nNo such node");
		}
		else{
			strncat(cmd, " -N ", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[5], sizeof(cmd)-strlen(cmd)-1);
		}
	}

	if (STRNCMP_CONST(argv[6], "") != 0){
		if (regex_match(nv_regex, argv[6])){
			strncat(cmd, " -s \"", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[6], sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
		}
		else {
			mgmt_log(LOG_ERR, "invalid attribute set ID specified: \"%s\"", argv[6]);
			return strdup(MSG_FAIL"\nInvalid attribute set ID");
		}
	}

	if (STRNCMP_CONST(argv[7], "") != 0){
		if (regex_match(nv_regex, argv[7])){
			strncat(cmd, " -i \"", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[7], sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
		}
		else {
			mgmt_log(LOG_ERR, "invalid attribute ID specified: \"%s\"", argv[7]);
			return strdup(MSG_FAIL"\nInvalid attribute ID");
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nInvoke crm_attribute failed");
	}

	if (STRNCMP_CONST(argv[2], "get") == 0){
		ret = strdup(MSG_OK);
	}
	else{
		ret = strdup(MSG_FAIL);
	}

	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;

}

static char*
on_get_crm_metadata(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];	
	char str[MAX_STRLEN];	
	char* ret = NULL;
	FILE *fstream = NULL;

	ARGC_CHECK(2);

	if (STRNCMP_CONST(argv[1], "pengine") != 0 &&
			STRNCMP_CONST(argv[1], "crmd") != 0) {
		return strdup(MSG_FAIL);
	}

	snprintf(cmd, sizeof(cmd), CRM_DAEMON_DIR"/%s metadata", argv[1]);
	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL);
	}

	ret = strdup(MSG_OK);
	gen_msg_from_fstream(fstream, ret, buf, str);

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

/* node functions */
char*
on_get_activenodes(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ret = strdup(MSG_OK);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (node->details->online) {
			ret = mgmt_msg_append(ret, node->details->uname);
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}

char*
on_get_crmnodes(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ret = strdup(MSG_OK);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		ret = mgmt_msg_append(ret, node->details->uname);
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}

char* 
on_get_dc(char* argv[], int argc)
{
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	if (data_set->dc_node != NULL) {
		char* ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, data_set->dc_node->details->uname);
		free_data_set(data_set);
		return ret;
	}
	free_data_set(data_set);
	return strdup(MSG_FAIL);
}


char*
on_get_node_config(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ARGC_CHECK(2);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (strncmp(argv[1],node->details->uname,MAX_STRLEN) == 0) {
			char* ret = strdup(MSG_OK);
			ret = mgmt_msg_append(ret, node->details->uname);
			ret = mgmt_msg_append(ret, node->details->online?"True":"False");
			ret = mgmt_msg_append(ret, node->details->standby?"True":"False");
			ret = mgmt_msg_append(ret, node->details->unclean?"True":"False");
			ret = mgmt_msg_append(ret, node->details->shutdown?"True":"False");
			ret = mgmt_msg_append(ret, node->details->expected_up?"True":"False");
			ret = mgmt_msg_append(ret, node->details->is_dc?"True":"False");
			ret = mgmt_msg_append(ret, node->details->type==node_ping?"ping":"member");
			ret = mgmt_msg_append(ret, node->details->pending?"True":"False");
			ret = mgmt_msg_append(ret, node->details->standby_onfail?"True":"False");
			
			free_data_set(data_set);
			return ret;
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return strdup(MSG_FAIL);
}

/* Liu Jiaming */
char * on_lic_verid(char * argv[], int argc) {

    struct halicense_info * ha_license = ha_license_init();
    if ( ha_license == NULL ) {
        return strdup(MSG_FAIL"\nFAIL");
    }
    int ret = halicense_get_info(ha_license);
    if ( ret != 0) {
        return strdup(MSG_FAIL"\nFAIL");
    }
    ret = ha_license_get_version_id(ha_license);
    switch(ret) {
        case 0:
            return strdup(MSG_OK"\ntrial");
        case 1:
            return strdup(MSG_OK"\nprofessional");
        case 2:
            return strdup(MSG_OK"\nadvanced");
        default:
            return strdup(MSG_FAIL"\nFAIL");
    }
}

char * on_lic_expire(char ** argv, int argc) {

          int ret = halicense_is_expired();
          if (ret == 0)
                return strdup(MSG_OK"\nOK");
          else
                return strdup(MSG_FAIL"\nFAIL");
}
/*Done*/

char*
on_get_running_rsc(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ARGC_CHECK(2);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		/*if (node->details->online) {*/
			if (strncmp(argv[1],node->details->uname,MAX_STRLEN) == 0) {
				GList* cur_rsc;
				char* ret = strdup(MSG_OK);
				cur_rsc = node->details->running_rsc;
				while(cur_rsc != NULL) {
					resource_t* rsc = (resource_t*)cur_rsc->data;
					ret = mgmt_msg_append(ret, rsc->id);
					cur_rsc = g_list_next(cur_rsc);
				}
				free_data_set(data_set);
				return ret;
			}
		/*}*/
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return strdup(MSG_FAIL);
}

char*
on_migrate_rsc(char* argv[], int argc)
{
	const char* id = NULL;
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* duration_regex = "^[A-Za-z0-9:-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(6)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	snprintf(cmd, sizeof(cmd), "crm_resource -M -r %s", argv[1]);

	if (STRNCMP_CONST(argv[2], "") != 0){
		id = uname2id(argv[2]);
		if (id == NULL) {
			return strdup(MSG_FAIL"\nNo such node");
		}
		else{
			strncat(cmd, " -H ", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[2], sizeof(cmd)-strlen(cmd)-1);
		}
	}

	if (STRNCMP_CONST(argv[3], "true") == 0){
		strncat(cmd, " -f", sizeof(cmd)-strlen(cmd)-1);
	}

	if (STRNCMP_CONST(argv[4], "") != 0){
		if (regex_match(duration_regex, argv[4])) {
			strncat(cmd, " -u \"", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[4], sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
		}
		else {
			mgmt_log(LOG_ERR, "invalid duration specified: \"%s\"", argv[1]);
			return strdup(MSG_FAIL"\nInvalid duration.\nPlease refer to "
					"http://en.wikipedia.org/wiki/ISO_8601#Duration for examples of valid durations");
		}
	}

        /*neoshineha ming.liu add*/
        if (STRNCMP_CONST(argv[5], "") != 0){
                if (regex_match(duration_regex, argv[5])) {
                        strncat(cmd, " -b \"", sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, argv[5], sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
                }
                else {
                        mgmt_log(LOG_ERR, "invalid date specified: \"%s\"", argv[1]);
                        return strdup(MSG_FAIL"\nInvalid date.\nPlease refer to "
                                        "http://en.wikipedia.org/wiki/ISO_8601#Dates for examples of valid dates");
                }
        }
        /*neoshineha ming.liu end*/

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nMigrate failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;

}

char*
on_set_node_standby(char* argv[], int argc)
{
	int rc;
	const char* id = NULL;
	const char* attr_value = NULL;

	ARGC_CHECK(3);
	id = uname2id(argv[1]);
	if (id == NULL) {
		return strdup(MSG_FAIL"\nNo such node");
	}

	if (STRNCMP_CONST(argv[2], "on") == 0 || STRNCMP_CONST(argv[2], "true") == 0){
		attr_value = "true";
	}
	else if (STRNCMP_CONST(argv[2], "off") == 0 || STRNCMP_CONST(argv[2], "false") == 0){
		attr_value = "false";
	}
	else{
		return strdup(MSG_FAIL"\nInvalid attribute value");
	}

	rc = set_standby(cib_conn, id, NULL, attr_value);
	if (rc < 0) {
		return crm_failed_msg(NULL, rc);
	}
	return strdup(MSG_OK);
}

/*
char*
on_set_node_standby(char* argv[], int argc)
{
	int rc;
	const char* id = NULL;
	crm_data_t* fragment = NULL;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	char xml[MAX_STRLEN];

	ARGC_CHECK(3);
	id = uname2id(argv[1]);
	if (id == NULL) {
		return strdup(MSG_FAIL"\nno such node");
	}
	
	snprintf(xml, MAX_STRLEN, 
		"<node id=\"%s\"><instance_attributes id=\"nodes-%s\">"
		"<attributes><nvpair id=\"standby-%s\" name=\"standby\" value=\"%s\"/>"
           	"</attributes></instance_attributes></node>", 
           	id, id, id, argv[2]);

	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	fragment = create_cib_fragment(cib_object, "nodes");

	mgmt_log(LOG_INFO, "(update)xml:%s",xml);

	rc = cib_conn->cmds->update(
			cib_conn, "nodes", fragment, cib_sync_call);

	free_xml(fragment);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	}
	free_xml(output);
	return strdup(MSG_OK);

}
*/
/* resource functions */
static int
delete_lrm_rsc(crm_ipc_t *crmd_channel, const char *host_uname, const char *rsc_id)
{
	crm_data_t *cmd = NULL;
	crm_data_t *msg_data = NULL;
	crm_data_t *rsc = NULL;
	crm_data_t *params = NULL;
	char our_pid[11];
	char *key = NULL; 
	
	snprintf(our_pid, 10, "%d", getpid());
	our_pid[10] = '\0';
	key = crm_concat(client_name, our_pid, '-');
	
	msg_data = create_xml_node(NULL, XML_GRAPH_TAG_RSC_OP);
	crm_xml_add(msg_data, XML_ATTR_TRANSITION_KEY, key);
	
	rsc = create_xml_node(msg_data, XML_CIB_TAG_RESOURCE);
	crm_xml_add(rsc, XML_ATTR_ID, rsc_id);

	params = create_xml_node(msg_data, XML_TAG_ATTRS);
	crm_xml_add(params, XML_ATTR_CRM_VERSION, CRM_FEATURE_SET);
	
	cmd = create_request(CRM_OP_LRM_DELETE, msg_data, host_uname,
			     CRM_SYSTEM_CRMD, client_name, our_pid);

	free_xml(msg_data);
	free(key);

#if !HAVE_CRM_IPC_NEW
	if(send_ipc_message(crmd_channel, cmd)) {
#else
        if(crm_ipc_send(crmd_channel, cmd, 0, 0, NULL)){
#endif
		free_xml(cmd);
		return 0;
	}
	free_xml(cmd);
	return -1;
}

static int
refresh_lrm(crm_ipc_t *crmd_channel, const char *host_uname)
{
	crm_data_t *cmd = NULL;
	char our_pid[11];
	
	snprintf(our_pid, 10, "%d", getpid());
	our_pid[10] = '\0';
	
	cmd = create_request(CRM_OP_LRM_REFRESH, NULL, host_uname,
			     CRM_SYSTEM_CRMD, client_name, our_pid);
	
#if !HAVE_CRM_IPC_NEW
	if(send_ipc_message(crmd_channel, cmd)) {
#else
        if(crm_ipc_send(crmd_channel, cmd, 0, 0, NULL)){
#endif
		free_xml(cmd);
		return 0;
	}
	free_xml(cmd);
	return -1;
}

char*
on_cleanup_rsc(char* argv[], int argc)
{
	crm_ipc_t *crmd_channel = NULL;
	char our_pid[11];
	char *now_s = NULL;
	time_t now = time(NULL);
	char *dest_node = NULL;
	int rc;
	char *buffer = NULL;
	int is_remote_node = 0;

	
	ARGC_CHECK(3);
	snprintf(our_pid, 10, "%d", getpid());
	our_pid[10] = '\0';
	
#if !HAVE_CRM_IPC_NEW
	init_client_ipc_comms(CRM_SYSTEM_CRMD, NULL,
				    NULL, &crmd_channel);

	send_hello_message(crmd_channel, our_pid, client_name, "0", "1");
#else
    crmd_channel = crm_ipc_new(CRM_SYSTEM_CRMD, 0);
    if (crmd_channel && crm_ipc_connect(crmd_channel)) {
        xmlNode *hello = NULL;
        hello = create_hello_message(our_pid, client_name, "0", "1");
        rc = crm_ipc_send(crmd_channel, hello, 0, 0, NULL);
        free_xml(hello);

    } else {
        rc = -ENOTCONN;
    }

    if (rc < 0) {
        if (crmd_channel) {
            crm_ipc_close(crmd_channel);
            crm_ipc_destroy(crmd_channel);
        }
        mgmt_log(LOG_ERR, "Error signing on to the CRMd service: %s", pcmk_strerror(rc));
        return strdup(MSG_FAIL"\nError signing on to the CRMd service");
    }
#endif

	delete_lrm_rsc(crmd_channel, argv[1], argv[2]);
	refresh_lrm(crmd_channel, NULL); 
	
	rc = query_node_uuid(cib_conn, argv[1], &dest_node, &is_remote_node);
	if (rc != 0) {
		mgmt_log(LOG_WARNING, "Could not map uname=%s to a UUID: %s\n",
				argv[1], pcmk_strerror(rc));
	} else {
		buffer = crm_concat("fail-count", argv[2], '-');

#if !HAVE_UPDATE_ATTR_DELEGATE
		delete_attr(cib_conn, cib_sync_call, XML_CIB_TAG_STATUS, dest_node, NULL, NULL,
				NULL, buffer, NULL, FALSE);
#else
                delete_attr_delegate(cib_conn, cib_sync_call, XML_CIB_TAG_STATUS, dest_node, NULL, NULL,
                                NULL, buffer, NULL, FALSE, NULL);
#endif

		free(dest_node);
		free(buffer);
		mgmt_log(LOG_INFO, "Delete fail-count for %s from %s", argv[2], argv[1]);
	}
	/* force the TE to start a transition */
	sleep(2); /* wait for the refresh */
	now_s = crm_itoa(now);
#if !HAVE_UPDATE_ATTR_DELEGATE
	update_attr(cib_conn, cib_sync_call,
		    XML_CIB_TAG_CRMCONFIG, NULL, NULL, NULL, NULL, "last-lrm-refresh", now_s, FALSE);
#else
        update_attr_delegate(cib_conn, cib_sync_call,
                    XML_CIB_TAG_CRMCONFIG, NULL, NULL, NULL, NULL, "last-lrm-refresh", now_s, FALSE, NULL, NULL);
#endif
	free(now_s);

#if !HAVE_CRM_IPC_NEW
	crmd_channel->ops->destroy(crmd_channel);
#else
    if (crmd_channel) {
        crm_ipc_close(crmd_channel);
        crm_ipc_destroy(crmd_channel);
    }
#endif
	
	return strdup(MSG_OK);
}

/* get all resources*/
char*
on_get_all_rsc(char* argv[], int argc)
{
	GList* cur;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	ret = strdup(MSG_OK);
	cur = data_set->resources;
	while (cur != NULL) {
		resource_t* rsc = (resource_t*)cur->data;
		if(is_not_set(rsc->flags, pe_rsc_orphan) || rsc->role != RSC_ROLE_STOPPED) {
			ret = mgmt_msg_append(ret, rsc->id);
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}
/* basic information of resource */
char*
on_get_rsc_running_on(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()

	ret = strdup(MSG_OK);
	cur = rsc->running_on;
	while (cur != NULL) {
		node_t* node = (node_t*)cur->data;
		ret = mgmt_msg_append(ret, node->details->uname);
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}
char*
on_get_rsc_status(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	pe_working_set_t* data_set;
	char* num_s;
	char buf[MAX_STRLEN];
	
	data_set = get_data_set();
	GET_RESOURCE()
	ret = strdup(MSG_OK);
	switch (rsc->variant) {
		case pe_unknown:
			ret = mgmt_msg_append(ret, "unknown");
			break;
		case pe_native:
			memset(buf, 0, sizeof(buf));

			if(is_not_set(rsc->flags, pe_rsc_managed)) {
				strncat(buf, "unmanaged", sizeof(buf)-strlen(buf)-1);
			}
			else if(is_set(rsc->flags, pe_rsc_failed)) {
				strncat(buf, "failed", sizeof(buf)-strlen(buf)-1);
			}
			else if (g_list_length(rsc->running_on) > 0
					&& rsc->fns->active(rsc, TRUE) == FALSE) {
				strncat(buf, "unclean", sizeof(buf)-strlen(buf)-1);
			}
			else if (g_list_length(rsc->running_on) == 0) {
				strncat(buf, "not running", sizeof(buf)-strlen(buf)-1);
			}
			else if (g_list_length(rsc->running_on) > 1) {
				strncat(buf, "multi-running", sizeof(buf)-strlen(buf)-1);
			}
			else if(is_set(rsc->flags, pe_rsc_start_pending)) {
				strncat(buf, "starting", sizeof(buf)-strlen(buf)-1);
			}
			else if(rsc->role == RSC_ROLE_MASTER) {
				strncat(buf, "running (Master)", sizeof(buf)-strlen(buf)-1);
			}
			else if(rsc->role == RSC_ROLE_SLAVE) {
				strncat(buf, "running (Slave)", sizeof(buf)-strlen(buf)-1);
			}
			else if(rsc->role == RSC_ROLE_STARTED) {
				strncat(buf, "running", sizeof(buf)-strlen(buf)-1);
			}
			else {
				strncat(buf, role2text(rsc->role), sizeof(buf)-strlen(buf)-1);
			}

			if(is_set(rsc->flags, pe_rsc_orphan)) {
				strncat(buf, " (orphaned)", sizeof(buf)-strlen(buf)-1);
			}

                        if(is_set(rsc->flags, pe_rsc_failure_ignored)) {
                                strncat(buf, " (failure ignored)", sizeof(buf)-strlen(buf)-1);
                        }

			ret = mgmt_msg_append(ret, buf);
			break;
		case pe_group:
			ret = mgmt_msg_append(ret, "group");
			break;
		case pe_clone:
			ret = mgmt_msg_append(ret, "clone");
			break;
		case pe_master:
			ret = mgmt_msg_append(ret, "master");
			break;
	}
	num_s = crm_itoa(rsc->migration_threshold);
	ret = mgmt_msg_append(ret, num_s);
	free(num_s);
	free_data_set(data_set);
	return ret;
}

char*
on_get_rsc_type(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()

	ret = strdup(MSG_OK);

	switch (rsc->variant) {
		case pe_unknown:
			ret = mgmt_msg_append(ret, "unknown");
			break;
		case pe_native:
			ret = mgmt_msg_append(ret, "native");
			break;
		case pe_group:
			ret = mgmt_msg_append(ret, "group");
			break;
		case pe_clone:
			ret = mgmt_msg_append(ret, "clone");
			break;
		case pe_master:
			ret = mgmt_msg_append(ret, "master");
			break;
	}
	free_data_set(data_set);
	return ret;
}

char*
on_op_status2str(char* argv[], int argc)
{
	int op_status;
	char* ret = strdup(MSG_OK);

	ARGC_CHECK(2);
	op_status = atoi(argv[1]);
#if !HAVE_DECL_SERVICES_LRM_STATUS_STR
	ret = mgmt_msg_append(ret, op_status2text(op_status));
#else
        ret = mgmt_msg_append(ret, services_lrm_status_str(op_status));
#endif
	return ret;
}

char*
on_get_sub_rsc(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	GList* cur = NULL;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()
		
	cur = rsc->children;
	
	ret = strdup(MSG_OK);
	while (cur != NULL) {
		resource_t* rsc = (resource_t*)cur->data;
		gboolean is_active = rsc->fns->active(rsc, TRUE);
		if (is_not_set(rsc->flags, pe_rsc_orphan) || is_active) {
			ret = mgmt_msg_append(ret, rsc->id);
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}

char*
on_crm_rsc_cmd(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	FILE* fstream = NULL;
	char buf[MAX_STRLEN];

	ARGC_CHECK(4)

	if (STRNCMP_CONST(argv[2], "refresh") == 0){
		strncpy(cmd, "crm_resource -R", sizeof(cmd)-1) ;
	}
	else if (STRNCMP_CONST(argv[2], "reprobe") == 0){
		strncpy(cmd, "crm_resource -P", sizeof(cmd)-1) ;
	}
	else if (STRNCMP_CONST(argv[2], "cleanup") == 0){
		strncpy(cmd, "crm_resource -C", sizeof(cmd)-1) ;
	}
	else if (STRNCMP_CONST(argv[2], "fail") == 0){
		strncpy(cmd, "crm_resource -F", sizeof(cmd)-1) ;
	}
	else{
		return strdup(MSG_FAIL"\nNo such command");
	}

	if (strlen(argv[1]) > 0){
		data_set = get_data_set();
		GET_RESOURCE()
		free_data_set(data_set);

		strncat(cmd, " -r ", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[1], sizeof(cmd)-strlen(cmd)-1);
	}

	if (strlen(argv[3]) > 0){
		if (uname2id(argv[3]) == NULL){
			return strdup(MSG_FAIL"\nNo such node");
		}
		else{
			strncat(cmd, " -H ", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s", cmd, strerror(errno));
		return strdup(MSG_FAIL"\nDo crm_resource command failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "failed to close pipe");
	}

	return ret;
}

char*
on_set_rsc_attr(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(5)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	if (STRNCMP_CONST(argv[2], "meta") == 0){
		snprintf(cmd, sizeof(cmd), "crm_resource --meta -r %s", argv[1]);
	}
	else{
		snprintf(cmd, sizeof(cmd), "crm_resource -r %s", argv[1]);
	}
	
	if (regex_match(nv_regex, argv[3])) {
		strncat(cmd, " -p \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	if (regex_match(nv_regex, argv[4])) {
		strncat(cmd, " -v \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[4], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute value specified: \"%s\"", argv[4]);
		return strdup(MSG_FAIL"\nInvalid attribute value");
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nSet the named attribute failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

char*
on_get_rsc_attr(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(4)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	if (STRNCMP_CONST(argv[2], "meta") == 0){
		snprintf(cmd, sizeof(cmd), "crm_resource --meta -r %s", argv[1]);
	}
	else{
		snprintf(cmd, sizeof(cmd), "crm_resource -r %s", argv[1]);
	}
	
	if (regex_match(nv_regex, argv[3])) {
		strncat(cmd, " -g \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nGet the named attribute failed");
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

char*
on_del_rsc_attr(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(4)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	if (STRNCMP_CONST(argv[2], "meta") == 0){
		snprintf(cmd, sizeof(cmd), "crm_resource --meta -r %s", argv[1]);
	}
	else{
		snprintf(cmd, sizeof(cmd), "crm_resource -r %s", argv[1]);
	}
	
	if (regex_match(nv_regex, argv[3])) {
		strncat(cmd, " -d \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nGet the named attribute failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

char*
on_cib_create(char* argv[], int argc)
{
	int rc;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = NULL;
	const char* xmls = NULL;
	ARGC_CHECK(3)

	type = argv[1];
	xmls = argv[2];
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "CIB create: %s", type);
		
	rc = cib_conn->cmds->create(cib_conn, type, cib_object, cib_sync_call);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}

/*
char*
on_cib_query(char* argv[], int argc)
{
	const char* type = NULL;
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];	
	char str[MAX_STRLEN];	
	char* ret = strdup(MSG_OK);
	FILE *fstream = NULL;
	ARGC_CHECK(2)

	type = argv[1];
	mgmt_log(LOG_INFO, "CIB query: %s", type);
		
	snprintf(cmd, sizeof(cmd), "cibadmin -Q -o %s", type);
	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL);
	}

	gen_msg_from_fstream(fstream, ret, buf, str);

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}
*/

char*
on_cib_query(char* argv[], int argc)
{
	int rc;
	crm_data_t* output = NULL;
	const char* type = NULL;
	char* ret = NULL;
	char* buffer = NULL;
	CIB_CHECK()
	ARGC_CHECK(2)

	type = argv[1];
	mgmt_log(LOG_INFO, "CIB query: %s", type);
		
	rc = cib_conn->cmds->query(cib_conn, type, &output, cib_sync_call|cib_scope_local);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		ret = strdup(MSG_OK);
		buffer = dump_xml_formatted(output);
		ret = mgmt_msg_append(ret, buffer);
#if 0		
		mgmt_log(LOG_INFO, "%s", buffer); 
#endif
		free(buffer);
		free_xml(output);
		return ret;
	}
}

char*
on_cib_update(char* argv[], int argc)
{
	int rc;
	crm_data_t* fragment = NULL;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = NULL;
	const char* xmls = NULL;
	ARGC_CHECK(3)

	type = argv[1];
	xmls = argv[2];
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "CIB update: %s", xmls);
		
/*	fragment = create_cib_fragment(cib_object, type); */
	rc = cib_conn->cmds->update(cib_conn, type, fragment, cib_sync_call);
/*	free_xml(fragment); */
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}

char*
on_cib_replace(char* argv[], int argc)
{
	int rc;
	/*crm_data_t* fragment = NULL;*/
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = NULL;
	const char* xmls = NULL;
	ARGC_CHECK(3)

	type = argv[1];
	xmls = argv[2];
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "CIB replace: %s", type);
		
	/*fragment = create_cib_fragment(cib_object, type);*/
	rc = cib_conn->cmds->replace(cib_conn, type, cib_object, cib_sync_call);
	/*free_xml(fragment);*/
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}

char*
on_cib_delete(char* argv[], int argc)
{
	int rc;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = NULL;
	const char* xmls = NULL;	
	ARGC_CHECK(3)
	
	type = argv[1];
	xmls = argv[2];	
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}
	mgmt_log(LOG_INFO, "CIB delete: %s", type);

	rc = cib_conn->cmds->delete(cib_conn, type, cib_object, cib_sync_call);
	free_xml(cib_object);	
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}		

static char*
on_gen_cluster_report(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char str[MAX_STRLEN];
	char filename[MAX_STRLEN];
	const char *tempdir = "/tmp";
	char *dest = tempnam(tempdir, "clrp_");
	struct stat statbuf;
	char *ret = NULL;
	FILE *fstream = NULL;
	const char* date_regex = \
		"^[0-9]{4}-[0-1][0-9]-[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9]$";

	ARGC_CHECK(3);

	if (regex_match(date_regex, argv[1])) {
		snprintf(buf, sizeof(buf), "-f \"%s\"", argv[1]);
	}
	else {
		mgmt_log(LOG_ERR, "cluster_report: invalid \"from\" date expression: \"%s\"", argv[1]);
		free(dest);
		return strdup(MSG_FAIL"\nInvalid \"from\" date expression");
	}

	if (strnlen(argv[2], MAX_STRLEN) != 0) {
		if (regex_match(date_regex, argv[2])) {
			strncat(buf, " -t \"", sizeof(buf)-strlen(buf)-1);
			strncat(buf, argv[2], sizeof(buf)-strlen(buf)-1);
			strncat(buf, "\"", sizeof(buf)-strlen(buf)-1);
		}
		else {
			mgmt_log(LOG_ERR, "cluster_report: invalid \"to\" date expression: \"%s\"", argv[2]);
			free(dest);
			return strdup(MSG_FAIL"\nInvalid \"to\" date expression");
		}
	}

	if (is_openais_cluster()){
		snprintf(cmd, sizeof(cmd), "hb_report -E '/var/log/neokylinha.log /var/log/neokylinha_ocf.log /var/log/neokylinha_qdiskd.log' -ADC %s %s", buf, dest);
	}
	else{
		snprintf(cmd, sizeof(cmd), "hb_report -DC %s %s", buf, dest);
	}

	mgmt_log(LOG_INFO, "cluster_report: %s", cmd);
	if (system(cmd) < 0) {
		mgmt_log(LOG_ERR, "cluster_report: error on system \"%s\": %s", cmd, strerror(errno));
		free(dest);
		return strdup(MSG_FAIL"\nError on execute the cluster report command");
	}

	snprintf(filename, sizeof(filename), "%s.tar.bz2", dest);
	if (stat(filename, &statbuf) < 0){
		snprintf(filename, sizeof(filename), "%s.tar.gz", dest);
		if (stat(filename, &statbuf) < 0){
			free(dest);
			mgmt_log(LOG_WARNING, "cluster_report: cannot stat the report file");
			mgmt_log(LOG_ERR, "cluster_report: failed to generate a cluster report");
			return strdup(MSG_FAIL"\nFailed to generate a cluster report");
		}
	}

	free(dest);
	mgmt_log(LOG_INFO, "cluster_report: successfully generated the cluster report");

	snprintf(cmd, sizeof(cmd), "/usr/bin/base64 %s", filename);
	if ((fstream = popen(cmd, "r")) == NULL) {
		mgmt_log(LOG_ERR, "cluster_report: error on popen \"%s\": %s", cmd, strerror(errno));
		unlink(filename);
		return strdup(MSG_FAIL"\nFailed to encode the cluster report to base64");
	 }

	ret = strdup(MSG_OK);
	ret = mgmt_msg_append(ret, filename);
	gen_msg_from_fstream(fstream, ret, buf, str);

	if (pclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "cluster_report: failed to close pipe");
	}

	mgmt_log(LOG_INFO, "cluster_report: send out the report");
	unlink(filename);
	return ret;
}

#ifdef PE_STATE_DIR
static const char* pe_state_dir = PE_STATE_DIR;
#else
static const char* pe_state_dir = HA_VARLIBHBDIR"/pengine";
#endif

typedef struct pe_series_s
{
	const char *name;
	const char *param;
	int wrap;
} pe_series_t;

pe_series_t pe_series[] = {
        { "pe-unknown", "_dont_match_anything_", -1 },
        { "pe-error",   "pe-error-series-max", -1, },
        { "pe-warn",    "pe-warn-series-max", 200, },
        { "pe-input",   "pe-input-series-max", 400, },
};

#define get_seq(seq, last, max, offset, new_seq)					\
	if (max > 0) {									\
		if (seq <= last) {							\
			if (seq + offset > last) {					\
				new_seq = -1;						\
			} else if (seq + offset <= 0 && seq + offset + max > last) {	\
				new_seq += max;						\
			} else {							\
				new_seq = seq + offset;					\
			}								\
		} else {								\
			if (seq + offset <= last) {					\
				new_seq = -1;						\
			} else if (seq + offset > max && seq + offset - max <= last) {	\
				new_seq -= max;						\
			} else {							\
				new_seq = seq + offset;					\
			}								\
		}									\
	} else {									\
		new_seq = seq + offset;							\
	}

#define get_mid_seq(first, last, max, mid)		\
	if (max > 0 && first > last) {			\
		mid = (first + last + max) / 2;		\
		if (mid > max) {			\
			mid -= max;			\
		}					\
	} else {					\
		mid = (first + last) / 2;		\
	}

#define seq_lt(seq1, seq2, last, max, is_lt)			\
	if (max > 0) {						\
		if (seq1 > last && seq2 <= last) {		\
			is_lt = (seq1 - max < seq2);		\
		} else if (seq1 <= last && seq2 > last) {	\
			is_lt = (seq1 < seq2 - max);		\
		} else {					\
			is_lt = (seq1 < seq2);			\
		}						\
	} else {						\
		is_lt = (seq1 < seq2);				\
	}

static char*
on_get_pe_inputs(char* argv[], int argc)
{
	char* ret = NULL;
	time_t from_time = -1;
	time_t to_time = -1;
	pe_working_set_t* data_set;
	int i = 0;
	int wrap = -1;
	char* value = NULL;
	int last_seq = -1;
	int start_seq = -1;
	int end_seq = -1;
	int first = -1;
	int last = -1;
	int from_seq = -1;
	int to_seq = -1;
	int mid = -1;
	char* filename = NULL;
	struct stat statbuf;
	int stat_rc = -1;
	int try_count = 0;
	int is_lt = 0;
	int seq = 0;
	char info[MAX_STRLEN];
	char buf[MAX_STRLEN];
	int compress = TRUE;

	ARGC_CHECK(3);

	if (STRNCMP_CONST(argv[1], "") != 0) {
		from_time = crm_int_helper(argv[1], NULL);
	}
	if (STRNCMP_CONST(argv[2], "") != 0) {
		to_time = crm_int_helper(argv[2], NULL);
	}

	if (from_time >= 0 && to_time >= 0 && from_time > to_time) {
		return strdup(MSG_FAIL"\n\"From\" time should be earlier than \"To\" time");
	}

	data_set = get_data_set();

	ret = strdup(MSG_OK);
	for (i = 0; i < 4 ; i++) {
		wrap = pe_series[i].wrap;
		if (data_set != NULL && data_set->config_hash != NULL) {
			value = g_hash_table_lookup(data_set->config_hash, pe_series[i].param);
			if (value != NULL) {
				wrap = crm_int_helper(value, NULL);
			}
		}

		last_seq = get_last_sequence(pe_state_dir, pe_series[i].name);

		if (wrap > 0) {
			for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
				filename = generate_series_filename(
        		                pe_state_dir, pe_series[i].name, last_seq, compress);
				stat_rc = stat(filename, &statbuf);
				free(filename);
				if (stat_rc == 0) {
					break;
				}
			}
			if (stat_rc == 0) {
				start_seq = last_seq;
				if (last_seq == 1) {
					end_seq = wrap;
				} else {
					end_seq = last_seq -1;
				}
			} else {
				start_seq = 0;
				end_seq = last_seq - 1;
			}
		} else {
			start_seq = 0;
			end_seq = last_seq - 1;
		}

		if (end_seq < 0) {
			continue;
		}

		first = start_seq;
		last = end_seq;
		mid = first;

		try_count = 50;
		while (first != last && try_count > 0) {
			for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
				filename = generate_series_filename(
        	        	        pe_state_dir, pe_series[i].name, mid, compress);
				stat_rc = stat(filename, &statbuf);
				free(filename);
				if (stat_rc == 0) {
					break;
				}
			}

			if (stat_rc == 0 && from_time < 0) {
				first = mid;
				break;
			}	

			if (stat_rc < 0 || statbuf.st_mtime < from_time) {
				if (mid == last) {
					first = mid;
				} else {
					get_seq(mid, end_seq, wrap, 1, first);
				}
			} else {
				last = mid;
			}
			get_mid_seq(first, last, wrap, mid);
			try_count--;
		}
		from_seq = first;

		first = start_seq;
		last = end_seq;
		mid = last;

		try_count = 50;
		while (first != last && try_count > 0) {
			for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
				filename = generate_series_filename(
        	                	pe_state_dir, pe_series[i].name, mid, compress);
				stat_rc = stat(filename, &statbuf);
				free(filename);
				if (stat_rc == 0) {
					break;
				}
			}

			if (stat_rc == 0 && to_time < 0) {
				last = mid;
				break;
			}	

			if (stat_rc < 0 || statbuf.st_mtime <= to_time) {
				if (mid == last) {
					first = mid;
				} else {
					get_seq(mid, end_seq, wrap, 1, first);
				}
			} else {
				last = mid;
			}
			get_mid_seq(first, last, wrap, mid);
			try_count--;
		}
		to_seq = last;

		memset(buf, 0, sizeof(buf));
		seq_lt(from_seq, to_seq, end_seq, wrap, is_lt);
		seq = from_seq;	
		while (seq >= 0 && (is_lt || seq == to_seq)) {
			for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
				filename = generate_series_filename(
        	                	pe_state_dir, pe_series[i].name, seq, compress);
				stat_rc = stat(filename, &statbuf);
				if (stat_rc == 0) {
					break;
				} else {
					free(filename);
				}
			}

			if (stat_rc == 0) {
				snprintf(info, sizeof(info), "%s %ld ", basename(filename), (long)statbuf.st_mtime);
				append_str(ret, buf, info);
				free(filename);
			}

			get_seq(seq, end_seq, wrap, 1, seq);
			seq_lt(seq, to_seq, end_seq, wrap, is_lt);
		}
		ret = mgmt_msg_append(ret, buf);

	}

	if (data_set != NULL) {
		free_data_set(data_set);
	}
	return ret;
}

static char*
on_get_pe_summary(char* argv[], int argc)
{
	char* ret = NULL;
	time_t time_stamp;
	char* filename = NULL;
	char info[MAX_STRLEN];
	struct stat statbuf;
	int stat_rc = -1;
	int compress = TRUE;

	ARGC_CHECK(3)
	if (STRNCMP_CONST(argv[1], "live") == 0) {
		time(&time_stamp);
		snprintf(info, sizeof(info), "%ld", time_stamp);
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, info);
	} else {
		for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
			filename = generate_series_filename(
                       		pe_state_dir, argv[1], crm_int_helper(argv[2], NULL), compress);
			stat_rc = stat(filename, &statbuf);
			free(filename);
			if (stat_rc == 0) {
				break;
			}
		}

		if (stat_rc == 0) {
			snprintf(info, sizeof(info), "%ld", statbuf.st_mtime);
			ret = strdup(MSG_OK);
			ret = mgmt_msg_append(ret, info);
		} else {
			mgmt_log(LOG_WARNING, "Cannot stat the transition file \"%s/%s-%s.*\": %s",
				pe_state_dir, argv[1], argv[2], strerror(errno));
			ret = strdup(MSG_FAIL"\nThe specified transition doesn't exist");
		}
	}

	return ret;
}

static char*
on_gen_pe_graph(char* argv[], int argc)
{
	char* ret = NULL;
	char* filename = NULL;
	struct stat statbuf;
	int stat_rc = -1;
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char str[MAX_STRLEN];
	char *dotfile = NULL;
	FILE *fstream = NULL;
	int compress = TRUE;

	ARGC_CHECK(3)
	if (STRNCMP_CONST(argv[1], "live") == 0) {
		strncpy(cmd, "crm_simulate -L", sizeof(cmd)-1);
	} else {
		for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
			filename = generate_series_filename(
                        	pe_state_dir, argv[1], crm_int_helper(argv[2], NULL), compress);
			stat_rc = stat(filename, &statbuf);
			if (stat_rc == 0) {
				break;
			} else {
				free(filename);
			}
		}

		if (stat_rc == 0) {
			snprintf(cmd, sizeof(cmd), "crm_simulate -x %s", filename);
			free(filename);
		} else {
			mgmt_log(LOG_WARNING, "Cannot stat the transition file \"%s/%s-%s.*\": %s",
				pe_state_dir, argv[1], argv[2], strerror(errno));
			return strdup(MSG_FAIL"\nThe specified transition doesn't exist");
		}
	}

	strncat(cmd, " -D ", sizeof(cmd)-strlen(cmd)-1);
	dotfile = tempnam("/tmp", "dot.");
	strncat(cmd, dotfile, sizeof(cmd)-strlen(cmd)-1);

	if (system(cmd) < 0){
		mgmt_log(LOG_ERR, "error on execute \"%s\": %s", cmd, strerror(errno));
		free(dotfile);
		return strdup(MSG_FAIL"\nError on execute the crm_simulate command");
	}

	if ((fstream = fopen(dotfile, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on fopen %s: %s", dotfile, strerror(errno));
		free(dotfile);
		unlink(dotfile);
		return strdup(MSG_FAIL"\nError on read the transition graph file");
	}

	ret = strdup(MSG_OK);
	gen_msg_from_fstream(fstream, ret, buf, str);

	if (fclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "failed to fclose stream");
	}

	unlink(dotfile);
	free(dotfile);
	return ret;
}

static char*
on_gen_pe_info(char* argv[], int argc)
{
	char* ret = NULL;
	char* filename = NULL;
	struct stat statbuf;
	int stat_rc = -1;
	char cmd[MAX_STRLEN];
	int i;
	char buf[MAX_STRLEN];
	char str[MAX_STRLEN];
	FILE *fstream = NULL;
	int compress = TRUE;

	ARGC_CHECK(4)
	if (STRNCMP_CONST(argv[1], "live") == 0){
		strncpy(cmd, "crm_simulate -L", sizeof(cmd)-1);
	} else {
		for (compress = 1, stat_rc = -1; compress >= 0; compress--) {
			filename = generate_series_filename(
                        	pe_state_dir, argv[1], crm_int_helper(argv[2], NULL), compress);
			stat_rc = stat(filename, &statbuf);
			if (stat_rc == 0) {
				break;
			} else {
				free(filename);
			}
		}

		if (stat_rc == 0) {
			snprintf(cmd, sizeof(cmd), "crm_simulate -x %s", filename);
			free(filename);
		} else {
			mgmt_log(LOG_WARNING, "Cannot stat the transition file \"%s/%s-%s.*\": %s",
				pe_state_dir, argv[1], argv[2], strerror(errno));
			return strdup(MSG_FAIL"\nThe specified transition doesn't exist");
		}
	}
	
	if (STRNCMP_CONST(argv[3], "scores") == 0) {
		strncat(cmd, " -s -Q", sizeof(cmd)-strlen(cmd)-1);
	} else {
		strncat(cmd, " -S", sizeof(cmd)-strlen(cmd)-1);

		for (i = 0; i < atoi(argv[3]); i++) {
			if (i == 0){
				strncat(cmd, " -V", sizeof(cmd)-strlen(cmd)-1);
			}
			else{
				strncat(cmd, "V", sizeof(cmd)-strlen(cmd)-1);
			}
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen \"%s\": %s", cmd, strerror(errno));
		return strdup(MSG_FAIL"\nError on popen the crm_simulate command");
	}

	ret = strdup(MSG_OK);
	gen_msg_from_fstream(fstream, ret, buf, str);

	if (fclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "failed to fclose stream");
	}

	return ret;
}

int
regex_match(const char *regex, const char *str)
{
	regex_t preg;
	int ret;

	if (regcomp(&preg, regex, REG_EXTENDED|REG_NOSUB) != 0){
		mgmt_log(LOG_ERR, "error regcomp regular expression: \"%s\"", regex);
		return 0;
	}

	ret = regexec(&preg, str, 0, NULL, 0);
	if (ret != 0) {
		mgmt_log(LOG_WARNING, "no match or error regexec: \"%s\" \"%s\"", regex, str);
	}

	regfree(&preg);
	return (ret == 0);
}

pid_t
popen2(const char *command, FILE **fp_in, FILE **fp_out)
{
	int pfd_in[2];
	int pfd_out[2];
	pid_t pid;

	if (fp_in != NULL) {
		if (pipe(pfd_in) < 0) {
			return -1;	/* errno set by pipe() */
		}
	}
	if (fp_out != NULL) {
		if (pipe(pfd_out) < 0) {
			return -1;
		}
	}

	if ((pid = fork()) < 0) {
		return -1;	/* errno set by fork() */
	} else if (pid == 0) {	/* child */
		if (fp_in != NULL) {
			close(pfd_in[1]);
			if (pfd_in[0] != STDIN_FILENO) {
				dup2(pfd_in[0], STDIN_FILENO);
				close(pfd_in[0]);
			}
		}

		if (fp_out != NULL) {
			close(pfd_out[0]);
			if (pfd_out[1] != STDOUT_FILENO) {
				dup2(pfd_out[1], STDOUT_FILENO);
				close(pfd_out[1]);
			}
		}

		execl("/bin/sh", "sh", "-c", command, NULL);
		_exit(127);
	}

	/* parent continues... */
	if (fp_in != NULL) {
		close(pfd_in[0]);
		if ((*fp_in = fdopen(pfd_in[1], "w")) == NULL) {
			return -1;
		}
	}
	if (fp_out != NULL) {
		close(pfd_out[1]);
		if ((*fp_out = fdopen(pfd_out[0], "r")) == NULL) {
			return -1;
		}
	}

	return pid;
}

int
pclose2(FILE *fp_in, FILE *fp_out, pid_t pid)
{
	int stat;

	if (fp_in != NULL && fclose(fp_in) != 0) {
		return -1;
	}

	if (fp_out != NULL && fclose(fp_out) != 0) {
		return -1;
	}

	while (waitpid(pid, &stat, 0) < 0) {
		if (errno != EINTR)
			return -1;	/* error other than EINTR from waitpid() */
	}

	return stat;	/* return child's termination status */
}

/*neoshineha ming.liu add*/
char *
on_system(char* argv[], int argc)
{
        char cmd[MAX_STRLEN];
        char* ret = NULL;
        FILE* fstream = NULL;
        char buf[MAX_STRLEN];

        ARGC_CHECK(2)

        strncpy(cmd, argv[1], sizeof(cmd)-1);
        /*strncat(cmd, " &> /tmp/output", sizeof(cmd)-strlen(cmd)-1);*/


        /*if (strlen(argv[1]) > 0){
                data_set = get_data_set();
                GET_RESOURCE()
                free_data_set(data_set);

                strncat(cmd, " -r ", sizeof(cmd)-strlen(cmd)-1);
                strncat(cmd, argv[1], sizeof(cmd)-strlen(cmd)-1);
        }*/


        strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

        if ((fstream = popen(cmd, "r")) == NULL){
                mgmt_log(LOG_ERR, "error on popen %s: %s", cmd, strerror(errno));
                return strdup(MSG_FAIL"\nDo crm_resource command failed");
        }
        ret = strdup(MSG_OK);

        while (!feof(fstream)){
                memset(buf, 0, sizeof(buf));
                if (fgets(buf, sizeof(buf), fstream) != NULL){
                        ret = mgmt_msg_append(ret, buf);
                        ret[strlen(ret)-1] = '\0';
                }
                else{
                        sleep(1);
                }
        }

        if (pclose(fstream) == -1){
                mgmt_log(LOG_WARNING, "failed to close pipe");
        }
        return ret;
}


char*
on_get_rsc_constraints(char* argv[], int argc)
{
        char cmd[MAX_STRLEN];
        pe_working_set_t* data_set;
        resource_t* rsc;
        char* ret = NULL;
        FILE* fstream = NULL;
        char buf[MAX_STRLEN];

        ARGC_CHECK(2)

        strncpy(cmd, "crm_resource -a", sizeof(cmd)-1) ;

        if (strlen(argv[1]) > 0){
                data_set = get_data_set();
                GET_RESOURCE()
                free_data_set(data_set);

                strncat(cmd, " -r ", sizeof(cmd)-strlen(cmd)-1);
                strncat(cmd, argv[1], sizeof(cmd)-strlen(cmd)-1);
        }

        strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

        if ((fstream = popen(cmd, "r")) == NULL){
                mgmt_log(LOG_ERR, "error on popen %s: %s", cmd, strerror(errno));
                return strdup(MSG_FAIL"\nDo crm_resource command failed");
        }
        ret = strdup(MSG_OK);

        while (!feof(fstream)){
                memset(buf, 0, sizeof(buf));
                if (fgets(buf, sizeof(buf), fstream) != NULL){
                        ret = mgmt_msg_append(ret, buf);
                        ret[strlen(ret)-1] = '\0';
                }
                else{
                        sleep(1);
                }
        }

        if (pclose(fstream) == -1){
                mgmt_log(LOG_WARNING, "failed to close pipe");
        }

        return ret;
}

static char*
get_localnodeinfo(void)
{
        static struct utsname   u;
	char * localnodename = NULL;

        if (uname(&u) < 0) {
                cl_perror("uname(2) call failed");
                return 0;
        }

        localnodename = u.nodename;

        return localnodename;
}

/* get local node*/
char*
on_get_localnode(char* argv[], int argc)
{
        char* ret = strdup(MSG_OK);
        static struct utsname   u;
	char * localnodename = NULL;

        if (uname(&u) < 0) {
                cl_perror("uname(2) call failed");
                return strdup(MSG_FAIL);
        }

        localnodename = u.nodename;

        ret = mgmt_msg_append(ret, localnodename);
        return ret;
}

char*
on_status_of_hblinks(char* argv[], int argc)
{
        char* ret = strdup(MSG_OK);
        cs_error_t result;
        corosync_cfg_handle_t handle;
        unsigned int interface_count;
        char **interface_names;
        char **interface_status;
        unsigned int i;
        unsigned int nodeid;

        result = corosync_cfg_initialize (&handle, NULL);
        if (result != CS_OK) {
                mgmt_log(LOG_WARNING, "Could not initialize corosync configuration API error %d\n", result);
                exit (1);
        }

        result = corosync_cfg_local_get(handle, &nodeid);
        if (result != CS_OK) {
                mgmt_log(LOG_WARNING, "Could not get the local node id, the error is: %d\n", result);
        }

        result = corosync_cfg_ring_status_get (handle,
                                &interface_names,
                                &interface_status,
                                &interface_count);
        if (result != CS_OK) {
                mgmt_log(LOG_WARNING, "Could not get the ring status, the error is: %d\n", result);
        } else {
                for (i = 0; i < interface_count; i++) {
                        ret = mgmt_msg_append(ret, interface_names[i]);
                        ret = mgmt_msg_append(ret, interface_status[i]);
                }
        }
        (void)corosync_cfg_finalize (handle);

        return ret;
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
        ssize_t n, rc;
        char c, *ptr;
        ptr = vptr;
        for (n = 1; n < maxlen; n++) {
                again:
                if ( (rc = read(fd, &c, 1)) == 1) {
                        if (c == '\n')
                                break; /* newline is stored, like fgets() */
                        *ptr++ = c;
                } else if (rc == 0) {
                        *ptr = 0;
                        return(n - 1); /* EOF, n - 1 bytes were read */
                } else {
                        if (errno == EINTR)
                                goto again;
                        return(-1);  /* error, errno set by read() */
                }
        }
        *ptr = 0; /* null terminate like fgets() */
        return(n);
}

char *
on_get_file_context(char* argv[], int argc)
{
        char * filename;
        char * buffer;
        char *ret;
        int  readlen;
        static int filefd;

        ARGC_CHECK(2);

        /* open file*/
        filename = argv[1];
        filefd = open(filename, O_RDONLY);
        if ( filefd == -1 )
        {
                close(filefd);
                return  strdup(MSG_FAIL);
        }
        /* read file*/
        buffer = (char *)malloc(200);

        ret = strdup(MSG_OK);
        readlen = readline(filefd, buffer, 200);
        while (readlen > 0)
        {
                ret = mgmt_msg_append(ret, buffer);
                readlen = readline(filefd, buffer, 200);
        }
        close(filefd);
        if (readlen == -1)
        {
                return strdup(MSG_FAIL);
        }else{
                return ret;
        }
}

char *
on_save_file_context(char* argv[], int argc)
{
	char * filename;
        char buffer[MAX_STRLEN];
        FILE *fstream = NULL;
        char cmd[MAX_STRLEN];
        int i;
        char *ret = strdup(MSG_OK);
	node_t *node;
        GList* cur;
        pe_working_set_t* data_set;
	const char* hostname = NULL;

        if (argc < 4) {                                      \
                mgmt_log(LOG_DEBUG, "%s msg should have at least %d params, but %d given",argv[0],4,argc);       \
                return strdup(MSG_FAIL"\nwrong parameter number");                      \
        }

        filename = argv[3];
	if ((fstream = fopen(filename, "w+")) == NULL) /* open file*/
        {
                return  strdup(MSG_FAIL);
        }

        for (i = 4; i < argc; i++)
        {
		fprintf(fstream, "%s\n", argv[i]);
        }

	fclose(fstream); /* close file */

        if (strcmp(argv[2], "true") == 0) /*chmod +x file*/
        {
                strncpy(cmd, "chmod +x ", sizeof(cmd)-1) ;
                strncat(cmd, filename, sizeof(cmd)-strlen(cmd)-1);

                if ((fstream = popen(cmd, "r")) == NULL){
                        mgmt_log(LOG_ERR, "error on popen %s: %s", cmd, strerror(errno));
                        return strdup(MSG_FAIL"\nDo crm_node command failed");
                }

                while (!feof(fstream)){
                        memset(buffer, 0, sizeof(buffer));
                        if (fgets(buffer, sizeof(buffer), fstream) != NULL){
                                ret = mgmt_msg_append(ret, buffer);
                        }
                        else{
                                sleep(1);
                        }
                }
                if (fclose(fstream) == -1){
                         mgmt_log(LOG_WARNING, "failed to fclose stream");
               }
        }

	if (strcmp(argv[1], "true") == 0)
        {
                /*copy to other node*/
                data_set = get_data_set();
                cur = data_set->nodes;

                while(cur!= NULL)
                {
                        node = (node_t*) cur->data;
			cur = g_list_next(cur);

			if (node->details->type==node_ping){
                                continue;
			}

			if (strcmp(get_localnodeinfo(), node->details->uname) == 0){
                                continue;
                        }

			hostname = strdup(node->details->uname);
                        ret = mgmt_msg_append(ret, hostname);

			if (!(node->details->online)){
                                ret = mgmt_msg_append(ret, "not online");
                                continue;
                        }

                        strncpy(cmd, "scp ", sizeof(cmd)-1) ;
                        strncat(cmd, filename, sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, " root@", sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, hostname, sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, ":", sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, filename, sizeof(cmd)-strlen(cmd)-1);
                        strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

                        if ((fstream = popen(cmd, "r")) == NULL){
                                mgmt_log(LOG_ERR, "error on popen %s: %s", cmd, strerror(errno));
                                return strdup(MSG_FAIL"\nDo crm_node command failed");
                        }

                        while (!feof(fstream)){
                                memset(buffer, 0, sizeof(buffer));
                                if (fgets(buffer, sizeof(buffer), fstream) != NULL){
                                        ret = mgmt_msg_append(ret, buffer);
                                }
                                else{
                                        sleep(1);
                                }
                        }

                        if (fclose(fstream) == -1){
                                mgmt_log(LOG_WARNING, "failed to fclose stream");
                        }
                }
        }
        return ret;
}

char *
on_get_coro_config(char* argv[], int argc)
{
       pe_working_set_t* data_set;
       node_t *dc = NULL;
       char *ret = strdup(MSG_OK);

       ARGC_CHECK(1)
       data_set = get_data_set();
       dc = data_set->dc_node;
       if(dc != NULL) {
               const char *quorum = crm_element_value(data_set->input, XML_ATTR_HAVE_QUORUM);
               mgmt_msg_append(ret, crm_is_true(quorum)?"with":"WITHOUT");
       }
       free_data_set(data_set);
       return ret;
}

char*
on_get_totem(char* argv[], int argc)
{
        char cmd[MAX_STRLEN];
        char* ret = NULL;
        FILE* fstream = NULL;
        char buf[MAX_STRLEN];

        ARGC_CHECK(2)

        snprintf(cmd, sizeof(cmd), "corosync-objctl -b totem %s", argv[1]);
        strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

        if ((fstream = popen(cmd, "r")) == NULL){
                mgmt_log(LOG_ERR, "error on popen %s: %s",
                         cmd, strerror(errno));
                return strdup(MSG_FAIL);
        }

        ret = strdup(MSG_FAIL);
        while (!feof(fstream)){
                memset(buf, 0, sizeof(buf));
                if (fgets(buf, sizeof(buf), fstream) != NULL){
                        ret = mgmt_msg_append(ret, buf);
                        ret[strlen(ret)-1] = '\0';
                }
                else{
                        sleep(1);
                }
        }

        if (pclose(fstream) == -1)
                mgmt_log(LOG_WARNING, "failed to close pipe");

        return ret;
}

char*
on_pwd_encode(char* argv[], int argc)
{
        char* ret = strdup(MSG_OK);
	char * encode;
        ARGC_CHECK(2)
	encode = malloc(200);
	pwd_encode(argv[1], &encode);
        ret = mgmt_msg_append(ret, encode);
	free(encode);
        return ret;
}

char*
on_pwd_decode(char* argv[], int argc)
{
        char* ret = strdup(MSG_OK);
	char * decode;
        ARGC_CHECK(2)
	decode = malloc(200);
	pwd_decode(argv[1], &decode);
        ret = mgmt_msg_append(ret, decode);
	free(decode);
        return ret;
}

char*
on_get_sqldata(char* argv[], int argc)
{
	char* ret = strdup(MSG_OK);
	sqlite3 *db;
	char dbPath[MAX_STRLEN];
	char *szErrMsg = 0;
	char **dbResult;
	int nRow, nColumn;
	int i , j;
	int index; 

	ARGC_CHECK(3)

	if (strcmp(argv[1], "") == 0) 
	{
		snprintf(dbPath, sizeof(dbPath), "/usr/lib/ocf/lib/heartbeat/db/%s.db", get_localnodeinfo());
	}else{
		snprintf(dbPath, sizeof(dbPath), "/usr/lib/ocf/lib/heartbeat/db/%s.db", argv[1]);
	}

	int rc= sqlite3_open(dbPath, &db);
	if( rc != SQLITE_OK )
	{
		ret = strdup(MSG_FAIL);
		ret = mgmt_msg_append(ret, sqlite3_errmsg(db));
		sqlite3_close(db);
		return ret;
	}

	rc = sqlite3_get_table(db, argv[2], &dbResult, &nRow, &nColumn, &szErrMsg);
	if( SQLITE_OK == rc )
	{
		index = nColumn;
		for( i = 0; i < nRow ; i++ )
		{
			char string[MAX_STRLEN];
			strncpy(string, "", sizeof(string)-1);
			for( j = 0 ; j < nColumn; j++ )
			{
				if (strnlen(string, MAX_STRLEN) != 0){
					strncat(string, "|", sizeof(string)-strlen(string)-1);
				}
				if (dbResult[index] != NULL){
					strncat(string, dbResult[index], sizeof(string)-strlen(string)-1);
				}
				index++;
			}
			ret = mgmt_msg_append(ret, string);
		}
	}else{
		ret = strdup(MSG_FAIL);
		ret = mgmt_msg_append(ret, szErrMsg);
	}

	sqlite3_free_table( dbResult );

	sqlite3_close(db);
	return ret;
}

char*
on_update_sqldata(char* argv[], int argc)
{
	char* ret = strdup(MSG_OK);
	sqlite3 *db;
	char dbPath[MAX_STRLEN];
	char *szErrMsg = 0;

	ARGC_CHECK(3)

	if (strcmp(argv[1], "") == 0) 
	{
		snprintf(dbPath, sizeof(dbPath), "/usr/lib/ocf/lib/heartbeat/db/%s.db", get_localnodeinfo());
	}else{
		snprintf(dbPath, sizeof(dbPath), "/usr/lib/ocf/lib/heartbeat/db/%s.db", argv[1]);
	}

	int rc= sqlite3_open(dbPath, &db);
	if( rc != SQLITE_OK )
	{
		ret = strdup(MSG_FAIL);
		ret = mgmt_msg_append(ret, sqlite3_errmsg(db));
		sqlite3_close(db);
		return ret;
	}

	rc=sqlite3_exec(db, argv[2] ,0, 0, &szErrMsg);

	if( SQLITE_OK != rc ){
		ret = strdup(MSG_FAIL);
		ret = mgmt_msg_append(ret, szErrMsg);
	}

	sqlite3_close(db);
	return ret;
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

    FD_SET(socket_fd, &fd);

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);

    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

    return rc;
}

char*
on_ssh_node(char* argv[], int argc)
{
    const char *hostname;
    const char *commandline;
    const char *username;
    const char *password;
    unsigned long hostaddr;
    int sock;
    struct sockaddr_in sin;
    const char *fingerprint;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;
    int rc;
    int exitcode;
    size_t len;
    LIBSSH2_KNOWNHOSTS *nh;
    int type;
    char* ret = strdup(MSG_OK);
    ARGC_CHECK(5)

#ifdef WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,0), &wsadata);
#endif
        /* must be ip address only */
    hostname = argv[1];
    username = argv[2];
    password = argv[3];
    commandline = argv[4];

    hostaddr = inet_addr(hostname);

    /* Ultra basic "connect to port 22 on localhost"
    * Your code is responsible for creating the socket establishing the
    * connection
    */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if (connect(sock, (struct sockaddr*)(&sin),
                sizeof(struct sockaddr_in)) != 0) {
        return strdup(MSG_FAIL"\nfailed to connect!");
    }

    /* Create a session instance */
    session = libssh2_session_init();
    if (!session)
        return strdup(MSG_FAIL"\nNo such command");

    /* tell libssh2 we want it all done non-blocking */
    libssh2_session_set_blocking(session, 0);

    /* ... start it up. This will trade welcome banners, exchange keys,
 *      * and setup crypto, compression, and MAC layers
 *           */
    while ((rc = libssh2_session_startup(session, sock)) ==
           LIBSSH2_ERROR_EAGAIN);
    if (rc) {
        return strdup(MSG_FAIL"\nFailure establishing SSH session");
    }

    nh = libssh2_knownhost_init(session);
    if(!nh) {
        /* eeek, do cleanup here */
        return strdup(MSG_FAIL"\n2");
    }

    /* read all hosts from here */
    libssh2_knownhost_readfile(nh, "known_hosts",
                               LIBSSH2_KNOWNHOST_FILE_OPENSSH);

    /* store all known hosts to here */
    libssh2_knownhost_writefile(nh, "dumpfile",
                                LIBSSH2_KNOWNHOST_FILE_OPENSSH);

    fingerprint = libssh2_session_hostkey(session, &len, &type);
    if(fingerprint) {
        struct libssh2_knownhost *host;
        /*int check = libssh2_knownhost_check(nh, (char *)hostname,*/
        libssh2_knownhost_check(nh, (char *)hostname,
                                            (char *)fingerprint, len,
                                            LIBSSH2_KNOWNHOST_TYPE_PLAIN|
                                            LIBSSH2_KNOWNHOST_KEYENC_RAW,
                                            &host);

        /*fprintf(stderr, "Host check: %d, key: %s\n", check,
                (check <= LIBSSH2_KNOWNHOST_CHECK_MISMATCH)?
                host->key:"<none>");*/
    }
    else {
        /* eeek, do cleanup here */
        return strdup(MSG_FAIL"\naa");
    }
    libssh2_knownhost_free(nh);

    if ( strlen(password) != 0 ) {
        /* We could authenticate via password */
        while ((rc = libssh2_userauth_password(session, username, password)) ==
               LIBSSH2_ERROR_EAGAIN);
        if (rc) {
            ret = strdup(MSG_FAIL);
            ret = mgmt_msg_append(ret,"Authentication by password failed.");
            goto shutdown;
        }
    }
    else {
        /* Or by public key */
        while ((rc = libssh2_userauth_publickey_fromfile(session, username,
                                                         "/home/user/"
                                                         ".ssh/id_rsa.pub",
                                                         "/home/user/"
                                                         ".ssh/id_rsa",
                                                         password)) ==
               LIBSSH2_ERROR_EAGAIN);
        if (rc) {
	    ret = strdup(MSG_FAIL);
            ret = mgmt_msg_append(ret, "\tAuthentication by public key failed");
            goto shutdown;
        }
    }

#if 0
    libssh2_trace(session, ~0 );
#endif

    /* Exec non-blocking on the remove host */
    while( (channel = libssh2_channel_open_session(session)) == NULL &&
           libssh2_session_last_error(session,NULL,NULL,0) ==
           LIBSSH2_ERROR_EAGAIN )
    {
        waitsocket(sock, session);
    }
    if( channel == NULL )
    {
        return strdup(MSG_FAIL"\nError!");
        exit( 1 );
    }
    while( (rc = libssh2_channel_exec(channel, commandline)) ==
           LIBSSH2_ERROR_EAGAIN )
    {
        waitsocket(sock, session);
    }
    if( rc != 0 )
    {
        return strdup(MSG_FAIL"\nError!");
        exit( 1 );
    }
    for( ;; )
    {
        /* loop until we block */
        int rc;
        do
        {
            char buffer[MAX_STRLEN];
	    memset(buffer, 0, sizeof(buffer));
            rc = libssh2_channel_read( channel, buffer, sizeof(buffer) );
	    if (LIBSSH2_ERROR_EAGAIN==rc)
            {
            	continue;
            }

            if( rc > 0 )
            {
                ret = mgmt_msg_append(ret, buffer);
            }
        }
        while( rc > 0 );

        /* this is due to blocking that would occur otherwise so we loop on
        this condition */
        if( rc == LIBSSH2_ERROR_EAGAIN )
        {
            waitsocket(sock, session);
        }
        else
            break;
    }
    exitcode = 127;
    while( (rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN )
        waitsocket(sock, session);

    if( rc == 0 )
    {
        exitcode = libssh2_channel_get_exit_status( channel );
    }

    libssh2_channel_free(channel);
    channel = NULL;

shutdown:

    libssh2_session_disconnect(session,
                               "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);

#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return ret;
}

/*neoshineha ming.liu add end*/
