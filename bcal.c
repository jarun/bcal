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

#include <ctype.h>
#include <quadmath.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dslib.h"

#define TRUE 1
#define FALSE !TRUE

#define SECTOR_SIZE 512 /* 0x200 */
#define MAX_HEAD 16 /* 0x10 */
#define MAX_SECTOR 63 /* 0x3f */
#define UINT_BUF_LEN 40 /* log10(1 << 128) + '\0' */
#define FLOAT_BUF_LEN 128
#define FLOAT_WIDTH 40

#ifdef __SIZEOF_INT128__
typedef __uint128_t maxuint_t;
typedef __float128 maxfloat_t;
#else
typedef __uint64_t maxuint_t;
typedef double maxfloat_t;
#endif

char *VERSION = "1.5";
char *units[] = {"b", "kib", "mib", "gib", "tib", "kb", "mb", "gb", "tb"};

char uint_buf[UINT_BUF_LEN];
char float_buf[FLOAT_BUF_LEN];

typedef unsigned char bool;
typedef unsigned long ulong;
typedef unsigned long long ull;

typedef struct {
	ulong c;
	ulong h;
	ulong s;
} t_chs;

void binprint(maxuint_t n)
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
		fprintf(stdout, "%40s %s\n", getstr_u128((maxuint_t)val, uint_buf), unit);
	else
		fprintf(stdout, "%s %s\n", getstr_f128(val, float_buf), unit);
}

void printhex_u128(maxuint_t n)
{
	ull high = (ull)(n >> (sizeof(maxuint_t) << 2));

	if (high)
		fprintf(stdout, "0x%llx%llx", high, (ull)n);
	else
		fprintf(stdout, "0x%llx", (ull)n);
}

char *strtolower(char *buf)
{
	char *p = buf;

	for (; *p; ++p)
		*p = tolower(*p);

	return buf;
}

/* This function adds check for binary input to strtoul() */
ulong strtoul_b(char *token)
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
ull strtoull_b(char *token)
{
	int base = 0;

	/* NOTE: no NULL check here! */

	if (strlen(token) > 2 && token[0] == '0' &&
			(token[1] == 'b' || token[1] == 'B')) {
		base = 2;
	}

	return strtoull(token + base, NULL, base);
}

