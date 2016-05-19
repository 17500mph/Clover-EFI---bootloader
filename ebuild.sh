#!/bin/bash

#  ebuild.sh ->ebuild.sh  //renamed to be unique file begining from E
#  Script for building CloverEFI source under OS X or Linux
#  Supported chainloads(compilers) are XCODE32, UNIXGCC and CLANG
#
#
#  Created by Jadran Puharic on 1/6/12.
#  Modified by JrCs on 3/9/13.
#  Zenith432, STLVNUB, cecekpawon 2016

# Go to the Clover root directory
cd "$(dirname $0)"

# Global variables
declare -r SELF="${0##*/}"
declare -r CLOVERROOT="$PWD"
declare -r SYSNAME="$(uname)"
if [[ "$SYSNAME" == Linux ]]; then
  declare -r NUMBER_OF_CPUS=$(nproc)
else
  declare -r NUMBER_OF_CPUS=$(sysctl -n hw.ncpu)
fi
declare -a EDK2_BUILD_OPTIONS=
print_option_help_wc=
have_fmt=
PLATFORMFILE=
MODULEFILE=
TARGETRULE=


# Default values
export TOOLCHAIN=XCODE5
export TARGETARCH=X64
export BUILDTARGET=RELEASE
export BUILDTHREADS=$(( NUMBER_OF_CPUS + 1 ))
export WORKSPACE=${WORKSPACE:-}
export CONF_PATH=${CONF_PATH:-}

# if building through Xcode, then TOOLCHAIN_DIR is not defined
# checking if it is where CloverGrowerPro put it
if [[ "$SYSNAME" == Linux ]]; then
  export TOOLCHAIN=GCC53
  TOOLCHAIN_DIR=${TOOLCHAIN_DIR:-/usr}
else
  TOOLCHAIN_DIR=${TOOLCHAIN_DIR:-"$CLOVERROOT"/../../toolchain}
fi
if [[ ! -d $TOOLCHAIN_DIR ]]; then
  TOOLCHAIN_DIR="${PWD}"/../../opt/local
fi
export TOOLCHAIN_DIR
echo "TOOLCHAIN_DIR: $TOOLCHAIN_DIR"

VBIOSPATCHCLOVEREFI=0
ONLYSATA0PATCH=0
USE_BIOS_BLOCKIO=0
USE_LOW_EBDA=1
CLANG=0
GENPAGE=0

declare -r GIT=`which git`
#declare -r GITDIR=`git status 2> /dev/null`        # unsafe as git repository may exist in parent directory
declare -r VERSTXT="vers.txt"
if [[ -x "/usr/bin/sw_vers" ]]; then
  declare -r OSVER="$(sw_vers -productVersion | sed -e 's/\.0$//g')"
elif [[ -x "/usr/bin/lsb_release" ]]; then
  declare -r OSVER="$(lsb_release -sir)"
fi
PATCH_FILE=

# Bash options
set -e # errexit
set -u # Blow on unbound variable

## FUNCTIONS ##

function exitTrap() {
    if [[ -n "$PATCH_FILE" && -n "$WORKSPACE" ]]; then
        echo -n "Unpatching edk2..."
        ( cd "$WORKSPACE" && cat "$CLOVERROOT"/Patches_for_EDK2/$PATCH_FILE | eval "$PATCH_CMD -p0 -R" &>/dev/null )
        if [[ $? -eq 0 ]]; then
            echo " done"
        else
            echo " failed"
        fi
    fi
}

# Check if we need to patch the sources
checkPatch() {
  #if [[ -x /usr/bin/git ]]; then
  #    PATCH_CMD="/usr/bin/git apply --whitespace=nowarn"
  if [[ -n "${GIT}" ]]; then
      PATCH_CMD="${GIT} apply --whitespace=nowarn"
  else
      PATCH_CMD="/usr/bin/patch"
  fi

  checkToolchain

  if [[ "$SYSNAME" == Linux ]]; then
    if [[ ! -x "$TOOLCHAIN_DIR"/bin/gcc ]]; then
        echo "No clover toolchain found !" >&2
        echo "Install on your system or define the TOOLCHAIN_DIR variable." >&2
        exit 1
    fi
  else
    if [[ -n "${XCODE_BUILD:-}" ]]; then
      #declare -r XCODE_MAJOR_VERSION="$(xcodebuild -version | sed -nE 's/^Xcode ([0-9]).*/\1/p')"
      XCODE_VERSION="$(echo `$XCODE_BUILD -version` | sed -nE 's/^Xcode ([0-9.]+).*/\1/p')"
      declare -r XCODE_MAJOR_VERSION="$(echo $XCODE_VERSION | cut -d. -f1)"

      case "$XCODE_MAJOR_VERSION" in
          5) PATCH_FILE=;;
      esac
    fi

    if [[ ! -x "$TOOLCHAIN_DIR"/cross/bin/x86_64-clover-linux-gnu-gcc && \
          ! -x "$TOOLCHAIN_DIR"/cross/bin/i686-clover-linux-gnu-gcc ]] && [[ $TOOLCHAIN == GCC* ]] ; then
        echo "No clover toolchain found !" >&2
        echo "Build it with the buidgcc.sh script or defined the TOOLCHAIN_DIR variable." >&2
        exit 1
    fi
  fi

  if [[ ! -x "$TOOLCHAIN_DIR"/bin/nasm ]]; then
      echo "No nasm binary found in toolchain directory !" >&2
      if [[ "$SYSNAME" != Linux ]]; then
        echo "Build it with the buidnasm.sh script." >&2
      fi
      exit 1
  fi
}

