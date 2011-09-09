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

void print_menu(struct menu *menu)
{
	struct menu *child;
	struct symbol *sym;

	for (child = menu->list; child; child = child->next) {

		sym = child->sym;

        if (menu_has_help(menu)) {
            if (sym && sym->name) {
                printf("%s\n{{{\n", sym->name);
                printf("%s", _(menu_get_help(menu))); //_( == gettext
                printf("}}}\n");
            }
        }
        print_menu(child);
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

