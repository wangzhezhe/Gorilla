
Gorilla project 

### INX (Indexing service for scientific workflow)

INX is the indexing service for the simulation, it mapps the from the userview to the dataview. Spacifically, it use vtkm to hand the mesh representation and the field data for the dataset of the vtkm is the indexing to the underlying data service instead of the real data.


### UniMOS (Universal In-memroy object service)

the data is destributed in-memory object store for partitioned data in time series. The typical use case is for the scientific simulation using numeric methods. The data is indexed by the variableName:timestep:blockid(partition id).

the object store is desgined to store the filed data for the simulation, for the mesh data, it is better to integrate IMOS with INX.


### installing

use the ./spack/package.yaml to set up the thallium env, copy this file into the ~/.spack/linux/


for example, if the current dir is the build dir

```
module load openmpi
module load gcc
source ~/cworkspace/src/spack/share/spack/setup-env.sh
spack load -r thallium
spack load -r abt-io
//make sure the cmake use the suitable gcc version, or let the spack find the correct gcc version
cmake  ~/cworkspace/src/Gorilla -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
```

run the unimos server and client

```
# use srun to start the server
srun --mem=2000 ./unimos_server

# use srun to start the client
srun --mpi=pmi2 --mem-per-cpu=1000 -n 16 ./unimos_client tcp://... (if the server start by verbs, using verbs://...)
```


### example

**use the UniMOS**

**use the ADIOS engine**
