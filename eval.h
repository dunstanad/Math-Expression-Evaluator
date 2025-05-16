#ifndef MATH_EVAL_H
#define MATH_EVAL_H

#include <stdbool.h>

bool ambiguous_spacing(char buffer[]);
bool validate_infix(char buffer[]);
void removeSpaces(char buffer[]);
int solve(char buffer[]);

#endif
