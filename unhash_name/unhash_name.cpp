// unhash_name.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _CRC_SECURE_NO_WARNINGS
//#define BENCHMARK

// PREFERENCE: reverses the order of most-significant letter when scanning all variations of character set.
//  charset = "ABC"
//  0: AAAA -> BAAA -> CAAA -> ABAA -> BBAA -> CBAA -> ACAA ...
//  1: AAAA -> AAAB -> AAAC -> AABA -> AABB -> AABC -> AACA ...
#define REVERSE_SCAN 0

// OPTIMIZATION: once a collision is found, dump current first/last 4 bytes and move onto changing 5th byte.
// reasoning for this is that only ONE combination of 4 bytes can ever change one accum value A, to another B.
// even if we don't know what the intermediate value is (when in forward scan), the fact that all remaining
// bytes are the same, let's us know that these first 4 bytes can no longer produce a target result with
// the remaining pattern.
//  charset = "ABC"
//  [ACBA]AA -> [AAAA]BA (forward)
//  AA[ABCA] -> AB[AAAA] (reverse)
#define COLLISION_DUMP_4 1

#include <zlib.h>    // crc32
#include <stdlib.h>
#include <string.h>  // str*
#include <stdio.h>   // printf


////////////////////////////////////////////////////////////
#pragma region INVERSE CRC32

static unsigned long CRC32_TABLE[256] = { 0 };
static unsigned long CRC32_INDICES[256] = { 0 };

void inverse_init() {
	if (CRC32_TABLE[0] != 0) // already initialized
		return;

	const unsigned long POLY = 0xEDB88320UL;
	for (unsigned long n = 0; n < 256; n++) {
		unsigned long c = n;
		for (int k = 0; k < 8; k++) {
			if ((c & 0x1UL) != 0UL)
				c = (c >> 1) ^ POLY;
			else
				c >>= 1;
		}
		CRC32_TABLE[n] = c;
		// most-significant byte of accum value is identical to that of last table entry
		CRC32_INDICES[c >> 24] = n;
	}
}
// use same parameter values as zlib for consistency
// usage is identical to zlib crc32(),
//  (though init should be fed the target accumulator instead of 0, and buf is read in reverse)
uLong inverse_crc32(uLong crc, const Bytef* buf, uInt len) {
	inverse_init();

	crc ^= 0xffffffffUL;  // xorout
	// order is fed in reverse to back-out accum value after len bytes
	for (int i = (int)len - 1; i >= 0; i--) {
		unsigned long x = CRC32_INDICES[crc >> 24]; // find index x of MSByte (where CRC_TABLE[x] == XX......)
		unsigned long y = CRC32_TABLE[x]; // find last used crc table value
		crc = ((crc ^ y) << 8) | (buf[i] ^ x);
	}
	return crc ^ 0xffffffffUL;  // xorout or init??
}

#pragma endregion

////////////////////////////////////////////////////////////
#pragma region BENCHMARKING (for template and stack-local hell)

#ifdef BENCHMARK

//<https://stackoverflow.com/a/22387757/7517185>
#include <chrono>
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

// benchmarking time point variables
static high_resolution_clock::time_point t1_, t2_;

#define benchmark_start() \
	t1_ = t2_ = high_resolution_clock::now()
#define benchmark_stop() \
	t2_ = high_resolution_clock::now()
// add difference since timing was paused
#define benchmark_resume() \
	t1_ += high_resolution_clock::now() - t2_

// benchmark_stop() must be called beforehand
void benchmark_log() {
	/* Getting number of milliseconds as an integer. */
	//auto ms_int_ = duration_cast<milliseconds>(t2_ - t1_);
	/* Getting number of milliseconds as a double. */
	duration<double, std::milli> ms_double_ = t2_ - t1_;
	
	printf("%14.3fms\n", ms_double_.count());
}

static int benchmark_printf_count_ = 0;

//<https://stackoverflow.com/a/5765197/7517185>
// strip out normal printf while benchmarking
// increment to force contained if statements to do something
#define printf(fmt, ...) benchmark_printf_count_++;
//#define printf(fmt, ...) (0)
#define benchmark_printf(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)

#else

// empty defines

