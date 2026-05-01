// Regex runtime facade over the native bridge.

import { RegexError, RegexFlag } from "./types"

export import class NativeRegexMatch from "./native_regex.hpp" {
  found(): bool
  value(): string
  start(): int
  end(): int
  nextSearchStart(previousStart: int): int
  captureCount(): int
  captureText(index: int): string
  captureStart(index: int): int
  captureEnd(index: int): int
  hasNamedCapture(name: string): bool
  namedCaptureText(name: string): string
  namedCaptureStart(name: string): int
  namedCaptureEnd(name: string): int
}

export import class NativeRegex from "./native_regex.hpp" {
  static compile(pattern: string, ignoreCase: bool, multiline: bool, dotAll: bool, extended: bool): Result<NativeRegex, string>
  test(input: string): bool
  find(input: string, startOffset: int): NativeRegexMatch
  replaceFirst(input: string, replacement: string): string
  replaceAll(input: string, replacement: string): string
}

export class Match {
  native: NativeRegexMatch
  value: string
  range: Tuple<int, int>
  captures: string[]
  captureRanges: Tuple<int, int>[]

  static fromNative(nativeMatch: NativeRegexMatch): Match {
    captures: string[] := []
    captureRanges: Tuple<int, int>[] := []

    for offset of 0..<nativeMatch.captureCount() {
      captureIndex := offset + 1
      captures.push(nativeMatch.captureText(captureIndex))
      captureRanges.push((nativeMatch.captureStart(captureIndex), nativeMatch.captureEnd(captureIndex)))
    }

    return Match {
      native: nativeMatch,
      value: nativeMatch.value(),
      range: (nativeMatch.start(), nativeMatch.end()),
      captures,
      captureRanges,
    }
  }

  capture(name: string): string | null {
    if !this.native.hasNamedCapture(name) {
      return null
    }

    return this.native.namedCaptureText(name)
  }

  captureRange(name: string): Tuple<int, int> | null {
    if !this.native.hasNamedCapture(name) {
      return null
    }

    return (this.native.namedCaptureStart(name), this.native.namedCaptureEnd(name))
  }
}

export class Regex {
  native: NativeRegex
  pattern: string
  flags: ReadonlySet<RegexFlag>

  static compileError(pattern: string, flags: ReadonlySet<RegexFlag>, message: string): RegexError {
    return RegexError {
      stage: "compile",
      pattern,
      flags,
      message,
    }
  }

  static compile(pattern: string, flags: ReadonlySet<RegexFlag> = []): Result<Regex, RegexError> {
    return case NativeRegex.compile(
      pattern,
      flags.has(RegexFlag.IgnoreCase),
      flags.has(RegexFlag.Multiline),
      flags.has(RegexFlag.DotAll),
      flags.has(RegexFlag.Extended),
    ) {
      s: Success -> Success {
        value: Regex {
          native: s.value,
          pattern,
          flags,
        }
      },
      f: Failure -> Failure {
        error: Regex.compileError(pattern, flags, f.error)
      }
    }
  }

  test(input: string): bool => this.native.test(input)

  find(input: string): Match | null {
    nativeMatch := this.native.find(input, 0)
    if !nativeMatch.found() {
      return null
    }

    return Match.fromNative(nativeMatch)
  }

  findAll(input: string): Match[] {
    matches: Match[] := []
    let startOffset = 0

    while true {
      nativeMatch := this.native.find(input, startOffset)
      if !nativeMatch.found() {
        return matches
      }

      matches.push(Match.fromNative(nativeMatch))

      nextOffset := nativeMatch.nextSearchStart(startOffset)
      if nextOffset <= startOffset {
        return matches
      }

      startOffset = nextOffset
    }
  }

  replaceFirst(input: string, replacement: string): string {
    return this.native.replaceFirst(input, replacement)
  }

  replaceAll(input: string, replacement: string): string {
    return this.native.replaceAll(input, replacement)
  }
}