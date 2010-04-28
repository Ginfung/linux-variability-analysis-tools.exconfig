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
