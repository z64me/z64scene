//
// test.c
//
// where code is sometimes tested before being integrated
//

#include <stdlib.h>
#include <stdio.h>
#include <wren.h>

#include "misc.h"

typedef struct TestWrenUdata
{
	float Xpos;
	float Ypos;
	float Zpos;
} TestWrenUdata;
#define WREN_UDATA ((TestWrenUdata*)wrenGetUserData(vm))

// will contain pos/rot/scale/params
// meanwhile, class Prop will be populated
// based on properties defined in the toml
#define WRENHOOK_PREFIXES " \
class World { \
	foreign static Xpos \n \
	foreign static Ypos \n \
	foreign static Zpos \n \
} \
\n \
class Draw { \
	foreign static SetScale(xscale, yscale, zscale) \n \
	foreign static SetScale(scale) \n \
} \
\n \
class props__hooks { \
	/* need to generate getter/setter for each actor property */ \
	Hello { _Hello }/* get */ \n \
	Hello=(v) { _Hello = v }/* set */ \n \
	construct new() { \n \
	} \n \
} \
\n \
var Props = props__hooks.new() \n \
\n \
"

#define CODE_AS_WRENHOOK(...) WRENHOOK_PREFIXES "class hooks { static draw() { \n " # __VA_ARGS__ " } } "

#define CODE_AS_WRENHOOK_R(WHAT) WRENHOOK_PREFIXES "class hooks { static draw() { \n " WHAT " } } "

#define streq(A, B) !strcmp(A, B)

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

static WrenForeignMethodFn bindForeignMethod(
	WrenVM* vm
	, const char* module
	, const char* className
	, bool isStatic
	, const char* signature
)
{
	void WorldGetXpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->Xpos); }
	void WorldGetYpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->Ypos); }
	void WorldGetZpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->Zpos); }
	
	void DrawSetScale3(WrenVM* vm) {
		float xscale = wrenGetSlotDouble(vm, 1);
		float yscale = wrenGetSlotDouble(vm, 2);
		float zscale = wrenGetSlotDouble(vm, 3);
		fprintf(stderr, "DrawSetScale3 = %f %f %f\n", xscale, yscale, zscale);
	}
	void DrawSetScale1(WrenVM* vm) {
		float xscale = wrenGetSlotDouble(vm, 1);
		float yscale = xscale;
		float zscale = xscale;
		fprintf(stderr, "DrawSetScale1 = %f %f %f\n", xscale, yscale, zscale);
	}
	
	if (streq(module, "main")) {
		// no setters, are read-only
		if (streq(className, "World")) {
			if (streq(signature, "Xpos")) return WorldGetXpos;
			else if (streq(signature, "Ypos")) return WorldGetYpos;
			else if (streq(signature, "Zpos")) return WorldGetZpos;
			//else if (streq(signature, "Xpos=(_)")) return WorldSetXpos; // no setter, is read-only
		}
		else if (streq(className, "Prop")) {
			// TODO
		}
		else if (streq(className, "Draw")) {
			if (streq(signature, "SetScale(_,_,_)")) return DrawSetScale3;
			else if (streq(signature, "SetScale(_)")) return DrawSetScale1;
		}
	}
	
	return 0;
}

void TestWren(void)
{
	WrenConfiguration config;
	wrenInitConfiguration(&config);
		config.writeFn = &writeFn;
		config.errorFn = &errorFn;
		config.bindForeignMethodFn = &bindForeignMethod;
	WrenVM* vm = wrenNewVM(&config);
	TestWrenUdata udata = {
		.Xpos = 123,
		.Ypos = 456,
		.Zpos = 789,
	};
	wrenSetUserData(vm, &udata);
	
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
	script = CODE_AS_WRENHOOK_R( R"(
		System.print("test %(World.Xpos) %(World.Xpos / 2) %(World.Xpos >> 1) ")
		Draw.SetScale(1.23)
		Draw.SetScale(1.23, 4.56, 7.89)
		System.print("Props.Hello = %(Props.Hello) ")
	)" );
	fprintf(stderr, "script = %s\n", script);
	
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
		
		// get the setter
		wrenEnsureSlots(vm, 2);
		wrenGetVariable(vm, module, "Props", 0);
		WrenHandle *propsVar = wrenGetSlotHandle(vm, 0);
		WrenHandle *propsSetHello = wrenMakeCallHandle(vm, "Hello=(_)");
		
		// use handles
		for (int i = 0; i < 10; ++i) {
			// use the setter
			wrenEnsureSlots(vm, 2);
			wrenSetSlotHandle(vm, 0, propsVar);
			wrenSetSlotDouble(vm, 1, i);
			wrenCall(vm, propsSetHello);
			// and the main func
			wrenEnsureSlots(vm, 1);
			wrenSetSlotHandle(vm, 0, class); // is necessary each time
			if (wrenCall(vm, handle) != WREN_RESULT_SUCCESS)
				fprintf(stderr, "failed to invoke function\n");
		}
		
		// free handles
		wrenReleaseHandle(vm, class);
		wrenReleaseHandle(vm, handle);
		wrenReleaseHandle(vm, propsVar);
		wrenReleaseHandle(vm, propsSetHello);
	}
	fprintf(stderr, "done\n");
	
	wrenFreeVM(vm);
	//exit(0);
}
