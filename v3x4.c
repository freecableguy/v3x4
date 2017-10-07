#include <PiPei.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrePiLib.h>
#include <Protocol/MpService.h>

// SVID fixed VCCIN voltages
#define			_default_SVID				0x00000734		// no change to default VCCIN
#define			_pos_1600_mV				0x00000667		// 1.600 V
#define			_pos_1625_mV				0x00000680		// 1.625 V
#define			_pos_1650_mV				0x0000069A		// 1.650 V
#define			_pos_1675_mV				0x000006B4		// 1.675 V
#define			_pos_1700_mV				0x000006CD		// 1.700 V
#define			_pos_1725_mV				0x000006E7		// 1.725 V
#define			_pos_1750_mV				0x00000700		// 1.750 V
#define			_pos_1775_mV				0x0000071A		// 1.775 V
#define			_pos_1800_mV				0x00000734		// 1.800 V (default)
#define			_pos_1825_mV				0x0000074D		// 1.825 V
#define			_pos_1850_mV				0x00000767		// 1.850 V
#define			_pos_1875_mV				0x00000780		// 1.875 V
#define			_pos_1900_mV				0x0000079A		// 1.900 V
#define			_pos_1925_mV				0x000007B4		// 1.925 V
#define			_pos_1950_mV				0x000007CD		// 1.950 V
#define			_pos_1975_mV				0x000007E7		// 1.975 V
#define			_pos_2000_mV				0x00000800		// 2.000 V
#define			_pos_2025_mV				0x0000081A		// 2.025 V
#define			_pos_2050_mV				0x00000834		// 2.050 V
#define			_pos_2075_mV				0x0000084D		// 2.075 V
#define			_pos_2100_mV				0x00000867		// 2.100 V

// Adaptive negative dynamic voltage offsets for all domains
#define			_default_FVID				0x0			// no change to default VID (Vcore)
#define			_neg_10_mV				0xFEC00000		// -10 mV  (-0.010 V)
#define			_neg_20_mV				0xFD800000		// -20 mV  (-0.020 V)
#define			_neg_30_mV				0xFC200000		// -30 mV  (-0.030 V)
#define			_neg_40_mV				0xFAE00000		// -40 mV  (-0.040 V)
#define			_neg_50_mV				0xF9A00000		// -50 mV  (-0.050 V)
#define			_neg_60_mV				0xF8600000		// -60 mV  (-0.060 V)
#define			_neg_70_mV				0xF7000000		// -70 mV  (-0.070 V)
#define			_neg_80_mV				0xF5C00000		// -80 mV  (-0.080 V)
#define			_neg_90_mV				0xF4800000		// -90 mV  (-0.090 V)
#define			_neg_100_mV				0xF3400000		// -100 mV (-0.100 V)
#define			_neg_110_mV				0xF1E00000		// -110 mV (-0.110 V)
#define			_neg_120_mV				0xF0A00000		// -120 mV (-0.120 V)
#define			_neg_130_mV				0xEF600000		// -130 mV (-0.130 V)
#define			_neg_140_mV				0xEE200000		// -140 mV (-0.140 V)
#define			_neg_150_mV				0xECC00000		// -150 mV (-0.150 V)

// Model Specific Registers
#define			MSR_IA32_BIOS_SIGN_ID			0x08B
#define			MSR_PLATFORM_INFO			0x0CE
#define			MSR_OC_MAILBOX				0x150
#define			MSR_FLEX_RATIO				0x194
#define			MSR_TURBO_RATIO_LIMIT			0x1AD
#define			MSR_TURBO_RATIO_LIMIT1 			0x1AE
#define			MSR_TURBO_RATIO_LIMIT2 			0x1AF
#define			MSR_UNCORE_RATIO_LIMIT			0x620

// toolbox for MSR OC Mailbox (experimental)
#define			OC_MB_COMMAND_EXEC			0x8000000000000000
#define			OC_MB_GET_CPU_CAPS			0x0000000100000000
#define			OC_MB_GET_TURBO_RATIOS			0x0000000200000000
#define			OC_MB_GET_CPU_VIDS_RATIOS		0x0000001000000000
#define			OC_MB_SET_CPU_VIDS_RATIOS		0x0000001100000000
#define			OC_MB_GET_SVID_PARAMS			0x0000001200000000
#define			OC_MB_SET_SVID_PARAMS			0x0000001300000000
#define			OC_MB_GET_FIVR_PARAMS			0x0000001400000000
#define			OC_MB_SET_FIVR_PARAMS			0x0000001500000000

