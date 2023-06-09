
#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/time.h>
//#include <net/ethernet.h>
//#include <netinet/in_systm.h>
//#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <netinet/udp.h>
//#include <netinet/ether.h>
//#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
//#include <pcap.h>
//#include <err.h>
//#include <regex.h>
#include "dhcp_options.h"

#ifndef HAVE_STRSEP
//#include "strsep.c"
#endif

#define SPERW	(7 * 24 * 3600)
#define SPERD	(24 * 3600)
#define SPERH	(3600)
#define SPERM	(60)

#define LARGESTRING 1024

// header variables
/*
char	timestamp[40];			// timestamp on header
char	mac_origin[40];			// mac address of origin
char	mac_destination[40];		// mac address of destination
char	ip_origin[40];			// ip address of origin
char	ip_destination[40];		// ip address of destination
int		max_data_len;			// maximum size of a packet
*/
//int	tcpdump_style = -1;
//char	errbuf[PCAP_ERRBUF_SIZE];
//char	*hmask = NULL;
//regex_t	preg;

typedef unsigned char u_char;

int	check_ch(u_char *data, int data_len);
int	readheader(u_char *buf);
int	readdata(u_char *buf, u_char *data, int *data_len);
int	printdata(u_char *data, int data_len);
void	pcap_callback(u_char *user, const struct pcap_pkthdr *h,
	    const u_char *sp);

void	printIPaddress(u_char *data);
void	printIPaddressAddress(u_char *data);
void	printIPaddressMask(u_char *data);
void	print8bits(u_char *data);
void	print16bits(u_char *data);
void	print32bits(u_char *data);
void	printTime8(u_char *data);
void	printTime32(u_char *data);
void	printReqParmList(u_char *data, int len);
void	printHexColon(u_char *data, int len);
void	printHex(u_char *data, int len);
void	printHexString(u_char *data, int len);

