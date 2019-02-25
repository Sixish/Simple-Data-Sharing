# Introduction
The Simple Data Sharing program is split into two programs:
* Threads: pthreads-solution
* Processes: process-solution

Navigate to one of the above directories depending on which program
is desired. Follow the steps below to compile and run the program.


# Compilation
From the command line, run the command
    make

# Running
From the command line, run the command
    ./bin/sds r w t1 t2
where
    r = number of readers
    w = number of writers
    t1 = time to sleep readers between reads
    t2 = time to sleep writers between writes

This assumes that shared_data is kept in the working directory.
