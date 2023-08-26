
#  Distributed Systems Labs in C++

MIT 6.824 style distributed systems lab rebuilt in C++.
It includes a series of labs in which you will build a transactional, sharded, fault-tolerant key/value storage system (like Google Spanner, MongoDB, etc). 

## Lab assignments

- Lab 1 - Replicated State Machine 
- Lab 2 - Fault-tolerant Key-value Store
- Lab 3 - Sharded Key-value Store
- Lab 4 - Distributed Transactions

## Lab environment

A modern linux environment (e.g., Ubuntu 22.04) is recommended for the labs. If you do not have access to this, consider using a virtual machine. 

## Getting started (Ubuntu 22.04)

Get source code:
```
git clone --recursive [repo_address]
```

Install dependencies:
```
sudo apt-get update
sudo apt-get install -y \
    git \
    pkg-config \
    build-essential \
    clang \
    libapr1-dev libaprutil1-dev \
    libboost-all-dev \
    libyaml-cpp-dev \
    libjemalloc-dev \
    python2 \
    python3-dev \
    python3-pip \
    python3-wheel \
    python3-setuptools \
    libgoogle-perftools-dev
sudo pip3 install -r requirements.txt
```

For next steps, checkout the guidelines in the [course web page](http://mpaxos.com/teaching/ds/22fa/labs.html).

## Author and acknowledgements
Author: Shuai Mu, Julie Lee (lab 1)

Many of the lab structure and the guideline text are taken from MIT 6.824. The code is based on the acedemic prototypes of previous research works including Rococo/Janus/Snow/DRP/Rolis/DepFast.
