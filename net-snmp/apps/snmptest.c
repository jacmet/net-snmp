/*
 * snmptest.c - send snmp requests to a network entity.
 *
 */
/***********************************************************************
	Copyright 1988, 1989, 1991, 1992 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
#include <config.h>

#include <sys/types.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>

#include "snmp.h"
#include "asn1.h"
#include "snmp_impl.h"
#include "snmp_api.h"
#include "snmp_client.h"
#include "party.h"
#include "context.h"
#include "view.h"
#include "acl.h"

extern int  errno;
int command = GET_REQ_MSG;
int	snmp_dump_packet = 0;

usage(){
    fprintf(stderr, "Usage: snmptest -v 1 [-q] hostname community      or:\n");
    fprintf(stderr, "Usage: snmptest [-v 2] [-q] hostname noAuth       or:\n");
    fprintf(stderr, "Usage: snmptest [-v 2] [-q] hostname srcParty dstParty context\n");
}

main(argc, argv)
    int	    argc;
    char    *argv[];
{
    struct snmp_session session, *ss;
    struct snmp_pdu *pdu, *response, *copy = NULL;
    struct variable_list *vars, *vp;
    int	arg, ret, version = 2;
    char *hostname = NULL;
    char *community = NULL;
    int port_flag = 0;
    int dest_port = 0;
    oid src[MAX_NAME_LEN], dst[MAX_NAME_LEN], context[MAX_NAME_LEN];
    int srclen = 0, dstlen = 0, contextlen = 0;
    u_long      srcclock, dstclock;
    int clock_flag = 0;
    struct partyEntry *pp;
    struct contextEntry *cxp;
    int	    status, count;
    char	input[128];
    int varcount, nonRepeaters = -1, maxRepetitions;
    int timeout_flag = 0, timeout, retransmission_flag = 0, retransmission;
    int trivialSNMPv2 = FALSE;
    struct hostent *hp;
    u_long destAddr;

    init_mib();
    /* Usage: snmptest -v 1 [-q] hostname community [objectID]      or:
     * Usage: snmptest [-v 2] [-q] hostname noAuth [objectID]       or:
     * Usage: snmptest [-v 2] [-q] hostname srcParty dstParty context [objectID]
     */
    for(arg = 1; arg < argc; arg++){
	if (argv[arg][0] == '-'){
	    switch(argv[arg][1]){
		case 'd':
		    snmp_dump_packet++;
		    break;
		case 'q':
		    quick_print++;
		    break;
                case 'p':
                    port_flag++;
                    dest_port = atoi(argv[++arg]);
                    break;
                case 't':
                    timeout_flag++;
                    timeout = atoi(argv[++arg]) * 1000000L;
                    break;
                case 'r':
                    retransmission_flag++;
                    retransmission = atoi(argv[++arg]);
                    break;
                case 'c':
                    clock_flag++;
                    srcclock = atoi(argv[++arg]);
                    dstclock = atoi(argv[++arg]);
                    break;
                case 'v':
                    version = atoi(argv[++arg]);
		    if (version < 1 || version > 2){
			fprintf(stderr, "Invalid version\n");
			usage();
			exit(1);
		    }
                    break;
		default:
		    fprintf(stderr, "invalid option: -%c\n", argv[arg][1]);
		    break;
	    }
	    continue;
	}
	if (hostname == NULL){
	    hostname = argv[arg];
	} else if (version == 1 && community == NULL){
	    community = argv[arg]; 
	} else if (version == 2 && srclen == 0 && !trivialSNMPv2) {
            if (read_party_database("/etc/party.conf") > 0){
                fprintf(stderr,
                        "Couldn't read party database from /etc/party.conf\n");
                exit(0);
            }
            if (read_context_database("/etc/context.conf") > 0){
                fprintf(stderr,
                        "Couldn't read context database from /etc/context.conf\n");
                exit(0);
            }
            if (read_acl_database("/etc/acl.conf") > 0){
                fprintf(stderr,
                        "Couldn't read access control database from /etc/acl.conf\n");
                exit(0);
            }
            if (!strcasecmp(argv[arg], "noauth")){
                trivialSNMPv2 = TRUE;
            } else {
		party_scanInit();
		for(pp = party_scanNext(); pp; pp = party_scanNext()){
		    if (!strcasecmp(pp->partyName, argv[arg])){
			srclen = pp->partyIdentityLen;
#ifdef SVR4
			memmove(src, pp->partyIdentity, srclen * sizeof(oid));
#else
			bcopy(pp->partyIdentity, src, srclen * sizeof(oid));
#endif
			break;
		    }
		}
		if (!pp){
		    srclen = MAX_NAME_LEN;
		    if (!read_objid(argv[arg], src, &srclen)){
			printf("Invalid source party: %s\n", argv[arg]);
			srclen = 0;
			usage();
			exit(1);
		    }
		}
	    }
	} else if (version == 2 && dstlen == 0 && !trivialSNMPv2){
            dstlen = MAX_NAME_LEN;
            party_scanInit();
            for(pp = party_scanNext(); pp; pp = party_scanNext()){
                if (!strcasecmp(pp->partyName, argv[arg])){
                    dstlen = pp->partyIdentityLen;
#ifdef SVR4
                    memmove(dst, pp->partyIdentity, dstlen * sizeof(oid));
#else
                    bcopy(pp->partyIdentity, dst, dstlen * sizeof(oid));
#endif
                    break;
                }
            }
            if (!pp){
                if (!read_objid(argv[arg], dst, &dstlen)){
                    printf("Invalid destination party: %s\n", argv[arg]);
                    dstlen = 0;
		    usage();
                    exit(1);
		}
            }
	} else if (version == 2 && contextlen == 0 && !trivialSNMPv2){
	    contextlen = MAX_NAME_LEN;
	    context_scanInit();
	    for(cxp = context_scanNext(); cxp; cxp = context_scanNext()){
		if (!strcasecmp(cxp->contextName, argv[arg])){
		    contextlen = cxp->contextIdentityLen;
#ifdef SVR4
		    memmove(context, cxp->contextIdentity,
			  contextlen * sizeof(oid));
#else
		    bcopy(cxp->contextIdentity, context,
			  contextlen * sizeof(oid));
#endif
		    break;
		}
	    }
	    if (!cxp){
		if (!read_objid(argv[arg], context, &contextlen)){
		    printf("Invalid context: %s\n", argv[arg]);
		    contextlen = 0;
		    usage();
		    exit(1);
		}
	    }
	} else {
	    usage();
	    exit(1);
	}
    }
    if (version == 1 && community == NULL)
	community = "public";	/* default to public */

    if (!hostname || (version < 1) || (version > 2)
      || (version == 1 && !community)
      || (version == 2 && (!srclen || !dstlen || !contextlen)
          && !trivialSNMPv2)){
              usage();
              exit(1);
    }

    if (trivialSNMPv2){
      if ((destAddr = inet_addr(hostname)) == -1){
          hp = gethostbyname(hostname);
          if (hp == NULL){
              fprintf(stderr, "unknown host: %s\n", hostname);
              exit(1);
          } else {
#ifdef SVR4
              memmove((char *)&destAddr, (char *)hp->h_addr,
                    hp->h_length);
#else
              bcopy((char *)hp->h_addr, (char *)&destAddr,
                    hp->h_length);
#endif
          }
      }
      srclen = dstlen = contextlen = MAX_NAME_LEN;
      ms_party_init(destAddr, src, &srclen, dst, &dstlen,
                    context, &contextlen);
    }

    if (clock_flag){
        pp = party_getEntry(src, srclen);
        if (pp){
            pp->partyAuthClock = srcclock;
            gettimeofday(&pp->tv, (struct timezone *)0);
            pp->tv.tv_sec -= pp->partyAuthClock;
        }
        pp = party_getEntry(dst, dstlen);
        if (pp){
            pp->partyAuthClock = dstclock;
            gettimeofday(&pp->tv, (struct timezone *)0);
            pp->tv.tv_sec -= pp->partyAuthClock;
        }
    }

