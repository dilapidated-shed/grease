BEGIN {
  FS = "\t"
  OFS = "\t"
}
NR == 1 {
  next
}
{
  key = $1 SUBSEP $2
  if (!(key in seen)) {
    seen[key] = 1
    order[++groups] = key
    case_name[key] = $1
    implementation[key] = $2
  }
  count[key]++
  value[key, count[key]] = $4 + 0
}
END {
  print "case", "implementation", "n", "min_ns", "p10_ns", "median_ns", "p90_ns"
  for (g = 1; g <= groups; g++) {
    key = order[g]
    n = count[key]

    for (i = 2; i <= n; i++) {
      x = value[key, i]
      j = i - 1
      while (j >= 1 && value[key, j] > x) {
        value[key, j + 1] = value[key, j]
        j--
      }
      value[key, j + 1] = x
    }

    p10 = int((n - 1) * 0.10) + 1
    p50 = int((n - 1) * 0.50) + 1
    p90 = int((n - 1) * 0.90) + 1

    print case_name[key], implementation[key], n, value[key, 1],       value[key, p10], value[key, p50], value[key, p90]
  }
}
