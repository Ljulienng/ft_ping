#include "ft_ping.h"
#include "libft.h"

int custom_getopt(int argc, char *const argv[], const char *optstring)
{
	static int optind = 1;
	static int optpos = 1;
	int optopt;
	int opterr = 0;
	int optlen = 0;
	int optfound = 0;
	char *optarg = NULL;

	if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
	{
		return (-1);
	}
	optopt = argv[optind][optpos];
	optlen = ft_strlen(optstring);
	for (int i = 0; i < optlen; i++)
	{
		if (optopt == optstring[i])
		{
			optfound = 1;
			if (optstring[i + 1] == ':')
			{
				optarg = argv[optind] + optpos + 1;
				if (*optarg == '\0' && optind + 1 < argc)
				{
					optarg = argv[++optind];
				}
				optpos = 1;
			}
		}
	}
	if (optfound == 0)
	{
		opterr = 1;
		optopt = '?';
	}
	if (opterr == 1)
	{
		return (-1);
	}
	return (optopt);
}

unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

double custom_sqrt(double num)
{
	if (num < 0)
	{
		printf("Error: Negative number passed to custom_sqrt.\n");
		return -1;
	}
	if (num == 0 || num == 1)
		return num;

	double epsilon = 1e-10; // Precision of the result
	double guess = num;
	double prev_guess;

	do
	{
		prev_guess = guess;
		guess = 0.5 * (guess + (num / guess));
	} while ((guess > prev_guess ? guess - prev_guess : prev_guess - guess) > epsilon);

	return guess;
}