#ifdef SVR4
    memset((char *)&session, NULL, sizeof(struct snmp_session));
#else
    bzero((char *)&session, sizeof(struct snmp_session));
#endif
    session.peername = hostname;
    if (port_flag)
        session.remote_port = dest_port;
    if (version == 1){
	session.version = SNMP_VERSION_1;
        session.community = (u_char *)community;
        session.community_len = strlen((char *)community);
    } else if (version == 2){
	session.version = SNMP_VERSION_2;
        session.srcParty = src;
        session.srcPartyLen = srclen;
        session.dstParty = dst;
        session.dstPartyLen = dstlen;
        session.context = context;
        session.contextLen = contextlen;
    }
    if (retransmission_flag)
	session.retries = retransmission;
    else
	session.retries = SNMP_DEFAULT_RETRIES;
    if (timeout_flag)
	session.timeout = timeout;
    else
	session.timeout = SNMP_DEFAULT_TIMEOUT;
    session.authenticator = NULL;
    snmp_synch_setup(&session);
    ss = snmp_open(&session);
    if (ss == NULL){
	fprintf(stderr, "Couldn't open snmp\n");
	exit(-1);
    }

    varcount = 0;
    while(1){
	vars = NULL;
	for(ret = 1; ret != 0;){
	    vp = (struct variable_list *)malloc(sizeof(struct variable_list));
	    vp->next_variable = NULL;
	    vp->name = NULL;
	    vp->val.string = NULL;

	    while((ret = input_variable(vp)) == -1)
		;
	    if (ret == 1){
		varcount++;
		/* add it to the list */
		if (vars == NULL){
		    /* if first variable */
		    pdu = snmp_pdu_create(command);
		    pdu->variables = vp;
		} else {
		    vars->next_variable = vp;
		}
		vars = vp;
	    } else {
		/* free the last (unused) variable */
		if (vp->name)
		    free((char *)vp->name);
		if (vp->val.string)
		    free((char *)vp->val.string);
		free((char *)vp);

		if (command == BULK_REQ_MSG){
		    if (nonRepeaters == -1){
			nonRepeaters = varcount;
			ret = -1;	/* so we collect more variables */
			printf("Now input the repeating variables\n");
		    } else {
			printf("What repeat count? ");
			fflush(stdout);
			gets(input);
			maxRepetitions = atoi(input);
			pdu->non_repeaters = nonRepeaters;
			pdu->max_repetitions = maxRepetitions;
		    }
		}
	    }
	    if (varcount == 0 && ret == 0){
		if (!copy){
		    printf("No PDU to send.\n");
		    ret = -1;
		} else {
		    pdu = snmp_clone_pdu(copy);
		    printf("Resending last PDU.\n");
		}
	    }
	}
	copy = snmp_clone_pdu(pdu);
	if (command == TRP2_REQ_MSG){
	    /* No response needed */
	    if (!snmp_send(ss, pdu)){
		printf("Couldn't send SNMPv2 trap.\n");
	    }
	} else {
	    status = snmp_synch_response(ss, pdu, &response);
	    if (status == STAT_SUCCESS){
		if (command == INFORM_REQ_MSG &&
		    response->errstat == SNMP_ERR_NOERROR){
		    printf("Inform Acknowledged\n");
		} else {
		    switch(response->command){
		      case GET_REQ_MSG:
			printf("Received Get Request ");
			break;
		      case GETNEXT_REQ_MSG:
			printf("Received Getnext Request ");
			break;
		      case GET_RSP_MSG:
			printf("Received Get Response ");
			break;
		      case SET_REQ_MSG:
			printf("Received Set Request ");
			break;
		      case TRP_REQ_MSG:
			printf("Received Trap Request ");
			break;
		      case BULK_REQ_MSG:
			printf("Received Bulk Request ");
			break;
		      case INFORM_REQ_MSG:
			printf("Received Inform Request ");
			break;
		      case TRP2_REQ_MSG:
			printf("Received SNMPv2 Trap Request ");
			break;
		    }
		    printf("from %s\n", inet_ntoa(response->address.sin_addr));
		    printf("requestid 0x%X errstat 0x%X errindex 0x%X\n",
			   response->reqid, response->errstat,
			   response->errindex);
		    if (response->errstat == SNMP_ERR_NOERROR){
			for(vars = response->variables; vars;
			    vars = vars->next_variable)
			    print_variable(vars->name, vars->name_length,
					   vars);
		    } else {
			fprintf(stderr, "Error in packet.\nReason: %s\n",
				snmp_errstring(response->errstat));
			if (response->errstat == SNMP_ERR_NOSUCHNAME){
			    for(count = 1, vars = response->variables;
				vars && count != response->errindex;
				vars = vars->next_variable, count++)
				;
			    if (vars){
				printf("This name doesn't exist: ");
				print_objid(vars->name, vars->name_length);
			    }
			    printf("\n");
			}
		    }
		}
	    } else if (status == STAT_TIMEOUT){
		fprintf(stderr, "No Response from %s\n", hostname);
	    } else {    /* status == STAT_ERROR */
		fprintf(stderr, "An error occurred, Quitting\n");
	    }
	    
	    if (response)
		snmp_free_pdu(response);
	}
	varcount = 0;
	nonRepeaters = -1;
    }
}

