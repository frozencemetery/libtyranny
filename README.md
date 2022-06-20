# libtyranny

If you've ever had to use libyaml for something legacy that you couldn't
migrate off using YAML altogether, you may have noticed that its interface is
rather unusual for a parser.  Specifically, it emits a stream of tokens
(rather than a parse tree) and is incremental - meaning you may get errors
pretty much whenever, but also that some errors will not be caught.

I submit that libyaml is much more of a lexer than a parser.  That's not
itself a problem, but there does need to be a parser until everything can be
migrated off YAML altogether.  This library attempts to fill that void: it is
a parser to the libyaml lexer.  More importantly, it's a few hundred lines of
C that anyone trying to use libyaml will need to write in some form.

I really dislike YAML, and recommend you use [almost anything
else](https://toml.io/en/).

Patches are welcome, especially if they fix bugs; however, while I'm willing
to review patches, I don't want to maintain this code long-term.  (If you are
in the unfortunate spot of needing code like this and are willing to do so,
reach out and we can talk.)

## Building

```
meson setup build
cd build
meson compile
meson install
```

## Technical details

libtyranny parses the libyaml stream using a recursive descent approach
(without any backtracking).  This cleanly mirrors the output parse tree, which
can be readily traversed.  The base type is string (`char *`) since, due to
YAML's bare words, there's no way to determine whether a scalar "should be" a
string, number, boolean, etc..

Everything is expected to work pretty much as expected, with the exception of
tags and anchors.  Tags and anchors aren't supported because I have yet to see
them used at all (let alone reasonably) in the wild... and also because I got
bored and wanted to do something else.  Patches for this are permissible.
