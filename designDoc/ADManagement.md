

ideas about the elastic in-situ anlytics based on data staging service


### Background and motivation

main research direactions about the in-situ data management/workflow in high level

| main direactions |  static |  dynamic/adaptive  | models to guide the dynamic strategy |
|---------|----------|---|---|
|where to place the data and how to transfer data | shared memory/RDMA + memory hierachies, or data compression before transfering | prefetch the data between between nodes/memory hierachies, compress the data when it is necessary | models based on data access pattern|
|where to place the in-situ analytics|simulation core, helper core, staging core, offline core, make decision before workflow start | switch places of analytics to save the workflow execution time | monitor specifc metric and use heuristic model (FlexIO) |
|how to deploy/run in-situ analytics| compiled before workflow starts, run on different accelarators|  interactive pattern, such as providing the python scripts or using the containers  | -- |
|when to run in-situ analytics|  run task in a fixed frequency, sequential pattern  |  data-driven pattern based on indicator-trigger mechanism | the model (indicator) depends on user's requirements |
|how much resources allocated to the in-situ analytics| the computing resources are fixed during the workflow execution | the resource can be scaled up/down during the workflow execution (thread, process, node, job)  | need to be explored further|
|fault tolerance about the in-situ analytics|  check the data and rerun the workflow  |  recover the workflow from the check point | model based on error detections|


The goal of the adaptive or dynamic data management is to make the proper option among the strategy space for different configurations/platform/context of the in-situ workflow to achieve specific goal such as improving the performance or decreasing the cost.

Previous work (https://rucore.libraries.rutgers.edu/rutgers-lib/49242/PDF/1/play/) has summarized the background, motivation, and exploration for typical adaptions for different layers in the scientific workflow, including the application, middleware, and resources used by in-transit analytics. However, there are still several problems that needed to be explored further.

1. The framework that implements the adaption is tightly coupled with the solutions that run the in-situ analytics and data management service in previous work. This limits the availability of the adaption service, such as monitoring and makes decisions. For example, the simulation may want to use the catalyst as the in-line processing and use other data staging services as the in-transit processing when it is necessary. It is necessary to provide a scalable and extensible solution that monitors the workflow status, makes decisions according to user-defined policies, and conveys the decision status to the dedicated agents to implement these decisions.

2. Although there are some abstractions such as data monitor, adaptive operator, adaptive mechanism conceptually in previous work (https://rucore.libraries.rutgers.edu/rutgers-lib/49242/PDF/1/play/), there still lacks a framework to insert the policy and monitored variables flexibly. For example, the policy/model that guides data placement, task placement, and resource allocation can be used simultaneously in the workflow. Current solutions tightly bind with the specific adaption solution, which lacks a high-level programming abstraction to extend new models/policies.

3. It lacks the infrastructure to support the model that updates the predicts/decision during the workflow runtime. How the corresponding agent executes the decision made by the policy engine need to be explored further for different adaption layers.

In this work, we more focuse on how to build a infrastructure that can support multiple adaptive strategies

1. we first try to describe the infrastructure that support the adaptive strategies for different layers

2. then we try to explains how the decision is made and executed based on this framework during the worklow running in near-real-time

3. then we try to integrate several adaption policies (there policies comes from the previous work) into the framework to check the performance of the in-situ workflow with multiple adaptive policies

(
  
  task placement (based on previous work about where to run apecific anlytics)
  
  data placement (based on this https://dl.acm.org/doi/pdf/10.1145/2807591.2807669), 
  
  resourcesused by in-transit analytcis (based on the model to udpated the thread pool's size for in-situ analytics)
  
)

### subquestions needed to be solved

**infrustructure**

We need to provide the mechanism that monitors the workflow's key parameters, such as time spent on data I/O, simulation, and analytics during the workflow execution. These parameters are supposed to be the input of the model, and the new monitored variables can be added into the infrastructure.

The infrastructure needs to change the strategies (such as where to run the analytics) according to the latest decisions made by the mathematical model.

We need to provide a flexible mechanism to notify the agent about the decisions made by the policies for adaptive data management.

**model** 

We use the mathematical method to model the workflow execution time when there are different in-situ data placement strategies in previous work, but there is a gap to use that model automatically. Some parameters are estimated value, such as the percentage of the interesting data, the distribution of the interesting data. The model needs to be updated to solve these problems.

The mathematical method still needs to be updated to find a suitable thread pool's size that runs the analytics. The thread pool's size might also influence other parameters, such as the time spent on data I/O.

### References

FlexIO: https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=6569822

Challenges of elastic in-situ analytics: https://dl.acm.org/doi/10.1145/3364228.3364234

The adaptive data management: https://rucore.libraries.rutgers.edu/rutgers-lib/49242/PDF/1/play/

data placement within the data staging service: https://dl.acm.org/doi/pdf/10.1145/2807591.2807669