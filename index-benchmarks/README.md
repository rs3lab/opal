# OPAL Index Benchmarks


## Build
The `CMakeLists.txt` is setup to look for dependencies in the `~/local` directory.

### Install deps locally
#### Install glog:
```
wget https://github.com/google/glog/archive/refs/tags/v0.7.1.tar.gz
tar xf glog-0.7.1
cd glog-0.7.1
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=~/local
cmake build --build
cmake --build build --target install
```

#### Install jemalloc with `--disable-initial-exec-tls` flag
```
wget https://github.com/jemalloc/jemalloc/archive/refs/tags/5.3.0.tar.gz
tar xf 5.3.0.tar.gz
cd jemalloc-5.3.0/
./autogen.sh --disable-initial-exec-tls --prefix=/home/$USER/local
make -j
make install
```

### Build project
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
*Fix cmake for glog if required
make -jN
```

## Run with [PiBench](https://github.com/rs3lab/pibench)
Use `--batch_size=n` to set the batch size for OPAL.

We use jemalloc for memory management. See [this](https://github.com/jemalloc/jemalloc/issues/1237) if you encounter similar issues. 

Running OPAL B+Tree:
```
./_deps/pibench-build/src/PiBench ./wrappers/libbtreeofp_numa_wrapper.so --mode=time --pcm=False --threads=56 --records=100000000 --seconds=10 --read_ratio=0 --update_ratio=1 --distribution=SELFSIMILAR --skew=0.2 --skip_verify=False --apply_hash=False --bulk_load --batch_size=1000
```

## Run all benchmarks
```
../benchmarks/run.py
```
You might need to install several Python dependencies, including `numpy`, `pandas`, `matplotlib`, and `seaborn`. See the `requirements.txt` file in the [plots directory](../plots/README.md).


## Wrappers
Wrappers in bold are added by this fork.
|      Wrapper name          |          Content                |
|:---------------------------|:--------------------------------|
| btreeomcs_leaf_op_read     | B+-tree with OptiQL             |
|     btreeofp_nor_numa      | B+-tree with OPAL-NOR           |
|     btreeofp_numa          | B+-tree with OPAL               |
|     artomcs_op_read        | ART with OptiQL                 |
|     artofp_nor_numa        | ART with OPAL-NOR               |
|     artofp_numa            | ART with OPAL                   | 
