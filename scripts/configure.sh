cmake \
-DCMAKE_BUILD_TYPE=DEBUG \
-DCMAKE_C_COMPILER=mpicc \
-DCMAKE_CXX_COMPILER=mpicxx \
-Dvt_DIR=$VT_DIR \
-DYamlCpp_ROOT_DIR=$YAMLCPP_DIR \
..