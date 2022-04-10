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

#include "cfg.h"
#include "plan.h"

using frames_t = unsigned long;

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
#else
	(void) word_no;
	(void) channels;
	(void) format;
#endif
	frames_t max_spike_size = static_cast<frames_t>(config.max_time_spike * (float)samplerate);
	if(start_and_length[1] <= max_spike_size)
	{
		printf("Spike of %f s (%lu samples) at pos %f\n", (float)start_and_length[1]/(float)samplerate, start_and_length[1], (float)start_and_length[0]/(float)samplerate);
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

//! @param results resulting pairs of (start, length) for each word
frames_t check_words(const cfg& config, std::vector<std::array<short, 2>>& file_content, int samplerate,
	float min_time_word, float max_time_idle, std::vector<std::array<frames_t, 2>>& results)
{
	std::array<short, 2> frame;

	const float idle = config.silence_lvl * 32768; // everything below is silence

	frames_t cur_frames_word = 0;
	frames_t idle_count = 0;
	frames_t last_word_start=0;

	const frames_t max_frames_idle =
		static_cast<frames_t>(max_time_idle * (float)samplerate);
	const frames_t min_frames_word =
		static_cast<frames_t>(min_time_word * (float)samplerate);

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
						(float)cur_frames_word/(float)samplerate, (float)tell/(float)samplerate);

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
		if(argc != 2)
			throw std::runtime_error("Expecting exactly 1 arg");
		plan_t plan;
		plan.read_from(argv[1]);

		for(const std::string& fname_in_plan : plan.files)
		{
			cfg config;
			std::string cfg_fname = fname_in_plan + ".cfg";
			config.read_from(cfg_fname.c_str());

			SndfileHandle file(fname_in_plan) ;

			printf ("Opened file '%s'\n", fname_in_plan.c_str()) ;
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
			frames_t spikes = check_words(config, file_content, file.samplerate(),
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

			frames_t words = check_words(config, file_content, file.samplerate(),
					config.min_time_word,
					config.max_time_idle,
					found_words
			);

			const frames_t fade_in_frames =
			static_cast<frames_t>(config.fade_in_time * (float)file.samplerate());
			const frames_t fade_out_frames =
			static_cast<frames_t>(config.fade_out_time * (float)file.samplerate());

#ifdef DUMP_SINGLE
			word_no = 0;
			for(const std::array<frames_t, 2>& start_and_length : found_words)
			{
				dump_word(config, file_content, word_no, start_and_length, file.format(), file.samplerate(), file.channels());
				++word_no;
			}
#else
			char pathname[256], pathname_frames[256];
			snprintf(pathname,        256, "/tmp/compressed.wav");
			snprintf(pathname_frames, 256, "/tmp/compressed.wav.frames");

			SndfileHandle outfile(pathname, SFM_WRITE, file.format(), file.channels(), file.samplerate()) ;
			if(outfile.error() != SF_ERR_NO_ERROR)
				throw std::runtime_error("Error opening outfile");

			std::ofstream out(pathname_frames);

			frames_t framepos = 0;
			for(const std::array<frames_t, 2>& start_and_length : found_words)
			{
				const frames_t written = outfile.writef(file_content[start_and_length[0]].data(), start_and_length[1]);
				if(written != start_and_length[1])
					throw std::runtime_error("Not enough bytes written");
				out << framepos << std::endl;
				framepos += written;
			}


#endif


			printf ("%s: %ld spikes, %ld words\n", fname_in_plan.c_str(), spikes, words);
		} // for each plan file
		puts ("Done.\n") ;
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		rval = EXIT_FAILURE;
	}

	return rval;
}

