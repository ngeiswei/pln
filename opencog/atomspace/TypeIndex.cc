/*
 * opencog/atomspace/TypeIndex.cc
 *
 * Copyright (C) 2008 Linas Vepstas <linasvepstas@gmail.com>
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

#include <opencog/atomspace/TypeIndex.h>
#include <opencog/atomspace/ClassServer.h>
#include <opencog/atomspace/HandleEntry.h>
#include <opencog/atomspace/TLB.h>
#include <opencog/atomspace/type_codes.h>

using namespace opencog;

TypeIndex::TypeIndex(void)
{
	// The typeIndex is NUMBER_OF_CLASSES+2 because NOTYPE is 
	// NUMBER_OF_CLASSES+1 and typeIndex[NOTYPE] is asked for if a
	// typename is misspelled, because ClassServer::getType()
	// returns NOTYPE in this case).
	resize(ClassServer::getNumberOfClasses() + 2);
}

void TypeIndex::insertHandle(Handle h)
{
	Atom *a = TLB::getAtom(h);
	Type t = a->getType();
	insert(t,h);
}

void TypeIndex::removeHandle(Handle h)
{
	Atom *a = TLB::getAtom(h);
	Type t = a->getType();
	remove(t,h);
}

HandleEntry * TypeIndex::getHandleSet(Type type, bool subclass) const
{
	iterator it = begin(type, subclass);
	iterator itend = end();

	HandleEntry *he = NULL;
	while (it != itend)
	{
		HandleEntry *nhe = new HandleEntry(*it);
		nhe->next = he;
		he = nhe;
		it++;
	}
	return he;
}

// ================================================================

TypeIndex::iterator TypeIndex::begin(Type t, bool sub) const
{
	iterator it(t, sub);
	it.send = idx.end();

	it.s = idx.begin();
	it.currtype = 0;
	while (it.s != it.send)
	{
		// Find the first type which is a subtype, and start iteration there.
		if ((it.type == it.currtype) || 
		    (sub && (ClassServer::isAssignableFrom(it.type, it.currtype))))
		{
			it.se = it.s->begin();
			if (it.se != it.s->end()) return it;
		}
		it.currtype++;
		it.s++;
	}

	return it;
}

TypeIndex::iterator TypeIndex::end(void) const
{
	iterator it(NOTYPE, false);
	it.se = idx.at(NOTYPE).end();
	it.s = idx.end();
	it.send = idx.end();
	it.currtype = NOTYPE;
	return it;
}

TypeIndex::iterator::iterator(Type t, bool sub)
{
	type = t;
	subclass = sub;
}

TypeIndex::iterator& TypeIndex::iterator::operator=(iterator v)
{
	s = v.s;
	send = v.send;
	se = v.se;
	currtype = v.currtype;
	type = v.type;
	subclass = v.subclass;
	return *this;
}

Handle TypeIndex::iterator::operator*(void)
{
	if (s == send) return Handle::UNDEFINED;
	return *se;
}

bool TypeIndex::iterator::operator==(iterator v)
{
	if ((v.s == v.send) && (s == send)) return true;
	return v.se == se;
}

bool TypeIndex::iterator::operator!=(iterator v)
{
	if ((v.s == v.send) && (s != send)) return v.se != se;
	if ((v.s != v.send) && (s == send)) return v.se != se;
	return false;
}

TypeIndex::iterator& TypeIndex::iterator::operator++(int i)
{
	if (s == send) return *this;

	se++;
	if (se == s->end())
	{
		do
		{
			s++;
			currtype++;

			// Find the first type which is a subtype, and start iteration there.
			if ((type == currtype) || 
			    (subclass && (ClassServer::isAssignableFrom(type, currtype))))
			{
				se = s->begin();
				if (se != s->end()) return *this;
			}
		} while (s != send);
	}

	return *this;
}

// ================================================================
