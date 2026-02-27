
# Lab 1: Replicated State Machine

* * *

## Introduction

This is the first in a series of labs in which you'll build a fault-tolerant key/value storage system. In this lab you'll implement Raft, a replicated state machine protocol. In the next lab you'll build a key/value service on top of Raft. 
<!-- Then you will “shard” your service over multiple replicated state machines for higher performance. -->

A replicated service achieves fault tolerance by storing complete copies of its state (i.e., data) on multiple replica servers. Replication allows the service to continue operating even if some of its servers experience failures (crashes or a broken or flaky network). The challenge is that failures may cause the replicas to hold differing copies of the data.

Raft manages a service's state replicas, and in particular it helps the service sort out what the correct state is after failures. Raft implements a replicated state machine. It organizes client requests into a sequence, called the log, and ensures that all the replicas agree on the contents of the log. Each replica executes the client requests in the log in the order they appear in the log, applying those requests to the replica's local copy of the service's state. Since all the live replicas see the same log contents, they all execute the same requests in the same order, and thus continue to have identical service state. If a server fails but later recovers, Raft takes care of bringing its log up to date. Raft will continue to operate as long as at least a majority of the servers are alive and can talk to each other. If there is no such majority, Raft will make no progress, but will pick up where it left off as soon as a majority can communicate again.

In this lab you'll implement Raft as a C++ class with associated methods, meant to be used as a module in a larger service. A set of Raft instances talk to each other with RPC to maintain replicated logs. Your Raft interface will support an indefinite sequence of numbered commands, also called log entries. The entries are numbered with _index numbers_. The log entry with a given index will eventually be committed. At that point, your Raft should send the log entry to the larger service for it to execute.

Your Raft instances are only allowed to interact using RPC. For example, different Raft instances are not allowed to share variables. Your code should not use files at all.

## Objective

In this lab you'll implement most of the Raft design described in the extended paper. You will not implement:  saving persistent state, cluster membership changes (Section 6), log compaction / snapshotting (Section 7).

Some general tips: 
*   Start early. Although the amount of code isn't large, getting it to work correctly will be challenging.
*   Read and understand the [extended Raft paper](../readings/raft.pdf) and the Raft lecture notes before you start. Your implementation should follow the paper's description closely, particularly Figure 2, since that's what the tests expect.

<!-- This lab is divided into two parts.  -->
<!-- You must submit each part on the corresponding due date. This lab does not involve a lot of code, but concurrency makes it challenging to debug; start each part early. -->

<!-- Implement the Raft algorithm using the given skeleton code. -->

<!-- This implementation should have all functionality of the Raft algorithm except saving persistent state (there is currently no way to crash servers in this codebase), cluster membership changes, and log compaction / snapshotting. -->

## Getting Started

First make sure you are working on the right branch, and create your working branch as `lab-raft-solution`.
```bash
$ git checkout -b lab-raft-solution origin/raft-lab-26
$ git submodule update --init
$ git push -u origin lab-raft-solution
```

We supply you with skeleton code and tests in `src/protocol/raft`.

To get up and running, execute the following commands:

```
$ cd your_labs_directory
$ cmake -S . -B build-cpp -DBUILD_RAFT_LAB_TESTS=ON
$ cmake --build build-cpp --target labtest -j$(nproc)
$ ./build-cpp/labtest -f config/raft_lab_test.yml
TEST 1: Initial election
TEST 1 Failed: waited too long for leader election
TESTS FAILED
``` 

Note: the runtime naming in this codebase now uses `fiber` terminology (older materials may say `coroutine`).

## The Code

Most of the Raft implementation will be located in `src/protocol/raft/`. In this directory, there are 3 `.cc` files (and respective headers) that you need to modify:
* `server.cc` and `server.h`
* `commo.cc` and `commo.h`
* `service.cc` and `service.h`

