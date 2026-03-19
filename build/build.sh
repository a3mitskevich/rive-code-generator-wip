#!/bin/bash

# build.sh: build the rive_code_generator project.
#
# Usage:
#
#   cd build
#   ./build.sh                # debug build
#   ./build.sh release        # release build
#   ./build.sh release clean  # clean, followed by a release build
#   ./build.sh compdb         # generate compile_commands.json for IDE integration
#   ./build.sh ninja          # use ninja for a debug build
#   ./build.sh ninja release  # use ninja for a release build
#   ./build.sh run            # build and run --help
#   ./build.sh dev            # build and run with dev sample args
#   ./build.sh rebuild out/debug  # relaunch build with previously configured args
#
# Specify build targets after "--":
#
#   ./build.sh -- rive_code_generator
#   ./build.sh ninja release -- rive_code_generator

set -e
set -o pipefail

# Resolve the directory where this script lives.
# https://stackoverflow.com/questions/59895/how-do-i-get-the-directory-where-a-bash-script-is-located-from-within-the-script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
SCRIPT_DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

RIVE_RUNTIME_BUILD_DIR="$SCRIPT_DIR/../rive-runtime/build"

# Detect host machine and number of CPU cores.
case "$(uname -s)" in
    Darwin*)
        if [[ $(arch) = "arm64" ]]; then
            HOST_MACHINE="mac_arm64"
        else
            HOST_MACHINE="mac_x64"
        fi
        NUM_CORES=$(($(sysctl -n hw.physicalcpu) + 1))
        ;;
    MINGW*|MSYS*)
        HOST_MACHINE="windows"
        NUM_CORES=$NUMBER_OF_PROCESSORS
        ;;
    Linux*)
        HOST_MACHINE="linux"
        NUM_CORES=$(grep -c processor /proc/cpuinfo)
        ;;
esac

# Windows fallback: delegate to PowerShell if msbuild is not available.
if [[ "$HOST_MACHINE" = "windows" ]]; then
    if ! command -v msbuild.exe &>/dev/null; then
        powershell "./build.ps1" "$@"
        exit $?
    fi
fi

RIVE_NO_BUILD=false
if [[ "${1:-}" = "nobuild" ]]; then
    RIVE_NO_BUILD=true
    shift
fi

RUN=false
DEV=false

if [[ "${1:-}" = "rebuild" ]]; then
    # Load args from an existing build.
    RIVE_OUT=$2
    shift
    shift

    if [ ! -d "$RIVE_OUT" ]; then
        echo "OUT directory '$RIVE_OUT' not found."
        exit 1
    fi

    ARGS_FILE=$RIVE_OUT/.rive_premake_args
    if [ ! -f "$ARGS_FILE" ]; then
        echo "Premake args file '$ARGS_FILE' not found."
        exit 1
    fi

    RIVE_PREMAKE_ARGS="$(< "$ARGS_FILE")"
    RIVE_BUILD_SYSTEM="$(awk '{print $1}' "$ARGS_FILE")"
