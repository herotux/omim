#!/bin/bash
################################################
# Builds whole planet in /media/ssd/common dir #
################################################

# At least "set -e -u" should always be here, not just for debugging!
# "set -x" is useful to see what is going on.
set -e -u -x

# displays usage and exits
function Usage {
  echo ''
  echo "Usage: $0 [path_to_data_folder_with_classsif_and_planet.osm.bz2] [bucketing_level] [optional_path_to_intermediate_data]"
  echo "Note, that for coastlines bucketing_level should be 0"
  echo "Planet squares size is (2^bucketing_level x 2^bucketing_level)"
  echo "If optional intermediate path is given, only second pass will be executed"
  exit 0
}

if [ $# -lt 2 ]; then
  Usage
fi

BUCKETING_LEVEL=$2

# set up necessary Windows MinGW settings
#if [ ${WINDIR+1} ]; then
#fi

# check if we have QT in PATH
if [ ! `which qmake` ]; then
  echo 'You should add your qmake binary into the PATH. This can be done in 2 ways:'
  echo '  1. Set it temporarily by executing: export PATH=/c/qt/your_qt_dir/bin:$PATH'
  echo '  2. Set it permanently by adding export... string above to your ~/.bashrc'
  echo 'Hint: for second solution you can type from git bash console: notepad ~/.bashrc'
  exit 0
fi

# determine script path
MY_PATH=`dirname $0`

# find indexer_tool
IT_PATHS_ARRAY=(  "$MY_PATH/../../../omim-build-release/out/release/indexer_tool" \
                  "$MY_PATH/../../out/release/indexer_tool" )

for i in {0..1}; do
  if [ -x ${IT_PATHS_ARRAY[i]} ]; then
    INDEXER_TOOL=${IT_PATHS_ARRAY[i]}
    echo TOOL: $INDEXER_TOOL
    break
  fi
done

if [[ ! -n $INDEXER_TOOL ]]; then
  echo 'No indexer_tool found, please build omim-build-release or omim/out/release'
  echo ""
  Usage
fi

OSM_BZ2=$1/planet.osm.bz2

if ! [ -f $OSM_BZ2 ]; then
  echo "Can't open file $OSM_BZ2, did you forgot to specify dataDir?"
  echo ""
  Usage
fi

TMPDIR=$1/intermediate_data/

if [ $# -ge 3 ]; then
  TMPDIR=$3/
fi

if ! [ -d $TMPDIR ]; then
  mkdir -p $TMPDIR
fi

PV="cat"
if [ `which pv` ]
then
  PV=pv
fi

LIGHT_NODES=false

# skip 1st pass if intermediate data path was given
if [ $# -lt 3 ]; then
  $PV $OSM_BZ2 | bzip2 -d | $INDEXER_TOOL --intermediate_data_path=$TMPDIR \
    --use_light_nodes=$LIGHT_NODES \
    --generate_intermediate_data
fi

$PV $OSM_BZ2 | bzip2 -d | $INDEXER_TOOL --intermediate_data_path=$TMPDIR \
  --use_light_nodes=$LIGHT_NODES --bucketing_level=$BUCKETING_LEVEL \
  --generate_final_data --sort_features=true --generate_index --output=101103 --worldmap_max_zoom=5