print_option_help () {
  if [[ x$print_option_help_wc = x ]]; then
      if wc -L  </dev/null > /dev/null 2>&1; then
          print_option_help_wc=-L
      elif wc -m  </dev/null > /dev/null 2>&1; then
          print_option_help_wc=-m
      else
          print_option_help_wc=-b
      fi
  fi
  if [[ x$have_fmt = x ]]; then
      if fmt -w 40  </dev/null > /dev/null 2>&1; then
          have_fmt=y;
      else
          have_fmt=n;
      fi
  fi
  local print_option_help_lead="  $1"
  local print_option_help_lspace="$(echo "$print_option_help_lead" | wc $print_option_help_wc)"
  local print_option_help_fill="$((26 - print_option_help_lspace))"
  printf "%s" "$print_option_help_lead"
  local print_option_help_nl=
  if [[ $print_option_help_fill -le 0 ]]; then
      print_option_help_nl=y
      echo
  else
      print_option_help_i=0;
      while [[ $print_option_help_i -lt $print_option_help_fill ]]; do
          printf " "
          print_option_help_i=$((print_option_help_i+1))
      done
      print_option_help_nl=n
  fi
  local print_option_help_split=
  if [[ x$have_fmt = xy ]]; then
      print_option_help_split="$(echo "$2" | fmt -w 50)"
  else
      print_option_help_split="$2"
  fi
  if [[ x$print_option_help_nl = xy ]]; then
      echo "$print_option_help_split" | awk '{ print "                          " $0; }'
  else
      echo "$print_option_help_split" | awk 'BEGIN   { n = 0 }
          { if (n == 1) print "                          " $0; else print $0; n = 1 ; }'
  fi
}

# Function to manage PATH
pathmunge () {
    if [[ ! $PATH =~ (^|:)$1(:|$) ]]; then
        if [[ "${2:-}" = "after" ]]; then
            export PATH=$PATH:$1
        else
            export PATH=$1:$PATH
        fi
    fi
}

# Add edk2 build option
addEdk2BuildOption() {
    EDK2_BUILD_OPTIONS=("${EDK2_BUILD_OPTIONS[@]}" $@)
}

# Add edk2 build macro
addEdk2BuildMacro() {
    local macro="$1"
    addEdk2BuildOption "-D" "$macro"
}

# Check Xcode toolchain
checkXcode () {
    XCODE_BUILD="/usr/bin/xcodebuild"
    if [[ ! -x "${XCODE_BUILD}" ]]; then
        echo "ERROR: Install Xcode Tools from Apple before using this script." >&2
        exit 1
    fi
}

