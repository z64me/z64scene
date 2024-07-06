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

// for reporting the correct line number in wren callbacks
static int sLine = 0;

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
			LogDebug("[%s line %d] [Error] %s", module, line + sLine, msg);
			break;
		case WREN_ERROR_STACK_TRACE:
			LogDebug("[%s line %d] in %s", module, line + sLine, msg);
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
	sLine = __LINE__;
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
		if (false || true)
		{
			System.print("curly brace after newline")
		}
		
		if (true) { System.print("same line if statement (block) ") }
		if (true) System.print("same line if statement (no block) ")
		System.print("next line")
		System.print(Fn.new {
			if (false) "no" else return "ok"
		}.call()) // expect: ok
		class TestClass
		{
			test { _test }
			construct new()
			{
				_test = "test"
			}
			static what()
			{
				System.print("wow ok")
			}
			
			PrintTest()
			{
				System.print("PrintTest() was invoked")
			}
		}
		var testClass = TestClass.new()
		System.print("testClass.test = %(testClass.test) ")
		testClass.PrintTest()
		
		// simple function
		var sayMessage = Fn.new
		{
			| recipient, message |
			
			System.print("message for %(recipient): %(message) ")
		}
		sayMessage.call("test", "hello world")
		
		for (i in 0..5)
		{
			System.print("%(i) for() loop with newline")
		}
		
		var whileTest = 0
		while (whileTest < 5)
		{
			System.print("%(whileTest) while() loop with newline")
			whileTest = whileTest + 1
		}
		
		//if (true)
		if (false)
		// test
		// this
		// comment
		/* and
		 /* this
		    block */
		 comment
		*/
		{
			System.print("if() with brace on newline")
		}
		else if (false)
		{
			System.print("else if() with brace on newline")
		}
		// comments too
		else
		{
			System.print("else{} with brace on newline")
		}
		
		System.print("this precedes a standalone block")
		// anonymous block
		{
			System.print("this is a standalone block")
		}
		{
			System.print("this is another standalone block")
		}
		
		if ((true)
			&& true
			&& (true
				|| true
				|| (true && true)
			) && (1 + 1 == 2)
		)
		{
			System.print("curly brace after complex block")
		}
		
		var multiLineAdd = (
			1
			+ 2
			+ 3
		)
		System.print("multiLineAdd result = %(multiLineAdd) ")
		System.print("multiLineAdd inline = "
			+ "%(
				1
				+ 2
				+ 3
			) "
		)
		
		if (true) System.print("what")
		else if (true) System.print("what-1")
		else System.print("wow")
		
		if (false)
			System.print("if w/ newline, no braces")
		else if (true
			&& true
			&& (false || true)
			&& false
		)
			System.print("if-else w/ newline, no braces")
		else
			System.print("else w/ newline, no braces")
		
		if (true)
			for (i in 0..5)
				System.print("if-for w/ newlines, no braces %(i) ")
		
		System.print("binary as decimal: %(0b0100_1101_0010) ") // expect 1234
		//System.print("binary as decimal: %(0b1234) ") // error
		//System.print("binary as decimal: %(0b1100_1101_0010_0010_0100_1101_0010_0010_0100_1101_0010_0010_0100_1101_0010_0010) ") // big number
		//System.print("binary as decimal: %(0b1_1100_1101_0010_0010_0100_1101_0010_0010_0100_1101_0010_0010_0100_1101_0010_0010) ") // number too big error
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

