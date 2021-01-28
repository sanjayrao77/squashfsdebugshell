/*
 * metablock.c - metablock caching (useful for networks)
 * Copyright (C) 2021 Sanjay Rao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#define DEBUG
#include "common/conventions.h"

#include "metablock.h"

#define LEFT(a)	((a)->treevars.left)
#define RIGHT(a)	((a)->treevars.right)
#define BALANCE(a)	((a)->treevars.balance)

static inline int cmp(struct cache_metablock *a, struct cache_metablock *b) {
return _FASTCMP(a->thisblocksoffset,b->thisblocksoffset);
}

static void rebalanceleftleft(struct cache_metablock **root_inout) {
struct cache_metablock *a=*root_inout;
struct cache_metablock *left=LEFT(a);
LEFT(a)=RIGHT(left);
RIGHT(left)=a;
*root_inout=left;
BALANCE(left)=0;
BALANCE(a)=0;
}

static void rebalancerightright(struct cache_metablock **root_inout) {
struct cache_metablock *a=*root_inout;
struct cache_metablock *right=RIGHT(a);
RIGHT(a)=LEFT(right);
LEFT(right)=a;
*root_inout=right;
BALANCE(right)=0;
BALANCE(a)=0;
}

static void rebalanceleftright(struct cache_metablock **root_inout) {
struct cache_metablock *a=*root_inout;
struct cache_metablock *left=LEFT(a);
struct cache_metablock *gchild=RIGHT(left);
int b;
RIGHT(left)=LEFT(gchild);
LEFT(gchild)=left;
LEFT(a)=RIGHT(gchild);
RIGHT(gchild)=a;
*root_inout=gchild;
b=BALANCE(gchild);
if (b>0) {
		BALANCE(a)=-1;
		BALANCE(left)=0;
} else if (!b) {
		BALANCE(a)=BALANCE(left)=0;
} else {
		BALANCE(a)=0;
		BALANCE(left)=1;
}
BALANCE(gchild)=0;
}

static void rebalancerightleft(struct cache_metablock **root_inout) {
struct cache_metablock *a=*root_inout;
struct cache_metablock *right=RIGHT(a);
struct cache_metablock *gchild=LEFT(right);
int b;
LEFT(right)=RIGHT(gchild);
RIGHT(gchild)=right;
RIGHT(a)=LEFT(gchild);
LEFT(gchild)=a;
*root_inout=gchild;
b=BALANCE(gchild);
if (b<0) {
		BALANCE(a)=1;
		BALANCE(right)=0;
} else if (!b) {
		BALANCE(a)=BALANCE(right)=0;
} else {
		BALANCE(a)=0;
		BALANCE(right)=-1;
}
BALANCE(gchild)=0;
}

static void rebalanceleftbalance(struct cache_metablock **root_inout) {
struct cache_metablock *a=*root_inout;
struct cache_metablock *left=LEFT(a);
LEFT(a)=RIGHT(left);
RIGHT(left)=a;
*root_inout=left;
BALANCE(left)=-1;
/* redundant */
// BALANCE(a)=1;
}

static void rebalancerightbalance(struct cache_metablock **root_inout) {
struct cache_metablock *a=*root_inout;
struct cache_metablock *right=RIGHT(a);
RIGHT(a)=LEFT(right);
LEFT(right)=a;
*root_inout=right;
BALANCE(right)=1;
/* redundant */
// BALANCE(a)=-1;
}

static int extracthighest(struct cache_metablock **root_inout, struct cache_metablock **node_out) {
/* returns 1 if depth decreased, else 0 */
struct cache_metablock *root=*root_inout;

if (RIGHT(root)) {
	int r=0;
	if (extracthighest(&RIGHT(root),node_out)) {
		int b;
		b=BALANCE(root);
		if (b<0) {
			BALANCE(root)=0; r=1;
		} else if (!b) {
			BALANCE(root)=1;
		} else {
				if (!BALANCE(LEFT(root))) {
					(void)rebalanceleftbalance(root_inout); 
				} else if (BALANCE(LEFT(root))==1) {
					(void)rebalanceleftleft(root_inout); 
					r=1;
				} else {
					(void)rebalanceleftright(root_inout);
					r=1;
				}
		}
	}
	return r;
} else {
	/* remove ourselves */
	*node_out=root;
	*root_inout=LEFT(root);		
	return 1;
}
}

static int extractlowest(struct cache_metablock **root_inout, struct cache_metablock **node_out) {
/* returns 1 if depth decreased, else 0 */
struct cache_metablock *root=*root_inout;

if (LEFT(root)) {
	int r=0;
	if (extractlowest(&LEFT(root),node_out)) {
		int b;
		b=BALANCE(root);
		if (b>0) {
			BALANCE(root)=0; r=1;
		} else if (!b) {
			BALANCE(root)=-1;
		} else {
				if (!BALANCE(RIGHT(root))) {
					(void)rebalancerightbalance(root_inout);
				} else if (BALANCE(RIGHT((root)))==-1) {
					(void)rebalancerightright(root_inout);
					r=1;
				} else {
					(void)rebalancerightleft(root_inout);
					r=1;
				}
		}
	}
	return r;
} else {
	/* remove ourselves */
	*node_out=root;
	*root_inout=RIGHT(root);
	return 1;
}
}