#define benchmark_start()
#define benchmark_stop()
#define benchmark_resume()
#define benchmark_log()
#define benchmark_printf(fmt, ...)

#endif

#pragma endregion

////////////////////////////////////////////////////////////
#pragma region TEMPLATE HELL

///NOTE: Currently template functions are not used. The benchmarks gave
///      little indicator of it helping... or I'm doing something wrong.

// single template case statement call to unhash_fixed_len<len, charset_len>
#define case_unhash(LEN, CHARSET_LEN) \
	case CHARSET_LEN: unhash_fixed<LEN, CHARSET_LEN>(accum, init, prefix, postfix, charset); \
	break;

// case_unhash(LEN, CHLEN:CHLEN+0x10)
#define case_unhash_charsetlen_range(LEN, CHLEN) \
	case_unhash(LEN, CHLEN+0x0) \
	case_unhash(LEN, CHLEN+0x1) \
	case_unhash(LEN, CHLEN+0x2) \
	case_unhash(LEN, CHLEN+0x3) \
	case_unhash(LEN, CHLEN+0x4) \
	case_unhash(LEN, CHLEN+0x5) \
	case_unhash(LEN, CHLEN+0x6) \
	case_unhash(LEN, CHLEN+0x7) \
	case_unhash(LEN, CHLEN+0x8) \
	case_unhash(LEN, CHLEN+0x9) \
	case_unhash(LEN, CHLEN+0xa) \
	case_unhash(LEN, CHLEN+0xb) \
	case_unhash(LEN, CHLEN+0xc) \
	case_unhash(LEN, CHLEN+0xd) \
	case_unhash(LEN, CHLEN+0xe) \
	case_unhash(LEN, CHLEN+0xf)

// case for LEN, and switch statement for all supported character lengths
#define choose_unhash_charsetlen(LEN) \
	case LEN: {switch (charset_len) { \
	case_unhash_charsetlen_range(LEN, 0x01) /* 0x01-0x10*/ \
	case_unhash_charsetlen_range(LEN, 0x11) /* 0x11-0x20*/ \
	case_unhash_charsetlen_range(LEN, 0x21) /* 0x21-0x30*/ \
	case_unhash_charsetlen_range(LEN, 0x31) /* 0x31-0x40*/ \
	case_unhash_charsetlen_range(LEN, 0x41) /* 0x41-0x50*/ \
	case_unhash_charsetlen_range(LEN, 0x51) /* 0x51-0x60*/ \
	default: \
		unhash_variable(len, accum, init, prefix, postfix, charset, charset_len); \
		break; \
	} break;}


// large and unruly nested switch statements with
// templated constant pattern lengths and charset lengths
// ...for optimization...maybe
//
// expects variables:  len, charset_len, accum, init, prefix, postfix, Charset
// depending on charset length, defining more pattern lengths may be required
#define choose_unhash_len() \
	switch (len) { \
	choose_unhash_charsetlen(1) \
	choose_unhash_charsetlen(2) \
	choose_unhash_charsetlen(3) \
	choose_unhash_charsetlen(4) \
	choose_unhash_charsetlen(5) \
	choose_unhash_charsetlen(6) \
	choose_unhash_charsetlen(7) \
	choose_unhash_charsetlen(8) \
	choose_unhash_charsetlen(9) \
	choose_unhash_charsetlen(10) \
	choose_unhash_charsetlen(11) \
	choose_unhash_charsetlen(12) \
	choose_unhash_charsetlen(13) \
	choose_unhash_charsetlen(14) \
	choose_unhash_charsetlen(15) \
	choose_unhash_charsetlen(16) \
	default: \
		unhash_variable(len, accum, init, prefix, postfix, charset, charset_len); \
		break; \
	}

// single template case statement call to unhash_fixed_len<len, charset_len>
#define case_unhash1(LEN) \
	case LEN: unhash_fixed1<LEN>(accum, init, prefix, postfix, charset, charset_len); \
	break;