# Print the usage.
usage() {
    echo "Script for building CloverEFI sources on Darwin OS X"
    echo
    printf "Usage: %s [OPTIONS] [all|fds|genc|genmake|clean|cleanpkg|cleanall|cleanlib|modules|libraries]\n" "$SELF"
    echo
    echo "Configuration:"
    print_option_help "-n THREADNUMBER" "Build the platform using multi-threaded compiler [default is number of CPUs + 1]"
    print_option_help "-h, --help"    "print this message and exit"
    print_option_help "-v, --version" "print the version information and exit"
    echo
    echo "Toolchain:"
    print_option_help "-clang"     "use XCode Clang toolchain"
    print_option_help "-gcc47"     "use GCC 4.7 toolchain"
    print_option_help "-gcc49"     "use GCC 4.9 toolchain [Default]"
    print_option_help "-gcc53"     "use GCC 5.3 toolchain"
    print_option_help "-xcode"     "use XCode 3.2 toolchain"
    print_option_help "-xcode5"     "use XCode 5+ toolchain"
    print_option_help "-t TOOLCHAIN, --tagname=TOOLCHAIN" "force to use a specific toolchain"
    echo
    echo "Target:"
    print_option_help "-ia32"      "build Clover in 32-bit [boot3]"
    print_option_help "-x64"       "build Clover in 64-bit [boot6] [Default]"
    print_option_help "-mc, --x64-mcp"   "build Clover in 64-bit [boot7] using BiosBlockIO (compatible with MCP chipset)"
    print_option_help "-a TARGETARCH, --arch=TARGETARCH" "overrides target.txt's TARGET_ARCH definition"
    print_option_help "-p PLATFORMFILE, --platform=PLATFORMFILE" "Build the platform specified by the DSC filename argument"
    print_option_help "-m MODULEFILE, --module=MODULEFILE" "Build only the module specified by the INF filename argument"
    print_option_help "-b BUILDTARGET, --buildtarget=BUILDTARGET" "using the BUILDTARGET to build the platform"
    echo
    echo "Options:"
    print_option_help "-D MACRO, --define=MACRO" "Macro: \"Name[=Value]\"."
    print_option_help "--vbios-patch-cloverefi" "activate vbios patch in CloverEFI"
    print_option_help "--only-sata0" "activate only SATA0 patch"
    print_option_help "--std-ebda" "ebda offset dont shift to 0x88000"
    print_option_help "--genpage" "dynamically generate page table under ebda"
    print_option_help "--no-usb" "disable USB support"
    print_option_help "--no-lto" "disable Link Time Optimisation"
    print_option_help "--edk2shell <MinimumShell|FullShell>" "copy edk2 Shell to EFI tools dir"
    echo
    echo "Report bugs to https://sourceforge.net/p/cloverefiboot/discussion/1726372/"
}

# Manage option argument
argument () {
  local opt=$1
  shift

  if [[ $# -eq 0 ]]; then
      printf "%s: option \`%s' requires an argument\n" "$0" "$opt"
      exit 1
  fi

  echo $1
}

# Check the command line arguments
checkCmdlineArguments() {
    while [[ $# -gt 0 ]]; do
        local option=$1
        shift
        case "$option" in
            -clang  | --clang)   TOOLCHAIN=XCLANG ; CLANG=1 ;;
            -llvm   | --llvm)    TOOLCHAIN=LLVM  ; CLANG=1 ;;
            -xcode5  | --xcode5 )  TOOLCHAIN=XCODE5 ; CLANG=1 ;;
            -GCC47  | --GCC47)   TOOLCHAIN=GCC47   ;;
            -gcc47  | --gcc47)   TOOLCHAIN=GCC47   ;;
            -GCC48  | --GCC48)   TOOLCHAIN=GCC48   ;;
            -gcc48  | --gcc48)   TOOLCHAIN=GCC48   ;;
            -GCC49  | --GCC49)   TOOLCHAIN=GCC49   ;;
            -gcc49  | --gcc49)   TOOLCHAIN=GCC49   ;;
            -GCC53  | --GCC53)   TOOLCHAIN=GCC53   ;;
            -gcc53  | --gcc53)   TOOLCHAIN=GCC53   ;;
            -unixgcc | --gcc)    TOOLCHAIN=UNIXGCC ;;
            -xcode  | --xcode )  TOOLCHAIN=XCODE32 ;;
            -ia32 | --ia32)      TARGETARCH=IA32   ;;
            -x64 | --x64)        TARGETARCH=X64    ;;
            -mc | --x64-mcp)     TARGETARCH=X64 ; USE_BIOS_BLOCKIO=1 ;;
            -clean)    TARGETRULE=clean ;;
            -cleanall) TARGETRULE=cleanall ;;
            -d | -debug | --debug)  BUILDTARGET=DEBUG ;;
            -r | -release | --release) BUILDTARGET=RELEASE ;;
            -a) TARGETARCH=$(argument $option "$@"); shift
                ;;
            --arch=*)
                TARGETARCH=$(echo "$option" | sed 's/--arch=//')
                ;;
            -p) PLATFORMFILE=$(argument $option "$@"); shift
                ;;
            --platform=*)
                PLATFORMFILE=$(echo "$option" | sed 's/--platform=//')
                ;;
            -m) MODULEFILE=$(argument $option "$@"); shift
                ;;
            --module=*)
                MODULEFILE=$(echo "$option" | sed 's/--module=//')
                ;;
            -b) BUILDTARGET=$(argument $option "$@"); shift
                ;;
            --buildtarget=*)
                BUILDTARGET=$(echo "$option" | sed 's/--buildtarget=//')
                ;;
            -t) TOOLCHAIN=$(argument $option "$@"); shift
                ;;
            --tagname=*)
                TOOLCHAIN=$(echo "$option" | sed 's/--tagname=//')
                ;;
            -D)
                addEdk2BuildMacro $(argument $option "$@"); shift
                ;;
            --define=*)
                addEdk2BuildMacro $(echo "$option" | sed 's/--define=//')
                ;;
            -n)
                BUILDTHREADS=$(argument $option "$@"); shift
                ;;
            --vbios-patch-cloverefi)
                VBIOSPATCHCLOVEREFI=1
                ;;
            --only-sata0)
                ONLYSATA0PATCH=1
                ;;
            --std-ebda)
                USE_LOW_EBDA=0
                ;;
            --genpage)
                GENPAGE=1
                ;;
            --no-usb)
                addEdk2BuildMacro DISABLE_USB_SUPPORT
                ;;
            --no-lto)
                addEdk2BuildMacro DISABLE_LTO
                ;;
            --edk2shell) EDK2SHELL=$(argument $option "$@"); shift
                ;;
            -h | -\? | -help | --help)
                usage && exit 0
                ;;
            -v | --version)
                echo "$SELF v1.0" && exit 0
                ;;
            -*)
                printf "Unrecognized option \`%s'\n" "$option" 1>&2
                exit 1
                ;;
            *)
               TARGETRULE="$option"
               ;;
        esac
    done

    # Update variables
    PLATFORMFILE="${PLATFORMFILE:-Clover/Clover.dsc}"
    if [ ! -z "${MODULEFILE}" ]; then
        MODULEFILE=" -m Clover/$MODULEFILE"
    fi
}

