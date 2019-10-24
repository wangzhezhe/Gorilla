
Gorilla project 

Gorilla is a in-memory data shared layer which is inspied by the DataSpaces and the ADIOS projects.
It build on the Mochi/Mercry RPC services.

### INX (Indexing service for scientific workflow)

INX is the indexing service for the simulation, it mapps the from the userview to the dataview. Spacifically, it use vtkm to hand the mesh representation and the field data for the dataset of the vtkm is the indexing to the underlying data service instead of the real data.


### UniMOS (Universal In-memroy object service)

the data is destributed in-memory object store for partitioned data in time series. The typical use case is for the scientific simulation using numeric methods. The data is indexed by the variableName:timestep:blockid(partition id).

the object store is desgined to store the filed data for the simulation, for the mesh data, it is better to integrate IMOS with INX.


### installing

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
cmake  ~/cworkspace/src/Gorilla -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc 
for testing use --cpus-per-task
```

run the unimos server and client

```
# use srun to start the server
srun --mpi=pmi2 --mem-per-cpu=1000 -n 8 ./unimos_server verbs 1

# use srun to start the client
srun --mpi=pmi2 --mem-per-cpu=1000 -n 16 ./unimos_client tcp
(if the server start by verbs, using verbs://...)
(the master address is written to the shared file system)
```


### todo list

use the blockMeta instead of the dataMeta

finish the interface for checking the existance of the blockid and return the blockMeta

when put, there is shape info
add the get by offset (the offset can be multi dimentional) (ok for the function, expose rpc for next step)

update the put and get, use the simplified interface to express that
for get, it needs to get metadata firstly then get the real data

implement the connect/gather for one element

store the summary for specific variable such as the latest timestep

update the client code to the async pattern

### example

**use the UniMOS**

**use the ADIOS engine**
