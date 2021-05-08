

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <algorithm>
#include "semant.h"
#include "utilities.h"

using namespace cool;

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

static ClassTable * classtable;

void ClassTable::halt() {
    if (errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}
bool ClassTable::is_subclass(Symbol return_type, Symbol decl_type, Symbol self_name) {
    if(return_type == SELF_TYPE && decl_type == SELF_TYPE){
        return true;
    }
    else if(return_type == SELF_TYPE && decl_type != SELF_TYPE) {
        return is_subclass(self_name, decl_type);
    }
    else if(return_type != SELF_TYPE && decl_type == SELF_TYPE) {
        return false;
    }
    else {
        return is_subclass(return_type, decl_type);
    }
}

bool ClassTable::is_subclass(Symbol return_type, Symbol decl_type) {
    Class_ class_node = graph[return_type];
    if (return_type == decl_type)
        return true;
    if (class_node == NULL)
        return false;
    return (is_subclass(class_node->get_parent(), decl_type));
}

Symbol ClassTable::get_lub(Symbol a, Symbol b) {
    if (is_subclass(a, b))
        return b;
    else if (is_subclass(b, a))
        return a;
    else 
        return get_lub(graph[a]->get_parent(), b);
}

bool ClassTable::is_type_exist(Symbol type, Symbol filename, tree_node *t) {
    if (graph.find(type) == graph.end() && type != SELF_TYPE) { 
        return false;
    }
    return true;
}

bool ClassTable::is_method_exist(Symbol class_name, Symbol method_name, Symbol filename, tree_node *t) {
    if(global_method_table[class_name]->lookup(method_name) == NULL) {
        return false;
    }
    return true;
}

std::map<Symbol, Class_> ClassTable::get_graph() {
    return graph;
}

SymbolTable<Symbol, attr_class>* ClassTable::get_attr_table(Symbol name) {
    return global_attr_table[name];
}

SymbolTable<Symbol, method_class>* ClassTable::get_method_table(Symbol name) {
    return global_method_table[name];
}

method_class* ClassTable::get_method_class(Symbol class_name, Symbol method_name) {
    return global_method_table[class_name]->lookup(method_name);
}

ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    /* Fill this in */
    install_basic_classes();
    add_class_nodes(classes);
    set_uids();
    check_parent_vaild();
    check_cycle();
}

void ClassTable::add_class_nodes(Classes classes) {
    for (auto i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ class_node = classes->nth(i);
        Symbol name = class_node->get_name();
        Symbol parent = class_node->get_parent();

        if (name == Int || name == Bool || name == Str || name == Object || name == IO || name == SELF_TYPE) {
            semant_error(class_node) << "Redefinition of basic class " << name << ".\n";
        }
        else {
            if(graph.find(name) == graph.end()) {
                graph[name] = class_node;
            }
            else {
                semant_error(class_node) << "Class " << name << " was previously defined.\n";
            }
        }
    }
}

void ClassTable::add_formals(Formals formals, SymbolTable<Symbol, attr_class> *attr_table, Symbol filename, tree_node *t) {
    std::vector<Symbol> param_vec;
    for(int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal formal = formals->nth(i);
        Symbol formal_type = formal->get_type();
        Symbol formal_name = formal->get_name();

        auto it = find(param_vec.begin(), param_vec.end(), formal_name);
        if (it == param_vec.end()) { //Not redundant
            param_vec.push_back(formal_name);
            attr_table->addid(formal_name, new attr_class(formal_name, formal_type, no_expr()));
        }
        else { //Redundant
            classtable->semant_error(filename, t) << "Formal parameter " << formal_name << " is multiply defined.\n";
        }
    }
}

void ClassTable::set_uids() {
    int i = 0;
    uid_table[No_class] = i++;
    for (auto iter = graph.begin(); iter != graph.end(); iter++) {
        Symbol name = iter->first;
        uid_table[name] = i++;
    }
}

