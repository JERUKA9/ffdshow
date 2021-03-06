[code]
type
  TSystemInfo = record
    wProcessorArchitecture: Word;
    wReserved: Word;
    dwPageSize: DWORD;
    lpMinimumApplicationAddress: Integer;
    lpMaximumApplicationAddress: Integer;
    dwActiveProcessorMask: DWORD;
    dwNumberOfProcessors: DWORD;
    dwProcessorType: DWORD;
    dwAllocationGranularity: DWORD;
    wProcessorLevel: Word;
    wProcessorRevision: Word;
  end;

procedure GetSystemInfo(var lpSystemInfo: TSystemInfo);
external 'GetSystemInfo@kernel32.dll stdcall';

function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';

#if include_quicksync
function QuickSyncCheck(): DWORD;
external 'getQsCaps@files:ffSpkCfg.dll stdcall delayload';
#endif

var
	cpu_sse: Boolean;
	cpu_sse2: Boolean;
	cpu_cores: Integer;
	cpu_level: Integer;
	cpu_intel_qs: Boolean;

procedure DetectCPU();
var
  SysInfo: TSystemInfo;
  qs_check: DWORD;
begin
	cpu_sse := IsProcessorFeaturePresent(6);
	cpu_sse2 := IsProcessorFeaturePresent(10);
	
	GetSystemInfo(SysInfo);
	cpu_level := SysInfo.wProcessorLevel;
	
	#if include_quicksync
	cpu_intel_qs := (InstallOnThisVersion('0,6','0,0') = irInstall) AND (SysInfo.wProcessorArchitecture = 0) AND (cpu_level >= 6) AND (QuickSyncCheck > 0);
	#else
	cpu_intel_qs := False;
	#endif
	
	cpu_cores := SysInfo.dwNumberOfProcessors;
	if cpu_cores > 8 then begin
    cpu_cores := 8;
  end;
  if cpu_cores < 1 then begin
    cpu_cores := 1;
  end;
end;

function Is_SSE_Supported(): Boolean;
begin
  Result := cpu_sse;
end;

function Is_SSE2_Supported(): Boolean;
begin
  Result := cpu_sse2;
end;

function GetCPULevel(): Integer;
begin
  Result := cpu_level;
end;

function GetNumberOfCores(): Integer;
begin
  Result := cpu_cores;
end;

function IsQSCapableIntelCPU(): Boolean;
begin
  Result := cpu_intel_qs;
end;