#define			OC_MB_DOMAIN_IACORE			0x0000000000000000	// IA Core domain
#define			OC_MB_DOMAIN_GFX			0x0000010000000000
#define			OC_MB_DOMAIN_CLR			0x0000020000000000	// CLR (CBo/LLC/Ring) a.k.a. Cache/Uncore domain
#define			OC_MB_DOMAIN_SA				0x0000030000000000
#define			OC_MB_DOMAIN_AIO			0x0000040000000000
#define			OC_MB_DOMAIN_DIO			0x0000050000000000

#define			OC_MB_DIS_FIVR_FAULTS			0x1			// bit 0
#define			OC_MB_DIS_FIVR_EFF_MAN			0x2			// bit 1
#define			OC_MB_DIS_FIVR_SVID_BUS			0x80000000		// bit 31

#define			OC_MB_SUCCESS				0x0
#define			OC_MB_REBOOT_REQUIRED			0x7

// constants
#define			AP_EXEC_TIMEOUT				1000000			// 1 second
#define			CPUID_VERSION_INFO			0x1
#define			CPUID_BRAND_STRING_BASE			0x80000002
#define			CPUID_BRAND_STRING_LEN			0x30
#define			MSR_FLEX_RATIO_OC_LOCK_BIT		0x100000		// bit 20, set to lock MSR 0x194
#define			MSR_TURBO_RATIO_SEMAPHORE_BIT		0x8000000000000000	// set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF

// build options
#define			BUILD_RELEASE_VER			L"v0.10b-i306f2-rc7"	// build version
#define			BUILD_TARGET_CPU_DESC			L"\"Haswell-E/EP\""	// target CPU description (codename)
#define			BUILD_TARGET_CPUID_SIGN			0x306F2			// target CPUID, set 0xFFFFFFFF to bypass checking
#define			MAX_PACKAGE_COUNT			2			// maximum number of supported packages/sockets, increase as needed

// driver settings
const UINTN		SET_MAX_TURBO_RATIO			= 0;			// 0 for auto-max Core turbo multiplier, not to exceed fused limit, no less than MFM
const UINTN		SET_MAX_UNCORE_RATIO			= 0;			// 0 for auto-max Uncore multiplier, not to exceed fused limit, no less than 12
const BOOLEAN		DIS_UNCORE_PWR_MAN			= FALSE;		// disable Uncore low power state, i.e. force frequency to remain at max
const BOOLEAN		DIS_FIVR_FAULTS				= TRUE;			// disable FIVR Faults
const BOOLEAN		DIS_FIVR_EFF_MAN			= TRUE;			// disable FIVR Efficiency Management
const BOOLEAN		SET_SVID_FIXED_VCCIN			= FALSE;		// disable auto VCCIN and program fixed value, not recommended if can be set in BIOS
const BOOLEAN		DIS_FIVR_SVID_BUS			= FALSE;		// disable FIVR SVID Control ("PowerCut"), forces SET_SVID_FIXED_VCCIN = TRUE
const BOOLEAN		SET_OC_LOCK				= TRUE;			// set Overlocking Lock Bit at completion of programming (recommended)

// Serial Voltage Identification (SVID) fixed voltages per package, adjust as needed
const UINT32 SVID_FIXED_VCCIN[MAX_PACKAGE_COUNT] \
	= { _default_SVID, _default_SVID }; // , _default_SVID, _default_SVID, _default_SVID, _default_SVID, _default_SVID, _default_SVID };

// Domain 0 (IA Core) dynamic voltage offsets per package, adjust as needed
const UINT32 IACORE_ADAPTIVE_OFFSET[MAX_PACKAGE_COUNT] \
	= { _default_FVID, _default_FVID }; // , _default_FVID, _default_FVID, _default_FVID, _default_FVID, _default_FVID, _default_FVID };

// Domain 2 (CLR) dynamic voltage offsets per package, adjust as needed
const UINT32 UNCORE_ADAPTIVE_OFFSET[MAX_PACKAGE_COUNT] \
	= { _default_FVID, _default_FVID }; // , _default_FVID, _default_FVID, _default_FVID, _default_FVID, _default_FVID, _default_FVID };