void ClassTable::check_parent_vaild() {
    for (auto iter = graph.begin(); iter != graph.end(); iter++) {
        Symbol name = iter->first;
        Class_ class_node = iter->second;
        Symbol parent = class_node->get_parent();
        
        if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
            semant_error(class_node) << "Class " << name << " cannot inherit class " << parent << "." << endl;
        }    
        else if (parent != No_class && graph.find(parent) == graph.end()) {
            semant_error(class_node) << "Class " << name << " inherits from an undefined class " << parent << ".\n";
        } 
    }
}

void ClassTable::check_cycle() {
    // https://www.geeksforgeeks.org/detect-cycle-in-a-graph/

    int num = graph.size();
    bool *visited = new bool[num];
    bool *recStack = new bool[num];
    for(int i = 0; i < num; i++)
    {
        visited[i] = false;
        recStack[i] = false;
    }
 
    for(int i = 0; i < num; i++)
        if (check_cycle_util(i, visited, recStack)) {
            semant_error() << "The inheritance graph is not acyclic.\n";
            return;
        }
}

bool ClassTable::check_cycle_util(int v, bool visited[], bool *recStack)
{
    if(visited[v] == false)
    {
        visited[v] = true;
        recStack[v] = true;

        if(graph.find(find_symbol_by_uid(v)) != graph.end()) {
            int i = uid_table[graph[find_symbol_by_uid(v)]->get_parent()];
            if ( !visited[i] && check_cycle_util(i, visited, recStack) )
                return true;
            else if (recStack[i])
                return true;
        }
    }
    recStack[v] = false;
    return false;
}

Symbol ClassTable::find_symbol_by_uid(int uid) {
    for (auto it = uid_table.begin(); it != uid_table.end(); ++it)
        if (it->second == uid)
            return it->first;
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
    curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    graph.insert(std::make_pair(Object, Object_class));
    graph.insert(std::make_pair(IO, IO_class));
    graph.insert(std::make_pair(Int, Int_class));
    graph.insert(std::make_pair(Bool, Bool_class));
    graph.insert(std::make_pair(Str, Str_class));
}

////////////////////////////////////////////////////////////////////
//
//   Naming and Scoping!!
//
///////////////////////////////////////////////////////////////////

void ClassTable::traverse_gather_all_decls() {
    for (auto iter = graph.begin(); iter != graph.end(); iter++) { //traverse
        gather_all_decls(iter->second);
    }
    check_main_exist();
}

void ClassTable::check_main_exist() {
    if(graph[Main] == NULL) {
        semant_error() << "Class Main is not defined." << endl;
    }
}

void ClassTable::gather_all_decls(Class_ c) {
    Symbol name = c->get_name();
    SymbolTable<Symbol, attr_class> *attr_table = new SymbolTable<Symbol, attr_class>();
    SymbolTable<Symbol, method_class> *method_table = new SymbolTable<Symbol, method_class>();

    if(name != Object) gather_parent_decls(graph[c->get_parent()], attr_table, method_table);
    gather_my_decls(c, attr_table, method_table);

    global_attr_table[name] = attr_table;
    global_method_table[name] = method_table;
}

void ClassTable::gather_parent_decls(Class_ c, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if(c->get_name() == Object) {
        attr_table->enterscope();
        method_table->enterscope();
    }
    else {
        gather_parent_decls(graph[c->get_parent()], attr_table, method_table);
    }
    add_features(c, attr_table, method_table);
}

void ClassTable::gather_my_decls(Class_ c, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Symbol name = c->get_name();

    attr_table->enterscope();
    method_table->enterscope();
    add_not_error_features(c, attr_table, method_table);
    attr_table->addid(self, new attr_class(self, name, no_expr()));

    if(name == Main && method_table->lookup(main_meth) == NULL) {
        semant_error(c->get_filename(), c) << "Method main is not defined.\n";
    }
}

void ClassTable::add_features(Class_ c, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Features features = c->get_features();
    for(auto i = features->first(); features->more(i); i = features->next(i)) {
        Feature feature = features->nth(i);
        feature->add_to_table(attr_table, method_table);
    }
}

