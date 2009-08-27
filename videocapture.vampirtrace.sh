#! /bin/bash

# no flush limit
# manual demangling
# unique prefix for easier deletion
# bigger buffer for fewer flushes
VT_MAX_FLUSHES=0 VT_GNU_DEMANGLE=yes VT_FILE_PREFIX=vt_videocapture VT_BUFFER_SIZE=1024M VT_PFORM_GDIR=./ VT_PFORM_LDIR=/tmp/ ./videocapture

