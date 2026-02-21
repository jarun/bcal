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
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <getopt.h>
#ifndef NORL
#include <readline/history.h>
#include <readline/readline.h>
#else
#include <termios.h>
#include <sys/stat.h>
#include <stdlib.h>
#endif
#include "dslib.h"
#include "log.h"

#define SECTOR_SIZE 512 /* 0x200 */
#define MAX_HEAD 16 /* 0x10 */
#define MAX_SECTOR 63 /* 0x3f */
#define UINT_BUF_LEN 40 /* log10(1 << 128) + '\0' */
#define FLOAT_BUF_LEN 128
#define FLOAT_WIDTH 40
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_BITS 128
#define ALIGNMENT_MASK_4BIT 0xF
#define ELEMENTS(x) (sizeof(x) / sizeof(*(x)))

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ull;
typedef long double maxfloat_t;

#ifdef __SIZEOF_INT128__
typedef __uint128_t maxuint_t;
#else
typedef __uint64_t maxuint_t;
#endif

/* CHS representation */
typedef struct {
	ulong c;
	ulong h;
	ulong s;
} t_chs;

/* Settings */
typedef struct {
	uchar maths   : 1;
	uchar minimal : 1;
	uchar repl    : 1;
	uchar rsvd    : 3; /* Reserved for future usage */
	uchar loglvl  : 2;
} settings;

static char *VERSION = "2.5";
static char *units[] = {"b", "kib", "mib", "gib", "tib", "kb", "mb", "gb", "tb"};
static char *logarr[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
static const char *PROMPT_BYTES = "bytes> ";
static const char *PROMPT_MATHS = "maths> ";

static char *FAILED = "1";
static char *PASSED = "\0";
static char *curexpr = NULL;
static char prompt[9] = "bytes> ";

static char uint_buf[UINT_BUF_LEN];
static char float_buf[FLOAT_BUF_LEN];

static Data lastres = {"\0", 0};
static settings cfg = {0, 0, 0, 0, INFO};

#ifdef NORL
/* Native history implementation */
#define MAX_HISTORY 50
#define MAX_INPUT_LEN 4096

static char *history_lines[MAX_HISTORY];
static int history_count = 0;
static char history_file_path[PATH_MAX];

/* Get history file path */
static void get_history_file_path(void)
{
	const char *config_home = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");

	if (config_home && config_home[0] != '\0') {
		snprintf(history_file_path, PATH_MAX, "%s/bcal", config_home);
	} else if (home && home[0] != '\0') {
		snprintf(history_file_path, PATH_MAX, "%s/.config/bcal", home);
	} else {
		history_file_path[0] = '\0';
		return;
	}

	/* Create directory if it doesn't exist */
	mkdir(history_file_path, 0755);

	/* Append history filename */
	strncat(history_file_path, "/history", PATH_MAX - strlen(history_file_path) - 1);
}

/* Read history from file */
static void read_history(const char *unused)
{
	(void)unused;
	FILE *fp;
	char line[MAX_INPUT_LEN];

	get_history_file_path();

	if (history_file_path[0] == '\0')
		return;

	fp = fopen(history_file_path, "r");
	if (!fp)
		return;

	history_count = 0;
	while (fgets(line, MAX_INPUT_LEN, fp) && history_count < MAX_HISTORY) {
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';

		if (line[0] != '\0')
			history_lines[history_count++] = strdup(line);
	}

	fclose(fp);
}

/* Write history to file */
static void write_history(const char *unused)
{
	(void)unused;
	FILE *fp;
	int i, start;

	if (history_file_path[0] == '\0')
		return;

	fp = fopen(history_file_path, "w");
	if (!fp)
		return;

	/* Write only the last MAX_HISTORY entries */
	start = (history_count > MAX_HISTORY) ? (history_count - MAX_HISTORY) : 0;
	for (i = start; i < history_count; i++) {
		if (history_lines[i] && history_lines[i][0] != '\0')
			fprintf(fp, "%s\n", history_lines[i]);
	}

	fclose(fp);
}

/* Add line to history */
static void add_history(const char *line)
{
	if (!line || line[0] == '\0')
		return;

	/* Don't add duplicate of last entry */
	if (history_count > 0 && strcmp(history_lines[history_count - 1], line) == 0)
		return;

	/* Free oldest entry if at capacity */
	if (history_count >= MAX_HISTORY) {
		free(history_lines[0]);
		memmove(history_lines, history_lines + 1, (MAX_HISTORY - 1) * sizeof(char *));
		history_count = MAX_HISTORY - 1;
	}

	history_lines[history_count++] = strdup(line);
}

/* Native readline with arrow key support */
static char *readline(const char *prompt_str)
{
	static char buffer[MAX_INPUT_LEN];
	int pos = 0;
	int len = 0;
	int history_pos = history_count;
	char *saved_input = NULL;
	int c;
	struct termios oldattr, newattr;
	int is_tty = isatty(STDIN_FILENO);

	if (!is_tty) {
		/* Non-TTY mode: use simple fgets */
		if (fgets(buffer, MAX_INPUT_LEN, stdin) == NULL)
			return NULL;

		size_t input_len = strlen(buffer);
		if (input_len > 0 && buffer[input_len - 1] == '\n')
			buffer[input_len - 1] = '\0';

		return strdup(buffer);
	}

	/* Set terminal to raw mode for arrow key capture */
	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	newattr.c_cc[VMIN] = 1;
	newattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);

	/* Print prompt */
	printf("%s", prompt_str);
	fflush(stdout);

	buffer[0] = '\0';
	len = 0;
	pos = 0;

	while (1) {
		unsigned char ch;
		if (read(STDIN_FILENO, &ch, 1) != 1)
			break;
		c = ch;

		if (c == '\n' || c == '\r') {
			/* Enter pressed */
			printf("\n");
			break;
		} else if (c == 127 || c == 8) {
			/* Backspace */
			if (pos > 0) {
				memmove(buffer + pos - 1, buffer + pos, len - pos + 1);
				pos--;
				len--;

				/* Redraw line */
				printf("\r%s%s ", prompt_str, buffer);
				for (int i = len; i < pos + 1; i++)
					printf("\b");
				for (int i = pos; i < len; i++)
					printf("\b");
				fflush(stdout);
			}
		} else if (c == 27) {
			/* Escape sequence */
			if (read(STDIN_FILENO, &ch, 1) != 1)
				continue;
			c = ch;
			if (c == '[') {
				if (read(STDIN_FILENO, &ch, 1) != 1)
					continue;
				c = ch;
				if (c == 'A') {
					/* Up arrow */
					if (history_pos > 0) {
						if (history_pos == history_count && len > 0) {
							/* Save current input */
							if (saved_input)
								free(saved_input);
							saved_input = strdup(buffer);
						}

						history_pos--;
						strcpy(buffer, history_lines[history_pos]);
						len = strlen(buffer);
						pos = len;

						/* Redraw line */
						printf("\r%s%s\033[K", prompt_str, buffer);
						fflush(stdout);
					}
				} else if (c == 'B') {
					/* Down arrow */
					if (history_pos < history_count) {
						history_pos++;
						if (history_pos == history_count) {
							/* Restore saved input */
							if (saved_input) {
								strcpy(buffer, saved_input);
								free(saved_input);
								saved_input = NULL;
							} else {
								buffer[0] = '\0';
							}
						} else {
							strcpy(buffer, history_lines[history_pos]);
						}

						len = strlen(buffer);
						pos = len;

						/* Redraw line */
						printf("\r%s%s\033[K", prompt_str, buffer);
						fflush(stdout);
					}
				} else if (c == 'C') {
					/* Right arrow */
					if (pos < len) {
						pos++;
						printf("\033[C");
						fflush(stdout);
					}
				} else if (c == 'D') {
					/* Left arrow */
					if (pos > 0) {
						pos--;
						printf("\033[D");
						fflush(stdout);
					}
				}
			}
		} else if (c == 4) {
			/* Ctrl-D (EOF) */
			if (len == 0) {
				printf("\n");
				tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
				if (saved_input)
					free(saved_input);
				return NULL;
			}
		} else if (c >= 32 && c < 127) {
			/* Printable character */
			if (len < MAX_INPUT_LEN - 1) {
				memmove(buffer + pos + 1, buffer + pos, len - pos + 1);
				buffer[pos] = c;
				pos++;
				len++;

				/* Redraw from cursor position */
				printf("%c", c);
				if (pos < len) {
					printf("%s", buffer + pos);
					for (int i = pos; i < len; i++)
						printf("\b");
				}
				fflush(stdout);
			}
		}
	}

	buffer[len] = '\0';

	/* Restore terminal attributes */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

	if (saved_input)
		free(saved_input);

	return strdup(buffer);
}
#endif

