# Distributed Systems Labs in C++

MIT 6.824 style distributed systems lab rebuilt in C++. This project includes a series of labs in which you will build a transactional, sharded, fault-tolerant key/value storage system.

## Lab Assignments

- **Lab 1** - Replicated State Machine (Raft Consensus)
- **Lab 2** - Fault-tolerant Key-value Store
- **Lab 3** - Sharded Key-value Store

## Lab Environment

A modern Linux environment (e.g., Debian 12 x86-64) with 8-core/16G-memory is recommended for the labs. If you do not have access to this, consider using a cloud virtual machine. The labs possibly work on other environments (Mac, WSL, Other Linux distros, or with fewer CPU/memory resources) but it is not tested. 

## Getting Started

### Get Source Code

```bash
git clone --recursive [repo-addr]
cd janus
```
### Install Dependencies (Debian 12)

```bash
sudo bash apt_packages.sh
```

### Build

```bash
make clean
make labtest 
```
First time build could take time (10 minutes). You can add `-j32` to speed up building if you have enough CPU and memory.

### Running Tests

#### Raft Tests (Lab 1)
```bash
./build/labtest -f config/raft_lab_test.yml
```

#### KV Tests (Lab 2)
```bash
./build/labtest -f config/kv_lab_test.yml
```

#### Shard Tests (Lab 3)
```bash
./build/labtest -f config/shard_lab_test.yml
```

## Authors and Acknowledgements

Authors of the lab framework: 
- Shuai Mu
- Julie Lee
- Devika Sudheer
- Radhika Agarwal

Many of the lab structure and guideline text are adapted from MIT 6.824. 

Thanks for external users of the labs for feedback and fixes: Seo Jin Park (USC) and their students.

The code is based on academic prototypes of previous research works including but not limited to:

- **Mako**: [OSDI'25] "Speculative Distributed Transactions with Geo-Replication"
- **NCC**: [OSDI'23] "Natural Concurrency Control for Strictly Serializable Datastores by Avoiding the Timestamp-Inversion Pitfall" 
- **Janus**: [OSDI'16] "Consolidating Concurrency Control and Consensus for Commits under Conflicts"
- **Rococo**: [OSDI'14] "Extracting More Concurrency from Distributed Transactions"

## Additional Resources

- Course website: Check the guidelines on the course web page for lab-specific instructions.

## License

MIT
