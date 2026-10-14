[Files]
DestDir: {commontemplates}; Source: template.ass; DestName: OpenKtooviot.ass

[Registry]
; File type registration
; Application registration for Open With dialogue
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "@{app}\openktooviot.exe,-10000"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe"; ValueType: string; ValueName: "ApplicationCompany"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\shell"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\Applications\openktooviot.exe\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\shell\open"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "@{app}\openktooviot.exe,-10000"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\Applications\openktooviot.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".ass"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".ssa"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".srt"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".sub"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".ttxt"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".txt"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".mkv"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".mka"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".mks"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".avi"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".mp3"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".mp4"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".aac"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".m4a"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".wav"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".ogg"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".avs"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".opus"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".h264"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".hevc"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".eac3"; ValueData: ""; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\Applications\openktooviot.exe\SupportedTypes"; ValueType: string; ValueName: ".webm"; ValueData: ""; Flags: uninsdeletekey
; Class for general subtitle formats
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot subtitle file"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10101"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Subtitle.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for .ass files
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot Advanced SSA subtitles"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10102"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.ASSA.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for .ssa files
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SSA.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot SubStation Alpha subtitles"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SSA.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SSA.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10103"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SSA.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SSA.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.SSA.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.SSA.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for .srt files
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SRT.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot SubRip text subtitles"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SRT.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SRT.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10104"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SRT.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.SRT.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.SRT.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.SRT.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for .ttxt files
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot MPEG-4 timed text"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10105"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.TTXT.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for .mks files
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.MKS.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot Matroska subtitles"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.MKS.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.MKS.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10106"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.MKS.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.MKS.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.MKS.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.MKS.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for .txt files
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TXT.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot raw text file"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TXT.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TXT.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10107"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TXT.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.TXT.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.TXT.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.TXT.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for undecideable media file types
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Media.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot media file"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Media.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Media.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10108"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Media.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Media.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Media.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Media.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for audio file types
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Audio.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot audio file"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Audio.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Audio.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10109"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Audio.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Audio.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Audio.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Audio.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Class for video file types
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Video.1"; ValueType: string; ValueName: ""; ValueData: "OpenKtooviot video file"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Video.1"; ValueType: dword; ValueName: "EditFlags"; ValueData: $af0; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Video.1"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\openktooviot.exe,-10110"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Video.1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\openktooviot.exe,0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\OpenKtooviot.Video.1\shell"; ValueType: string; ValueName: ""; ValueData: "open"; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Video.1\shell\open"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; SubKey: "SOFTWARE\Classes\OpenKtooviot.Video.1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\openktooviot.exe"" ""%L"""; Flags: uninsdeletekey
; Default Programs registration
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities"; ValueType: none
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "@{app}\openktooviot.exe,-10001"
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities\FileAssociations"; ValueType: none
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ass"; ValueData: "OpenKtooviot.ASSA.1"
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ssa"; ValueData: "OpenKtooviot.SSA.1"
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities\FileAssociations"; ValueType: string; ValueName: ".srt"; ValueData: "OpenKtooviot.SRT.1"
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ttxt"; ValueData: "OpenKtooviot.TTXT.1"
Root: HKLM; Subkey: "SOFTWARE\OpenKtooviot\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mks"; ValueData: "OpenKtooviot.MKS.1"
Root: HKLM; Subkey: "SOFTWARE\RegisteredApplications"; ValueType: string; ValueName: "OpenKtooviot"; ValueData: "SOFTWARE\OpenKtooviot\Capabilities"; Flags: uninsdeletevalue
; Default handler for .ass
; Only register us as owner of the type if there isn't one already. Windows XP doesn't cooperate well when a type has no owner,
; even if everything else exists. If it already has an owner, use some other UI to take over ownership if the user desires.
; Only set perceived types for the main text-based subtitle formats.
; We only have a template file for .ass, that's the primary subtitle format.
; Register us as a valid ProgID for every type we can reasonably handle, and a few we might not be able to.
Root: HKLM; SubKey: "SOFTWARE\Classes\.ass"; ValueType: string; ValueData: "OpenKtooviot.ASSA.1"; Flags: createvalueifdoesntexist
Root: HKLM; SubKey: "SOFTWARE\Classes\.ass"; ValueType: string; ValueName: "PerceivedType"; ValueData: "text"; Flags: createvalueifdoesntexist
Root: HKLM; Subkey: "SOFTWARE\Classes\.ass\OpenKtooviot.ASSA.1"; ValueType: none; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\.ass\OpenKtooviot.ASSA.1\ShellNew"; ValueType: string; ValueName: "FileName"; ValueData: "{commontemplates}\OpenKtooviot.ass"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\.ass\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.ASSA.1"; Flags: uninsdeletevalue
; Default handler for .ssa
Root: HKLM; SubKey: "SOFTWARE\Classes\.ssa"; ValueType: string; ValueData: "OpenKtooviot.SSA.1"; Flags: createvalueifdoesntexist
Root: HKLM; SubKey: "SOFTWARE\Classes\.ssa"; ValueType: string; ValueName: "PerceivedType"; ValueData: "text"; Flags: createvalueifdoesntexist
Root: HKLM; Subkey: "SOFTWARE\Classes\.ssa\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.SSA.1"; Flags: uninsdeletevalue
; Default handler for .srt
Root: HKLM; SubKey: "SOFTWARE\Classes\.srt"; ValueType: string; ValueData: "OpenKtooviot.SRT.1"; Flags: createvalueifdoesntexist
Root: HKLM; SubKey: "SOFTWARE\Classes\.srt"; ValueType: string; ValueName: "PerceivedType"; ValueData: "text"; Flags: createvalueifdoesntexist
Root: HKLM; Subkey: "SOFTWARE\Classes\.srt\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.SRT.1"; Flags: uninsdeletevalue
; Default handler for .ttxt
Root: HKLM; SubKey: "SOFTWARE\Classes\.ttxt"; ValueType: string; ValueData: "OpenKtooviot.TTXT.1"; Flags: createvalueifdoesntexist
Root: HKLM; SubKey: "SOFTWARE\Classes\.ttxt"; ValueType: string; ValueName: "PerceivedType"; ValueData: "text"; Flags: createvalueifdoesntexist
Root: HKLM; Subkey: "SOFTWARE\Classes\.ttxt\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.TTXT.1"; Flags: uninsdeletevalue
; Default handler for .mks
Root: HKLM; SubKey: "SOFTWARE\Classes\.mks"; ValueType: string; ValueData: "OpenKtooviot.MKS.1"; Flags: createvalueifdoesntexist
Root: HKLM; SubKey: "SOFTWARE\Classes\.mks"; ValueType: string; ValueName: "PerceivedType"; ValueData: "text"; Flags: createvalueifdoesntexist
Root: HKLM; Subkey: "SOFTWARE\Classes\.mks\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.MKS.1"; Flags: uninsdeletevalue
; Support opening a bunch more types
Root: HKLM; Subkey: "SOFTWARE\Classes\.sub\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Subtitle.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.txt\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.TXT.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.mkv\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Video.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.mka\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.avi\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Video.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.mp3\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.mp4\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Media.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.aac\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.m4a\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.wav\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.ogg\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Media.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.avs\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Video.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.opus\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.h264\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Video.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.hevc\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Video.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.eac3\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Audio.1"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Classes\.webm\OpenWithProgids"; ValueType: string; ValueName: "OpenKtooviot.Media.1"; Flags: uninsdeletevalue
