#include <PiPei.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrePiLib.h>
#include <Protocol/MpService.h>

// SVID fixed VCCIN voltages
#define		_DYNAMIC_SVID				0x0				// FIVR-controlled SVID
#define		_DEFAULT_SVID				0x0000074D			// no change to default VCCIN
#define		_VCCIN_1pt600				0x00000667			// 1.600 V
#define		_VCCIN_1pt625				0x00000680			// 1.625 V
#define		_VCCIN_1pt650				0x0000069A			// 1.650 V
#define		_VCCIN_1pt675				0x000006B4			// 1.675 V
#define		_VCCIN_1pt700				0x000006CD			// 1.700 V
#define		_VCCIN_1pt725				0x000006E7			// 1.725 V
#define		_VCCIN_1pt750				0x00000700			// 1.750 V
#define		_VCCIN_1pt775				0x0000071A			// 1.775 V
#define		_VCCIN_1pt800				0x00000734			// 1.800 V
#define		_VCCIN_1pt825				0x0000074D			// 1.825 V (default)
#define		_VCCIN_1pt850				0x00000767			// 1.850 V
#define		_VCCIN_1pt875				0x00000780			// 1.875 V
#define		_VCCIN_1pt900				0x0000079A			// 1.900 V
#define		_VCCIN_1pt925				0x000007B4			// 1.925 V
#define		_VCCIN_1pt950				0x000007CD			// 1.950 V
#define		_VCCIN_1pt975				0x000007E7			// 1.975 V
#define		_VCCIN_2pt000				0x00000800			// 2.000 V
#define		_VCCIN_2pt025				0x0000081A			// 2.025 V
#define		_VCCIN_2pt050				0x00000834			// 2.050 V
#define		_VCCIN_2pt075				0x0000084D			// 2.075 V
#define		_VCCIN_2pt100				0x00000867			// 2.100 V

// Adaptive negative dynamic voltage offsets for all domains
#define		_DEFAULT_FVID				0x0				// no change to default VID (Vcore)
#define		_FVID_MINUS_10_MV			0xFEC00000			// -10 mV  (-0.010 V)
#define		_FVID_MINUS_20_MV			0xFD800000			// -20 mV  (-0.020 V)
#define		_FVID_MINUS_30_MV			0xFC200000			// -30 mV  (-0.030 V)
#define		_FVID_MINUS_40_MV			0xFAE00000			// -40 mV  (-0.040 V)
#define		_FVID_MINUS_50_MV			0xF9A00000			// -50 mV  (-0.050 V)
#define		_FVID_MINUS_60_MV			0xF8600000			// -60 mV  (-0.060 V)
#define		_FVID_MINUS_65_MV			0xF7A00000			// -65 mV  (-0.065 V)
#define		_FVID_MINUS_70_MV			0xF7000000			// -70 mV  (-0.070 V)
#define		_FVID_MINUS_80_MV			0xF5C00000			// -80 mV  (-0.080 V)
#define		_FVID_MINUS_90_MV			0xF4800000			// -90 mV  (-0.090 V)
#define		_FVID_MINUS_100_MV			0xF3400000			// -100 mV (-0.100 V)
#define		_FVID_MINUS_110_MV			0xF1E00000			// -110 mV (-0.110 V)
#define		_FVID_MINUS_120_MV			0xF0A00000			// -120 mV (-0.120 V)
#define		_FVID_MINUS_130_MV			0xEF600000			// -130 mV (-0.130 V)
#define		_FVID_MINUS_140_MV			0xEE200000			// -140 mV (-0.140 V)
#define		_FVID_MINUS_150_MV			0xECC00000			// -150 mV (-0.150 V)

