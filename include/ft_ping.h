#ifndef FT_PING
#define FT_PING

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <limits.h> // Include limits.h for LONG_MAX

#define PACKET_SIZE 56
#define TIMEOUT 1

struct icmp_packet
{
	struct icmphdr hdr;
	char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

typedef struct s_ping_data
{
	int sockfd;
	int packets_sent;
	int packets_received;
	const char *host;
	char ip_str[INET_ADDRSTRLEN];
	int verbose;
	struct timeval send_times[0xFFFF];
	long rtt_min;
	long rtt_max;
	double rtt_sum;
	double rtt_sum_squares;
} t_ping_data;

int custom_getopt(int argc, char *const argv[], const char *optstring);
double custom_sqrt(double num);
unsigned short checksum(void *b, int len);
void handle_signal(int signal);
void print_statistics(void);
void send_icmp_packet(struct addrinfo *res, int seq, t_ping_data *pdata);
void receive_icmp_packet(int seq, t_ping_data *pdata);
void usage(void);
int ft_ping(const char *target, struct addrinfo *res);

extern t_ping_data pdata;
#endif
