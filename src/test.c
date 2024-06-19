//
// test.c
//
// where code is sometimes tested before being integrated
//

#include <stdio.h>
#include <wren.h>

#include "misc.h"

static void writeFn(WrenVM* vm, const char* text)
{
	fprintf(stderr, "%s", text);
}

static void errorFn(WrenVM* vm, WrenErrorType errorType,
	const char* module, const int line,
	const char* msg
)
{
	switch (errorType)
	{
		case WREN_ERROR_COMPILE:
			fprintf(stderr, "[%s line %d] [Error] %s\n", module, line, msg);
			break;
		case WREN_ERROR_STACK_TRACE:
			fprintf(stderr, "[%s line %d] in %s\n", module, line, msg);
			break;
		case WREN_ERROR_RUNTIME:
			fprintf(stderr, "[Runtime Error] %s\n", msg);
			break;
	}
}

void TestWren(void)
{
	WrenConfiguration config;
	wrenInitConfiguration(&config);
		config.writeFn = &writeFn;
		config.errorFn = &errorFn;
	WrenVM* vm = wrenNewVM(&config);
	
	const char* module = "main";
	const char* script = CODE_AS_STRING(System.print("I am running in a VM!"));
	
	WrenInterpretResult result = wrenInterpret(vm, module, script);
	
	switch (result)
	{
		case WREN_RESULT_COMPILE_ERROR:
			fprintf(stderr, "Compile Error!\n");
			break;
		case WREN_RESULT_RUNTIME_ERROR:
			fprintf(stderr, "Runtime Error!\n");
			break;
		case WREN_RESULT_SUCCESS:
			fprintf(stderr, "Success!\n");
			break;
	}
	
	wrenFreeVM(vm);
}
