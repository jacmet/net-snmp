/*
 *  AgentX sub-agent
 */
#include "config.h"

#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/errno.h>

#include "asn1.h"
#include "snmp_api.h"
#include "snmp_impl.h"
#include "snmp_client.h"
#include "snmp.h"

#include "snmp_vars.h"
#include "snmp_agent.h"
#include "var_struct.h"
#include "snmpd.h"
#include "agentx/protocol.h"
#include "agentx/client.h"
#include "default_store.h"
#include "ds_agent.h"
#include "callback.h"
#include "snmp_debug.h"


int
handle_agentx_packet(int operation, struct snmp_session *session, int reqid,
                   struct snmp_pdu *pdu, void *magic)
{
    struct agent_snmp_session  *asp;
    int status, allDone, i;
    struct variable_list *var_ptr, *var_ptr2;

    asp = init_agent_snmp_session( session, pdu );

    DEBUGMSGTL(("agentx/subagent","handling agentx request....\n"));
    switch (pdu->command) {
    case AGENTX_MSG_GET:
        DEBUGMSGTL(("agentx/subagent","  -> get\n"));
	status = handle_next_pass( asp );
	break;

    case AGENTX_MSG_GETBULK:
        DEBUGMSGTL(("agentx/subagent","  -> getbulk\n"));
	    /*
	     * GETBULKS require multiple passes. The first pass handles the
	     * explicitly requested varbinds, and subsequent passes append
	     * to the existing var_op_list.  Each pass (after the first)
	     * uses the results of the preceeding pass as the input list
	     * (delimited by the start & end pointers.
	     * Processing is terminated if all entries in a pass are
	     * EndOfMib, or the maximum number of repetitions are made.
	     */
	asp->exact   = FALSE;
		/*
		 * Limit max repetitions to something reasonable
		 *	XXX: We should figure out what will fit somehow...
		 */
	if ( asp->pdu->errindex > 100 )
	    asp->pdu->errindex = 100;

	status = handle_next_pass( asp );	/* First pass */
	if ( status != SNMP_ERR_NOERROR )
	    break;

	while ( asp->pdu->errstat-- > 0 )	/* Skip non-repeaters */
	    asp->start = asp->start->next_variable;

	while ( asp->pdu->errindex-- > 0 ) {	/* Process repeaters */
		/*
		 * Add new variable structures for the
		 * repeating elements, ready for the next pass.
		 * Also check that these are not all EndOfMib
		 */
	    allDone = TRUE;		/* Check for some content */
	    for ( var_ptr = asp->start;
		  var_ptr != asp->end->next_variable;
		  var_ptr = var_ptr->next_variable ) {
				/* XXX: we don't know the size of the next
					OID, so assume the maximum length */
		var_ptr2 = snmp_add_null_var(asp->pdu, var_ptr->name, MAX_OID_LEN);
		for ( i=var_ptr->name_length ; i<MAX_OID_LEN ; i++)
		    var_ptr2->name[i] = '\0';
		var_ptr2->name_length = var_ptr->name_length;

		if ( var_ptr->type != SNMP_ENDOFMIBVIEW )
		    allDone = FALSE;
	    }
	    if ( allDone )
		break;

	    asp->start = asp->end->next_variable;
	    while ( asp->end->next_variable != NULL )
		asp->end = asp->end->next_variable;
	    
	    status = handle_next_pass( asp );
	    if ( status != SNMP_ERR_NOERROR )
		break;
	}
	break;

    case AGENTX_MSG_GETNEXT:
        DEBUGMSGTL(("agentx/subagent","  -> getnext\n"));
	asp->exact   = FALSE;
	status = handle_next_pass( asp );
	break;

    case AGENTX_MSG_TESTSET:
        DEBUGMSGTL(("agentx/subagent","  -> testset\n"));
    	    /*
	     * In the UCD architecture the first two passes through var_op_list
	     * verify that all types, lengths, and values are valid
	     * and may reserve resources.
	     * These correspond to the first AgentX pass - TESTSET
	     */
	asp->rw      = WRITE;

        asp->mode = RESERVE1;
	status = handle_next_pass( asp );

	if ( status != SNMP_ERR_NOERROR ) {
	    asp->mode = FREE;
	    (void) handle_next_pass( asp );
	    break;
	}

        asp->mode = RESERVE2;
	status = handle_next_pass( asp );

	if ( status != SNMP_ERR_NOERROR ) {
	    asp->mode = FREE;
	    (void) handle_next_pass( asp );
	}
	break;


	    /*
	     * The third and fourth passes in the UCD architecture
	     *   correspond to distinct AgentX passes,
	     *   as does the "undo" pass, in case of errors elsewhere
	     */
    case AGENTX_MSG_COMMITSET:
        DEBUGMSGTL(("agentx/subagent","  -> commitset\n"));
        asp->mode = ACTION;
	status = handle_next_pass( asp );

	if ( status != SNMP_ERR_NOERROR ) {
	    asp->mode = UNDO;
	    (void) handle_next_pass( asp );
	}
	break;

    case AGENTX_MSG_CLEANUPSET:
        DEBUGMSGTL(("agentx/subagent","  -> cleanupset\n"));
        asp->mode = COMMIT;
	status = handle_next_pass( asp );
	break;

    case AGENTX_MSG_UNDOSET:
        DEBUGMSGTL(("agentx/subagent","  -> undoset\n"));
        asp->mode = UNDO;
	status = handle_next_pass( asp );
	break;

    case AGENTX_MSG_RESPONSE:
        DEBUGMSGTL(("agentx/subagent","  -> response\n"));
	free( asp );
	return 0;

    default:
        DEBUGMSGTL(("agentx/subagent","  -> unknown\n"));
	free( asp );
	return 0;
    }
	
    
	