int
ascii_to_binary(cp, bufp)
    u_char  *cp;
    u_char *bufp;
{
    int	subidentifier;
    u_char *bp = bufp;

    for(; *cp != '\0'; cp++){
	if (isspace(*cp))
	    continue;
	if (!isdigit(*cp)){
	    fprintf(stderr, "Input error\n");
	    return -1;
	}
	subidentifier = atoi(cp);
	if (subidentifier > 255){
	    fprintf(stderr, "subidentifier %d is too large ( > 255)\n",
		    subidentifier);
	    return -1;
	}
	*bp++ = (u_char)subidentifier;
	while(isdigit(*cp))
	    cp++;
	cp--;
    }
    return bp - bufp;
}


int
hex_to_binary(cp, bufp)
    u_char  *cp;
    u_char *bufp;
{
    int	subidentifier;
    u_char *bp = bufp;

    for(; *cp != '\0'; cp++){
	if (isspace(*cp))
	    continue;
	if (!isxdigit(*cp)){
	    fprintf(stderr, "Input error\n");
	    return -1;
	}
	sscanf((char *)cp, "%x", &subidentifier);
	if (subidentifier > 255){
	    fprintf(stderr, "subidentifier %d is too large ( > 255)\n",
		    subidentifier);
	    return -1;
	}
	*bp++ = (u_char)subidentifier;
	while(isxdigit(*cp))
	    cp++;
	cp--;
    }
    return bp - bufp;
}


