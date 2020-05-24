# ksysguard-network-microbenchmark

This repository contains a benchmark for testing the performance of [phabricator revision D29808](https://phabricator.kde.org/D29808).
Compile with `cmake`, `make` and run `regex-test`.

There are 3 implementations of parsing `/proc/net/tcp{,6}`. In this order:

||Original|D29808 proposed|Boost.Sphinx|
|---|---|---|---|
|Correctness:|Some bugs were found<ul><li>one condition was flipped</li><li>still gives wrong results, probably because the regex is wrong</li></ul>|One of the magic numbers was wrong. Now correct|Correct|
|Performance:|worst|best|ok|

When run, the program will print sums of the parsed data on the first line. They should be all 3 the same, unfortunately, there is a bug
in the original implementation that I haven't corrected which stems from wrong usage of regexes.

The second line shows the time of one call, in seconds.

You can change the [regex-test.data](regex-test.data) file to test the implementations on different data. It should be made just by copying the content of `/proc/net/tcp`.