void ClassTable::add_not_error_features(Class_ c, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Features features = c->get_features();
    for(auto i = features->first(); features->more(i); i = features->next(i)) {
        Feature feature = features->nth(i);
        auto error = std::bind(&ClassTable::semant_error_callback, this, c->get_filename(), feature);
        feature->check_error(error);
        add_not_error_feature(c->get_filename(), feature, error, attr_table, method_table);
    }
}

void ClassTable::add_not_error_feature(
    Symbol filename,
    Feature feature,
    std::function<ostream&()> error,
    SymbolTable<Symbol, attr_class> *attr_table, 
    SymbolTable<Symbol, method_class> *method_table) {
    
    Symbol type = feature->get_type();
    if(!feature->check_redefined(error, attr_table, method_table))
        feature->add_to_table(attr_table, method_table);
}

void attr_class::check_error(std::function<ostream&()> error) {
    if(name == self) {
        error() << "'self' cannot be the name of an attribute.\n";
    }
}

void method_class::check_error(std::function<ostream&()> error) {

}

bool attr_class::check_redefined(std::function<ostream&()> error, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if(attr_table->lookup(name) != NULL) {
        error() << "Attribute " << name << " is an attribute of an inherited class.\n";
        return true;
    }
    return false;
}

bool method_class::check_redefined(std::function<ostream&()> error, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {    
    if(method_table->probe(name) != NULL) {
        error() << "Method " << name << " is redefined in the current class.\n";
        return true;
    }

    if(method_table->lookup(name) != NULL) {
        method_class* prev_method = method_table->lookup(name);
        Formals prev_formals = prev_method->get_formals();

        if(prev_method->get_type() != return_type) {
            error() << "In redefined method " << name << ", return type " << return_type 
                    << " is different from original return type " << prev_method->get_type() << ".\n";
            return true;
        }
        else if(prev_formals->len() != formals->len()) {
            error() << "Incompatible number of formal parameters in redefined method " << name << ".\n";
            return true;
        }
        else {
            for(int i = formals->first(); formals->more(i); i = formals->next(i)) {
                Formal formal = formals->nth(i);
                Formal prev_formal = prev_formals->nth(i);
                if(formal->get_type() != prev_formal->get_type()) {
                    error() << "In redefined method " << name << ", parameter type " << formal->get_type()
                    << " is different from original type " << prev_formal->get_type() <<"\n";
                    return true;
                }
            }
        }
    }

    for(auto i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal formal = formals->nth(i);
        if (formal->get_name() == self) {
            error() << "'self' cannot be the name of a formal parameter.\n";
            return true;
        }
        if (formal->get_type() == SELF_TYPE) {
            error() << "Formal parameter " << formal->get_name() << " cannot have type SELF_TYPE.\n";
            return true;
        }
    }

    return false;
}

void attr_class::add_to_table(SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    attr_table->addid(name, this);
}

void method_class::add_to_table(SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    method_table->addid(name, this);
}

////////////////////////////////////////////////////////////////////
//
//   Type check
//
///////////////////////////////////////////////////////////////////

void ClassTable::traverse_type_check_annotate(Classes classes) {
    for(auto i = classes->first(); classes->more(i); i = classes->next(i)) {
        Symbol name = classes->nth(i)->get_name();
        Class_ class_node = classes->nth(i);
        class_node->check_type_annotate(global_attr_table[name], global_method_table[name]);
    }
}

void class__class::check_type_annotate(SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    for(auto i = features->first(); features->more(i); i = features->next(i)) {
        features->nth(i)->check_type_annotate(this, attr_table, method_table);
    }
}

void attr_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    init = init->check_type_annotate(class_node, attr_table, method_table);
    if (init->get_type() != No_type){
        Symbol decl_type = type_decl == SELF_TYPE ? attr_table->lookup(self)->get_type() : type_decl;
        Symbol init_type = init->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : init->get_type();
        if (!classtable->is_subclass(init_type, decl_type)) {
            classtable->semant_error(class_node->get_filename(), this) 
                << " " << name << " " << decl_type << "  " << init_type << " .\n"; 
        }
    }
    else {
        init = init->set_type(No_type);
    }
}

