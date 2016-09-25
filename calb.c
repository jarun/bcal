#include <ctype.h>
#include <math.h>
#include <quadmath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE !TRUE

#define SECTOR_SIZE 512 /* 0x200 */
#define MAX_HEAD 16 /* 0x10 */
#define MAX_SECTOR 63 /* 0x3f */
#define UINT_BUF_LEN 40 /* log10(1 << 128) + '\0' */
#define FLOAT_BUF_LEN 128
#define FLOAT_WIDTH 40

typedef unsigned char bool;
typedef unsigned long ulong;
typedef unsigned long long ull;

#ifdef __SIZEOF_INT128__
typedef __uint128_t maxuint_t;
typedef __float128 maxfloat_t;
#else
typedef __uint64_t maxuint_t;
typedef double maxfloat_t;
#endif

char *VERSION = "0.1";
char *units[] = { "b", "kib", "mib", "gib", "tib",
                       "kb", "mb", "gb", "tb", };

char int_buf[UINT_BUF_LEN];
char float_buf[FLOAT_BUF_LEN];

typedef struct {
	ulong c;
	ulong h;
	ulong s;
} t_chs;

void printbin(maxuint_t n)
{
	int count = 127;
	char binstr[129] = {0};

	if (!n) {
		fprintf(stdout, "0b0");
		return;
	}

	while (n && count >= 0) {
		binstr[count--] = "01"[n & 1];
		n >>= 1;
	}

	count++;

	fprintf(stdout, "0b%s", binstr + count);
}

char *getstr_u128(maxuint_t n, char *buf) {
	if (n == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return buf;
	}

	memset(buf, 0, UINT_BUF_LEN);
	char *loc = buf + UINT_BUF_LEN - 1; /* start at the end */

	while (n != 0) {
		if (loc == buf)
			return NULL; /* should not happen */

		*--loc = "0123456789"[n % 10]; /* save the last digit */
		n /= 10; /* drop the last digit */
	}

	return loc;
}

char *getstr_f128(maxfloat_t val, char *buf)
{
	int n = quadmath_snprintf(buf, FLOAT_BUF_LEN, "%#*.10Qe", FLOAT_WIDTH, val);
	buf[n] = '\0';
	return buf;
}

void printval(maxfloat_t val, char *unit)
{
	if (val - (maxuint_t)val == 0)
		fprintf(stdout, "%40s %s\n", getstr_u128((maxuint_t)val, int_buf), unit);
	else
		fprintf(stdout, "%s %s\n", getstr_f128(val, float_buf), unit);
}

char *strtolower(char *buf)
{
	char *p = buf;

	for (; *p; ++p)
		*p = tolower(*p);

	return buf;
}

