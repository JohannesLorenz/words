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

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <vector>

#include <sndfile.hh>

using frames_t = unsigned long;

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

	void read_from(const std::string& fname)
	{
		std::string line;
		{
			std::ifstream cfg_file(fname);
			if(!cfg_file.good())
			{
				throw std::runtime_error("Cannot open file: " + fname);
			}
			while(std::getline(cfg_file, line))
			{
				std::regex re(R"XXX(^(\s*(\S+)\s*=\s*([0-9]+\.[0-9]+))?\s*(#.*)?$)XXX",
					std::regex::optimize);
				std::smatch match;
				if(std::regex_search(line, match, re))
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
private:
	std::map<std::string, float&> float_map
	= {
		{ "silence_lvl", silence_lvl},
		{ "max_time_spike", max_time_spike},
		{ "min_time_word_spike", min_time_word_spike},
		{ "max_time_idle_spike", max_time_idle_spike},
		{ "min_time_word", min_time_word},
		{ "max_time_idle", max_time_idle},
	};
};

void dump_word(const cfg& , std::vector<std::array<short, 2>>& file_content, int word_no, const std::array<frames_t, 2>& start_and_length, int format, int samplerate, int channels)
{
/*	printf ("word %ld: %f (%f -> %f) seconds word at %ld (found at %ld)\n",
							words, (last_word_end-last_word_start)/(float)samplerate,
							last_word_start/(float)samplerate,
							last_word_end/(float)samplerate,
							(tell-idle_count)/samplerate, tell/samplerate);
	printf ("frames: %ld, %ld, idle_count: %ld\n",
		cur_frames_word, max_frames_idle, idle_count);
*/
	char pathname[256];
	snprintf(pathname, 256, "/tmp/words/%03d.wav", word_no);

	SndfileHandle outfile(pathname, SFM_WRITE, format, channels, samplerate) ;
	if(outfile.error() != SF_ERR_NO_ERROR)
		throw std::runtime_error("Error opening outfile");

	const frames_t written = outfile.writef(file_content[start_and_length[0]].data(), start_and_length[1]);
	if(written != start_and_length[1])
		throw std::runtime_error("Not enough bytes written");
}

void remove_spike(const cfg& config, std::vector<std::array<short, 2>>& file_content, int word_no, const std::array<frames_t, 2>& start_and_length, int format, int samplerate, int channels)
{
#if 0
/*	printf ("word %ld: %f (%f -> %f) seconds word at %ld (found at %ld)\n",
							words, (last_word_end-last_word_start)/(float)samplerate,
							last_word_start/(float)samplerate,
							last_word_end/(float)samplerate,
							(tell-idle_count)/samplerate, tell/samplerate);
	printf ("frames: %ld, %ld, idle_count: %ld\n",
		cur_frames_word, max_frames_idle, idle_count);
*/
	char pathname[256];
	snprintf(pathname, 256, "/tmp/words/%03d.wav", word_no);

	SndfileHandle outfile(pathname, SFM_WRITE, format, channels, samplerate) ;
	assert(outfile.error() == SF_ERR_NO_ERROR);

	const frames_t written = outfile.writef(cur_word.data()->data(), cur_word.size());
	assert(written == cur_word.size());
#endif
	frames_t max_spike_size = config.max_time_spike * samplerate;
	if(start_and_length[1] <= max_spike_size)
	{
		printf("Spike of %f s (%lu samples) at pos %f\n", start_and_length[1]/(float)samplerate, start_and_length[1], start_and_length[0]/(float)samplerate);
		for(int i = 0; i < (int)start_and_length[1]; ++i)
		{
			file_content[i+start_and_length[0]][0] = 0;
			file_content[i+start_and_length[0]][1] = 0;
		}
	/*	char pathname[256];
		snprintf(pathname, 256, "/tmp/words/spike-%03d.wav", word_no);

		SndfileHandle outfile(pathname, SFM_WRITE, format, channels, samplerate) ;
		assert(outfile.error() == SF_ERR_NO_ERROR);

		const frames_t written = outfile.writef(cur_word.data()->data(), cur_word.size());
		assert(written == cur_word.size());*/

	}
}

frames_t check_words(const cfg& config, std::vector<std::array<short, 2>>& file_content, int format, int samplerate, int channels,
	float min_time_word, float max_time_idle, std::vector<std::array<frames_t, 2>>& results)
{
	std::array<short, 2> frame;

	const float idle = config.silence_lvl * 32768; // everything below is silence

	frames_t cur_frames_word = 0;
	frames_t idle_count = 0;
	frames_t last_word_start=0;

	const frames_t max_frames_idle = max_time_idle * samplerate;
	const frames_t min_frames_word = min_time_word * samplerate;

	frames_t cur_word_size = 0;
	results.clear();



	const auto this_frame_is_word_func = [idle](const std::array<short, 2> frame) -> bool
	{
		return (frame[0] > idle || frame[0] < -idle || frame[1] > idle || frame[1] < -idle);
	};

	for (frames_t tell = 0; tell < file_content.size(); ++tell)
	{
		frame = file_content[tell];

		bool this_frame_is_word = this_frame_is_word_func(frame);

		if(this_frame_is_word /*&&
			(file.frames() - tell < 3000 ||
			(
				this_frame_is_word_func(file_content[tell + 1000]) &&
				this_frame_is_word_func(file_content[tell + 2000]) &&
				this_frame_is_word_func(file_content[tell + 3000])
			)
		)*/)
		{
			//frames_t cur_max_frames_idle = std::min(cur_frames_word, max_frames_idle);
			const frames_t cur_max_frames_idle = max_frames_idle;

			// we are now in a word, but the previous "cur_max_frames_idle+1" frames were idle?
			if(idle_count > cur_max_frames_idle)
			{
				// is word long enough?
				if (cur_frames_word > min_frames_word)
				{
					cur_word_size -= idle_count;
					//(*functor)(file_content, (int)words, cur_word, format, samplerate, channels, last_word_start);
					results.push_back(std::array<frames_t, 2>{last_word_start, cur_word_size});
				}
				else
					printf ("only %f seconds word at %f\n",
						cur_frames_word/(float)samplerate, tell/(float)samplerate);

				cur_word_size   = 0;
				cur_frames_word = 0;
				last_word_start = tell;
			}
			else {
				// end of the word
			}

			++cur_frames_word;
			idle_count = 0;
		}
		else
			++idle_count;

		++cur_word_size;
	}

	if(cur_frames_word)
	{
		cur_word_size -= idle_count;
		results.push_back(std::array<frames_t, 2>{last_word_start, cur_word_size});
	}

	return results.size();
}

int main (int argc, char** argv)
{
	int rval = EXIT_SUCCESS;
	try
	{
		cfg config;

		if(argc != 2)
			throw std::runtime_error("Expecting exactly 1 arg");
		const char * fname = argv[1];

		std::string cfg_fname = std::string(fname) + ".cfg";
		config.read_from(cfg_fname.c_str());

		SndfileHandle file(fname) ;

		printf ("Opened file '%s'\n", fname) ;
		printf ("    Sample rate : %d\n", file.samplerate ()) ;
		printf ("    Channels    : %d\n", file.channels ()) ;
		printf ("    Frames      : %ld\n",file.frames()) ;
		printf ("    Format:     : %x\n", file.format());

		if(!(file.format() & (SF_FORMAT_WAV | SF_FORMAT_PCM_16)))
			throw std::runtime_error("Invalid WAV format");

		std::vector<std::array<short, 2>> file_content(file.frames() * 4);
		{
			const sf_count_t read = file.readf(file_content.data()->data(), file.frames());
			if(read != file.frames())
				throw std::runtime_error("Not enough frames read");
		}

		std::vector<std::array<frames_t, 2>> found_words;
		frames_t spikes = check_words(config, file_content, file.format(), file.samplerate(), file.channels(),
				config.min_time_word_spike,
				config.max_time_idle_spike,
				found_words
		);

		int word_no = 0;
		for(const std::array<frames_t, 2>& start_and_length : found_words)
		{
			remove_spike(config, file_content, word_no, start_and_length, file.format(), file.samplerate(), file.channels());
			++word_no;
		}

		/*{
			SndfileHandle outfile("/tmp/words/nospikes.wav", SFM_WRITE, file.format(), file.channels(), file.samplerate()) ;
			assert(outfile.error() == SF_ERR_NO_ERROR);

			const frames_t written = outfile.writef(file_content.data()->data(), file_content.size());
			assert(written == file_content.size());
		}*/

		frames_t words = check_words(config, file_content, file.format(), file.samplerate(), file.channels(),
				config.min_time_word,
				config.max_time_idle,
				found_words
		);

		word_no = 0;
		for(const std::array<frames_t, 2>& start_and_length : found_words)
		{
			dump_word(config, file_content, word_no, start_and_length, file.format(), file.samplerate(), file.channels());
			++word_no;
		}


		printf ("%ld spikes, %ld words\n", spikes, words);

		puts ("Done.\n") ;
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		rval = EXIT_FAILURE;
	}

	return rval;
}

