/*
** Copyright (C) 2020 Johannes Lorenz <mail_umleitung@web.de>
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
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include <sndfile.hh>

int main (void)
{
	const char * fname = "/tmp/in.wav" ;
	SndfileHandle file(fname) ;

	const float max_time_idle = 0.7f; // without breaking a word
	const float min_time_word = 0.1f;
	const float idle = 0.01f * 32768; // everything below is silence
	//float min_loudness = 0.2f * 3276;

	printf ("Opened file '%s'\n", fname) ;
	printf ("    Sample rate : %d\n", file.samplerate ()) ;
	printf ("    Channels    : %d\n", file.channels ()) ;
	printf ("    Frames      : %ld\n",file.frames()) ;
	printf ("    Format:     : %x\n", file.format());

	assert(file.format() & (SF_FORMAT_WAV | SF_FORMAT_PCM_16));

	std::array<short, 2> frame;

	using frames_t = unsigned long;

	const frames_t max_frames_idle = max_time_idle * file.samplerate();
	const frames_t min_frames_word = min_time_word * file.samplerate();

	frames_t cur_frames_word = 0;
	frames_t words = 0;
	frames_t last_word_start=0;
	frames_t last_word_end=0;
	frames_t idle_count=0; // frames idle after last word

	std::vector<std::array<short, 2>> cur_word;

	std::vector<std::array<short, 2>> file_content(file.frames() * 4);
	{
		const sf_count_t read = file.readf(file_content.data()->data(), file.frames());
		assert(read == file.frames());
	}

	frames_t tell = 0;
	for (frames_t i = 0; i < (frames_t)file.frames(); ++i)
	{
		frame = file_content[tell];
		++tell; // TODO: should be at end?

		const bool this_frame_is_word =
			(frame[0] > idle || frame[0] < -idle || frame[1] > idle || frame[1] < -idle);

		if(this_frame_is_word)
		{
			//frames_t cur_max_frames_idle = std::min(cur_frames_word, max_frames_idle);
			const frames_t cur_max_frames_idle = max_frames_idle;

			if(idle_count > cur_max_frames_idle)
			{
				if (cur_frames_word > min_frames_word)
				{
					printf ("word %ld: %f (%f -> %f) seconds word at %ld (found at %ld)\n",
						words, (last_word_end-last_word_start)/(float)file.samplerate(),
						last_word_start/(float)file.samplerate(),
						last_word_end/(float)file.samplerate(),
						(tell-idle_count)/file.samplerate(), tell/file.samplerate());
					printf ("frames: %ld, %ld, idle_count: %ld\n",
						cur_frames_word, max_frames_idle, idle_count);

					char pathname[256];
					snprintf(pathname, 256, "/tmp/words/%03ld.wav", words);

					SndfileHandle outfile(pathname, SFM_WRITE, file.format(), file.channels(), file.samplerate()) ;
					assert(outfile.error() == SF_ERR_NO_ERROR);

					const frames_t written = outfile.writef(cur_word.data()->data(), cur_word.size());
					assert(written == cur_word.size());

					++words;
				}
				else
					printf ("only %f seconds word at %f\n",
						cur_frames_word/(float)file.samplerate(), tell/(float)file.samplerate());

				cur_word.clear();
				cur_frames_word = 0;
				last_word_start = tell;
			}
			else
				last_word_end = tell;

			++cur_frames_word;
			idle_count = 0;
		}
		else
			++idle_count;

		cur_word.push_back(frame);
	}
	printf ("%ld words\n", words);

	puts ("Done.\n") ;
	return 0 ;
}

