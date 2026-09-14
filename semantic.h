#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include "exprtree.h"

int analyzeSemantics(struct tnode *functions, struct tnode *root, FILE *out);

#endif
