
set -euo pipefail

if (($# != 2)); then
    echo 'usage: bash cmake/strings.sh <strings.json> <output.hpp>' >&2
    exit 2
fi
input=$1
output=$2
if [[ ! -f $input || ! -r $input ]]; then
    echo "strings: cannot read $input" >&2
    exit 1
fi

mkdir -p -- "$(dirname -- "$output")"
tmp=$(mktemp -- "$output.XXXXXX")
trap 'rm -f -- "$tmp"' EXIT
trap 'exit 1' INT TERM HUP

"${JQ:-jq}" -nr --arg file "$input" '
def caps: ["SectionWarfare", "SectionCities", "SectionDiplomacy"];
def widthkeys: caps + ["SectionBadge"];
def names: map(tojson) | join(", ");
def lit: "u" + tojson;
def num: . + 0 | tostring;

def text($at; $source):
    if type != "string" then "\($at): expected a string, got \(type)"
    elif . == "" then "\($at): empty string"
    elif explode | any(. < 32) then "\($at): control character"
    elif test("\\A\\s|\\s\\z") then "\($at): leading or trailing whitespace"
    elif ($source | type) == "string" and contains("#") != ($source | contains("#")) then
        "\($at): # placeholder does not match the source text"
    else empty end;

def widths($c):
    if has("widths") | not then "\($c): missing widths"
    elif .widths | type != "object" then "\($c).widths: expected an object, got \(.widths | type)"
    else
        (widthkeys - (.widths | keys_unsorted) | select(. != []) | "\($c).widths: missing keys \(names)"),
        (.widths | keys_unsorted - widthkeys | select(. != []) | "\($c).widths: unknown keys \(names)"),
        (.widths | to_entries[] | select(.key | IN(widthkeys[]))
            | select(.value | type == "number" and . == floor and . >= 0 and . <= 65535 | not)
            | "\($c).widths.\(.key): expected an integer in 0..65535, got \(.value | tojson)")
    end;

def problems:
    keys_unsorted[0] as $src
    | .[$src] as $first
    | [$first | objects | keys_unsorted[] | select(. != "widths")] as $ids
    | if $first | type != "object" then "\($src): expected an object, got \($first | type)"
    elif $ids == [] then "\($src): no text ids"
    else
        ($ids[] | select(test("\\A[A-Za-z_][A-Za-z0-9_]*\\z") | not) | "\($src): text id \(tojson) is not a C++ identifier"),
        (to_entries[] | .key as $c | .value as $t
            | (select($c | test("\\A[a-z]{2}-[a-z]{2}\\z") | not) | "locale code \($c | tojson) does not match xx-yy"),
            if $t | type != "object" then "\($c): expected an object, got \($t | type)"
            else
                ($ids - ($t | keys_unsorted) | select(. != []) | "\($c): missing keys \(names)"),
                ($t | keys_unsorted - ["widths"] - $ids | select(. != []) | "\($c): unknown keys \(names)"),
                ($ids[] as $k | $t | select(has($k)) | .[$k] | text("\($c).\($k)"; $first[$k])),
                ($t | widths($c))
            end)
    end;

def render:
    . as $data
    | keys_unsorted as $codes
    | ($data[$codes[0]] | keys_unsorted - ["widths"]) as $ids
    | [
        "constexpr unsigned LocaleCount = \($codes | length);",
        "enum Text : unsigned { \($ids | join(", ")), TextCount };",
        "#ifdef CIV_TEXT_TABLES",
        "const char LocaleCodes[LocaleCount][6] = {\($codes | names)};",
        "const char16_t* const Strings[LocaleCount][TextCount] = {",
        ($codes[] as $c | "    {\([$data[$c][$ids[]] | lit] | join(", "))},"),
        "};",
        "const unsigned short CaptionWidths[LocaleCount][3] = {\(
            [$codes[] as $c | "{\([$data[$c].widths[caps[]] | num] | join(", "))}"] | join(", "))};",
        "const unsigned short BadgeWidths[LocaleCount] = {\([$data[$codes[]].widths.SectionBadge | num] | join(", "))};",
        "#endif"
    ]
    | join("\n");

try (
    (try [inputs] catch error("\($file): invalid JSON: \(.)")) as $docs
    | if $docs | length != 1 then error("\($file): expected one JSON value, found \($docs | length)") else $docs[0] end
    | if type != "object" or length == 0 then error("\($file): top level must be a non-empty object of locales") else . end
    | [problems] as $errors
    | if $errors == [] then render else error($errors | join("\nstrings: ")) end
) catch ("strings: \(.)\n" | halt_error(1))
' -- "$input" >"$tmp"

chmod 644 -- "$tmp"
mv -f -- "$tmp" "$output"