    if ( asp->outstanding_requests == NULL ) {
	asp->pdu->command = AGENTX_MSG_RESPONSE;
	asp->pdu->errstat = status;
	snmp_send( asp->session, asp->pdu );
	free( asp );
    }
    DEBUGMSGTL(("agentx/subagent","  FINISHED\n\n"));

    return 1;
}






struct snmp_session *agentx_session;

int
subagent_shutdown(int majorID, int minorID, void *serverarg, void *clientarg) {
  struct snmp_session *thesession = (struct snmp_session *) clientarg;
  DEBUGMSGTL(("agentx/subagent","shutting down session....\n"));
  if (thesession == NULL) {
    DEBUGMSGTL(("agentx/subagent","Empty session to shutdown\n"));
    return 0;
  }
  agentx_close_session(thesession);
  snmp_close(thesession);
  free(thesession);
  DEBUGMSGTL(("agentx/subagent","shut down finished.\n"));
  return 1;
}

void
init_subagent( void )
{
    struct snmp_session
                        sess,
                       *session=&sess;

    DEBUGMSGTL(("agentx/subagent","initializing....\n"));
    if ( ds_get_boolean(DS_APPLICATION_ID, DS_AGENT_ROLE) != SUB_AGENT )
	return;

    memset(session, 0, sizeof(struct snmp_session));
    session->version = AGENTX_VERSION_1;
    session->peername = strdup(AGENTX_SOCKET);
    session->retries = SNMP_DEFAULT_RETRIES;
    session->timeout = SNMP_DEFAULT_TIMEOUT;
    session->flags  |= SNMP_FLAGS_STREAM_SOCKET;
     
    session->local_port = 0;	/* client */
    session->callback = handle_agentx_packet;
    session->authenticator = NULL;
    agentx_session = snmp_open( &sess );

    if ( agentx_session == NULL ) {
	snmp_sess_perror("init_subagent", &sess);
	exit(1);
    }

    set_parse( agentx_session, agentx_parse );
    set_build( agentx_session, agentx_build );

    if ( agentx_open_session( agentx_session ) < 0 ) {
	snmp_close( agentx_session );
	free( agentx_session );
	exit(1);
    }
    snmp_register_callback(SNMP_CALLBACK_LIBRARY, SNMP_CALLBACK_SHUTDOWN,
                           subagent_shutdown, agentx_session);
    DEBUGMSGTL(("agentx/subagent","initializing....  DONE\n"));
}



