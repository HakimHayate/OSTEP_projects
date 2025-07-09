In this project I had to implement another version of wzip to take advantage of the cpus in the machine. So I have to write a multi-threaded program that concurrently compress files.

My design was to have a queue and 2 locks, 1 for the head to avoid the case where different threads access the head at the same time, and the other for the tail, same problem, different threads try to add to queue at the same time (even tho I only added at the moment 1 thread for reading from file so it's not necessary).

(START DAY) 04/07/25: Just thought of how I'll implement the data structure and things I have to take care of.


05-06/07/25: wrote the program, tested it (seem to work but maybe there are still some bugs in it and definetly many things in the code to optimize to make the code faster), and also compared it to the other version that doesn't use threads and found it that this version is 2 times faster than the other which is good.

Some possible optimizations:
-> Instead of copying a chunk of file into a buffer then giving to a wroker to compress, we can just give a ptr that points to that chunk.
-> If a file contains a very big number of 1 letter consecutively, we may have a trouble with writing. Sol would be to write at the end and do some checks between chunks of files before writing.

So instead of having 3 stages 1 for reading, compressing and writing, we'll follow the producer/consumer protocole where the producer give consumers work to do dynamically (if a thread finished the current chunk it moves on to the next), once they all finished the work, the producer will check if there are characters to merge together at the end of the chunk and then print out the compressed chunks in ordre.
