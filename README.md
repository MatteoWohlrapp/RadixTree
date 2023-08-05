# Radix Tree

Codebase for the Thesis: Evaluation of a Radix Tree Cache for Database Management Systems <br>
PDF of the thesis can be found in the root. <br>
Author: Matteo Wohlrapp <br>


## Docker 
Docker is not necessary to run the executable, but if you want to use it, the following commands can be used: <br>
1. Build the images: 
```
docker build -t radixtree .
```
2. Run the docker container: 
If you want to debug, you need to run <br>
```
docker run -v <path_to_project>:/app -p 8000:8000 -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined radixtree
```
otherwise, the following is enough, 
```
docker run -v <path_to_project>:/app -p 8000:8000 -it radixtree
```

## Build

### Build without Doxygen

1. Navigate into the build folder: `cd build`
2. Run `cmake ..` to create the makefile. Version 10.0.0-4ubuntu1 of the clang compiler is used. If you want to build in debug, use `cmake -DCMAKE_BUILD_TYPE=Debug ..`
3. Run `make` to generate the executable and tests.

### Build with Doxygen

1. Navigate into the build folder: `cd build`
2. Run `cmake -D BUILD_DOC=ON ..` to create the makefile. Version 10.0.0-4ubuntu1 of the Clang compiler is used.
3. Run `make` to generate the executable and tests and `make doc_doxygen` to generate the documentation.

## Run 
There are multiple options to run the code. Check the help menu to find out more: <br>
1. `-s` runs the script that is also used for the benchmarks in the evaluation section of the thesis and saves the results to a CSV file. Before running, create a `results` folder under root if not already available
2. `-w{,a,b,c,e,x}`runs single workloads. Either the predefined ones or an individual one, defined through the command line parameters
3. `-r <number>` executes a 'run confiugration', which is basically a way for you to specify what you want to compute. Just modify `run_config_one.cc` or `run_config_two.cc`. The data manager will be created automatically

## Testing
In the `tests` folder, there are multiple numerous unit tests, testing all important components. You can run them by executing the `./Alltests` executable. <br>
Additionally, there is a debugger, defined in `Debugger.h`, that offers a function to traverse both the radix- and the B+ tree and print information about the nodes. You can use it in one of the `run_config` files and pass the data manager in order to extract information about the trees.

## Results 
In the `analysis` folder in the root, you can find a Matlab script that generates graphs from the data in the CSV files generated when running with `-s`. 
The graphs are generated and saved as tickz files in the `analysis` folder. The files present are for the run specified in the Matlab script.