// object structures
typedef struct _PLATFORM_OBJECT {
	UINTN			Packages;						// number of physical processor packages
	UINTN			LogicalProcessors;					// total number of logical processors
	UINTN			EnabledLogicalProcessors;				// total number of enabled logical processors
	CHAR16			Specification[MAX_PACKAGE_COUNT][98];			// processor brand name
	UINTN			Cores[MAX_PACKAGE_COUNT];				// number cores in package
	UINTN			Threads[MAX_PACKAGE_COUNT];				// number threads in package
	UINTN			APICID[MAX_PACKAGE_COUNT];				// logical processor number for first core/thread in package (i.e. APIC ID)
} PLATFORM_OBJECT, *PPLATFORM_OBJECT;

typedef struct _PACKAGE_OBJECT {
	UINT32			CPUID;							// CPUID
	UINTN			LFMMulti;						// Low Frequency Mode (LFM) multiplier, min non-turbo Core multiplier
	UINTN			HFMMulti;						// High Frequecy Mode (HFM) multiplier, max non-turbo Core multiplier
	UINTN			Turbo1CMulti;						// maximum (1C) Core turbo multiplier
	UINTN			TurboMultiLimit;					// highest allowed turbo mulitplier
	UINTN			MinUncoreMulti;						// minimum Uncore multiplier
	UINTN			MaxUncoreMulti;						// maximum Uncore multiplier
	UINTN			UncoreMultiLimit;					// highest allowed Uncore multiplier
} PACKAGE_OBJECT, *PPACKAGE_OBJECT;

typedef struct _SYSTEM_OBJECT {
	PPLATFORM_OBJECT	Platform;						// Platform object
	PPACKAGE_OBJECT		Package;						// MAX_PACKAGE_COUNT size array of Package objects dynamically built at runtime
	UINTN			BootstrapProcessor;					// bootstrap processor (BSP) assignment at driver entry
	UINTN			NextPackage;						// package number of next package, used in sequencing of programming, etc.
	UINT64			MSRResponseBuffer;					// global buffer for MSR data read
	UINT64			MSRProgramBuffer;					// global buffer for MSR data write
	BOOLEAN			RebootRequired;						// set if reboot is required for some settings to take effect
} SYSTEM_OBJECT, *PSYSTEM_OBJECT;

// global variables
EFI_MP_SERVICES_PROTOCOL	*MpServicesProtocol;					// MP Services Protocol handle
SYSTEM_OBJECT			*System;						// ptr to global System object

// function prototypes
EFI_STATUS	EFIAPI EnumProcessors(IN OUT PPLATFORM_OBJECT *PlatformObject);
EFI_STATUS	EFIAPI GetPackageCaps(IN OUT PPACKAGE_OBJECT *PackageObject);
BOOLEAN		EFIAPI IsMicrocodePatchDone(VOID);
BOOLEAN		EFIAPI IsValidPackage(VOID);
VOID		EFIAPI ProgramPackage(IN OUT VOID *Buffer);
VOID		EFIAPI SetTurboRatios(VOID);
VOID		EFIAPI SetUncoreRatios(VOID);
VOID		EFIAPI SetFVIDParams(VOID);
VOID		EFIAPI SetFIVRParams(VOID);
VOID		EFIAPI SetSVIDParams(VOID);
VOID		EFIAPI SetOverclockingLock(VOID);
VOID		EFIAPI OC_MAILBOX_DISPATCH(VOID);
VOID		EFIAPI OC_MAILBOX_READ(VOID);
BOOLEAN		EFIAPI OC_MAILBOX_CLEANUP(VOID);
BOOLEAN		EFIAPI OC_MAILBOX_ERROR(VOID);

