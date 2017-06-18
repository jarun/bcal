/*
 * A modular, level based log file implementation
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
 * Copyright (C) 2014 by Arun Prakash Jana <engineerarun@gmail.com>
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

#include "log.h"

extern int cur_loglevel;
char *logarr[] = {"ERROR", "INFO", "DEBUG"};

void debug_log(const char *func, int level, const char *format, ...)
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