Besides these three files, there is also `src/protocol/raft/raft_rpc.rpc`, where you will find `abstract service Raft` containing sample prototypes for the `RequestVote` and `AppendEntries` RPCs. Here, you can modify the arguments and return values for the `RequestVote` and `AppendEntries` RPCs and add any other RPCs as you see fit.

Do not create new files, and do not modify any existing files not mentioned.

### RPCs

Class `RaftServiceImpl` (`service.cc`) implements receiver-end handlers for the RPCs declared in `src/protocol/raft/raft_rpc.rpc`. To register a handler for an RPC in `RaftServiceImpl`, use the `RpcHandler` macro.

`RpcHandler` usage: `RpcHandler(RPC_NAME, N_PARAMS, PARAMS...) { DEFAULTLOGIC }`
* `RPC_NAME`: should match the name of the RPC declared in `raft_rpc.rpc`
* `N_PARAMS`: number of RPC arguments + number of RPC return values
* `PARAMS`: the RPC arguments and return values in the same order as in `raft_rpc.rpc`, with comma separations between the type and name.
* `DEFAULTLOGIC`: write code to assign default values to the RPC's return value in these brackets. This code will get invoked when the server is disconnected from the network to simulate a failed RPC. It is important that your RPC sender code recognizes the default values and ignores them when they happen.

Class `RaftCommo` (`commo.cc`) is meant to handle sending RPC requests. See the example sender functions to understand how to send RPCs in `RaftCommo`.

### Server logic

Class `RaftServer` (`server.cc`) is your starting point for writing most of the server logic.

Some useful member variables (inherited from a parent class of `RaftServer`):
* `loc_id_`: id of the server.
* `commo()`: function that returns the server's `RaftCommo` instance.
* `mtx_`: a `std::recursive_mutex` you can use for protecting data from concurrent access.

## Required Functionality

There are a few specific functionalities in `RaftServer` you need to implement in order to pass all tests.

`bool Start(shared_ptr<Marshallable> &cmd, uint64_t *index, uint64_t *term, slotid_t slot_id, ballot_t ballot)`
* Implement this member function
* If server is not the leader, return false
* Else, start agreement on `cmd` in a new log entry, set `index` and `term` with the server's current index and term, and return true

`void GetState(bool *is_leader, uint64_t *term)`
* Implement this member function
* Populate is_leader with true if the server thinks it is the leader
* Populate term with the server's current term number

`function<void(Marshallable&)> app_next_`
* A member variable of a parent class of `RaftServer`.
* Each server must pass each committed commmand to `app_next_` exactly once, in the correct order, as soon as each command is committed on each server.

Your first implementation may not be clean enough that you can easily reason about its correctness. Give yourself enough time to rewrite your implementation so that you can easily reason about its correctness. Subsequent labs will build on this lab, so it is important to do a good job on your implementation.

You are recommended to do this lab following these two steps: 

### Part 1A

Implement leader election and heartbeats (<tt>AppendEntries</tt> RPCs with no log entries). The goal for Part 1A is for a single leader to be elected, for the leader to remain the leader if there are no failures, and for a new leader to take over if the old leader fails or if packets to/from the old leader are lost. 
<!-- Run <tt>go test -run 1A</tt> to test your 1A code. -->

