#!/bin/bash

# Prevent the script from doing bad things
set -u  # Abort with unset variables

packagename="Clover"

# Go to the script directory to create the package
cd "$(dirname $0)"

declare -r PKGROOT="$PWD"
declare -r SYMROOT="$PKGROOT"/../sym
declare -r PKG_BUILD_DIR="${SYMROOT}"/package

# ====== LANGUAGE SETUP ======
export LANG='en_US.UTF-8'
export LC_COLLATE='C'
export LC_CTYPE='C'

# ====== REVISION/VERSION ======
declare -r CLOVER_VERSION='v2.0'
declare -r CLOVER_REVISION=$( cat "$PKGROOT"/../../vers.txt )

# ========== OPTIONS ===========
UPDATE_PO=0

while [[ $# -gt 0 ]]; do
    option=$1
    shift
    case "$option" in
        --update-po) UPDATE_PO=1 ;;
        -*)
            printf "Unrecognized option \`%s'\n" "$option" 1>&2
            exit 1
            ;;
    esac
done

PODIR="po"

if [[ "$UPDATE_PO" -ne 1 ]]; then
    # Copying po and pot files outside the repository
    po_tmpdir=$(mktemp -d -t po)
    ditto --noextattr --noqtn "${PKGROOT}"/po/  "$po_tmpdir"/
    # Automatically remove temporary directory at exit
    trap 'echo; rm -rf "$po_tmpdir"' EXIT
    PODIR="$po_tmpdir"
fi

PERLLIB=bin/po4a/lib                                               \
 bin/po4a/po4a                                                     \
 --package-name 'Clover'                                           \
 --package-version "${CLOVER_VERSION}-r${CLOVER_REVISION}"         \
 --msgmerge-opt '--lang=$lang'                                     \
 --variable PODIR="$PODIR"                                         \
 --variable TEMPLATES_DIR="Resources/templates"                    \
 --variable OUTPUT_DIR="${PKG_BUILD_DIR}/${packagename}/Resources" \
 po4a-clover.cfg


# Local Variables:      #
# mode: ksh             #
# tab-width: 4          #
# indent-tabs-mode: nil #
# End:                  #
#
# vi: set expandtab ts=4 sw=4 sts=4: #