// print the data as an IP address
void printIPaddress(u_char *data) {
	printf("%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
}

// print the data as an IP address and an IP address
void printIPaddressAddress(u_char *data) {
	printf("%d.%d.%d.%d %d.%d.%d.%d",
	    data[0], data[1], data[2], data[3],
	    data[4], data[5], data[6], data[7]);
}

// print the data as an IP address and mask
void printIPaddressMask(u_char *data) {
	printf("%d.%d.%d.%d/%d.%d.%d.%d",
	    data[0], data[1], data[2], data[3],
	    data[4], data[5], data[6], data[7]);
}

// prints a value of 8 bits (1 byte)
void print8bits(u_char *data) {
	printf("%d", data[0]);
}

// prints a value of 16 bits (2 bytes)
void print16bits(u_char *data) {
	printf("%d", (data[0] << 8) + data[1]);
}

// prints a value of 32 bits (4 bytes)
void print32bits(u_char *data) {
	printf("%d",
	    (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3]);
}

// print the data as a 8bits time-value
void printTime8(u_char *data) {
	int t = data[0];
	printf("%d (", t);
	if (t > SPERW) { printf("%dw", t / (SPERW)); t %= SPERW; }
	if (t > SPERD) { printf("%dd", t / (SPERD)); t %= SPERD; }
	if (t > SPERH) { printf("%dh", t / (SPERH)); t %= SPERH; }
	if (t > SPERM) { printf("%dm", t / (SPERM)); t %= SPERM; }
	if (t > 0) printf("%ds", t);
	printf(")");
}

// print the data as a 32bits time-value
void printTime32(u_char *data) {
	int t = (data[0] << 24) + (data[1] << 16) + (data[2] <<8 ) + data[3];
	printf("%d (", t);
	if (t > SPERW) { printf("%dw", t / (SPERW)); t %= SPERW; }
	if (t > SPERD) { printf("%dd", t / (SPERD)); t %= SPERD; }
	if (t > SPERH) { printf("%dh", t / (SPERH)); t %= SPERH; }
	if (t > SPERM) { printf("%dm", t / (SPERM)); t %= SPERM; }
	if (t > 0) printf("%ds", t);
	printf(")");
}

// print the data as a hex-list, with the translation into ascii behind it
void printHexString(u_char *data, int len) {
	int i, j, k;

	for (i = 0; i <= len / 8; i++) {
		for (j = 0; j < 8; j++) {
			if (i * 8 + j >= len) break;
			printf("%02x", data[i * 8 + j]);
		}
		for (k = j; k < 8; k++)
			printf("  ");
		printf(" ");
		for (j = 0; j < 8; j++) {
			char c = data[i * 8 + j];
			if (i * 8 + j >= len) break;
			printf("%c", isprint(c) ? c : '.');
		}
		if (i * 8 + j < len) printf("|");
	}
}

// print the data as a hex-list, without the translation into ascii behind it
void printHex(u_char *data, int len) {
	int i, j;

	for (i = 0; i <= len / 8; i++) {
		for (j = 0; j < 8; j++) {
			if (i * 8 + j >= len) break;
			printf("%02x", data[i * 8 + j]);
		}
		if (i * 8 + j < len) printf("|");
	}
}

// print the data as a hex-list seperated by colons
void printHexColon(u_char *data, int len) {
	int i;

	for (i = 0; i < len; i++) {
		if (i != 0 ) printf(":");
		printf("%02x", data[i]);
	}
}

// print the list of requested parameters
void printReqParmList(u_char *data, int len) {
	int i;

	for (i = 0; i < len; i++) {
		printf("%d (%s)", data[i], dhcp_options[data[i]]);
		if (i < len-1) printf(",");
		//printf("|");
	}
}

// print the header and the options.
int printdata(u_char *data, int data_len) {
	int	j, i;
	char	buf[LARGESTRING];

	if (data_len == 0)
		return 0;

	/*
	printf("TIME: %s", timestamp);
	printf("|IP: %s (%s) > %s (%s)",
	    ip_origin, mac_origin, ip_destination, mac_destination);
		*/
	printf("|OP: %d (%s)", data[0], operands[data[0]]);
	printf("|HTYPE: %d (%s)", data[1], htypes[data[1]]);
	printf("|HLEN: %d", data[2]);
	printf("|HOPS: %d", data[3]);
	printf("|XID: %02x%02x%02x%02x",
	    data[4], data[5], data[6], data[7]);
	printf("|SECS: "); print16bits(data + 8);
	printf("|FLAGS: %x", 255 * data[10] + data[11]);

	printf("|CIADDR: "); printIPaddress(data + 12);
	printf("|YIADDR: "); printIPaddress(data + 16);
	printf("|SIADDR: "); printIPaddress(data + 20);
	printf("|GIADDR: "); printIPaddress(data + 24);
	printf("|CHADDR: "); printHexColon(data+28, 16);
	printf("|SNAME: %s.", data + 44);
	printf("|FNAME: %s.", data + 108);

	j = 236;
	j += 4;	/* cookie */
	while (j < data_len && data[j] != 255) {
		printf("|OPTION: %3d (%3d) %-26s", data[j], data[j + 1],
		    dhcp_options[data[j]]);

	switch (data[j]) {
	default:
		printHexString(data + j + 2, data[j + 1]);
		break;

	case 0:		// pad
		break;

	case  1:	// Subnetmask
	case  3:	// Routers
	case 16:	// Swap server
	case 28:	// Broadcast address
	case 32:	// Router solicitation
	case 50:	// Requested IP address
	case 54:	// Server identifier
		printIPaddress(data + j + 2);
		break;

	case 12:	// Hostname
	case 14:	// Merit dump file
	case 15:	// Domain name
	case 17:	// Root Path
	case 18:	// Extensions path
	case 40:	// NIS domain
	case 56:	// Message
	case 62:	// Netware/IP domain name
	case 64:	// NIS+ domain
	case 66:	// TFTP server name
	case 67:	// bootfile name
	case 60:	// Domain name
	case 86:	// NDS Tree name
	case 87:	// NDS context
		strncpy(buf, (char *)&data[j + 2], data[j + 1]);
		buf[data[j + 1]] = 0;
		printf("%s", buf);
		break;

	case  4:	// Time servers
	case  5:	// Name servers
	case  6:	// DNS server
	case  7:	// Log server
	case  8:	// Cookie server
	case  9:	// LPR server
	case 10:	// Impress server
	case 11:	// Resource location server
	case 41:	// NIS servers
	case 42:	// NTP servers
	case 44:	// NetBIOS name server
	case 45:	// NetBIOS datagram distribution server
	case 48:	// X Window System font server
	case 49:	// X Window System display server
	case 65:	// NIS+ servers
	case 68:	// Mobile IP home agent
	case 69:	// SMTP server
	case 70:	// POP3 server
	case 71:	// NNTP server
	case 72:	// WWW server
	case 73:	// Finger server
	case 74:	// IRC server
	case 75:	// StreetTalk server
	case 76:	// StreetTalk directory assistance server
	case 85:	// NDS server
		for (i = 0; i < data[j + 1] / 4; i++) {
			if (i != 0) printf(",");
			printIPaddress(data + j + 2 + i * 4);
		}
		break;

	case 21:	// Policy filter
		for (i = 0; i < data[j + 1] / 8; i++) {
			if (i != 0) printf(",");
			printIPaddressMask(data + j + 2 + i * 8);
		}
		break;

	case 33:	// Static route
		for (i = 0; i < data[j + 1] / 8; i++) {
			if (i != 0) printf(",");
			printIPaddressAddress(data + j + 2 + i * 8);
		}
		break;

	case 25:	// Path MTU plateau table
		for (i = 0; i < data[j + 1] / 2; i++) {
			if (i != 0) printf(",");
			print16bits(data + j + 2 + i * 2);
		}
		break;

	case 13:	// bootfile size
	case 22:	// Maximum datagram reassembly size
	case 26:	// Interface MTU
	case 57:	// Maximum DHCP message size
		print16bits(data + j + 2);
		break;

	case 19:	// IP forwarding enabled/disable
	case 20:	// Non-local source routing
	case 27:	// All subnets local
	case 29:	// Perform mask discovery
	case 30:	// Mask supplier
	case 31:	// Perform router discovery
	case 34:	// Trailer encapsulation
	case 39:	// TCP keepalive garbage
		printf("%d (%s)", data[j + 2], enabledisable[data[j + 2]]);
		break;

	case 23:	// Default IP TTL
		printTime8(data + j + 2);
		break;

	case 37:	// TCP default TTL
		print8bits(data + j + 2);
		break;

	case 43:	// Vendor specific info
	case 47:	// NetBIOS scope (no idea how it looks like)
		printHexString(data + j + 2, data[j + 1]);
		break;

	case 46:	// NetBIOS over TCP/IP node type
		printf("%d (%s)",
		    data[j + 2], netbios_node_type[data[j + 2]]);
		break;
	    
	case  2:	// Time offset
	case 24:	// Path MTU aging timeout
	case 35:	// ARP cache timeout
	case 38:	// TCP keepalive interval
	case 51:	// IP address leasetime
	case 58:	// T1
	case 59:	// T2
		printTime32(data + j + 2);
		break;

	case 36:	// Ethernet encapsulation
		printf("%d (%s)",
		    data[j + 2],
		    data[j +2 ] > sizeof(ethernet_encapsulation) ?
			"*wrong value*" :
			ethernet_encapsulation[data[j + 2]]);
		break;

	case 52:	// Option overload
		printf("%d (%s)",
		    data[j + 2],
		    data[j + 2] > sizeof(option_overload) ?
			"*wrong value*" :
			option_overload[data[j + 2]]);
		break;

	case 53:	// DHCP message type
		printf("%d (%s)",
		    data[j + 2],
		    data[j + 2] > sizeof(dhcp_message_types) ?
			"*wrong value*" :
			dhcp_message_types[data[j + 2]]);
		break;

	case 55:	// Parameter Request List
		printReqParmList(data + j + 2, data[j + 1]);
		break;

	case 63:	// Netware/IP domain information
		printHex(data + j + 2, data[j + 1]);
		break;

	case 61:	// Client identifier
		printHexColon(data + j + 2, data[j + 1]);
		break;

	case 81:	// Client FQDN
		print8bits(data + j + 2);
		printf("-");
		print8bits(data + j + 3);
		printf("-");
		print8bits(data + j + 4);
		printf(" ");
		strncpy(buf, (char *)&data[j + 5], data[j + 1] - 3);
		buf[data[j + 1] - 3]=0;
		printf("%s", buf);
		break;

	case 82:	// Relay Agent Information
		printf("|");
		for (i = j + 2; i < j + data[j + 1]; ) {
			if (i != j+2) {
				printf("|");
			}
			printf("%-17s %-13s ", " ",
			    data[i] > sizeof(relayagent_suboptions) ?
			    "*wrong value*" :
			    relayagent_suboptions[data[i]]);
			if (i + data[i + 1] > j + data[j + 1]) {
				printf("*MALFORMED -- TOO LARGE*\n");
				break;
			}
			printHexColon(data + i + 2, data[i + 1]);
			i += data[i + 1] + 2;
		}
		break;

	}
	//printf("\n");

	/*
	// This might go wrong if a mallformed packet is received.
	// Maybe from a bogus server which is instructed to reply
	// with invalid data and thus causing an exploit.
	// My head hurts... but I think it's solved by the checking
	// for j<data_len at the begin of the while-loop.
	*/
	if (data[j]==0)		// padding
		j++;
	else
		j+=data[j + 1] + 2;

	}

	//puts("\n");
	printf("\n");
	fflush(stdout);

	return 0;
}