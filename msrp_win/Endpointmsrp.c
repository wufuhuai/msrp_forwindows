#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "msrp.h"

#include "Turn_LinuxToWin.h"

/* Callbacks from MSRP library */
void callback_msrp(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes);
void events_msrp(int event, void *info);
/* Helper to load and parse a configuration file */
int load_config_file(char *filename);

/*
Global variables to store the configuration values:

	debug		= 0/1 (whether to show or not error and log event notifications)
	call_id		= the SIP Call-ID
	label		= the SIP label associated with the MSRP stream
	active		= 0/1 (0=passive=server or 1=active=client)
	address		= our address
	display		= our display name (nick, only for visualization)
	port		= our listening port

MSRP URIs (both From-Path and To-Path) are formed as such:
	msrp://address:port/session:tcp
e.g.:
	msrp://atlanta.example.com:7654/jshA7weztas;tcp

*/
int active, debug, label, be_alive;
char *call_id, *address, *display, *peerdisplay;
unsigned short int port;

int main(int argc, char *argv[])
{
	char client_config[] = "client.conf";
	MsrpEndpoint *endpoint = NULL;
	int content = 0;
	int flags = 0;
	char *path = NULL;
	char *lineptr = NULL, *command = NULL;
	if(load_config_file(client_config) < 0)
		return -1;
	/* Initialize the library, by passing the callback to receive events */
	if(msrp_init(events_msrp) < 0) {
		printf("Error initializing the MSRP library...\n");
		return -1;
	}
	printf("\nMSRP library initialized.\n");
	/* Setup the callback to receive endpoint related info */
	msrp_ep_callback(callback_msrp);
	
	/* Create a new endpoint instance */
	endpoint = msrp_endpoint_new();
	if(!endpoint) {
		printf("Error creating new endpoint...\n");
		return -1;
	}
	printf("New endpoint (ID %lu) created:\n", endpoint->ID);
	msrp_endpoint_set_callid(endpoint, call_id);
	printf("\tCall-ID:\t%s\n", msrp_endpoint_get_callid(endpoint));
	msrp_endpoint_set_label(endpoint, label);
	printf("\tLabel:\t\t%d\n", msrp_endpoint_get_label(endpoint));

	/* Create a new peer for us in the endpoint */
	content = MSRP_TEXT_PLAIN | MSRP_TEXT_HTML;
	flags = MSRP_OVER_TCP;
	if(active)
		flags |= MSRP_ACTIVE;
	else
		flags |= MSRP_PASSIVE;
	if(msrp_endpoint_set_from(endpoint, address, port, content, flags, MSRP_SENDRECV) < 0) {
		printf("Error creating new %s 'From' peer...\n", active ? "active" : "passive");
		return -1;
	}
	printf("\tFrom:\t\t%s (%s)\n", msrp_endpoint_get_from_fullpath(endpoint), active ? "active" : "passive");

	/*
	   Now, in real world, we would wait for SIP/SDP negotiation to know our peer...
	   Since this is an example, we'll ask these values on the console
	   to simulate a negotiated peer to be the 'To':

		msrp://to_uri:to_port/to_session;tcp
	*/
	flags = MSRP_OVER_TCP;
	if(active)	/* If we are active, our peer is passive and viceversa */
		flags |= MSRP_PASSIVE;
	else
		flags |= MSRP_ACTIVE;


	printf("Insert your peer's nickname (for visualization only): ");
	peerdisplay = calloc(LEN_128, sizeof(char));
	memset(peerdisplay, 0, LEN_128*sizeof(char));
	scanf(" %a[^\n]", &peerdisplay);
	printf("Insert the full 'To' path: ");
	path = calloc(LEN_128, sizeof(char));
	memset(path, 0, LEN_128*sizeof(char));
	scanf(" %a[^\n]", &path);

	if(msrp_endpoint_set_to(endpoint, path, content, flags, MSRP_SENDRECV) < 0) {
		printf("Error creating new %s 'To' peer...\n", active ? "passive" : "active");
		return -1;
	}
	printf("\tTo:\t\t%s (%s)\n", msrp_endpoint_get_to_fullpath(endpoint), active ? "passive" : "active");


	/* This simulates a chat window */
	be_alive = 1;

	while(be_alive) {
		printf("<%s> ", display);
		scanf(" %a[^\n]", &lineptr);
		command = lineptr;
		while(*command && *command < 33)
			command++;
		if(!strcasecmp(command, "\\quit"))
			be_alive = 0;
		else {
			/* The '0' after the buffer stands for 'No reports' */			
			if(msrp_send_text(endpoint, lineptr, 0) < 0)				
				printf("Error sending text...\n");
		}
	}

	if(msrp_endpoint_destroy(endpoint) < 0)
		printf("Couldn't destroy endpoint...\n");
	else
		printf("Endpoint destroyed\n");

	msrp_quit();

	system("pause");
	exit(0);
}