// toolbox for MSR OC Mailbox (experimental)
#define		OC_MB_COMMAND_EXEC			0x8000000000000000
#define		OC_MB_GET_CPU_CAPS			0x0000000100000000
#define		OC_MB_GET_TURBO_RATIOS			0x0000000200000000
#define		OC_MB_GET_FVIDS_RATIOS			0x0000001000000000
#define		OC_MB_SET_FVIDS_RATIOS			0x0000001100000000
#define		OC_MB_GET_SVID_PARAMS			0x0000001200000000
#define		OC_MB_SET_SVID_PARAMS			0x0000001300000000
#define		OC_MB_GET_FIVR_PARAMS			0x0000001400000000
#define		OC_MB_SET_FIVR_PARAMS			0x0000001500000000

#define		OC_MB_DOMAIN_IACORE			0x0000000000000000		// IA Core domain
#define		OC_MB_DOMAIN_GFX			0x0000010000000000
#define		OC_MB_DOMAIN_CLR			0x0000020000000000		// CLR (CBo/LLC/Ring) a.k.a. Cache/Uncore domain
#define		OC_MB_DOMAIN_SA				0x0000030000000000		// System Agent (SA) domain
#define		OC_MB_DOMAIN_AIO			0x0000040000000000
#define		OC_MB_DOMAIN_DIO			0x0000050000000000

#define		OC_MB_FIVR_FAULTS_OVRD_EN		0x1				// bit 0
#define		OC_MB_FIVR_EFF_MODE_OVRD_EN		0x2				// bit 1
#define		OC_MB_FIVR_DYN_SVID_CONTROL_DIS		0x80000000			// bit 31

#define		OC_MB_SUCCESS				0x0
#define		OC_MB_REBOOT_REQUIRED			0x7

// Model Specific Registers
#define		MSR_IA32_BIOS_SIGN_ID			0x08B
#define		MSR_PLATFORM_INFO			0x0CE
#define		MSR_OC_MAILBOX				0x150
#define		MSR_FLEX_RATIO				0x194
#define		MSR_TURBO_RATIO_LIMIT			0x1AD
#define		MSR_TURBO_RATIO_LIMIT1 			0x1AE
#define		MSR_TURBO_RATIO_LIMIT2 			0x1AF
#define		MSR_UNCORE_RATIO_LIMIT			0x620

// constants
#define		AP_EXEC_TIMEOUT				1000000				// 1 second
#define		CPUID_VERSION_INFO			0x1
#define		CPUID_BRAND_STRING_BASE			0x80000002
#define		CPUID_BRAND_STRING_LEN			0x30
#define		MSR_FLEX_RATIO_OC_LOCK_BIT		0x100000			// bit 20, set to lock MSR 0x194
#define		MSR_TURBO_RATIO_SEMAPHORE_BIT		0x8000000000000000		// set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF
#define		MSR_BIOS_NO_UCODE_PATCH			0x0

// build options
#define		BUILD_RELEASE_VER			L"v3x4-0.10b-i306f2-rc9"	// build version
#define		BUILD_TARGET_CPU_DESC			L"\"Haswell-E/EP\""		// target CPU description (codename)
#define		BUILD_TARGET_CPUID_SIGN			0x306F2				// target CPUID, set 0xFFFFFFFF to bypass checking
#define		MAX_PACKAGE_COUNT			2				// maximum number of supported packages/sockets, increase as needed

// driver settings
const UINTN	CPU_SET_MAX_TURBO_RATIO		=	0;				// 0 for auto max: Core turbo ratio, not to exceed fused limit, no less than MFM
const UINTN	CPU_SET_MAX_UNCORE_RATIO	=	0;				// 0 for auto max: Uncore ratio, not to exceed fused limit, no less than 12

const BOOLEAN	CPU_INPUT_VOLT_MODE_FIXED	=	FALSE;				// set fixed VCCIN (reboot required)
const BOOLEAN	UNCORE_PERF_PLIMIT_OVRD_EN	=	FALSE;				// disable Uncore P-states (i.e. force max frequency)
const BOOLEAN	FIVR_FAULTS_OVRD_EN		=	TRUE;				// disable FIVR Faults
const BOOLEAN	FIVR_EFF_MODE_OVRD_EN		=	TRUE;				// disable FIVR Efficiency Mode
const BOOLEAN	FIVR_DYN_SVID_CONTROL_DIS	=	FALSE;				// disable FIVR SVID Control ("PowerCut"), forces CPU_INPUT_VOLT_MODE_FIXED = TRUE
const BOOLEAN	CPU_SET_OC_LOCK			=	FALSE;				// set Overlocking Lock at completion of programming

