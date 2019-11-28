## The scalable metadata management service

The data management layer needs to provide the scalability for data put and the profile subscribe operation to support the large scale use cases. The metadata service is the core component to support large scale capability.

The data management service is divided into the raw data object service and the metadata service. This design assumes that the data partitions are fixed during the running time. 

The core problem is to solve how data is indexed in the scalable way. There are three questions need to be addressed separately.

1 how the metadata server is indexed according to the user's query (the query might contains spatial information such as the bounding box)

2 how the metadata on the specific metadata server is indexed

3 how the raw data on the data object server is indexed

### static partition of the simulation

In this case, we assume the partition of the simulation is fixed from the beginning of the simulation

During the initialization stage, all processes owned by data management service is divided into the master service (rank=0) and the raw data object service. (Every service is a process of the MPI which run distributed among different nodes, every process only need to start on one node since we provide the thread pool to use the multiple multiple CPU on each node)

Once the simulation starts, it sends the `init` RPC call to the data management service. This RPC call contains the bounding box for the specific partition (This RPC should be called when the operation of partitioning data at the simulation finish). The data service which receives the `init` RPC is selected by a round-robin policy. The service that accepts the `init` RPC will also become the metadata service at the same time. During the initializing step, the metadata server will send its address and the partition info (bounding box) to the master service. The master service will use the R tree (https://dl.acm.org/citation.cfm?id=602266) to maintain the partition received from specific metadata service. The leaf node of the R tree store the address of the metadata service.

When there is a subscribe operation, the metadata service will be acquired by asking the master service according to the partition of the R tree (it might be a list). Then the subscription will be sent to all the corresponding metadata services. If the R tree is not built yet, the subscription will be put at the master service, and the above operation will be put into a callback function which will be called after the R tree is built successfully.

When there is data put operation, the client will choose a raw data service by the hash value of the (varname, partitionID, step). It is also ok to use the random selection or the round-robin pattern. The goal is to distribute the data among all the available data object service evenly as much as possible. After the `put` operation, the raw data object service will ask the master service to get the address of corresponding metadata service for specific partition (by R tree search). Then the subscribed profile can be acquired by asking this metadata service. The filter manager will be created according to the content of the subscribed profiles. This information (profile and the metadata server for this data partition, the part of the R tree) will be stored at the specific data server for future reuse. The information at the metadata server will be updated after the data put. The checking service will be started asynchronously. If there is an unsubscribe operation, the profile and the filter manager will be deleted for the whole cluster. The R tree index could find the metadata that holds the corresponding profile, and the unsubscribe operation will be sent to the corresponding metadata service, the metadata service will send unsubscribe to all the corresponding data services.

### dynamic partition of the simulation

If the partition of the simulation is changing dynamically during the workflow running, it is hard to use the single R tree to maintain the spatial index for data at different steps.

One solution is to maintain several R trees at each step and distribute those R trees for each step to different metadata nodes (need to be explored further). Another solution is to use the DHT based on the space-filling curve. In this case, the data management service needs to know the bounding box of the global domain, then use the space-filling curve such as the Hilbert curve to transfer the 2d space into the 1d space, after the computing, every cell has one id in the 1d space. If we assume there are N metadata services, we could partition the whole 1d space into the N partition, and every metadata server is in charge of the specific span. (The communication here can be the MPI-based mechanism)

One disadvantage of this method is that the data partition from the put operation might not match with the domain that is managed by the specific metadata server perfectly, which means one partition from data put operation may relate with several metadata services. The advantage is that we don't need to maintain different R tree for index for every step with the variant data partition from the simulation. One optimization is to utilize the information from the producer (simulation) to adjust the id calculated from the space-filling curve dynamically (different group policy, need to explore further).

When there is data put, the information of the bounding box is also included, the id is calculated according to the space-filling curve, then the id of the metadata server can be calculated according to the method discussed above. In this case, the data partition from the put operation might relate with several metadata services, and every metadata server will be updated (the raw data object service will send the corresponding metadata to the list of the metadata service)

When there is data subscribed, the same operation can be used, the subscription that across several metadata services will be sent to the corresponding metadata service. The operation of calculating the meta server can be done at the client end. 