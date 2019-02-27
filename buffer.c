#include "utils.h"

//#define N 4096
#define N 4096
#define BUFFER_INPUT_FILENO script_fileno

static char A[N+1];
static char B[N+1];
static char* front;
static char* back;
static char* mark;
static size_t count;

extern unsigned int script_fileno;

static void buff_eof() {
	error("Reached end of file!");
}

static void buff_update() {
	unsigned int n = read(BUFFER_INPUT_FILENO, back, N);
	while (n < N) {
		*(back+n) = '\0';
		n++;
	}
	swap(&front, &back);
}

static int buff_check(char* p) {
	if (back <= p && p <= back+N) return 0;
	if (front <= p && p <= front+N) return 1;
	return -1;
}

char* buff_next() {
	++count;
	++mark;
	if (*mark == '\0') {	
		if (mark == back+N) mark = front;
		else if (mark == front+N) {
			buff_update();
			mark = front;
		}
	}	
	return mark;
}

char* buff_back() {
	--count;
	--mark;
	if (mark == front-1) mark = back+N-1;
	return mark;
}

char* buff_rewind(unsigned int n) {
	char * f = mark; while(n) {
		f = buff_back(); n--;
	} return f;
}

char* buff_copy(char* begin, char* end) {
	char* dest = (char*) malloc(sizeof(char)*buff_distance(begin, end)+1);
	char* adv = dest;
	if (begin == end) {
		adv[0] = '\0'; return dest;
	}

	char* t = mark;
	size_t c = count;
	mark = begin;
	do {
		*adv = *begin;
		begin = buff_next();
		adv++;
	} while (begin!=end && *begin != '\0');
	*adv = '\0';
	mark = t;
	count = c;
	return dest;
}

void buff_print(char* begin, char* end) {
	if (begin == end) return;
	char* t = mark;
	size_t c = count;
	mark = begin;
	do {
		write(STDOUT_FILENO, begin, 1);
		begin = buff_next();
	} while (begin!=end && *begin != '\0');
	mark = t;
	count = c;
}

unsigned int buff_distance(char* begin, char* end) {
	if (begin == end) return 0;
	if (buff_check(begin) == buff_check(end)) return end - begin;
	if (back == (char*) &A) return end - begin - 1 ;
	return begin - end - 1;
}

void buff_print_all() {
	write(STDOUT_FILENO, back, N);
	write(STDOUT_FILENO, front, N);
	printf("\n");
}

size_t buff_get_count() { return count; }

char* buff_init() {
	front = A;
	back = B;
	mark = A;
	count = 0;
	buff_update();
	buff_update();
	mark = back;
	return mark;
}