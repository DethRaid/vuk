# Vuk Examples

Look at these examples running on Android. Isn't Vuk amazing?????

Tested on a Pixel 4 XL - Adreno 640. Other phones may not like these examples. Run at your own risk

Note - Android NDK 25 has "C++20 support" but not really. The `libc++` in this version is woefully 
incomplete. You have to tell PLF::Colony to not use C++20. Comment out line 172 in 
`ext/plf_colony/plf_colony.h`