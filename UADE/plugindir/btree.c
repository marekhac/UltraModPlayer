/* This source file is public domain (do whatever you wish with it).
   Created by Heikki Orsila <heikki.orsila@iki.fi>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "btree.h"

tnode *btree_createtree(void *data, tnodekey *key)
{
  tnode *new = calloc(1, sizeof(tnode));
  if (!new) {
    fprintf(stderr, "uade: out of memory in btree_createtree\n");
    return 0;
  }
  new->data = data;
  new->key = *key;
  return new;
}

void btree_removetree(tnode *node)
{
  if (node) {
    if (node->low) {
      btree_removetree(node->low);
    }
    if (node->high) {
      btree_removetree(node->high);
    }
    free(node);
  }
}

tnode *btree_addnode(tnode *root, void *data, tnodekey *key, int insert)
{
  tnode *node, *parent, *newnode;
  tnodekey newkey;

  int or;

  if (!root)
    return 0;

  node = root;
  parent = 0;
  or = 0;
  newkey = *key;

  while (node) {
    or = strcasecmp(newkey.key, node->key.key);
    parent = node;
    if (or < 0) {
      node = node->low;
    } else if (or > 0) {
      node = node->high;
    } else {
      if (insert) {
	free(node->data);
	node->data = data;
      }
      return node;
    }
  }

  if (!insert)
    return 0;

  newnode = calloc(1, sizeof(tnode));
  if (!newnode) {
    fprintf(stderr, "uade: out of memory in btree_addnode\n");
    return 0;
  }
  newnode->key = newkey;
  newnode->data = data;
  if (or < 0) {
    parent->low = newnode;
  } else {
    parent->high = newnode;
  }
  return newnode;
}

int btree_traverse(tnode *node, int (*callback)(tnode *node, void *arg),
		   void *arg)
{
  if (node) {
    if (node->low) {
      if (!btree_traverse(node->low, callback, arg)) {
	return 0;
      }
    }
    if (!callback(node, arg)) {
      return 0;
    }
    if (node->high) {
      if (!btree_traverse(node->high, callback, arg)) {
	return 0;
      }
    }
  }
  return 1;
}
