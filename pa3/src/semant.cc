

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
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

Symbol last_formal_type(Formals formals) {
    return formals->nth(formals->len() -1 )->get_type();
}

void ClassTable::halt() {
    if (errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}

bool ClassTable::is_subclass(Symbol a, Symbol b) {
    Class_ class_node = graph[a];
    if (class_node == NULL)
        return false;
    if (a == b)
        return true;
    return (is_subclass(class_node->get_parent(), b));
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
    if (graph.find(type) == graph.end() && type != prim_slot && type != SELF_TYPE) { 
        semant_error(filename, t) << "Type " << type << " is not defined.\n";
        return false;
    }
    return true;
}

bool ClassTable::is_method_exist(Symbol class_name, Symbol method_name, Symbol filename, tree_node *t) {
    if(global_method_table[class_name]->lookup(method_name) == NULL) {
        semant_error(filename, t) << "Dispatch to undefined method " << method_name << ".\n";
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
    check_parent_exist();
    check_cycle();
}

void ClassTable::add_class_nodes(Classes classes) {
    for (auto i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ class_node = classes->nth(i);
        Symbol name = class_node->get_name();
        Symbol parent = class_node->get_parent();
        
        if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
            semant_error(class_node) << "Class " << name << " cannot inherit class " << parent << "." << endl;
        }

        if(graph.find(name) == graph.end()) {
            graph[name] = class_node;
        }
        else {
            semant_error(class_node) << "Class " << name << " is defined multiple times.\n";
        }
    }
}

void ClassTable::add_formals(Formals formals, SymbolTable<Symbol, attr_class> *attr_table) {
    for(int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal formal = formals->nth(i);
        Symbol formal_type = formal->get_type();
        Symbol formal_name = formal->get_name();
        attr_table->addid(formal_name, new attr_class(formal_name, formal_type, no_expr()));
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

void ClassTable::check_parent_exist() {
    for (auto iter = graph.begin(); iter != graph.end(); iter++) {
        Symbol name = iter->first;
        Class_ class_node = iter->second;
        Symbol parent = class_node->get_parent();
        
        if (parent != No_class && graph.find(parent) == graph.end()) { //Except object class
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
    add_not_error_features(c, attr_table, method_table);
}

void ClassTable::gather_my_decls(Class_ c, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Symbol name = c->get_name();

    attr_table->enterscope();
    method_table->enterscope();
    add_not_error_features(c, attr_table, method_table);
    attr_table->addid(self, new attr_class(self, name, no_expr()));

    if (name == Main) {
        if (method_table->probe(main_meth) == NULL)
            semant_error(c->get_filename(), c) << "Method main is not defined.\n";
        else if (method_table->probe(main_meth)->get_formals()->len() != 0)
            semant_error(c->get_filename(), c) << "Method main should take no formal parameters.\n";
    }
}

void ClassTable::add_not_error_features(Class_ c, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Features features = c->get_features();
    for(auto i = features->first(); features->more(i); i = features->next(i)) {
        Feature feature = features->nth(i);
        auto error = std::bind(&ClassTable::semant_error_callback, this, c->get_filename(), feature);
        feature->check_error(error); //TODO : 순서
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
    if (!is_type_exist(type, filename, feature)) {
    }
    else {
        if(!feature->check_redefined(error, attr_table, method_table))
            feature->add_to_table(attr_table, method_table);
    }
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
        error() << "Attribute " << name << " is defined multiple times.\n";
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
        //TODO: 개량
    }

    for(auto i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal formal = formals->nth(i);
        if (formal->get_name() == self) {
            error() << "self is not allowed to appear in a formal binding.\n";
            return true;
        }
        if (formal->get_type() == SELF_TYPE) {
            error() << "SELF_TYPE is not allowed to appear in a formal binding.\n";
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
        if (!classtable->is_subclass(init_type, decl_type))
            classtable->semant_error(class_node->get_filename(), this) << "Identifier " << name << " declared type " << decl_type << " but assigned type " << init_type << " .\n";
    }
    else {
        init = init->set_type(type_decl);
    }
}

void method_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    attr_table->enterscope();

    classtable->add_formals(formals, attr_table);
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : expr->get_type();
    Symbol decl_type = return_type == SELF_TYPE ? attr_table->lookup(self)->get_type() : return_type;

    if (!classtable->is_subclass(expr_type, decl_type))
         classtable->semant_error(class_node->get_filename(), this) << "The declared return type of method " << name << " is " << decl_type << " but the type of the method body is " << expr_type << " .\n";
    
    attr_table->exitscope();
}

Expression assign_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type();
    Symbol decl_type = attr_table->lookup(name)->get_type();

    if (classtable->is_subclass(expr_type, decl_type)) {
        return set_type(expr_type);
    }
    else {
        classtable->semant_error(class_node->get_filename(), this) << "Identifier " << name << " declared type " << decl_type << " but assigned type " << expr_type << " .\n";
        return set_type(Object);
    }
}

Expression static_dispatch_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (!classtable->is_type_exist(type_name, class_node->get_filename(), this) || 
        !classtable->is_method_exist(type_name, name, class_node->get_filename(), this)) {
        return set_type(Object);
    }    
    
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : expr->get_type();
    if (!classtable->is_subclass(expr_type, type_name))
        classtable->semant_error(class_node->get_filename(), this) << "The caller is not of type " << type_name << ".\n";


    Formals original_formals = classtable->get_method_class(type_name, name)->get_formals();
    if(original_formals->len() != actual->len()) {
        classtable->semant_error(class_node->get_filename(), this) << "Number of parameters is not the same of declared.\n";
        return set_type(Object);
    }

    Expressions new_actual = nil_Expressions();
    for(auto i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression expr = actual->nth(i)->check_type_annotate(class_node, attr_table, method_table);
        Symbol expr_type = expr->get_type();
        Symbol decl_type = original_formals->nth(i)->get_type();
        if(!classtable->is_subclass(expr_type, decl_type)) {
            classtable->semant_error(class_node->get_filename(), this) << "Parameters are not of the types declared.\n";
            expr = expr->set_type(Object); //TODO: WHY???
        }
        new_actual = (i == actual->first()) ? single_Expressions(expr) : append_Expressions(new_actual, single_Expressions(expr));
    }
    
    actual = new_actual;
    Symbol return_type = last_formal_type(original_formals);
    return return_type == SELF_TYPE ? set_type(expr_type) : set_type(return_type);
}

Expression dispatch_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (!classtable->is_method_exist(type_name, name, class_node->get_filename(), this)) {
        return set_type(Object);
    }    
    
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    Symbol expr_type = expr->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : expr->get_type();

    Formals original_formals = classtable->get_method_class(type_name, name)->get_formals();
    if(original_formals->len() != actual->len()) {
        classtable->semant_error(class_node->get_filename(), this) 
            << "Method " << name << " called with wrong number of arguments." << "\n";
        return set_type(Object);
    }

    Expressions new_actual = nil_Expressions();
    for(auto i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression expr = actual->nth(i)->check_type_annotate(class_node, attr_table, method_table);
        Symbol expr_type = expr->get_type();
        Symbol decl_type = original_formals->nth(i)->get_type();
        Symbol formal_name = original_formals->nth(i)->get_name();
        if(!classtable->is_subclass(expr_type, decl_type)) {
            classtable->semant_error(class_node->get_filename(), this) 
                << "In call of method " << name << ", " << "type " << expr_type << " of parameter "
                << formal_name << " does not conform to declared type " << decl_type << ".\n";
            expr = expr->set_type(Object);
        }
        new_actual = (i == actual->first()) ? single_Expressions(expr) : append_Expressions(new_actual, single_Expressions(expr));
    }
    
    actual = new_actual;
    Symbol return_type = last_formal_type(original_formals);
    return return_type == SELF_TYPE ? set_type(expr_type) : set_type(return_type);
}

Expression cond_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    pred = pred->check_type_annotate(class_node, attr_table, method_table);
    then_exp = then_exp->check_type_annotate(class_node, attr_table, method_table);
    else_exp = else_exp->check_type_annotate(class_node, attr_table, method_table);
    if (pred->get_type() != Bool) {
        classtable->semant_error(class_node->get_filename(), this) << "Predicate is not of type Bool.\n";
        return set_type(Object);
    }
    else {
        return set_type(classtable->get_lub(then_exp->get_type(), else_exp->get_type()));
    }
}

Expression loop_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    pred = pred->check_type_annotate(class_node, attr_table, method_table);
    body = body->check_type_annotate(class_node, attr_table, method_table);
    if (pred->get_type() != Bool) {
        classtable->semant_error(class_node->get_filename(), this) << "Predicate is not of type Bool.\n";
    }
    else {
        return set_type(Object);
    }
}