## Check tools for the toolchain
checkToolchain() {
    case "$TOOLCHAIN" in
        XCLANG|XCODE32|XCODE5) checkXcode ;;
    esac
}

# Main build script
MainBuildScript() {
    checkCmdlineArguments $@
    #checkToolchain
    checkPatch

    if [[ -d .svn ]]; then
        svnversion -n | tr -d [:alpha:] > "${VERSTXT}"
    elif [[ -d .git ]]; then
        git svn find-rev git-svn | tr -cd [:digit:] > "${VERSTXT}"
    else
        echo -n "0000" > "${VERSTXT}"
    fi

    #
    # Setup workspace if it is not set
    #
    if [[ -z "$WORKSPACE" ]]; then
        echo "Initializing workspace"
        local EDK2DIR=$(cd "$CLOVERROOT"/.. && echo "$PWD")
        if [[ ! -x "${EDK2DIR}"/edksetup.sh ]]; then
            echo "Error: Can't find edksetup.sh script !" >&2
            exit 1
        fi

        # This version is for the tools in the BaseTools project.
        # this assumes svn pulls have the same root dir
        #  export EDK_TOOLS_PATH=`pwd`/../BaseTools
        # This version is for the tools source in edk2
        cd "$EDK2DIR"
        export EDK_TOOLS_PATH="${PWD}"/BaseTools
        source ./edksetup.sh BaseTools
        cd "$CLOVERROOT"
    else
        echo "Building from: $WORKSPACE"
    fi

    # Trying to patch edk2
    if [[ -n "$PATCH_FILE" ]]; then
        echo -n "Patching edk2..."
        ( cd "$WORKSPACE" && cat "$CLOVERROOT"/Patches_for_EDK2/$PATCH_FILE | eval "$PATCH_CMD -p0" &>/dev/null )
        if [[ $? -eq 0 ]]; then
            echo " done"
        else
            echo " failed"
        fi
    fi

    export CLOVER_PKG_DIR="$CLOVERROOT"/CloverPackage/CloverV2

    # Cleaning part of the script if we have told to do it
    if [[ "$TARGETRULE" == cleanpkg ]]; then
        if [[ "$SYSNAME" != Linux ]]; then
            # Make some house cleaning
            echo "Cleaning CloverUpdater files..."
            make -C "$CLOVERROOT"/CloverPackage/CloverUpdater clean

            echo "Cleaning CloverPrefpane files..."
            make -C "$CLOVERROOT"/CloverPackage/CloverPrefpane clean
        fi

        echo "Cleaning bootsector files..."
        local BOOTHFS="$CLOVERROOT"/BootHFS
        DESTDIR="$CLOVER_PKG_DIR"/BootSectors make -C $BOOTHFS clean

        echo
        # Use subshell to use shopt
        (
            echo "Cleaning packaging files..."
            shopt -s nullglob
            find  "$CLOVER_PKG_DIR"/Bootloaders/{ia32,x64}/ -mindepth 1 -not -path "**/.svn*" -delete
            if [[ -d "$CLOVER_PKG_DIR"/EFI/BOOT ]]; then
                find  "$CLOVER_PKG_DIR"/EFI/BOOT/ -name '*.efi' -mindepth 1 -not -path "**/.svn*" -delete
                rmdir "$CLOVER_PKG_DIR"/EFI/BOOT &>/dev/null
            fi
            local dir
            for dir in "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers*; do
                find  "$dir" -mindepth 1 -not -path "**/.svn*" -delete
                rmdir "$dir" &>/dev/null
            done
            find  "$CLOVER_PKG_DIR"/EFI/CLOVER/ -name '*.efi' -maxdepth 1 -not -path "**/.svn*" -delete
            for dir in "$CLOVER_PKG_DIR"/drivers-Off/drivers*; do
                find  "$dir" -mindepth 1 -not -path "**/.svn*" -delete
            done
        )
        echo  "Done!"
        exit $?

    elif [[ "$TARGETRULE" == clean || "$TARGETRULE" == cleanall ]]; then
        build -p $PLATFORMFILE -a $TARGETARCH -b $BUILDTARGET \
         -t $TOOLCHAIN -n $BUILDTHREADS $TARGETRULE
        [[ "$TARGETRULE" == cleanall ]] && make -C $WORKSPACE/BaseTools clean
        exit $?
    fi

    # Create edk tools if necessary
    if  [[ ! -x "$EDK_TOOLS_PATH/Source/C/bin/GenFv" ]]; then
        echo "Building tools as they are not found"
        make -C "$WORKSPACE"/BaseTools CC="gcc -Wno-deprecated-declarations"
    fi

    # Apply options
    [[ "$USE_BIOS_BLOCKIO" -ne 0 ]]    && addEdk2BuildMacro 'USE_BIOS_BLOCKIO'
    [[ "$VBIOSPATCHCLOVEREFI" -ne 0 ]] && addEdk2BuildMacro 'ENABLE_VBIOS_PATCH_CLOVEREFI'
    [[ "$ONLYSATA0PATCH" -ne 0 ]] && addEdk2BuildMacro 'ONLY_SATA_0'
    [[ "$USE_LOW_EBDA" -ne 0 ]] && addEdk2BuildMacro 'USE_LOW_EBDA'
    [[ "$CLANG" -ne 0 ]] && addEdk2BuildMacro 'CLANG'

    local cmd="build ${EDK2_BUILD_OPTIONS[@]}"
    cmd="$cmd -p $PLATFORMFILE $MODULEFILE -a $TARGETARCH -b $BUILDTARGET"
    cmd="$cmd -t $TOOLCHAIN -n $BUILDTHREADS $TARGETRULE"

    echo
    echo "Running edk2 build for Clover$TARGETARCH using the command:"
    echo "$cmd"
    echo

    # Build Clover
    local clover_revision=$(cat "${CLOVERROOT}/${VERSTXT}")
    local clover_build_date=$(date '+%Y-%m-%d %H:%M:%S')
    echo "#define FIRMWARE_VERSION \"2.31\"" > "$CLOVERROOT"/Version.h
    echo "#define FIRMWARE_BUILDDATE \"${clover_build_date}\"" >> "$CLOVERROOT"/Version.h
    echo "#define FIRMWARE_REVISION L\"${clover_revision}\""   >> "$CLOVERROOT"/Version.h
    echo "#define REVISION_STR \"Clover revision: ${clover_revision}\"" >> "$CLOVERROOT"/Version.h

    local clover_build_info="Args: ./${SELF}"
    if [[ -n "$@" ]]; then
      clover_build_info="${clover_build_info} $@"
    fi
    clover_build_info="${clover_build_info} | Command: $(echo $cmd | xargs)"

    if [[ -n "${OSVER:-}" ]]; then
      clover_build_info="${clover_build_info} | OS: ${OSVER}"
    fi
    if [[ -n "${XCODE_VERSION:-}" ]]; then
      clover_build_info="${clover_build_info} | XCODE: ${XCODE_VERSION}"
    fi

    echo "#define BUILDINFOS_STR \"${clover_build_info}\"" >> "$CLOVERROOT"/Version.h

    cp "$CLOVERROOT"/Version.h "$CLOVERROOT"/rEFIt_UEFI/

    eval "$cmd"
}

