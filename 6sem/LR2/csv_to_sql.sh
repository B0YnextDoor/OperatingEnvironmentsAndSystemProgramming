#!/bin/bash

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <input_file.csv>"
	exit 1
fi

INPUT_FILE="$1"

if [ ! -f "$INPUT_FILE" ]; then
	echo "Error: File '$INPUT_FILE' not found."
	exit 1
fi

if [ ! -s "$INPUT_FILE" ]; then
	echo "Error: File '$INPUT_FILE' is empty."
	exit 1
fi

TABLE_NAME=$(head -n 1 "$INPUT_FILE" | tr -d '\r\n')

if [ -z "$TABLE_NAME" ]; then
	echo "Error: Table name is missing in the first line of the file."
	exit 1
fi

COLUMN_NAMES=$(sed -n '2p' "$INPUT_FILE" | tr -d '\r\n')

if [ -z "$COLUMN_NAMES" ]; then
	echo "Error: Column names are missing in the second line of the file."
	exit 1
fi

IFS=',' read -r -a COLUMNS <<<"$COLUMN_NAMES"

if [ "$(wc -l <"$INPUT_FILE")" -le 2 ]; then
	echo "Error: No data found in the file."
	exit 1
fi

OUTPUT_FILE="${INPUT_FILE%.csv}.sql"

{
	echo "-- SQL INSERT statements generated from $INPUT_FILE"
	echo ""

	tail -n +3 "$INPUT_FILE" | while IFS=',' read -r -a DATA; do
		if [ "${#DATA[@]}" -ne "${#COLUMNS[@]}" ]; then
			echo "Error: Data row does not match column count. Skipping row." >&2
			continue
		fi
		VALUES=""
		for VALUE in "${DATA[@]}"; do
			ESCAPED_VALUE=$(echo "$VALUE" | sed "s/'/''/g")
			VALUES="${VALUES},'${ESCAPED_VALUE}'"
		done
		VALUES="${VALUES:1}"
		echo "INSERT INTO $TABLE_NAME (${COLUMN_NAMES}) VALUES (${VALUES});"
	done
} >"$OUTPUT_FILE"

echo "SQL statements have been written to $OUTPUT_FILE"