maxuint_t convertbyte(char *buf)
{
	/* Convert and print in bytes */
	maxuint_t bytes = strtoull(buf, NULL, 0);
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	fprintf(stdout, "%40s B\n", getstr_u128(bytes, uint_buf));

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
	ulong chsparam[5] = {0, 0, 0, MAX_HEAD, MAX_SECTOR};

	ptr = token = chs;

	while (*ptr && token_no < 5) {
		if (*ptr == '-') {
			/* Replace '-' with NULL and get the token */
			*ptr = '\0';
			chsparam[token_no++] = strtoul_b(token);
			/* Restore the '-' */
			*ptr++ = '-';
			/* Point to start of next token */
			token = ptr;

			if (*ptr == '\0' && token_no < 5)
				chsparam[token_no++] = strtoul_b(token);

			continue;
		}

		ptr++;

		if (*ptr == '\0' && token_no < 5)
			chsparam[token_no++] = strtoul_b(token);
	}

	/* Fail if CHS is omitted */
	if (token_no < 3) {
		fprintf(stderr, "CHS2LBA: CHS missing\n");
		return FALSE;
	}

	if (!chsparam[3]) {
		fprintf(stderr, "CHS2LBA: MAX_HEAD = 0\n");
		return FALSE;
	}

	if (!chsparam[4]) {
		fprintf(stderr, "CHS2LBA: MAX_SECTOR = 0\n");
		return FALSE;
	}

	if (!chsparam[2]) {
		fprintf(stderr, "CHS2LBA: S = 0\n");
		return FALSE;
	}

	if (chsparam[1] > chsparam[3]) {
		fprintf(stderr, "CHS2LBA: H > MAX_HEAD\n");
		return FALSE;
	}

	if (chsparam[2] > chsparam[4]) {
		fprintf(stderr, "CHS2LBA: S > MAX_SECTOR\n");
		return FALSE;
	}


	*lba = chsparam[3] * chsparam[4] * chsparam[0]; /* MH * MS * C */
	*lba += chsparam[4] * chsparam[1]; /* MS * H */

	*lba += chsparam[2] - 1; /* S - 1 */

	fprintf(stdout, "\033[1mCHS2LBA\033[0m\n\tC %lu H %lu S %lu MAX_HEAD %lu MAX_SECTOR %lu\n",
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
			chsparam[token_no++] = strtoull_b(token);
			*ptr++ = '-';
			token = ptr;

			if (*ptr == '\0' && token_no < 3)
				chsparam[token_no++] = strtoull_b(token);

			continue;
		}

		ptr++;

		if (*ptr == '\0' && token_no < 3)
			chsparam[token_no++] = strtoull_b(token);
	}

	/* Fail if LBA is omitted */
	if (!token_no) {
		fprintf(stderr, "LBA2CHS: LBA missing\n");
		return FALSE;
	}

	if (!chsparam[1]) {
		fprintf(stderr, "LBA2CHS: MAX_HEAD = 0\n");
		return FALSE;
	}

	if (!chsparam[2]) {
		fprintf(stderr, "LBA2CHS: MAX_SECTOR = 0\n");
		return FALSE;
	}

	p_chs->c = (ulong)(chsparam[0] / (chsparam[2] * chsparam[1])); /* L / (MS * MH) */

	p_chs->h = (ulong)((chsparam[0] / chsparam[2]) % chsparam[1]); /* (L / MS) % MH */
	if (p_chs->h > MAX_HEAD) {
		fprintf(stderr, "LBA2CHS: H > MAX_HEAD\n");
		return FALSE;
	}

	p_chs->s = (ulong)((chsparam[0] % chsparam[2]) + 1); /* (L % MS) + 1 */
	if (p_chs->s > MAX_SECTOR) {
		fprintf(stderr, "LBA2CHS: S > MAX_SECTOR\n");
		return FALSE;
	}

	fprintf(stdout, "\033[1mLBA2CHS\033[0m\n\tLBA %s ",
		getstr_u128(chsparam[0], uint_buf));
	fprintf(stdout, "MAX_HEAD %s ", getstr_u128(chsparam[1], uint_buf));
	fprintf(stdout, "MAX_SECTOR %s\n", getstr_u128(chsparam[2], uint_buf));

	return TRUE;
}

void InputError()
{
	fprintf(stderr, "\nPlease enter inputs properly. Note that a byte unit can only be divided or multiplied by a plain integer.\nAlso only byte units can be used for addition or substraction.\nExample:\nbcal \"(5kb+2mb)/3\"\nbcal \"5tb/12\"\nbcal \"2.5mb*3\" etc.\n\n");
	exit(-1);
}

/* Convert any unit in bytes */
maxuint_t unitconv(Data bunit, short *u) 
{
		/* Data is a C structure containing a string p and a short indicating if the string is a unit or a plain number */
		char *numstr = bunit.p;

		if(numstr == NULL) {	/* If the string is empty */
			InputError();	/* Throw Error */	
		}

		char *num;
		char unit[4] = {'\0'};
		int len = strlen(numstr);

		if (isalpha(numstr[len - 3])) {		/* 3 char unit Eg: Kib, Mib etc. */
			unit[0] = numstr[len - 3];
			unit[1] = numstr[len - 2];
			unit[2] = numstr[len - 1];
			numstr[len - 1] = numstr[len - 2] = numstr[len - 3] = '\0';
			*u = 1;
			
		} else if (isalpha(numstr[len - 2])) {	/* 2 char unit Eg: kb, mb etc. */
			unit[0] = numstr[len - 2];
			unit[1] = numstr[len - 1];
			numstr[len - 1] = numstr[len - 2] = '\0';
			*u = 1;
		
		} else if (isalpha(numstr[len - 1])) {	/* 1 char unit Eg: b */
			unit[0] = numstr[len - 1];
			numstr[len - 1] = '\0';
			*u = 1;
		} else {
			if(*u != 1)	/* If not already converted from unit to bytes */
				*u = 0;
			return strtoull(numstr, NULL, 0);
		}

		num = numstr;		

		int count = 9;	/* Number of available units is 9(from "b" to "tb"). */
		maxuint_t bytes = 0;

		while (count-- > 0)
			if (!strcmp(units[count], strtolower(unit)))
                                break;

		if (count == -1) {
			fprintf(stderr, "No matching unit\n");
			InputError();
		}

		maxfloat_t byte_metric = 0;

		switch (count) {
		case 0:
			bytes = strtoull(num, NULL, 0);
			break;
		case 1:
			byte_metric = strtod(num, NULL);	
			bytes = (maxuint_t)(byte_metric * 1024);	/* Kibibyte */
			break;
		case 2:
			byte_metric = strtod(num, NULL);
			bytes = (maxuint_t)(byte_metric * (1 << 20));	/* Mebibyte */
			break;
		case 3:
			byte_metric = strtod(num, NULL);
			bytes = (maxuint_t)(byte_metric * (1 << 30));	/* Gibibyte */
			break;
		case 4:
			byte_metric = strtod(num, NULL);
			bytes = (maxuint_t)(byte_metric * ((maxuint_t)1 << 40)); /* Tebibyte */
			break;
		case 5:
			byte_metric = strtod(num, NULL);
			bytes = (maxuint_t)(byte_metric * 1000);	/* Kilobyte */
			break;
		case 6:
			byte_metric = strtod(num, NULL);
			bytes = (maxuint_t)(byte_metric * 1000000);	/* Megabyte */
			break;
		case 7:
			byte_metric = strtod(num, NULL);
			bytes = (maxuint_t)(byte_metric * 1000000000);	/* Gigabyte */
			break;
		case 8:
			byte_metric = strtod(num, NULL);
			bytes = (__uint128_t)(byte_metric * 1000000000000);	/* Terabyte */
			break;
		default:
			fprintf(stderr, "Unknown unit\n");
			return 1;
		}
	*u = 1;
	return bytes;
}

