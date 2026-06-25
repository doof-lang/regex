# std/regex Guide

`std/regex` provides compiled regular expressions backed by PCRE2. Patterns are
compiled once into `Regex` values and can then be reused for tests, searches,
capture extraction, and replacement.

Doof automatically acquires the pinned PCRE2 source archive into `vendor/pcre2`
during build/test.

## Quick Start

```doof
import { Regex, RegexFlag } from "std/regex"

email := try! Regex.compile("(\\w+)@(\\w+\\.\\w+)")

if email.test("hello@example.com") {
  first := email.find("hello@example.com")
  if first != null {
    println(first!.captures[0])
  }
}

words := try! Regex.compile("\\w+", [RegexFlag.IgnoreCase])
redacted := words.replaceAll("hello world", "x")
```

## Compilation And Flags

`Regex.compile(pattern, flags)` returns `Result<Regex, RegexError>`. Invalid
patterns do not panic; the failure contains the original pattern, selected
flags, stage, and PCRE2 error message.

Flags:

- `IgnoreCase` enables case-insensitive matching.
- `Multiline` makes `^` and `$` match line starts and ends.
- `DotAll` lets `.` match newlines.
- `Extended` ignores unescaped whitespace and `#` comments in the pattern.

## Matches And Captures

`find(input)` returns the first match or `null`. `findAll(input)` returns all
non-overlapping matches. The native matcher advances carefully after each match,
so zero-width matches cannot cause an infinite loop.

`Match.captures` contains positional capture text, where index `0` corresponds
to regex group 1. `captureRanges` stores `(start, end)` offsets for each
positional capture. Use `capture(name)` and `captureRange(name)` for named
captures.

Offsets are byte offsets as reported by the native engine.

## Replacement

`replaceFirst(input, replacement)` and `replaceAll(input, replacement)` delegate
to PCRE2 replacement behavior and support `$1`-style backreferences.

## API

### `RegexFlag`

```doof
export enum RegexFlag {
  IgnoreCase = 0,
  Multiline = 1,
  DotAll = 2,
  Extended = 3,
}
```

Defined in [types.do](../types.do).

### `RegexError`

```doof
export class RegexError
```

Fields:

- `stage: string`
- `pattern: string`
- `flags: ReadonlySet<RegexFlag>`
- `message: string`

Defined in [types.do](../types.do).

### `Match`

```doof
export class Match
```

Fields:

- `value: string`
- `range: Tuple<int, int>`
- `captures: string[]`
- `captureRanges: Tuple<int, int>[]`

Methods:

- `capture(name: string): string | null`
- `captureRange(name: string): Tuple<int, int> | null`

Defined in [runtime.do](../runtime.do).

### `Regex`

```doof
export class Regex
```

Fields:

- `pattern: string`
- `flags: ReadonlySet<RegexFlag>`

Methods:

- `static compile(pattern: string, flags: ReadonlySet<RegexFlag> = []): Result<Regex, RegexError>`
- `test(input: string): bool`
- `find(input: string): Match | null`
- `findAll(input: string): Match[]`
- `replaceFirst(input: string, replacement: string): string`
- `replaceAll(input: string, replacement: string): string`

Defined in [runtime.do](../runtime.do).