void method_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    attr_table->enterscope();

    classtable->add_formals(formals, attr_table, class_node->get_filename(), this);
    
    if(!classtable->is_type_exist(return_type, class_node->get_filename(), this)) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Undefined return type " << return_type << " in method " << name << ".\n";
        expr = expr->check_type_annotate(class_node, attr_table, method_table);
    }
    else {
        expr = expr->check_type_annotate(class_node, attr_table, method_table);

        //For type check, convert
        Symbol expr_type = expr->get_type();
        Symbol decl_type = return_type;

        if (!classtable->is_subclass(expr_type, decl_type, attr_table->lookup(self)->get_type())) {
            classtable->semant_error(class_node->get_filename(), this) 
                << "Inferred return type " << expr_type << " of method " << name << " does not conform to declared return type " << decl_type << ".\n";
        }
    }
    attr_table->exitscope();
}

Expression assign_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type();
    Symbol decl_type = attr_table->lookup(name)->get_type();

    if(name == self) {
        classtable->semant_error(class_node->get_filename(), this) << "Cannot assign to 'self'.\n";
        return set_type(idtable.add_string(expr_type->get_string()));    
    }

    if (!classtable->is_subclass(expr_type, decl_type)) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Type " << expr_type << " of assigned expression does not conform to declared type " << decl_type << " of identifier " << name << ".\n";
        return set_type(Object);
    }
    else {
        return set_type(idtable.add_string(expr_type->get_string()));
    }
}

Expression let_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (identifier == self) {
        classtable->semant_error(class_node->get_filename(), this) << "'self' cannot be bound in a 'let' expression.\n";
        return set_type(idtable.add_string(type_decl->get_string()));
    }

    init = init->check_type_annotate(class_node, attr_table, method_table);
    if (init->get_type() != No_type){
        Symbol decl_type = type_decl == SELF_TYPE ? attr_table->lookup(self)->get_type() : type_decl;
        Symbol init_type = init->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : init->get_type();
        if (!classtable->is_subclass(init_type, decl_type))
            classtable->semant_error(class_node->get_filename(), this) 
                << "Inferred type " << init_type << " of initialization of " << identifier << " does not conform to identifier's declared type " << decl_type << ".\n";
    }
    else {
        init = init->set_type(No_type);
    }
    
    attr_table->enterscope();
    attr_table->addid(identifier, new attr_class(identifier, type_decl, init));
    body = body->check_type_annotate(class_node, attr_table, method_table);
    attr_table->exitscope();
    return set_type(idtable.add_string(body->get_type()->get_string()));
}

Expression static_dispatch_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (!classtable->is_type_exist(type_name, class_node->get_filename(), this)) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Dispatch on undefined class " << type_name << ".\n";
        return set_type(Object);
    }
    if (!classtable->is_method_exist(type_name, name, class_node->get_filename(), this)) {
        classtable->semant_error(class_node->get_filename(), this)  << "Dispatch to undefined method " << name << ".\n";
        return set_type(Object);
    }
    
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type();
    if (!classtable->is_subclass(expr_type, type_name)) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Expression type " << expr_type << " does not conform to declared static dispatch type " << type_name <<".\n";
        return set_type(Object);
    }

    Formals original_formals = classtable->get_method_class(type_name, name)->get_formals();
    if(original_formals->len() != actual->len()) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Method " << name << " called with wrong number of arguments." << "\n";
        return set_type(Object);
    }

    for(auto i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression expr = actual->nth(i)->check_type_annotate(class_node, attr_table, method_table);
        Symbol expr_type = expr->get_type();
        Symbol decl_type = original_formals->nth(i)->get_type();
        Symbol formal_name = original_formals->nth(i)->get_name();
        if(!classtable->is_subclass(expr_type, decl_type, attr_table->lookup(self)->get_type())) {
            classtable->semant_error(class_node->get_filename(), expr) 
                << "In call of method " << name << ", " << "type " << expr_type << " of parameter "
                << formal_name << " does not conform to declared type " << decl_type << ".\n";
            expr = expr->set_type(Object);
        }
    }
    Symbol return_type = classtable->get_method_class(type_name, name)->get_type();    
    if(type_name == SELF_TYPE) {
        return set_type(idtable.add_string(return_type->get_string()));
    }
    else {
        return return_type == SELF_TYPE ? 
            set_type(idtable.add_string(expr_type->get_string())) : 
            set_type(idtable.add_string(return_type->get_string()));
    }
    
}

