#ifndef STATELESS_H
#define STATELESS_H

// https://stackoverflow.com/questions/1872220/is-it-possible-to-iterate-over-arguments-in-variadic-macros

/*
#define UNWRAP(first, ...) do {\
	printf("%s\n", (first));\
	DEFER(UNWRAP(__VA_ARGS__));\
} while (0)

#define WITH_STATE(state, enabler, disabler, code) do {\
	gl##enabler((state));\
	code\
	gl##disabler((state));\
} while (0)
*/

////////// FOR_EACH iterates over its arguments

#define FOR_EACH_1(f, x) f(x)
#define FOR_EACH_2(f, x, ...) f(x); FOR_EACH_1(f, __VA_ARGS__)
#define FOR_EACH_3(f, x, ...) f(x); FOR_EACH_2(f, __VA_ARGS__)
#define FOR_EACH_4(f, x, ...) f(x); FOR_EACH_3(f, __VA_ARGS__)
#define FOR_EACH_5(f, x, ...) f(x); FOR_EACH_4(f, __VA_ARGS__)
#define FOR_EACH_6(f, x, ...) f(x); FOR_EACH_5(f, __VA_ARGS__)
#define FOR_EACH_7(f, x, ...) f(x); FOR_EACH_6(f, __VA_ARGS__)
#define FOR_EACH_8(f, x, ...) f(x); FOR_EACH_7(f, __VA_ARGS__)
#define FOR_EACH_9(f, x, ...) f(x); FOR_EACH_8(f, __VA_ARGS__)
#define FOR_EACH_10(f, x, ...) f(x); FOR_EACH_9(f, __VA_ARGS__)

#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N 
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__) 
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define CONCAT(arg1, arg2) arg1##arg2
#define _FOR_EACH(n, f, ...) CONCAT(FOR_EACH_, n)(f, __VA_ARGS__)
#define FOR_EACH(f, ...) _FOR_EACH(FOR_EACH_NARG(__VA_ARGS__), f, __VA_ARGS__)

//////////

#define PREFIX(x) gl##x

#define WITH_SETTING(enabler, disabler, state, code) do {\
	gl##enabler(state);\
	code\
	gl##disabler(state);\
} while (0)

#define WITH_BINDING()

#endif
