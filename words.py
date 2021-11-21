#! /usr/bin/python

import struct

max_time_idle = 0.7 # without breaking a word
min_time_word = 0.1
idle = 0.01 * 32768 # everything below is silence
#min_word_length = 0.2
min_loudness = 0.2 * 3276




import wave
w = wave.open('/tmp/in.wav', 'r')

max_frames_idle = max_time_idle * w.getframerate()
min_frames_word = min_time_word * w.getframerate()
cur_frames_word = 0
words = 0

print (w.getsampwidth())
print (w.getnchannels())

last_word_start=0
last_word_end=0
idle_count=0 # frames idle after last word

cur_word = []

for i in range(w.getnframes()):
    
    this_frame_is_word = False

    ### read 1 frame and the position will updated ###
    frame = w.readframes(1)

    if len(frame) != w.getsampwidth() * w.getnchannels():
        raise Exception('Incorrect length: %d != %d * %d' % (len(frame), w.getsampwidth(), w.getnchannels()) )
    
    (value1, value2) = struct.unpack("<hh", frame)

    if value1 > idle or value1 < -idle or value2 > idle and value2 < -idle:
      this_frame_is_word = True
          
    if this_frame_is_word:
        cur_max_frames_idle = min(cur_frames_word, max_frames_idle)
        if(idle_count > cur_max_frames_idle): # too many idle -> previous word ends, new word starts
            #print (idle_count/w.getframerate())
            if cur_frames_word > min_frames_word:
                print ('word %d: %s (%s -> %s) seconds word at %s (found at %s)' % (words, (last_word_end-last_word_start)/w.getframerate(), last_word_start/w.getframerate(), last_word_end/w.getframerate(), (w.tell()-idle_count)/w.getframerate(), w.tell()/w.getframerate()))
                print ('frames: %d, %d, idle_count: %d' % (cur_frames_word, max_frames_idle, idle_count))
                #if(words % 5 == 0)
                if(True):
                    wout = wave.open('/tmp/words/%03d.wav' % words, 'w')
                    wout.setnchannels(w.getnchannels())
                    wout.setsampwidth(w.getsampwidth())
                    wout.setframerate(w.getframerate())
                    wout.setnframes(last_word_end-last_word_start)
                    for writeframe in cur_word:
                        wout.writeframesraw(writeframe)
                    wout.close
                words = words + 1
            else:
                print ('only %s seconds word at %s' % (cur_frames_word/w.getframerate(), w.tell()/w.getframerate()))
            cur_word = []
            cur_frames_word = 0
            last_word_start = w.tell()
        else:
            last_word_end = w.tell() # TODO - idle_count
    
        cur_frames_word = cur_frames_word + 1
        idle_count = 0
        #print (cur_frames_word)
    else:
        idle_count = idle_count + 1;
    cur_word.append(frame)

print ('%d words'% words)
