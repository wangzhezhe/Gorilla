### Evaluation plan

This document shows the evaluation plan for the in-staging dynamic task trigger service.

#### Comparable solutions

Baseline_1: Using the put/get interface of the DataSpace to organize the workflow.

Baseline_2: Using the put/get interface of our in-staging service to organize the workflow.

Experimental Solution: Using in-staging dynamic task trigger to organize the workflow. 

Since we need a more efficient in-memory management thread management tool to support the in-staging data execution, our solution provides the middleware that uses a similar architecture with the DataSpaces based on the Mochi and Argobot RPC service. Besides, we also add the capability of the in-memory data task execution and the programmable dynamic task trigger interface.

Before the evaluation, we first show that our middleware is similar with DataSpaces for the overhead of data put/get.

#### Performance and Scalability

The workflow used to evaluate the performance of the scalability is executing one data analytics in staging service.

The metric for this evaluation is the **task trigger time**. In particular, it is the time period from the beginning of the data put at the simulation to the beginning of the analytics. We also record how much time is consumed in data put and how much time is consumed in task execution in staging.

Weak scale: fix the rate of \<number of simulation process\>/\<number of staging process\>, and increase the number of the simulation task.

Strong scale: fix the setting of the data producer (number of the process and the size of the data for each process), modify the number of the staging process.
 
#### Resource utilization

We hope to show that, compared with the original solution (using put/get interface), in-staging dynamic task trigger better utilize the computing resource at the staging nodes and execute the same workflow by fewer computing resources in similar execution time. The workflow used in this case is the same as the workflow described at the [documentation for the use cases](./architectureAndUsecase). 

There are two factors associated with this experiment, the first one is the percentage of the interesting data, and the second one is the resource used by the whole workflow. We could pick three typical cases about the percentage of the interesting data (low, medium, high) and vary the resource utilized by the workflow to compare the execution time.

#### The flexibility of programming

In this part, we aim to show the flexibility of the in-staging dynamic task trigger. Specifically, we will show how to use the dynamic task trigger to express the following scenarios and show the related results: 

Critical area detection: use the threshold filter to extract the interesting domain and output the interesting domain to persistent storage.

Data error detection: detect the error at the simulation data and notify the user if there is error detection.

Blob detection: calculate the isosurface of the simulation data and detect the number of blobs based on simulated value.

The first two operations can be achieved by executing the action for the separate data partition, but the third operation needs to be executed based on aggregated data (there is no efficient parallel version of this algorithm).
