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

#ifndef CFG_H
#define CFG_H

#include <map>
#include <string>

class cfg
{
public:
    //! everything below is considered silence
    float silence_lvl = 0.01f;
    //! longer audio is considere no spike, but "useful"
    float max_time_spike = 0.05f;
    //! for spike detection: shorter audio is no word
    float min_time_word_spike = 0.0f;
    //! for spike detection:  max time a word can be idle - longer idle time splits words
    float max_time_idle_spike = 0.1f;
    //! shorter audio is no word
    float min_time_word = 0.1f;
    //! max time a word can be idle - longer idle time splits words
    float max_time_idle = 0.6f;

    float fade_in_time = 0.05f;
    float fade_out_time = 0.05f;

    void read_from(const std::string& fname);

private:
    std::map<std::string, float&> float_map
    = {
        {"silence_lvl", silence_lvl},
        {"max_time_spike", max_time_spike},
        {"min_time_word_spike", min_time_word_spike},
        {"max_time_idle_spike", max_time_idle_spike},
        {"min_time_word", min_time_word},
        {"max_time_idle", max_time_idle},
        {"fade_in_time", fade_in_time},
        {"fade_out_time", fade_out_time}
    };
};

#endif // CFG_H
