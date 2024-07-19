//
// instead.c
//
// experimenting with overriding one sb_array with another, by index
//
// will be useful for having scene/room headers selectively inheriting
// different data blocks from other scene/room headers, also by index
//
// gcc -o bin/instead -Isrc/ -Wall -Wextra -Wno-unused-function -Wno-unused-variable test/instead.c && valgrind bin/instead
//

#include <stdio.h>
#include <stretchy_buffer.h>

// the base macro, resolves one level deep (acceptable for most cases)
#define HEADERBLOCK_INSTEAD(STRUCT, MEMBER, OTHERS) ( \
	((STRUCT)->MEMBER) \
		? (sb_udata_instead((STRUCT)->MEMBER) != SB_HEADER_DEFAULT_UDATA_INSTEAD \
			? &((OTHERS)[(instead = sb_udata_instead((STRUCT)->MEMBER))].MEMBER) \
			: (instead = -1, &((STRUCT)->MEMBER))) \
		: &((OTHERS)[(instead = 0)].MEMBER) \
)

// this macro accepts output from the above macro, and walks full depth
// (beware of circular dependencies; ideally, this is avoided altogether by
//  not inherting header blocks that are themselves inheriting other blocks)
// (so ideally, this macro never has to be used)
#define HEADERBLOCK_FINALIZE(VARIABLE, MEMBER, OTHERS) \
	while (VARIABLE && sb_udata_instead((*VARIABLE)) != SB_HEADER_DEFAULT_UDATA_INSTEAD) \
		VARIABLE = &((OTHERS)[(instead = sb_udata_instead((*VARIABLE)))].MEMBER);

// this macro handles the variable declaration and full inheritance resolution
#define HEADERBLOCK(TYPE, NAME, STRUCT, MEMBER, OTHERS) \
	TYPE **NAME = HEADERBLOCK_INSTEAD(STRUCT, MEMBER, OTHERS); \
	HEADERBLOCK_FINALIZE(NAME, MEMBER, OTHERS);

struct ListItem
{
	int a;
};

struct SceneHeader
{
	sb_array(struct ListItem, hey);
};

int main(void)
{
	int numOthers = 5;
	struct SceneHeader *others = calloc(numOthers, sizeof(*others));
	for (int i = 0; i < numOthers; ++i)
	{
		sb_push(others[i].hey, ((struct ListItem){i}));
		
		// 4 refs 3 refs 1
		if (i == 3)
			sb_udata_instead(others[i].hey) = 1;
		else if (i == 4)
			sb_udata_instead(others[i].hey) = 3;
	}
	
	for(int i = 0; i < numOthers; ++i)
		printf("others[%d].hey[0].a == %d, instead = %d\n"
			, i
			, others[i].hey[0].a
			, sb_udata_instead(others[i].hey)
		);
	
	for (int i = 0; i < numOthers; ++i)
	{
		int instead = -123; // this will be overwritten
		struct SceneHeader *each = &others[i];
		
		// resolves 1 deep
		struct ListItem *test = *HEADERBLOCK_INSTEAD(each, hey, others);
		printf("selected %d, %d\n", test->a, instead);
		
		// resolves full depth
		HEADERBLOCK(struct ListItem, testPP, each, hey, others);
		printf("selected %d, %d (PointerPointer)\n", (*testPP)->a, instead);
	}
	
	for (int i = 0; i < numOthers; ++i)
		sb_free(others[i].hey);
	free(others);
	return 0;
}
