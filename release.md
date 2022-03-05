## Release format
### Steps
1. Each new feature should account as minor release
2. Must include all the previous features list with feature identifier numbers
3. A combined feature list

current version : 1.0.0

Release Notes
=================
1. Fix the bug in reporting of statistics by dbtest (pseudo-client)
2. Add release note.

Release 1.0.0

1. Fix for correct statistics report by `dbtest` client
2. new configurable changes
    - `ALLOW_FOLLOWER_REPLAY` : Flag to enable follower replay
    - `ALLOW_WAIT_AFTER_SUBMIT` : Flag for making paxos worker to finish the processing after each submit @depricated
    - `ALLOW_WAIT_AT_PAXOS_END` : Defualt enabled flag indicating wait for each leader/follow to finish processing