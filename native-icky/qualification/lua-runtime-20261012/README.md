# Standalone modified-Lua qualification

The unchanged Flexible Pipes qualifier at
5caaba31256bfb424638f8a3d4d554c56fa9a970 compiled and executed the actual Lua fork
87306483cec50f8c750a22dda1d0742246fad756 through the retained ICK producer.
The standalone and embedded symbolic-assignment probes passed. The embedded
source was byte-identical to the checked input; no dynamic Lua library was
linked. See receipt.tsv and the two captured probe outputs.

The producer came from FastChat workflow 37589849631, artifact 11468256648,
archive SHA256 03a86b94e862b1d726ffc369407168145485a055322fda46370a99ccde2e0475.
Its source is ICK 515c0f29fe6e2e96e10495fbaf25da93532e7722. The exact permitted
objcopy --strip-debug derivative of cc1 was used; the shared qualifier checks
its digest. This producer accepts C assignment arrows but cannot compile this
port's multiplication glyph. The native bridge therefore uses current ICK
c61e448251744a2f40ad743ebef1a027bdcd2f9d in a separately recorded stage.

Paths in the receipt are relative to the temporary replay root. No binary is
bundled in this directory. These receipts prove the modified runtime and raw
source probe on the recorded Linux host. They do not prove execution of this
Grease port, Android/Bionic, or a physical device. The port's exact-head C and
Lua gates remain separate requirements.
