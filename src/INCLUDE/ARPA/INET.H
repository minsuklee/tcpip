/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/
 
// Definitions of functions in the internet library

unsigned long inet_addr(char *addr_str); 
unsigned long inet_lnaof(struct in_addr in); 
unsigned long inet_makeaddr(unsigned long net, unsigned long lna); 
unsigned long inet_netof(struct in_addr in); 
unsigned long inet_network(char *addr_str); 
char *inet_ntoa(struct in_addr in); 
