# v3x4
Intel(R) Xeon(R) v3 Processor SuperTurbo UEFI DXE driver

Description:

- Programs Intel(R) Xeon(R) "Haswell-E/EP(4S)/EX" E5/E7 v3 processors (cpuid = 306F2h, 306F3h, 306F4h) on X99 (single), C612 (dual), including QUAD (C602J?) and ABOVE (presumed up to 8S albiet unverified at this time) platforms to allow for maximum all-core turbo boost for all cores regardless of whether there are motherboard options present for overclocking/voltage control or not. For example, the 18-core Xeon(R) E5-2696 v3 processor has set from the factory an all-core turbo of 2.8GHz. This driver programs the highest un-fused ratio (i.e. the 1C Turbo bin) as the new Turbo bin for all boost configurations including all-core turbo. In other words, the 1C turbo bin becomes the all-core turbo bin and the E5-2696 v3 processor now demonstrates an all-core turbo of 3.8GHz!

- Allows for per-package, dynamic undervolting (retains PCU control while applying a fixed negative Vcore offset) IA (i.e. Core), CLR (CBo/LLC/Ring) a.k.a Uncore, and System Agent (SA) voltage domains independently which provides for higher all-core sustained clocks during heavy workloads, including AVX2 workloads

- Allows for setting static Uncore ratio for maximum performance (lowest typical access latency and accompanying maximum throughput) or setting to limit less than maximum (typical 30x). It is possible to trade cache speed for Core speed and studies show that 100MHz of Core speed-up is roughly equivalent to 1GHz of cache speed-up. That being said, lowering your Uncore power budget to make it to that next-higher Core speed bin is often a worthwhile trade-off

- Allows to disable CPU SVID telemetry (a.k.a. "PowerCut") which may reduce or remove altogether TDP power limitations for some system combinations. Allows to set a fixed VCCIN voltage (not recommended if available to be set in BIOS)

- Driver is designed to work on up to 8S systems. Verified functional on multiple 1S, 2S, and 4S systems with accompanying modified BIOS (remove any microcode revision update patches)

Successful use requirements:

- Haswell-E/EP(4S)/EX processor (cpuid = 306F2h, 306F3h, 306F4h) or processors. This can be overriden at compile time

- CPU microcode revision patch must *not* be loaded during POST process (requires modified BIOS). Instructions on how to modify BIOS ROM image to remove microcode for external SPI programming will *not* be provided here

- Use EFI Shellx64 or other suitable UEFI shell to set automatic load (and execution) of v3x4.efi during system boot.  Use cmd ' bcfg driver add 0 fs1:\EFI\Boot\v3x4.efi "V3 Full Turbo" ' where 'fs1:\EFI\Boot\v3x4.efi' is path to DXE driver file on UEFI boot partition (use cmd 'mountvol x: /s' to mount in Windows as drive X: for writing) {Note: there is a bug which prevents accessing the mounted drive through File Explorer in Windows 10 build 1803. Workaround is to open Task Manager and use new process launch dialog to navigate to mounted drive and copy directly}. Toggle enable/disable in BIOS (if presented as an option; default: enabled)

"Features:"
- If you program too great a negative of an offset voltage for any of the enabled domains, this is the equivalent of setting too low a VID in BIOS and expecting your system to be remain stable. Since this is always done (no recovery), then to recover you would need to temporarily disconnect the disk source for the drive binary (preventing load with automatic bypass). Similiar to the trial-and-error method of overclocking, so will there be here, too
- Driver will abort if microcode revision update patch detected and there is no point in defeating this feature as this is a hard requirement for successful driver execution
- Driver will abort if not expected CPUID. This can be programmed to whatever you want or entirely overridden using special build flag at compile time. At this time, only Haswell-E/EP(4S)/EX has been verified workable

To COMPILE for point-releases, in Windows:

1) Download and install UDK2015: http://www.tianocore.org/udk/udk2015/
Note: will also in turn require Windows 2003 DDK 3790.1830 installed (separate download)

2) Any C-compiler that is supported by UDK2015. Currently verified working with Visual Studio 2015

3) Edit file [UDK2015]/BaseTools/Conf/target.txt to change and/or verify proper the following parameters:

ACTIVE_PLATFORM = MdeModulePkg/MdeModulePkg.dsc

TARGET = RELEASE

TARGET_ARCH = X64

TOOL_CHAIN_TAG = VS2015x86 (or VS2013x86, etc.)

4) open admin command prompt and change path to [UDK2015]/BaseTools

5) Execute edksetup(.bat)

6) Execute build. If it prints "- Done -" all is fine and the UDK2015 build environment is properly setup  ...
 
7) Unpack/copy source into [UDK2015]/BaseTools/MdeModulePkg/v3x4 (create this subdirectory)

8) Edit file [UDK2015]/BaseTools/MdeModulePkg/MdeModulePkg.dsc and add "MdeModulePkg/v3x4/v3x4.inf" to [Components] section

9) Execute build again. Wait for "- Done -" and take compiled EFI DXE driver from:
[UDK2015]/BaseTools/Build/MdeModule/RELEASE_VS2015x86/X64/MdeModulePkg/v3x4/v3x4/OUTPUT/v3x4.efi