input_variable(vp)
    struct variable_list    *vp;
{
    char  buf[256];
    u_char value[256], ch;
    oid name[MAX_NAME_LEN];

    printf("Variable: ");
    fflush(stdout);
    gets(buf);

    if (*buf == 0){
	vp->name_length = 0;
	return 0;
    }
    if (*buf == '$'){
	switch(buf[1]){
	    case 'G':
		command = GET_REQ_MSG;
		printf("Request type is Get Request\n");
		break;
	    case 'N':
		command = GETNEXT_REQ_MSG;
		printf("Request type is Getnext Request\n");
		break;
	    case 'S':
		command = SET_REQ_MSG;
		printf("Request type is Set Request\n");
		break;
	    case 'B':
		command = BULK_REQ_MSG;
		printf("Request type is Bulk Request\n");
		printf("Enter a blank line to terminate the list of non-repeaters\n");
		printf("and to begin the repeating variables\n");
		break;
	    case 'I':
		command = INFORM_REQ_MSG;
		printf("Request type is Inform Request\n");
		printf("(Are you sending to the right port?)\n");
		break;
	    case 'T':
		command = TRP2_REQ_MSG;
		printf("Request type is SNMPv2 Trap Request\n");
		printf("(Are you sending to the right port?)\n");
		break;
	    case 'D':
		if (snmp_dump_packet){
		    snmp_dump_packet = 0;
		    printf("Turned packet dump off\n");
		} else {
		    snmp_dump_packet = 1;
		    printf("Turned packet dump on\n");
		}
		break;
	    case 'Q':
		switch(buf[2]){
		    case NULL:
		        printf("Quitting,  Goodbye\n");
			exit(0);
			break;
		    case 'P':
			if (quick_print){
			   quick_print = 0;
			   printf("Turned quick printing off\n");
			} else {
			   quick_print = 1;
			   printf("Turned quick printing on\n");
			}
			break;
		}
		break;
	    default:
		fprintf(stderr, "Bad command\n");
	}
	return -1;
    }
    vp->name_length = MAX_NAME_LEN;
    if (!read_objid(buf, name, &vp->name_length))
	return -1;
    vp->name = (oid *)malloc(vp->name_length * sizeof(oid));
#ifdef SVR4
    memmove((char *)vp->name, (char *)name, vp->name_length * sizeof(oid));
#else
    bcopy((char *)name, (char *)vp->name, vp->name_length * sizeof(oid));
#endif

