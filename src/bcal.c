/*
 * Byte CALculator
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
 * Copyright (C) 2016 by Arun Prakash Jana <engineerarun@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bcal.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "dslib.h"
#include "log.h"

#define TRUE 1
#define FALSE !TRUE

#define SECTOR_SIZE 512 /* 0x200 */
#define MAX_HEAD 16 /* 0x10 */
#define MAX_SECTOR 63 /* 0x3f */
#define UINT_BUF_LEN 40 /* log10(1 << 128) + '\0' */
#define FLOAT_BUF_LEN 128
#define FLOAT_WIDTH 40
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_BITS 128
#define _ALIGNMENT_MASK 0xF

typedef unsigned char bool;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ull;
typedef long double maxfloat_t;

#ifdef __SIZEOF_INT128__
typedef __uint128_t maxuint_t;
#else
typedef __uint64_t maxuint_t;
#endif

typedef struct {
	ulong c;
	ulong h;
	ulong s;
} t_chs;

static char *VERSION = "1.9";
static char *units[] = {"b", "kib", "mib", "gib", "tib", "kb", "mb", "gb", "tb"};

static char *FAILED = "1";
static char *PASSED = "\0";
static char *curexpr = NULL;

static char uint_buf[UINT_BUF_LEN];
static char float_buf[FLOAT_BUF_LEN];

static bool bcmode;
static int minimal;
static int repl;

static Data lastres = {"\0", 0};