# Deploy Clover files for packaging
MainPostBuildScript() {
    if [[ -z "$EDK_TOOLS_PATH" ]]; then
        export BASETOOLS_DIR="$WORKSPACE"/BaseTools/Source/C/bin
    else
        export BASETOOLS_DIR="$EDK_TOOLS_PATH"/Source/C/bin
    fi
    export BOOTSECTOR_BIN_DIR="$CLOVERROOT"/BootSector/bin
    export BUILD_DIR="${WORKSPACE}/Build/Clover/${BUILDTARGET}_${TOOLCHAIN}"
    export BUILD_DIR_ARCH="${BUILD_DIR}/$TARGETARCH"

    echo Compressing DUETEFIMainFv.FV ...
    "$BASETOOLS_DIR"/LzmaCompress -e -o "${BUILD_DIR}/FV/DUETEFIMAINFV${TARGETARCH}.z" "${BUILD_DIR}/FV/DUETEFIMAINFV${TARGETARCH}.Fv"

    echo Compressing DxeCore.efi ...
    "$BASETOOLS_DIR"/LzmaCompress -e -o "${BUILD_DIR}/FV/DxeMain${TARGETARCH}.z" "$BUILD_DIR_ARCH/DxeCore.efi"

    echo Compressing DxeIpl.efi ...
    "$BASETOOLS_DIR"/LzmaCompress -e -o "${BUILD_DIR}/FV/DxeIpl${TARGETARCH}.z" "$BUILD_DIR_ARCH/DxeIpl.efi"

    echo "Generate Loader Image ..."

    if [[ "${TARGETARCH}" = IA32 ]]; then
        cloverEFIFile=boot3
        "$BASETOOLS_DIR"/GenFw --rebase 0x10000 -o "$BUILD_DIR_ARCH/EfiLoader.efi" "$BUILD_DIR_ARCH/EfiLoader.efi"
        "$BASETOOLS_DIR"/EfiLdrImage -o "${BUILD_DIR}"/FV/Efildr32 \
         "${BUILD_DIR}"/${TARGETARCH}/EfiLoader.efi                \
         "${BUILD_DIR}"/FV/DxeIpl${TARGETARCH}.z                   \
         "${BUILD_DIR}"/FV/DxeMain${TARGETARCH}.z                  \
         "${BUILD_DIR}"/FV/DUETEFIMAINFV${TARGETARCH}.z

        cat $BOOTSECTOR_BIN_DIR/start32H.com2 $BOOTSECTOR_BIN_DIR/efi32.com3 \
         "${BUILD_DIR}"/FV/Efildr32 > "${BUILD_DIR}"/FV/boot

        mkdir -p "$CLOVER_PKG_DIR"/Bootloaders/ia32
        mkdir -p "$CLOVER_PKG_DIR"/EFI/BOOT
        mkdir -p "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers32
        mkdir -p "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers32UEFI
        mkdir -p "$CLOVER_PKG_DIR"/drivers-Off/drivers32
        mkdir -p "$CLOVER_PKG_DIR"/drivers-Off/drivers32UEFI
        # CloverEFI
        cp -v "${BUILD_DIR}"/FV/boot "$CLOVER_PKG_DIR"/Bootloaders/ia32/$cloverEFIFile

        # Mandatory drivers
        cp -v "$BUILD_DIR_ARCH"/FSInject.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers32/FSInject-32.efi
        cp -v "$BUILD_DIR_ARCH"/FSInject.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers32UEFI/FSInject-32.efi
        cp -v "$BUILD_DIR_ARCH"/OsxFatBinaryDrv.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers32UEFI/OsxFatBinaryDrv-32.efi
        cp -v "$BUILD_DIR_ARCH"/VBoxHfs.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers32UEFI/VBoxHfs-32.efi

        # Optional drivers
        #cp -v "${BUILD_DIR}"/${TARGETARCH}/VBoxIso9600.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/VBoxIso9600-32.efi
        cp -v "$BUILD_DIR_ARCH"/VBoxExt2.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/VBoxExt2-32.efi
        cp -v "$BUILD_DIR_ARCH"/VBoxExt4.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/VBoxExt4-32.efi
        cp -v "$BUILD_DIR_ARCH"/GrubEXFAT.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/GrubEXFAT-32.efi
        cp -v "$BUILD_DIR_ARCH"/GrubISO9660.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/GrubISO9660-32.efi
        cp -v "$BUILD_DIR_ARCH"/GrubNTFS.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/GrubNTFS-32.efi
        cp -v "$BUILD_DIR_ARCH"/GrubUDF.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/GrubUDF-32.efi
        cp -v "$BUILD_DIR_ARCH"/Ps2KeyboardDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/Ps2KeyboardDxe-32.efi
        cp -v "$BUILD_DIR_ARCH"/Ps2MouseAbsolutePointerDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/Ps2MouseAbsolutePointerDxe-32.efi
        cp -v "$BUILD_DIR_ARCH"/Ps2MouseDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/Ps2MouseDxe-32.efi
        cp -v "$BUILD_DIR_ARCH"/UsbMouseDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/UsbMouseDxe-32.efi
        cp -v "$BUILD_DIR_ARCH"/XhciDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers32/XhciDxe-32.efi

        #applications
        cp -v "$BUILD_DIR_ARCH"/bdmesg.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/tools/bdmesg-32.efi
        cp -v "$BUILD_DIR_ARCH"/CLOVER${TARGETARCH}.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/
        cp -v "$BUILD_DIR_ARCH"/CLOVER${TARGETARCH}.efi "$CLOVER_PKG_DIR"/EFI/BOOT/BOOTIA32.efi

        if [[ "${EDK2SHELL:-}" == "MinimumShell" ]]; then
            cp -v "${WORKSPACE}"/ShellBinPkg/MinUefiShell/Ia32/Shell.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/tools/Shell32.efi
        elif [[ "${EDK2SHELL:-}" == "FullShell" ]]; then
            cp -v "${WORKSPACE}"/ShellBinPkg/UefiShell/Ia32/Shell.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/tools/Shell32.efi
        fi
    fi

    if [[ "$TARGETARCH" = X64 ]]; then
        cloverEFIFile=boot$((6 + USE_BIOS_BLOCKIO))

        "$BASETOOLS_DIR"/GenFw --rebase 0x10000 -o "$BUILD_DIR_ARCH/EfiLoader.efi" "$BUILD_DIR_ARCH/EfiLoader.efi"
        "$BASETOOLS_DIR"/EfiLdrImage -o "${BUILD_DIR}"/FV/Efildr64 \
        "$BUILD_DIR_ARCH"/EfiLoader.efi                \
        "${BUILD_DIR}"/FV/DxeIpl${TARGETARCH}.z        \
        "${BUILD_DIR}"/FV/DxeMain${TARGETARCH}.z       \
        "${BUILD_DIR}"/FV/DUETEFIMAINFV${TARGETARCH}.z
        if [[ "$GENPAGE" -eq 0 && "$USE_LOW_EBDA" -ne 0 ]]; then
            if [[ "$SYSNAME" == Linux ]]; then
                local -r EL_SIZE=$(stat -c "%s" "${BUILD_DIR}"/FV/Efildr64)
            else
                local -r EL_SIZE=$(stat -f "%z" "${BUILD_DIR}"/FV/Efildr64)
            fi
            if (( $((EL_SIZE)) > 417792 )); then
                echo 'warning: boot file bigger than low-ebda permits, switching to --std-ebda'
                USE_LOW_EBDA=0
            fi
        fi
        local -ar COM_NAMES=(H H2 H3 H4 H5 H6 H5 H6)           # Note: (H{,2,3,4,5,6,5,6}) works in Linux bash, but not Darwin bash
        startBlock=Start64${COM_NAMES[$((GENPAGE << 2 | USE_LOW_EBDA << 1 | USE_BIOS_BLOCKIO))]}.com
        if [[ "$GENPAGE" -ne 0 ]]; then
        cat $BOOTSECTOR_BIN_DIR/$startBlock $BOOTSECTOR_BIN_DIR/efi64.com3 "${BUILD_DIR}"/FV/Efildr64 > "${BUILD_DIR}"/FV/boot
        else
        cat $BOOTSECTOR_BIN_DIR/$startBlock $BOOTSECTOR_BIN_DIR/efi64.com3 "${BUILD_DIR}"/FV/Efildr64 > "${BUILD_DIR}"/FV/Efildr20Pure

        if [[ "$USE_LOW_EBDA" -ne 0 ]]; then
           "$BASETOOLS_DIR"/GenPage "${BUILD_DIR}"/FV/Efildr20Pure -b 0x88000 -f 0x68000 -o "${BUILD_DIR}"/FV/Efildr20
        else
          "$BASETOOLS_DIR"/GenPage "${BUILD_DIR}"/FV/Efildr20Pure -o "${BUILD_DIR}"/FV/Efildr20
        fi
        # Create CloverEFI file
        dd if="${BUILD_DIR}"/FV/Efildr20 of="${BUILD_DIR}"/FV/boot bs=512 skip=1
        fi

        # Be sure that all needed directories exists
        mkdir -p "$CLOVER_PKG_DIR"/Bootloaders/x64
        mkdir -p "$CLOVER_PKG_DIR"/EFI/BOOT
        mkdir -p "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers64
        mkdir -p "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers64UEFI
        mkdir -p "$CLOVER_PKG_DIR"/drivers-Off/drivers64
        mkdir -p "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI

        # Install CloverEFI file
        cp -v "${BUILD_DIR}"/FV/boot "$CLOVER_PKG_DIR"/Bootloaders/x64/$cloverEFIFile

        # Mandatory drivers
        cp -v "$BUILD_DIR_ARCH"/FSInject.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers64/FSInject-64.efi

        cp -v "$BUILD_DIR_ARCH"/FSInject.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers64UEFI/FSInject-64.efi
        cp -v "$BUILD_DIR_ARCH"/OsxFatBinaryDrv.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers64UEFI/OsxFatBinaryDrv-64.efi
        cp -v "$BUILD_DIR_ARCH"/VBoxHfs.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/drivers64UEFI/VBoxHfs-64.efi

        # Optional drivers
        cp -v "$BUILD_DIR_ARCH"/VBoxIso9600.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/VBoxIso9600-64.efi
        cp -v "$BUILD_DIR_ARCH"/VBoxExt2.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/VBoxExt2-64.efi
        cp -v "$BUILD_DIR_ARCH"/VBoxExt4.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/VBoxExt4-64.efi
        cp -v "$BUILD_DIR_ARCH"/PartitionDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/PartitionDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/DataHubDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/DataHubDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/GrubEXFAT.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/GrubEXFAT-64.efi
        cp -v "$BUILD_DIR_ARCH"/GrubISO9660.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/GrubISO9660-64.efi
        cp -v "$BUILD_DIR_ARCH"/GrubNTFS.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/GrubNTFS-64.efi
        cp -v "$BUILD_DIR_ARCH"/GrubUDF.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/GrubUDF-64.efi

        #cp -v "$BUILD_DIR_ARCH"/Ps2KeyboardDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/Ps2KeyboardDxe-64.efi
        #cp -v "$BUILD_DIR_ARCH"/Ps2MouseAbsolutePointerDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/Ps2MouseAbsolutePointerDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/Ps2MouseDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/Ps2MouseDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/UsbMouseDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/UsbMouseDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/XhciDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/XhciDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/NvmExpressDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64/NvmExpressDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/OsxAptioFixDrv.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/OsxAptioFixDrv-64.efi
        cp -v "$BUILD_DIR_ARCH"/OsxAptioFix2Drv.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/OsxAptioFix2Drv-64.efi
        cp -v "$BUILD_DIR_ARCH"/OsxLowMemFixDrv.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/OsxLowMemFixDrv-64.efi
        cp -v "$BUILD_DIR_ARCH"/CsmVideoDxe.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/CsmVideoDxe-64.efi
        cp -v "$BUILD_DIR_ARCH"/EmuVariableUefi.efi "$CLOVER_PKG_DIR"/drivers-Off/drivers64UEFI/EmuVariableUefi-64.efi

        #applications
        cp -v "$BUILD_DIR_ARCH"/bdmesg.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/tools/
        cp -v "$BUILD_DIR_ARCH"/CLOVER${TARGETARCH}.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/
        cp -v "$BUILD_DIR_ARCH"/CLOVER${TARGETARCH}.efi "$CLOVER_PKG_DIR"/EFI/BOOT/BOOTX64.efi

        if [[ "${EDK2SHELL:-}" == "MinimumShell" ]]; then
            cp -v "${WORKSPACE}"/ShellBinPkg/MinUefiShell/X64/Shell.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/tools/Shell64U.efi
        elif [[ "${EDK2SHELL:-}" == "FullShell" ]]; then
            cp -v "${WORKSPACE}"/ShellBinPkg/UefiShell/X64/Shell.efi "$CLOVER_PKG_DIR"/EFI/CLOVER/tools/Shell64U.efi
        fi
    fi

    echo "Done!"

    # Build and install Bootsectors
    echo
    echo "Generating BootSectors"
    local BOOTHFS="$CLOVERROOT"/BootHFS
    DESTDIR="$CLOVER_PKG_DIR"/BootSectors make -C $BOOTHFS
    echo "Done!"
}

# BUILD START #
trap 'exitTrap' EXIT

# Default locale
export LC_ALL=POSIX


# Add toolchain bin directory to the PATH
if [[ "$SYSNAME" != Linux ]]; then
  pathmunge "$TOOLCHAIN_DIR/bin"
fi

MainBuildScript $@
if [[ -z $MODULEFILE  ]]; then
    MainPostBuildScript
fi

# Local Variables:      #
# mode: ksh             #
# tab-width: 4          #
# indent-tabs-mode: nil #
# End:                  #
#
# vi: set expandtab ts=4 sw=4 sts=4: #