// large and unruly nested switch statements with
// templated constant pattern lengths and charset lengths
// ...for optimization...maybe
//
// expects variables:  len, charset_len, accum, init, prefix, postfix, Charset
// depending on charset length, defining more pattern lengths may be required
#define choose_unhash_len_template() \
	switch (len) { \
	case_unhash1(1) \
	case_unhash1(2) \
	case_unhash1(3) \
	case_unhash1(4) \
	case_unhash1(5) \
	case_unhash1(6) \
	case_unhash1(7) \
	case_unhash1(8) \
	case_unhash1(9) \
	case_unhash1(10) \
	case_unhash1(11) \
	case_unhash1(12) \
	case_unhash1(13) \
	case_unhash1(14) \
	case_unhash1(15) \
	case_unhash1(16) \
	default: \
		unhash_variable(len, accum, init, prefix, postfix, charset, charset_len); \
		break; \
	}

#pragma endregion

////////////////////////////////////////////////////////////
#pragma region UNHASH TEMPLATE FUNCTIONS

// templated function for constant compile-time pattern length and charset length.
// ...hopefully this actually has a positive impact on runtime.
template <unsigned int LEN, unsigned int CHARSET_LEN>

void unhash_fixed(unsigned long accum,      // target CRC-32 result
				  unsigned long init,       // initial value fed to crc32(), result of hashing prefix
				  const char* prefix,       // pattern prefix, only needed for printing results
				  const char* postfix,      // pattern postfix, calculated with generated pattern
				  const char* CHARSET)
{
	///NOTE: arbitrary large lengths chosen
	char buffer[0x400];  // buffer fed to crc32()
	unsigned int levels[LEN] = { 0 };  // track charset indexes in the buffer
	unsigned int buffer_len = LEN + (unsigned int)strlen(postfix);

	// setup initial buffer state: "<CHARSET[0]>*LEN" + "<POSTFIX>"
	strcpy(buffer + LEN, postfix);
	for (unsigned int j = 0; j < LEN; j++) {
		levels[j] = 0;
		buffer[j] = CHARSET[0];
	}

	unsigned int m;
	do {
		// check current pattern
		unsigned long crc = crc32(init, (const unsigned char*)buffer, buffer_len);
		if (crc == accum) {
			printf("\"%s%s\"\n", prefix, buffer); // buffer includes postfix
			//break;  // keep going in-case we encounter garbage collisions
		}

		// change to next pattern
		//  charset = "ABC"
		//  AAAA -> BAAA -> CAAA -> ABAA -> BBAA -> CBAA -> ACAA ...
		for (m = 0; m < LEN; m++) {
			unsigned int idx = ++levels[m];
			if (idx != CHARSET_LEN) {
				// letter incremented, break and calculate new pattern
				buffer[m] = CHARSET[idx];
				break;
			}
			else {
				// wrap letter back to CHARSET[0], continue to next index
				levels[m] = 0;
				buffer[m] = CHARSET[0];
			}
		}
		// m = LEN when the for loop has finished without the break;
	} while (m != LEN);
}

// templated function for constant compile-time pattern length and charset length.
// ...hopefully this actually has a positive impact on runtime.
template <unsigned int LEN>

void unhash_fixed1(unsigned long accum,      // target CRC-32 result
				   unsigned long init,       // initial value fed to crc32(), result of hashing prefix
				   const char* prefix,       // pattern prefix, only needed for printing results
				   const char* postfix,      // pattern postfix, calculated with generated pattern
				   const char* CHARSET,
				   unsigned int CHARSET_LEN)
{
	///NOTE: arbitrary large lengths chosen
	char buffer[0x400];  // buffer fed to crc32()
	unsigned int levels[LEN] = { 0 };  // track charset indexes in the buffer
	unsigned int buffer_len = LEN + (unsigned int)strlen(postfix);

	// setup initial buffer state: "<CHARSET[0]>*LEN" + "<POSTFIX>"
	strcpy(buffer + LEN, postfix);
	for (unsigned int j = 0; j < LEN; j++) {
		levels[j] = 0;
		buffer[j] = CHARSET[0];
	}

	unsigned int m;
	do {
		// check current pattern
		unsigned long crc = crc32(init, (const unsigned char*)buffer, buffer_len);
		if (crc == accum) {
			printf("\"%s%s\"\n", prefix, buffer); // buffer includes postfix
			//break;  // keep going in-case we encounter garbage collisions
		}

		// change to next pattern
		//  charset = "ABC"
		//  AAAA -> BAAA -> CAAA -> ABAA -> BBAA -> CBAA -> ACAA ...
		for (m = 0; m < LEN; m++) {
			unsigned int idx = ++levels[m];
			if (idx != CHARSET_LEN) {
				// letter incremented, break and calculate new pattern
				buffer[m] = CHARSET[idx];
				break;
			}
			else {
				// wrap letter back to CHARSET[0], continue to next index
				levels[m] = 0;
				buffer[m] = CHARSET[0];
			}
		}
		// m = LEN when the for loop has finished without the break;
	} while (m != LEN);
}

