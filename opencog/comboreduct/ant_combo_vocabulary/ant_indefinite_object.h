#ifndef _ANT_INDEFINITE_OBJECT_H
#define _ANT_INDEFINITE_OBJECT_H

#include <LADSUtil/numeric.h>

#include "ComboReduct/combo/indefinite_object.h"
#include "ComboReduct/ant_combo_vocabulary/ant_operator.h"

namespace combo {

  namespace id {
    enum ant_indefinite_object_enum {
      ant_indefinite_object_count
    };
  }

  typedef id::ant_indefinite_object_enum ant_indefinite_object_enum;

  /*********************************************************************
   *   Arrays containing indefinite_object name type and properties    *
   *                 to be edited by the developer                     *
   *********************************************************************/

  namespace ant_indefinite_object_properties {

    //struct for description of name and type
    typedef combo::ant_operator<ant_indefinite_object_enum, id::ant_indefinite_object_count>::basic_description indefinite_object_basic_description;

    //empty but kept for example
    static const indefinite_object_basic_description iobd[] = {
      //indefinite_object        name                   type
    };

  }//~namespace ant_perception_properties

  //ant_indefinite_object both derive
  //from indefinite_object_base and ant_operator
  class ant_indefinite_object : public ant_operator<ant_indefinite_object_enum, id::ant_indefinite_object_count>, public indefinite_object_base {

  private:

    //private methods

    //ctor
    ant_indefinite_object();

    const basic_description * get_basic_description_array() const;
    unsigned int get_basic_description_array_count() const;

    static const ant_indefinite_object* init_indefinite_object();
    void set_indefinite_object(ant_indefinite_object_enum);

  public:
    //name
    const std::string& get_name() const;

    //type_tree
    const type_tree& get_type_tree() const;

    //helper methods for fast access type properties
    //number of arguments that takes the operator
    arity_t arity() const;
    //return the type node of the operator
    type_tree get_output_type_tree() const;
    //return the type tree of the argument of index i
    //if the operator has arg_list(T) as last input argument
    //then it returns always T past that index
    const type_tree& get_input_type_tree(arity_t i) const;

    //return a pointer of the static action_symbol corresponding
    //to a given name string
    //if no such action_symbol exists then return NULL pointer
    static indefinite_object instance(const std::string& name);

    //return a pointer of the static pet_perception_action corresponding
    //to a given pet_perception_enum
    static indefinite_object instance(ant_indefinite_object_enum);

  };
}//~namespace combo

#endif
