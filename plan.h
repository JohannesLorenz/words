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

#ifndef PLAN_H
#define PLAN_H

#include <map>
#include <string>
#include <vector>

class plan_t
{
public:
    float def_break = 0.75f;
    float def_nbreak = 1.5f;
    int def_follow = 0;

    std::vector <std::string> files;
    std::map <std::string, std::size_t> file_dict;
    std::vector <std::pair<std::size_t, std::string>> words; // prefix + word

    plan_t();
    void read_from(const std::string& fname);

};

#endif // PLAN_H
