#./autoconf/AutoRegen.sh

./configure --disable-docs \
--disable-doxygen \
--disable-bindings \
--disable-jit \
--disable-libffi \
--enable-debug-symbols \
--with-llvmsrc=../tools/llvm/llvm3.6 \
--with-llvmobj=../tools/llvm/llvm3.6 \
--disable-optimized

make -j16
