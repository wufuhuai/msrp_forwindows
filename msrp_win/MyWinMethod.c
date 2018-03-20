#include "MyWinMethod.h"
#include "msrp.h"
#include "msrp_utils.h"

char *strSplit(char **ori, const char *delim){
	char *token, *temp1, *temp2;
	int len;
	token = temp1 = temp2 = NULL;
	len = 0;

	temp1 = strtok(*ori, delim);
	temp2 = strtok(NULL, delim);

	len = strlen(temp1);
	token = malloc(len+1);
	memset(token, 0, len+1);

	strcpy(token,temp1);	
	strcpy(*ori,temp2);

	return token;
}

static int __stream_socketpair(struct addrinfo* ai, SOCKET sock[2]){
	SOCKET listener,client = INVALID_SOCKET,server = INVALID_SOCKET;
	int opt = 1;

	listener = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
	if (INVALID_SOCKET==listener)
		goto fail;

	setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt));

	if(SOCKET_ERROR==bind(listener,ai->ai_addr,ai->ai_addrlen))
		goto fail;

	if (SOCKET_ERROR==getsockname(listener,ai->ai_addr,(int*)&ai->ai_addrlen))
		goto fail;

	if(SOCKET_ERROR==listen(listener,SOMAXCONN))
		goto fail;

	client = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);

	if (INVALID_SOCKET==client)
		goto fail;

	if (SOCKET_ERROR==connect(client,ai->ai_addr,ai->ai_addrlen))
		goto fail;

	server = accept(listener,0,0);
	if (INVALID_SOCKET==server)
		goto fail;

	closesocket(listener);
	sock[0] = client, sock[1] = server;
	return 0;
fail:
	if(INVALID_SOCKET!=listener)
		closesocket(listener);
	if (INVALID_SOCKET!=client)
		closesocket(client);
	return -1;
}

static int __dgram_socketpair(struct addrinfo* ai,SOCKET sock[2])
{
	SOCKET client = INVALID_SOCKET,server=INVALID_SOCKET;
	struct addrinfo addr,*res = NULL;
	const char* address;
	int opt = 1;

	server = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
	if (INVALID_SOCKET==server)
		goto fail;

	setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt));

	if(SOCKET_ERROR==bind(server,ai->ai_addr,ai->ai_addrlen))
		goto fail;

	if (SOCKET_ERROR==getsockname(server,ai->ai_addr,(int*)&ai->ai_addrlen))
		goto fail;

	client = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
	if (INVALID_SOCKET==client)
		goto fail;

	memset(&addr,0,sizeof(addr));
	addr.ai_family = ai->ai_family;
	addr.ai_socktype = ai->ai_socktype;
	addr.ai_protocol = ai->ai_protocol;

	if (AF_INET6==addr.ai_family)
		address = "0:0:0:0:0:0:0:1";
	else
		address = "127.0.0.1";

	if (getaddrinfo(address,"0",&addr,&res))
		goto fail;

	setsockopt(client,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt));
	if(SOCKET_ERROR==bind(client,res->ai_addr,res->ai_addrlen))
		goto fail;
	
	if (SOCKET_ERROR==getsockname(client,res->ai_addr,(int*)&res->ai_addrlen))
		goto fail;
	
	if (SOCKET_ERROR==connect(server,res->ai_addr,res->ai_addrlen))
			goto fail;
	
	if (SOCKET_ERROR==connect(client,ai->ai_addr,ai->ai_addrlen))
		goto fail;
	
	freeaddrinfo(res);
	sock[0] = client, sock[1] = server;
	return 0;
	
fail:
	if (INVALID_SOCKET!=client)
		closesocket(client);
	if (INVALID_SOCKET!=server)
		closesocket(server);
	if (res)
		freeaddrinfo(res);
	return -1;
}

int win32_socketpair(int family, int type, int protocol,SOCKET sock[2]){
	const char* address;
	struct addrinfo addr,*ai;
	int ret = -1;

	memset(&addr,0,sizeof(addr));
	addr.ai_family = family;
	addr.ai_socktype = type;
	addr.ai_protocol = protocol;
	if (AF_INET6==family)
		address = "0:0:0:0:0:0:0:1";
	else
		address = "127.0.0.1";
	
	if (0==getaddrinfo(address,"0",&addr,&ai)){
		if (SOCK_STREAM==type)
			ret = __stream_socketpair(ai,sock);
		else if(SOCK_DGRAM==type)
			ret = __dgram_socketpair(ai,sock);
		freeaddrinfo(ai);
	}
	return ret;
}