static void debug_log(const char *func, int level, const char *format, ...)
{
	va_list ap;

	if (level < 0 || level > DEBUG)
		return;

	va_start(ap, format);

	if (level <= cfg.loglvl) {
		if (cfg.loglvl == DEBUG) {
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
	static const uint WORD_SHIFT = (sizeof(ulong) == 8) ? 3 : 2;

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
	if ((n >= lsize) && (((ulong)src & ALIGNMENT_MASK_4BIT) == 0
	    && ((ulong)dest & ALIGNMENT_MASK_4BIT) == 0)) {
		s = (ulong *)src;
		d = (ulong *)dest;
		blocks = n >> WORD_SHIFT;
		n &= lsize - 1;

		while (blocks) {
			*d = *s;
			++d, ++s;
			--blocks;
		}

		if (!n) {
			dest = (char *)d;
			*--dest = '\0'; // NOLINT
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

static bool program_exit(const char *str)
{
	if (!strcmp(str, "exit") || !strcmp(str, "quit"))
		return true;
	return false;
}

static void remove_commas(char *str)
{
	if (!str || !*str)
		return;

	char *iter1, *iter2;

	for (iter1 = iter2 = str; *iter2; iter2++)
		if (*iter2 != ',')
			*iter1++ = *iter2;

	*iter1 = '\0';
}

static bool has_function_call(const char *str)
{
	if (!str || !*str)
		return false;

	for (size_t i = 0; str[i] != '\0'; ++i) {
		if (isalpha((unsigned char)str[i])) {
			size_t j = i + 1;
			while (isalnum((unsigned char)str[j]))
				++j;
			while (isspace((unsigned char)str[j]))
				++j;
			if (str[j] == '(')
				return true;
			i = j;
		}
	}

	return false;
}

static void remove_thousands_commas(char *str)
{
	if (!str || !*str)
		return;

	size_t read = 0;
	size_t write = 0;
	int func_paren_depth = 0;

	while (str[read] != '\0') {
		if (str[read] == '(') {
			/* Check if this is a function call by looking back for an identifier */
			int is_function = 0;
			if (write > 0) {
				size_t j = write - 1;
				/* Skip back over alphanumeric characters */
				while (j > 0 && isalnum((unsigned char)str[j]))
					j--;
				/* If we found alphanumeric chars right before '(', it's likely a function */
				if ((j < write - 1 && isalpha((unsigned char)str[j + 1])) ||
				    (write > 0 && isalpha((unsigned char)str[write - 1])))
					is_function = 1;
			}
			if (is_function)
				func_paren_depth++;
			str[write++] = str[read++];
		} else if (str[read] == ')') {
			if (func_paren_depth > 0)
				func_paren_depth--;
			str[write++] = str[read++];
		} else if (str[read] == ',') {
			/* Don't process comma if we're inside a function call */
			if (func_paren_depth > 0) {
				str[write++] = str[read++];
			} else {
				/* Check if this is a thousands separator */
				if (read > 0 && isdigit((unsigned char)str[read - 1]) &&
				    isdigit((unsigned char)str[read + 1])) {
					size_t k = read + 1;
					int digits = 0;

					while (isdigit((unsigned char)str[k])) {
						++digits;
						++k;
						if (digits > 3)
							break;
					}

					if (digits == 3) {
						++read;
						continue;
					}
				}
				str[write++] = str[read++];
			}
		} else {
			str[write++] = str[read++];
		}
	}

	str[write] = '\0';
}

typedef struct {
	char *digits;
	size_t len;
	int scale;
	bool negative;
} decnum_t;

static bool parse_decimal_token(const char *start, size_t len, decnum_t *out)
{
	if (!start || !out || len == 0)
		return false;

	bool negative = false;
	bool seen_dot = false;
	bool seen_digit = false;
	int scale = 0;
	size_t i = 0;

	if (start[i] == '+' || start[i] == '-') {
		negative = (start[i] == '-');
		++i;
		if (i >= len)
			return false;
	}

	char *digits = (char *)malloc(len + 1);
	if (!digits)
		return false;

	size_t dpos = 0;
	for (; i < len; ++i) {
		unsigned char ch = (unsigned char)start[i];
		if (isdigit(ch)) {
			digits[dpos++] = (char)ch;
			seen_digit = true;
			if (seen_dot)
				++scale;
		} else if (ch == '.' && !seen_dot) {
			seen_dot = true;
		} else {
			free(digits);
			return false;
		}
	}

	if (!seen_digit) {
		free(digits);
		return false;
	}

	/* Trim leading zeros, but keep at least one digit */
	size_t first = 0;
	while (first + 1 < dpos && digits[first] == '0')
		++first;
	if (first > 0) {
		memmove(digits, digits + first, dpos - first);
		dpos -= first;
	}

	digits[dpos] = '\0';

	out->digits = digits;
	out->len = dpos;
	out->scale = scale;
	out->negative = negative;
	return true;
}

static char *mul_digits(const char *a, size_t la, const char *b, size_t lb, size_t *out_len)
{
	if (!a || !b || la == 0 || lb == 0)
		return NULL;

	size_t n = la + lb;
	int *acc = (int *)calloc(n, sizeof(int));
	if (!acc)
		return NULL;

	for (size_t i = 0; i < la; ++i) {
		int da = a[la - 1 - i] - '0';
		for (size_t j = 0; j < lb; ++j) {
			int db = b[lb - 1 - j] - '0';
			acc[n - 1 - (i + j)] += da * db;
		}
	}

	for (size_t k = n - 1; k > 0; --k) {
		if (acc[k] >= 10) {
			acc[k - 1] += acc[k] / 10;
			acc[k] %= 10;
		}
	}

	size_t start = 0;
	while (start + 1 < n && acc[start] == 0)
		++start;

	size_t len = n - start;
	char *digits = (char *)malloc(len + 1);
	if (!digits) {
		free(acc);
		return NULL;
	}

	for (size_t i = 0; i < len; ++i)
		digits[i] = (char)('0' + acc[start + i]);
	digits[len] = '\0';

	free(acc);
	if (out_len)
		*out_len = len;
	return digits;
}

static bool round_digits(char **digits, size_t *len, int *scale, int desired_scale)
{
	if (!digits || !*digits || !len || !scale)
		return false;

	if (*len <= (size_t)(*scale)) {
		size_t pad = (size_t)(*scale) - *len + 1;
		char *tmp = (char *)malloc(*len + pad + 1);
		if (!tmp)
			return false;
		memset(tmp, '0', pad);
		memcpy(tmp + pad, *digits, *len + 1);
		free(*digits);
		*digits = tmp;
		*len += pad;
	}

	if (*scale > desired_scale) {
		int drop = *scale - desired_scale;
		size_t keep_len = *len - (size_t)drop;
		char round_digit = (*digits)[keep_len];

		if (round_digit >= '5') {
			size_t idx = keep_len;
			while (idx > 0) {
				--idx;
				if ((*digits)[idx] < '9') {
					(*digits)[idx]++;
					break;
				}
				(*digits)[idx] = '0';
			}

			if (idx == 0 && (*digits)[0] == '0') {
				char *tmp = (char *)malloc(*len + 2);
				if (!tmp)
					return false;
				tmp[0] = '1';
				memcpy(tmp + 1, *digits, *len + 1);
				free(*digits);
				*digits = tmp;
				++*len;
				++keep_len;
			}
		}

		(*digits)[keep_len] = '\0';
		*len = keep_len;
		*scale = desired_scale;
	} else if (*scale < desired_scale) {
		size_t pad = (size_t)(desired_scale - *scale);
		char *tmp = (char *)malloc(*len + pad + 1);
		if (!tmp)
			return false;
		memcpy(tmp, *digits, *len);
		memset(tmp + *len, '0', pad);
		tmp[*len + pad] = '\0';
		free(*digits);
		*digits = tmp;
		*len += pad;
		*scale = desired_scale;
	}

	return true;
}

static void trim_trailing_zeros(char *buf)
{
	char *dot = strchr(buf, '.');
	if (!dot)
		return;

	char *end = buf + strlen(buf) - 1;
	while (end > dot && *end == '0')
		--end;

	if (end == dot)
		*dot = '\0';
	else
		*(end + 1) = '\0';
}

static bool format_decimal_result(char *digits, size_t len, int scale, bool negative,
				 char *buf, size_t buflen)
{
	if (!digits || !buf || buflen == 0)
		return false;

	if (len == 1 && digits[0] == '0')
		negative = false;

	size_t int_len = len - (size_t)scale;
	size_t needed = len + (scale ? 1 : 0) + (negative ? 1 : 0) + 1;
	if (needed > buflen)
		return false;

	size_t pos = 0;
	if (negative)
		buf[pos++] = '-';

	memcpy(buf + pos, digits, int_len);
	pos += int_len;
	if (scale) {
		buf[pos++] = '.';
		memcpy(buf + pos, digits + int_len, (size_t)scale);
		pos += (size_t)scale;
	}
	buf[pos] = '\0';

	trim_trailing_zeros(buf);
	return true;
}

static bool eval_decimal_multiply(const char *expr, char *out, size_t out_len)
{
	if (!expr || !out || out_len == 0)
		return false;

	const char *p = expr;
	while (isspace((unsigned char)*p))
		++p;

	const char *a_start = p;
	while (*p && (isdigit((unsigned char)*p) || *p == '.' || *p == '+' || *p == '-'))
		++p;
	size_t a_len = (size_t)(p - a_start);
	if (a_len == 0)
		return false;

	while (isspace((unsigned char)*p))
		++p;
	if (*p != '*')
		return false;
	++p;

	while (isspace((unsigned char)*p))
		++p;
	const char *b_start = p;
	while (*p && (isdigit((unsigned char)*p) || *p == '.' || *p == '+' || *p == '-'))
		++p;
	size_t b_len = (size_t)(p - b_start);
	if (b_len == 0)
		return false;

	while (isspace((unsigned char)*p))
		++p;
	if (*p != '\0')
		return false;

	decnum_t a = {0};
	decnum_t b = {0};
	if (!parse_decimal_token(a_start, a_len, &a))
		return false;
	if (!parse_decimal_token(b_start, b_len, &b)) {
		free(a.digits);
		return false;
	}

	size_t prod_len = 0;
	char *prod = mul_digits(a.digits, a.len, b.digits, b.len, &prod_len);
	if (!prod) {
		free(a.digits);
		free(b.digits);
		return false;
	}

	int scale = a.scale + b.scale;
	bool negative = (a.negative != b.negative);

	if (!round_digits(&prod, &prod_len, &scale, 10)) {
		free(a.digits);
		free(b.digits);
		free(prod);
		return false;
	}

	bool ok = format_decimal_result(prod, prod_len, scale, negative, out, out_len);

	free(a.digits);
	free(b.digits);
	free(prod);
	return ok;
}

/* Evaluate arithmetic expression */
static int eval_expr(char *expr_str, maxfloat_t *result);

/* Forward declarations for recursive descent parser */
static int parse_expr(const char *expr, int *pos, maxfloat_t *result);
static int parse_factor(const char *expr, int *pos, maxfloat_t *result);
static int parse_term(const char *expr, int *pos, maxfloat_t *result);
static char *fixexpr(char *exp, int *unitless);

/* Skip whitespace */
static void skip_space(const char *expr, int *pos)
{
	while (expr[*pos] && isspace(expr[*pos]))
		(*pos)++;
}

/* Parse primary expression: numbers, parentheses, functions */
static int parse_factor(const char *expr, int *pos, maxfloat_t *result)
{
	skip_space(expr, pos);

	if (expr[*pos] == '(') {
		(*pos)++;
		if (parse_expr(expr, pos, result) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ')') {
			log(ERROR, "missing closing parenthesis\n");
			return -1;
		}
		(*pos)++;
		return 0;
	}



	/* exp */
	if (strncmp(&expr[*pos], "exp", 3) == 0 && !isalnum(expr[*pos + 3])) {
		*pos += 3;
		skip_space(expr, pos);
		if (expr[*pos] != '(') {
			log(ERROR, "exp requires parenthesis\n");
			return -1;
		}
		(*pos)++;
		if (parse_expr(expr, pos, result) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ')') {
			log(ERROR, "missing closing parenthesis\n");
			return -1;
		}
		(*pos)++;
		*result = expl(*result);
		return 0;
	}

	/* log base */
	if (strncmp(&expr[*pos], "log", 3) == 0 && !isalnum(expr[*pos + 3])) {
		maxfloat_t base;
		maxfloat_t num;
		*pos += 3;
		skip_space(expr, pos);
		if (expr[*pos] != '(') {
			log(ERROR, "log requires parenthesis\n");
			return -1;
		}
		(*pos)++;
		if (parse_expr(expr, pos, &base) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ',') {
			log(ERROR, "log requires two arguments\n");
			return -1;
		}
		(*pos)++;
		if (parse_expr(expr, pos, &num) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ')') {
			log(ERROR, "missing closing parenthesis\n");
			return -1;
		}
		(*pos)++;
		if (base <= 0 || base == 1) {
			log(ERROR, "log base must be positive and not 1\n");
			return -1;
		}
		if (num <= 0) {
			log(ERROR, "log of non-positive number\n");
			return -1;
		}
		*result = logl(num) / logl(base);
		return 0;
	}

	/* ln natural logarithm */
	if (strncmp(&expr[*pos], "ln", 2) == 0 && !isalnum(expr[*pos + 2])) {
		*pos += 2;
		skip_space(expr, pos);
		if (expr[*pos] != '(') {
			log(ERROR, "ln requires parenthesis\n");
			return -1;
		}
		(*pos)++;
		if (parse_expr(expr, pos, result) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ')') {
			log(ERROR, "missing closing parenthesis\n");
			return -1;
		}
		(*pos)++;
		if (*result <= 0) {
			log(ERROR, "ln of non-positive number\n");
			return -1;
		}
		*result = logl(*result);
		return 0;
	}


	/* root */
	if (strncmp(&expr[*pos], "root", 4) == 0 && !isalnum(expr[*pos + 4])) {
		*pos += 4;
		skip_space(expr, pos);
		if (expr[*pos] != '(') {
			log(ERROR, "root requires parenthesis\n");
			return -1;
		}
		(*pos)++;
		maxfloat_t n, x;
		if (parse_expr(expr, pos, &n) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ',') {
			log(ERROR, "root requires two arguments\n");
			return -1;
		}
		(*pos)++;
		if (parse_expr(expr, pos, &x) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ')') {
			log(ERROR, "missing closing parenthesis\n");
			return -1;
		}
		(*pos)++;
		if (n == 0) {
			log(ERROR, "root index cannot be zero\n");
			return -1;
		}
		*result = powl(x, 1.0L / n);
		return 0;
	}

	/* pow */
	if (strncmp(&expr[*pos], "pow", 3) == 0 && !isalnum(expr[*pos + 3])) {
		*pos += 3;
		skip_space(expr, pos);
		if (expr[*pos] != '(') {
			log(ERROR, "pow requires parenthesis\n");
			return -1;
		}
		(*pos)++;
		maxfloat_t left, right;
		if (parse_expr(expr, pos, &left) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ',') {
			log(ERROR, "pow requires two arguments\n");
			return -1;
		}
		(*pos)++;
		if (parse_expr(expr, pos, &right) == -1)
			return -1;
		skip_space(expr, pos);
		if (expr[*pos] != ')') {
			log(ERROR, "missing closing parenthesis\n");
			return -1;
		}
		(*pos)++;
		*result = powl(left, right);
		return 0;
	}

	/* Check for 'r' - reference to last result */
	if (expr[*pos] == 'r' && !isalnum(expr[*pos + 1])) {
		(*pos)++;
		if (lastres.p[0] == '\0') {
			log(ERROR, "no result stored\n");
			return -1;
		}
		*result = strtold(lastres.p, NULL);
		return 0;
	}

	/* Parse number (decimal or hex) */
	char *endptr;
	maxfloat_t val = strtold(&expr[*pos], &endptr);
	if (endptr == &expr[*pos]) {
		log(ERROR, "invalid operand or unit\n");
		return -1;
	}
	*pos = (int)(endptr - expr);
	*result = val;
	return 0;
}

	/* Parse multiplication and division */
static int parse_term(const char *expr, int *pos, maxfloat_t *result)
{
	if (parse_factor(expr, pos, result) == -1)
		return -1;

	while (1) {
		skip_space(expr, pos);
		if (expr[*pos] == '*') {
			(*pos)++;
			maxfloat_t right;
			if (parse_factor(expr, pos, &right) == -1)
				return -1;
			*result = *result * right;
		} else if (expr[*pos] == '/') {
			(*pos)++;
			maxfloat_t right;
			if (parse_factor(expr, pos, &right) == -1)
				return -1;
			if (right == 0) {
				log(ERROR, "division by zero\n");
				return -1;
			}
			*result = *result / right;
		} else {
			break;
		}
	}

	return 0;
}

/* Parse addition and subtraction */
static int parse_expr(const char *expr, int *pos, maxfloat_t *result)
{
	if (parse_term(expr, pos, result) == -1)
		return -1;

	while (1) {
		skip_space(expr, pos);
		if (expr[*pos] == '+') {
			(*pos)++;
			maxfloat_t right;
			if (parse_term(expr, pos, &right) == -1)
				return -1;
			*result = *result + right;
		} else if (expr[*pos] == '-') {
			(*pos)++;
			maxfloat_t right;
			if (parse_term(expr, pos, &right) == -1)
				return -1;
			*result = *result - right;
		} else {
			break;
		}
	}

	return 0;
}

/* Evaluate arithmetic expression */
static int eval_expr(char *expr_str, maxfloat_t *result)
{
	int pos = 0;

	if (!expr_str || !*expr_str) {
		log(ERROR, "empty expression\n");
		return -1;
	}

	if (parse_expr(expr_str, &pos, result) == -1)
		return -1;

	skip_space(expr_str, &pos);
	if (expr_str[pos] != '\0') {
		log(ERROR, "unexpected character in expression\n");
		return -1;
	}

	return 0;
}

/* Format long double removing trailing zeros */
static void format_result(maxfloat_t result, char *buf, size_t buflen)
{
	snprintf(buf, buflen, "%.10Lf", result);

	/* Find decimal point */
	char *dot = strchr(buf, '.');
	if (!dot)
		return;

	/* Find last non-zero digit after decimal point */
	char *end = buf + strlen(buf) - 1;
	while (end > dot && *end == '0')
		end--;

	/* If we stopped at the decimal point, remove it too */
	if (end == dot) {
		*dot = '\0';
	} else {
		*(end + 1) = '\0';
	}
}

static int is_integral_result(maxfloat_t value, long long *out)
{
	long double intpart;

	if (modfl(value, &intpart) != 0.0L)
		return 0;

	if (intpart < (long double)LLONG_MIN || intpart > (long double)LLONG_MAX)
		return 0;

	*out = (long long)intpart;
	return 1;
}

/* Evaluate expression and print result */
static int evaluate_expr(char *expr)
{
	if (!expr) {
		if (curexpr)
			expr = curexpr;
		else
			return -1;
	}

	maxfloat_t result;
	if (eval_expr(expr, &result) == 0) {
		long long int_result;
		if (is_integral_result(result, &int_result)) {
			printf("%lld\n", int_result);
			snprintf(lastres.p, UINT_BUF_LEN, "%lld", int_result);
		} else {
			format_result(result, lastres.p, UINT_BUF_LEN);
			printf("%s\n", lastres.p);
		}
		lastres.unit = 0;
		return 0;
	}
	return -1;
}

static void printbin(maxuint_t n)
{
	int count = MAX_BITS - 1;
	int pos = MAX_BITS + (MAX_BITS >> 2) - 1;
	char binstr[MAX_BITS + (MAX_BITS >> 2) + 1] = {0};

	if (!n) {
		printf("0");
		return;
	}

	while (n && count >= 0) {
		binstr[pos] = "01"[n & 1];
		--pos;
		n >>= 1;
		if (n && count && !(count & 7)) {
			binstr[pos] = ' ';
			--pos;
		}
		--count;
	}

	++pos;

	printf("%s", binstr + pos);
}

static void printbin_positions(maxuint_t n)
{
	if (!n) {
		printf("0");
		return;
	}

	printf("\n");

	/* Find the highest bit position */
	int highest_bit = 0;
	maxuint_t temp = n;
	while (temp) {
		highest_bit++;
		temp >>= 1;
	}
	highest_bit--; /* Adjust to 0-based */

	/* Print positions 0-31, 32-63, etc. Always print all positions in each row */
	for (int start_bit = 0; start_bit <= 127; start_bit += 32) {
		int end_bit = start_bit + 31;

		/* Skip rows where no bits exist in the value */
		if (start_bit > highest_bit)
			break;

		/* Print bit positions for this row (MSB to LSB) */
		for (int bit = end_bit; bit >= start_bit; --bit) {
			maxuint_t bit_value = (bit <= highest_bit) ? ((n >> bit) & 1) : 0;
			if (bit_value == 1)
				printf("\033[7m%3d\033[0m ", bit);
			else
				printf("%3d ", bit);
		}
		printf("\n");

		/* Print bit values for this row (MSB to LSB) - only if bit exists in value */
		for (int bit = end_bit; bit >= start_bit; --bit) {
			if (bit <= highest_bit) {
				int bit_value = (int)(n >> bit) & 1;
				printf("  %d ", bit_value);
			} else
				printf("    ");  /* Leave blank for bits beyond the value */
		}
		printf("\n\n");
	}
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
	if (val - (maxuint_t)val == 0) // NOLINT
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
			return true;
		}
	} else if (base == 16) {
		if (ch >= '0' && ch <= '9') {
			*val = ch - '0';
			return true;
		}

		if (ch >= 'a' && ch <= 'f') {
			*val = (ch - 'a') + 10;
			return true;
		}

		if (ch >= 'A' && ch <= 'F') {
			*val = (ch - 'A') + 10;
			return true;
		}
	} else if (base == 10) {
		if (ch >= '0' && ch <= '9') {
			*val = ch - '0';
			return true;
		}
	}

	return false;
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

static bool parse_prefixed_uint(const char *token, maxuint_t *val, char **endptr)
{
	if (!token || !*token || !val || !endptr)
		return false;

	uint base = 10, multiplier = 0, digit = 0, bits_used = 0;
	uint max_bit_len = sizeof(maxuint_t) << 3;

	if (token[0] == '0') {
		if (token[1] == 'b' || token[1] == 'B') {
			base = 2;
			multiplier = 1;
		} else if (token[1] == 'x' || token[1] == 'X') {
			base = 16;
			multiplier = 4;
		}
	}

	if (base != 2 && base != 16)
		return false;

	const char *ptr = token + 2;
	if (!*ptr) {
		*endptr = (char *)ptr;
		return false;
	}

	maxuint_t res = 0;
	bool seen_digit = false;
	while (*ptr && ischarvalid(*ptr, base, &digit)) {
		seen_digit = true;
		if (bits_used == max_bit_len)
			return false;
		res = (res << multiplier) + digit;
		++bits_used;
		++ptr;
	}

	if (!seen_digit)
		return false;

	*val = res;
	*endptr = (char *)ptr;
	return true;
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
	}

	*ret = 0;

	if (cfg.minimal) {
		printf("%s B\n", getstr_u128(bytes, uint_buf));
		return bytes;
	}

	printf("%40s B\n", getstr_u128(bytes, uint_buf));

	/* Convert and print in IEC standard units */

	printf("\n            IEC standard (base 2)\n\n");
	val = (maxfloat_t)bytes / 1024;
	printval(val, "KiB");

	val = (maxfloat_t)bytes / (1 << 20);
	printval(val, "MiB");

	val = (maxfloat_t)bytes / (1 << 30);
	printval(val, "GiB");

	val = (maxfloat_t)bytes / ((unsigned long long)1 << 40);
	printval(val, "TiB");

	/* Convert and print in SI standard values */

	printf("\n            SI standard (base 10)\n\n");
	val = (maxfloat_t)bytes / 1000;
	printval(val, "kB");

	val = (maxfloat_t)bytes / 1000000;
	printval(val, "MB");

	val = (maxfloat_t)bytes / 1000000000;
	printval(val, "GB");

	val = (maxfloat_t)bytes / 1000000000000;
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(kib * 1024);

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(mib * (1 << 20));

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(gib * (1 << 30));

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(tib * ((maxuint_t)1 << 40));

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(kb * 1000);

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(mb * 1000000);

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (maxuint_t)(gb * 1000000000);

	if (cfg.minimal) {
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
	}

	*ret = 0;

	maxuint_t bytes = (__uint128_t)(tb * 1000000000000);

	if (cfg.minimal) {
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
		return false;
	}

	if (!param[3]) {
		log(ERROR, "MAX_HEAD = 0\n");
		return false;
	}

	if (!param[4]) {
		log(ERROR, "MAX_SECTOR = 0\n");
		return false;
	}

	if (!param[2]) {
		log(ERROR, "S = 0\n");
		return false;
	}

	if (param[1] > param[3]) {
		log(ERROR, "H > MAX_HEAD\n");
		return false;
	}

	if (param[2] > param[4]) {
		log(ERROR, "S > MAX_SECTOR\n");
		return false;
	}

	*lba = (maxuint_t)param[3] * param[4] * param[0]; /* MH * MS * C */
	*lba += (maxuint_t)param[4] * param[1]; /* MS * H */

	*lba += param[2] - 1; /* S - 1 */

	printf("\033[1mCHS2LBA\033[0m\n");
	printf("  C:%lu  H:%lu  S:%lu  MAX_HEAD:%lu  MAX_SECTOR:%lu\n",
		param[0], param[1], param[2], param[3], param[4]);

	return true;
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
		return false;
	}

	if (!param[1]) {
		log(ERROR, "MAX_HEAD = 0\n");
		return false;
	}

	if (!param[2]) {
		log(ERROR, "MAX_SECTOR = 0\n");
		return false;
	}

	/* L / (MS * MH) */
	p_chs->c = (ulong)(param[0] / (param[2] * param[1]));
	/* (L / MS) % MH */
	p_chs->h = (ulong)((param[0] / param[2]) % param[1]);
	if (p_chs->h > MAX_HEAD) {
		log(ERROR, "H > MAX_HEAD\n");
		return false;
	}

	/* (L % MS) + 1 */
	p_chs->s = (ulong)((param[0] % param[2]) + 1);
	if (p_chs->s > MAX_SECTOR) {
		log(ERROR, "S > MAX_SECTOR\n");
		return false;
	}

	printf("\033[1mLBA2CHS\033[0m\n  LBA:%s  ",
		getstr_u128(param[0], uint_buf));
	printf("MAX_HEAD:%s  ", getstr_u128(param[1], uint_buf));
	printf("MAX_SECTOR:%s\n", getstr_u128(param[2], uint_buf));

	return true;
}

static void show_basic_sizes()
{
	printf("---------------\ntype       size\n---------------\n"
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

static void prompt_help()
{
	printf("prompt keys:\n\
 b          toggle general-purpose mode\n\
 c N        convert N to binary, decimal, hex\n\
 p N        print N as bit position/value pairs\n\
 r          result from last operation\n\
 s          sizes of storage types\n\
 ?          help\n\
 q/double ↵ quit\n");
}

static void usage()
{
	printf("usage: bcal [-b [expr]] [-c N] [-p N] [-f loc]\n\
	    [-s bytes] [expr] [N [unit]] [-m] [-d] [-h]\n\n\
Bits, bytes and general-purpose calculator.\n\n\
positional arguments:\n\
 expr       expression in decimal/hex operands\n\
 N [unit]   capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB\n\
            https://en.wikipedia.org/wiki/Binary_prefix\n\
            default unit is B (byte), case is ignored\n\
            N can be decimal or '0x' prefixed hex value\n\n\
optional arguments:\n\
 -b [expr]  start in general-purpose REPL mode\n\
            or, evaluate expression and quit\n\
 -c N       convert N to binary, decimal, hex\n\
 -p N       print N as bit position/value pairs\n\
 -f loc     convert CHS to LBA or LBA to CHS\n\
            refer to the operational notes in man page\n\
 -s bytes   sector size [default 512]\n\
 -m         minimal output (e.g. decimal bytes)\n\
 -d         enable debug information and logs\n\
 -h         show this help\n\n");

	prompt_help();

	printf("\nVersion %s\n\
Copyright © 2016 Arun Prakash Jana <engineerarun@gmail.com>\n\
License: GPLv3\n\
Webpage: https://github.com/jarun/bcal\n", VERSION);
}

static int bstricmp(const char *s1, const char *s2)
{
	while ((int)*s1 && (tolower((int)*s1) == tolower((int)*s2))) {
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
	char *numstr = bunit.p, *punit = NULL;
	int  count;
	maxfloat_t byte_metric = 0;

	if (numstr == NULL || *numstr == '\0') {
		log(ERROR, "invalid token\n");
		*out = -1;
		return 0;
	}

	log(DEBUG, "numstr: %s\n", numstr);
	*out = 0;

	/* ensure this is not the result of a previous operation */
	if (*isunit != 1)
		*isunit = 0;

	if (numstr[0] == '0' &&
	    (numstr[1] == 'x' || numstr[1] == 'X' ||
	     numstr[1] == 'b' || numstr[1] == 'B')) {
		char *pch = NULL;
		maxuint_t val = 0;
		if (!parse_prefixed_uint(numstr, &val, &pch)) {
			log(ERROR, "invalid token\n");
			*out = -1;
			return 0;
		}
		if (*pch == '\0')
			return val;
		if (!isalpha((unsigned char)*pch)) {
			log(ERROR, "invalid token\n");
			*out = -1;
			return 0;
		}
		byte_metric = (maxfloat_t)val;
		punit = pch;
		goto parse_unit;
	}

	byte_metric = strtold(numstr, &punit);
	log(DEBUG, "byte_metric: %Lf\n", byte_metric);
	if (*numstr != '\0' && *punit == '\0')
		return (maxuint_t)byte_metric;

parse_unit:
	log(DEBUG, "punit: %s\n", punit);

	count = ARRAY_SIZE(units);
	while (--count >= 0)
		if (!bstricmp(units[count], punit))
			break;

	if (count == -1) {
		if (cfg.minimal)
			log(ERROR, "unknown unit\n");
		else
			evaluate_expr(NULL);

		*out = -1;
		return 0;
	}

	*isunit = 1;

	switch (count) {
	case 0:
		return (maxuint_t)byte_metric;
	case 1: /* Kibibyte */
		return (maxuint_t)(byte_metric * 1024);
	case 2: /* Mebibyte */
		return (maxuint_t)(byte_metric * (1 << 20));
	case 3: /* Gibibyte */
		return (maxuint_t)(byte_metric * (1 << 30));
	case 4: /* Tebibyte */
		return (maxuint_t)(byte_metric * ((maxuint_t)1 << 40));
	case 5: /* Kilobyte */
		return (maxuint_t)(byte_metric * 1000);
	case 6: /* Megabyte */
		return (maxuint_t)(byte_metric * 1000000);
	case 7: /* Gigabyte */
		return (maxuint_t)(byte_metric * 1000000000);
	case 8: /* Terabyte */
		return (maxuint_t)(byte_metric * 1000000000000);
	default:
		log(ERROR, "unknown unit\n");
		*out = -1;
		return 0;
	}
}

/* Get the priority of operators.
 * Higher priority, higher value.
 */
static int priority(char sign) /* Get the priority of operators, higher priprity */
{
	switch (sign) {
	case '|': return 1;
	case '^': return 2;
	case '&': return 3;
	case '>':
	case '<': return 4;
	case '-':
	case '+': return 5;
	case '%':
	case '/':
	case '*': return 6;
	case '~': return 7;
	default : return 0;
	}

	return 0;
}

/* Convert Infix mathematical expression to Postfix */
static int infix2postfix(char *exp, queue **resf, queue **resr)
{
	stack *op = NULL;  /* Operator Stack */
	char *token = strtok(exp, " ");
	static Data tokenData, ct;
	int balanced = 0;
	bool tokenize = true;

	tokenData.p[0] = '\0';
	tokenData.unit = 0;

	log(DEBUG, "exp: %s\n", exp);
	log(DEBUG, "token: %s\n", token);

	while (token) {
		/* Copy argument to string part of the structure */
		bstrlcpy(tokenData.p, token, NUM_LEN);

		switch (token[0]) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '>':
		case '<':
		case '&':
		case '|':
		case '^':
		case '~':
			if (token[1] != '\0') {
				log(ERROR, "invalid token terminator\n");
				emptystack(&op);
				cleanqueue(resf);
				return -1;
			}

			while (!isempty(op) && top(op)[0] != '(' &&
				       ((token[0] == '~' && priority(token[0]) < priority(top(op)[0])) ||
				        (token[0] != '~' && priority(token[0]) <= priority(top(op)[0])))) {
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
			/*
			 * Check if unit is specified
			 * This also guards against a case of 0xn b
			 */
			token = strtok(NULL, " ");
			if (token && token[0] == 'b' && token[1] == '\0') {
				tokenData.unit = 1;
				log(DEBUG, "unit found\n");
			} else
				tokenize = false; /* We already toknized here */

			/* Enqueue operands */
			log(DEBUG, "tokenData: %s %d\n", tokenData.p, tokenData.unit);
			enqueue(resf, resr, tokenData);
			if (tokenize)
				tokenData.unit = 0;
		}

		if (tokenize)
			token = strtok(NULL, " ");
		else
			tokenize = true;

		log(DEBUG, "token: %s\n", token);
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

		if (cfg.loglvl == DEBUG) {
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
		if (strlen(arg.p) == 1 && !isdigit((int)arg.p[0])) {
			if (arg.p[0] == '~') {
				pop(&est, &raw_a);

				a = unitconv(raw_a, &raw_a.unit, out);
				if (*out == -1)
					return 0;

				c = ~a;
				raw_c.unit = raw_a.unit ? 1 : 0;
				bstrlcpy(raw_c.p, getstr_u128(c, uint_buf), NUM_LEN);
				push(&est, raw_c);
				continue;
			}

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
			case '<':
				if (raw_b.unit) {
					log(ERROR, "unit mismatch in %c%c\n", arg.p[0], arg.p[0]);
					goto error;
				}

				if (arg.p[0] == '>')
					c = a >> b;
				else
					c = a << b;
				raw_c.unit = raw_a.unit;
				break;
			case '+':
			case '&':
			case '|':
			case '^':
				/* Check if the units match */
				if (raw_a.unit == raw_b.unit) {
					switch (arg.p[0]) {
					case '+':
						c = a + b;
						break;
					case '&':
						c = a & b;
						break;
					case '|':
						c = a | b;
						break;
					case '^':
						c = a ^ b;
						break;
					default:
						break;
					}

					if (raw_a.unit)
						raw_c.unit = 1;
					break;
				}

				log(ERROR, "unit mismatch in %c\n", arg.p[0]);
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

					validate_div(a, b, c);
					break;
				}

				if (!raw_b.unit) {
					c = a / b;
					if (raw_a.unit)
						raw_c.unit = 1;

					validate_div(a, b, c);
					break;
				}

				log(ERROR, "unit mismatch in /\n");
				goto error;
			case '%':
				if (b == 0) {
					log(ERROR, "division by 0\n");
					goto error;
				}

				if (!(raw_a.unit || raw_b.unit)) {
					c = a % b;
					break;
				}

				log(ERROR, "unit mismatch in modulo\n"); // fallthrough
			default:
				goto error;
			}

			/* Convert to string */
			bstrlcpy(raw_c.p, getstr_u128(c, uint_buf), NUM_LEN);
			log(DEBUG, "c: %s unit: %d\n", raw_c.p, raw_c.unit);

			/* Push to stack */
			push(&est, raw_c);

		} else {
			log(DEBUG, "pushing (%s %d)\n", arg.p, arg.unit);
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
	char *pch = NULL;
	maxuint_t val = strtouquad(res.p, &pch);
	if (*pch) {
		*out = -1;
		return 0;
	}
	return val;

error:
	*out = -1;
	emptystack(&est);
	cleanqueue(front);
	return 0;
}

static bool has_bitwise_ops(const char *expr)
{
	if (!expr)
		return false;

	for (size_t i = 0; expr[i] != '\0'; ++i) {
		switch (expr[i]) {
		case '&':
		case '|':
		case '^':
		case '~':
			return true;
		case '<':
			if (expr[i + 1] == '<')
				return true;
			break;
		case '>':
			if (expr[i + 1] == '>')
				return true;
			break;
		default:
			break;
		}
	}

	return false;
}

static bool has_units(const char *expr)
{
	if (!expr)
		return false;

	for (size_t i = 0; i < ARRAY_SIZE(units); ++i) {
		/* Check if unit keyword exists in expression
		 * Units must not be followed by alphanumeric characters */
		size_t unit_len = strlen(units[i]);
		const char *pos = expr;
		while ((pos = strstr(pos, units[i])) != NULL) {
			char after = *(pos + unit_len);
			/* Unit found if it's not followed by alphanumeric */
			if (!isalnum((unsigned char)after)) {
				return true;
			}
			pos++;
		}
	}

	return false;
}

static int eval_bitwise_expr(char *expr, char *out, size_t out_len)
{
	if (!expr || !out || out_len == 0)
		return -1;

	int unitless = 0;
	queue *front = NULL, *rear = NULL;
	char *parsed = fixexpr(expr, &unitless);
	if (!parsed)
		return -1;

	int ret = infix2postfix(parsed, &front, &rear);
	free(parsed);
	if (ret == -1)
		return -1;

	int eval_ret = 0;
	maxuint_t value = eval(&front, &rear, &eval_ret);
	if (eval_ret == -1)
		return -1;

	/* Print result based on minimal mode setting */
	if (cfg.minimal) {
		printf("%s\n", getstr_u128(value, uint_buf));
	} else {
		/* Print result in binary, decimal, and hex formats */
		printf(" (b) ");
		printbin(value);
		printf("\n (d) %s\n (h) ",
			getstr_u128(value, uint_buf));
		printhex_u128(value);
		printf("\n");
	}

	/* Store decimal value for lastres */
	bstrlcpy(out, getstr_u128(value, uint_buf), out_len);
	return 0;
}

static int issign(char c)
{
	switch (c) {
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case '>':
	case '<':
	case '&':
	case '|':
	case '^':
	case '~':
		return 1;
	default:
		return 0;
	}
}

/* Check if a char is operator or not */
static int isoperator(int c)
{
	switch (c) {
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case '>':
	case '<':
	case '&':
	case '|':
	case '^':
	case '~':
	case '(':
	case ')': return 1;
	default: return 0;
	}
}

/* Check if valid storage arithmetic expression */
/*
static int checkexp(char *exp)
{
	while (*exp) {
		if (*exp == 'b' || *exp == 'B')
			return 1;

		++exp;
	}

	return 0;
}
*/

/* Trim ending newline and whitespace from both ends, in place */
static void strstrip(char *s)
{
	if (!s || !*s)
		return;

	int len = (int)strlen(s) - 1;

	if (s[len] == '\n')
		--len;
	while (len >= 0 && (isspace((int)s[len]) || s[len] == '\"' || s[len] == '\''))
		--len;
	s[len + 1] = '\0';

	len = 0;
	while (s[len] && (isspace((int)s[len]) || s[len] == '\"' || s[len] == '\''))
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
		/* We should not combine 0xn b*/
		if (!isspace((int)*s) || (*(s + 1) == 'b')) {
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

	/*
	if (!checkexp(exp)) {
		log(DEBUG, "no unit in expression [%s]\n", exp);
		*unitless = 1;
		return NULL;
	}
	*/

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

		if (isoperator((int)exp[i]) && isalpha((int)exp[i + 1]) && (exp[i + 1] != 'r')) {
			log(ERROR, "invalid expression\n");
			free(parsed);
			return NULL;
		}

		if ((isdigit((int)exp[i]) && isoperator((int)exp[i + 1])) ||
		    (isoperator((int)exp[i]) && (isdigit((int)exp[i + 1]) ||
		     isoperator((int)exp[i + 1]))) ||
		    (isalpha((int)exp[i]) && isoperator((int)exp[i + 1])) ||
		    (isoperator((int)exp[i]) && ((int)exp[i + 1] == 'r'))) {
			if (exp[i] == '<' || exp[i] == '>') { /* handle shift operators << and >> */
				if (prev != exp[i] && exp[i] != exp[i + 1]) {
					log(ERROR, "invalid operator %c\n", exp[i]);
					*unitless = 0;
					free(parsed);
					return NULL;
				}

				if (prev == exp[i + 1]) { /* handle <<< or >>> */
					log(ERROR, "invalid sequence %c%c%c\n", prev, exp[i], exp[i + 1]);
					*unitless = 0;
					free(parsed);
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
		int unitchars = 0, len = (int)strlen(value);

		while (len) {
			if (!isalpha((int)value[len - 1]))
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

	if (!cfg.minimal && unit)
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
		if (cfg.minimal || unit) /* For running python test cases */
			log(ERROR, "malformed input\n");
		else
			return evaluate_expr(NULL);

		return -1;
	}

	bstrlcpy(lastres.p, getstr_u128(bytes, uint_buf), UINT_BUF_LEN);
	lastres.unit = 1;
	log(DEBUG, "result: %s %d\n", lastres.p, lastres.unit);

	if (cfg.minimal)
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

	if (expr)
		log(DEBUG, "expr: %s\n", expr);

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

	if (ret == 1) {
		ptr = getstr_u128(bytes, uint_buf);
		printf("%s\n", ptr);
		bstrlcpy(lastres.p, getstr_u128(bytes, uint_buf), UINT_BUF_LEN);
		lastres.unit = 0;
		log(DEBUG, "result1: %s %d\n", lastres.p, lastres.unit);
		return 0;
	}

	if (!(cfg.minimal || cfg.repl))
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

	if (cfg.minimal)
		return 0;

	printf("\nADDRESS\n (d) %s\n (h) ", ptr);
	printhex_u128(bytes);
	printf("\n");

	return 0;
}

int convertbase(char *arg, bool bitposition)
{
	char *pch;

	strstrip(arg);

	if (*arg == '\0') {
		log(ERROR, "empty input\n");
		return -1;
	}

	if (*arg == '-') {
		log(ERROR, "N must be >= 0\n");
		return -1;
	}

	if (cfg.repl && arg[0] == 'r' && arg[1] == '\0')
		arg = lastres.p;

	maxuint_t val = strtouquad(arg, &pch);
	if (*pch) {
		log(ERROR, "invalid input\n");
		return -1;
	}

	if (bitposition)
		printbin_positions(val);
	else {
		printf(" (b) ");
		printbin(val);
		printf("\n (d) %s\n (h) ",
			getstr_u128(val, uint_buf));
		printhex_u128(val);
		printf("\n");
	}

	return 0;
}

int main(int argc, char **argv)
{
	int opt = 0, operation = 0;
	ulong sectorsz = SECTOR_SIZE;

	opterr = 0;
#ifndef NORL
	rl_bind_key('\t', rl_insert);
#endif

	while ((opt = getopt(argc, argv, "bc:df:hmp:s:")) != -1) {
		switch (opt) {
		case 'c':
			operation = 1;
			convertbase(optarg, false);
			printf("\n");
			break;
		case 'f':
			operation = 1;

			if (tolower((int)*optarg) == 'c') {
				maxuint_t lba = 0;

				if (chs2lba(optarg + 1, &lba)) {
					printf("  LBA: (d) %s, (h) ",
						getstr_u128(lba, uint_buf));
					printhex_u128(lba);
					printf("\n\n");
				}
			} else if (tolower((int)*optarg) == 'l') {
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
			cfg.minimal = 1;
			break;
		case 's':
			if (*optarg == '-') {
				log(ERROR, "sector size must be +ve\n");
				return -1;
			}
			sectorsz = strtoul_b(optarg);
			break;
		case 'b':
			cfg.maths = 1;
			strncpy(prompt, PROMPT_MATHS, 8);
			break;
		case 'd':
			cfg.loglvl = DEBUG;
			log(DEBUG, "bcal v%s\n", VERSION);
			log(DEBUG, "maxuint_t is %lu bytes\n", sizeof(maxuint_t));
			break;
		case 'p':
			operation = 1;
			convertbase(optarg, true);
			printf("\n");
			break;
		case 'h':
			usage();
			return 0;
		default:
			log(ERROR, "invalid option \'%c\'\n\n", (char)optopt);
			usage();
			return -1;
		}
	}

	log(DEBUG, "argc %d, optind %d\n", argc, optind);

	if (!operation && (argc == optind)) {
		char *ptr = NULL, *tmp = NULL;
		cfg.repl = 1;
		int enters = 0;
		int is_tty = isatty(STDIN_FILENO);

		read_history(NULL);

		while (1) {
			/* Manually print prompt for non-TTY mode (e.g., tests with pipes) */
			if (!is_tty) {
				printf("%s", prompt);
				fflush(stdout);
			}
			tmp = readline(prompt);
			if (!tmp)
				break;

			if (program_exit(tmp)) {
				free(tmp);
				exit(0);
			}

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

			add_history(tmp);

			if (cfg.maths) {
				if (has_function_call(tmp))
					remove_thousands_commas(tmp);
				else
					remove_commas(tmp);
			} else
				remove_commas(tmp);

			log(DEBUG, "ptr: [%s]\n", ptr);
			log(DEBUG, "tmp: [%s]\n", ptr);

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
					cfg.maths ^= 1;
					strncpy(prompt, cfg.maths ? PROMPT_MATHS : PROMPT_BYTES, 8);
					free(ptr);
					continue;
				case 'q':
					free(ptr);
					write_history(NULL);
					return 0;
				case 's':
					show_basic_sizes();
					free(ptr);
					continue;
				case '?':
					prompt_help();
					free(ptr);
					continue;
				default:
					printf("invalid input\n");
					free(ptr);
					continue;
				}
			}

			/* Handle 'c' and 'p' switches in both storage and expression modes */
			if (tmp[0] == 'c' && !isalpha(tmp[1])) {
				convertbase(tmp + 1, false);
				free(ptr);
				continue;
			}

			if (tmp[0] == 'p' && !isalpha(tmp[1])) {
				convertbase(tmp + 1, true);
				free(ptr);
				continue;
			}

			if (cfg.maths) {
				if (has_bitwise_ops(tmp)) {
					if (eval_bitwise_expr(tmp, lastres.p, UINT_BUF_LEN) == 0) {

						continue;
					}
					free(ptr);
					continue;
				}

				if (eval_decimal_multiply(tmp, lastres.p, UINT_BUF_LEN)) {
					printf("%s\n", lastres.p);
					lastres.unit = 0;
					free(ptr);
					continue;
				}

				maxfloat_t result;
				if (eval_expr(tmp, &result) == 0) {
					long long int_result;
					if (is_integral_result(result, &int_result)) {
						printf("%lld\n", int_result);
						snprintf(lastres.p, UINT_BUF_LEN, "%lld", int_result);
					} else {
						format_result(result, lastres.p, UINT_BUF_LEN);
						printf("%s\n", lastres.p);
					}
					/* Store result for next use */
					lastres.unit = 0;
				}
				free(ptr);
				continue;
			}

			/* Check for bitwise operations first */
			if (has_bitwise_ops(tmp)) {
				if (eval_bitwise_expr(tmp, lastres.p, UINT_BUF_LEN) == 0) {
					free(ptr);
					continue;
				}
			}

			curexpr = tmp;

			/* Evaluate the expression */
			evaluate(tmp, sectorsz);

			free(ptr);
		}

		write_history(NULL);
		return 0;
	}

	/* Unit conversion */
	if (argc - optind == 2)
		if (convertunit(argv[optind], argv[optind + 1], sectorsz) == -1)
			return -1;

	/*Arithmetic operation*/
	if (argc - optind == 1) {
		char *tmp = strdup(argv[optind]);
		if (!tmp)
			return -1;
		strstrip(tmp);
		if (cfg.maths) {
			if (has_function_call(tmp))
				remove_thousands_commas(tmp);
			else
				remove_commas(tmp);
		}

		/* Check for bitwise operations first, but only if no units are present */
		if (has_bitwise_ops(tmp) && !has_units(tmp)) {
			int bitwise_ret = eval_bitwise_expr(tmp, lastres.p, UINT_BUF_LEN);
			free(tmp);
			return bitwise_ret;
		}

		if (cfg.maths) {
			if (eval_decimal_multiply(tmp, lastres.p, UINT_BUF_LEN)) {
				printf("%s\n", lastres.p);
				lastres.unit = 0;
				free(tmp);
				return 0;
			}

			maxfloat_t result;
			if (eval_expr(tmp, &result) == 0) {
				long long int_result;
				if (is_integral_result(result, &int_result)) {
					printf("%lld\n", int_result);
					snprintf(lastres.p, UINT_BUF_LEN, "%lld", int_result);
				} else {
					format_result(result, lastres.p, UINT_BUF_LEN);
					printf("%s\n", lastres.p);
				}
				/* Store result for next use */
				lastres.unit = 0;
				free(tmp);
				return 0;
			}
			free(tmp);
			return -1;
		}

		curexpr = argv[optind];
		free(tmp);
		return evaluate(argv[optind], sectorsz);
	}

	return -1;
}
