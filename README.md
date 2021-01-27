## Motivation

Gorilla framework is a in-memory data management servie. The name of the framework comes from the brand "gorilla glue", since we are basically gluing different components together. It mainly supoorts follwing capabilities:

(1) suppot M:N data put/get for data based on grid mesh. 

(2)User can use customized trigger to express the logic flow of the task executions. The implementation of in-memory data storage service layer is inspired by the [DataSpaces](https://github.com/philip-davis/dataspaces) and the [ADIOS](https://github.com/ornladios/ADIOS2) projects. 

(3)There is specific event queue binded with the trigger to support the data-driven task executions, the properties of the data can be captured and client can acquire the metadata of the raw data by poll events. The idea of data driven approach mainly comes from the [OSTI technical report](https://www.osti.gov/biblio/1493245).

**More key design strategies can be found at the designDoc/scratch.md**

## Compiling and running the server

this is an eample to compile the gorilla server on cori cluster

```
source ~/.gorilla
cmake ~/cworkspace/src/Gorilla/ -DCMAKE_CXX_COMPILER=CC -DCMAKE_C_COMPILER=cc -DVTK_DIR=~/cworkspace/src/VTK/build/ -DUSE_GNI=ON -DADIOS2_DIR=/global/cscratch1/sd/zw241/build_adios
```

If the paraveiw is used for particular test

```
cmake ~/cworkspace/src/Gorilla/ -DCMAKE_CXX_COMPILER=CC -DCMAKE_C_COMPILER=cc -DVTK_DIR=$SCRATCH/build_paraview_matthieu/ -DUSE_GNI=ON -DADIOS2_DIR=/global/cscratch1/sd/zw241/build_adios -DParaView_DIR=$SCRATCH/build_paraview_matthieu -DBUILD_SHARED_LIBS=ON
```

this is the content of the `~/.gorilla` file on cori cluster:

```
#!/bin/bash
source ~/.color
module load cmake/3.18.2
module load python/3.7-anaconda-2019.10
module load spack
#spack load cmake@3.18.2%gcc@8.2.0

module swap PrgEnv-intel PrgEnv-gnu
module swap gcc/8.3.0 gcc/8.2.0

spack load -r mochi-thallium
spack load mochi-cfg
spack load -r mochi-abt-io

export CRAYPE_LINK_TYPE=dynamic
cd $SCRATCH/build_Gorilla

export MPICH_GNI_NDREG_ENTRIES=1024 
# get more mercury info
export HG_NA_LOG_LEVEL=debug
```

refer to the ./scripts dir to check exmaples of running multiple servers. The configuration of the server contains item such as protocol used by communication layer, the log level, the global domain and if the trigger is started and so on. The example of the configuration is in ./server/settings.json

### run

exmaple on cori
```
srun -C haswell -n 8 ./unimos_server ~/cworkspace/src/Gorilla/server/settings_gni.json
```
remember to set the env if MPICH is used

```
MPICH_GNI_NDREG_ENTRIES=1024
```

simple example to put the data

```
srun -C haswell -n 16 ./example/gray-scott-stg ~/cworkspace/src/Gorilla/example/gssimulation/settings.json gni
```

simple example to get the data for further processing

```
srun  -n 4 ./example/isosurface ~/cworkspace/src/Gorilla/example/gssimulation/settings.json 10 0.5 gni
```

### Version info

v0.1

M:N put get for Cartesian grid

memory and file backend 
(file backend will be used when there is not enough mem space)

in-memory data trigger (experimental)


### related issue


```
/usr/bin/ld: /global/common/sw/cray/sles15/x86_64/mesa/18.3.6/gcc/8.2.0/qozjngg/lib/libOSMesa.so: undefined reference to `del_curterm@NCURSES6_TINFO_5.0.19991023'
```
try this:

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ltinfo")

refer to

https://github.com/halide/Halide/issues/1112