else
    # New build. Parse arguments into premake options.
    RIVE_PREMAKE_ARGS="${RIVE_PREMAKE_ARGS:-}"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            debug) RIVE_CONFIG="${RIVE_CONFIG:-debug}" ;;
            release) RIVE_CONFIG="${RIVE_CONFIG:-release}" ;;
            clean) RIVE_CLEAN="${RIVE_CLEAN:-true}" ;;
            compdb)
                RIVE_BUILD_SYSTEM="${RIVE_BUILD_SYSTEM:-export-compile-commands}"
                RIVE_OUT="${RIVE_OUT:-out/compdb}"
                ;;
            ninja) RIVE_BUILD_SYSTEM="${RIVE_BUILD_SYSTEM:-ninja}" ;;
            xcode) RIVE_BUILD_SYSTEM="${RIVE_BUILD_SYSTEM:-xcode4}" ;;
            run) RUN=true ;;
            dev) DEV=true ;;
            --)
                shift
                break
                ;;
            *) RIVE_PREMAKE_ARGS="$RIVE_PREMAKE_ARGS $1" ;;
        esac
        shift
    done

    RIVE_CONFIG="${RIVE_CONFIG:-debug}"

    if [ -z "${RIVE_OUT:-}" ]; then
        RIVE_OUT="out/$RIVE_CONFIG"
    fi

    if [[ "$HOST_MACHINE" = "windows" ]]; then
        RIVE_BUILD_SYSTEM="${RIVE_BUILD_SYSTEM:-vs2022}"
    else
        RIVE_BUILD_SYSTEM="${RIVE_BUILD_SYSTEM:-gmake2}"
    fi

    RIVE_PREMAKE_ARGS="$RIVE_BUILD_SYSTEM --config=$RIVE_CONFIG --out=$RIVE_OUT --scripts=$RIVE_RUNTIME_BUILD_DIR --file=premake5_code_generator.lua $RIVE_PREMAKE_ARGS"

    if [[ "${RIVE_CLEAN:-}" = true ]]; then
        echo "Cleaning $RIVE_OUT..."
        rm -fr "./$RIVE_OUT"
    fi
fi

echo "Building rive_code_generator ($RIVE_CONFIG)"

# ---- Dependencies ----

mkdir -p "$SCRIPT_DIR/dependencies"
pushd "$SCRIPT_DIR/dependencies" > /dev/null

# Build premake5 from source, cached by tag.
RIVE_PREMAKE_TAG="${RIVE_PREMAKE_TAG:-v5.0.0-beta7}"
PREMAKE_INSTALL_DIR="$SCRIPT_DIR/dependencies/premake-core/bin/${RIVE_PREMAKE_TAG}_release"
if [ ! -f "$PREMAKE_INSTALL_DIR/premake5" ]; then
    echo "Building Premake ($RIVE_PREMAKE_TAG)..."
    rm -fr premake-core
    git clone --depth 1 --branch $RIVE_PREMAKE_TAG https://github.com/premake/premake-core.git
    pushd premake-core > /dev/null
    case "$HOST_MACHINE" in
        mac_arm64) make -f Bootstrap.mak osx PLATFORM=ARM ;;
        mac_x64) make -f Bootstrap.mak osx ;;
        windows) ./Bootstrap.bat ;;
        *) make -f Bootstrap.mak linux ;;
    esac
    cp -r bin/release "$PREMAKE_INSTALL_DIR"
    popd > /dev/null
fi
export PATH="$PREMAKE_INSTALL_DIR:$PATH"

# Add rive-runtime build scripts to the premake path.
export PREMAKE_PATH="$RIVE_RUNTIME_BUILD_DIR"

# Setup premake-ninja.
if [[ "$RIVE_BUILD_SYSTEM" = "ninja" ]]; then
    if [ ! -d premake-ninja ]; then
        git clone --branch rive_modifications https://github.com/rive-app/premake-ninja.git
    fi
    export PREMAKE_PATH="$SCRIPT_DIR/dependencies/premake-ninja:$PREMAKE_PATH"
fi

# Setup premake-export-compile-commands.
if [[ "$RIVE_BUILD_SYSTEM" = "export-compile-commands" ]]; then
    if [ ! -d premake-export-compile-commands ]; then
        git clone --branch more_cpp_support https://github.com/rive-app/premake-export-compile-commands.git
    fi
    export PREMAKE_PATH="$SCRIPT_DIR/dependencies/premake-export-compile-commands:$PREMAKE_PATH"
fi

popd > /dev/null # leave dependencies

# ---- Incremental build validation ----