Expression typcase_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    expr = expr->check_type_annotate(class_node, attr_table, method_table);
    attr_table->enterscope();

    Expression expr_with_type;
    Symbol curr_type;
    Cases new_cases;
    Symbol final_type;
    List<Entry> *type_list = NULL;

    /*
    for (auto i = cases->first(); cases->more(i); i = cases->next(i)) {
        Case curr_case = cases->nth(i);
        if (curr_case->get_name() == self) {
            curr_classtable->semant_error(curr_class->get_filename(), this) << "self is not allowed to appear in a case binding.\n";
            return set_type(Object);
        }
        curr_type = curr_case->get_type();
        for (List<Entry> *l = type_list; l; l = l->tl()) {
            if (l->hd() == curr_type) {
                curr_classtable->semant_error(curr_class->get_filename(), this) << "Type " << curr_type << " appears multiple times.\n";
                return set_type(Object);
            }
        }
        type_list = new List<Entry>(curr_type, type_list);
        if (curr_classtable->no_type_error(curr_class->get_filename(), curr_type, curr_case))
            expr_with_type = curr_case->get_expr()->set_type(Object);
        else {
            attr_declarations->enterscope();
            attr_declarations->addid(curr_case->get_name(), curr_type);
            expr_with_type = curr_case->get_expr()->type_checking(attr_declarations, method_declarations, curr_classtable, curr_class);
            attr_declarations->exitscope();
        }
        if (i == cases->first()) {
            final_type = expr_with_type->get_type();
            new_cases = single_Cases(branch(curr_case->get_name(), curr_type, expr_with_type));
        }
        else {
            final_type = curr_classtable->lub(final_type, expr_with_type->get_type()); // Update final type (see README)
            new_cases = append_Cases(new_cases, single_Cases(branch(curr_case->get_name(), curr_type, expr_with_type)));
        }
    }
    cases = new_cases;
    return set_type(final_type);
    */
}

