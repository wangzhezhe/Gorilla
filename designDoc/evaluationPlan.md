### Evaluation plan

This document shows the evaluation plan for the in-staging dynamic task trigger service.

#### Comparable solutions

Baseline: Using put/get interface of the DataSpace to organize the workflow.

Our Solution: Using in-staging dynamic task trigger to organize the workflow. 

Since we need more efficient in-memory management thread management tool to support the in-staging data execution, our solution provide the middleware that use the similar architecture with the DataSpaces based on the Mochi and Argobot RPC service. Based on this service, we add the capability of the in-memory data task execution and the programmable dynamic task trigger interface.

Before the evaluation, we first show that our middleware is similar with DataSpaces for the overhead of put/get.

#### Performance of the scalability

The workflow used to evaluate the performance of the scalability is executing one data analytics in staging service.

weak scale: fix the rate of \<number of simulation process\>/\<number of staging process\>, and increase the number of the simulation task.

strong scale: fix the setting of the data producer (number of the process and the size of the data for each process), modify the number of the staging process.
 
The metric is the task trigger time. In particular, it is the time period from the beginning of the data put at the simulation to the beginning of the notification operation in staging service. We also record how much time is consumed in data put and how much time is consumed in task execution in staging. 

#### Resource utilization

We hope to show that, compared with the original solution (using DataSpaces), in-staging dynamic task trigger better utilize the computing resource at the staging nodes and execute the same workflow by less resources with similar execution time. The workflow used in this case is the same as the workflow described at the [documentation for the use cases](./architectureAndUsecase).

#### The flexibility of programming

In this part, we aim to show the flexibility of the in-staging dynamic task trigger. Specifically, we will show how to use the dynamic task trigger to express the following user scenarios and show the related resutls: 

Blob detection: calculate the isosurface of the simulation and detect the number of blobs based on simulated value.

Critical area detection: use the threshold filter to extract the interesting domain and output the interesting domain to persistent storage.

Data error detection: detect the error at the simulation data and notify the user if there is error detection.