    if (command == SET_REQ_MSG || command == INFORM_REQ_MSG
	|| command == TRP2_REQ_MSG){
	printf("Type [i|s|x|d|n|o|t|a]: ");
	fflush(stdout);
	gets(buf);
	ch = *buf;
	switch(ch){
	    case 'i':
		vp->type = INTEGER;
		break;
	    case 's':
		vp->type = STRING;
		break;
	    case 'x':
		vp->type = STRING;
		break;
	    case 'd':
		vp->type = STRING;
		break;
	    case 'n':
		vp->type = NULLOBJ;
		break;
	    case 'o':
		vp->type = OBJID;
		break;
	    case 't':
		vp->type = TIMETICKS;
		break;
	    case 'a':
		vp->type = IPADDRESS;
		break;
	    default:
		fprintf(stderr, "bad type \"%c\", use \"i\", \"s\", \"x\", \"d\", \"n\", \"o\", \"t\", or \"a\".\n", *buf);
		return -1;
	}
	printf("Value: "); fflush(stdout);
	gets(buf);
	switch(vp->type){
	    case INTEGER:
		vp->val.integer = (long *)malloc(sizeof(long));
		*(vp->val.integer) = atoi(buf);
		vp->val_len = sizeof(long);
		break;
	    case STRING:
		if (ch == 'd'){
		    vp->val_len = ascii_to_binary((u_char *)buf, value);
		} else if (ch == 's'){
		    strcpy(value, buf);
		    vp->val_len = strlen(buf);
		} else if (ch == 'x'){
		    vp->val_len = hex_to_binary((u_char *)buf, value);
		}
		vp->val.string = (u_char *)malloc(vp->val_len);
#ifdef SVR4
		memmove((char *)vp->val.string, (char *)value, vp->val_len);
#else
		bcopy((char *)value, (char *)vp->val.string, vp->val_len);
#endif
		break;
	    case NULLOBJ:
		vp->val_len = 0;
		vp->val.string = NULL;
		break;
	    case OBJID:
		vp->val_len = MAX_NAME_LEN;;
		read_objid(buf, (oid *)value, &vp->val_len);
		vp->val_len *= sizeof(oid);
		vp->val.objid = (oid *)malloc(vp->val_len);
#ifdef SVR4
		memmove((char *)vp->val.objid, (char *)value, vp->val_len);
#else
		bcopy((char *)value, (char *)vp->val.objid, vp->val_len);
#endif
		break;
	    case TIMETICKS:
		vp->val.integer = (long *)malloc(sizeof(long));
		*(vp->val.integer) = atoi(buf);
		vp->val_len = sizeof(long);
		break;
	    case IPADDRESS:
		vp->val.integer = (long *)malloc(sizeof(long));
		*(vp->val.integer) = inet_addr(buf);
		vp->val_len = sizeof(long);
		break;
	    default:
		fprintf(stderr, "Internal error\n");
		break;
	}
    } else {	/* some form of get message */
	vp->type = NULLOBJ;
	vp->val_len = 0;
    }
    return 1;
}

