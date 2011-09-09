#ifndef EXCONF_EXPR_H
#define EXCONF_EXPR_H

#include "expr.h"

void exconf_expr_gstr_print(struct expr *e, struct gstr *gs);
struct expr *expr_eliminate_sub_expression(struct expr *base, struct expr *sub);

#endif
