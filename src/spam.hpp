// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SPAM_HPP_INCLUDED
#define SPAM_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace SpamSystem
{

const int MIN_OFFTOP_LVL = 6;
enum { OFFTOP_MODE };
bool check(CHAR_DATA *ch, int mode);

} // SpamSystem


#endif // SPAM_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