// driver main
EFI_STATUS
EFIAPI
EFIDriverEntry(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
) {
	EFI_STATUS status;
	
	status = SystemTable->ConOut->OutputString(
		SystemTable->ConOut,
		L"Intel(R) Xeon(R) Processor Max Effort Turbo Boost UEFI DXE driver\r\n\0"
		);

	if (EFI_ERROR(status)) { 
		goto DriverExit;
	}

	// driver init
	Print(
		L"Build %s %s (up to %dS)\r\n\0",
		BUILD_RELEASE_VER,
		BUILD_TARGET_CPU_DESC,
		MAX_PACKAGE_COUNT
		);

	// create System object
	System = (PSYSTEM_OBJECT)AllocatePool(
		sizeof(SYSTEM_OBJECT)
		);

	if (!System) {
		goto DriverExit;
	}
	else {
		// zero it
		SetMem(
			System,
			sizeof(SYSTEM_OBJECT),
			0
			);
	}

	// verify no microcode patch loaded
	if (IsMicrocodePatchDone()) {
		goto DriverExit;
	}

	// get handle to UEFI MP Services Protocol
	EFI_GUID efi_mp_service_protocol_guid = EFI_MP_SERVICES_PROTOCOL_GUID;

	status = SystemTable->BootServices->LocateProtocol(
		&efi_mp_service_protocol_guid,
		NULL,
		(VOID**)&MpServicesProtocol
		);

	if (EFI_ERROR(status)) {
		Print(
			L"[FAILURE] Unable to locate EFI MP Services Protocol (%r)\r\n\0",
			status
			);

		goto DriverExit;
	}

	// get default BSP
	status = MpServicesProtocol->WhoAmI(
		MpServicesProtocol,
		&System->BootstrapProcessor
		);

	if (EFI_ERROR(status)) {
		Print(
			L"[FAILURE] Unable to get current bootstrap processor (%r)\r\n\0",
			status
			);

		goto DriverExit;
	}
	
	// enumerate processors
	Print(
		L"Enumerating processors...\r\n\0"
		);

	if (EFI_ERROR(
		EnumProcessors(
			&System->Platform))) {
		goto DriverExit;
	}

	Print(
		L"Gathering processor capabilities...\r\n\0"
		);

	// build processor information structure
	if (EFI_ERROR(
		GetPackageCaps(
			&System->Package))) {
		goto DriverExit;
	}

	for (System->NextPackage ; System->NextPackage < System->Platform->Packages; System->NextPackage++) {
		// program CPU using current AP if same as current BSP
		if (System->BootstrapProcessor == System->Platform->APICID[System->NextPackage]) {
			ProgramPackage(
				NULL
				);			
		}
		else {
			// dispatch AP to program CPU
			status = MpServicesProtocol->StartupThisAP(
				MpServicesProtocol,
				ProgramPackage,
				System->Platform->APICID[System->NextPackage],
				NULL,
				AP_EXEC_TIMEOUT,
				NULL,
				NULL
				);

			if (EFI_ERROR(status)) {
				Print(
					L"[WARNING] Failed to startup programming AP on CPU%d (%r)\r\n\0",
					System->NextPackage,
					status
					);
			}
		}
	}
	
DriverExit:

	if (System->RebootRequired == TRUE) {
		Print(
			L"!!! REBOOT REQUIRED FOR SOME SETTINGS TO TAKE EFFECT !!!\r\n\0"
			);
	}

	// always return success, no cleanup as everything is automatically destroyed
	return EFI_SUCCESS;
}

