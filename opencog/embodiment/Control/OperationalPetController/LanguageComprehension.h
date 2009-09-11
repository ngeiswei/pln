/*
 * opencog/embodiment/Control/OperationalPetController/LanguageComprehension.h
 *
 * Copyright (C) 2009 Novamente LLC
 * All Rights Reserved
 * Author(s): Samir Araujo
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

#ifndef LANGUAGECOMPREHENSION_H
#define LANGUAGECOMPREHENSION_H

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/embodiment/Control/PetInterface.h>
#include "OutputRelex.h"
#include "FramesToRelexRuleEngine.h"


namespace OperationalPetController
{
    
    class LanguageComprehension 
    {
    public: 
        LanguageComprehension( Control::PetInterface& agent );
        
        virtual ~LanguageComprehension( void );        

        void solveLatestSentenceReference( void );

        void answerLatestQuestion( void );

        void solveLatestSentenceCommand( void );

        std::string resolveFrames2Relex( );
    protected:

        void init(void);

//        opencog::SchemeEval evaluator;
        Control::PetInterface& agent;
        std::string nlgen_server_host;
        int nlgen_server_port;
        FramesToRelexRuleEngine framesToRelexRuleEngine;
    private:

    };

};






#endif // LANGUAGECOMPREHENSION_H
