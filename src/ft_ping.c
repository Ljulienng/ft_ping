#include "ft_ping.h"
#include "libft.h"

t_ping_data pdata;

void handle_signal(int signal)
{
	if (signal == SIGINT)
	{
		double packet_loss = 100.0 - (((double)pdata.packets_received / (double)pdata.packets_sent) * 100.0);
		printf("\n--- %s ping statistics ---\n", pdata.host);
		printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n", pdata.packets_sent, pdata.packets_received, packet_loss);

		if (pdata.packets_received > 0)
		{
			double rtt_avg = pdata.rtt_sum / pdata.packets_received;
			double rtt_stddev = custom_sqrt((pdata.rtt_sum_squares / pdata.packets_received) - (rtt_avg * rtt_avg));
			printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
				   pdata.rtt_min / 1000.0, rtt_avg / 1000.0, pdata.rtt_max / 1000.0, rtt_stddev / 1000.0);
		}
		close(pdata.sockfd);
		exit(0);
	}
}
void send_icmp_packet(struct addrinfo *res, int seq, t_ping_data *pdata)
{
	struct icmp_packet pckt;
	ft_memset(&pckt, 0, sizeof(pckt));
	pckt.hdr.type = ICMP_ECHO;
	pckt.hdr.un.echo.id = getpid();
	pckt.hdr.un.echo.sequence = seq;
	for (size_t i = 0; i < sizeof(pckt.msg) - 1; i++)
		pckt.msg[i] = i + '0';
	pckt.msg[sizeof(pckt.msg) - 1] = 0;
	pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

	int sent = sendto(pdata->sockfd, &pckt, sizeof(pckt), 0, res->ai_addr, res->ai_addrlen);
	if (sent < 0)
	{
		printf("Error: %s\n", strerror(errno));
		exit(1);
	}
	pdata->packets_sent++;
	gettimeofday(&pdata->send_times[seq], NULL);
	if (pdata->verbose)
	{
		printf("Sent ICMP packet: ID=%d, SEQ=%d, Checksum=%#x\n", pckt.hdr.un.echo.id, pckt.hdr.un.echo.sequence, pckt.hdr.checksum);
	}
}

void receive_icmp_packet(int seq, t_ping_data *pdata)
{
	struct sockaddr_in r_addr;
	socklen_t r_addr_len = sizeof(r_addr);
	char buf[1024];
	struct timeval timeout;
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(pdata->sockfd, &readfds);

	int select_result = select(pdata->sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (select_result > 0)
	{
		int received = recvfrom(pdata->sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&r_addr, &r_addr_len);
		if (received < 0)
		{
			printf("Error: %s\n", strerror(errno));
			exit(1);
		}

		struct timeval tv;
		// printf("seq: %d\n", seq);

		gettimeofday(&tv, NULL);
		// printf("pdata->send_times[seq].tv_sec: %ld, pdata->send_times[seq].tv_usec: %ld\n", (long)pdata->send_times[seq].tv_sec, (long)pdata->send_times[seq].tv_usec);

		struct icmphdr *icmph = (struct icmphdr *)(buf + 20);
		if (icmph->type == ICMP_ECHOREPLY)
		{
			// printf("tv.tv_sec: %ld, pdata->send_times[seq].tv_sec: %ld\n", (long)tv.tv_sec, (long)pdata->send_times[seq].tv_sec);
			// printf("tv.tv_usec: %ld, pdata->send_times[seq].tv_usec: %ld\n", (long)tv.tv_usec, (long)pdata->send_times[seq].tv_usec);

			time_t sec_diff = tv.tv_sec - pdata->send_times[seq].tv_sec;
			suseconds_t usec_diff = tv.tv_usec - pdata->send_times[seq].tv_usec;
			long rtt = (sec_diff * 1000000) + usec_diff;
			printf("64 bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
				   pdata->ip_str, icmph->un.echo.sequence, buf[8],
				   (float)rtt / 1000);
			pdata->packets_received++;
			// Update RTT statistics
			if (rtt < pdata->rtt_min)
				pdata->rtt_min = rtt;
			if (rtt > pdata->rtt_max)
				pdata->rtt_max = rtt;
			pdata->rtt_sum += rtt;
			pdata->rtt_sum_squares += rtt * rtt;
			if (pdata->verbose)
			{
				printf("Received ICMP packet: Type=%d, ID=%d, SEQ=%d, TTL=%d\n", icmph->type, icmph->un.echo.id, icmph->un.echo.sequence, buf[8]);
			}
		}
		else
		{
			if (pdata->verbose)
			{
				printf("Error: Received ICMP type %d\n", icmph->type);
			}
		}
	}
	else if (select_result == 0 && pdata->verbose)
	{
		printf("Request timeout for icmp_seq %d\n", seq);
	}
}

void usage()
{
	printf("Usage: sudo ./ping [options] destination\n");
	printf("Options:\n");
	printf("  -v      Enable verbose output\n");
	printf("  -h      Display this help message\n");
}

int ft_ping(const char *target, struct addrinfo *res)
{

	struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
	inet_ntop(res->ai_family, &(ipv4->sin_addr), pdata.ip_str, sizeof(pdata.ip_str));

	printf("PING %s (%s) %d(%d) bytes of data.\n", target, pdata.ip_str, PACKET_SIZE, PACKET_SIZE + 28);

	// Initialize RTT statistics
	pdata.rtt_min = LONG_MAX;
	pdata.rtt_max = 0;
	pdata.rtt_sum = 0;
	pdata.rtt_sum_squares = 0;

	while (1)
	{
		send_icmp_packet(res, pdata.packets_sent, &pdata);
		receive_icmp_packet(pdata.packets_sent - 1, &pdata);
		sleep(1);
	}
}

int main(int argc, char **argv)
{
	int opt;
	const char *target = NULL;

	for (optind = 1; optind < argc; optind++)
	{
		if (argv[optind][0] == '-')
		{
			opt = custom_getopt(argc, argv, "vh");
			switch (opt)
			{
			case 'v':
				pdata.verbose = 1;
				break;
			case 'h':
				usage();
				exit(0);
			default:
				usage();
				exit(1);
			}
		}
		else
		{
			target = argv[optind];
			break;
		}
	}

	if (!target)
	{
		usage();
		exit(1);
	}

	signal(SIGINT, handle_signal);

	struct addrinfo hints, *res;
	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	int r = getaddrinfo(target, NULL, &hints, &res);

	if (r != 0)
	{
		printf("Ping: unknown host\n");
		// printf("Error: %s\n", gai_strerror(r));
		exit(1);
	}

	pdata.host = target;
	pdata.sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (pdata.sockfd < 0)
	{
		printf("Error: %s\n", strerror(errno));
		exit(1);
	}

	ft_ping(target, res);

	close(pdata.sockfd);
	return 0;
}