* Add any state you need to the <tt>RaftServer</tt> class (but do not use static member variable to share states between different `RaftServer` instances) in <tt>server.cc</tt>. You'll also need to define a struct to hold information about each log entry. Your code should follow Figure 2 in the paper as closely as possible.
* In `RaftServer`, create the function to handle `RequestVote`. Fill in the `RequestVote` RPC handler in `service.cc`, connect the RPC handler and the `RaftServer`.
* When initializing the `RaftServer`, create a background fiber that will kick off leader election periodically by sending out <tt>RequestVote</tt> RPCs when it hasn't heard from another peer for a while. This way a peer will learn who is the leader, if there is already a leader, or become the leader itself.
<!-- *   Fill in the <tt>RequestVoteArgs</tt> and <tt>RequestVoteReply</tt> structs. Modify <tt>Make()</tt> to create a background goroutine that will kick off leader election periodically by sending out <tt>RequestVote</tt> RPCs when it hasn't heard from another peer for a while. This way a peer will learn who is the leader, if there is already a leader, or become the leader itself. Implement the <tt>RequestVote()</tt> RPC handler so that servers will vote for one another. -->
*   To implement heartbeats, implement a separate `EmptyAppendEntries` RPC (`AppendEntry` without log entries), and have the leader send them out periodically. Write an `EmptyAppendEntries` RPC handler method that resets the election timeout so that other servers don't step forward as leaders when one has already been elected.
*   Make sure the election timeouts in different peers don't always fire at the same time, or else all peers will vote only for themselves and no one will become the leader.
*   The tester requires that the leader send heartbeat RPCs no more than ten times per second.
*   The tester requires your Raft to elect a new leader within five seconds of the failure of the old leader (if a majority of peers can still communicate). Remember, however, that leader election may require multiple rounds in case of a split vote (which can happen if packets are lost or if candidates unluckily choose the same random backoff times). You must pick election timeouts (and thus heartbeat intervals) that are short enough that it's very likely that an election will complete in less than five seconds even if it requires multiple rounds.
*   The paper's Section 5.2 mentions election timeouts in the range of 150 to 300 milliseconds. Such a range only makes sense if the leader sends heartbeats considerably more often than once per 150 milliseconds. Because the tester limits the RPC count (you are recommended to set the heartbeat interval to 100ms), you will have to use an election timeout larger than the paper's 150 to 300 milliseconds, but not too large, because then you may fail to elect a leader within five seconds.
<!-- *   You may find Go's [rand](https://golang.org/pkg/math/rand/) useful. -->
<!-- *   You'll need to write code that takes actions periodically or after delays in time. The easiest way to do this is to create a goroutine with a loop that calls [time.Sleep()](https://golang.org/pkg/time/#Sleep). The hard way is to use Go's <tt>time.Timer</tt> or <tt>time.Ticker</tt>, which are difficult to use correctly. -->
<!-- *   If you are puzzled about locking, you may find this [advice](raft-locking.txt) helpful. -->
*   If your code has trouble passing the tests, read the paper's Figure 2 again; the full logic for leader election is spread over multiple parts of the figure.
*   A good way to debug your code is to insert print statements when a peer sends or receives a message, and redirect the output in a file `compile_cmd &> out`. Then, by studying the trace of messages in the <tt>out</tt> file, you can identify where your implementation deviates from the desired protocol. You might find `Log_info` and `Log_debug` in the framework useful to turn printing on and off as you debug different problems.
<!-- *   Go RPC sends only struct fields whose names start with capital letters. Sub-structures must also have capitalized field names (e.g. fields of log records in an array). The <tt>labgob</tt> package will warn you about this; don't ignore the warnings. -->
<!-- *   You should check your code with <tt>go test -race</tt>, and fix any races it reports. -->

<!-- Be sure you pass the 2A tests before submitting Part 2A, so that you see something like this:

<pre>$ go test -run 2A
Test (2A): initial election ...
  ... Passed --   2.5  3   30    0
Test (2A): election after network failure ...
  ... Passed --   4.5  3   78    0
PASS
ok      raft    7.016s
$
</pre>

Each "Passed" line contains four numbers; these are the time that the test took in seconds, the number of Raft peers (usually 3 or 5), the number of RPCs sent during the test, and the number of log entries that Raft reports were committed. Your numbers will differ from those shown here. You can ignore the numbers if you like, but they may help you sanity-check the number of RPCs that your implementation sends. For all of labs 2, 3, and 4, the grading script will fail your solution if it takes more than 600 seconds for all of the tests (<tt>go test</tt>), or if any individual test takes more than 120 seconds. -->