int prio(char var) /* Get the priority of operators */
{
	switch(var)
	{
		case '+': return 2; break;
		case '-': return 1; break;
		case '*': return 3; break;
		case '/': return 4; break;
	}
	return 0;
}

int checkExp(char *exp)	/* Check if arithmetic expression */
{
	int k = strtoull(exp, NULL, 0);
	char *e2 = getstr_u128(k, uint_buf);

	if (strcmp(e2, exp) == 0) {
		return 0;
	} 
	return 1;
}

/* Convert Infix mathematical expression to Postfix */
void Infix2Postfix(char *exp, queue **resf, queue **resr)
{
	if (!checkExp(exp))
		return;
		
	stack *op = NULL;			/* Opeartor Stack */
	char *token = strtok(exp, " ");
	char e = '\0';
	Data tokenData = {"\0", 0};		/* C structure: distinguish between plain data & unit data */
	int BraceBalence = 0;

	while (token != NULL) {
		
		e = token[0];
		strcpy(tokenData.p, token);	/* Copy arument to string part of the structure */
		
		switch(e) {
	
			case '+':
			case '-':
			case '*':
			case '/':
					if(token[1] != '\0') {	/* NULL String */
						InputError();
					}
					while (!isEmpty(op) && top(op)[0] != '(' && prio(e) <= prio(top(op)[0])) {

							Data ct = pop(&op);	/* Pop from operator stack */
							Enqueue(resf, resr, ct);/* Insert to Queue */
				        }
					push(&op, tokenData);
					break;
			
			case '(':	BraceBalence++;
					push(&op, tokenData);
					break;
		
			case ')':	while (!isEmpty(op) && top(op)[0] != '(') {
							Data ct = pop(&op);
							Enqueue(resf, resr ,ct);
					}
					pop(&op);
					BraceBalence--;
					break;
						
			default:	Enqueue(resf, resr, tokenData);  /* For Operands */				
		}

		token = strtok(NULL," ");
	}
	
	while (!isEmpty(op)) {
		Enqueue(resf , resr, pop(&op));	/* Put remaining elements into the queue */
	}

	if (BraceBalence != 0)
		InputError();
}

