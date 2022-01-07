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

using frames_t = unsigned long;

typedef void (*functor_t)(std::vector<std::array<short, 2>>& , int, std::vector<std::array<short, 2>>& , int , int , int , int);

void dump_word(std::vector<std::array<short, 2>>&, int word_no, std::vector<std::array<short, 2>>& cur_word, int format, int samplerate, int channels, int)
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
	assert(outfile.error() == SF_ERR_NO_ERROR);

	const frames_t written = outfile.writef(cur_word.data()->data(), cur_word.size());
	assert(written == cur_word.size());
}

void remove_spike(std::vector<std::array<short, 2>>& file_content, int word_no, std::vector<std::array<short, 2>>& cur_word, int format, int samplerate, int channels, int last_word_start)
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
	float max_time_spike = 0.05f;
	frames_t max_spike_size = max_time_spike * samplerate;
	if(cur_word.size() <= max_spike_size)
	{
		printf("Spike of %f s (%lu samples) at pos %f\n", cur_word.size()/(float)samplerate, cur_word.size(), last_word_start/(float)samplerate);
		for(int i = -1; i < (int)cur_word.size(); ++i)
		{
			file_content[i+last_word_start][0] = 0;
			file_content[i+last_word_start][1] = 0;
		}
	/*	char pathname[256];
		snprintf(pathname, 256, "/tmp/words/spike-%03d.wav", word_no);

		SndfileHandle outfile(pathname, SFM_WRITE, format, channels, samplerate) ;
		assert(outfile.error() == SF_ERR_NO_ERROR);

		const frames_t written = outfile.writef(cur_word.data()->data(), cur_word.size());
		assert(written == cur_word.size());*/

	}
}

frames_t check_words(std::vector<std::array<short, 2>>& file_content, int format, int samplerate, int channels,
	float min_time_word, float max_time_idle, functor_t functor)
{
	std::array<short, 2> frame;

	const float idle = 0.01f * 32768; // everything below is silence


	frames_t cur_frames_word = 0;
	frames_t idle_count = 0;
	frames_t last_word_start=0;
	frames_t last_word_end=0;

	frames_t tell = 0;

	frames_t words = 0;

	const frames_t max_frames_idle = max_time_idle * samplerate;
	const frames_t min_frames_word = min_time_word * samplerate;


	std::vector<std::array<short, 2>> cur_word;



	const auto this_frame_is_word_func = [idle](const std::array<short, 2> frame) -> bool
	{
		return (frame[0] > idle || frame[0] < -idle || frame[1] > idle || frame[1] < -idle);
	};

	//for (frames_t i = 0; i < (frames_t)file.frames(); ++i)
	for (frames_t i = 0; i < file_content.size(); ++i)
	{
		assert(tell == i); // TODO: remove tell?
		frame = file_content[tell];
		++tell; // TODO: should be at end?

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
					cur_word.resize(cur_word.size() - idle_count);
					(*functor)(file_content, (int)words, cur_word, format, samplerate, channels, last_word_start);
					++words;
				}
				else
					printf ("only %f seconds word at %f\n",
						cur_frames_word/(float)samplerate, tell/(float)samplerate);

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

	if(cur_frames_word)
	{
		cur_word.resize(cur_word.size() - idle_count);
		(*functor)(file_content, (int)words, cur_word, format, samplerate, channels, last_word_start);
		++words;
	}

	return words;
}

int main (void)
{
	const char * fname = "/tmp/in.wav" ;
	SndfileHandle file(fname) ;



	//float min_loudness = 0.2f * 3276;

	printf ("Opened file '%s'\n", fname) ;
	printf ("    Sample rate : %d\n", file.samplerate ()) ;
	printf ("    Channels    : %d\n", file.channels ()) ;
	printf ("    Frames      : %ld\n",file.frames()) ;
	printf ("    Format:     : %x\n", file.format());

	assert(file.format() & (SF_FORMAT_WAV | SF_FORMAT_PCM_16));



	frames_t idle_count_prev=10000000; // frames idle after last word

	std::vector<std::array<short, 2>> file_content(file.frames() * 4);
	{
		const sf_count_t read = file.readf(file_content.data()->data(), file.frames());
		assert(read == file.frames());
	}

	frames_t spikes = check_words(file_content, file.format(), file.samplerate(), file.channels(),
			0.0f, // min time word
			0.1f, // max time idle
			remove_spike
	);

	{
		SndfileHandle outfile("/tmp/words/nospikes.wav", SFM_WRITE, file.format(), file.channels(), file.samplerate()) ;
		assert(outfile.error() == SF_ERR_NO_ERROR);

		const frames_t written = outfile.writef(file_content.data()->data(), file_content.size());
		assert(written == file_content.size());
	}

	frames_t words = check_words(file_content, file.format(), file.samplerate(), file.channels(),
			0.1f, // min time word
			0.6f, // max time idle
			dump_word
	);



	printf ("%ld spikes, %ld words\n", spikes, words);

	puts ("Done.\n") ;
	return 0 ;
}

