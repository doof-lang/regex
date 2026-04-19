# std/regex

Regular expression matching, search, and replacement. Backed by [PCRE2](https://www.pcre.org/) (bundled). Patterns are compiled once and reused. Named and positional captures are supported.

## Usage

```doof
import { Regex, RegexFlag } from "std/regex"

// Compile once, reuse many times
re := try Regex.compile("(\\w+)@(\\w+\\.\\w+)")

// Test
if re.test(email) {
  println("valid email")
}

// Find the first match
match := re.find("hello@example.com")
if match != null {
  println(match.value)       // "hello@example.com"
  println(match.captures[0]) // "hello"
  println(match.captures[1]) // "example.com"
}

// Find all matches
for m of re.findAll(text) {
  println(m.value)
}

// Replace
result := re.replaceAll(text, "[redacted]")
```

## Exports

### `RegexFlag`

Flags that modify how a pattern is compiled and matched.

| Member | Value | Description |
|--------|-------|-------------|
| `IgnoreCase` | `0` | Case-insensitive matching |
| `Multiline` | `1` | `^` and `$` match the start/end of each line |
| `DotAll` | `2` | `.` matches newline characters |
| `Extended` | `3` | Ignore unescaped whitespace and `#` comments in the pattern |

---

### `RegexError`

Returned by `Regex.compile` on failure.

| Field | Type | Description |
|-------|------|-------------|
| `stage` | `string` | Phase where the error occurred (always `"compile"`) |
| `pattern` | `string` | The pattern that failed to compile |
| `flags` | `ReadonlySet<RegexFlag>` | The flags that were in use |
| `message` | `string` | Human-readable error description from PCRE2 |

---

### `Match`

Describes a single regex match.

| Member | Type | Description |
|--------|------|-------------|
| `value` | `string` | The matched substring |
| `range` | `Tuple<int, int>` | `(start, end)` byte offsets of the match |
| `captures` | `string[]` | Positional capture group texts (index 0 = group 1) |
| `captureRanges` | `Tuple<int, int>[]` | `(start, end)` offsets for each capture |

#### `Match.capture(name: string): string | null`

Return the text of the named capture group, or `null` if the name does not exist.

#### `Match.captureRange(name: string): Tuple<int, int> | null`

Return the `(start, end)` offsets of the named capture group, or `null` if the name does not exist.

---

### `Regex`

A compiled regular expression.

#### `Regex.compile(pattern: string, flags?: ReadonlySet<RegexFlag>): Result<Regex, RegexError>`

Compile a PCRE2 pattern. Returns a `Failure` if the pattern is syntactically invalid.

```doof
re := try Regex.compile("^\\d{4}-\\d{2}-\\d{2}$")
caseInsensitive := try Regex.compile("hello", [RegexFlag.IgnoreCase])
```

#### `test(input: string): bool`

Return `true` if the pattern matches anywhere in `input`.

#### `find(input: string): Match | null`

Return the first `Match` in `input`, or `null` if there is no match.

#### `findAll(input: string): Match[]`

Return all non-overlapping matches in `input`.

#### `replaceFirst(input: string, replacement: string): string`

Return a copy of `input` with the first match replaced by `replacement`. Supports `$1`-style backreferences.

#### `replaceAll(input: string, replacement: string): string`

Return a copy of `input` with all matches replaced by `replacement`. Supports `$1`-style backreferences.
