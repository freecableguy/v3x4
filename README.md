# v3x4
Intel(R) Xeon(R) Processor v3 (Haswell-E/EP) Full Turbo Boost DXE driver

Description: Unlocks Haswell-E/EP processors (cpuid = 306F2h) on X99 (single) and C612 (dual) platforms to allow for maximum all-core turbo boost for all cores. For example, the 18-core Xeon(R) E5-2696 v3 processor has set from the factory an all-core turbo of 2.8GHz. This driver programs the higest un-fused ratio (i.e. the 1C Turbo bin) as the new Turbo bin for all boost configurations including all-core turbo. In other words, the 1C Turbo bin becomes the all-core Turbo bin and the E5-2696 v3 processor now demonstrates an all-core turbo of 3.8GHz. Allows for dynamically undervolting (retains PCU control while applying a fixed negative Vcore offset) Core, CLR (CBo/LLC/Ring), and System Agent voltage domains independently per package which provides for higher all-core sustained clocks during heavy workloads including AVX2 workloads. Allows for locking Uncore ratio to maximum performance (typical 30x). May work for other Intel Xeon steppings.

Use requirements:

-- Haswell-E/EP processor (cpuid = 306F2h). This can be overriden with build flag during compile

-- CPU microcode revision patch must not be loaded (requires modified BIOS)
  - instructions on how to modify BIOS to remove microcode will not be given here

-- Use EFI Shellx64 to load v3x4.efi during system POST
  - bcfg driver add 0 fs1:\EFI\Boot\v3x4.EFI "V3 Full Turbo" where 'fs1:\EFI\Boot\v3x4.efi' is path to DXE driver file on UEFI boot partition (use 'mountvol x: /s' to mount in Windows as X: for writting)
  - toggle enable/disable in BIOS (if presented as option)

To compile for point-releases, in Windows you will need:

1) UDK2015: http://www.tianocore.org/udk/udk2015/

2) Any C-compiler that is supported by UDK2015. Currently verified working with Visual Studio 2015.

3) In file [UDK2015]\BaseTools\Conf\target.txt change next parameters to:

ACTIVE_PLATFORM = MdeModulePkg/MdeModulePkg.dsc

TARGET = RELEASE

TARGET_ARCH = X64

TOOL_CHAIN_TAG = VS2015x86 (or VS2013x86, etc.)

4) open command prompt (admin0, go to folder [UDK2015]\BaseTools

5) execute edksetup.bat

6) execute build

If it prints - Done - all is fine and UDK2015 is functioning properly...
 
7) unpack source into [UDK2015]\BaseTools\MdeModulePkg\v3x4

8) in file [UDK2015]\BaseTools\MdeModulePkg\MdeModulePkg.dsc, in section [Components], add string MdeModulePkg/v3x4/v3x4.inf

9) execute build again

10) wait for - Done - and take compiled EFI DVX driver from
[UDK2015]\BaseTools\Build\MdeModule\RELEASE_VS2013x86\X64\MdeModulePkg\v3x4\v3x4\OUTPUT\v3x4.efi
