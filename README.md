Gorilla project is a in-memory data shared layer which is inspied by the DataSpaces and the ADIOS projects.
It build on the Mochi/Mercry RPC services.

It contains following services:

### UniMOS (Universal In-memroy object service)

The data is destributed in-memory object store for partitioned data in time series. The typical use case is for the scientific simulation using numeric methods. The data is indexed by the variableName:timestep:blockid(partition id). The UniMOS service could also be adapted to store the observing data.

### Installing

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


### performance summary

3 dimension 500 grid data, 10 steps, ~9s


### example

**Run UniMOS wih Gray-Scott Simulation**


### TODO list

add the monitoring interface

when put, there is shape info
add the get by offset (the offset can be multi dimentional) (ok for the function, expose rpc for next step)

implement the connect/gather for one element

store the summary for specific variable such as the latest timestep

update the client code to the async pattern