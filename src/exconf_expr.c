#include "exconf_expr.h"

void exconf_expr_print(struct expr *e, void (*fn)(void *, struct symbol *, const char *), void *data, int prevtoken)
{
	if (!e) {
		fn(data, NULL, "y");
		return;
	}

	if (expr_compare_type(prevtoken, e->type) > 0)
		fn(data, NULL, "(");
	switch (e->type) {
	case E_SYMBOL:
		if (e->left.sym->name)
			fn(data, e->left.sym, e->left.sym->name);
		else
			fn(data, NULL, "<choice>");
		break;
	case E_NOT:
		fn(data, NULL, "!");
		exconf_expr_print(e->left.expr, fn, data, E_NOT);
		break;
	case E_EQUAL:
		if (e->left.sym->name)
			fn(data, e->left.sym, e->left.sym->name);
		else
			fn(data, NULL, "<choice>");
		fn(data, NULL, "=\"");
		fn(data, e->right.sym, e->right.sym->name);
        fn(data, NULL, "\"");
		break;
	case E_UNEQUAL:
		if (e->left.sym->name)
			fn(data, e->left.sym, e->left.sym->name);
		else
			fn(data, NULL, "<choice>");
		fn(data, NULL, "!=\"");
		fn(data, e->right.sym, e->right.sym->name);
        fn(data, NULL, "\"");
		break;
	case E_OR:
		exconf_expr_print(e->left.expr, fn, data, E_OR);
		fn(data, NULL, " || ");
		exconf_expr_print(e->right.expr, fn, data, E_OR);
		break;
	case E_AND:
		exconf_expr_print(e->left.expr, fn, data, E_AND);
		fn(data, NULL, " && ");
		exconf_expr_print(e->right.expr, fn, data, E_AND);
		break;
	case E_RANGE:
		fn(data, NULL, "[");
		fn(data, e->left.sym, e->left.sym->name);
		fn(data, NULL, " ");
		fn(data, e->right.sym, e->right.sym->name);
		fn(data, NULL, "]");
		break;
	default:
	  {
		char buf[32];
		sprintf(buf, "<unknown type %d>", e->type);
		fn(data, NULL, buf);
		break;
	  }
	}
	if (expr_compare_type(prevtoken, e->type) > 0)
		fn(data, NULL, ")");
}

static void exconf_expr_print_gstr_helper(void *data, struct symbol *sym, const char *str)
{
	str_append((struct gstr*)data, str);
}

void exconf_expr_gstr_print(struct expr *e, struct gstr *gs)
{
	exconf_expr_print(e, exconf_expr_print_gstr_helper, gs, E_NONE);
}



bool expr_eq_simple(struct expr *e1, struct expr *e2) {
    if (e1 == e2) return true;
    else if (!e1 || !e2) return false;
    else if (e1->type != e2->type) return false;

	switch (e1->type) {
	case E_EQUAL:
	case E_UNEQUAL:
		return e1->left.sym == e2->left.sym && e1->right.sym == e2->right.sym;
	case E_SYMBOL:
		return e1->left.sym == e2->left.sym;
	case E_NOT:
		return expr_eq_simple(e1->left.expr, e2->left.expr);
	case E_AND:
	case E_OR:
		return expr_eq_simple(e1->left.expr, e2->left.expr) && 
            expr_eq_simple(e1->right.expr, e2->right.expr);
	case E_LIST:
	case E_RANGE:
	case E_NONE:
		/* panic */;
	}
    return false;
}

struct expr *expr_eliminate_sub_expression(struct expr *base, struct expr *sub) {
    struct expr *left_res, *right_res;
    if (!base || !sub || expr_eq_simple(base, sub)) {
        return NULL;
    }

    if (base->type == E_AND) {
        left_res = expr_eliminate_sub_expression(base->left.expr, sub);
        right_res = expr_eliminate_sub_expression(base->right.expr, sub); 

        if (!left_res && !right_res)
            return NULL;
        else if (!right_res)
            return base->left.expr;
        else if (!left_res)
            return base->right.expr;
        else {
            base->left.expr = left_res;
            base->right.expr = right_res;
        }
    }
    return base;
}
