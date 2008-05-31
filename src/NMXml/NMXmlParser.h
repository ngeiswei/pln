/*
 * src/NMXml/NMXmlParser.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
 *
 * Written by Thiago Maia <thiago@vettatech.com>
 *            Andre Senna <senna@vettalabs.com>
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

#ifndef NMXMLPARSER_H
#define NMXMLPARSER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include <agents/AtomTableManagementAgent.h>
#include "utils.h"
#include "HandleMap.h"
#include "exceptions.h"
#include "XMLBufferReader.h"

#include <Link.h>
#include <Node.h>
#include <vector>
#include <AtomSpace.h>

#ifndef WIN32
#include <ext/hash_map>
#endif

enum NMXmlParseType { PARSE_NODES, PARSE_LINKS };

/**
 * This class implements an XML parser that reads XML files and inserts the
 * corresponding structure of nodes and links in the MindDB.
 */
class NMXmlParser
{
private:
    Handle parse_pass(XMLBufferReader*, NMXmlParseType);

public:

    //static Transaction *transaction;
    //AtomTable* atomTable;
    AtomSpace * atomSpace;

    static bool fresh;
    static bool freshLinks;

    /**
     * Store the name->handle mapping obtained during the first pass of
     * the parsing (when only Nodes are read) to be used in the second
     * pass (to resolve references in Links).
     */
    static Util::hash_map<char *, Handle, Util::hash<char *>, Util::eqstr> hypHandles;

    /**
     * A special default (Simple) TruthValue object for any atom loaded from a XML doc,
     * unless the xml specifies explicitly the mean (strength) and/or count (confidence)
     * using proper xml elements.
     * NOTE: this is actually the old DEFAULT TV returned by the former
     * static TruthValue::factoryDefaultTruthValue() method.
     */
    static const TruthValue& DEFAULT_TV();

    /**
     * Constructor for this class.
     */
    //NMXmlParser(Transaction *, bool = true, bool = false);
    //NMXmlParser(AtomTable*, bool = true, bool = false);
    NMXmlParser(AtomSpace*, bool = true, bool = false);

    /**
     * Destructor for this class.
     */
    ~NMXmlParser();

    /**
     * Parses a XML document.
     *
     * @param XMLBufferReader object that provides the bytes of a NM-xml document.
     * @param Parsing pass. If 1, the first pass is executed where all
     * nodes are read from XML. If 2, the second pass is executed where
     * all links are read from XML. The second pass can only be executed
     * after the first pass has been executed for all XML files in a set,
     * so that all nodes pointed to by links have been inserted.
     * @return the Handle of the last atomm inserted/merged into the atom table. Or UNDEFINED_HANDLE, if no atom was inserted/merged.
     */
    Handle parse(XMLBufferReader*, NMXmlParseType);

    /**
     * This method loads XML files, reading them and creating the
     * corresponding atom structure in the atom table.
     *
     * @param Vector of XMLBufferReader objects to get content to be read and parsed as XML.
     * @param Transaction to be used. Used in the case a user
     *        wants to use a registered Transaction.
     * @param boolean indicating whether the XML contains solely
     *        new Nodes. Note: Hypothetical links are always fresh.
    * @return a HandleEntry with the last outter link inserted/merged in the atom table for each parsed NM-xml.
     */
    //static HandleEntry* loadXML(const std::vector<XMLBufferReader*>&, AtomTable*,  bool = true, bool = false);
    static HandleEntry* loadXML(const std::vector<XMLBufferReader*>&, AtomSpace*,  bool = true, bool = false);

    /**
     * Sets a node name. This method exists because Parser is friends with
     * Node, and the standard C portions of the parser need to access
     * nodes directly.
     *
     * @param Node to have its name set.
     * @param Node name.
     */
    static void setNodeName(Node*, const char* name);

    /**
     * Adds a handle to the outgoing set of an atom . This method exists because
     * Parser is friends with Atom, and the standard C portions of the
     * parser need to access atoms directly.
     *
     * @param Atom to have its outgoing set changed.
     * @param Handle to be added to the outgoing set.
     */
    static void addOutgoingAtom(Link*, Handle);

    /**
      * Sets the outgoing set of the given atom using the given const reference to a vector of handles.
      * This method can be called only if the atom is not inserted in an AtomTable yet.
      * Otherwise, it throws a RuntimeException.
      */
    static void setOutgoingSet(Link*, const std::vector<Handle>&);

};

#endif //NMXMLPARSER_H
