#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

shopt -s globstar nullglob
FILES=()
for f in src/**/*.h src/**/*.cc tests/**/*.h tests/**/*.cc; do
  [[ -f "$f" ]] && FILES+=("$f")
done
IFS=$'\n' FILES=($(printf "%s\n" "${FILES[@]}" | sort))
unset IFS

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "未找到符合条件的文件（仅统计 src/tests 下的 .h/.cc）"
  exit 0
fi

printf '%s\n' "${FILES[@]}" | awk '
function init_prefix(p) {
  files[p]=0; total[p]=0; code[p]=0; comment[p]=0; blank[p]=0; mixed[p]=0;
}

function update(kind, p) {
  total[p]++;
  if (kind=="blank") { blank[p]++; }
  else if (kind=="comment") { comment[p]++; }
  else if (kind=="mixed") { mixed[p]++; code[p]++; }
  else { code[p]++; }
}

function classify(line,   i,n,ch,nxt,in_string,escaped,has_code,has_comment) {
  if (line ~ /^[[:space:]]*$/) return "blank";

  n=length(line);
  in_string=0; escaped=0; has_code=0; has_comment=0;
  i=1;
  while (i<=n) {
    ch=substr(line,i,1);
    nxt=(i<n)?substr(line,i+1,1):"";

    if (in_block_comment) {
      has_comment=1;
      if (ch=="*" && nxt=="/") { in_block_comment=0; i+=2; continue; }
      i++;
      continue;
    }

    if (in_string) {
      has_code=1;
      if (escaped) escaped=0;
      else if (ch=="\\") escaped=1;
      else if (ch==string_quote) in_string=0;
      i++;
      continue;
    }

    if (ch=="/" && nxt=="/") { has_comment=1; break; }
    if (ch=="/" && nxt=="*") { has_comment=1; in_block_comment=1; i+=2; continue; }
    if (ch=="\"" || ch=="\047") { has_code=1; in_string=1; string_quote=ch; i++; continue; }
    if (ch !~ /[[:space:]]/) has_code=1;
    i++;
  }

  if (has_code && has_comment) return "mixed";
  if (has_comment && !has_code) return "comment";
  return "code";
}

BEGIN {
  init_prefix("src");
  init_prefix("tests");
  init_prefix(".h");
  init_prefix(".cc");
  init_prefix("all");
}

{
  path=$0;
  dir=(path ~ /^src\//) ? "src" : "tests";
  ext=(path ~ /\.h$/) ? ".h" : ".cc";
  files[dir]++; files[ext]++; files["all"]++;

  in_block_comment=0;
  while ((getline line < path) > 0) {
    kind=classify(line);
    update(kind, dir);
    update(kind, ext);
    update(kind, "all");
  }
  close(path);
}

function print_header(title) {
  print "";
  print title;
  print "------------------------------------------------------------------------------------------";
  printf "%-20s%8s%10s%10s%10s%10s%12s\n", "分类", "文件数", "总行数", "代码行", "注释行", "空行", "代码+注释";
  print "------------------------------------------------------------------------------------------";
}

function print_row(name) {
  printf "%-20s%8d%10d%10d%10d%10d%12d\n", name, files[name], total[name], code[name], comment[name], blank[name], mixed[name];
}

END {
  print "统计范围：src/tests 下的 .h/.cc 文件";
  print_header("按目录统计");
  print_row("src");
  print_row("tests");
  print_header("按后缀统计");
  print_row(".h");
  print_row(".cc");
  print_header("总计");
  print_row("all");
}
'