// creates Platform object and intializes all member variables
EFI_STATUS
EFIAPI
EnumProcessors(
	IN OUT PPLATFORM_OBJECT *PlatformObject
) {
	EFI_STATUS status;
	
	// allocate memory
	PPLATFORM_OBJECT Platform = (PPLATFORM_OBJECT)AllocatePool(
		sizeof(PLATFORM_OBJECT)
		);

	if (!Platform) {
		return EFI_OUT_OF_RESOURCES; 
	}
	else {
		// zero it
		SetMem(
			Platform,
			sizeof(PLATFORM_OBJECT),
			0
			);
	}

	// set initial value
	Platform->Packages = 1;
	
	// get number of logical processors, enabled logical processors for entire system
	status = MpServicesProtocol->GetNumberOfProcessors(
		MpServicesProtocol,
		&Platform->LogicalProcessors,
		&Platform->EnabledLogicalProcessors
		);
	
	if (EFI_ERROR(status)) {
		return status;
	}

	EFI_PROCESSOR_INFORMATION processor_info;
	UINTN thread_counter = 0;
	UINTN htt_enabled = 0;

	for (UINTN thread_index = 0; thread_index < Platform->LogicalProcessors; thread_index++) {
		MpServicesProtocol->GetProcessorInfo(
			MpServicesProtocol,
			thread_index,
			&processor_info
			);

		thread_counter++;

		// detect if HyperThreading enabled
		if ((processor_info.Location.Thread == 1) && (htt_enabled != 1)) {
			htt_enabled = 1;
		}

		// last logical processor
		if (thread_index == (Platform->LogicalProcessors - 1)) {
			Platform->Threads[System->NextPackage] = thread_counter;

			Platform->Cores[System->NextPackage] = Platform->Threads[System->NextPackage] / (htt_enabled + 1);

			Platform->APICID[System->NextPackage] = (thread_index - thread_counter) + 1;

			break;
		}
	
		// package ID changes -> new package found
		if (processor_info.Location.Package != System->NextPackage) {
			Platform->Packages++;
			
			Platform->Threads[System->NextPackage] = thread_counter - 1;

			Platform->Cores[System->NextPackage] = Platform->Threads[System->NextPackage] / (htt_enabled + 1);
			
			Platform->APICID[System->NextPackage] = (thread_index - thread_counter) + 1;
			
			System->NextPackage++;

			thread_counter = 1;
		}
	}

	System->NextPackage = 0;
	
	CHAR8 processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1];
	UINT32 cpuid_string[4];
	UINTN k;

	// build processor brand name string	
	for (System->NextPackage; System->NextPackage < Platform->Packages; System->NextPackage++) {

		k = 0;

		for (UINTN i = 0; i < 3; i++) {
			AsmCpuid(
				(UINT32)(CPUID_BRAND_STRING_BASE + i),
				&cpuid_string[0],
				&cpuid_string[1],
				&cpuid_string[2],
				&cpuid_string[3]
				);

			for (UINTN j = 0; j < 4; j++) {
				CopyMem(
					processor_brand_string_buffer + k,
					&cpuid_string[j],
					4
					);

				k += 4;
			}
		}

		processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1] = '\0';

		// convert ASCII to Unicode
		AsciiStrToUnicodeStrS(
			processor_brand_string_buffer,
			Platform->Specification[System->NextPackage],
			sizeof(Platform->Specification)
			);

		Print(
			L"CPU%d: %s (%dC/%dT)\r\n\0",
			System->NextPackage,
			Platform->Specification[System->NextPackage],
			Platform->Cores[System->NextPackage],
			Platform->Threads[System->NextPackage]
			);
	}

	System->NextPackage = 0;
	
	*PlatformObject = Platform;

	return EFI_SUCCESS;
}

