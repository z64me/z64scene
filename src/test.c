//
// test.c
//
// where code is sometimes tested before being integrated
//

#include <stdlib.h>
#include <stdio.h>
#include <wren.h>
#include <stdbool.h>

#include "logging.h"
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
class Inst { \
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
	// skip newlines
	if (strspn(text, "\r\n\t ") == strlen(text))
		return;
	
	LogDebug("%s", text);
}

static void errorFn(WrenVM* vm, WrenErrorType errorType,
	const char* module, const int line,
	const char* msg
)
{
	switch (errorType)
	{
		case WREN_ERROR_COMPILE:
			LogDebug("[%s line %d] [Error] %s", module, line, msg);
			break;
		case WREN_ERROR_STACK_TRACE:
			LogDebug("[%s line %d] in %s", module, line, msg);
			break;
		case WREN_ERROR_RUNTIME:
			LogDebug("[Runtime Error] %s", msg);
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
	void InstGetXpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->Xpos); }
	void InstGetYpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->Ypos); }
	void InstGetZpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->Zpos); }
	
	void DrawSetScale3(WrenVM* vm) {
		float xscale = wrenGetSlotDouble(vm, 1);
		float yscale = wrenGetSlotDouble(vm, 2);
		float zscale = wrenGetSlotDouble(vm, 3);
		LogDebug("DrawSetScale3 = %f %f %f", xscale, yscale, zscale);
	}
	void DrawSetScale1(WrenVM* vm) {
		float xscale = wrenGetSlotDouble(vm, 1);
		float yscale = xscale;
		float zscale = xscale;
		LogDebug("DrawSetScale1 = %f %f %f", xscale, yscale, zscale);
	}
	
	if (streq(module, "main")) {
		// no setters, are read-only
		if (streq(className, "Inst")) {
			if (streq(signature, "Xpos")) return InstGetXpos;
			else if (streq(signature, "Ypos")) return InstGetYpos;
			else if (streq(signature, "Zpos")) return InstGetZpos;
			//else if (streq(signature, "Xpos=(_)")) return InstSetXpos; // no setter, is read-only
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

// testing falsy 0 and more
void TestWrenSimple(void)
{
	WrenConfiguration config;
	wrenInitConfiguration(&config);
		config.writeFn = &writeFn;
		config.errorFn = &errorFn;
		config.bindForeignMethodFn = &bindForeignMethod;
	WrenVM* vm = wrenNewVM(&config);
	
	const char *module = "main";
	const char *script = R"(
		System.print("0 == false : %(0 == false) ")
		System.print("1 == true  : %(1 == true) ")
		System.print("2 == true  : %(2 == true) ")
		System.print("2 != true  : %(2 != true) ")
		System.print("2 == false : %(2 == false) ")
		System.print("0 == true  : %(0 == true) ")
		System.print("false == 0 : %(false == 0) ")
		System.print("true == 1  : %(true == 1) ")
		System.print("true == 2  : %(true == 2) ")
		System.print("false == false  : %(false == false) ")
		System.print("true  == true   : %(true  == true) ")
		System.print("false != true   : %(false != true) ")
		System.print("false == true   : %(false == true) ")
		System.print("true  == false  : %(true  == false) ")
		for (i in 0..5) {
			System.print("%(i) == false : %(i == false) ")
		}
		System.print("!0 = %(!0) ")
		System.print("!1 = %(!1) ")
		System.print("!!0 = %(!!0) ")
		System.print("!!1 = %(!!1) ")
		var test = 0
		System.print("test = %(test), !test = %(!test) ")
		test = 100
		System.print("test = %(test), !test = %(!test) ")
		test = (true && 0 && true)
		System.print(test)
		if (0 || 0 || false) {
			System.print("falsy 0 not working")
		} else {
			System.print("falsy 0 is working")
		}
		if (1) {
			System.print("if (1) ")
		}
	)";
	LogDebug("script = %s", script);
	LogDebug("((bool)123) == true  : %d", ((bool)123) == true);
	LogDebug("((bool)123) == false : %d", ((bool)123) == false);
	
	// compile the code
	WrenInterpretResult result = wrenInterpret(vm, module, script);
	
	switch (result)
	{
		case WREN_RESULT_COMPILE_ERROR:
			LogDebug("Compile Error!");
			break;
		case WREN_RESULT_RUNTIME_ERROR:
			LogDebug("Runtime Error!");
			break;
		case WREN_RESULT_SUCCESS:
			LogDebug("Success!");
			break;
	}
	
	wrenFreeVM(vm);
	//exit(0);
}

void TestWren(void)
{
	TestWrenSimple();
	return;
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
		System.print("test %(Inst.Xpos) %(Inst.Xpos / 2) %(Inst.Xpos >> 1) ")
		Draw.SetScale(1.23)
		Draw.SetScale(1.23, 4.56, 7.89)
		System.print("Props.Hello = %(Props.Hello) ")
	)" );
	LogDebug("script = %s", script);
	
	// compile the code
	WrenInterpretResult result = wrenInterpret(vm, module, script);
	
	switch (result)
	{
		case WREN_RESULT_COMPILE_ERROR:
			LogDebug("Compile Error!");
			break;
		case WREN_RESULT_RUNTIME_ERROR:
			LogDebug("Runtime Error!");
			break;
		case WREN_RESULT_SUCCESS:
			LogDebug("Success!");
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
				LogDebug("failed to invoke function");
		}
		
		// free handles
		wrenReleaseHandle(vm, class);
		wrenReleaseHandle(vm, handle);
		wrenReleaseHandle(vm, propsVar);
		wrenReleaseHandle(vm, propsSetHello);
	}
	LogDebug("done");
	
	wrenFreeVM(vm);
	//exit(0);
}
