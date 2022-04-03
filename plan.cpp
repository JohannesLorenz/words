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

#include <fstream>
#include <limits>
#include <regex>
#include <iostream>  // TODO
#include <sys/stat.h>
#include <unistd.h>

#include "plan.h"






plan_t::plan_t()
{

}

void plan_t::read_from(const std::string &fname)
{
	{
		std::string line;
		std::size_t cur_caption;
		std::ifstream cfg_file(fname);
		if(!cfg_file.good())
		{
			throw std::runtime_error("Cannot open file: " + fname);
		}
			const char* x = "";
		std::regex
			re_blankline	(R"XXX(^\s*$)XXX",
					std::regex::optimize),
			re_break	(R"XXX(^\s*\\(n)?break\s*([0-9]+(\.[0-9]+)?)\s*$)XXX",
					std::regex::optimize),
			re_follow	(R"XXX(^\s*\\follow\s*([0-9]+)\s*$)XXX",
					std::regex::optimize),
			re_caption	(R"XXX(^\s*#\s*(\S+)\s*$)XXX",
					std::regex::optimize),
			re_words	(R"XXX(^(\s*(([^:]+):)?([0-9]+)|\s*\\br\s*([0-9]+(\.[0-9]+)?))+\s*$)XXX",
					std::regex::optimize),
			re_word		(R"XXX(^\s*((([^:]+):)?([0-9]+)|\s*\\br\s*([0-9]+(\.[0-9]+)?)))XXX",
					std::regex::optimize);

		std::string last_prefix;
		for(std::size_t line_no = 1; std::getline(cfg_file, line); ++line_no)
		{
			std::smatch match;
			if(std::regex_match(line, match, re_blankline))
			{
			}
			else if(std::regex_match(line, match, re_break))
			{
				if(match.length(1))
					def_nbreak = std::stof(match.str(2));
				else
					def_break = std::stof(match.str(2));

			}
			else if(std::regex_match(line, match, re_follow))
			{
				def_follow = std::stof(match.str(1));
			}
			else if(std::regex_match(line, match, re_caption))
			{
				last_prefix = match.str(1);
				if(last_prefix[last_prefix.length()-1] != '/')
				{
					struct stat st;
					if (lstat(last_prefix.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
					{
						last_prefix += '/';
					}
				}
			}
			else if(std::regex_match(line, match, re_words))
			{
				std::string::const_iterator next( line.cbegin() );
				while ( regex_search( next, line.cend(), match, re_word) )
				{
					if(match.length(4))
					{
						std::string path = last_prefix + match.str(3);
						struct stat st;
						if (lstat(path.c_str(), &st) == 0 &&
							S_ISREG(st.st_mode) &&
							0 == access(path.c_str(), R_OK))
						{
							std::size_t cur_file;
							auto itr = file_dict.find(path);
							if(itr == file_dict.end())
							{
								files.push_back(path);
								cur_file = files.size() - 1;
								file_dict.emplace(path, cur_file);
							}
							else
							 cur_file = itr->second;

							words.emplace_back(cur_file, match.str(4));
						}
						else
							throw std::runtime_error("File not found: " + path);
					}
					else
						words.emplace_back(std::numeric_limits<decltype(cur_caption)>::max(),
								match.str(5));
					next = match.suffix().first;
				}
			}
			else
			 throw std::runtime_error("Invalid line " + std::to_string(line_no) + ": " + line);
		}
	}
	for(const auto& pr : words)
	{
		std::string file = (pr.first == std::numeric_limits<std::size_t>::max())
				? "(break)"
				: files.at(pr.first);
		const std::string& word = pr.second;

		std::cout << file << ", " << word << std::endl;
	}

}

