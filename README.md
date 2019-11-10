
# Contents
1. [Motivation](#introduction)
2. [In-memory data storage](#storage)
3. [In situ data checking](#checking)
4. [Trigger based task controller](#controller)
5. [Installing](#install)
6. [Example](#examples)


## Motivation <a name="motivation"></a>

Gorilla project is an in-memory data management and workflow service for scientific applications such as physical simulation or observing system. This project aims to evaluate the idea of in-situ data management (ISDM) proposed at the [OSTI technical report](https://www.osti.gov/biblio/1493245).

Peterka, Tom, Bard, Deborah, Bennett, Janine, Bethel, E. Wes, Oldfield, Ron, Pouchard, Line, Sweeney, Christine, and Wolf, Matthew. ASCR Workshop on In Situ Data Management: Enabling Scientific Discovery from Diverse Data Sources. United States: N. p., 2019. Web. doi:10.2172/1493245.

In general, the goal of the Gorilla project is to achieve the workflow management shown in the following figure (The figure comes from the osti report) to use in-situ analysis and trigger-based mechanism to relieve I/O bottleneck of the workflow.

<img src="fig/dynamic_workflow_ensemble.png" alt="drawing" width="300"/>

Specifically, the Gorilla project contains three parts, the in-memory storage service, in-memory data checking service and trigger-based task controller service. 

## In-memory data storage <a name="storage"></a>

The main goal of the in-memory data storage service is to provide the capability to put and get data between the application and the data staging service. The implementation of this layer is inspired by the [DataSpaces](https://github.com/philip-davis/dataspaces) and the [ADIOS](https://github.com/ornladios/ADIOS2) projects.


#### Networking and RPC service

We adopt the [Mochi](https://mochi.readthedocs.io/en/latest/) software stack for networking and the RPC layer. Specifically, the Thallium project is used as the wrapper to implement the RPC service (Margo and Mercury project) and the Argobot is used to manage the threads. The Mercury project could provide multiple data transfer strategies such as TCP, verbs, and shared memory.

#### Data representation and data index

The typical data from the scientific application can be divided into the object data and the metadata. For example, the data generated from the physical simulation includes the field data (object data) and the mesh data (metadata). For simplicity of the explanation, let us take the image data as an example.

For example, the metadata of the image data may include the variable name, step, lower bound and the upper bound, and the field data is the real value at every cell (pixel). If we view the field data from the storage's perspective, it is one dimension array essentially. The application view represents how those data organized from the view of the application, for example, two adjacent data point in storage view can be at a different position at the application view. The simple representation is the bounding box includes the lower bound and the upper bound or offset and the data shape (the coordinates of the data point overlap with the position of the cell in mesh).

The simulation usually uses multiple processes to update the data at every step. It needs the strategy to map the id of the process/thread into specific data partition. For example, the Gray-Scott simulation in the example folder use the following strategies to map the data partition to a specific MPI process:

```
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    MPI_Dims_create(procs, 3, dims);
    npx = dims[0];
    npy = dims[1];
    npz = dims[2];

    MPI_Cart_create(comm, 3, dims, periods, 0, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 3, coords);
    px = coords[0];
    py = coords[1];
    pz = coords[2];

    size_x = (settings.L + npx - 1) / npx;
    size_y = (settings.L + npy - 1) / npy;
    size_z = (settings.L + npz - 1) / npz;

    offset_x = size_x * px;
    offset_y = size_y * py;
    offset_z = size_z * pz;
```

In order to make the storage layer suitable for multiple data resources, we also add a block id into the metadata to represent the specific data partition. The logic that how to relates the partition id with the data partition depends on how simulation partition the data. For example, in Gray-Scott simulation, the block id is the same with the rank id of the process.

There are two benefits to use the block id to represent the block data

(1) the first is that it makes the storage layer more general and decouple the metadata with the field data data.
(2) it simplifies the data consumer in some cases, since it does not need to recalculate the bounding box of the data partition, the information of the offset can be stored at the metadata and put into the staging service when simulation put the data.

The strategy of the data index is inspired by the ADIOS project. Specifically, the data can be indexed by the variable name, step, and the bounding box ar the block id. For example, This is the interface from the `Variable.h` file of ADIOS:

```
    void SetBlockSelection(const size_t blockID);
    void SetSelection(const adios2::Box<adios2::Dims> &selection);
```

There are several cases to show how to index data by those two methods:

> 1 the data consumer process the data that matches with the data partition of the simulation

For example, there are 4 MPI processes at simulation, and every process calculates a specific region of the data then output to the staging service. If there are also 4 processes at the consumer, each process will get the data by object id (rank id in this case) or calculate the corresponding bounding box then get the data by bounding box.

> 2 the data consumer process the data that is the partial area of the specific data partition of the simulation

In this case, the first step is to know that the partial area belongs to which data partition of the simulation, this can be calculated by a function that maps the bounding box into the block id. The user controls this function since the simulation can partition the data with different strategies. The strategy of caculting the id should same with the strategy of the data partition at the simualtion. Then the data can be indexed with the block id and the specific bounding box. Only the data within the specific bounding box will be transferred back from staging service to the client.

TODO: Another strategy is to let specific slave service to provide the capability that map from the bounding box into the block id. Since all the block within specific step is located on the same serivce (details of the deistributed mechanism is explained at the following parts).

> 3 the data consumer process the data that includes several data partition

In this case, the first case is to calculate the list of the block id within this domain and fetch the data from the staging service according to the variable, step, and block id. Since the data may be located at a different node, they will be merged together at the client-side.


> 4 hybrid pattern of case 2 and case 3


#### Distributed strategy

The master-slave pattern is used as the distributed strategy of the staging service. During the stage of the initialization, the slave node will register their address into the master process (process with rank 0). The master service will write the address into the configuration file when the registration step finishes. 

When the client sends the API to the server, they will first ask the master which server it could use, and then the client will send the RPC to the slave nodes. The strategy of the getting slave nodes can be extended easily, and it only needs to provide a new function that gets the address from the list of the slave address. Currently, we use the `std::hash<std::string>` to map the ` <variablevame>_step` into the id, than use `id%<number of server process>` to get the specific server address. The reason for this setting is that the simulation always runs multiple steps, the number of the step is larger than the number of the staging service. The size of the server usually far more large (hundreds of GB) than the data from one step (several GB). So we put the data from the different steps on different staging services.

The master node will become the bottleneck if there is a large amount of the client sending the request at the same time. The solution to release this simulation is to set multiple master servers, for example, both the process with rank 0 and rank 1 can be the master server and the burden will be reduced by half. Similarly, we could adopt more services according to the specific use cases.


#### Interfaces of the data put and get

> put data by bounding box

descriptions: put the data into the staging service 

parameters: variable name, step, lower bound, the shape of the data object

return value: the status of the data put

> get data by bounding box 

descriptions: get the data from the staging service

parameters: variable name, step, lower bound, the shape of the data object

return value: the metadata of the object

> put data by block id

descriptions: put the data into the staging service, the block id is usually 
set as the rank id of the MPI process

parameters: variable name, step, lower bound, data shape, block id

return value: the status of the data put operation

> get data by block id

descriptions: get the data from the staging service

parameters: variable name, step, block id

return value: the metadata of the object


We only support case 1. and 2. (discussed at `Data representation and data index`) when getting the data by bounding box currently. 

#### Performance testing

coming soon

## In situ data checking <a name="checking"></a>

The in-situ data checking service at the staging service could check the content of the data in-memory and make the decision according to specific user-defined requirements. It includes two main components. The first is the data checking engine, which checks the data according to the subscription from the user. The second one is the mechanism to trigger the task according to the checking results. The idea of data-driven and in-situ checking service is inspired by the [Meteor project](https://onlinelibrary.wiley.com/doi/full/10.1002/cpe.1278).

The staging service provides a subscribe interface. The input of this interface is a profile that describes the data checking operation. The data structure of this profile is listed as follows:

```
struct FilterProfile{
    ...
    std::string m_profileName="default";
    std::string m_stepFilterName="default";
    std::string m_blockIDFilterName="default";
    std::string m_contentFilterName="default";
    std::string m_subscriberAddr;
```

The data checking engine will generate concrete data constraints manager according to the name of the filter. For example, if the `m_contentFilterName="default"`, constraints manager will bind with the function called default. This function is defined at the constraints manager. If we need a customized function, we need to program the function that returns true or false and put it into the constrains manager, then we could make the constraints manager bind with the customized function by using the corresponding name of the function in the profile. This is the sample of the default data function that we use for the proof of the concepts:

```
bool defaultStepFilter(size_t step)
{
    std::cout << "default StepFilter" << std::endl;

    return true;
}

bool defaultBlockFilter(size_t blockID)
{
    std::cout << "default BlockFilter" << std::endl;
    return true;
}

bool defaultContentFilter(void *data)
{
    std::cout << "default ContentFilter" << std::endl;
    return true;
}
```

Since the data with the same variable name may at all slave service, we need to send subscription operation to all the slave service. In order to decrease the unnecessary overhear, the client just needs to send one subscription to the master service, and then the master service will broadcast this profile to all the slave service.

When the data checking service is loaded into the staging service, we will start a new thread asynchronously to check the data according to the user-defined profile. This thread is managed by Argoboot library. For example, this is the specific logic to execute the data checking operation:

```
bool constraintManager::execute(size_t step, size_t blockID, std::array<size_t, 3> shape, std::array<size_t, 3> offset, void *data)
{

    bool stepConstraint = this->stepConstraintsPtr(step);

    if (stepConstraint == false)
    {
        return false;
    }

    bool blockConstraint = this->blockConstraintsPtr(blockID);

    if (blockConstraint == false)
    {
        return false;
    }

    bool boxConstraint = this->boxConstraintsPtr(shape, offset);

    if (boxConstraint == false)
    {
        return false;
    }

    bool contentConstraint = this->contentConstraintPtr(data);

    return contentConstraint;
}
```

The constraints of the step, block, and the data content will be executed step by step. This function is only in the beta version currently.

For the future goal, we plan to integrate the data checking service targeted on more specific using scenarios. There are some examples of the in-situ filter to reduce the data or detect interesting data patterns.

(1) filter based on data sampling

https://arxiv.org/pdf/1506.08258.pdf

(2) filter based on precision deduction

https://www.osti.gov/servlets/purl/1338570

(3) filter based on the machine learning/deep learning model

https://www.nature.com/articles/s41586-019-1116-4

https://dl.acm.org/citation.cfm?id=3291724

When specific data finish the execution function listed above, it will send the notification back to the subscriber. This notification contains the metadata of the real data. The subscriber could do the following tasks based on this metadata.

There is a workflow that implements the process mentioned above at the `scripts/testnotify.scripts`.


## Task controller and dynamic trigger <a name="controller"></a> 

The task controller is still in the beta version. Currently, it could accept the notification from the staging service. For the future goal, it will maintain a table that contains all the execution results of the data block it subscribed for every step. Then the specific task fill be triggered according to this table. For example, if the task controller subscribes to 4 data block and the execution results for step 1 from those 4 blocks are all false, the specific task will not be triggered. If some of them are true, the specific task defined at the task controller will be triggered with the metadata. This part can be modified flexibly based on specific use cases. 


## Installing <a name="install"></a> 

use the ./spack/package.yaml to set up the thallium env, copy this yaml file into the ~/.spack/linux/

for example, if the current dir is the build dir (it is better to put following contents in .gorilla of ~/.gorilla.sh)

```
module load openmpi
module load gcc
source ~/cworkspace/src/spack/share/spack/setup-env.sh
spack load -r thallium
spack load -r abt-io

//make sure the cmake use the suitable gcc version, or let the spack find the correct gcc version
cd /home/zw241/cworkspace/build/build_Gorilla
cmake  ~/cworkspace/src/Gorilla -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DVTK_DIR=~/cworkspace/build/build_vtk

for testing use --cpus-per-task
```

run the unimos server and client on same dir, the address of the master service is stored at the unimos_server.conf

```
# use srun to start the server
srun --mpi=pmi2 --mem-per-cpu=1000 -n 8 ./unimos_server verbs 1
pmix_v2 for mpi 3.x

# use srun to start the client
srun --mpi=pmi2 --mem-per-cpu=1000 -n 16 ./unimos_client verbs
(if the server start by verbs, using verbs://...)
(the master address is written to the shared file system)
pmix_v2 for mpi 3.x
```


## Examples <a name="examples"></a>

**Run UniMOS wih Gray-Scott Simulation**

star the unimos server

```
srun --mpi=pmix_v2 --mem-per-cpu=200000 -n 4 ./unimos_server verbs 1
```

start the simulation and put the data

```
srun --mpi=pmix_v2 -n 4 ./example/gray-scott ~/cworkspace/src/Gorilla/example/simulation/settings.json
```

star the analytics to pull the data

```
# the first parameter is the number of steps and the second parameter is the threshold value for isosurface
srun --mpi=pmix_v2 -n 4 ./example/isosurface 20 0.5
```


### TODO list

update the put/get interface to the async mode

memory hierachy, set the limitation of the memory, what if the data is full, set the policy, if the old data can be covered

modify the get interface to make the writer and reader run concurrently

add the monitoring interface

when put, there is shape info
add the get by offset (the offset can be multi dimentional) (ok for the function, expose rpc for next step)

implement the connect/gather for one element

store the summary for specific variable such as the latest timestep

update the client code to the async pattern