//
// analyze actors in every room in every scene, to better understand MM rotation flags
//
// findings:
//  - anywhere rotation flags are used, they are used consistently for all
//    instances of actors sharing that type (with only two exceptions)
//  - actor type 0x01ab - the timer that kicks the player out of rooms
//    depending on the hour/minute of the day, but a variation of this
//    actor that is used in the swamp for the owl song engraving (the
//    rotation values seem to be unused in this case?)
//  - actor type 0x001c - the y rotation is only used in calculations
//    if ((params & 3) == 1), and seems to be unused otherwise
//  - any other discrepancies where a yxz flag is not set, its rotation
//    bits are all 0, which means the same rotation values will be
//    derived whether or not the flag is set for that rotation axis
//
void TestAnalyzeSceneActors(struct Scene *scene, const char *logFilename)
{
	static struct {
		bool isInit;
		bool isYrotRaw;
		bool isXrotRaw;
		bool isZrotRaw;
		sb_array(char*, sets); // unique yxz flag variations
		char set[5];
		char *latestRoomName;
		uint16_t params;
	} *db = 0, *type, work;
	static FILE *log = 0;
	const int dbCount = 0xfff;
	
	#define ROT_CHARS(ACCESSOR) \
		ACCESSOR isYrotRaw ? 'y' : ' ' \
		, ACCESSOR isXrotRaw ? 'x' : ' ' \
		, ACCESSOR isZrotRaw ? 'z' : ' '
	
	if (!db)
	{
		db = calloc(dbCount, sizeof(*db));
		if (logFilename)
			log = fopen(logFilename, "w");
	}
	
	if (!scene && db)
	{
		for (int i = 0; i < dbCount; ++i)
		{
			type = &db[i];
			
			// no actors of this type encountered
			if (type->isInit == false)
				continue;
			
			// this actor doesn't use any rotation flags
			if (!strcmp(type->set, "   "))
				continue;
			
			if (log)
			{
				//fprintf(log, "%04x %d %d %d\n", i, type->isYrotRaw, type->isXrotRaw, type->isZrotRaw);
				if (true)
					fprintf(log, "%04x: [%s]\n", i, type->set);
				else // print all unique variations
				{
					fprintf(log, "%04x: %s --- ", i, type->set);
					sb_foreach(type->sets, {
						fprintf(log, "[%s] ", *each);
					})
					fprintf(log, "\n");
				}
			}
		}
		
		free(db);
		db = 0;
		
		if (log)
			fclose(log);
		log = 0;
	}
	
	if (!scene)
		return;
	
	sb_foreach_named(scene->rooms, room, {
		const char *currentRoomName = room->file->filename;
		sb_foreach_named(room->headers, header, {
			sb_foreach_named(header->instances, inst, {
				uint16_t id = inst->id;
				uint16_t masked = id & 0xfff;
				type = &db[masked];
				work = *type;
				work.isYrotRaw = id & 0x8000;
				work.isXrotRaw = id & 0x4000;
				work.isZrotRaw = id & 0x2000;
				work.isInit = true;
				uint16_t yrot = inst->yrot >> 7;
				uint16_t xrot = inst->xrot >> 7;
				uint16_t zrot = inst->zrot >> 7;
				if (type->isInit)
				{
					// report if any setting is turned off and uses a non-zero rotation
					if ((work.isYrotRaw < type->isYrotRaw && yrot)
						|| (work.isXrotRaw < type->isXrotRaw && xrot)
						|| (work.isZrotRaw < type->isZrotRaw && zrot)
					)
					{
						fprintf(log, "differing rot settings on 0x%04x(%04x->%04x): [%c%c%c] -> [%c%c%c]\n"
							, masked
							, work.params
							, inst->params
							, ROT_CHARS(type->)
							, ROT_CHARS(work.)
						);
						if (work.latestRoomName && strcmp(work.latestRoomName, currentRoomName))
							fprintf(log, "   (%s -> %s)\n", work.latestRoomName, currentRoomName);
					}
				}
				char set[5];
				bool setExists = false;
				snprintf(set, sizeof(set), "%c%c%c", ROT_CHARS(work.));
				sb_foreach(work.sets, {
					if (!strcmp(*each, set))
						setExists = true;
				})
				if (setExists == false)
					sb_push(work.sets, strdup(set));
				for (int i = 0; i < 3; ++i)
					work.set[i] = MAX(work.set[i], set[i]);
				if (!work.latestRoomName || strcmp(work.latestRoomName, currentRoomName))
				{
					if (work.latestRoomName)
						free(work.latestRoomName);
					work.latestRoomName = strdup(currentRoomName);
				}
				work.params = inst->params;
				*type = work;
			})
		})
	})
}
