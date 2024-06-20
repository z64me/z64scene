//
// test.c
//
// where code is sometimes tested before being integrated
//

#include <stdlib.h>
#include <stdio.h>
#include <wren.h>

#include "misc.h"

#define CODE_AS_WRENHOOK(...) "class hooks { static draw() { " # __VA_ARGS__ " } } "

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
	const char* script = CODE_AS_STRING(
		class hooks {
			static draw() {
				System.print("ran draw()")
			}
		}
		\n // unfortunately, wren requires newline
		System.print("ran wrenInterpet()")
	);
	script = CODE_AS_WRENHOOK( System.print("ran autowrapped draw() function") );
	
	// compile the code
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
	
	// success
	if (result == WREN_RESULT_SUCCESS)
	{
		// get handles
		wrenEnsureSlots(vm, 1);
		wrenGetVariable(vm, module, "hooks", 0);
		WrenHandle *class = wrenGetSlotHandle(vm, 0);
		WrenHandle *handle = wrenMakeCallHandle(vm, "draw()");
		
		// use handles
		for (int i = 0; i < 10; ++i) {
			wrenSetSlotHandle(vm, 0, class); // is necessary each time
			if (wrenCall(vm, handle) != WREN_RESULT_SUCCESS)
				fprintf(stderr, "failed to invoke function\n");
		}
		
		// free handles
		wrenReleaseHandle(vm, class);
		wrenReleaseHandle(vm, handle);
	}
	fprintf(stderr, "done\n");
	
	//exit(0);
}