// Serial Voltage Identification (SVID) fixed voltages per package, adjust as needed
const UINT32 SVID_FIXED_VCCIN[MAX_PACKAGE_COUNT] \
	= { _DEFAULT_SVID, _DEFAULT_SVID }; // , _DEFAULT_SVID, _DEFAULT_SVID };

// Domain 0 (IA Core) dynamic voltage offsets per package, adjust as needed
const UINT32 IACORE_ADAPTIVE_OFFSET[MAX_PACKAGE_COUNT] \
	= { _DEFAULT_FVID, _DEFAULT_FVID }; // , _DEFAULT_FVID, _DEFAULT_FVID };

// Domain 2 (CLR) dynamic voltage offsets per package, adjust as needed
const UINT32 CLR_ADAPTIVE_OFFSET[MAX_PACKAGE_COUNT] \
	= { _DEFAULT_FVID, _DEFAULT_FVID }; // , _DEFAULT_FVID, _DEFAULT_FVID };

// Domain 3 (SA) dynamic voltage offsets per package, adjust as needed
const UINT32 SA_ADAPTIVE_OFFSET[MAX_PACKAGE_COUNT] \
	= { _DEFAULT_FVID, _DEFAULT_FVID }; // , _DEFAULT_FVID, _DEFAULT_FVID };

// object structures
typedef struct _PLATFORM_OBJECT {
UINTN			Packages;							// number of physical processor packages
UINTN			LogicalProcessors;						// total number of logical processors
UINTN			EnabledLogicalProcessors;					// total number of enabled logical processors
} PLATFORM_OBJECT, *PPLATFORM_OBJECT;

typedef struct _PACKAGE_OBJECT {
UINT32			CPUID;								// CPUID
CHAR16			Specification[128];						// processor specification (brand name)
UINTN			APICID;								// APIC ID
UINTN			Cores;								// number of cores in package
UINTN			Threads;							// number of threads in package
UINTN			CoreMinRatio;							// Low Frequency Mode (LFM) ratio
UINTN			CoreFlexRatio;							// max non-turbo ratio (MNTR) aka Flex ratio, High Frequency Mode (HFM) ratio
UINTN			CoreMaxRatio;							// fused maximum Core ratio aka 1C Turbo ratio
UINTN			CoreLimitRatio;							// max allowable Core ratio
UINTN			UncoreMinRatio;							// min Uncore ratio
UINTN			UncoreMaxRatio;							// fused max Uncore ratio
UINTN			UncoreLimitRatio;						// max allowable Uncore ratio
} PACKAGE_OBJECT, *PPACKAGE_OBJECT;

typedef struct _SYSTEM_OBJECT {
PPLATFORM_OBJECT	Platform;							// Platform object
PACKAGE_OBJECT		Package[MAX_PACKAGE_COUNT];					// array of Package objects
UINTN			ThisPackage;							// package index of current package, used in sequencing of programming, etc.
UINTN			BootstrapProcessor;						// bootstrap processor (BSP) assignment at driver entry
UINT64			ResponseBuffer;							// global buffer for MSR data reading
UINT64			ProgramBuffer;							// global buffer for MSR data writing
BOOLEAN			RebootRequired;							// flag set when reboot required
} SYSTEM_OBJECT, *PSYSTEM_OBJECT;

// global variables
EFI_MP_SERVICES_PROTOCOL	*MpServicesProtocol;					// MP Services Protocol handle
PSYSTEM_OBJECT			System;							// global System object

