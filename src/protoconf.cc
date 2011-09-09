#include <iostream>
#include <fstream>
#include <string>
#include "protobuf/extract.pb.h"
#include "lkc.h"
#include "expr.h"

extern "C" {
#include "exconf_expr.h"
}

using namespace std;

string expr_to_string(struct expr *e) {
    if (!e) return "";

    struct gstr e_str = str_new();
    exconf_expr_gstr_print(e, &e_str);
    string result = string(str_get(&e_str));
    str_free(&e_str);
    return result;
}

Node::DataType parse_datatype(symbol_type stype) {
    switch (stype) {
        case S_BOOLEAN:
            return Node::BOOLEAN;
        case S_TRISTATE:
            return Node::TRISTATE;
        case S_INT:
            return Node::INT;
        case S_HEX:
            return Node::HEX;
        case S_STRING:
            return Node::STRING;
        case S_UNKNOWN:
        case S_OTHER:
        default:
            cerr << "error, unknown symbol type: %s" << sym_type_name(stype) << endl;
            return Node::BOOLEAN;
    }
}

// Removes the depends on properties defined in sym from base
void remove_depends_on_properties(symbol* sym, expr* base_expr) {
    if (!sym)
        return;

    for (property* prop = sym->prop; prop; prop = prop->next) {
        enum prop_type ptype = prop ? prop->type : P_UNKNOWN;
        switch (ptype) {
            case P_DEPENDSON:
                expr* dep_expr = expr_copy(prop->expr);
                base_expr = expr_eliminate_sub_expression(base_expr, dep_expr);
                expr_free(dep_expr);
                break;
        }
    }
}

void parse_properties(symbol* sym, Node* curr) {
    if (!sym)
        return;

    for (property* prop = sym->prop; prop; prop = prop->next) {
        enum prop_type ptype = prop ? prop->type : P_UNKNOWN;
        PropertyProto* node_prop = curr->add_property();

        switch (ptype) {
            case P_MENU:    /* prompt associated with a menuconfig option */ 
            case P_PROMPT:  /* prompt "foo prompt" or "BAZ Value" */
                node_prop->set_propertytype(PropertyProto::PROMPT);
                break;
            case P_DEFAULT:
                node_prop->set_propertytype(PropertyProto::DEFAULT);
                break;
            case P_SELECT:
                node_prop->set_propertytype(PropertyProto::SELECT);
                break;
            case P_RANGE:
                node_prop->set_propertytype(PropertyProto::RANGE);
                break;
            case P_ENV:
                node_prop->set_propertytype(PropertyProto::ENV);
                break;
            case P_DEPENDSON:
                node_prop->set_propertytype(PropertyProto::DEPENDS_ON);

                //If the symbol is boolean, we rewrite the inherited to match
                //the rules of the properties in finalize_menu
                if (sym->type != S_TRISTATE)
                    prop->expr = expr_trans_bool(prop->expr);
                break;
            case P_CHOICE:
                // TODO currently ignored...
                cerr << "Ignoring choice value property..." << endl;
                node_prop->set_propertytype(PropertyProto::UNKNOWN);
                break;
            default:
                cerr << "Unknown property of type: " << ptype << endl;
                node_prop->set_propertytype(PropertyProto::UNKNOWN);
                break;
        }

        // visibility condition for the property
        node_prop->set_visibleexpr(expr_to_string(prop->visible.expr));

        // value (select X or depends on Y)
        node_prop->set_value(expr_to_string(prop->expr));

        // prompt text if any
        if (prop->text)
            node_prop->set_text(prop->text);
    }
}

void parse_menu(struct menu *menu, Node* parent) {
	for (struct menu* item = menu->list; item; item = item->next) {
        Node* curr = parent->add_child();

        symbol* sym = item->sym;
	    property* prompt = item->prompt;
        prop_type ptype = prompt ? prompt->type : P_UNKNOWN;

        // expression from if conditions and menus 
        expr* inherited_expr = item->dep ? expr_copy(item->dep) : NULL;

        remove_depends_on_properties(sym, inherited_expr);
        curr->set_inherited(expr_to_string(inherited_expr));
        expr_free(inherited_expr);

        // choice
        if (sym && sym_is_choice(sym)) {
            curr->set_nodetype(Node::CHOICE);
            curr->set_datatype(parse_datatype(sym->type));

            // choice is optional if the reverse-dependency is mod
            curr->set_opt(expr_contains_symbol(sym->rev_dep.expr, &symbol_mod));
        }
        // menuconfig or config
        else if (sym) {
            curr->set_id(sym->name);
            curr->set_nodetype(ptype == P_MENU ? Node::MENUCONFIG : Node::CONFIG);
            curr->set_datatype(parse_datatype(sym->type));
        }
        else if (ptype == P_MENU || ptype == P_COMMENT) {
            curr->set_nodetype(ptype == P_MENU ? Node::MENU : Node::COMMENT);

            // Special case to handle menu prompt and depends on
            PropertyProto* prompt_prop = curr->add_property();
            prompt_prop->set_propertytype(PropertyProto::PROMPT);
            prompt_prop->set_text(prompt->text);
        }
        else if (ptype == P_IF) {
            // TODO If nodes don't have depends on condition - so we don't
            // know the syntactic declaration
            curr->set_nodetype(Node::IF);
        }
        else {
            cerr << "Unknown node detected of type: " << ptype << endl;
            curr->set_nodetype(Node::UNKNOWN);
        }

        // add properties to the current node
        parse_properties(sym, curr);

        // parse children of this item
        parse_menu(item, curr);
    }
}

int main(int argc, const char** argv) {
    conf_parse(argv[1]);

    Node rootnode;
    rootnode.set_id("root");
    rootnode.set_nodetype(Node::MENU);
    rootnode.set_datatype(Node::BOOLEAN);

    parse_menu(&rootmenu, &rootnode);

    fstream out("extract.pb", ios::out | ios::binary | ios::trunc);
    if (!rootnode.SerializeToOstream(&out)) {
         cerr << "failed to write to standard output." << endl;
         return 1;
    }

    out.close();

    // rootnode.PrintDebugString();

    google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
