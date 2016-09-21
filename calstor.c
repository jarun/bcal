#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE !TRUE

typedef unsigned long long ull;

char *units[] = {
	"b",
	"kib",
	"mib",
	"gib",
	"tib",
	"kb",
	"mb",
	"gb",
	"tb",
};

void printval(double val, char *unit)
{
	if (trunc(val) == val)
		printf("%32llu %s\n", (ull)val, unit);
	else
		printf("%32f %s\n", val, unit);
}

ull convertbyte(char *buf)
{
	/* Convert and print in bytes */
	ull bytes = strtoull(buf, NULL, 0);
	printf("%32llu B\n\n", bytes);

	printf("          IEC standard          \n          ------------\n");
	/* Convert and print in IEC standard units */
	double val = bytes / (double)(1 << 10);
	printval(val, "KiB");

	val = bytes / (double)(1 << 20);
	printval(val, "MiB");

	val = bytes / (double)(1 << 30);
	printval(val, "GiB");

	val = bytes / (double)((ull)1 << 40);
	printval(val, "TiB");

	/* Convert and print in SI standard values */
	printf("\n          SI standard          \n          -----------\n");
	val = bytes / (double)1000;
	printval(val, "kB");

	val = bytes / (double)1000000;
	printval(val, "MB");

	val = bytes / (double)1000000000;
	printval(val, "GB");

	val = bytes / (double)1000000000000;
	printval(val, "TB");

	return bytes;
}

char *strtolower(char *buf)
{
	char *p = buf;

	for (; *p; ++p)
		*p = tolower(*p);

	return buf;
}

int main(int argc, char **argv)
{
	int opt = 0;

	opterr = 0;

	while ((opt = getopt (argc, argv, "cs:")) != -1) {
		switch (opt) {
		case 'c':
			break;
		case 's':
			break;
		default:
			if (isprint (optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option\n");

			return 1;
		}
	}

	printf("Non-option args: %d\n", argc - optind);

	if (argc - optind == 2) {
		int ret = 0;
		int count = sizeof(units)/sizeof(*units);
		ull bytes = 0;

		while (count-- > 0) {
			ret = strcmp(units[count], strtolower(argv[optind + 1]));
			if (!ret)
                                break;
		}

		if (count == -1) {
			fprintf(stderr, "No matching unit\n");
			return 1;
		}

		printf("count: %x unit: %s\n", count, units[count]);

		switch (count) {
		case 0:
			bytes = convertbyte(argv[optind]);
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			break;
		case 8:
			break;
		default:
			fprintf(stderr, "Unknown unit\n");
			return 1;
		}

		printf("\nADDRESS: %llu, 0x%llx\n", bytes, bytes);
	}

	return 0;
}
