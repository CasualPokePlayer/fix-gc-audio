Since 5.0-5700, a hardware bug of the GameCube was emulated revolving the sample rate. This resulted in the sample rate being changed to 32029/48043Hz. This however was inaccurate, as the GC actually outputs at a non-integer sample rate. Rounding to an integer backfired here, as these sampe rates do not have an even CPU cycles per sample rate. So the code ends up actually outputting different sample rates, which were several hertz off (32024.2488139/48040.3301537Hz). Since 5.0-16788, this has been corrected, so the correct sample rates are output and this discrepancy no longer exists (32028.4697509/48042.7046263Hz). However, wav files can only store an integer sample rate in their header, so while the code outputs an accurate sample rate, it is rounded off in the wav header (or more so it's now less than a hertz off from reality).

This program resolves the issue by simply taking wav files and resampling them back to 48000Hz. It will be able to correctly identify the true sample rate from any Dolphin wav file, including pre-5.0-5700 builds. Usage is simple, if you only have a single file you can just drag and drop the file in. If you have multiple files (i.e. split due to changing sample rates) you can give all of them at once, and it will concat them together. The order used is the order given, so you probably want to make a simple batch script to give all the files, e.g.

```batch
fix_gc_audio.exe "path1.wav" "path2.wav" "path3.wav"
```

Of course, anything before 5.0-5700 along with Wii games on any Dolphin version do not have this issue, so this program isnt super useful. It can be used just for concatting multiple audio files together, but that's already possible with AviSynth anyways.
