/*
* Copyright (C) 2020 cen1
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#pragma once

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_PF
#define INCLUDED_PF

#define JUST_NEED_TYPES
#include "game.h"
#undef JUST_NEED_TYPES

#include <string>
#include "stdint.h"

namespace pvpgn
{
	namespace bnetd
	{
		extern uint16_t pf_get_port(std::string ip);
		extern void pf_replace_host(t_game * game, std::string realIp);
	}
}
#endif
#endif