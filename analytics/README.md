Steps to generate the statistics for Read/Write testcase
---

1. Write to file (***Serialize***)
    - Buffered (? Failed in creating unbuffer log write)
    - flush controlled by filesystem deamon directly
    - Performance is bad while writing to files
    - Steps for performance graph
    
    Logs are generated by fixing the few parameters as:
        - NUMA Size : 4G
        - db type : mbta
        - benchmark type : tpcc
        - run time : 40 Sec
        
    Case A:
        - Scale and Number of threads are same and increasing linearly
        - Number of thread count from (1,2,3,..,31)
    Case B:
        - Scale is constant but Number of threads are increasing linearly
        - Number of thread count from (1,2,3,..,31)
    
    For every successful thread count, a collection of log will be generated.
    
## TODO Serialize
    1. Generate graphs with average throughput while write
    - This is simple and code is already implemented
    
    2. Generate graphs for each thread count, 
    explaining the performance during the execution, not average
        
2. Read from file (***Replay***)
    - No read buffer
    - Whole Log is loaded at once
    - Majority of individual log size is ~500-600 MB size
    - Performance is good when replay

## TODO Replay
    1. Generate graphs with average throughput while replay for each collection of logs
    - This is simple and code is already implemented