// creates Package objects and intializes all member variables
EFI_STATUS
EFIAPI
GetPackageCaps(
	IN OUT PPACKAGE_OBJECT *PackageObject
) {
	// allocate memory
	PPACKAGE_OBJECT Package = (PPACKAGE_OBJECT)AllocatePool(
		System->Platform->Packages * sizeof(PACKAGE_OBJECT)
		);
	
	if (!Package) {
		return EFI_OUT_OF_RESOURCES;
	}
	else {
		// zero it
		SetMem(
			Package, 
			sizeof(PACKAGE_OBJECT),
			0
			);
	}
		
	// initialize processor data
	for (System->NextPackage; System->NextPackage < System->Platform->Packages; System->NextPackage++) {			
		// get processor CPUID
		AsmCpuid(
			CPUID_VERSION_INFO,
			&Package[System->NextPackage].CPUID,
			NULL,
			NULL,
			NULL
			);

		// get non-turbo multipliers
		System->MSRResponseBuffer = AsmReadMsr64(
			MSR_PLATFORM_INFO
			);

		Package[System->NextPackage].LFMMulti = (System->MSRResponseBuffer >> 40) & 0xFF;
		
		Package[System->NextPackage].HFMMulti = (System->MSRResponseBuffer >> 8) & 0xFF;

		// get maximum (1C) Core turbo multiplier
		System->MSRProgramBuffer = OC_MB_GET_CPU_CAPS \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC;
		
		OC_MAILBOX_DISPATCH();

		if (OC_MAILBOX_ERROR()) {
			Print(
				L"[FAILURE] Error getting maximum (1C) Core turbo ratio\r\n\0"
				);
			
			return EFI_ABORTED;
		}

		Package[System->NextPackage].Turbo1CMulti = System->MSRResponseBuffer & 0xFF;

		// set turbo multi limit
		if (SET_MAX_TURBO_RATIO > Package[System->NextPackage].Turbo1CMulti) {
			Package[System->NextPackage].TurboMultiLimit = Package[System->NextPackage].Turbo1CMulti;
		}

		if ((SET_MAX_TURBO_RATIO < Package[System->NextPackage].LFMMulti) \
			&& (SET_MAX_TURBO_RATIO != 0)) {
			Package[System->NextPackage].TurboMultiLimit = Package[System->NextPackage].LFMMulti;
		}

		if (SET_MAX_TURBO_RATIO == 0) {
			Package[System->NextPackage].TurboMultiLimit = Package[System->NextPackage].Turbo1CMulti;
		}
		else {
			Package[System->NextPackage].TurboMultiLimit = SET_MAX_TURBO_RATIO;
		}

		// get minimum Uncore multiplier
		System->MSRResponseBuffer = AsmReadMsr64(
			MSR_UNCORE_RATIO_LIMIT
			);

		Package[System->NextPackage].MinUncoreMulti = (System->MSRResponseBuffer >> 8) & 0xFF;
		
		// get maximum Uncore multiplier
		System->MSRProgramBuffer = OC_MB_GET_CPU_CAPS \
			| OC_MB_DOMAIN_CLR \
			| OC_MB_COMMAND_EXEC;
		
		OC_MAILBOX_DISPATCH();

		if (OC_MAILBOX_ERROR()) {
			Print(
				L"[FAILURE] Error getting maximum Uncore ratio\r\n\0"
				);
			
			return EFI_ABORTED;
		}

		Package[System->NextPackage].MaxUncoreMulti = System->MSRResponseBuffer & 0xFF;
		
		// set Uncore multi limit
		if (SET_MAX_UNCORE_RATIO > Package[System->NextPackage].MaxUncoreMulti) {
			Package[System->NextPackage].UncoreMultiLimit = Package[System->NextPackage].MaxUncoreMulti;
		}
	
		if ((SET_MAX_UNCORE_RATIO < Package[System->NextPackage].MinUncoreMulti) \
			&& (SET_MAX_UNCORE_RATIO != 0)) {
			Package[System->NextPackage].UncoreMultiLimit = Package[System->NextPackage].MinUncoreMulti;
		}
	
		if (SET_MAX_UNCORE_RATIO == 0) {
			Package[System->NextPackage].UncoreMultiLimit = Package[System->NextPackage].MaxUncoreMulti;
		}
		else {
			Package[System->NextPackage].UncoreMultiLimit = SET_MAX_UNCORE_RATIO;
		}
	}

	System->NextPackage = 0;

	*PackageObject = Package;
	
	return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
IsMicrocodePatchDone(
	VOID
) {
	Print(
		L"Verifying no microcode update patch loaded...\r\n\0"
		);

	AsmWriteMsr64(
		MSR_IA32_BIOS_SIGN_ID,
		0
		);

	AsmCpuid(
		CPUID_VERSION_INFO,
		NULL,
		NULL,
		NULL,
		NULL
		);

	System->MSRResponseBuffer = AsmReadMsr64(
		MSR_IA32_BIOS_SIGN_ID
		);

	// high word contains microcode update revision patch level
	if (((System->MSRResponseBuffer >> 32) & 0xFF) != 0) {
		Print(
			L"[FAILURE] Processor microcode update revision detected\r\n\0"
			);

		return TRUE;
	}

	return FALSE;
}

BOOLEAN
EFIAPI
IsValidPackage(
	VOID
) {
	Print(
		L"Validating CPU%d for programming... \r\n\0",
		System->NextPackage
		);

	// verify package CPUID matchs build target CPUID
	if ((System->Package[System->NextPackage].CPUID != BUILD_TARGET_CPUID_SIGN) \
		&& (BUILD_TARGET_CPUID_SIGN != 0xFFFFFFFF)) {
		Print(
			L"[FAILURE] CPUID (0x%x) does not match target CPUID: 0x%x\r\n\0",
			System->Package[System->NextPackage].CPUID,
			BUILD_TARGET_CPUID_SIGN
			);

		return FALSE;
	}

	// check OC Lock Bit not set
	System->MSRResponseBuffer = AsmReadMsr64(
		MSR_FLEX_RATIO
		);

	if ((System->MSRResponseBuffer & MSR_FLEX_RATIO_OC_LOCK_BIT) == MSR_FLEX_RATIO_OC_LOCK_BIT) {
		Print(
			L"[FAILURE] Overclock enable lock bit (MSR 0x194[20]) set\r\n\0"
			);

		return FALSE;
	}

	return TRUE;
}

// programs processor package; MUST be executed in context of package to be programmed
VOID
EFIAPI
ProgramPackage(
	IN OUT VOID *Buffer
) {	
	// validate package for programming
	if (IsValidPackage()) {
		
		// Turbo ratios
		SetTurboRatios();

		// Uncore ratios
		SetUncoreRatios();

		// FVID
		SetFVIDParams();

		// FIVR
		SetFIVRParams();

		// SVIR
		SetSVIDParams();

		// OC Lock Bit
		SetOverclockingLock();
	}

	return;
}

VOID
EFIAPI
SetTurboRatios(
	VOID
) {
	System->MSRProgramBuffer = System->Package[System->NextPackage].TurboMultiLimit \
		| (System->Package[System->NextPackage].TurboMultiLimit << 8)  \
		| (System->Package[System->NextPackage].TurboMultiLimit << 16) \
		| (System->Package[System->NextPackage].TurboMultiLimit << 24) \
		| (System->Package[System->NextPackage].TurboMultiLimit << 32) \
		| (System->Package[System->NextPackage].TurboMultiLimit << 40) \
		| (System->Package[System->NextPackage].TurboMultiLimit << 48) \
		| (System->Package[System->NextPackage].TurboMultiLimit << 56);

	Print(
		L"Programming new max allowable Core turbo ratio (%dx)...\r\n\0",
		System->Package[System->NextPackage].TurboMultiLimit
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT,
		System->MSRProgramBuffer
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT1,
		System->MSRProgramBuffer
		);

	System->MSRProgramBuffer |= MSR_TURBO_RATIO_SEMAPHORE_BIT;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT2,
		System->MSRProgramBuffer
		);

	return;
}

