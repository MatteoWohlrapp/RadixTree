# Use an official Ubuntu runtime as a parent image
FROM ubuntu:20.04

# Set the working directory in the container to /app
WORKDIR /app

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install any necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    libgtest-dev \
    clang \
    lldb \
    doxygen \
    curl \
    libssl-dev \
    libboost-all-dev \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Set default compiler options
ENV CXX=/usr/bin/clang++
ENV CC=/usr/bin/clang

# Download, build and install specific version of CMake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2.tar.gz \
    && tar -xzvf cmake-3.20.2.tar.gz \
    && cd cmake-3.20.2 \
    && ./bootstrap \
    && make \
    && make install

# Clone and build googletest
RUN git clone https://github.com/google/googletest.git --branch release-1.11.0 && \
    cd googletest && \
    cmake . && \
    make && \
    make install

# Copy the current directory contents into the container at /app
COPY . /app

# Make port 80 available to the world outside this container
EXPOSE 80
