
# build pcd
echo "Build PCD process"
cd ./apps/space-pcd-master/pcd-1.1.6/
make clean
make pcd
cd ../../../

echo "Build DPDK processes"
export EXTRA_CFLAGS='-o0 -g'
make

# copy the binaries
echo "Copy the binaries to bins directories"
cp apps/app_manager/apps/app_manager/x86_64-native-linuxapp-gcc/app_manager ./bins/
cp apps/dispatcher/apps/dispatcher/x86_64-native-linuxapp-gcc/dispatcher ./bins/
cp apps/space-pcd-master/pcd-1.1.6/bin/target/usr/sbin/pcd ./bins/