// function prototypes
EFI_STATUS	EFIAPI GatherPlatformInfo(IN OUT PPLATFORM_OBJECT *PlatformObject);
EFI_STATUS  EFIAPI InitializeSystem(IN EFI_SYSTEM_TABLE *SystemTable);
BOOLEAN		EFIAPI IsMicrocodePatchDone(VOID);
BOOLEAN		EFIAPI IsValidPackage(VOID);
EFI_STATUS	EFIAPI EnumeratePackages(VOID);
VOID		EFIAPI ProgramPackage(IN OUT VOID *Buffer);
VOID		EFIAPI FinalizeProgramming(VOID);
VOID		EFIAPI SetCPUConfiguration(VOID);
VOID		EFIAPI SetFIVRConfiguration(VOID);

VOID		EFIAPI OC_MAILBOX_DISPATCH(VOID);
VOID		EFIAPI OC_MAILBOX_REQUEST(VOID);
BOOLEAN		EFIAPI OC_MAILBOX_CLEANUP(VOID);

// driver main
EFI_STATUS
EFIAPI
EFIDriverEntry(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
) {
	EFI_STATUS status;
	
	// driver init
	status = SystemTable->ConOut->OutputString(
		SystemTable->ConOut,
		L"Intel(R) Xeon(R) Processor Max Effort Turbo Boost UEFI DXE driver\r\n\0"
		);

	if (EFI_ERROR(status)) { 
		goto DriverExit;
	}
	else {
		Print(
			L"Build %s %s (up to max %dS)\r\n\0",
			BUILD_RELEASE_VER,
			BUILD_TARGET_CPU_DESC,
			MAX_PACKAGE_COUNT
			);
	}
	
	// verify no microcode patch loaded
	if (IsMicrocodePatchDone()) {
		goto DriverExit;
	}

	// initialize system data
	if (EFI_ERROR(InitializeSystem(SystemTable))) {
		goto DriverExit;
	}

	// initialize platform data
	if (EFI_ERROR(GatherPlatformInfo(&System->Platform))) {
		goto DriverExit;
	}
	
	// enumerate processors and gather processor data
	if (EFI_ERROR(EnumeratePackages())) {
		goto DriverExit;
	}

	// program packages
	for (System->ThisPackage ; System->ThisPackage < System->Platform->Packages; System->ThisPackage++) {
		// program CPU using current AP if same as current BSP
		if (System->BootstrapProcessor == System->Package[System->ThisPackage].APICID) {
			ProgramPackage(
				NULL
				);			
		}
		else {
			// dispatch AP to program CPU
			status = MpServicesProtocol->StartupThisAP(
				MpServicesProtocol,
				ProgramPackage,
				System->Package[System->ThisPackage].APICID,
				NULL,
				AP_EXEC_TIMEOUT,
				NULL,
				NULL
				);

			if (EFI_ERROR(status)) {
				Print(
					L"[WARNING] Failed to startup programming AP on CPU%d (%r)\r\n\0",
					System->ThisPackage,
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

EFI_STATUS
EFIAPI
InitializeSystem(
	IN EFI_SYSTEM_TABLE *SystemTable
) {
	EFI_GUID efi_mp_service_protocol_guid = EFI_MP_SERVICES_PROTOCOL_GUID;
	EFI_STATUS status;
	
	// create System object
	System = (PSYSTEM_OBJECT)AllocatePool(
		sizeof(SYSTEM_OBJECT)
		);

	if (!System) {
		return EFI_OUT_OF_RESOURCES;
	}
	else {
		// zero it
		SetMem(
			System,
			sizeof(SYSTEM_OBJECT),
			0
			);
	}

	// get handle to MP (Multiprocessor) Services Protocol
	status = SystemTable->BootServices->LocateProtocol(
		&efi_mp_service_protocol_guid,
		NULL,
		(VOID**)&MpServicesProtocol
		);

	if (EFI_ERROR(status)) {
		Print(
			L"[FAILURE] Unable to locate MP Services Protocol (%r)\r\n\0",
			status
			);

		return status;
	}

	// get default BSP (bootstrap processor)
	status = MpServicesProtocol->WhoAmI(
		MpServicesProtocol,
		&System->BootstrapProcessor
		);

	if (EFI_ERROR(status)) {
		Print(
			L"[FAILURE] Unable to get current bootstrap processor (%r)\r\n\0",
			status
			);

		return status;
	}

	return EFI_SUCCESS;
}


// creates Platform object and intializes all member variables
EFI_STATUS
EFIAPI
GatherPlatformInfo(
	IN OUT PPLATFORM_OBJECT *PlatformObject
) {
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

	// get number of logical processors, enabled logical processors
	EFI_STATUS status = MpServicesProtocol->GetNumberOfProcessors(
		MpServicesProtocol,
		&Platform->LogicalProcessors,
		&Platform->EnabledLogicalProcessors
		);

	if (EFI_ERROR(status)) {
		return status;
	}
	
	// get number of packages
	EFI_PROCESSOR_INFORMATION processor_info;
	for (UINTN thread_index = 0; thread_index < Platform->LogicalProcessors; thread_index++) {
		MpServicesProtocol->GetProcessorInfo(
			MpServicesProtocol,
			thread_index,
			&processor_info
			);

		if (processor_info.Location.Package != Platform->Packages) {
			Platform->Packages++;
		}
	}

	Platform->Packages += 1;
	
	*PlatformObject = Platform;

	return EFI_SUCCESS;
}

// creates Packages object(s) and initializes all member variables
EFI_STATUS
EFIAPI
EnumeratePackages(
	VOID
) {
	Print(
		L"Enumerating processors...\r\n\0"
		);

	// count number of Threads, detect HyperThreading, calculate Core count and APIC ID
	EFI_PROCESSOR_INFORMATION processor_info;
	CHAR8 processor_brand_string_buffer[CPUID_BRAND_STRING_LEN + 1];
	UINT32 cpuid_string[4] = { 0, 0, 0, 0 };
	UINTN thread_counter = 0;
	UINTN htt_enabled = 0;
	UINTN k;
	
	for (UINTN thread_index = 0; thread_index < System->Platform->LogicalProcessors; thread_index++) {
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
		if (thread_index == (System->Platform->LogicalProcessors - 1)) {
			System->Package[System->ThisPackage].Threads = thread_counter;

			System->Package[System->ThisPackage].Cores = System->Package[System->ThisPackage].Threads / (htt_enabled + 1);

			System->Package[System->ThisPackage].APICID = (thread_index - thread_counter) + 1;

			break;
		}
	
		// package ID changes -> new package found
		if (processor_info.Location.Package != System->ThisPackage) {
			System->Package[System->ThisPackage].Threads = thread_counter - 1;

			System->Package[System->ThisPackage].Cores = System->Package[System->ThisPackage].Threads / (htt_enabled + 1);
			
			System->Package[System->ThisPackage].APICID = (thread_index - thread_counter) + 1;
			
			System->ThisPackage++;

			thread_counter = 1;
		}
	}

	// reset counter
	System->ThisPackage = 0;
	
	// initialize processor data
	for (System->ThisPackage; System->ThisPackage < System->Platform->Packages; System->ThisPackage++) {
		// CPUID
		AsmCpuid(
			CPUID_VERSION_INFO,
			&System->Package[System->ThisPackage].CPUID,
			NULL,
			NULL,
			NULL
			);

		// Specification
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
			System->Package[System->ThisPackage].Specification,
			CPUID_BRAND_STRING_LEN + 1
			);

		Print(
			L"CPU%d: %s [ %dC/%dT ]\r\n\0",
			System->ThisPackage,
			System->Package[System->ThisPackage].Specification,
			System->Package[System->ThisPackage].Cores,
			System->Package[System->ThisPackage].Threads
			);
				
		// get non-turbo multipliers
		System->ResponseBuffer = AsmReadMsr64(
			MSR_PLATFORM_INFO
			);

		System->Package[System->ThisPackage].CoreMinRatio = (System->ResponseBuffer >> 40) & 0xFF;
		
		System->Package[System->ThisPackage].CoreFlexRatio = (System->ResponseBuffer >> 8) & 0xFF;

		// get maximum (1C) Core turbo multiplier
		System->ProgramBuffer = OC_MB_GET_CPU_CAPS \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC;
		
		OC_MAILBOX_DISPATCH();

		if (OC_MAILBOX_CLEANUP()) {
			Print(
				L"[FAILURE] Error getting maximum (1C) Core turbo ratio\r\n\0"
				);
			
			return EFI_ABORTED;
		}

		System->Package[System->ThisPackage].CoreMaxRatio = System->ResponseBuffer & 0xFF;

		// set turbo multi limit
		if (CPU_SET_MAX_TURBO_RATIO > System->Package[System->ThisPackage].CoreMaxRatio) {
			System->Package[System->ThisPackage].CoreLimitRatio = System->Package[System->ThisPackage].CoreMaxRatio;
		}

		if ((CPU_SET_MAX_TURBO_RATIO < System->Package[System->ThisPackage].CoreMinRatio) \
			&& (CPU_SET_MAX_TURBO_RATIO != 0)) {
			System->Package[System->ThisPackage].CoreLimitRatio = System->Package[System->ThisPackage].CoreMinRatio;
		}

		if (CPU_SET_MAX_TURBO_RATIO == 0) {
			System->Package[System->ThisPackage].CoreLimitRatio = System->Package[System->ThisPackage].CoreMaxRatio;
		}
		else {
			System->Package[System->ThisPackage].CoreLimitRatio = CPU_SET_MAX_TURBO_RATIO;
		}

		// get minimum Uncore multiplier
		System->ResponseBuffer = AsmReadMsr64(
			MSR_UNCORE_RATIO_LIMIT
			);

		System->Package[System->ThisPackage].UncoreMinRatio = (System->ResponseBuffer >> 8) & 0xFF;
		
		// get maximum Uncore multiplier
		System->ProgramBuffer = OC_MB_GET_CPU_CAPS \
			| OC_MB_DOMAIN_CLR \
			| OC_MB_COMMAND_EXEC;
		
		OC_MAILBOX_DISPATCH();

		if (OC_MAILBOX_CLEANUP()) {
			Print(
				L"[FAILURE] Error getting maximum Uncore ratio\r\n\0"
				);
			
			return EFI_ABORTED;
		}

		System->Package[System->ThisPackage].UncoreMaxRatio = System->ResponseBuffer & 0xFF;
		
		// set Uncore multi limit
		if (CPU_SET_MAX_UNCORE_RATIO > System->Package[System->ThisPackage].UncoreMaxRatio) {
			System->Package[System->ThisPackage].UncoreLimitRatio = System->Package[System->ThisPackage].UncoreMaxRatio;
		}
	
		if ((CPU_SET_MAX_UNCORE_RATIO < System->Package[System->ThisPackage].UncoreMinRatio) \
			&& (CPU_SET_MAX_UNCORE_RATIO != 0)) {
			System->Package[System->ThisPackage].UncoreLimitRatio = System->Package[System->ThisPackage].UncoreMinRatio;
		}
	
		if (CPU_SET_MAX_UNCORE_RATIO == 0) {
			System->Package[System->ThisPackage].UncoreLimitRatio = System->Package[System->ThisPackage].UncoreMaxRatio;
		}
		else {
			System->Package[System->ThisPackage].UncoreLimitRatio = CPU_SET_MAX_UNCORE_RATIO;
		}
	}

	System->ThisPackage = 0;

	return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
IsMicrocodePatchDone(
	VOID
) {
	AsmWriteMsr64(
		MSR_IA32_BIOS_SIGN_ID,
		0x0
		);

	AsmCpuid(
		CPUID_VERSION_INFO,
		NULL,
		NULL,
		NULL,
		NULL
		);

	System->ResponseBuffer = AsmReadMsr64(
		MSR_IA32_BIOS_SIGN_ID
		);

	// high word contains microcode update revision patch level
	if (((System->ResponseBuffer >> 32) & 0xFF) != MSR_BIOS_NO_UCODE_PATCH) {
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
		System->ThisPackage
		);

	// verify package CPUID matchs build target CPUID
	if ((System->Package[System->ThisPackage].CPUID != BUILD_TARGET_CPUID_SIGN) \
		&& (BUILD_TARGET_CPUID_SIGN != 0xFFFFFFFF)) {
		Print(
			L"[FAILURE] CPUID (0x%x) does not match target CPUID: 0x%x\r\n\0",
			System->Package[System->ThisPackage].CPUID,
			BUILD_TARGET_CPUID_SIGN
			);

		return FALSE;
	}

	// check OC Lock Bit not set
	System->ResponseBuffer = AsmReadMsr64(
		MSR_FLEX_RATIO
		);

	if ((System->ResponseBuffer & MSR_FLEX_RATIO_OC_LOCK_BIT) == MSR_FLEX_RATIO_OC_LOCK_BIT) {
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
		SetCPUConfiguration();

		// FVIR Configuration
		SetFIVRConfiguration();

		// OC Lock Bit
		FinalizeProgramming();
	}

	return;
}

VOID
EFIAPI
SetCPUConfiguration(
	VOID
) {
	// CPU Max Core Ratio
	System->ProgramBuffer = System->Package[System->ThisPackage].CoreLimitRatio \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 8)  \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 16) \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 24) \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 32) \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 40) \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 48) \
		| (System->Package[System->ThisPackage].CoreLimitRatio << 56);

	Print(
		L"Programming maximum %dC Core turbo ratio (%dx)\r\n\0",
		System->Package[System->ThisPackage].Cores,
		System->Package[System->ThisPackage].CoreLimitRatio
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT,
		System->ProgramBuffer
		);

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT1,
		System->ProgramBuffer
		);

	System->ProgramBuffer |= MSR_TURBO_RATIO_SEMAPHORE_BIT;

	AsmWriteMsr64(
		MSR_TURBO_RATIO_LIMIT2,
		System->ProgramBuffer
		);

	// CPU Cache Ratio, Min CPU Cache Ratio
	System->ResponseBuffer = AsmReadMsr64(
		MSR_UNCORE_RATIO_LIMIT
		);

	System->ProgramBuffer = (System->ResponseBuffer & 0xFFFFFFFFFFFFFF00) \
		| System->Package[System->ThisPackage].UncoreLimitRatio;

	if (UNCORE_PERF_PLIMIT_OVRD_EN == TRUE) {
		Print(
			L"Disabling Uncore P-states, setting fixed Uncore ratio (%dx)\r\n\0",
			System->Package[System->ThisPackage].UncoreLimitRatio
			);

		System->ProgramBuffer &= 0xFFFFFFFFFFFF00FF;

		System->ProgramBuffer |= (System->Package[System->ThisPackage].UncoreLimitRatio << 8);
	}
	else {
		Print(
			L"Programming maximum allowable Uncore ratio (%dx)\r\n\0",
			System->Package[System->ThisPackage].UncoreLimitRatio
			);
	}

	AsmWriteMsr64(
		MSR_UNCORE_RATIO_LIMIT,
		System->ProgramBuffer
		);

	return;
}

