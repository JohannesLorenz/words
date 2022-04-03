/*
** Copyright (C) 2022 Johannes Lorenz <mail_umleitung@web.de>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "cfg.h"

#include <fstream>
#include <regex>

void cfg::read_from(const std::string &fname)
{
	std::string line;
	{
		std::ifstream cfg_file(fname);
		if(!cfg_file.good())
		{
			throw std::runtime_error("Cannot open file: " + fname);
		}
		std::regex re(R"XXX(^(\s*(\S+)\s*=\s*([0-9]+\.[0-9]+))?\s*(#.*)?$)XXX",
				std::regex::optimize);
		while(std::getline(cfg_file, line))
		{
			std::smatch match;
			if(std::regex_match(line, match, re))
			{
				if(match.length(2))
				{
					std::string key = match.str(2);
					float value = std::stof(match.str(3));
					std::map<std::string, float&>::iterator itr = float_map.find(key);
					if(itr == float_map.end())
					 throw std::runtime_error("Key invalid: " + key);
					else
					 itr->second = value;
				}
			}
			else
			 throw std::runtime_error("Invalid line (should be \"key = X.Y\"): " + line);
		}
	}
}