static int rmnode(struct cache_metablock **root_inout, struct cache_metablock *node) {
/* returns 1 if depth decreased, else 0 */
struct cache_metablock *root=*root_inout;
int r=0;
int c;

if (!root) { D2WHEREAMI; return 0; } /* if the node doesn't exist in the tree */

c=cmp(node,root);
if (c<0) {
	if (rmnode(&LEFT(root),node)) {
		int b;
		b=BALANCE(root);
		if (b>0) {
				BALANCE(root)=0;
				r=1;
		} else if (!b) {
				BALANCE(root)=-1;
		} else {
				if (!BALANCE(RIGHT(root))) {
					(void)rebalancerightbalance(root_inout);
				} else if (BALANCE(RIGHT(root))<0) {
					(void)rebalancerightright(root_inout);
					r=1;
				} else {
					(void)rebalancerightleft(root_inout);
					r=1;
				}
		}
	}
} else if (c>0) {
	if (rmnode(&RIGHT(root),node)) {
		int b;
		b=BALANCE(root);
		if (b<0) {
				BALANCE(root)=0;
				r=1;
		} else if (!b) {
				BALANCE(root)=1;
		} else {
				if (!BALANCE(LEFT(root))) {
					(void)rebalanceleftbalance(root_inout);
				} else if (BALANCE(LEFT(root))>0) {
					(void)rebalanceleftleft(root_inout);
					r=1;
				} else {
					(void)rebalanceleftright(root_inout);
					r=1;
				}
		}
	}
} else {
	/* found it */
	struct cache_metablock *temp;
	if (BALANCE(root)==1) {
		if (extracthighest(&LEFT(root),&temp)) {
			BALANCE(temp)=0;
			r=1;
		} else {
			BALANCE(temp)=1;
		}
		LEFT(temp)=LEFT(root);
		RIGHT(temp)=RIGHT(root);
		*root_inout=temp;
	} else if (BALANCE(root)==-1) {
		if (extractlowest(&RIGHT(root),&temp)) {
			BALANCE(temp)=0;
			r=1;
		} else {
			BALANCE(temp)=-1;
		}
		LEFT(temp)=LEFT(root);
		RIGHT(temp)=RIGHT(root);
		*root_inout=temp;
	} else { /* balance 0 */
		if (LEFT(root)) {
			if (extracthighest(&LEFT(root),&temp)) {
				BALANCE(temp)=-1;
			} else {
				BALANCE(temp)=0;
			}
			LEFT(temp)=LEFT(root);
			RIGHT(temp)=RIGHT(root);
			*root_inout=temp;
		} else {
			*root_inout=NULL;
			r=1;
		}
	}
}

return r;
}

static int addnode(struct cache_metablock **root_inout, struct cache_metablock *node) {
/* returns 1 if depth increased, else 0 */
struct cache_metablock *root=*root_inout;
int r=0;

if (!root) {
	*root_inout=node;
	return 1;
}

if (cmp((node),(root))<0) {
	if (addnode(&LEFT(root),node)) {
		int b;
		b=BALANCE(root);
		if (!b) {
			BALANCE(root)=1; r=1;
		} else if (b>0) {
				if (BALANCE(LEFT(root))>0) (void)rebalanceleftleft(root_inout); else (void)rebalanceleftright(root_inout);
		} else {
			BALANCE(root)=0;
		}
	}
} else {
	if (addnode(&RIGHT(root),node)) {
		int b;
		b=BALANCE(root);
		if (!b) {
			BALANCE(root)=-1; r=1;
		} else if (b>0) {
			BALANCE(root)=0;
		} else {
				if (BALANCE(RIGHT(root))<0) (void)rebalancerightright(root_inout); else (void)rebalancerightleft(root_inout);
		}
	}
}

return r;
}

void addnode_cache_metablock(struct cache_metablock **treetop_inout, struct cache_metablock *node) {
(ignore)addnode(treetop_inout,node);
}
void rmnode_cache_metablock(struct cache_metablock **treetop_inout, struct cache_metablock *node) {
(ignore)rmnode(treetop_inout,node);
}

struct cache_metablock *find_cache_metablock(struct cache_metablock *root, uint64_t offset) {
while (root) {
	if (offset<root->thisblocksoffset) {
		root=LEFT(root);
	} else if (offset>root->thisblocksoffset) {
		root=RIGHT(root);
	} else {
		return root;
	}
}
return NULL;
}

int printlist_metablock(struct cache_metablock *c) {
while (c) {
	printf("struct metablock {\n"\
"	uint64_t thisblocksoffset = %"PRIu64", nextblocksoffset = %"PRIu64";\n"\
"	uint32_t usecounter = %u;\n"\
"} = %p;\n",c->thisblocksoffset,c->nextblocksoffset,c->usecounter,c);

	c=c->listvars.next;
}
return 0;
}