static int cur_loglevel = INFO;
static char *logarr[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

static void debug_log(const char *func, int level, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	if (level < 0 || level > DEBUG)
		return;

	if (level <= cur_loglevel) {
		if (cur_loglevel == DEBUG) {
			fprintf(stderr, "%s(), %s: ", func, logarr[level]);
			vfprintf(stderr, format, ap);
		} else {
			fprintf(stderr, "%s: ", logarr[level]);
			vfprintf(stderr, format, ap);
		}
	}

	va_end(ap);
}

/*
 * Just a safe strncpy(3)
 * Always null ('\0') terminates if both src and dest are valid pointers.
 * Returns the number of bytes copied including terminating null byte.
 */
static size_t bstrlcpy(char *dest, const char *src, size_t n)
{
	static ulong *s, *d;
	static size_t len, blocks;
	static const uint lsize = sizeof(ulong);
	static const uint _WSHIFT = (sizeof(ulong) == 8) ? 3 : 2;

	if (!src || !dest || !n)
		return 0;

	len = strlen(src) + 1;
	if (n > len)
		n = len;
	else if (len > n)
		/* Save total number of bytes to copy in len */
		len = n;

	/*
	 * To enable -O3 ensure src and dest are 16-byte aligned
	 * More info: http://www.felixcloutier.com/x86/MOVDQA.html
	 */
	if ((n >= lsize) && (((ulong)src & _ALIGNMENT_MASK) == 0 && ((ulong)dest & _ALIGNMENT_MASK) == 0)) {
		s = (ulong *)src;
		d = (ulong *)dest;
		blocks = n >> _WSHIFT;
		n &= lsize - 1;

		while (blocks) {
			*d = *s;
			++d, ++s;
			--blocks;
		}

		if (!n) {
			dest = (char *)d;
			*--dest = '\0';
			return len;
		}

		src = (char *)s;
		dest = (char *)d;
	}

	while (--n && (*dest = *src))
		++dest, ++src;

	if (!n)
		*dest = '\0';

	return len;
}

/*
 * Try to evaluate en expression using bc
 * If argument is NULL, global curexpr is picked
 */
static int try_bc(char *expr)
{
	pid_t pid;
	int pipe_pc[2], pipe_cp[2], ret;

	if (!expr) {
		if (curexpr)
			expr = curexpr;
		else
			return -1;
	}

	log(DEBUG, "expression: \"%s\"\n", expr);

	if (pipe(pipe_pc) == -1 || pipe(pipe_cp) == -1) {
		printf("Could not pipe\n");
		exit(EXIT_FAILURE);
	}

	pid = fork();

	if (pid == -1) {
		log(ERROR, "fork() failed!\n");
		return -1;
	}

	if (pid == 0) { /* child */
		close(STDOUT_FILENO);
		close(STDIN_FILENO);
		close(pipe_pc[1]); // Close writing end
		close(pipe_cp[0]); // Close reading end

		dup2(pipe_pc[0], STDIN_FILENO); // Take stdin from parent
		dup2(pipe_cp[1], STDOUT_FILENO); // Give stdout to parent
		dup2(pipe_cp[1], STDERR_FILENO); // Give stderr to parent

		ret = execlp("bc", "bc", "-q", (char*) NULL);
		log(ERROR, "execlp() failed!\n");
		exit(ret);
	}

	/* parent */
	char buffer[128] = "";
	ret = write(pipe_pc[1], "scale=5\n", 8);

	ret = write(pipe_pc[1], "last=", 5);
	if (lastres.p[0])
		ret = write(pipe_pc[1], lastres.p, strlen(lastres.p));
	else
		ret = write(pipe_pc[1], "0", 1);
	ret = write(pipe_pc[1], "\n", 1);

	ret = write(pipe_pc[1], expr, strlen(expr));
	ret = write(pipe_pc[1], "\n", 1);
	ret = read(pipe_cp[0], buffer, sizeof(buffer));

	if (buffer[0] != '(') {
		printf("%s", buffer);
		bstrlcpy(lastres.p, buffer, NUM_LEN);
		/* remove newline */
		lastres.p[strlen(lastres.p) - 1] = '\0';
		lastres.unit = 0;
		log(DEBUG, "result: %s %d\n", lastres.p, lastres.unit);
		return 0;
	}

	log(ERROR, "invalid expression\n");
	return -1;
}

static void binprint(maxuint_t n)
{
	int count = MAX_BITS - 1;
	char binstr[MAX_BITS + 1] = {0};

	if (!n) {
		printf("0b0");
		return;
	}

	while (n && count >= 0) {
		binstr[count] = "01"[n & 1];
		--count;
		n >>= 1;
	}

	++count;

	printf("0b%s", binstr + count);
}

static char *getstr_u128(maxuint_t n, char *buf)
{
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

static char *getstr_f128(maxfloat_t val, char *buf)
{
	int n = snprintf(buf, FLOAT_BUF_LEN, "%#*.10Le", FLOAT_WIDTH, val);

	buf[n] = '\0';
	return buf;
}

static void printval(maxfloat_t val, char *unit)
{
	if (val - (maxuint_t)val == 0)
		printf("%40s %s\n", getstr_u128((maxuint_t)val, uint_buf), unit);
	else
		printf("%s %s\n", getstr_f128(val, float_buf), unit);
}

static void printhex_u128(maxuint_t n)
{
	ull high = (ull)(n >> (sizeof(maxuint_t) << 2));

	if (high)
		printf("0x%llx%llx", high, (ull)n);
	else
		printf("0x%llx", (ull)n);
}

/* This function adds check for binary input to strtoul() */
static ulong strtoul_b(char *token)
{
	int base = 0;

	/* NOTE: no NULL check here! */

	if (strlen(token) > 2 && token[0] == '0' &&
	    (token[1] == 'b' || token[1] == 'B')) {
		base = 2;
	}

	return strtoul(token + base, NULL, base);
}

/* This function adds check for binary input to strtoull() */
static ull strtoull_b(char *token)
{
	int base = 0;

	/* NOTE: no NULL check here! */

	if (strlen(token) > 2 && token[0] == '0' &&
	    (token[1] == 'b' || token[1] == 'B')) {
		base = 2;
	}

	return strtoull(token + base, NULL, base);
}

/* Converts a char to unsigned int according to base */
static bool ischarvalid(char ch, uint base, uint *val)
{
	if (base == 2)
	{
		if (ch == '0' || ch == '1') {
			*val = ch - '0';
			return TRUE;
		}
	} else if (base == 16) {
		if (ch >= '0' && ch <= '9') {
			*val = ch - '0';
			return TRUE;
		}

		if (ch >= 'a' && ch <= 'f') {
			*val = (ch - 'a') + 10;
			return TRUE;
		}

		if (ch >= 'A' && ch <= 'F') {
			*val = (ch - 'A') + 10;
			return TRUE;
		}
	} else if (base == 10) {
		if (ch >= '0' && ch <= '9') {
			*val = ch - '0';
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Converts a non-floating representing string to maxuint_t
 */
static maxuint_t strtouquad(char *token, char **pch)
{
	*pch = PASSED;

	if (!token || !*token) {
		*pch = FAILED;
		return 0;
	}

	char *ptr;
	maxuint_t val = 0, prevval = 0;
	uint base = 10, multiplier = 0, digit, bits_used = 0;
	uint max_bit_len = sizeof(maxuint_t) << 3;

	if (token[0] == '0') {
		if (token[1] == 'b' || token[1] == 'B') { /* binary */
			base = 2;
			multiplier = 1;
		} else if (token[1] == 'x' || token[1] == 'X') { /* hex */
			base = 16;
			multiplier = 4;
		}
	}

	if (base == 2 || base == 16) {
		ptr = token + 2;

		if (!*ptr) {
			*pch = FAILED;
			return 0;
		}

		while (*ptr && *ptr == '0')
			++ptr;

		if (!*ptr)
			return 0;

		while (*ptr) {
			if (bits_used == max_bit_len || !ischarvalid(*ptr, base, &digit)) {
				*pch = FAILED;
				return 0;
			}

			val = (val << multiplier) + digit;

			++bits_used;
			++ptr;
		}

		return val;
	}

	/* Try base 10 for any other pattern */
	ptr = token;
	while (*ptr && *ptr == '0')
		++ptr;

	if (!*ptr)
		return 0;

	while (*ptr) {
		if (!ischarvalid(*ptr, base, &digit)) {
			*pch = FAILED;
			return 0;
		}

		val = (val * 10) + digit;

		/* Try to to detect overflow */
		if (val < prevval) {
			*pch = FAILED;
			return 0;
		}

		prevval = val;
		++ptr;
	}

	return val;
}

static maxuint_t convertbyte(char *buf, int *ret)
{
	maxfloat_t val;
	char *pch;
	/* Convert and print in bytes (cannot be in float) */
	maxuint_t bytes = strtouquad(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	/* Convert and print in IEC standard units */

	printf("\n            IEC standard (base 2)\n\n");
	val = bytes / (maxfloat_t)1024;
	printval(val, "KiB");

	val = bytes / (maxfloat_t)(1 << 20);
	printval(val, "MiB");

	val = bytes / (maxfloat_t)(1 << 30);
	printval(val, "GiB");

	val = bytes / (maxfloat_t)((maxuint_t)1 << 40);
	printval(val, "TiB");

	/* Convert and print in SI standard values */

	printf("\n            SI standard (base 10)\n\n");
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

static maxuint_t convertkib(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, kib = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(kib * 1024);

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	printval(kib, "KiB");

	val = kib / 1024;
	printval(val, "MiB");

	val = kib / (1 << 20);
	printval(val, "GiB");

	val = kib / (1 << 30);
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
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

static maxuint_t convertmib(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, mib = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(mib * (1 << 20));

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = mib * 1024;
	printval(val, "KiB");

	printval(mib, "MiB");

	val = mib / 1024;
	printval(val, "GiB");

	val = mib / (1 << 20);
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	val = mib * (1 << 20) / 1000;
	printval(val, "kB");

	val = mib * (1 << 20) / 1000000;
	printval(val, "MB");

	val = mib * (1 << 20) / 1000000000;
	printval(val, "GB");

	val = mib * (1 << 20) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

static maxuint_t convertgib(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, gib = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(gib * (1 << 30));

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = gib * (1 << 20);
	printval(val, "KiB");

	val = gib * 1024;
	printval(val, "MiB");

	printval(gib, "GiB");

	val = gib / 1024;
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	val = gib * (1 << 30) / 1000;
	printval(val, "kB");

	val = gib * (1 << 30) / 1000000;
	printval(val, "MB");

	val = gib * (1 << 30) / 1000000000;
	printval(val, "GB");

	val = gib * (1 << 30) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

static maxuint_t converttib(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, tib = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(tib * ((maxuint_t)1 << 40));

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = tib * (1 << 30);
	printval(val, "KiB");

	val = tib * (1 << 20);
	printval(val, "MiB");

	val = tib * 1024;
	printval(val, "GiB");

	printval(tib, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	val = tib * ((maxuint_t)1 << 40) / 1000;
	printval(val, "kB");

	val = tib * ((maxuint_t)1 << 40) / 1000000;
	printval(val, "MB");

	val = tib * ((maxuint_t)1 << 40) / 1000000000;
	printval(val, "GB");

	val = tib * ((maxuint_t)1 << 40) / 1000000000000;
	printval(val, "TB");

	return bytes;
}

static maxuint_t convertkb(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, kb = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(kb * 1000);

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = kb * 1000 / 1024;
	printval(val, "KiB");

	val = kb * 1000 / (1 << 20);
	printval(val, "MiB");

	val = kb * 1000 / (1 << 30);
	printval(val, "GiB");

	val = kb * 1000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	printval(kb, "kB");

	val = kb / 1000;
	printval(val, "MB");

	val = kb / 1000000;
	printval(val, "GB");

	val = kb / 1000000000;
	printval(val, "TB");

	return bytes;
}

static maxuint_t convertmb(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, mb = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(mb * 1000000);

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = mb * 1000000 / 1024;
	printval(val, "KiB");

	val = mb * 1000000 / (1 << 20);
	printval(val, "MiB");

	val = mb * 1000000 / (1 << 30);
	printval(val, "GiB");

	val = mb * 1000000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	val = mb * 1000;
	printval(val, "kB");

	printval(mb, "MB");

	val = mb / 1000;
	printval(val, "GB");

	val = mb / 1000000;
	printval(val, "TB");

	return bytes;
}

static maxuint_t convertgb(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, gb = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (maxuint_t)(gb * 1000000000);

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = gb * 1000000000 / 1024;
	printval(val, "KiB");

	val = gb * 1000000000 / (1 << 20);
	printval(val, "MiB");

	val = gb * 1000000000 / (1 << 30);
	printval(val, "GiB");

	val = gb * 1000000000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	val = gb * 1000000;
	printval(val, "kB");

	val = gb * 1000;
	printval(val, "MB");

	printval(gb, "GB");

	val = gb / 1000;
	printval(val, "TB");

	return bytes;
}

static maxuint_t converttb(char *buf, int *ret)
{
	char *pch;
	maxfloat_t val, tb = strtold(buf, &pch);
	if (*pch) {
		*ret = -1;
		return 0;
	} else
		*ret = 0;

	maxuint_t bytes = (__uint128_t)(tb * 1000000000000);

	if (minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	printf("\n            IEC standard (base 2)\n\n");
	val = tb * 1000000000000 / 1024;
	printval(val, "KiB");

	val = tb * 1000000000000 / (1 << 20);
	printval(val, "MiB");

	val = tb * 1000000000000 / (1 << 30);
	printval(val, "GiB");

	val = tb * 1000000000000 / ((maxuint_t)1 << 40);
	printval(val, "TiB");

	printf("\n            SI standard (base 10)\n\n");
	val = tb * 1000000000;
	printval(val, "kB");

	val = tb * 1000000;
	printval(val, "MB");

	val = tb * 1000;
	printval(val, "GB");

	printval(tb, "TB");

	return bytes;
}

static bool chs2lba(char *chs, maxuint_t *lba)
{
	int token_no = 0;
	char *ptr, *token;
	ulong param[5] = {0, 0, 0, MAX_HEAD, MAX_SECTOR};

	ptr = token = chs;

	while (*ptr && token_no < 5) {
		if (*ptr == '-') {
			/* Replace '-' with NULL and get the token */
			*ptr = '\0';
			param[token_no] = strtoul_b(token);
			++token_no;
			/* Restore the '-' */
			*ptr = '-';
			++ptr;
			/* Point to start of next token */
			token = ptr;

			if (*ptr == '\0' && token_no < 5) {
				param[token_no] = strtoul_b(token);
				++token_no;
			}

			continue;
		}

		++ptr;

		if (*ptr == '\0' && token_no < 5) {
			param[token_no] = strtoul_b(token);
			++token_no;
		}
	}

	/* Fail if CHS is omitted */
	if (token_no < 3) {
		log(ERROR, "CHS missing\n");
		return FALSE;
	}

	if (!param[3]) {
		log(ERROR, "MAX_HEAD = 0\n");
		return FALSE;
	}

	if (!param[4]) {
		log(ERROR, "MAX_SECTOR = 0\n");
		return FALSE;
	}

	if (!param[2]) {
		log(ERROR, "S = 0\n");
		return FALSE;
	}

	if (param[1] > param[3]) {
		log(ERROR, "H > MAX_HEAD\n");
		return FALSE;
	}

	if (param[2] > param[4]) {
		log(ERROR, "S > MAX_SECTOR\n");
		return FALSE;
	}


	*lba = param[3] * param[4] * param[0]; /* MH * MS * C */
	*lba += param[4] * param[1]; /* MS * H */

	*lba += param[2] - 1; /* S - 1 */

	printf("\033[1mCHS2LBA\033[0m\n");
	printf("  C:%lu  H:%lu  S:%lu  MAX_HEAD:%lu  MAX_SECTOR:%lu\n",
		param[0], param[1], param[2], param[3], param[4]);

	return TRUE;
}

static bool lba2chs(char *lba, t_chs *p_chs)
{
	int token_no = 0;
	char *ptr, *token;
	ull param[3] = {0, MAX_HEAD, MAX_SECTOR};

	ptr = token = lba;

	while (*ptr && token_no < 3) {
		if (*ptr == '-') {
			*ptr = '\0';
			param[token_no] = strtoull_b(token);
			++token_no;
			*ptr = '-';
			++ptr;
			token = ptr;

			if (*ptr == '\0' && token_no < 3) {
				param[token_no] = strtoull_b(token);
				++token_no;
			}

			continue;
		}

		++ptr;

		if (*ptr == '\0' && token_no < 3) {
			param[token_no] = strtoull_b(token);
			++token_no;
		}
	}

	/* Fail if LBA is omitted */
	if (!token_no) {
		log(ERROR, "LBA missing\n");
		return FALSE;
	}

	if (!param[1]) {
		log(ERROR, "MAX_HEAD = 0\n");
		return FALSE;
	}

	if (!param[2]) {
		log(ERROR, "MAX_SECTOR = 0\n");
		return FALSE;
	}

	/* L / (MS * MH) */
	p_chs->c = (ulong)(param[0] / (param[2] * param[1]));
	/* (L / MS) % MH */
	p_chs->h = (ulong)((param[0] / param[2]) % param[1]);
	if (p_chs->h > MAX_HEAD) {
		log(ERROR, "H > MAX_HEAD\n");
		return FALSE;
	}

	/* (L % MS) + 1 */
	p_chs->s = (ulong)((param[0] % param[2]) + 1);
	if (p_chs->s > MAX_SECTOR) {
		log(ERROR, "S > MAX_SECTOR\n");
		return FALSE;
	}

	printf("\033[1mLBA2CHS\033[0m\n  LBA:%s  ",
		getstr_u128(param[0], uint_buf));
	printf("MAX_HEAD:%s  ", getstr_u128(param[1], uint_buf));
	printf("MAX_SECTOR:%s\n", getstr_u128(param[2], uint_buf));

	return TRUE;
}

static void show_basic_sizes()
{
	printf("\n---------------\n Storage sizes\n---------------\n"
		"char       : %lu\n"
		"short      : %lu\n"
		"int        : %lu\n"
		"long       : %lu\n"
		"long long  : %lu\n"
#ifdef __SIZEOF_INT128__
		"__int128_t : %lu\n"
#else
		"__int64_t  : %lu\n"
#endif
		"float      : %lu\n"
		"double     : %lu\n"
		"long double: %lu\n",
		sizeof(unsigned char),
		sizeof(unsigned short),
		sizeof(unsigned int),
		sizeof(unsigned long),
		sizeof(unsigned long long),
		sizeof(maxuint_t),
		sizeof(float),
		sizeof(double),
		sizeof(long double));
}

static void usage()
{
	printf("usage: bcal [-c N] [-f FORMAT] [-s bytes] [expr]\n\
            [N [unit]] [-b expr] [-m] [-d] [-h]\n\n\
Storage expression calculator.\n\n\
positional arguments:\n\
 expr       evaluate storage arithmetic expression\n\
            +, -, *, /, >>, << with decimal/hex operands\n\
            Examples:\n\
               bcal \"(5kb+2mb)/3\"\n\
               bcal \"5 tb / 12\"\n\
               bcal \"2.5mb*3\"\n\
               bcal \"(2giB * 2) / (2kib >> 2)\"\n\
 N [unit]   capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB\n\
            see https://wiki.ubuntu.com/UnitsPolicy\n\
            default unit is B (byte), case is ignored\n\
            N can be decimal or '0x' prefixed hex value\n\n\
optional arguments:\n\
 -c N       show +ve integer N in binary, decimal, hex\n\
 -f FORMAT  convert CHS to LBA or LBA to CHS\n\
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
            default MAX_HEAD: 16, default MAX_SECTOR: 63\n\
 -s bytes   sector size [default 512]\n\
 -b expr    evaluate expression in bc\n\
 -m         show minimal output (e.g. decimal bytes)\n\
 -d         enable debug information and logs\n\
 -h         show this help, storage sizes and exit\n\n\
Version %s\n\
Copyright © 2016-2018 Arun Prakash Jana <engineerarun@gmail.com>\n\
License: GPLv3\n\
Webpage: https://github.com/jarun/bcal\n", VERSION);
}

static int bstricmp(const char *s1, const char *s2)
{
	while (*s1 && (tolower(*s1) == tolower(*s2))) {
		++s1;
		++s2;
	}
	return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* Convert any unit in bytes
 * Failure if out parameter holds -1
 */
static maxuint_t unitconv(Data bunit, char *isunit, int *out)
{
	/* Data is a C structure containing a string p and a char
	 * indicating if the string is a unit or a plain number
	 */
	char *numstr = bunit.p, *punit;
	int len, count;
	maxfloat_t byte_metric = 0;

	if (numstr == NULL || *numstr == '\0') {
		log(ERROR, "invalid token\n");
		*out = -1;
		return 0;
	}

	*out = 0;
	len = strlen(numstr) - 1;
	if (isdigit(numstr[len])) {
		char *pc = NULL;
		maxuint_t r;

		/* ensure this is not the result of a previous operation */
		if (*isunit != 1)
			*isunit = 0;
		r = strtoull(numstr, &pc, 0);
		if (*numstr != '\0' && *pc == '\0')
			return r;

		log(ERROR, "invalid token: %s\n", pc);
		*out = -1;
		return 0;
	}

	while (isalpha(numstr[len]))
		--len;

	punit = numstr + len + 1;

	count = ARRAY_SIZE(units);
	while (--count >= 0)
		if (!bstricmp(units[count], punit))
			break;

	if (count == -1) {
		if (minimal)
			log(ERROR, "unknown unit\n");
		else
			try_bc(NULL);

		*out = -1;
		return 0;
	}

	*isunit = 1;
	*punit = '\0';

	switch (count) {
	case 0: {
		maxuint_t bytes;
		bytes = (maxuint_t)strtold(numstr, &punit);
		if (*punit)
			goto error;
		return bytes;
		}
	case 1: /* Kibibyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * 1024);
	case 2: /* Mebibyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * (1 << 20));
	case 3: /* Gibibyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * (1 << 30));
	case 4: /* Tebibyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * ((maxuint_t)1 << 40));
	case 5: /* Kilobyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * 1000);
	case 6: /* Megabyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * 1000000);
	case 7: /* Gigabyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * 1000000000);
	case 8: /* Terabyte */
		byte_metric = strtold(numstr, &punit);
		if (*punit)
			goto error;
		return (maxuint_t)(byte_metric * 1000000000000);
	default:
		log(ERROR, "unknown unit\n");
		*out = -1;
		return 0;
	}

error:
	log(ERROR, "malformed input\n");
	*out = -1;
	return 0;
}

static int priority(char sign) /* Get the priority of operators */
{
	switch (sign) {
	case '>': return 1;
	case '<': return 2;
	case '-': return 3;
	case '+': return 4;
	case '*': return 5;
	case '/': return 6;
	}

	return 0;
}

/* Convert Infix mathematical expression to Postfix */
static int infix2postfix(char *exp, queue **resf, queue **resr)
{
	stack *op = NULL;  /* Operator Stack */
	char *token = strtok(exp, " ");
	Data tokenData = {"\0", 0};
	int balanced = 0;
	Data ct;

	while (token) {
		/* Copy argument to string part of the structure */
		bstrlcpy(tokenData.p, token, NUM_LEN);

		switch (token[0]) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '>':
		case '<':
			if (token[1] != '\0') {
				log(ERROR, "invalid token terminator\n");
				emptystack(&op);
				cleanqueue(resf);
				return -1;
			}

			while (!isempty(op) && top(op)[0] != '(' &&
			       priority(token[0]) <= priority(top(op)[0])) {
				/* Pop from operator stack */
				pop(&op, &ct);
				/* Insert to Queue */
				enqueue(resf, resr, ct);
			}

			push(&op, tokenData);
			break;
		case '(':
			++balanced;
			push(&op, tokenData);
			break;
		case ')':
			while (!isempty(op) && top(op)[0] != '(') {
				pop(&op, &ct);
				enqueue(resf, resr, ct);
			}

			pop(&op, &ct);
			--balanced;
			break;
		case 'r':
			if (lastres.p[0] == '\0') {
				log(ERROR, "no result stored\n");
				emptystack(&op);
				cleanqueue(resf);
				return -1;
			}

			enqueue(resf, resr, lastres);
			break;
		default:
			/* Enqueue operands */
			enqueue(resf, resr, tokenData);
		}

		token = strtok(NULL, " ");
	}

	while (!isempty(op)) {
		/* Put remaining elements into the queue */
		pop(&op, &ct);
		enqueue(resf, resr, ct);
	}

	if (balanced != 0) {
		log(ERROR, "unbalanced expression\n");
		cleanqueue(resf);
		return -1;
	}

	return 0;
}

/*
 * Checks for underflow in division
 * Returns:
 *  0 - no issues
 * -1 - underflow
 */
static int validate_div(maxuint_t dividend, maxuint_t divisor, maxuint_t quotient)
{
	if (divisor * quotient < dividend) {
		log(WARNING, "result truncated\n");

		if (cur_loglevel == DEBUG) {
			printhex_u128(dividend);
			printf(" (dividend)\n");
			printhex_u128(divisor);
			printf(" (divisor)\n");
			printhex_u128(quotient);
			printf(" (quotient)\n");
		}

		return -1;
	}

	return 0;
}

/* Evaluates Postfix Expression
 * Numeric result if out parameter holds 1
 * Failure if out parameter holds -1
 */
static maxuint_t eval(queue **front, queue **rear, int *out)
{
	stack *est = NULL;
	Data res, arg, raw_a, raw_b, raw_c;
	*out = 0;
	maxuint_t a, b, c;

	/* Check if queue is empty */
	if (*front == NULL)
		return 0;

	/* Check if only one element in the queue */
	if (*front == *rear) {
		char unit = 0;

		dequeue(front, rear, &res);
		return unitconv(res, &unit, out);
	}

	while (*front) {
		dequeue(front, rear, &arg);

		/* Check if arg is an operator */
		if (strlen(arg.p) == 1 && !isdigit(arg.p[0])) {
			pop(&est, &raw_b);
			pop(&est, &raw_a);

			b = unitconv(raw_b, &raw_b.unit, out);
			if (*out == -1)
				return 0;
			a = unitconv(raw_a, &raw_a.unit, out);
			if (*out == -1)
				return 0;

			log(DEBUG, "(%s, %d) %c (%s, %d)\n",
			    raw_a.p, raw_a.unit, arg.p[0], raw_b.p, raw_b.unit);

			c = 0;
			raw_c.unit = 0;

			switch (arg.p[0]) {
			case '>':
				if (raw_b.unit) {
					log(ERROR, "unit mismatch in >>\n");
					goto error;
				}

				c = a >> b;
				raw_c.unit = raw_a.unit;
				break;
			case '<':
				if (raw_b.unit) {
					log(ERROR, "unit mismatch in <<\n");
					goto error;
				}

				c = a << b;
				raw_c.unit = raw_a.unit;
				break;
			case '+':
				/* Check if the units match */
				if (raw_a.unit == raw_b.unit) {
					c = a + b;
					if (raw_a.unit)
						raw_c.unit = 1;
					break;
				}

				log(ERROR, "unit mismatch in +\n");
				goto error;
			case '-':
				/* Check if the units match */
				if (raw_a.unit == raw_b.unit) {
					if (b > a) {
						log(ERROR, "negative result\n");
						goto error;
					}

					c = a - b;
					if (raw_a.unit)
						raw_c.unit = 1;
					break;
				}

				log(ERROR, "unit mismatch in -\n");
				goto error;
			case '*':
				/* Check if only one is unit */
				if (!(raw_a.unit && raw_b.unit)) {
					c = a * b;
					if (raw_a.unit || raw_b.unit)
						raw_c.unit = 1;
					break;
				}

				log(ERROR, "unit mismatch in *\n");
				goto error;
			case '/':
				if (b == 0) {
					log(ERROR, "division by 0\n");
					goto error;
				}

				if (raw_a.unit && raw_b.unit) {
					c = a / b;
					raw_c.unit = 0;

					validate_div(a, b, c);
					break;
				} else if (!raw_b.unit) {
					c = a / b;
					if (raw_a.unit)
						raw_c.unit = 1;

					validate_div(a, b, c);
					break;
				}

				log(ERROR, "unit mismatch in /\n");
				goto error;
			}

			/* Convert to string */
			bstrlcpy(raw_c.p, getstr_u128(c, uint_buf), NUM_LEN);
			/* Push to stack */
			push(&est, raw_c);

		} else {
			log(DEBUG, "in else (%s)\n", arg.p);
			push(&est, arg);
		}
	}

	pop(&est, &res);

	/* Stack must be empty at this point */
	if (!isempty(est)) {
		log(ERROR, "invalid expression\n");
		goto error;
	}

	if (res.unit == 0)
		*out = 1;

	/* Convert string to integer */
	return strtoull(res.p, NULL, 0);

error:
	*out = -1;
	emptystack(&est);
	cleanqueue(front);
	return 0;
}

static int issign(char c)
{
	switch (c) {
	case '>':
	case '<':
	case '+':
	case '-':
	case '*':
	case '/':
		return 1;
	default:
		return 0;
	}
}

/* Check if a char is operator or not */
static int isoperator(char c)
{
	switch (c) {
	case '>':
	case '<':
	case '+':
	case '-':
	case '*':
	case '/':
	case '(':
	case ')': return 1;
	default: return 0;
	}
}

#if 0
/* Check if valid storage arithmetic expression */
static int checkexp(char *exp)
{
	while (*exp) {
		if (*exp == 'b' || *exp == 'B')
			return 1;

		++exp;
	}

	return 0;
}
#endif

/* Trim ending newline and whitespace from both ends, in place */
static void strstrip(char *s)
{
	if (!s || !*s)
		return;

	int len = strlen(s) - 1;

	if (s[len] == '\n')
		--len;
	while (len >= 0 && (isspace(s[len]) || s[len] == '\"' || s[len] == '\''))
		--len;
	s[len + 1] = '\0';

	len = 0;
	while (s[len] && (isspace(s[len]) || s[len] == '\"' || s[len] == '\''))
		++len;

	if (len) {
		while (s[len]) {
			*s = s[len];
			++s;
		}

		*s = '\0';
	}
}

/* Replace consecutive inner whitespaces with a single space */
static void removeinnerspaces(char *s)
{
	char *p = s;

	while (*s != '\0') {
		if (!isspace(*s)) {
			*p = *s;
			++p;
		}

		++s;
	}

	*p = '\0';
}

/* Make the expression compatible with parsing by
 * inserting/removing space between arguments
 */
static char *fixexpr(char *exp, int *unitless)
{
	*unitless = 0;

	strstrip(exp);
	removeinnerspaces(exp);

#if 0
	if (!checkexp(exp)) {
		log(DEBUG, "no unit in expression [%s]\n", exp);
		*unitless = 1;
		return NULL;
	}
#endif

	int i = 0, j = 0;
	char *parsed = (char *)calloc(1, 2 * strlen(exp) * sizeof(char));
	char prev = '(';

	log(DEBUG, "exp (%s)\n", exp);

	while (exp[i] != '\0') {
		if (exp[i] == '{' || exp[i] == '}' || exp[i] == '[' || exp[i] == ']') {
			log(ERROR, "first brackets only\n");
			free(parsed);
			return NULL;
		}

		if (exp[i] == '-' && (issign(prev) || prev == '(')) {
			log(ERROR, "negative token\n");
			free(parsed);
			return NULL;
		}

		if (isoperator(exp[i]) && isalpha(exp[i + 1]) && (exp[i + 1] != 'r')) {
			log(ERROR, "invalid expression\n");
			free(parsed);
			return NULL;
		}

		if ((isdigit(exp[i]) && isoperator(exp[i + 1])) ||
		    (isoperator(exp[i]) && (isdigit(exp[i + 1]) ||
		     isoperator(exp[i + 1]))) ||
		    (isalpha(exp[i]) && isoperator(exp[i + 1])) ||
		    (isoperator(exp[i]) && (exp[i + 1] == 'r'))) {
			if (exp[i] == '<' || exp[i] == '>') { /* handle shift operators << and >> */
				log(DEBUG, "in here\n");
				if (prev != exp[i] && exp[i] != exp[i + 1]) {
					log(ERROR, "invalid operator %c\n", exp[i]);
					*unitless = 0;
					return NULL;
				}

				if (prev == exp[i + 1]) { /* handle <<< or >>> */
					log(ERROR, "invalid sequence %c%c%c\n", prev, exp[i], exp[i + 1]);
					*unitless = 0;
					return NULL;
				}

				if (exp[i] == exp[i + 1])
					goto loop_end;
			}

			parsed[j] = exp[i];
			++j;
			parsed[j] = ' ';
			++j;
			parsed[j] = exp[i + 1];
		} else {
			parsed[j] = exp[i];
			++j;
		}

loop_end:
		prev = exp[i];
		++i;
	}

	if (parsed[j])
		parsed[++j] = '\0';

	log(DEBUG, "parsed (%s)\n", parsed);

	/* If there's no space, this is either
	 * a number or malformed expression
	 */
	i = 0;
	while (parsed[i] && parsed[i] != ' ')
		++i;

	if (!parsed[i]) {
		log(DEBUG, "no operator in expression [%s]\n", parsed);
		free(parsed);
		*unitless = 1;
		return NULL;
	}

	return parsed;
}

static int convertunit(char *value, char *unit, ulong sectorsz)
{
	int count = ARRAY_SIZE(units), ret;
	maxuint_t bytes = 0, lba = 0, offset = 0;

	strstrip(value);
	if (value[0] == '\0') {
		log(ERROR, "invalid value\n");
		return -1;
	}

	if (!unit) {
		int unitchars = 0, len = strlen(value);

		while (len) {
			if (!isalpha(value[len - 1]))
				break;

			++unitchars;
			--len;
		}

		if (unitchars) {
			while (--count >= 0)
				if (!bstricmp(units[count], value + len))
					break;

			if (count == -1) {
				log(ERROR, "unknown unit\n");
				return -1;
			}

			value[len] = '\0';
		} else
			count = 0;
	} else {
		strstrip(unit);

		while (--count >= 0)
			if (!bstricmp(units[count], unit))
				break;

		if (count == -1) {
			log(ERROR, "unknown unit\n");
			return -1;
		}
	}

	log(DEBUG, "%s %s\n", value, units[count]);

	if (!minimal && unit)
		printf("\033[1mUNIT CONVERSION\033[0m\n");

	switch (count) {
	case 0:
		bytes = convertbyte(value, &ret);
		break;
	case 1:
		bytes = convertkib(value, &ret);
		break;
	case 2:
		bytes = convertmib(value, &ret);
		break;
	case 3:
		bytes = convertgib(value, &ret);
		break;
	case 4:
		bytes = converttib(value, &ret);
		break;
	case 5:
		bytes = convertkb(value, &ret);
		break;
	case 6:
		bytes = convertmb(value, &ret);
		break;
	case 7:
		bytes = convertgb(value, &ret);
		break;
	case 8:
		bytes = converttb(value, &ret);
		break;
	default:
		log(ERROR, "unknown unit\n");
		return -1;
	}

	if (ret == -1) {
		if (minimal || unit) /* For running python test cases */
			log(ERROR, "malformed input\n");
		else
			return try_bc(NULL);

		return -1;
	}

	bstrlcpy(lastres.p, getstr_u128(bytes, uint_buf), UINT_BUF_LEN);
	lastres.unit = 1;
	log(DEBUG, "result: %s %d\n", lastres.p, lastres.unit);

	if (minimal)
		return 0;

	printf("\nADDRESS\n (d) %s\n (h) ",
		getstr_u128(bytes, uint_buf));
	printhex_u128(bytes);

	/* Calculate LBA and offset */
	lba = bytes / sectorsz;
	offset = bytes % sectorsz;

	printf("\n\nLBA:OFFSET (sector size: 0x%lx)\n", sectorsz);
	/* We use a global buffer, so print decimal lba first, then offset */
	printf(" (d) %s:", getstr_u128(lba, uint_buf));
	printf("%s\n (h) ", getstr_u128(offset, uint_buf));
	printhex_u128(lba);
	printf(":");
	printhex_u128(offset);
	printf("\n");

	return 0;
}

static int evaluate(char *exp, ulong sectorsz)
{
	int ret = 0;
	maxuint_t bytes = 0;
	queue *front = NULL, *rear = NULL;
	char *expr = fixexpr(exp, &ret);  /* Make parsing compatible */
	char *ptr;

	if (expr == NULL) {
		if (ret)
			return convertunit(exp, NULL, sectorsz);

		return -1;
	}

	ret = infix2postfix(expr, &front, &rear);
	free(expr);
	if (ret == -1)
		return -1;

	bytes = eval(&front, &rear, &ret);  /* Evaluate Expression */
	if (ret == -1)
		return -1;
	else if (ret == 1) {
		ptr = getstr_u128(bytes, uint_buf);
		printf("%s\n", ptr);
		bstrlcpy(lastres.p, getstr_u128(bytes, uint_buf), UINT_BUF_LEN);
		lastres.unit = 0;
		log(DEBUG, "result1: %s %d\n", lastres.p, lastres.unit);
		return 0;
	}

	if (!(minimal || repl))
		printf("\033[1mRESULT\033[0m\n");

	convertbyte(getstr_u128(bytes, uint_buf), &ret);
	if (ret == -1) {
		log(ERROR, "malformed input\n");
		return -1;
	}

	ptr = getstr_u128(bytes, uint_buf);
	bstrlcpy(lastres.p, ptr, UINT_BUF_LEN);
	lastres.unit = 1;
	log(DEBUG, "result2: %s %d\n", lastres.p, lastres.unit);

	if (minimal)
		return 0;

	printf("\nADDRESS\n (d) %s\n (h) ", ptr);
	printhex_u128(bytes);
	printf("\n");

	return 0;
}

int convertbase(char *arg)
{
	char *pch;

	strstrip(arg);

	if (*arg == '-') {
		log(ERROR, "N must be >= 0\n");
		return -1;
	}

	maxuint_t val = strtouquad(arg, &pch);
	if (*pch) {
		log(ERROR, "invalid input\n");
		return -1;
	}

	printf(" (b) ");
	binprint(val);
	printf("\n (d) %s\n (h) ",
		getstr_u128(val, uint_buf));
	printhex_u128(val);
	printf("\n");

	return 0;
}

int main(int argc, char **argv)
{
	int opt = 0, operation = 0;
	ulong sectorsz = SECTOR_SIZE;

	opterr = 0;
	rl_bind_key('\t', rl_insert);

	while ((opt = getopt(argc, argv, "b:c:df:hms:")) != -1) {
		switch (opt) {
		case 'c':
		{
			operation = 1;
			convertbase(optarg);
			printf("\n");
			break;
		}
		case 'f':
			operation = 1;

			if (tolower(*optarg) == 'c') {
				maxuint_t lba = 0;

				if (chs2lba(optarg + 1, &lba)) {
					printf("  LBA: (d) %s, (h) ",
						getstr_u128(lba, uint_buf));
					printhex_u128(lba);
					printf("\n\n");
				}
			} else if (tolower(*optarg) == 'l') {
				t_chs chs;

				if (lba2chs(optarg + 1, &chs)) {
					printf("  CHS: (d) %lu %lu %lu, ",
						chs.c, chs.h, chs.s);
					printf("(h) 0x%lx 0x%lx 0x%lx\n\n",
						chs.c, chs.h, chs.s);
				}
			} else
				log(ERROR, "invalid input\n");
			break;
		case 'm':
			minimal = 1;
			break;
		case 's':
			if (*optarg == '-') {
				log(ERROR, "sector size must be +ve\n");
				return -1;
			}
			sectorsz = strtoul_b(optarg);
			break;
		case 'b':
			return try_bc(optarg);
		case 'd':
			cur_loglevel = DEBUG;
			log(DEBUG, "bcal v%s\n", VERSION);
			log(DEBUG, "maxuint_t is %lu bytes\n", sizeof(maxuint_t));

			break;
		case 'h':
			usage();
			show_basic_sizes();
			return 0;
		default:
			usage();
			return -1;
		}
	}

	log(DEBUG, "argc %d, optind %d\n", argc, optind);

	if (!operation && (argc == optind)) {
		char *ptr = NULL, *tmp = NULL;
		repl = 1;
		int enters = 0;

		printf("q/double Enter -> quit, ? -> help\n");
		while ((tmp = readline("bcal> ")) != NULL) {
			/* Quit on double Enter */
			if (tmp[0] == '\0') {
				if (enters == 1) {
					free(tmp);
					break;
				}

				++enters;
				free(tmp);
				continue;
			}

			enters = 0;

			/* Save the original pointer from readline() */
			ptr = tmp;

			strstrip(tmp);
			if (tmp[0] == '\0') {
				free(ptr);
				continue;
			}

			log(DEBUG, "ptr: [%s]\n", ptr);
			log(DEBUG, "tmp: [%s]\n", ptr);

			add_history(tmp);

			if ((strlen(tmp) == 1) && tmp[1] == '\0') {
				switch (tmp[0]) {
				case 'r':
					/* Show the last stored result */
					if (lastres.p[0] == '\0')
						printf("no result stored\n");
					else {
						printf("r = %s ", lastres.p);
						if (lastres.unit)
							printf("B");
						printf("\n");
					}

					free(ptr);
					continue;
				case 'b':
					bcmode = !bcmode;
					if (bcmode)
						printf("entering bc mode\n");
					else
						printf("exiting bc mode\n");

					free(ptr);
					continue;
				case '?':
					usage();

					free(ptr);
					continue;
				case 'q':
					free(ptr);
					return 0;
				default:
					printf("invalid input\n");
					free(ptr);
					continue;
				}
			}

			if (tmp[0] == 'c') {
				convertbase(tmp + 1);
				free(ptr);
				continue;
			}

			if (bcmode) {
				try_bc(tmp);
				free(ptr);
				continue;
			}

			curexpr = tmp;

			/* Evaluate the expression */
			evaluate(tmp, sectorsz);

			free(ptr);
		}

		return 0;
	}

	/* Unit conversion */
	if (argc - optind == 2)
		if (convertunit(argv[optind], argv[optind + 1], sectorsz) == -1)
			return -1;

	/*Arithmetic operation*/
	if (argc - optind == 1) {
		curexpr = argv[optind];
		return evaluate(argv[optind], sectorsz);
	}

	return -1;
}
