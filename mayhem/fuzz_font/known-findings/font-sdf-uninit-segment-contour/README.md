# font_sdf: `ClassifySegmentBoundaries` memset(NULL) after reading a bogus `m_Contour`

Reproducer: `crash-cf74b2c7b036b92bf479bb5dfee3426143c5dfc5` (a TrueType font, 124800 bytes). Run it with
`/mayhem/fuzz_font-standalone <file>`.

## Symptom

```
engine/font/src/font_sdf.cpp:1144:12: runtime error: null pointer passed as argument 1, which is declared to never be null
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior engine/font/src/font_sdf.cpp:1144:12
```

## Cause

`ClassifySegmentBoundaries` sizes its scratch arrays from `segments[segments.Size() - 1].m_Contour + 1`
(`font_sdf.cpp:1131`). For this glyph outline that value is not a valid contour index (an earlier run read
`0xBEBEBEBE`, ASan's malloc fill, i.e. an uninitialised `FontSDFSegment` field; here the sum wraps to a
zero-sized `dmArray`). `SetCapacity` then leaves the buffer null and `memset(intersecting_contours.Begin(), ...)`
at line 1144 is passed NULL. If the sum did not wrap, the indexing at lines 1162-1163 would write out of bounds
instead.

## Impact

Reachable from `FontGetGlyphByIndex` with `GenerateOutline`/`GenerateImage` on an untrusted TTF: undefined
behaviour (null `memset`), and a potential out-of-bounds write on `intersecting_contours`.

## One-line upstream fix

Return early from `ClassifySegmentBoundaries` unless the last segment's `m_Contour` is a valid contour index
(and make `BuildSegments` skip draw commands that arrive before any `FONT_OUTLINE_MOVE_TO`, when `contour` is
still `UINT32_MAX`).
