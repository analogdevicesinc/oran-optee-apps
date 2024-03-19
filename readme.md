# Source

Source for OP-TEE Trusted Applications (TAs) and their Linux host counterparts. One directory per TA. A TA can be built as either (1) A "regular" TA (stored on the rootfs and loaded by tee-supplicant) or (2) an "Early TA" (compiled into the OP-TEE OS binary and available as soon as OP-TEE is booted). See https://optee.readthedocs.io/en/latest/building/trusted_applications.html for more info on TAs. See https://optee.readthedocs.io/en/latest/architecture/trusted_applications.html#early-ta for more info in Early TAs.

## Examples

Example     | Contents
----------- | -------------------------
hello_world | Example of an early TA
foo_bar     | Example of a regular TA

To differentiate between a regular TA and early TA, the TA portion of the app should be placed in either a "ta" or "early_ta" directory (see examples above). The build system will pick up on this difference and compile and link accordingly. Buildroot places OP-TEE host applications in /usr/bin.
