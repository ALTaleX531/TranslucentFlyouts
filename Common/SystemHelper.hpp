﻿#pragma once
#include "framework.h"

namespace TranslucentFlyouts::SystemHelper
{
	struct KSYSTEM_TIME
	{
		ULONG LowPart;
		LONG High1Time;
		LONG High2Time;
	};
	enum class NT_PRODUCT_TYPE
	{
		NtProductWinNt = 1,
		NtProductLanManNt,
		NtProductServer
	};
	enum class ALTERNATIVE_ARCHITECTURE_TYPE
	{
		StandardDesign,
		NEC98x86,
		EndAlternatives
	};
	constexpr UINT PROCESSOR_FEATURE_MAX = 64;
	struct KUSER_SHARED_DATA
	{
		ULONG                         TickCountLowDeprecated;
		ULONG                         TickCountMultiplier;
		KSYSTEM_TIME                  InterruptTime;
		KSYSTEM_TIME                  SystemTime;
		KSYSTEM_TIME                  TimeZoneBias;
		USHORT                        ImageNumberLow;
		USHORT                        ImageNumberHigh;
		WCHAR                         NtSystemRoot[260];
		ULONG                         MaxStackTraceDepth;
		ULONG                         CryptoExponent;
		ULONG                         TimeZoneId;
		ULONG                         LargePageMinimum;
		ULONG                         AitSamplingValue;
		ULONG                         AppCompatFlag;
		ULONGLONG                     RNGSeedVersion;
		ULONG                         GlobalValidationRunlevel;
		LONG                          TimeZoneBiasStamp;
		ULONG                         NtBuildNumber;
		NT_PRODUCT_TYPE               NtProductType;
		BOOLEAN                       ProductTypeIsValid;
		BOOLEAN                       Reserved0[1];
		USHORT                        NativeProcessorArchitecture;
		ULONG                         NtMajorVersion;
		ULONG                         NtMinorVersion;
		BOOLEAN                       ProcessorFeatures[PROCESSOR_FEATURE_MAX];
		ULONG                         Reserved1;
		ULONG                         Reserved3;
		ULONG                         TimeSlip;
		ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
		ULONG                         BootId;
		LARGE_INTEGER                 SystemExpirationDate;
		ULONG                         SuiteMask;
		BOOLEAN                       KdDebuggerEnabled;
		union
		{
			UCHAR MitigationPolicies;
			struct
			{
				UCHAR NXSupportPolicy : 2;
				UCHAR SEHValidationPolicy : 2;
				UCHAR CurDirDevicesSkippedForDlls : 2;
				UCHAR Reserved : 2;
			};
		};
		USHORT                        CyclesPerYield;
		ULONG                         ActiveConsoleId;
		ULONG                         DismountCount;
		ULONG                         ComPlusPackage;
		ULONG                         LastSystemRITEventTickCount;
		ULONG                         NumberOfPhysicalPages;
		BOOLEAN                       SafeBootMode;
		union
		{
			UCHAR VirtualizationFlags;
			struct
			{
				UCHAR ArchStartedInEl2 : 1;
				UCHAR QcSlIsSupported : 1;
			};
		};
		UCHAR                         Reserved12[2];
		union
		{
			ULONG SharedDataFlags;
			struct
			{
				ULONG DbgErrorPortPresent : 1;
				ULONG DbgElevationEnabled : 1;
				ULONG DbgVirtEnabled : 1;
				ULONG DbgInstallerDetectEnabled : 1;
				ULONG DbgLkgEnabled : 1;
				ULONG DbgDynProcessorEnabled : 1;
				ULONG DbgConsoleBrokerEnabled : 1;
				ULONG DbgSecureBootEnabled : 1;
				ULONG DbgMultiSessionSku : 1;
				ULONG DbgMultiUsersInSessionSku : 1;
				ULONG DbgStateSeparationEnabled : 1;
				ULONG SpareBits : 21;
			} DUMMYSTRUCTNAME2;
		} DUMMYUNIONNAME2;
		ULONG                         DataFlagsPad[1];
		ULONGLONG                     TestRetInstruction;
		LONGLONG                      QpcFrequency;
		ULONG                         SystemCall;
		ULONG                         Reserved2;
		ULONGLONG                     SystemCallPad[2];
		union
		{
			KSYSTEM_TIME TickCount;
			ULONG64      TickCountQuad;
			struct
			{
				ULONG ReservedTickCountOverlay[3];
				ULONG TickCountPad[1];
			} DUMMYSTRUCTNAME;
		} DUMMYUNIONNAME3;
		ULONG                         Cookie;
		ULONG                         CookiePad[1];
		LONGLONG                      ConsoleSessionForegroundProcessId;
		ULONGLONG                     TimeUpdateLock;
		ULONGLONG                     BaselineSystemTimeQpc;
		ULONGLONG                     BaselineInterruptTimeQpc;
		ULONGLONG                     QpcSystemTimeIncrement;
		ULONGLONG                     QpcInterruptTimeIncrement;
		UCHAR                         QpcSystemTimeIncrementShift;
		UCHAR                         QpcInterruptTimeIncrementShift;
		USHORT                        UnparkedProcessorCount;
		ULONG                         EnclaveFeatureMask[4];
		ULONG                         TelemetryCoverageRound;
		USHORT                        UserModeGlobalLogger[16];
		ULONG                         ImageFileExecutionOptions;
		ULONG                         LangGenerationCount;
		ULONGLONG                     Reserved4;
		ULONGLONG                     InterruptTimeBias;
		ULONGLONG                     QpcBias;
		ULONG                         ActiveProcessorCount;
		UCHAR                         ActiveGroupCount;
		UCHAR                         Reserved9;
		union
		{
			USHORT QpcData;
			struct
			{
				UCHAR QpcBypassEnabled;
				UCHAR QpcShift;
			};
		};
		LARGE_INTEGER                 TimeZoneBiasEffectiveStart;
		LARGE_INTEGER                 TimeZoneBiasEffectiveEnd;
		XSTATE_CONFIGURATION          XState;
		KSYSTEM_TIME                  FeatureConfigurationChangeStamp;
		ULONG                         Spare;
		ULONG64                       UserPointerAuthMask;
	};
	using PKUSER_SHARED_DATA = KUSER_SHARED_DATA* const;

	static PKUSER_SHARED_DATA GetKernelSharedInfo()
	{
		return reinterpret_cast<PKUSER_SHARED_DATA>(0x7FFE0000);
	}

	inline const ULONG g_buildNumber{ GetKernelSharedInfo()->NtBuildNumber };
	inline const ULONG g_minorVersion{ GetKernelSharedInfo()->NtMinorVersion };
	inline const ULONG g_majorVersion{ GetKernelSharedInfo()->NtMajorVersion };
	inline const NT_PRODUCT_TYPE g_productType{ GetKernelSharedInfo()->NtProductType };
}