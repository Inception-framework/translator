DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

./autoconf/AutoRegen.sh

./configure --disable-docs \
--disable-doxygen \
--disable-bindings \
--disable-jit \
--disable-libffi \
--enable-debug-symbols \
--with-llvmsrc=$DIR/../tools/llvm/llvm3.6/ \
--with-llvmobj=$DIR/../tools/llvm/build_debug \
--disable-optimized

make -j16

sudo make install
