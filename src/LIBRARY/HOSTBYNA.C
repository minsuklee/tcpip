/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"
 
struct hostent __ahost;
unsigned long __h_inaddr;
char __h_name[CONF_HOSTLEN];

#define LINESIZE 120

struct hostent *
gethostbyname(char *hostname)
{
	static char *as[MAXALIASES + 3];
	static char line[LINESIZE], *lp;
	struct hostent *rhost = NULL;
	int i, entry;
	char far *str;
	FILE *fp;

	if (!hostname || (hostname[0] == 0))
		goto leave;

#define DOMAIN_NAME_SERVER
#ifdef DOMAIN_NAME_SERVER
	//if (_imige_vec)
	{
		//printf("BEGIN RESOLVE\n");
		rhost = _resolve_name(hostname, 0, 0);
		//printf("END RESOLVE %p\n", rhost);
		if (rhost)
			return(rhost);
	}
#endif
	// And then Search "hosts" file
	if ((str = getenv(IMIGETCP_ENV)) == NULL)
		strcpy(line, "hosts");
	else {
		strcpy(line, str);
		strcat(line, "\\hosts");
	}
	if ((fp = fopen(line, "r")) == NULL)
		goto leave;

	while (!feof(fp)) {
		lp = line;
		if (fgets(lp, LINESIZE - 1, fp) == NULL)
			break;
		if (line[strlen(lp) - 1] < 32) // Non printable character
			line[strlen(lp) - 1] = 0; // printf("'%s' ", lp);
		entry = 0;
		for (i = 0; i < (MAXALIASES + 3); i++)
			as[i] = NULL;
		while ((*lp) && (entry < (MAXALIASES + 2))) {
			while ((*lp == ' ') || (*lp == 9))
				lp++;
			if ((*lp == '#') || (*lp == 0))
				break;
			as[entry++] = lp;
			while (*lp && (*lp != ' ') && (*lp != 9) && (*lp != '#'))
				lp++;
			*lp++ = 0;
			//printf("[%d]='%s' ", entry - 1, as[entry - 1]);
		}
		if (entry < 2)	// Line Has Error
			continue;
		for (i = 1; i < entry; i++) {
			if (!(stricmp(as[i], hostname))) {
				strcpy(__h_name, as[i]);
				__h_inaddr = inet_addr(as[0]);
				__ahost.h_name = __h_name;
				__ahost.h_addr = (char *)&__h_inaddr;
				__ahost.h_addrtype = AF_INET;
				__ahost.h_length = sizeof(unsigned long);
				if (i != 1) {
					char *tmp = as[i];
					as[i] = as[1];
					as[1] = tmp;
				}
				__ahost.h_aliases = as + 2;
				rhost = &__ahost;
				goto done;
			}
		}
	}
done:	fclose(fp);
leave:	return(rhost);
}
