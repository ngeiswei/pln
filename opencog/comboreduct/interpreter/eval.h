/*
 * opencog/comboreduct/combo/eval.h
 *
 * Copyright (C) 2002-2008 Novamente LLC
 * All Rights Reserved
 *
 * Written by Nil Geisweiller
 *            Moshe Looks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _COMBO_EVAL_H
#define _COMBO_EVAL_H

#include <exception>
#include <boost/unordered_map.hpp>

#include <opencog/util/tree.h>
#include <opencog/util/numeric.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/mt19937ar.h>

#include "../combo/vertex.h"
#include "../combo/using.h"
#include "../combo/variable_unifier.h"
#include "../crutil/exception.h"
#include "../type_checker/type_tree.h"

namespace opencog { namespace combo {

struct Evaluator {
    virtual ~Evaluator() { }
    virtual vertex eval_percept(combo_tree::iterator, variable_unifier&) = 0;
    // eval_indefinite_object takes no arguments because it is assumed
    // that it has no child, this assumption may change over time
    virtual vertex eval_indefinite_object(indefinite_object,
                                          variable_unifier&) = 0;
};

// it has this name because it evaluates a procedure and returns a tree
combo_tree eval_procedure_tree(const vertex_seq& bmap, combo_tree::iterator it, Evaluator* pe);


#define ALMOST_DEAD_EVAL_CODE 1
#if ALMOST_DEAD_EVAL_CODE
/// @todo all users of the code below should switch to using
/// eval_throws_binding() instead.

// Used by Embodiment. Previously supported a tacky variable unification system, but now just calls the normal evaluator.
template<typename It>
vertex eval_throws(It it, Evaluator* pe = NULL,
                   combo::variable_unifier& vu = combo::variable_unifier::DEFAULT_VU())
    throw(EvalException, ComboException,
          AssertionException, std::bad_exception)
{

    vertex_seq empty;
    return eval_throws_binding(empty, it, pe);
}

template<typename It>
vertex eval(It it)
     throw(ComboException,
           AssertionException, std::bad_exception)
{
    try {
        return eval_throws(it);
    } catch (EvalException e) {
        return e.get_vertex();
    }
}

template<typename T>
vertex eval(const tree<T>& tr)
     throw(StandardException, std::bad_exception)
{
    return eval(tr.begin());
}

template<typename T>
vertex eval_throws(const tree<T>& tr)
     throw(EvalException,
           ComboException,
           AssertionException,
           std::bad_exception)
{
    return eval_throws(tr.begin());
}
#endif /* ALMOST_DEAD_EVAL_CODE */

/// eval_throws_binding -- evaluate a combo tree, using the argument
/// values supplied in the vertex_seq list.
///
/// This proceedure does not do any type-checking; the static type-checker
/// should be used for this purpose.
/// The Evaluator is currently unused; we're waiting for variable unification
/// to be made obsolete (!?)
vertex eval_throws_binding(const vertex_seq& bmap,
                           combo_tree::iterator it, Evaluator* pe = NULL)
    throw(EvalException, ComboException,
          AssertionException, std::bad_exception);

vertex eval_throws_vertex(const vertex_seq& bmap,
                           combo_tree::iterator it, Evaluator* pe = NULL)
    throw(EvalException, ComboException,
          AssertionException, std::bad_exception);

vertex eval_throws_binding(const vertex_seq& bmap, const combo_tree& tr)
     throw(EvalException, ComboException, AssertionException,
           std::bad_exception);

// As above, but returns combo tree instead ov vertex.  The above,
// non-tree variants cannot be used when using lists, since the only
// way to represent a list is as a tree.
combo_tree eval_throws_tree(const vertex_seq& bmap, const combo_tree& tr)
     throw(EvalException, ComboException, AssertionException,
           std::bad_exception);

combo_tree eval_throws_tree(const vertex_seq& bmap,
                           combo_tree::iterator it, Evaluator* pe = NULL)
     throw(EvalException, ComboException, AssertionException,
           std::bad_exception);

// As above, but EvalException is never thrown.
vertex eval_binding(const vertex_seq& bmap, combo_tree::iterator it)
    throw(ComboException, AssertionException, std::bad_exception);

vertex eval_binding(const vertex_seq& bmap, const combo_tree& tr)
    throw(StandardException, std::bad_exception);

// return the arity of a tree
template<typename T>
arity_t arity(const tree<T>& tr)
{
    arity_t a = 0;
    for (typename tree<T>::iterator it = tr.begin();
         it != tr.end(); ++it)
        if (is_argument(*it))
            a = std::max(a, (arity_t)std::abs(get_argument(*it).idx));
    return a;
}

}} // ~namespaces combo opencog

#endif
