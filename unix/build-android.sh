# Requires standalone toolchain (as made by
# make-standalone-toolchain.sh from NDK).
# See docs/Programmers_Guide/html/md_3__key__topics__building__s_t_a_n_d_a_l_o_n_e-_t_o_o_l_c_h_a_i_n.html
# under SDK dir ("Invoking the compiler (the easy way)" subsection).

# Change for other architectures or toolchains
export CROSS_COMPILE=arm-linux-androideabi-
make deplibs

# LDFLAGS_ARCH= suppresses map file generation, which
# causes android-ndk-r10e gcc to fault
make MICROPY_STANDALONE=1 MICROPY_PY_JNI=1 LDFLAGS_ARCH= V=1 -B
make MICROPY_STANDALONE=1 MICROPY_PY_JNI=1 LDFLAGS_ARCH= V=1 lib

# Automatically install on device/emulator for testing
adb push micropython /data/local/tmp
