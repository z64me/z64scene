//
// rendercode.h <z64scene>
//
// for parsing and running
// user-defined rendercode
// for game instances
//

#ifndef RENDERCODE_H_INCLUDED
#define RENDERCODE_H_INCLUDED

#include <wren.h>
#include "stretchy_buffer.h"

enum ActorRenderCodeType
{
	ACTOR_RENDER_CODE_TYPE_UNINITIALIZED,
	ACTOR_RENDER_CODE_TYPE_SOURCE,
	ACTOR_RENDER_CODE_TYPE_VM,
	ACTOR_RENDER_CODE_TYPE_VM_ERROR,
};

struct ActorRenderCode
{
	enum ActorRenderCodeType type;
	union
	{
		const char *src;
		WrenVM *vm;
		char *vmErrorMessage;
	};
	long signed int lineErrorOffset;
	
	sb_array(WrenHandle *, vmHandles);
	WrenHandle *slotHandle;
	WrenHandle *callHandle;
};

#endif // RENDERCODE_H_INCLUDED
