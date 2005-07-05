/*************************************************************************
*   File: privileges.hpp                              Part of Bylins     *
*   Immortal privilege checking                                          *
*                                                                        *
*  $Author$                                                       * 
*  $Date$                                          *
*  $Revision$                                                      *
**************************************************************************/

#ifndef _PRIVILEGES_HPP_
#define _PRIVILEGES_HPP_

#include <stdio.h>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

typedef
 std::map < std::string, std::set < std::string > > PrivListType;

class PrivList {
      public:
	static const char *const
	 priv_file;

//constructors
	explicit PrivList();
//methods
	bool reload();
	bool save();

	bool enough_cmd_priv(const std::string & name, int char_level, const std::string & cmd_name, int cmd_number);
	bool enough_cmd_set_priv(const std::string & name, int char_level, const std::string & set_subcmd);
	bool enough_cmd_show_priv(const std::string & name, int char_level, const std::string & show_subcmd);
	bool add_cmd_priv(const std::string & char_name, const std::string & cmd_name);
	bool add_cmd_set_priv(const std::string & char_name, const std::string & set_subcmd);
	bool add_cmd_show_priv(const std::string & char_name, const std::string & show_subcmd);
	bool rm_cmd_priv(const std::string & char_name, const std::string & cmd_name);
	bool rm_cmd_set_priv(const std::string & char_name, const std::string & set_subcmd);
	bool rm_cmd_show_priv(const std::string & char_name, const std::string & show_subcmd);
	void
	 PrintPrivList(struct char_data *ch);
	void
	 PrintPrivList(struct char_data *ch, const std::string & char_name);

      private:

	PrivListType Priv_List;
	PrivListType Set_Priv_List;
	PrivListType Show_Priv_List;

};

#endif
