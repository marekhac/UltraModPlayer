/* This source file is public domain (do whatever you wish with it).
   Created by Heikki Orsila <heikki.orsila@tut.fi>
*/

#ifndef _UADE_BTREE_H_
#define _UADE_BTREE_H_

struct btreekeydata {
  char key[34]; /* 32 bytes + 1 byte for zero is used (1 more for safety) */
};

typedef struct btreekeydata tnodekey;

struct btreenode {
  struct btreenode *low;
  struct btreenode *high;
  tnodekey key;
  void *data;
};

typedef struct btreenode tnode;

tnode *btree_createtree(void *data, tnodekey *key);
void btree_removetree(tnode *node);
tnode *btree_addnode(tnode *root, void *data, tnodekey *key, int insert);
int btree_traverse(tnode *node, int (*callback)(tnode *node, void *arg), void *arg);

int btree_removenode(tnode **root, tnodekey *key);

#endif