<!-- Part B will test leader election in more challenging settings and may expose bugs in your leader election code which the 1A tests miss. -->



### Part 1B

We want Raft to keep a consistent, replicated log of operations. A call to <tt>Start()</tt> at the leader starts the process of adding a new operation to the log; the leader sends the new operation to the other servers in <tt>AppendEntries</tt> RPCs.

Implement the leader and follower code to append new log entries. This will involve implementing <tt>Start()</tt>, completing the <tt>AppendEntries</tt> RPC structs, sending them, fleshing out the <tt>AppendEntry</tt> RPC handler, and advancing the <tt>commitIndex</tt> at the leader. 
<!-- Your first goal should be to pass the <tt>TestBasicAgree2B()</tt> test (in <tt>test_test.go</tt>). Once you have that working, you should get all the 2B tests to pass (<tt>go test -run 2B</tt>). -->

*  You will need to implement the election restriction (section 5.4.1 in the paper).
*  One way to fail the tests is to hold un-needed elections, that is, an election even though the current leader is alive and can talk to all peers. This can prevent agreement in situations where the tester believes agreement is possible. Bugs in election timer management, or not sending out heartbeats immediately after winning an election, can cause un-needed elections.
*  You may need to write code that waits for certain events to occur. Do not write loops that execute continuously without pausing, since that will slow your implementation enough that it fails tests. 
<!-- *  You can wait efficiently with Go's channels, or Go's [condition variables](https://golang.org/pkg/sync/#Cond), or (if all else fails) by inserting a <tt>time.Sleep(10 * time.Millisecond)</tt> in each loop iteration. -->
<!-- *  Give yourself time to rewrite your implementation in light of lessons learned about structuring concurrent code. In later labs you'll thank yourself for having Raft code that's as clear and clean as possible. For ideas, you can re-visit our [structure](raft-structure.txt), [locking](raft-locking.txt) and [guide](https://thesquareplanet.com/blog/students-guide-to-raft/), pages. -->

The tests for upcoming labs may fail your code if it runs too slowly. Try to write efficient code.
<!-- You can check how much real time and CPU time your solution uses with the time command. Here's some typical output for Lab 2B: -->

## Compile and Test

Compile (recommended):
```
$ cmake -S . -B build-cpp -DBUILD_RAFT_LAB_TESTS=ON
$ cmake --build build-cpp --target labtest -j$(nproc)
```

Test:
```
$ ./build-cpp/labtest -f config/raft_lab_test.yml
TEST 1: Initial election
TEST 1 Passed
TEST 2: Re-election after network failure
TEST 2 Passed
TEST 3: Basic agreement
TEST 3 Passed
TEST 4: Agreement despite follower disconnection
TEST 4 Passed
TEST 5: No agreement if too many followers disconnect
TEST 5 Passed 
TEST 6: Rejoin of disconnected leader
TEST 6 Passed
TEST 7: Concurrently started agreements
TEST 7 Passed
TEST 8: Leader backs up quickly over incorrect follower logs
TEST 8 Passed
TEST 9: RPC counts aren't too high 
TEST 9 Passed
TEST 10: Unreliable agreement
TEST 10 Passed
TEST 11: Figure 8
TEST 11 Passed
ALL TESTS PASSED
Total RPC count: 6609
```
If you used the legacy `make` flow, run:
```
$ ./build/labtest -f config/raft_lab_test.yml
```
The source for the tests is in `test/labtest.cc` and `test/labtestconf.cc`.


## Submit

Commit your code to the `lab-raft-solution` branch, with a message starting with "submit" (lower case).

## DO NOT

Make sure you do not do any of the following; otherwise you will fail the labs, even if the code may pass the testing scripts.

* Change/create files other than the files allowed. 
* Use shared variables between Raft instances. 
* Use file/network/IPC interface other than the RPC interface given to you.