if [[ -d "$RIVE_OUT" && "$RIVE_NO_BUILD" = false ]]; then
    if [ -f "$RIVE_OUT/.rive_premake_args" ]; then
        if [[ "$RIVE_PREMAKE_ARGS" != "$(< "$RIVE_OUT/.rive_premake_args")" ]]; then
            echo "error: premake5 arguments for current build do not match previous arguments"
            echo "  previous command: premake5 $(< "$RIVE_OUT/.rive_premake_args")"
            echo "   current command: premake5 $RIVE_PREMAKE_ARGS"
            echo "If you wish to overwrite the existing build, please use 'clean'"
            exit 1
        fi
    fi
else
    mkdir -p "$RIVE_OUT"
    echo "$RIVE_PREMAKE_ARGS" > "$RIVE_OUT/.rive_premake_args"
fi

# ---- Run premake5 ----

echo "premake5 $RIVE_PREMAKE_ARGS"
premake5 $RIVE_PREMAKE_ARGS | grep -v '^Done ([1-9]*ms).$'

if [[ "$RIVE_NO_BUILD" = true ]]; then
    echo "Not building as nobuild was specified"
    exit 0
fi

# ---- Build dispatch ----

case "$RIVE_BUILD_SYSTEM" in
    export-compile-commands)
        rm -f "$SCRIPT_DIR/../compile_commands.json"
        cp "$RIVE_OUT/compile_commands/default.json" "$SCRIPT_DIR/../compile_commands.json"
        echo "compile_commands.json copied to project root"
        wc "$SCRIPT_DIR/../compile_commands.json"
        ;;
    gmake2)
        echo "make -C $RIVE_OUT -j$NUM_CORES $@"
        make -C "$RIVE_OUT" -j$NUM_CORES "$@"
        ;;
    ninja)
        echo "ninja -C $RIVE_OUT $@"
        ninja -C "$RIVE_OUT" "$@"
        ;;
    xcode4)
        if [[ $# = 0 ]]; then
            echo 'No targets specified for xcode: Attempting to grok them from "xcodebuild -list".'
            XCODE_SCHEMES=$(for f in $(xcodebuild -list -workspace "$RIVE_OUT/rive.xcworkspace" | grep '^        '); do printf " $f"; done)
            echo "  -> grokked:$XCODE_SCHEMES"
        else
            XCODE_SCHEMES="$@"
        fi
        for SCHEME in $XCODE_SCHEMES; do
            echo "xcodebuild -workspace $RIVE_OUT/rive.xcworkspace -scheme $SCHEME"
            xcodebuild -workspace "$RIVE_OUT/rive.xcworkspace" -scheme "$SCHEME"
        done
        ;;
    vs2022)
        MSVC_TARGETS=""
        for TARGET in "$@"; do
            MSVC_TARGETS="$MSVC_TARGETS -t:$TARGET"
        done
        echo "msbuild.exe ./$RIVE_OUT/rive.sln $MSVC_TARGETS"
        msbuild.exe "./$RIVE_OUT/rive.sln" $MSVC_TARGETS
        ;;
    *)
        echo "Unsupported build system: $RIVE_BUILD_SYSTEM"
        exit 1
        ;;
esac

# ---- Post-build ----

EXECUTABLE_NAME=rive_code_generator
if [[ "$HOST_MACHINE" = "windows" ]]; then
    EXECUTABLE="$EXECUTABLE_NAME.exe"
else
    EXECUTABLE="$EXECUTABLE_NAME"
fi

if [[ "$RIVE_BUILD_SYSTEM" != "export-compile-commands" ]]; then
    echo -e "\033[0;32m\nBuild complete: $RIVE_OUT/$EXECUTABLE\033[0m"
fi

if [[ $RUN = true ]]; then
    "$RIVE_OUT/$EXECUTABLE" --help
fi

if [[ $DEV = true ]]; then
    "$RIVE_OUT/$EXECUTABLE" -i ../samples/ -o "$RIVE_OUT/generated/rive_viewmodel.dart" -t ../templates/viewmodel_template.mustache
fi
