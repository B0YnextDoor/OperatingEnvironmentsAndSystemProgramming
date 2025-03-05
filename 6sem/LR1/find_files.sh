#!/bin/bash

search_directory() {
	local dir="$1"
	local header="$2"
	local regex_mode="$3"
	shift 3
	local patterns=("$@")
	for entry in "$dir"/*; do
		if [ -d "$entry" ]; then
			search_directory "$entry" "$header" "$regex_mode" "${patterns[@]}"
		elif [ -f "$entry" ]; then
			file_name=$(basename "$entry")
			process_file "$entry" "$file_name" "$header" "$regex_mode" "${patterns[@]}"
		fi
	done
}

process_file() {
	local file_path="$1"
	local file_name="$2"
	local header="$3"
	local regex_mode="$4"
	shift 4
	local patterns=("$@")

	local match=0

	if [ "$regex_mode" -eq 1 ]; then
		if [[ "$file_name" =~ ${patterns[0]} ]]; then
			match=1
		fi
	else
		for pattern in "${patterns[@]}"; do
			if [[ "$file_name" == "$pattern" ]]; then
				match=1
				break
			fi
		done
	fi

	if [ "$match" -eq 1 ]; then
		first_line=$(head -n 1 "$file_path")
		if [[ "$first_line" == "$header" ]]; then
			echo "==== $file_path ===="
			awk '{print NR ": " $0}' "$file_path"
		fi
	fi
}

if [ "$#" -lt 2 ]; then
	echo "Usage: $0 [-r <regex>] <header> <file1 file2 ...>"
	exit 1
fi

regex_mode=0
regex=""

if [ "$1" == "-r" ]; then
	if [ "$#" -lt 3 ]; then
		echo "Usage: $0 [-r <regex>] <header> <file1 file2 ...>"
		exit 1
	fi
	regex_mode=1
	regex="$2"
	header="$3"
	shift 3
else
	header="$1"
	shift
fi

patterns=("$@")

if [ "$regex_mode" -eq 1 ]; then
	search_directory "." "$header" "$regex_mode" "$regex"
else
	search_directory "." "$header" "$regex_mode" "${patterns[@]}"
fi