maxuint_t convertbyte(char *buf)
{
	/* Convert and print in bytes */
	maxuint_t bytes = strtoull(buf, NULL, 0);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	/* Convert and print in IEC standard units */

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = bytes / (maxfloat_t)1024;
	printval(val, "KiB");

	val = bytes / (maxfloat_t)(1 << 20);
	printval(val, "MiB");

	val = bytes / (maxfloat_t)(1 << 30);
	printval(val, "GiB");

	val = bytes / (maxfloat_t)((maxuint_t)1 << 40);
	printval(val, "TiB");

	/* Convert and print in SI standard values */

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = bytes / (maxfloat_t)1000;
	printval(val, "kB");

	val = bytes / (maxfloat_t)1000000;
	printval(val, "MB");

	val = bytes / (maxfloat_t)1000000000;
	printval(val, "GB");

	val = bytes / (maxfloat_t)1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertkib(char *buf)
{
	maxfloat_t kib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(kib * 1024);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	printval(kib, "KiB");

	maxfloat_t val = kib / 1024;
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

maxuint_t convertmib(char *buf)
{
	maxfloat_t mib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(mib * (1 << 20));
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = mib * 1024;
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

maxuint_t convertgib(char *buf)
{
	maxfloat_t gib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(gib * (1 << 30));
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = gib * (1 << 20);
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

maxuint_t converttib(char *buf)
{
	maxfloat_t tib = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(tib * ((maxuint_t)1 << 40));
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = tib * (1 << 30);
	printval(val, "KiB");

	val = tib * (1 << 20);
	printval(val, "MiB");

	val = tib * 1024;
	printval(val, "GiB");

	printval(tib, "TiB");

	fprintf(stdout, "\n            SI standard (base 10)\n\n");
	val = tib * ((maxuint_t)1 << 40)/ 1000;
	printval(val, "kB");

	val = tib * ((maxuint_t)1 << 40) / 1000000;
	printval(val, "MB");

	val = tib * ((maxuint_t)1 << 40) / 1000000000;
	printval(val, "GB");

	val = tib * ((maxuint_t)1 << 40) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

maxuint_t convertkb(char *buf)
{
	maxfloat_t kb = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(kb * 1000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = kb * 1000 / 1024;
	printval(val, "KiB");

	val = kb * 1000 / (1 << 20);
	printval(val, "MiB");

	val = kb * 1000 / (1 << 30);
	printval(val, "GiB");

	val = kb * 1000 / ((maxuint_t)1 << 40);
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

maxuint_t convertmb(char *buf)
{
	maxfloat_t mb = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(mb * 1000000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = mb * 1000000 / 1024;
	printval(val, "KiB");

	val = mb * 1000000 / (1 << 20);
	printval(val, "MiB");

	val = mb * 1000000 / (1 << 30);
	printval(val, "GiB");

	val = mb * 1000000 / ((maxuint_t)1 << 40);
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

maxuint_t convertgb(char *buf)
{
	maxfloat_t gb = strtod(buf, NULL);

	maxuint_t bytes = (maxuint_t)(gb * 1000000000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = gb * 1000000000 / 1024;
	printval(val, "KiB");

	val = gb * 1000000000 / (1 << 20);
	printval(val, "MiB");

	val = gb * 1000000000 / (1 << 30);
	printval(val, "GiB");

	val = gb * 1000000000 / ((maxuint_t)1 << 40);
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

maxuint_t converttb(char *buf)
{
	maxfloat_t tb = strtod(buf, NULL);

	maxuint_t bytes = (__uint128_t)(tb * 1000000000000);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, int_buf));

	fprintf(stdout, "\n            IEC standard (base 2)\n\n");
	maxfloat_t val = tb * 1000000000000 / 1024;
	printval(val, "KiB");

	val = tb * 1000000000000 / (1 << 20);
	printval(val, "MiB");

	val = tb * 1000000000000 / (1 << 30);
	printval(val, "GiB");

	val = tb * 1000000000000 / ((maxuint_t)1 << 40);
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

bool chs2lba(char *chs, maxuint_t *lba)
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
	maxuint_t chsparam[3] = {0, MAX_HEAD, MAX_SECTOR};

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

	fprintf(stdout, "LBA %s MAX_HEAD %s MAX_SECTOR %s\n",
		getstr_u128(chsparam[0], int_buf),
		getstr_u128(chsparam[1], int_buf),
		getstr_u128(chsparam[2], int_buf));

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
			maxuint_t val;

			if (*optarg == '0' && tolower(*(optarg + 1)) == 'b')
				val = strtoull(optarg + 2, NULL, 2);
			else
				val = strtoull(optarg, NULL, 0);

			fprintf(stdout, "\tbin: ");
			printbin(val);
			fprintf(stdout, "\n\tdec: %s\n", getstr_u128(val, int_buf));
			fprintf(stdout, "\thex: 0x%llx%llx\n\n",
					(ull)(val >> (sizeof(maxuint_t) << 2)), (ull)val);
			break;
		case 'f':
			{
				int ret;

				if (tolower(*optarg) == 'c') {
					maxuint_t lba = 0;
					ret = chs2lba(optarg + 1, &lba);
					if (ret)
						fprintf(stdout, "LBA: (dec) %s, (hex) 0x%llx%llx\n",
							getstr_u128(lba, int_buf),
							(ull)(lba >> (sizeof(maxuint_t) << 2)),
							(ull)(lba));
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
		maxuint_t bytes = 0, lba = 0, offset = 0;

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

		fprintf(stdout, "\n\nADDRESS\n\tdec: %s\n\thex: 0x%llx%llx\n\n",
							getstr_u128(bytes, int_buf),
							(ull)(bytes >> (sizeof(maxuint_t) << 2)),
							(ull)(bytes));

		/* Calculate LBA and offset */
		lba = bytes / sectorsize;
		offset = bytes % sectorsize;

		fprintf(stdout, "LBA:OFFSET\n\tsector size: 0x%lx\n", sectorsize);
		/* We use a global buffer, so print decimal lba first, then offset */
		fprintf(stdout, "\n\tdec: %s:", getstr_u128(lba, int_buf));
		fprintf(stdout, "%s\n\thex: 0x%llx%llx:0x%llx%llx\n",
			getstr_u128(offset, int_buf),
			(ull)(lba >> (sizeof(maxuint_t) << 2)), (ull)(lba),
			(ull)(offset >> (sizeof(maxuint_t) << 2)), (ull)(offset));
	}

	return 0;
}
