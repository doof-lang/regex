import { Match, Regex, RegexFlag } from "./index"

function requireRegex(pattern: string, flags: ReadonlySet<RegexFlag> = []): Regex {
  let regex: Regex | null = null

  case Regex.compile(pattern, flags) {
    s: Success -> {
      regex = s.value
    }
    f: Failure -> {
      assert(false, "expected regex compile success: ${f.error.message}")
    }
  }

  assert(regex != null, "expected regex compile helper to produce a value")
  return regex!
}

export function testCompileRejectsInvalidPatterns(): void {
  case Regex.compile("[abc") {
    s: Success -> assert(false, "expected invalid regex compile to fail")
    f: Failure -> assert(f.error.stage == "compile", "expected compile failures to use the compile stage")
  }
}

export function testIgnoreCaseFlagControlsTest(): void {
  regex := requireRegex("doof", [RegexFlag.IgnoreCase])
  assert(regex.test("DOOF"), "expected IgnoreCase to enable case-insensitive matches")
}

export function testFindExposesNamedCapturesAndRanges(): void {
  regex := requireRegex("(?P<year>[0-9]{4})-(?P<month>[0-9]{2})-(?P<day>[0-9]{2})")
  hit := regex.find("released on 2026-03-30")
  assert(hit != null, "expected the date regex to find a match")

  [year, month, day] := hit!.captures
  assert(year == "2026", "expected the first positional capture")
  assert(month == "03", "expected the second positional capture")
  assert(day == "30", "expected the third positional capture")
  assert(hit!.capture("year") == "2026", "expected named capture lookup to resolve the year")

  range := hit!.captureRange("month")
  assert(range != null, "expected named capture range lookup to resolve the month")
  (start, end) := range!
  assert(start == 17, "expected the month capture start offset")
  assert(end == 19, "expected the month capture end offset")
}

export function testFindAllReturnsAllMatches(): void {
  regex := requireRegex("[A-Z]+-[0-9]+")
  matches := regex.findAll("DOOF-101 / CORE-202 / ui")
  assert(matches.length == 2, "expected two uppercase ticket matches")
  assert(matches[0].value == "DOOF-101", "expected the first match value")
  assert(matches[1].value == "CORE-202", "expected the second match value")
}

export function testDotAllAllowsCrossLineMatches(): void {
  regex := requireRegex("build.release", [RegexFlag.DotAll])
  assert(regex.test("build\nrelease"), "expected DotAll to allow dot to match newline")
}

export function testMultilineAllowsLineAnchorsWithinInput(): void {
  regex := requireRegex("^beta$", [RegexFlag.Multiline])
  assert(regex.test("alpha\nbeta\ngamma"), "expected Multiline to let anchors match internal lines")
}

export function testExtendedIgnoresUnescapedWhitespace(): void {
  regex := requireRegex("a b c", [RegexFlag.Extended])
  assert(regex.test("abc"), "expected Extended to ignore literal pattern whitespace")
}

export function testOptionalCapturesPreserveParallelArrays(): void {
  regex := requireRegex("(a)?(b)")
  hit := regex.find("b")
  assert(hit != null, "expected the optional-capture regex to match")
  assert(hit!.captures.length == 2, "expected both capture slots to exist")
  assert(hit!.captures[0] == "", "expected unmatched captures to surface as empty strings")

  (start, end) := hit!.captureRanges[0]
  assert(start == -1, "expected unmatched capture ranges to use -1 for the start")
  assert(end == -1, "expected unmatched capture ranges to use -1 for the end")
}

export function testReplaceOperationsUseBackreferences(): void {
  regex := requireRegex("([A-Z]+)-([0-9]+)")
  assert(
    regex.replaceFirst("DOOF-101 / CORE-202", "$2:$1") == "101:DOOF / CORE-202",
    "expected replaceFirst to substitute the first match only"
  )
  assert(
    regex.replaceAll("DOOF-101 / CORE-202", "$2:$1") == "101:DOOF / 202:CORE",
    "expected replaceAll to substitute every match"
  )
}