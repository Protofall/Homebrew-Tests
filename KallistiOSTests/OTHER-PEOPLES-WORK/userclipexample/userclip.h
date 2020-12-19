#include <assert.h>

struct ta_userclip_cmd {
	int cmd;
	int padding[3];
	int minx;
	int miny;
	int maxx;
	int maxy;
};

static inline void make_userclip (struct ta_userclip_cmd *dst, unsigned int minx, unsigned int miny,
		unsigned int maxx, unsigned int maxy) {
	#ifndef NDEBUG
		assert((minx % 32) == 0);
		assert((miny % 32) == 0);
		assert((maxx % 32) == 0);
		assert((maxy % 32) == 0);
		assert(minx < maxx);
		assert(miny < maxy);
		assert(maxx <= 1280);
		assert(maxy <= 480);
	#endif
	
	dst->cmd = PVR_CMD_USERCLIP;
	dst->minx = minx >> 5;
	dst->miny = miny >> 5;
	dst->maxx = (maxx >> 5) - 1;
	dst->maxy = (maxy >> 5) - 1;
}

