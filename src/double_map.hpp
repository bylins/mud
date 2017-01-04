/* ************************************************************************
*   File: double_map.hpp                                Part of Bylins    *
*  Usage: header file: simple double map                                  *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _DOUBLE_MAP_HPP_
#define _DOUBLE_MAP_HPP_

#include "conf.h"

#include <map>

template < typename _Key2, typename _Key1, typename _Tp2,
typename _Compare2 = std::less<_Key2>, typename _Compare1 = std::less<_Key1>,
typename _Alloc2 = std::allocator<std::pair<const _Key2, _Tp2> >,
typename _Alloc1 = std::allocator<std::pair<const _Key1, std::map<_Key2, _Tp2, _Compare2, _Alloc2> > > >
class double_map : public std::map<_Key1, std::map<_Key2, _Tp2, _Compare2, _Alloc2>, _Compare1, _Alloc1>
{
public:
	typedef std::map<_Key1, std::map<_Key2, _Tp2, _Compare2, _Alloc2>, _Compare1, _Alloc1> _Base;

	typedef typename _Base::key_type				key_type;
	typedef typename _Base::mapped_type				mapped_type;
	typedef typename _Base::value_type				value_type;
	typedef typename _Base::key_compare				key_compare;
	typedef typename _Base::allocator_type				allocator_type;
	typedef typename _Base::reference				reference;
	typedef typename _Base::const_reference				const_reference;
	typedef typename _Base::iterator				iterator;
	typedef typename _Base::const_iterator				const_iterator;
	typedef typename _Base::size_type				size_type;
	typedef typename _Base::difference_type				difference_type;
	typedef typename _Base::pointer					pointer;
	typedef typename _Base::const_pointer				const_pointer;
	typedef typename _Base::reverse_iterator			reverse_iterator;
	typedef typename _Base::const_reverse_iterator			const_reverse_iterator;

	typedef typename _Base::mapped_type::key_type			key_type2;
#if defined(_MSC_VER) && (_MSC_VER < 1400)
	typedef _Tp2 mapped_type2;
#else
	typedef typename _Base::mapped_type::mapped_type		mapped_type2;
#endif
	typedef typename _Base::mapped_type::value_type			value_type2;
	typedef typename _Base::mapped_type::key_compare		key_compare2;
	typedef typename _Base::mapped_type::allocator_type		allocator_type2;
	typedef typename _Base::mapped_type::reference			reference2;
	typedef typename _Base::mapped_type::const_reference		const_reference2;
	typedef typename _Base::mapped_type::iterator			iterator2;
	typedef typename _Base::mapped_type::const_iterator		const_iterator2;
	typedef typename _Base::mapped_type::size_type			size_type2;
	typedef typename _Base::mapped_type::difference_type		difference_type2;
	typedef typename _Base::mapped_type::pointer			pointer2;
	typedef typename _Base::mapped_type::const_pointer		const_pointer2;
	typedef typename _Base::mapped_type::reverse_iterator		reverse_iterator2;
	typedef typename _Base::mapped_type::const_reverse_iterator	const_reverse_iterator2;

	using _Base::lower_bound;
	using _Base::end;
	using _Base::key_comp;
	using _Base::insert;
	using _Base::erase;

	void
	add_elem(key_type __k1, key_type2 __k2, mapped_type2 __info)
	{
		iterator __i = lower_bound(__k1);
		// __i->first is greater than or equivalent to __k1.
		if (__i == end() || key_comp()(__k1, (*__i).first))
			__i = insert(__i, value_type(__k1, mapped_type()));
		(*__i).second[__k2] = __info;
	}

	void
	del_elem(key_type __k1, key_type2 __k2)
	{
		iterator __i = lower_bound(__k1);
		// __i->first is greater than or equivalent to __k1.
		if (__i != end() && !key_comp()(__k1, (*__i).first))
		{
			(*__i).second.erase(__k2);
			if ((*__i).second.empty())
				erase(__i);
		}
	}
};

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
