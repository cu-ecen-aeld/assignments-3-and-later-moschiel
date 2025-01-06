#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
IS_64BIT=1

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

SOURCE_KERNEL_IMAGE=${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image
if [ ! -e ${SOURCE_KERNEL_IMAGE} ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # mrproper for "deep clean" the kernel build tree - removing the .config file with any existing configurations 
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    # defconfig to set default configurations for our "virtual" ARM dev board we will simulate in QEMU
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # all to build the kernel image for booting with QEMU, the 'j' allows multicore running the compiling to make it faster (j8 for 8 cores at once)
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # build any kernel modules
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    # build the device tree
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"
cp "${SOURCE_KERNEL_IMAGE}" "${OUTDIR}"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
# create base directories
mkdir "${OUTDIR}/rootfs"
cd "${OUTDIR}/rootfs"
mkdir -p bin dev etc home proc sbin sys tmp usr var lib
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
if [ "$IS_64BIT" -eq 1 ]; then
    mkdir -p lib64
    mkdir -p usr/lib64
fi


cd "${OUTDIR}"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    # configure BusyBox by starting with the default configuration, which enables pretty much all of the features of BusyBox:
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
# crosscompile busybox binaries
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
#configure path to install busybox binaries on base directory, then install it
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install


# TODO: Add library dependencies to rootfs
echo "Library dependencies"
cd "${OUTDIR}/rootfs"
echo "  Searching 'program interpreter' in the cross-compiled busybox binary"
# Inspect the busybox executable to verify what is the program interpreter
PROGRAM_INTERPRETER=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter")
# Extract the part after ': ' using awk
PROGRAM_INTERPRETER=$(echo "$PROGRAM_INTERPRETER" | awk -F': ' '{print $2}')
# Remove the square brackets ('[' and ']') using tr
PROGRAM_INTERPRETER=$(echo "$PROGRAM_INTERPRETER" | tr -d '[]')
echo "    Found Program interpreter: $PROGRAM_INTERPRETER"

echo "  Searching 'shared libraries' in the cross-compiled busybox binary"
# Inspect the busybox executable to extract all lines that match "Shared library"
RAW_SHARED_LIBRARIES=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library")
# Initialize an empty array to store the cleaned library paths
SHARED_LIBRARIES=()
# Loop through each line of the raw output
while IFS= read -r LINE; do
    # Extract the library name after ': ' using awk
    LIBRARY_WITH_BRACKETS=$(echo "$LINE" | awk -F': ' '{print $2}')
    # Remove the square brackets ('[' and ']') using tr
    LIBRARY=$(echo "$LIBRARY_WITH_BRACKETS" | tr -d '[]')
    # Add the cleaned library path to the array
    SHARED_LIBRARIES+=("$LIBRARY")
done <<< "$RAW_SHARED_LIBRARIES"

echo "    Found Shared libraries:"
for LIB in "${SHARED_LIBRARIES[@]}"; do
    echo "      $LIB"
done


SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot) # Path to the toolchain sysroot
#libraries folder for searching dependencies
LIB_DIR=lib
if [ "$IS_64BIT" -eq 1 ]; then
    LIB_DIR=lib64
fi

echo "  Copying toolchain's Program Interpreter to rootfs..."
if [ -n "$PROGRAM_INTERPRETER" ]; then
    SOURCE_INTERPRETER="${SYSROOT}${PROGRAM_INTERPRETER}"
    DEST_INTERPRETER="${OUTDIR}/rootfs/${LIB_DIR}"
    echo "    Copying Program Interpreter: ${PROGRAM_INTERPRETER}"
    cp -a "${SOURCE_INTERPRETER}" "${DEST_INTERPRETER}"

    # If the program interpreter is a symbolic link, copy its target
    LINK_TARGET=$(readlink -f "${SOURCE_INTERPRETER}")
    #echo $LINK_TARGET
    #echo $SOURCE_INTERPRETER
    if [ "$LINK_TARGET" != "${SOURCE_INTERPRETER}" ]; then
        echo "    Copying symbolic link target for program interpreter: ${LINK_TARGET}"
        cp -a "$LINK_TARGET" "${DEST_INTERPRETER}"
    fi
fi

echo "  Copying toolchain's Shared Libraries to rootfs..."
for LIB in "${SHARED_LIBRARIES[@]}"; do
    SOURCE_LIB="${SYSROOT}/${LIB_DIR}/${LIB}"
    DEST_LIB="${OUTDIR}/rootfs/${LIB_DIR}"
    echo "    Copying shared library: $LIB"
    cp -a "$SOURCE_LIB" "${DEST_LIB}"

    # If the library is a symbolic link, copy its target
    LINK_TARGET=$(readlink -f "$SOURCE_LIB")
    if [ "$LINK_TARGET" != "$SOURCE_LIB" ]; then
        echo "    Copying symbolic link target for shared library: ${LINK_TARGET}"
        cp -a "$LINK_TARGET" "${DEST_LIB}"
    fi
done

if [ "$IS_64BIT" -eq 1 ]; then
    #*** WORKAROUND ****: The assignment ask us to place the libraries in /lib64, but the linux initialization only works when I place them on /lib
    echo "    Copying all dependencies in rootfs/lib64 to rootfs/lib"
    cp -a ${OUTDIR}/rootfs/lib64/* ${OUTDIR}/rootfs/lib/
fi

# TODO: Make device nodes
echo "Make device nodes"
cd "${OUTDIR}/rootfs"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
echo "Cleand and build the writer utility"
# Change back to the directory where this script is located (it is the same where the writer utility makefile is located)
cd "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory on the target rootfs
echo "Copying finder-app related files to the home directory"
ROOTFS_HOME_DIR="${OUTDIR}/rootfs/home"
cp writer "${ROOTFS_HOME_DIR}/"
cp finder.sh "${ROOTFS_HOME_DIR}/"
cp finder-test.sh "${ROOTFS_HOME_DIR}/"
cp autorun-qemu.sh "${ROOTFS_HOME_DIR}/"
mkdir -p "${ROOTFS_HOME_DIR}/conf"
cp conf/username.txt "${ROOTFS_HOME_DIR}/conf/"
cp conf/assignment.txt "${ROOTFS_HOME_DIR}/conf/"


# TODO: Chown the root directory
echo "Chown root directory to initramfs.cpio"
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio


# TODO: Create initramfs.cpio.gz
echo "Compressimg to initramfs.cpio.gz"
cd "${OUTDIR}"
gzip -f initramfs.cpio