Expression block_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    Expression expr = no_expr();
    for (auto i = body->first(); body->more(i); i = body->next(i)) {
        Expression expr;
        expr = body->nth(i)->check_type_annotate(class_node, attr_table, method_table);
    }
    return set_type(expr->get_type());
}

Expression plus_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of + is not of type Int.\n";
        return set_type(Object);
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of + is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression sub_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of - is not of type Int.\n";
        return set_type(Object);
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of - is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression mul_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of * is not of type Int.\n";
        return set_type(Object);
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of * is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression divide_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of / is not of type Int.\n";
        return set_type(Object);
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of / is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression neg_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of ~ is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression lt_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of < is not of type Int.\n";
        return set_type(Object);
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of < is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression eq_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() == Int) {
        if (e2->get_type() != Int) {
            classtable->semant_error(class_node->get_filename(), this) << "Right part of = is not of type Int.\n";
            return set_type(Object);
        }
        else
            return set_type(Bool);
    }
    else if (e1->get_type() == Str) {
        if (e2->get_type() != Str) {
            classtable->semant_error(class_node->get_filename(), this) << "Right part of = is not of type Str.\n";
            return set_type(Object);
        }
        else
            return set_type(Bool);
    }
    else if (e1->get_type() == Bool) {
        if (e2->get_type() != Bool) {
            classtable->semant_error(class_node->get_filename(), this) << "Right part of = is not of type Bool.\n";
            return set_type(Object);
        }
        else
            return set_type(Bool);
    }
    else {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of = is not of type Int or Bool or String.\n";
        return set_type(Object);
    }
}

Expression leq_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    e2 = e2->check_type_annotate(class_node, attr_table, method_table);

    if (e1->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Left part of <= is not of type Int.\n";
        return set_type(Object);
    }
    if (e2->get_type() != Int) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of <= is not of type Int.\n";
        return set_type(Object);
    }
    return set_type(Int);
}

Expression comp_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    e1 = e1->check_type_annotate(class_node, attr_table, method_table);
    if (e1->get_type() != Bool) {
        classtable->semant_error(class_node->get_filename(), this) << "Right part of \"not\" is not of type Int.\n";
        return set_type(Object);
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

Expression new__class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (!classtable->is_type_exist(class_node->get_filename(), type_name, this)) 
        return set_type(Object);
    else 
        return set_type(type_name);
}

Expression isvoid_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(Bool);
}

Expression no_expr_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(No_type);
}

Expression object_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    return set_type(attr_table->lookup(name)->get_type());
}

Expression let_class::check_type_annotate(Class_ class_node, SymbolTable<Symbol, attr_class> *attr_table, SymbolTable<Symbol, method_class> *method_table) {
    if (identifier == self) {
        classtable->semant_error(class_node->get_filename(), this) << "self is not allowed to appear in a let binding.\n";
        return set_type(Object);
    }

    init = init->check_type_annotate(class_node, attr_table, method_table);
    if (init->get_type() != No_type){
        Symbol decl_type = type_decl == SELF_TYPE ? attr_table->lookup(self)->get_type() : type_decl;
        Symbol init_type = init->get_type() == SELF_TYPE ? attr_table->lookup(self)->get_type() : init->get_type();
        if (!classtable->is_subclass(init_type, decl_type))
            classtable->semant_error(class_node->get_filename(), this) << "Identifier " << identifier << " declared type " << decl_type << " but assigned type " << init_type << " .\n";
    }
    else {
        init = init->set_type(type_decl);
    }
    
    attr_table->enterscope();
    attr_table->addid(identifier, new attr_class(identifier, type_decl, init));
    body = body->check_type_annotate(class_node, attr_table, method_table);
    attr_table->exitscope();
    return set_type(body->get_type());
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


