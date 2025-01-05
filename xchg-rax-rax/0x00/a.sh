if [ -z "$1" ]; then
	echo "Usage: $0 <input-file>"
	exit 1
fi

input_file_with_ext="$1"
input_file="${input_file_with_ext%.*}"
listing_file="${input_file}.lst"
output_file="${input_file}.com"

nasm -f bin "${input_file_with_ext}" -l "${listing_file}" -o "${output_file}"

