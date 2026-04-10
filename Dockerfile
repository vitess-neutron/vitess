FROM ubuntu:latest

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies (same as .gitlab-ci.yml compile-ubuntu before_script)
RUN apt-get update && apt-get install -y \
    make \
    gcc \
    g++ \
    libxpm-dev \
    libpng-dev \
    libgd-dev \
    zlib1g-dev \
    git \
    unzip \
    cmake \
    libxml2-dev \
    python3 \
    tk \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /vitess

# Copy Vitess source code
COPY . /vitess

# Build Vitess from source
WORKDIR /vitess/SRC

RUN mkdir -p ../MODULES && \
    make all LTO=1 && \
    make install && \
    make distclean

ENV V=/vitess/MODULES
ENV P=/data/projects
ENV L=/data/logs/logfile.log

# Create directories for shared data
# Note: L is a file path, so we create the parent directory
RUN mkdir -p /data/projects /data/logs

# Make Vitess modules accessible
WORKDIR /vitess

# Default command (can be overridden in docker-compose)
CMD ["/bin/bash"]

