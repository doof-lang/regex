// Public regex enums and error shapes.

export enum RegexFlag {
  IgnoreCase = 0,
  Multiline = 1,
  DotAll = 2,
  Extended = 3,
}

export class RegexError {
  stage: string
  pattern: string
  flags: ReadonlySet<RegexFlag>
  message: string
}