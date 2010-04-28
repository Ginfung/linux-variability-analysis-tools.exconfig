/*
 * Copyright (C) 2009 Steven She <shshe@uwaterloo.ca>
 * Released under the terms of the GNU GPL v2.0.
 */

#define LKC_DIRECT_LINK
#include "lkc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

//Customized expr print functions
#include "exconf_expr.c"

void replace_str(char *str, char to_search, char *to_replace_with)
{
    int i=0,j=0,k=0;
    char temp[1024];

    while(str[i] != '\0')
    {
        if(str[i] == to_search)
        {
            while (to_replace_with[k] != '\0') {
                temp[j] = to_replace_with[k];
                j++; k++;
            }
            k=0; i++;
        }
        else
            temp[j++] = str[i++];
    }
    temp[j] = '\0';
    strcpy(str, temp);
} 

void print_indent(int n)
{
    if (n == 0) return;
    printf("%-*c", n, ' ');
}

void expr_cpy(struct expr *e, char *buf)
{
    if (!e) {
        buf[0] = '\0';
        return;
    }

	struct gstr e_str = str_new();
    exconf_expr_gstr_print(e, &e_str);
    strcpy(buf, str_get(&e_str));
    str_free(&e_str);
}

void print_properties(struct symbol *sym, int indent)
{
    char text_str[1024];
    char expr_str[1024];
    char visible_str[1024];

	enum prop_type ptype;
    struct property *prop;

	for (prop = sym->prop; prop; prop = prop->next) {
        ptype = prop ? prop->type : P_UNKNOWN;

        expr_cpy(prop->visible.expr, visible_str);

        switch (ptype) {
            case P_MENU:    /* prompt associated with a menuconfig option */ 
            case P_PROMPT:  /* prompt "foo prompt" or "BAZ Value" */
                print_indent(indent);
                strcpy(text_str, prop->text);
                replace_str(text_str, '"', "\\\"");
                printf("prompt \"%s\" if [%s]\n", text_str, visible_str);
                break;
            case P_COMMENT: /* text associated with a comment */
                print_indent(indent);
                strcpy(text_str, prop->text);
                replace_str(text_str, '"', "\\\"");
                printf("comment \"%s\" if [%s]\n", text_str, visible_str);
                break;
            case P_DEFAULT: /* default y */ 
                print_indent(indent);
                expr_cpy(prop->expr, expr_str);

                if (sym->type == S_STRING)
                    printf("default [\"%s\"] if [%s]\n", expr_str, visible_str);
                else
                    printf("default [%s] if [%s]\n", expr_str, visible_str);
                break;
            case P_SELECT:  /* select BAR */ 
                print_indent(indent);
                expr_cpy(prop->expr, expr_str);
                printf("select %s if [%s]\n", expr_str, visible_str);
                break;
            case P_RANGE:   /* range 7..100 (for a symbol) */
                print_indent(indent);
                expr_cpy(prop->expr, expr_str);
                printf("range %s if [%s]\n", expr_str, visible_str);
                break;
            case P_ENV:     /* value from environment variable */
                print_indent(indent);
                expr_cpy(prop->expr, expr_str);
                printf("env %s if [%s]\n", expr_str, visible_str);
                break;      
            case P_DEPENDSON: /* Looks to be the same as depends_on */
                print_indent(indent);

                //If the symbol is boolean, we rewrite the inherited to match
                //the rules of the properties in finalize_menu
                if (sym->type != S_TRISTATE)
                    prop->expr = expr_trans_bool(prop->expr);
                
                expr_cpy(prop->expr, expr_str);

                printf("depends on [%s]\n", expr_str);
                break;
            case P_CHOICE:  /* choice value */
            case P_UNKNOWN:
                //DO NOTHING
                break;
        }
    }
}

void print_menu(struct menu *menu)
{
	static int indent = 0;
    bool printed = true;

    char depends_str[1024];
	struct menu *child;
	struct symbol *sym;
	struct property *prompt;
	struct expr *depends;

	enum prop_type ptype;
	enum symbol_type stype;

	menu->flags |= MENU_ROOT;
	for (child = menu->list; child; child = child->next) {
        prompt = child->prompt;
		sym = child->sym;
        depends = child->dep; //Misleading name: really depends on plus conditions from If / Menu
        ptype = prompt ? prompt->type : P_UNKNOWN;
		stype = sym ? sym->type : S_UNKNOWN;
        printed = true;

        if (!sym && ptype == P_MENU) {
            print_indent(indent);
            printf("menu \"%s\" {\n", prompt->text);
            if (depends) {
                print_indent(indent+1);
                expr_cpy(depends, depends_str);
                printf("depends on [%s]\n", depends_str);
            }
        }
        else if (!sym && ptype == P_IF) {
            print_indent(indent);
            expr_cpy(depends, depends_str);
            printf("if [%s] {\n", depends_str);
        }
        else if (sym && sym_is_choice(sym)) {
            print_indent(indent);

            printf("choice %s %s{\n", sym_type_name(stype), 
                expr_contains_symbol(sym->rev_dep.expr, &symbol_mod) ? "" : "optional ");
            print_properties(sym, indent+1);
        }
        else if (sym) {
            assert (sym->name);
            print_indent(indent);

            if (ptype == P_MENU)
                printf("menuconfig %s %s {\n",sym->name, sym_type_name(stype));
            else
                printf("config %s %s {\n",sym->name, sym_type_name(stype));

            print_properties(sym, indent+1);
            if (depends) {
                print_indent(indent+1);

                //If the symbol is boolean, we rewrite the inherited to match
                //the rules of the properties in finalize_menu
				if (stype != S_TRISTATE)
                    depends = expr_trans_bool(depends);

                expr_cpy(depends, depends_str);
                printf("inherited [%s]\n", depends_str);
            }    
		}
        else {
            printed = false;
        }

        indent++;
        print_menu(child);
        indent--;

        if (printed) {
            print_indent(indent);
            printf("}\n");
        }
	}
}

int main(int ac, char **av)
{
    FILE *base;
    if (ac < 1) {
        if ((base = fopen ("arch/x86/Kconfig", "r")) == NULL) {
            //Kernel < 2.6.24 with old arch
            conf_parse("arch/i386/Kconfig");
        }
        else {
            conf_parse("arch/i386/Kconfig");
            fclose(base);
        }
    }
    else
        conf_parse(av[1]);

    print_menu(&rootmenu);
	return 0;
}