/* The callback to receive incoming messages from the MSRP library */
void callback_msrp(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes)
{
	switch(event) {
		case MSRP_INCOMING_SEND:
			if(content == MSRP_TEXT_PLAIN) {
				if(debug) {
					printf("[MSRP-SEND] Incoming SEND text (%d bytes)\n", bytes);
					printf("[MSRP-SEND] \tFrom: %s\n", msrp_endpoint_get_to_fullpath(endpoint));
					printf("[MSRP-SEND] \tTo:   %s\n", msrp_endpoint_get_from_fullpath(endpoint));
					printf("[MSRP-SEND] \t\t%s\n", data ? (char *)data : "??\n");
				} else {
					printf("<%s> %s\n", peerdisplay, data ? (char *)data : "??");
				}
			}
			break;
		case MSRP_INCOMING_REPORT:
			if(content == MSRP_TEXT_PLAIN) {
				if(debug) {
					printf("[MSRP-REPORT] Incoming REPORT text (%d bytes)\n", bytes);
					printf("[MSRP-REPORT] \tFrom: %s\n", msrp_endpoint_get_to_fullpath(endpoint));
					printf("[MSRP-REPORT] \tTo:   %s\n", msrp_endpoint_get_from_fullpath(endpoint));
					printf("[MSRP-REPORT] \t%s\n", data ? (char *)data : "??\n");
				} else
					printf(" *** Announcement: %s\n", data ? (char *)data : "??\n");
			}
			break;
		case MSRP_LOCAL_CONNECT:
			printf("Endpoint %lu: we connected towards 'To' (%s)\n",
				endpoint->ID, msrp_endpoint_get_to_fullpath(endpoint));
			break;
		case MSRP_LOCAL_DISCONNECT:
			printf("Endpoint %lu: we disconnected (%s)\n",
				endpoint->ID, msrp_endpoint_get_from_fullpath(endpoint));
			be_alive = 0;
			break;
		case MSRP_REMOTE_CONNECT:
			printf("Endpoint %lu: 'To' connected to us (%s)\n",
				endpoint->ID, msrp_endpoint_get_to_fullpath(endpoint));
			break;
		case MSRP_REMOTE_DISCONNECT:
			printf("Endpoint %lu: 'To' disconnected (%s)\n",
				endpoint->ID, msrp_endpoint_get_to_fullpath(endpoint));
			be_alive = 0;
			break;
		default:	/* Here you can handle what to do with code responses (e.g. event=403 --> Forbidden) */
			if(event == 403)
				printf("\n *** '403 Forbidden' in reply to our message!\n");
			break;
	}
	fflush(stdout);
}

/* The callback to receive incoming events from the library */
void events_msrp(int event, void *info)
{
	char *head;

	if(!debug || (event == MSRP_NONE))
		return;

	/* Debug text notifications */
	head = NULL;
	if(event == MSRP_LOG)
		head = "MSRP_LOG";
	else if(event == MSRP_ERROR)
		head = "MSRP_ERROR";
	else
		return;

	printf("[%s] %s\n", head ? head : "??", info ? (char *)info : "??");
}

/* A helper to read and parse the application configuration file */
int load_config_file(char *filename)
{
	FILE *f = NULL;
	char buffer[500];
	char *var  = NULL, *value = NULL;

	if(!filename)
		return -1;
	
	f = fopen(filename, "rt");
	if(!f) {
		printf("Couldn't open file %s...\n", filename);
		return -1;
	}
	//printf("\nConfiguration file: %s\n", filename);

	debug = -1; call_id = NULL; label = 0;
	address = NULL; port = 0;

	
	while(fgets(buffer, 100, f) != NULL) {
		value = buffer;
		var = strsep(&value, "=");
		if(var) {
			if(!strcasecmp(var, "debug")) {
				debug = atoi(value);
				if(debug != 0)
					debug = 1;
				printf("\tDebug:\t\t%s\n", debug ? "yes" : "no");
			} else if(!strcasecmp(var, "active")) {
				active = atoi(value);
				if(active != 0)
					active = 1;
				printf("\tActive:\t\t%s\n", active ? "yes" : "no");
			} else if(!strcasecmp(var, "call_id")) {
				call_id = calloc(strlen(value) + 1, sizeof(char));
				strncpy(call_id, value, strlen(value) - 1);
				printf("\tCall-ID:\t%s\n", call_id);
			} else if(!strcasecmp(var, "label")) {
				label = atoi(value);
				printf("\tLabel:\t\t%d\n", label);
			} else if(!strcasecmp(var, "address")) {
				address = calloc(strlen(value) + 1, sizeof(char));
				strncpy(address, value, strlen(value) - 1);
				printf("\tAddress:\t%s\n", address);
			} else if(!strcasecmp(var, "port")) {
				port = atoi(value);
				printf("\tPort:\t\t%hu\n", port);
			} else if(!strcasecmp(var, "display")) {
				display = calloc(strlen(value) + 1, sizeof(char));
				strncpy(display, value, strlen(value) - 1);
				printf("\tDisplay:\t%s\n", display);
			}
		}
	}
	fclose(f);

	if(debug < 0) {
		debug = 0;
		printf("\tNo 'debug' in %s, defaulting to '%s'\n", filename, debug ? "yes" : "no");
	}
	if(!call_id) {
		call_id = "a1b2c3d4e5";
		printf("\tNo 'call_id' in %s, defaulting to %s\n", filename, call_id);
	}
	if(!label) {
		label = 10;
		printf("\tNo 'label' in %s, defaulting to %d\n", filename, label);
	}
	if(!address) {
		address = "127.0.0.1";
		printf("\tNo 'address' in %s, defaulting to %s\n", filename, address);
	}
	if(!port) {
		printf("\tNo 'port' in %s, choosing random one\n", filename);
	}
	if(!display) {
		display = "Me";
		printf("\tNo 'display' in %s, defaulting to %s\n", filename, display);
	}
	return 0;
}