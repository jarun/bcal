#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE !TRUE

#define SECTOR_SIZE 512 /* 0x200 */
#define MAX_HEAD 16 /* 0x10 */
#define MAX_SECTOR 63 /* 0x3f */

typedef unsigned char bool;
typedef unsigned long ulong;
typedef unsigned long long ull;

char *VERSION = "0.1";
char *units[] = { "b", "kib", "mib", "gib", "tib",
                       "kb", "mb", "gb", "tb", };
typedef struct {
	ulong c;
	ulong h;
	ulong s;
} t_chs;

void printval(double val, char *unit)
{
	if (trunc(val) == val)
		fprintf(stdout, "%40llu %s\n", (ull)val, unit);
	else
		fprintf(stdout, "%40.10f %s\n", val, unit);
}

ull convertbyte(char *buf)
{
	/* Convert and print in bytes */
	ull bytes = strtoull(buf, NULL, 0);
	fprintf(stdout, "%40llu B\n\n", bytes);

	/* Convert and print in IEC standard units */

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = bytes / (double)1024;
	printval(val, "KiB");

	val = bytes / (double)(1 << 20);
	printval(val, "MiB");

	val = bytes / (double)(1 << 30);
	printval(val, "GiB");

	val = bytes / (double)((ull)1 << 40);
	printval(val, "TiB");

	/* Convert and print in SI standard values */

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
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

ull convertkib(char *buf)
{
	double kib = strtod(buf, NULL);

	ull bytes = (ull)(kib * 1024);
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	printval(kib, "KiB");

	double val = kib / 1024;
	printval(val, "MiB");

	val = kib / (1 << 20);
	printval(val, "GiB");

	val = kib / (1 << 30);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = kib * 1024 / 1000;
	printval(val, "kB");

	val = kib * 1024 / 1000000;
	printval(val, "MB");

	val = kib * 1024 / 1000000000;
	printval(val, "GB");

	val = kib * 1024 / 1000000000000;
	printval(val, "TB");

	return bytes;
}

ull convertmib(char *buf)
{
	double mib = strtod(buf, NULL);

	ull bytes = (ull)(mib * (1 << 20));
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = mib * 1024;
	printval(val, "KiB");

	printval(mib, "MiB");

	val = mib / 1024;
	printval(val, "GiB");

	val = mib / (1 << 20);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = mib * (1 << 20)/ 1000;
	printval(val, "kB");

	val = mib * (1 << 20) / 1000000;
	printval(val, "MB");

	val = mib * (1 << 20) / 1000000000;
	printval(val, "GB");

	val = mib * (1 << 20) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

ull convertgib(char *buf)
{
	double gib = strtod(buf, NULL);

	ull bytes = (ull)(gib * (1 << 30));
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = gib * (1 << 20);
	printval(val, "KiB");

	val = gib * 1024;
	printval(val, "MiB");

	printval(gib, "GiB");

	val = gib / 1024;
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = gib * (1 << 30)/ 1000;
	printval(val, "kB");

	val = gib * (1 << 30) / 1000000;
	printval(val, "MB");

	val = gib * (1 << 30) / 1000000000;
	printval(val, "GB");

	val = gib * (1 << 30) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

ull converttib(char *buf)
{
	double tib = strtod(buf, NULL);

	ull bytes = (ull)(tib * ((ull)1 << 40));
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = tib * (1 << 30);
	printval(val, "KiB");

	val = tib * (1 << 20);
	printval(val, "MiB");

	val = tib * 1024;
	printval(val, "GiB");

	printval(tib, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = tib * ((ull)1 << 40)/ 1000;
	printval(val, "kB");

	val = tib * ((ull)1 << 40) / 1000000;
	printval(val, "MB");

	val = tib * ((ull)1 << 40) / 1000000000;
	printval(val, "GB");

	val = tib * ((ull)1 << 40) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

ull convertkb(char *buf)
{
	double kb = strtod(buf, NULL);

	ull bytes = (ull)(kb * 1000);
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = kb * 1000 / 1024;
	printval(val, "KiB");

	val = kb * 1000 / (1 << 20);
	printval(val, "MiB");

	val = kb * 1000 / (1 << 30);
	printval(val, "GiB");

	val = kb * 1000 / ((ull)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	printval(kb, "kB");

	val = kb / 1000;
	printval(val, "MB");

	val = kb / 1000000;
	printval(val, "GB");

	val = kb / 1000000000;
	printval(val, "TB");

	return bytes;
}

ull convertmb(char *buf)
{
	double mb = strtod(buf, NULL);

	ull bytes = (ull)(mb * 1000000);
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = mb * 1000000 / 1024;
	printval(val, "KiB");

	val = mb * 1000000 / (1 << 20);
	printval(val, "MiB");

	val = mb * 1000000 / (1 << 30);
	printval(val, "GiB");

	val = mb * 1000000 / ((ull)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = mb * 1000;
	printval(val, "kB");

	printval(mb, "MB");

	val = mb / 1000;
	printval(val, "GB");

	val = mb / 1000000;
	printval(val, "TB");

	return bytes;
}

ull convertgb(char *buf)
{
	double gb = strtod(buf, NULL);

	ull bytes = (ull)(gb * 1000000000);
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = gb * 1000000000 / 1024;
	printval(val, "KiB");

	val = gb * 1000000000 / (1 << 20);
	printval(val, "MiB");

	val = gb * 1000000000 / (1 << 30);
	printval(val, "GiB");

	val = gb * 1000000000 / ((ull)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = gb * 1000000;
	printval(val, "kB");

	val = gb * 1000;
	printval(val, "MB");

	printval(gb, "GB");

	val = gb / 1000;
	printval(val, "TB");

	return bytes;
}

ull converttb(char *buf)
{
	double tb = strtod(buf, NULL);

	ull bytes = (ull)(tb * 1000000000000);
	fprintf(stdout, "%40llu B\n\n", bytes);

	fprintf(stdout, "            IEC standard (base 2)\n\n");
	double val = tb * 1000000000000 / 1024;
	printval(val, "KiB");

	val = tb * 1000000000000 / (1 << 20);
	printval(val, "MiB");

	val = tb * 1000000000000 / (1 << 30);
	printval(val, "GiB");

	val = tb * 1000000000000 / ((ull)1 << 40);
	printval(val, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = tb * 1000000000;
	printval(val, "kB");

	val = tb * 1000000;
	printval(val, "MB");

	val = tb * 1000;
	printval(val, "GB");

	printval(tb, "TB");

	return bytes;
}

void printbin(ull val)
{
	int count = 63;
	char binstr[65] = {0};

	if (!val) {
		fprintf(stdout, "0b0");
		return;
	}

	while (val && count >= 0) {
		binstr[count--] = "01"[val & 1];
		val >>= 1;
	}

	count++;

	fprintf(stdout, "0b%s", binstr + count);
}

char *strtolower(char *buf)
{
	char *p = buf;

	for (; *p; ++p)
		*p = tolower(*p);

	return buf;
}

bool chs2lba(char *chs, ull *lba)
{
	int token_no = 0;
	char *ptr, *token;
	long chsparam[5] = {0, 0, 0, MAX_HEAD, MAX_SECTOR};

	ptr = token = chs;

	while (*ptr && token_no < 5) {
		if (*ptr == '-') {
			*ptr = '\0';
			chsparam[token_no++] = strtol(token, NULL, 0);
			*ptr++ = '-';
			token = ptr;

			if (*ptr == '\0' && token_no < 5)
				chsparam[token_no++] = strtol(token, NULL, 0);

			continue;
		}

		ptr++;

		if (*ptr == '\0' && token_no < 5)
			chsparam[token_no++] = strtol(token, NULL, 0);
	}

	/* Fail if CHS is omitted */
	if (token_no < 3)
		return FALSE;

	if (chsparam[1] > chsparam[3]) {
		fprintf(stderr, "H > MAX_HEAD\n");
		return FALSE;
	}

	if (chsparam[2] > chsparam[4]) {
		fprintf(stderr, "S > MAX_SECTOR\n");
		return FALSE;
	}


	*lba = chsparam[3] * chsparam[4] * chsparam[0]; /* MH * MS * C */
	*lba += chsparam[4] * chsparam[1]; /* MS * H */

	if (!*lba && !chsparam[2])
		return FALSE;

	*lba += chsparam[2] - 1; /* S - 1 */

	fprintf(stdout, "C %ld H %ld S %ld MAX_HEAD %ld MAX_SECTOR %ld\n",
		chsparam[0], chsparam[1], chsparam[2], chsparam[3], chsparam[4]);

	return TRUE;
}

bool lba2chs(char *lba, t_chs *p_chs)
{
	int token_no = 0;
	char *ptr, *token;
	ull chsparam[3] = {0, MAX_HEAD, MAX_SECTOR};

	ptr = token = lba;

	while (*ptr && token_no < 3) {
		if (*ptr == '-') {
			*ptr = '\0';
			chsparam[token_no++] = strtoull(token, NULL, 0);
			*ptr++ = '-';
			token = ptr;

			if (*ptr == '\0' && token_no < 3)
				chsparam[token_no++] = strtoull(token, NULL, 0);

			continue;
		}

		ptr++;

		if (*ptr == '\0' && token_no < 3)
			chsparam[token_no++] = strtoull(token, NULL, 0);
	}

	/* Fail if LBA is omitted */
	if (!token_no)
		return FALSE;

	if (!chsparam[1] || !chsparam[2])
		return FALSE;

	p_chs->c = (ulong)(chsparam[0] / (chsparam[2] * chsparam[1])); /* L / (MS * MH) */

	p_chs->h = (ulong)((chsparam[0] / chsparam[2]) % chsparam[1]); /* (L / MS) % MH */
	if (p_chs->h > MAX_HEAD) {
		fprintf(stderr, "H > MAX_HEAD\n");
		return FALSE;
	}

	p_chs->s = (ulong)((chsparam[0] % chsparam[2]) + 1); /* (L % MS) + 1 */
	if (p_chs->s > MAX_SECTOR) {
		fprintf(stderr, "S > MAX_SECTOR\n");
		return FALSE;
	}

	fprintf(stdout, "LBA %llu MAX_HEAD %llu MAX_SECTOR %llu\n",
		chsparam[0], chsparam[1], chsparam[2]);

	return TRUE;
}

void usage()
{
	fprintf(stdout, "usage: calb [-c N] [-s bytes] [-h]\n\
            [N unit]\n\n\
Perform storage conversions and calculations.\n\n\
positional arguments:\n\
  N unit           capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB\n\
                   see https://wiki.ubuntu.com/UnitsPolicy\n\
                   should be space-separated, case is ignored\n\
                   N can be decimal or '0x' prefixed hex value\n\n\
optional arguments:\n\
  -c N             show N in binary, decimal and hex formats\n\
                   N must be non-negative\n\
                   use prefix '0b' for binary, '0x' for hex\n\
  -f FORMAT        convert CHS to LBA or LBA to CHS\n\
                   formats are hyphen-separated\n\
                   LBA format:\n\
                       starts with 'l':\n\
                       lLBA-MAX_HEAD-MAX_SECTOR\n\
                   CHS format:\n\
                       starts with 'c':\n\
                       cC-H-S-MAX_HEAD-MAX_SECTOR\n\
                   omitted values are considered 0\n\
                   FORMAT 'c-50--0x12-' denotes:\n\
                     C = 0, H = 50, S = 0, MH = 0x12, MS = 0\n\
                   FORMAT 'l50-0x12' denotes:\n\
                     LBA = 50, MH = 0x12, MS = 0\n\
                   decimal or '0x' prefixed hex values accepted\n\
                   default MAX_HEAD: 16, default MAX_SECTOR: 63\n\
  -s               sector size in bytes [decimal/hex, default 512]\n\
  -h               show this help and exit\n\n\
Version %s\n\
Copyright (C) 2016 Arun Prakash Jana <engineerarun@gmail.com>\n\
License: GPLv3\n\
Webpage: https://github.com/jarun/calb\n", VERSION);
}

int main(int argc, char **argv)
{
	int opt = 0;
	long sectorsize = SECTOR_SIZE;

	opterr = 0;

	while ((opt = getopt (argc, argv, "c:f:hs:")) != -1) {
		switch (opt) {
		case 'c':
			if (*optarg == '-') {
				fprintf(stderr, "-ve values not accepted for conversion\n");
				return 1;
			}

			fprintf(stdout, "CONVERSION\n");
			ull val;

			if (*optarg == '0' && tolower(*(optarg + 1)) == 'b')
				val = strtoull(optarg + 2, NULL, 2);
			else
				val = strtoull(optarg, NULL, 0);

			fprintf(stdout, "\tbin: ");
			printbin(val);
			fprintf(stdout, "\n\tdec: %llu\n", val);
			fprintf(stdout, "\thex: 0x%llx\n\n", val);
			break;
		case 'f':
			{
				int ret;

				if (tolower(*optarg) == 'c') {
					ull lba = 0;
					ret = chs2lba(optarg + 1, &lba);
					if (ret)
						fprintf(stdout, "LBA: (dec) %llu, (hex) 0x%llx\n",
							lba, lba);
					else
						fprintf(stderr, "Invalid input\n");
				} else if (tolower(*optarg) == 'l') {
					t_chs chs = {0};
					ret = lba2chs(optarg + 1, &chs);
					if (ret)
						fprintf(stdout, "CHS: (dec) %lu %lu %lu, (hex) 0x%lx 0x%lx 0x%lx\n",
							chs.c, chs.h, chs.s, chs.c, chs.h, chs.s);
					else
						fprintf(stderr, "Invalid input\n");
				} else
					fprintf(stderr, "Invalid input\n");
			}
			break;
		case 's':
			sectorsize = strtol(optarg, NULL, 0);
			if (sectorsize <= 0) {
				fprintf(stderr, "sector size must be +ve\n");
				return 1;
			}
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return 1;
		}
	}

	if (argc - optind == 1 || (argc == 1 && optind == 1)) {
		usage();
		return 1;
	}

	if (argc - optind == 2) {
		int ret = 0;
		int count = sizeof(units)/sizeof(*units);
		ull bytes = 0, lba = 0, offset = 0;

		while (count-- > 0) {
			ret = strcmp(units[count], strtolower(argv[optind + 1]));
			if (!ret)
                                break;
		}

		if (count == -1) {
			fprintf(stderr, "No matching unit\n");
			return 1;
		}

		fprintf(stdout, "UNITS\n");

		switch (count) {
		case 0:
			bytes = convertbyte(argv[optind]);
			break;
		case 1:
			bytes = convertkib(argv[optind]);
			break;
		case 2:
			bytes = convertmib(argv[optind]);
			break;
		case 3:
			bytes = convertgib(argv[optind]);
			break;
		case 4:
			bytes = converttib(argv[optind]);
			break;
		case 5:
			bytes = convertkb(argv[optind]);
			break;
		case 6:
			bytes = convertmb(argv[optind]);
			break;
		case 7:
			bytes = convertgb(argv[optind]);
			break;
		case 8:
			bytes = converttb(argv[optind]);
			break;
		default:
			fprintf(stderr, "Unknown unit\n");
			return 1;
		}

		fprintf(stdout, "\n\nADDRESS\n\tdec: %llu\n\thex: 0x%llx\n\n", bytes, bytes);

		lba = bytes / sectorsize;
		offset = bytes % sectorsize;
		fprintf(stdout, "LBA:OFFSET\n\tsector size: 0x%lx\n", sectorsize);
		fprintf(stdout, "\n\tdec: %llu:%llu\n\thex: 0x%llx:0x%llx\n",
			lba, offset, lba, offset);
	}

	return 0;
}
