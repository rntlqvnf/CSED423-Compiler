#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"
#include <map>
#include <functional>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  std::map<Symbol, SymbolTable<Symbol, attr_class> *> global_attr_table;
  std::map<Symbol, SymbolTable<Symbol, method_class> *> global_method_table;
  std::map<Symbol, Class_> graph;
  std::map<Symbol, int> uid_table;
  int semant_errors;
  ostream& error_stream;

  void install_basic_classes();
  void add_class_nodes(Classes);
  void set_uids();
  void check_parent_exist();
  void check_cycle();
  bool check_cycle_util(int v, bool visited[], bool *recStack);
  Symbol find_symbol_by_uid(int);

  void add_not_error_features(Class_, SymbolTable<Symbol, attr_class> *, SymbolTable<Symbol, method_class> *);
  void add_not_error_feature(Symbol, Feature, std::function<ostream&()>,  SymbolTable<Symbol, attr_class> *, SymbolTable<Symbol, method_class> *);

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
  ostream& semant_error_callback(Symbol filename, tree_node *t);

  void halt();
  
  void gather_parent_decls(Class_, SymbolTable<Symbol, attr_class> *, SymbolTable<Symbol, method_class> *);
  void gather_my_decls(Class_, SymbolTable<Symbol, attr_class> *, SymbolTable<Symbol, method_class> *);
  void gather_all_decls(Class_);
  void traverse_gather_all_decls();
  void traverse_type_check_annotate(Classes);

  SymbolTable<Symbol, attr_class>* get_attr_table(Symbol);
  SymbolTable<Symbol, method_class>* get_method_table(Symbol);
  method_class* get_method_class(Symbol, Symbol);
  std::map<Symbol, Class_> get_graph();

  Symbol get_lub(Symbol, Symbol);
  bool is_subclass(Symbol, Symbol);
  bool is_type_exist(Symbol, Symbol, tree_node *);
  bool is_method_exist(Symbol, Symbol, Symbol, tree_node *);
  void add_formals(Formals, SymbolTable<Symbol, attr_class> *);
};


#endif

