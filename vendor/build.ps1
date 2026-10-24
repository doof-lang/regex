# Builds lib\pcre2-8.lib and installs the public PCRE2 header into include\.
# Runs inside a private copy of vendor\ with CC, AR, and CFLAGS supplied by
# the Doof compiler from a Visual Studio developer environment.
$ErrorActionPreference = 'Stop'

$library = 'pcre2-8.lib'
$defines = @('/DHAVE_CONFIG_H', '/DPCRE2_CODE_UNIT_WIDTH=8', '/DPCRE2_STATIC', '/DSUPPORT_PCRE2_8', '/DSUPPORT_UNICODE')
$includes = @('/Ipcre2\src')
$sources = 'auto_possess', 'chkdint', 'chartables', 'compile', 'compile_cgroup', 'compile_class', 'config',
  'context', 'convert', 'dfa_match', 'error', 'extuni', 'find_bracket', 'jit_compile', 'maketables', 'match',
  'match_data', 'match_next', 'newline', 'ord2utf', 'pattern_info', 'script_run', 'serialize', 'string_utils',
  'study', 'substitute', 'substring', 'tables', 'ucd', 'valid_utf', 'xclass' | ForEach-Object { "pcre2\src\pcre2_$_.c" }

function Install-Headers { Copy-Item pcre2\src\pcre2.h include\ }

New-Item -ItemType Directory -Force -Path include, lib, obj | Out-Null
Install-Headers

$flags = @()
if ($env:CFLAGS) { $flags = $env:CFLAGS -split ' ' | Where-Object { $_ -ne '' } }
$objects = @()
foreach ($source in $sources) {
  $object = 'obj\' + ($source -replace '[\\/]', '_' -replace '\.c$', '.obj')
  & $env:CC @flags @defines @includes /TC /c $source "/Fo$object"
  if ($LASTEXITCODE -ne 0) { throw "Compiling $source failed" }
  $objects += $object
}

& $env:AR /nologo "/OUT:lib\$library" @objects
if ($LASTEXITCODE -ne 0) { throw "Archiving $library failed" }
