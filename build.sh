#./autoconf/AutoRegen.sh

./configure --disable-docs \
--disable-doxygen \
--disable-bindings \
--disable-jit \
--disable-libffi \
--enable-debug-symbols \
--with-llvmsrc=/home/zer0day/Projects/Inception/tools/llvm/llvm3.6 \
--with-llvmobj=/home/zer0day/Projects/Inception/tools/llvm/llvm3.6/build_u16 \
--disable-optimized

make -j16
