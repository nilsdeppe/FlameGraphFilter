# FlameGraphFilter

FlameGraphFilter is used to reduce the amount of data displayed on inverse
[flamegraphs](https://github.com/brendangregg/FlameGraph). The main use case I
found was to construct a "bottom-up" or
callgraph of callees view as a flamegraph. If all the low-percentage functions
are included viewing the flamegraph in a browser becomes slow. However, by
ignoring functions that take less than 0.5% of the total runtime and by limiting
the displayed stack depth a general overview of which functions are consuming
the most time can be quickly obtained. The next step is to filter the flamegraph
to only display the functions you are interested in and deepen the stack
trace. It is then easy to get a detailed overview of how the slow functions are
being invoked and where code changes are necessary.

**Note:** I work with C++ code and so FlameGraphFilter was designed to suit my
needs there.

# Installation

To install FlameGraphFilter you must have a working C++ compiler,
Boost.ProgramOptions installed, and CMake installed. To build FlameGraphFilter:

- `git clone FLAMEGRAPH`
- `cd ./FlameGraphFilter && mkdir build && cd build && cmake .. && make`

# Tutorial

You should first familiarize yourself with
[FlameGraph](https://github.com/brendangregg/FlameGraph). Once you understand
the basics of FlameGraph then you can use the filter to improve your profiling
experience.

FrameGraphFilter takes the folded stacks produced by the `stackcollapse*.pl`
scripts that are part FrameGraph and limits the data that will be displayed on
the reverse FlameGraph. I prefer thinking of performance from the call graph of
callees/bottom-up perspective, which is where FlameGraph tends to struggle with
too much data. FlameGraphFilter allows you to show only functions that take up,
say more than 0.5% of the total run time, and to also limit the stack depth
displayed. These two features allow you to quickly figure out which parts of
your code that are consuming the most time. You can then filter the data to only
show the lowest level functions that match user-specified regular expressions.

Let's start off after the
[Fold stacks](https://github.com/brendangregg/FlameGraph#2-fold-stacks) section
of the FlameGraph tutorial. You should have a collapsed stack to work with. If
you render the raw stack with `framegraph --reverse` the resulting file might be
quite large (well over 5MB). These large SVGs tend to render quite slowly
sometimes and so we want to reduce the amount irrelevant information. To do this
run `flamegraphfilter --stack-limit 8 --cutoff-percentage -o out.folded.filtered
out.folded`. This will generate a new folded stack file named
`out.folded.filtered` that only contains functions that take up more than 0.5%
of the total run time, and only shows them to a stack depth of 8. Now we can
generate a new SVG using `flamegraph --reverse out.folded.filtered >
filtered.svg` which will be quicker too load. If load times are still too long
try either increasing the cutoff percentage or decreasing the stack limit. With
a quick loading flamegraph you should be able to identify the functions that are
taking up the most time. Let's say it `malloc` and the member functions of a
`Vector` class.

To only see what these functions are doing we create a new folded file using
`flamegraphfilter --cutoff-percentage 0 --show malloc --show "Vector.*" -o
out.folded.malloc_vector out.folded`. We are interested in seeing all functions
that satisfy the regular expressions so we set the cutoff percentage to `0`. The
default stack-limit is `0`, which means show the whole stack. Next run
`flamegraph --reverse out.folded.malloc_vector > malloc_vector.svg` to get a
flamegraph that loads quickly and shows the full stack so you can analyze how
the slow functions were called.

# Contributing

Contributions are more than welcome, and if you're more capable than I am at
writing perl code I think it would be great to have this functionality added
into FlameGraph itself :)

# Maintenance

I expect that FlameGraphFilter will be quite low maintenance since it very
quickly suited my needs completely. Thus, just because there's no activity for a
long time does not mean the software isn't being updated, it just might not need
updates often :)
