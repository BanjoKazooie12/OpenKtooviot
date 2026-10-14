#ifdef ARM64
#define ARCH "arm64"
#else
#define ARCH "x64"
#endif

#include "fragment_setupbase.iss"
#include "fragment_strings.iss"

[Setup]
; OpenKtooviot has its own ID so that it installs alongside Aegisub
AppID={{EC56659D-C5AE-495F-BC8F-79B4323EDF8D}
DefaultDirName={commonpf}\OpenKtooviot
PrivilegesRequired=poweruser
#ifdef ARM64
ArchitecturesInstallIn64BitMode=arm64
ArchitecturesAllowed=arm64
#else
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
#endif

#include "fragment_mainprogram.iss"
#include "fragment_associations.iss"
#ifndef ARM64
; The bundled VSFilter binary is x64-only.
#include "fragment_codecs.iss"
#endif
#include "fragment_automation.iss"
#include "fragment_translations.iss"
#include "fragment_spelling.iss"
#include "fragment_runtimes.iss"

[Code]
#include "fragment_shell_code.iss"
#include "fragment_migrate_code.iss"
#include "fragment_beautify_code.iss"

procedure InitializeWizard;
begin
  InitializeWizardBeautify;
end;

function InitializeSetup: Boolean;
begin
  // The migration code finds and removes old Aegisub installations, which an
  // OpenKtooviot installer must leave alone
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Updates: String;
begin
  if CurStep = ssPostInstall then
  begin
    // OpenKtooviot is built without the update checker, which would check
    // for Aegisub updates
    Updates := 'false';

    SaveStringToFile(
      ExpandConstant('{app}\installer_config.json'),
      FmtMessage('{"App": {"Auto": {"Check For Updates": %1}, "First Start": false, "Language": "%2"}}', [
        Updates,
        ExpandConstant('{language}')]),
      False);
  end;
end;