#pragma endregion

////////////////////////////////////////////////////////////
#pragma region UNHASH RUNTIME FUNCTIONS

// non-templated unhash function for when a pattern or charset length has no matching template function
void unhash_variable(unsigned int LEN,
					 unsigned long accum,      // target CRC-32 result (including postfix in buffer)
					 unsigned long init,       // initial value fed to crc32(), result of hashing prefix
					 unsigned long target,     // target CRC-32 result, result of inverse hashing postfix
					 const char* prefix,       // pattern prefix, only needed for printing results
					 const char* postfix,      // pattern postfix, calculated with generated pattern
					 const char* CHARSET,
					 unsigned int CHARSET_LEN)
{
	///NOTE: arbitrary large lengths chosen
	char buffer[0x400];  // buffer fed to crc32()
	unsigned int levels[0x200] = { 0 };  // track charset indexes in the buffer
	unsigned int buffer_len = LEN;

	// setup initial buffer state: "<CHARSET[0]>*LEN"
	buffer[LEN] = 0; // null-terminate since that's no longer handled by strcpy
	for (unsigned int j = 0; j < LEN; j++) {
		levels[j] = 0;
		buffer[j] = CHARSET[0];
	}

#if REVERSE_SCAN
	int m;
#else
	unsigned int m;
#endif
	do {
		// check current pattern
		unsigned long crc = crc32(init, (const unsigned char*)buffer, buffer_len);
		if (crc == target) {
			printf("\"%s%s%s\"\n", prefix, buffer, postfix); // buffer does NOT include postfix anymore
			// special handling: there is only ONE 4-byte combination to transform one accum A into another B
#if COLLISION_DUMP_4
			if (LEN >= 4) {
				// we can give up the current forward (REVERSED_LOOKUP=0), or last 4 bytes
				// this is only a minor optimization, since 4 ASCII bytes of depth is pretty fast to scan through
				// (when LEN = 4, this behaves the same as a break;)
				// next for loop iteration will push out last 4 chars from any combination:
				//  charset = "ABC"
#if REVERSE_SCAN
				//  AA[ABCA] -> AB[AAAA]
				for (unsigned int i = LEN - 4; i < LEN; i++) {
#else
				//  [ACBA]AA -> [AAAA]BA
				for (unsigned int i = 0; i < 4; i++) {
#endif
					levels[i] = CHARSET_LEN - 1;
				}
			}
#else
			if (LEN == 4)
				break;
#endif
			//break;  // keep going in-case we encounter garbage collisions
		}
		// change to next pattern (reversed)
		//  charset = "ABC"
#if REVERSE_SCAN
		//  AAAA -> AAAB -> AAAC -> AABA -> AABB -> AABC -> AACA ...
		for (m = LEN - 1; m >= 0; m--) {
#else
		//  AAAA -> BAAA -> CAAA -> ABAA -> BBAA -> CBAA -> ACAA ...
		for (m = 0; m < LEN; m++) {
#endif
			unsigned int idx = ++levels[m];
			if (idx != CHARSET_LEN) {
				// letter incremented, break and calculate new pattern
				buffer[m] = CHARSET[idx];
				break;
			}
			else {
				// wrap letter back to CHARSET[0], continue to next index
				levels[m] = 0;
				buffer[m] = CHARSET[0];
			}
		}
#if REVERSE_SCAN
		// m = -1 when the for loop has finished without the break;
	} while (m != -1);
#else
		// m = LEN when the for loop has finished without the break;
	} while (m != LEN);
#endif
}

#pragma endregion

////////////////////////////////////////////////////////////
#pragma region DO_UNHASH

// setup unhashing and call unhash functions for each length
void do_unhash(unsigned long accum,
			   const char* prefix,
			   const char* postfix,
			   unsigned int max_len,
			   unsigned int min_len,
			   const char* charset)
{
	unsigned int charset_len = (unsigned int)strlen(charset);
	printf("charset = \"%s\"\n", charset);

	// pre-calculate crc32(prefix) and use as the initial value from now on
	unsigned long init = crc32(0, (const unsigned char*)prefix, (unsigned int)strlen(prefix));
	unsigned long target = inverse_crc32(accum, (const unsigned char*)postfix, (unsigned int)strlen(postfix));
	printf("accum = 0x%08x, init = 0x%08x, target = 0x%08x\n", (unsigned int)accum, (unsigned int)init, (unsigned int)target);

	if (min_len == 0) {
		min_len = 1;
		printf("depth = %u\n", 0u);
		if (init == target) {
			printf("\"%s%s\"\n", prefix, postfix);
		}
	}

	benchmark_start();
	// check for matches with all remaining pattern lengths
	for (unsigned int len = min_len; len <= max_len; len++) {
		printf("depth = %u\n", len);
		if (len == 0) {
			// special check for match with pattern length 0
			if (init == target) {
				printf("\"%s%s\"\n", prefix, postfix);
			}
			continue;
		}
#ifndef BENCHMARK
		// large and unruly nested switch statements with
		// templated constant pattern lengths and charset lengths
		// ...for optimization...maybe
		//
		// expects variables:  len, charset_len, accum, init, prefix, postfix, charset
		//choose_unhash_len();
		unhash_variable(len, accum, init, target, prefix, postfix, charset, charset_len);
#else
		//choose_unhash_len_template();
		//choose_unhash_len();
		unhash_variable(len, accum, init, target, prefix, postfix, charset, charset_len);
#endif
	}
	benchmark_stop();
	benchmark_printf("unhash_variable()\n");
	benchmark_log();
}

#pragma endregion

////////////////////////////////////////////////////////////
#pragma region ENTRYPOINT

// handle commandline arguments and call do_unhash()
int main(int argc, char *argv[]) {
	// handle help/usage
	if (argc < 2) {
		fprintf(stderr, "usage: unhash.exe <ACCUM> [PRE=] [POST=] [MAX=16] [MIN=0] [CHARSET=a-z_0-9]\n");
		return 1;
	}
	else if (!strcmp(argv[1], "/?") || !stricmp(argv[1], "/h") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printf("usage: unhash.exe <ACCUM> [PRE=] [POST=] [MAX=16] [MIN=0] [CHARSET=a-z_0-9]\n");
		printf("\n");
		printf("arguments:\n");
		printf("  ACCUM    target CRC-32 result to match (accepts hex prefix '0x' and '$')\n");
		printf("  PRE      constant ASCII name prefix\n");
		printf("  POST     constant ASCII name postfix\n");
		printf("  MAX      maximum character length to test\n");
		printf("  MIN      minimum character length to test\n");
		printf("  CHARSET  list of characters for pattern (accepts ranges and '\\' escapes)\n");
		return 0;
	}

	///NOTE: arbitrary large lengths chosen
	unsigned long accum;
	char prefix[0x400];   // arbitrary length
	char postfix[0x400];  // arbitrary length
	int max_len = 16;  // default
	int min_len = 0;   // default
	char charset[0x200];  // arbitrary extra room for error reporting duplicates

	// parse argument: <accum> (accumulator, CRC-32 value)
	if (argv[1][0] == '$')  // support for $ hex prefix
		accum = strtoul(argv[1] + 1, nullptr, 16);
	else  // otherwise parse based on prefix or no prefix
		accum = strtoul(argv[1], nullptr, 0);

	// parse argument: [prefix=]
	if (argc >= 3)
		strcpy(prefix, argv[2]);
	else
		prefix[0] = '\0';

	// parse argument: [postfix=]
	if (argc >= 4)
		strcpy(postfix, argv[3]);
	else
		postfix[0] = '\0';

	// parse argument: [max_len=16]
	if (argc >= 5) {
		max_len = (int)strtol(argv[4], nullptr, 0);
		if (max_len > 0x200) {
			// dumb extra constraint because everything is a local buffer (SPEED BABY! SPEED!)
			fprintf(stderr, "error: max_len %d is greater than 512!\n", max_len);
			return 1;
		} else if (max_len < 0) {
			fprintf(stderr, "error: max_len %d is less than zero!\n", max_len);
			return 1;
		}
	}

	// parse argument: [min_len=0]
	if (argc >= 6) {
		min_len = (int)strtol(argv[5], nullptr, 0);
		if (min_len > max_len) {
			fprintf(stderr, "error: min_len %d is greater than max_len %d!\n", min_len, max_len);
			return 1;
		} else if (min_len < 0) {
			fprintf(stderr, "error: min_len %d is less than zero!\n", min_len);
			return 1;
		}
	}

	// parse argument: [charset=a-z_0-9]
	if (argc < 7) {
		strcpy(charset, "abcdefghijklmnopqrstuvwxyz_0123456789"); // default charset
	} else {
		char* charset_ptr = &charset[0];
		const char* pattern = argv[6];
		int pattern_len = (int)strlen(pattern);
		for (int i = 0; i < pattern_len; i++) {
			char c = pattern[i];
			switch (c) {
			case '\\': // escape literal
				if (i+1 == pattern_len) {
					fprintf(stderr, "error: trailing '\\' escape in charset!\n");
					return 1;
				}
				/*switch (pattern[++i]) {
				case 'n': *charset_ptr++ = '\n'; break;
				case 'r': *charset_ptr++ = '\r'; break;
				case 't': *charset_ptr++ = '\t'; break;
				case 'v': *charset_ptr++ = '\v'; break;
				default: *charset_ptr++ = pattern[i]; break;
				}*/
				*charset_ptr++ = pattern[++i];
				break;
			case '-': { // character range
				if (i == 0) {
					fprintf(stderr, "error: unescaped '-' first character in charset!\n");
					return 1;
				}
				else if (i+1 == pattern_len) {
					fprintf(stderr, "error: unescaped trailing '-' in charset!\n");
					return 1;
				}
				short start = ((unsigned char*)pattern)[i-1];
				short end = ((unsigned char*)pattern)[++i];
				if (end == '\\') {
					if (i+1 == pattern_len) {
						fprintf(stderr, "error: trailing '\\' escape in charset!\n");
						return 1;
					}
					/*switch (pattern[++i]) {
					case 'n': end = '\n'; break;
					case 'r': end = '\r'; break;
					case 't': end = '\t'; break;
					case 'v': end = '\v'; break;
					default: end = ((unsigned char*)pattern)[i]; break;
					}*/
					end = ((unsigned char*)pattern)[++i];
				}
				if (end < start) {
					fprintf(stderr, "error: range [%c-%c], end char is less than start char!\n", (unsigned char)start, (unsigned char)end);
					return 1;
				}
				// add range: start char already added, start at +1
				for (short cc = start + 1; cc <= end; cc++) {
					*((unsigned char*)charset_ptr++) = (unsigned char)cc;
				}
				break; }
			default: // single character
				*charset_ptr++ = c;
				break;
			}
		}
		*charset_ptr = '\0'; // null-terminate

		// error-check resulting charset
		int charset_len = (int)strlen(charset);
		if (!charset_len && max_len) { // ignore when max length is 0, we won't be using this
			fprintf(stderr, "error: charset is empty!\n");
			return 1;
		}

		// string of duplicates to print in error message
		char duplicates[sizeof(charset) / sizeof(char)];
		int count = 0;

		for (int i = 0; i < charset_len; i++) {
			bool first = true;
			if (!charset[i]) // already identifed as duplicate character
				continue;
			for (int j = i+1; j < charset_len; j++) {
				if (charset[i] == charset[j]) {
					charset[j] = '\0'; // identified as duplicate, erase from checks
					if (first) {
						first = false;
						duplicates[count++] = charset[i];
					}
				}
			}
		}
		duplicates[count] = '\0'; // null-terminate
		if (count) {
			fprintf(stderr, "error: found %d duplicate characters in charset!\n", count);
			fprintf(stderr, "duplicates = \"%s\"\n", duplicates);
			return 1;
		}
	}

	do_unhash(accum, prefix, postfix, (unsigned int)max_len, (unsigned int)min_len, charset);
	return 0;
}

#pragma endregion