VOID
EFIAPI
SetUncoreRatios(
	VOID
) {
	System->MSRResponseBuffer = AsmReadMsr64(
		MSR_UNCORE_RATIO_LIMIT
		);

	System->MSRProgramBuffer = (System->MSRResponseBuffer & 0xFFFFFFFFFFFFFF00) \
		| System->Package[System->NextPackage].UncoreMultiLimit;

	if (DIS_UNCORE_PWR_MAN == TRUE) {
		System->MSRProgramBuffer &= 0xFFFFFFFFFFFF00FF;

		System->MSRProgramBuffer |= (System->Package[System->NextPackage].UncoreMultiLimit << 8);

		Print(
			L"Programming max fixed Uncore ratio (%dx)...\r\n\0",
			System->Package[System->NextPackage].UncoreMultiLimit
			);
	}
	else {
		Print(
			L"Programming max allowable Uncore ratio (%dx)...\r\n\0",
			System->Package[System->NextPackage].UncoreMultiLimit
			);
	}

	AsmWriteMsr64(
		MSR_UNCORE_RATIO_LIMIT,
		System->MSRProgramBuffer
		);

	return;
}

VOID
EFIAPI
SetFVIDParams(
	VOID
) {
	// set IA Core dynamic voltage offset
	Print(
		L"Programming Core FVID dynamic offset voltage... \0"
		);

	System->MSRProgramBuffer = OC_MB_SET_CPU_VIDS_RATIOS \
		| OC_MB_DOMAIN_IACORE \
		| OC_MB_COMMAND_EXEC \
		| IACORE_ADAPTIVE_OFFSET[System->NextPackage] \
		| System->Package[System->NextPackage].Turbo1CMulti;

	OC_MAILBOX_DISPATCH();

	OC_MAILBOX_CLEANUP();

	// set CLR dynamic voltage offset
	Print(
		L"Programming Uncore FVID dynamic offset voltage... \0"
		);

	System->MSRProgramBuffer = OC_MB_SET_CPU_VIDS_RATIOS \
		| OC_MB_DOMAIN_CLR \
		| OC_MB_COMMAND_EXEC \
		| UNCORE_ADAPTIVE_OFFSET[System->NextPackage] \
		| System->Package[System->NextPackage].MaxUncoreMulti;

	OC_MAILBOX_DISPATCH();

	OC_MAILBOX_CLEANUP();

	return;
}

