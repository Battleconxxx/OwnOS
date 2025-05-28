#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++) {
		if (putchar(bytes[i]) == EOF)
			return false;
	}
	return true;
}

static void itoa(int value, char* buffer) {
	char temp[12];
	int i = 0;
	bool negative = false;

	if (value == -2147483648) { // Handle INT_MIN
		strcpy(buffer, "-2147483648");
		return;
	}

	if (value < 0) {
		negative = true;
		value = -value;
	}

	do {
		temp[i++] = '0' + (value % 10);
		value /= 10;
	} while (value != 0);

	if (negative)
		temp[i++] = '-';

	for (int j = 0; j < i; j++)
		buffer[j] = temp[i - j - 1];
	buffer[i] = '\0';
}

static void itox(unsigned int value, char* buffer) {
	char digits[] = "0123456789abcdef";
	char temp[9];
	int i = 0;

	do {
		temp[i++] = digits[value % 16];
		value /= 16;
	} while (value != 0);

	for (int j = 0; j < i; j++)
		buffer[j] = temp[i - j - 1];
	buffer[i] = '\0';
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) return -1;
			if (!print(format, amount)) return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int);
			if (!maxrem) return -1;
			if (!print(&c, sizeof(c))) return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) return -1;
			if (!print(str, len)) return -1;
			written += len;
		} else if (*format == 'd') {
			format++;
			int value = va_arg(parameters, int);
			char buffer[32];
			itoa(value, buffer);
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else if (*format == 'x') {
			format++;
			unsigned int value = va_arg(parameters, unsigned int);
			char buffer[32];
			itox(value, buffer);
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) return -1;
			if (!print(format, len)) return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
