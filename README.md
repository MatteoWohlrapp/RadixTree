# RadixTree

Codebase for the Thesis: Evaluation of a RadixTree Cache for Database Management Systems


## Docker Instructions 
To run the image with docker, the following commands are needed: <br>
1. Build the images: 
```
docker build -t radixtree .
```
2. Run the docker container: 
If you want to debug, you need to run <br>
```
docker run -v <path_to_project>:/app -p 8000:8000 -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfine radixtree
```
otherwise the following is enough, 
```
docker run -v <path_to_project>:/app -p 8000:8000 -it radixtree
```


## Build and run

### Build without Doxygen

1. Navigate into the build folder: `cd build`
2. Run `cmake ..` to create the makefile. Version 10.0.0-4ubuntu1 of the clang compiler is used.
3. Run `make` to generate the executable and tests.

### Build with Doxygen

1. Navigate into the build folder: `cd build`
2. Run `cmake -D BUILD_DOC=ON ..` to create the makefile. Version 10.0.0-4ubuntu1 of the clang compiler is used.
3. Run `make` to generate the executable and tests and `make doc_doxygen` to generate the documentation.