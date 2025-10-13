  # 1. Create a new orphan branch (no parent commits)
  git checkout --orphan lab-25-clean

  # 2. Add all files to the new branch
  git add -A

  # 3. Create the initial commit
  git commit -m "Initial commit: Lab-25 skeleton code

  This repository contains skeleton code for three distributed systems labs:
  - Lab 1: Raft consensus algorithm
  - Lab 2: Key-Value service with Raft
  - Lab 3: Sharded Key-Value service with Shard Master
  ..."

  # 4. Delete the old branch with history
  git branch -D lab-25

  # 5. Rename the clean branch to lab-25
  git branch -m lab-25

  # 6. Force push to remote (overwrites remote history)
  git push -f dslabs lab-25

