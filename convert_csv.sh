#!/bin/bash

echo 'Sorting geotags by id...'
sort -nt, -k1,1 geotag/geotag.csv >geotag/geotag_sorted_by_id.csv

echo 'Sorting tags by id...'
sort -nt, -k1,1 geotag/tag.csv >geotag/tag_sorted_by_id.csv

echo 'Merging geotags and tags...'
join -t, -j1 geotag/tag_sorted_by_id.csv geotag/geotag_sorted_by_id.csv >geotag/merged.csv

echo 'Sorting by length, date...'
./csvsearch.out -c geotag/merged.csv | sort -t, -k3n -k2,2 -k4r | cut -d, -f1-2,4- >geotag/merged_sorted_by_len_date.csv

echo 'Minimizing...'
sed -E 's@^([0-9]{,10}),([^,]*),"([0-9]{4})-([0-9]{2})-([0-9]{2}) ([0-9]{2}):([0-9]{2}):([0-9]{2})",(.*)$@\2,\3\4\5\6\7\8,\9@' geotag/merged_sorted_by_len_date.csv | sed -E 's@^([^,]*),([^,]*),([-e0-9.]*),([-e0-9.]*),http://farm([0-9])\.static\.flickr\.com/([0-9]{,4})/([0-9]{,10})_([0-9a-f]{10})\.jpg$@\1,\3,\4,\2\5\6,\7,\8@' >geotag/merged_sorted_by_len_date_min.csv

echo 'Limiting 100 records per geotag...'
./csvsearch.out -m geotag/merged_sorted_by_len_date_min.csv >geotag/merged_sorted_by_len_date_min_100.csv

echo 'Creating binary...'
./csvsearch.out -b geotag/data.bin geotag/merged_sorted_by_len_date_min_100.csv