Expression dispatch_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {    
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : expr->get_type();

    if (!classtable->is_type_exist(expr_type, class_node->get_filename(), this)) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Dispatch on undefined class " << expr_type << ".\n";
        return set_type(Object);
    }
    if (!classtable->is_method_exist(expr_type, name, class_node->get_filename(), this)) {
        classtable->semant_error(class_node->get_filename(), this)  << "Dispatch to undefined method " << name << ".\n";
        return set_type(Object);
    }

    Formals original_formals = classtable->get_method_class(expr_type, name)->get_formals();
    if(original_formals->len() != actual->len()) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Method " << name << " called with wrong number of arguments." << "\n";
        return set_type(Object);
    }
    for(auto i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression expr = actual->nth(i)->check_type_annotate(class_node, attr_table, method_table);
        Symbol expr_type = expr->get_type();
        Symbol decl_type = original_formals->nth(i)->get_type();
        Symbol formal_name = original_formals->nth(i)->get_name();
        if(!classtable->is_subclass(expr_type, decl_type, attr_table->lookup(self)->get_type())) {
            classtable->semant_error(class_node->get_filename(), expr) 
                << "In call of method " << name << ", " << "type " << expr_type << " of parameter "
                << formal_name << " does not conform to declared type " << decl_type << ".\n";
            expr = expr->set_type(Object);
        }
    }
    Symbol return_type = classtable->get_method_class(expr_type, name)->get_type();
    if(expr->check_type_annotate(class_node, attr_table, method_table)->get_type() == SELF_TYPE) {
        return set_type(idtable.add_string(return_type->get_string()));
    }
    else {
        return return_type == SELF_TYPE ? 
            set_type(idtable.add_string(expr_type->get_string())) : 
            set_type(idtable.add_string(return_type->get_string()));
    }
}

Expression cond_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    pred = pred->check_type_annotate(class_node, attr_table, method_table);
    then_exp = then_exp->check_type_annotate(class_node, attr_table, method_table);
    else_exp = else_exp->check_type_annotate(class_node, attr_table, method_table);
    if (pred->get_type() != Bool) {
        classtable->semant_error(class_node->get_filename(), this) << "Predicate of 'if' does not have type Bool.\n";
    }
    else {
        if(then_exp->get_type() == SELF_TYPE && else_exp->get_type() == SELF_TYPE) {
            return set_type(SELF_TYPE);
        }
        else {
            Symbol lub_type = classtable->get_lub(
                then_exp->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : then_exp->get_type(), 
                else_exp->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : else_exp->get_type());
            
            return set_type(idtable.add_string(lub_type->get_string()));
        }
    }
}

Expression typcase_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    attr_table->enterscope();

    //No same type case
    Cases new_cases;
    Symbol closest_ancestor;
    std::vector<Symbol> type_vec;
    for (auto i = cases->first(); cases->more(i); i = cases->next(i)) {
        Case case_ = cases->nth(i);
        Symbol name = case_->get_name();
        Symbol case_type = case_->get_type();
        Expression expr;

        if(name == self) {
            classtable->semant_error(class_node->get_filename(), this) << "self is not allowed to appear in a case binding.\n";
            return set_type(Object);
        }
  
        auto it = find(type_vec.begin(), type_vec.end(), case_type);
        if (it == type_vec.end()) { //Not redundant
            type_vec.push_back(case_type);
        }
        else { //Redundant
            classtable->semant_error(class_node->get_filename(), this) << " Duplicate branch " << case_type << " in case statement.\n";
            return set_type(Object);
        }

        if(!classtable->is_type_exist(case_type, class_node->get_filename(), this)){
            classtable->semant_error(class_node->get_filename(), case_) << "Class " << case_type << " of case branch is undefined.\n";
        }
        attr_table->enterscope();
        attr_table->addid(name, new attr_class(name, case_type, case_->get_expr()));
        expr = case_->get_expr()->check_type_annotate(class_node, attr_table, method_table);
        attr_table->exitscope();

        if (i == cases->first()) {
            closest_ancestor = expr->get_type();
        }
        else {
            closest_ancestor = classtable->get_lub(closest_ancestor, expr->get_type());
        }
    }
    return set_type(idtable.add_string(closest_ancestor->get_string()));
}

