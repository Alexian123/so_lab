# so_lab - Project

## File structure
The **in.bak** directory contains the sample test data. Do not remove it, or the makefile will stop working! This directory contains the original (colored) sample bitmap images.

The **include** directory contains headers used in the C program.

**filestats.c** is the main (and only) source file.

**count_correct_sentences.sh** is a shell script which reads lines from stdin until EOF and outputs the number of correct sentences to stdout.

This project uses a **Makefile** for a simpler and more scalable build.

## Makefile information
#### Build & Run
```
make run
```
This command will first run ```make clean``` automatically to ensure a clean build!
After running this command, a new directory named **in** will be created, and all of the files from **in.bak** will be copied there. Also, the **out** dir will be created. When the program ends, the bitmap images from **in** will be in grayscale and **out** will contain a statistics file for each file in the **in** dir. 

#### Clean
```
make clean
```
After running this command, **filestats** (the main binary), the **in** directory and the **out** directory will all be REMOVED.

## Running without the makefile
Compile the source file using gcc:<br>
```gcc -Wall -Iinclude filestats.c -o filestats```


You will need to provide EXISTING input and output directories.
For this example, we will use the files from **in.bak**:<br>
```mkdir input```<br>
```cp -r ./in.bak/* ./input```

Then we create an output directory:<br>
```mkdir output```

Now we run the program:<br>
```./filestats input output a```

The last argument is a character which **count_correct_sentences.sh** will use to determine the number of correct sentences which contain it.