/* Evaluates Postfix Expression */
maxuint_t eval(queue **front, queue **rear)
{
	maxuint_t ans = 0;
	stack *est = NULL;
	Data ansdata;

	if (*front == NULL)	/* If Queue is Empty */
		return 0;

	if (*front == *rear) {		/* If only one element in the queue */
		short s = 0;
		ansdata = Dequeue(front,rear);
		return unitconv(ansdata, &s);
	}
		
	while (*front != NULL && *rear != NULL) {
	
		Data arg = Dequeue(front, rear);
		
		if (strlen(arg.p) == 1 && !isdigit(arg.p[0])){ /* Check if arg is an operator */
		
			Data raw_b = pop(&est);			/* Pop data from stack */
			Data raw_a = pop(&est);

			maxuint_t b = unitconv(raw_b, &raw_b.unit);	/* Convert to integer */
			maxuint_t a = unitconv(raw_a, &raw_a.unit);

			maxuint_t c = 0;	/* Result data */
			Data raw_c;
			
			switch (arg.p[0]) {
			
				case '+': if (raw_a.unit && raw_b.unit) { /* Check if both are units */					
					  	c = a + b;
						raw_c.unit = 1;
					  } else {
						InputError();
					  }
					  break;
				case '-': if (raw_a.unit && raw_b.unit) { /* Check if both are units */		
					  	c = a - b;
						raw_c.unit = 1;
					  } else {
						InputError();
					  }
					  break;
				case '*': if (!raw_a.unit || !raw_b.unit) { /* Check if only one is unit */		
					  	c = a * b;
						raw_c.unit = 1;
					  } else {
						InputError();
					  }
					  break;
				case '/': if (raw_a.unit && !raw_b.unit) { /* Check if only the dividend is unit */		
					  	c = a / b;
						raw_c.unit = 1;
					  } else {
						InputError();
					  }
					  break;
			}
			
			strcpy(raw_c.p, getstr_u128(c, uint_buf));	/* Convert to string */			        
			push(&est, raw_c);				/* Put in stack */
		
		} else {	      
			push(&est, arg);	
		}			
	}
	
	ansdata = pop(&est);
        ans = strtoull(ansdata.p, NULL, 0);	/* Convert string to integer */	
	
	return ans;
}

int isOpr(char c) { /* Check if a char is operator or not */

	switch (c) {
	
		case '+':
		case '-':
		case '*':
		case '/':
		case '(':
		case ')': return 1;
		 default: return 0;
	}
}

char *FixExpr(char *exp) {	/* Make the expression compatible with parsing by inserting/removing space between arguments */
	
	char *newExp = (char*)malloc(2 * strlen(exp) * sizeof(char));
	int i=0 ,j=0;
	
	while (exp[i] != '\0') {
	
		if ((isdigit(exp[i]) && isOpr(exp[i + 1])) ||
		    ( isOpr(exp[i]) && isdigit(exp[i + 1])) ||
		    ( isOpr(exp[i]) && isOpr(exp[i + 1])) ||
		    (isalpha(exp[i]) && isOpr(exp[i + 1])) ) {
			
			newExp[j++] = exp[i];
			newExp[j++] = ' ';
			newExp[j] = exp[i + 1];
		
		} else if (isdigit(exp[i]) && exp[i + 1]==' ' && isalpha(exp[i + 2])) {
			
			newExp[j++] = exp[i];
			newExp[j] = exp[i + 2];
			i++;
		} else
			newExp[j++] = exp[i];
	  i++;
	}
	return newExp;
}

