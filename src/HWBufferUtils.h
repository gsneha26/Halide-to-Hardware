#ifndef HALIDE_HW_BUFFER_UTILS_H
#define HALIDE_HW_BUFFER_UTILS_H

/** \file
 *
 * Defines a general utility functions that are used in the
 * hardware passes.
 * 
 */

#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "IR.h"
#include "Scope.h"
#include "IRVisitor.h"

namespace Halide {
namespace Internal {

std::ostream& operator<<(std::ostream& os, const std::vector<Expr>& vec);
std::ostream& operator<<(std::ostream& os, const std::vector<int>& vec);
std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& vec);

int to_int(Expr expr);
int id_const_value(const Expr e);

std::vector<std::string> get_tokens(const std::string &line, const std::string &delimiter);

// return true if the for loop is not serialized
bool is_parallelized(const For *op);

std::string first_for_name(Stmt s);
bool contains_call(Stmt s, std::string var);

// return if provide creates the var at this level (not recursing into serial loops)
bool provide_at_level(Stmt s, std::string var = "");

// return if call uses the var at this level (not recursing into serial loops)
bool call_at_level(Stmt s, std::string var = "");

// Perform all the substitutions in a scope
Expr expand_expr(Expr e, const Scope<Expr> &scope);
Stmt expand_expr(Stmt e, const Scope<Expr> &scope);

// Produce a vector of the loops within a for-loop-nest.
//   Note: this can be used to find the streaming loops between the store and compute level.
std::vector<std::string> get_loop_levels_between(Stmt s, Function func,
                                                 LoopLevel outer_level_exclusive,
                                                 LoopLevel inner_level_inclusive,
                                                 bool start_inside = false);

Stmt substitute_in_constants(Stmt s);

template<typename D, typename R>
std::set<D> domain(const std::map<D, R>& m) {
  std::set<D> d;
  for (auto e : m) {
    d.insert(e.first);
  }
  return d;
}

class RealizeFinder : public IRGraphVisitor {
public:

  using IRGraphVisitor::visit;

  const Realize* r;
  std::string target;

  RealizeFinder(const std::string& target_) : r(nullptr), target(target_) {}

  void visit(const Realize* rl) override {
    std::cout << "Searching realize: " << rl->name << " for " << target << std::endl;
    if (rl->name == target) {
      r = rl;
    } else {
      rl->body->accept(this);
    }
  }
};

std::string exprString(const Expr e);

class VarSpec {
public:
  std::string name;
  Expr min;
  Expr extent;

  bool is_const() const {
    return name == "";
  }

  int const_value() const {
    internal_assert(is_const());
    return id_const_value(min);
  }
};

typedef std::vector<VarSpec> StmtSchedule;
bool operator==(const VarSpec& a, const VarSpec& b);
std::ostream& operator<<(std::ostream& out, const VarSpec& e);
std::ostream& operator<<(std::ostream& out, const StmtSchedule& s);

} // Internal
} // Halide

#endif