VOID
EFIAPI
SetFIVRParams(
	VOID
) {
	// FIRV Faults
	if (DIS_FIVR_FAULTS == TRUE) {
		Print(
			L"Disabling FIVR Faults... \0"
			);
		
		System->MSRProgramBuffer = OC_MB_SET_FIVR_PARAMS \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC \
			| OC_MB_DIS_FIVR_FAULTS;
		
		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}
			
	// FIVR Efficiency Mananagement
	if (DIS_FIVR_EFF_MAN == TRUE) {
		Print(
			L"Disabling FIVR Efficiency Management... \0"
			);

		System->MSRProgramBuffer = OC_MB_SET_FIVR_PARAMS \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC \
			| OC_MB_DIS_FIVR_EFF_MAN;

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}
	
	return;
}

VOID
EFIAPI
SetSVIDParams(
	VOID
) {
	System->MSRProgramBuffer = OC_MB_GET_SVID_PARAMS \
		| OC_MB_DOMAIN_IACORE \
		| OC_MB_COMMAND_EXEC;

	OC_MAILBOX_DISPATCH();

	// check if VCCIN already set
	if ((SET_SVID_FIXED_VCCIN == TRUE) \
		&& ((System->MSRResponseBuffer & 0xFFF) == SVID_FIXED_VCCIN[System->NextPackage])) {
		Print(
			L"Fixed VCCIN already set, skipping...\r\n\0",
			System->NextPackage
			);
	}
	else {
		if (SET_SVID_FIXED_VCCIN == TRUE) {
			// set fixed VCCIN, PowerCut
			System->MSRProgramBuffer = OC_MB_SET_SVID_PARAMS \
				| OC_MB_DOMAIN_IACORE \
				| OC_MB_COMMAND_EXEC \
				| SVID_FIXED_VCCIN[System->NextPackage];

			if (DIS_FIVR_SVID_BUS == TRUE) {
				System->MSRProgramBuffer |= OC_MB_DIS_FIVR_SVID_BUS;  // PowerCut

				Print(
					L"Disabling FIVR SVID data bus link, programming fixed VCCIN... \0"
					);
			}
			else {
				Print(
					L"Programming fixed VCCIN... \0"
					);
			}

			if (SVID_FIXED_VCCIN[System->NextPackage] == 0) {
				System->MSRProgramBuffer |= _default_SVID; // fail safe in case of compile with no data set

				Print(
					L"[WARNING] No valid VCCIN setting found, using default\r\n\0",
					System->NextPackage
				);
			}

			OC_MAILBOX_DISPATCH();

			OC_MAILBOX_CLEANUP();
		}
	}

	return;
}

VOID
EFIAPI
SetOverclockingLock(
	VOID
) {
	if (SET_OC_LOCK == TRUE) {
		Print(
			L"Finalizing programming...\r\n\0"
		);
		
		System->MSRResponseBuffer = AsmReadMsr64(
			MSR_FLEX_RATIO
			);

		// prevents any changes to write-once MSR values once set (reboot required to clear)
		System->MSRProgramBuffer = System->MSRResponseBuffer | MSR_FLEX_RATIO_OC_LOCK_BIT;

		AsmWriteMsr64(
			MSR_FLEX_RATIO,
			System->MSRProgramBuffer
			);
	}

	return;
}

VOID
EFIAPI
OC_MAILBOX_READ(
	VOID
) {
	System->MSRResponseBuffer = AsmReadMsr64(
		MSR_OC_MAILBOX
		);
	
	return;
}

VOID
EFIAPI
OC_MAILBOX_DISPATCH(
	VOID
) {
	// write to mailbox
	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		System->MSRProgramBuffer
		);

	// retrieve immediate response from mailbox
	System->MSRResponseBuffer = AsmReadMsr64(
		MSR_OC_MAILBOX
		);

	return;
}

BOOLEAN
EFIAPI
OC_MAILBOX_CLEANUP(
	VOID
) {
	// special case where response indicates reboot is required for setting to take effect
	if (((System->MSRResponseBuffer >> 32) & 0xFF) == OC_MB_REBOOT_REQUIRED) {
		// set flag for reboot required
		System->RebootRequired = TRUE;
	}
	else {
		// if non-zero there was an error
		if (OC_MAILBOX_ERROR()) {
			Print(
				L"failed!\r\n\0"
			);

			return TRUE;
		}
	}

	// next line
	Print(
		L"\r\n\0"
		);

	return FALSE;
}

BOOLEAN
EFIAPI
OC_MAILBOX_ERROR(
	VOID
) {
	if (((System->MSRResponseBuffer >> 32) & 0xFF) != OC_MB_SUCCESS) {
		return TRUE;
	}

	return FALSE;
}