Expression loop_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    pred = pred->check_type_annotate(class_node, attr_table, method_table);
    body = body->check_type_annotate(class_node, attr_table, method_table);
    if (pred->get_type() != Bool) {
        classtable->semant_error(class_node->get_filename(), this) << "Loop condition does not have type Bool.\n"; 
    }
    return set_type(Object);
}

Expression block_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Expression expr = no_expr();
    for (auto i = body->first(); body->more(i); i = body->next(i)) {
        expr = body->nth(i)->check_type_annotate(class_node, attr_table, method_table);
    }
    return set_type(idtable.add_string(expr->get_type()->get_string()));
}

Expression plus_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " + " << e2->get_type() << "\n";
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " + " << e2->get_type() << "\n";
    }
    return set_type(Int);
}

Expression sub_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " - " << e2->get_type() << "\n";
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " - " << e2->get_type() << "\n";
    }
    return set_type(Int);
}

Expression mul_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " * " << e2->get_type() << "\n";
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " * " << e2->get_type() << "\n";
    }
    return set_type(Int);
}

Expression divide_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " / " << e2->get_type() << "\n";
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " / " << e2->get_type() << "\n";
    }
    return set_type(Int);
}

Expression neg_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Argument of '~' has type " << e1->get_type() << " instead of Int.\n";
    }
    return set_type(Int);
}

Expression lt_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " < " << e2->get_type() << "\n";
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " < " << e2->get_type() << "\n";
    }
    return set_type(Bool);
}

Expression eq_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);
    
    Symbol e1_type = e1->get_type();
    Symbol e2_type = e2->get_type();

    if (e1_type == Int || e2_type == Int || e1_type == Bool || e2_type == Bool || e1_type == Str || e2_type == Str) {
        if(e1_type != e2_type) {
            classtable->semant_error(class_node->get_filename(), this) << "Illegal comparison with a basic type.\n";
        }
    }

    return set_type(Bool);
}

Expression leq_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " <= " << e2->get_type() << "\n";
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "non-Int arguments: " << e1->get_type() << " <= " << e2->get_type() << "\n";
    }
    return set_type(Bool);
}

Expression comp_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    if (e1->get_type() != Bool) {
        classtable->semant_error(class_node->get_filename(), this) << "Argument of 'not' has type " << e1->get_type() << " instead of Bool.\n";
    }
    return set_type(Bool);
}

Expression int_const_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(Int);
}

Expression bool_const_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(Bool);
}

Expression string_const_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(Str);
}
Expression isvoid_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    return set_type(Bool);
}

Expression no_expr_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(No_type);
}

Expression object_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if(attr_table->lookup(name) == NULL) {
        classtable->semant_error(class_node->get_filename(), this) << "Undeclared identifier " <<  name << ".\n";
        return set_type(Object);
    }
    else if(name == self) {
        return set_type(SELF_TYPE);
    }
    else {
        return set_type(idtable.add_string(attr_table->lookup(name)->get_type()->get_string()));
    }
}

Expression new__class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (!classtable->is_type_exist(type_name, class_node->get_filename(), this)) {
        classtable->semant_error(class_node->get_filename(), this) << "'new' used with undefined class " <<  type_name << ".\n";
        return set_type(Object);
    }
    else 
        return set_type(idtable.add_string(type_name->get_string()));
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error_callback(Symbol filename, tree_node *t)
{
    return semant_error(filename, t);
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 

/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    classtable = new ClassTable(classes);
    classtable->halt();

    /* some semantic analysis code may go here */
    classtable->traverse_gather_all_decls();
    classtable->traverse_type_check_annotate(classes);
    classtable->halt();
}