VOID
EFIAPI
SetFIVRConfiguration(
	VOID
) {
	Print(
		L"Programming Core/CLR/SA Adaptive Mode offset voltages\r\n\0"
		);

	// IA Core Adaptive Mode offset voltage
	if (IACORE_ADAPTIVE_OFFSET[System->ThisPackage] != _DEFAULT_FVID) {
		System->ProgramBuffer = OC_MB_SET_FVIDS_RATIOS \
			| IACORE_ADAPTIVE_OFFSET[System->ThisPackage] \
			| System->Package[System->ThisPackage].CoreMaxRatio \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}
	
	// CLR Adaptive Mode voltage offset
	if (CLR_ADAPTIVE_OFFSET[System->ThisPackage] != _DEFAULT_FVID) {
		System->ProgramBuffer = OC_MB_SET_FVIDS_RATIOS \
			| CLR_ADAPTIVE_OFFSET[System->ThisPackage] \
			| System->Package[System->ThisPackage].UncoreMaxRatio \
			| OC_MB_DOMAIN_CLR \
			| OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}

	// SA Adaptive Mode voltage offset
	if (SA_ADAPTIVE_OFFSET[System->ThisPackage] != _DEFAULT_FVID) {
		System->ProgramBuffer = OC_MB_SET_FVIDS_RATIOS \
			| SA_ADAPTIVE_OFFSET[System->ThisPackage] \
			| OC_MB_DOMAIN_SA \
			| OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}

	// FIRV Faults
	if (FIVR_FAULTS_OVRD_EN == TRUE) {
		Print(
			L"Disabling Integrated VR Faults\r\n\0"
			);

		System->ProgramBuffer = OC_MB_FIVR_FAULTS_OVRD_EN \
			| OC_MB_SET_FIVR_PARAMS \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}

	// FIVR Efficiency Mode
	if (FIVR_EFF_MODE_OVRD_EN == TRUE) {
		Print(
			L"Disabling Integrated VR Efficiency Mode\r\n\0"
			);

		System->ProgramBuffer = OC_MB_FIVR_EFF_MODE_OVRD_EN \
			| OC_MB_SET_FIVR_PARAMS \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}

	// Fixed VCCIN, Dynamic SVID Control (PowerCut)
	if ((CPU_INPUT_VOLT_MODE_FIXED == TRUE) \
		|| (FIVR_DYN_SVID_CONTROL_DIS == TRUE)) {
		System->ProgramBuffer = OC_MB_SET_SVID_PARAMS \
			| SVID_FIXED_VCCIN[System->ThisPackage] \
			| OC_MB_DOMAIN_IACORE \
			| OC_MB_COMMAND_EXEC;

		// fail safe in case of compile with no data set
		if (SVID_FIXED_VCCIN[System->ThisPackage] == _DYNAMIC_SVID) {
			Print(
				L"[WARNING] Valid VCCIN setpoint not found, using default\r\n\0",
				System->ThisPackage
				);

			System->ProgramBuffer |= _DEFAULT_SVID;
		}

		// Dynamic SVID Control (PowerCut)
		if (FIVR_DYN_SVID_CONTROL_DIS == TRUE) {
			Print(
				L"Disabling FIVR Dynamic SVID Control, setting fixed VCCIN\r\n\0"
				);

			System->ProgramBuffer |= OC_MB_FIVR_DYN_SVID_CONTROL_DIS;
		}
		else {
			Print(
				L"Programming fixed CPU Input Voltage (VCCIN)\r\n\0"
				);
		}

		OC_MAILBOX_DISPATCH();

		OC_MAILBOX_CLEANUP();
	}

	return;
}

VOID
EFIAPI
FinalizeProgramming(
	VOID
) {
	if (CPU_SET_OC_LOCK == TRUE) {
		System->ResponseBuffer = AsmReadMsr64(
			MSR_FLEX_RATIO
			);

		// prevents any changes to write-once MSR values once set (reboot required to clear)
		System->ProgramBuffer = System->ResponseBuffer | MSR_FLEX_RATIO_OC_LOCK_BIT;

		AsmWriteMsr64(
			MSR_FLEX_RATIO,
			System->ProgramBuffer
			);
	}

	return;
}

VOID
EFIAPI
OC_MAILBOX_REQUEST(
	VOID
) {
	System->ResponseBuffer = AsmReadMsr64(
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
		System->ProgramBuffer
		);

	// retrieve immediate response from mailbox
	System->ResponseBuffer = AsmReadMsr64(
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
	if (((System->ResponseBuffer >> 32) & 0xFF) == OC_MB_REBOOT_REQUIRED) {
		System->RebootRequired = TRUE;

		return FALSE;
	}
	
	// if non-zero there was an error
	if (((System->ResponseBuffer >> 32) & 0xFF) != OC_MB_SUCCESS) {
		return TRUE;
	}

	return FALSE;
}
