# producer-consumer-server
Course Project on Operating Systems on C

Projects includes threads, mutexes/semaphores, multiplexing, file descriptors, etc.

### 1. make all
### 2. run server
./pcserver [host(optional)] [bufsize] 
### 3. run producer && consumer
./producer [host] [producer_num] [arrival_rate] [bad_prod_percent] 
./consumer [host] [producer_num] [arrival_rate] [bad_prod_percent]
### 3. run status client
./status [host]
