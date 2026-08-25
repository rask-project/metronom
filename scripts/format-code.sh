#!/bin/bash
set -o pipefail

if [[ $1 == *"clang-format"* ]]; then
    CLANG_FORMAT_EXEC="$1"
    shift
else
    CLANG_FORMAT_EXEC="clang-format"
fi

if [ $# -eq 0 ]; then
    ADDITIONAL_ARGS=(-i)
else
    ADDITIONAL_ARGS=("$@")
fi

IN_PLACE=false
DRY_RUN=false
FORMAT_ARGS=()
for arg in "${ADDITIONAL_ARGS[@]}"; do
    case "$arg" in
    -i)
        IN_PLACE=true
        ;;
    -n|--dry-run)
        DRY_RUN=true
        ;;
    *)
        FORMAT_ARGS+=("$arg")
        ;;
    esac
done

status=0
format_file() {
    local file="$1"
    local assume_filename="$file"
    if [[ "$file" == *.h ]]; then
        assume_filename="$file.cpp"
    fi

    local tmp
    tmp="$(mktemp)" || {
        status=1
        return
    }

    if ! "$CLANG_FORMAT_EXEC" "${FORMAT_ARGS[@]}" --assume-filename="$assume_filename" < "$file" > "$tmp"; then
        rm -f "$tmp"
        status=1
        return
    fi

    if $DRY_RUN; then
        if ! cmp -s "$file" "$tmp"; then
            echo "$file: warning: code should be clang-formatted" >&2
            status=1
        fi
        rm -f "$tmp"
    elif $IN_PLACE; then
        mv "$tmp" "$file"
    else
        cat "$tmp"
        rm -f "$tmp"
    fi
}

while IFS= read -r -d '' file; do
    format_file "$file"
done < <(
    find . \
        \( -path ./build -o -path ./external \) -prune -o \
        \( -iname '*.c' -o -iname '*.cc' -o -iname '*.cpp' -o -iname '*.cxx' -o \
           -iname '*.h' -o -iname '*.hh' -o -iname '*.hpp' -o -iname '*.hxx' -o -iname '*.ipp' -o -iname '*.tpp' \) \
        -print0
)

exit $status