void usage()
{
	fprintf(stdout, "usage: bcal [-c N] [-f FORMAT] [-s bytes] [-h]\n\
            [N unit]\n\n\
Perform storage conversions and calculations.\n\n\
positional arguments:\n\
  N unit           capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB\n\
                   see https://wiki.ubuntu.com/UnitsPolicy\n\
                   must be space-separated, case is ignored\n\
                   N can be decimal or '0x' prefixed hex value\n\n\
  \"Expresstion\"  arithmetic operation\n\n\
optional arguments:\n\
  -c N             show N in binary, decimal and hex\n\
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
                   default MAX_HEAD: 16, default MAX_SECTOR: 63\n\
  -s bytes         sector size [default 512]\n\
  -h               show this help and exit\n\n\
Version %s\n\
Copyright © 2016-2017 Arun Prakash Jana <engineerarun@gmail.com>\n\
License: GPLv3\n\
Webpage: https://github.com/jarun/bcal\n", VERSION);
}

int main(int argc, char **argv)
{
	int opt = 0;
	ulong sectorsize = SECTOR_SIZE;

	opterr = 0;

	while ((opt = getopt(argc, argv, "c:f:hs:")) != -1) {
		switch (opt) {
		case 'c':
			if (*optarg == '-') {
				fprintf(stderr, "N must be >= 0\n");
				return 1;
			}

			fprintf(stdout, "\033[1mBASE CONVERSION\033[0m\n");
			maxuint_t val = strtoull_b(optarg);

			fprintf(stdout, "\tbin: ");
			binprint(val);
			fprintf(stdout, "\n\tdec: %s\n\thex: ", getstr_u128(val, uint_buf));
			printhex_u128(val);
			fprintf(stdout, "\n\n");
			break;
		case 'f':
			if (tolower(*optarg) == 'c') {
				maxuint_t lba = 0;
				if (chs2lba(optarg + 1, &lba)) {
					fprintf(stdout, "\tLBA: (dec) %s, (hex) ",
						getstr_u128(lba, uint_buf));
					printhex_u128(lba);
					fprintf(stdout, "\n\n");
				} else
					fprintf(stderr, "Invalid input\n");
			} else if (tolower(*optarg) == 'l') {
				t_chs chs;
				if (lba2chs(optarg + 1, &chs))
					fprintf(stdout, "\tCHS: (dec) %lu %lu %lu, (hex) 0x%lx 0x%lx 0x%lx\n\n",
						chs.c, chs.h, chs.s, chs.c, chs.h, chs.s);
				else
					fprintf(stderr, "Invalid input\n");
			} else
				fprintf(stderr, "Invalid input\n");
			break;
		case 's':
			if (*optarg == '-') {
				fprintf(stderr, "sector size must be +ve\n");
				return 1;
			}
			sectorsize = strtoul_b(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return 1;
		}
	}

	if (argc == 1 && optind == 1) {
		usage();
		return 1;
	}

	if (argc - optind == 2) {
		int count = sizeof(units)/sizeof(*units);
		maxuint_t bytes = 0, lba = 0, offset = 0;

		while (count-- > 0)
			if (!strcmp(units[count], strtolower(argv[optind + 1])))
                                break;

		if (count == -1) {
			fprintf(stderr, "No matching unit\n");
			return 1;
		}

		fprintf(stdout, "\033[1mUNIT CONVERSION\033[0m\n");

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

		fprintf(stdout, "\n    ADDRESS\n\tdec: %s\n\thex: ", getstr_u128(bytes, uint_buf));
		printhex_u128(bytes);

		/* Calculate LBA and offset */
		lba = bytes / sectorsize;
		offset = bytes % sectorsize;

		fprintf(stdout, "\n\n    LBA:OFFSET\n\tsector size: 0x%lx\n", sectorsize);
		/* We use a global buffer, so print decimal lba first, then offset */
		fprintf(stdout, "\n\tdec: %s:", getstr_u128(lba, uint_buf));
		fprintf(stdout, "%s\n\thex: ", getstr_u128(offset, uint_buf));
		printhex_u128(lba);
		fprintf(stdout, ":");
		printhex_u128(offset);
		fprintf(stdout, "\n");
	}
	
	/*Arithmetic Operation*/
	if (argc - optind == 1) {
	
		char *expr = FixExpr(argv[1]);	 /* Make parsing compatible */
		
		maxuint_t byteans = 0;
		maxuint_t lba = 0, offset = 0;

		queue *front = NULL, *rear = NULL;		
		Infix2Postfix(expr, &front, &rear);
		byteans = eval(&front, &rear);		/* Evaluate Expression */

		fprintf(stdout, "\033[1mUNIT  CONVERSION\033[0m\n");

		convertbyte(getstr_u128(byteans, uint_buf));

		fprintf(stdout, "\n    ADDRESS\n\tdec: %s\n\thex: ", getstr_u128(byteans, uint_buf));
		printhex_u128(byteans);

		/* Calculate LBA and offset */
		lba = byteans / sectorsize;
		offset = byteans % sectorsize;

		fprintf(stdout, "\n\n    LBA:OFFSET\n\tsector size: 0x%lx\n", sectorsize);
		/* We use a global buffer, so print decimal lba first, then offset */
		fprintf(stdout, "\n\tdec: %s:", getstr_u128(lba, uint_buf));
		fprintf(stdout, "%s\n\thex: ", getstr_u128(offset, uint_buf));
		printhex_u128(lba);
		fprintf(stdout, ":");
		printhex_u128(offset);
		fprintf(stdout, "\n");		
	}

	return 0;
}
