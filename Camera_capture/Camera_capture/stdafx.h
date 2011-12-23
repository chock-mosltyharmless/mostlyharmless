// stdafx.h : Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Teile der Windows-Header nicht einbinden.
// Windows-Headerdateien:
#include <windows.h>

// C RunTime-Headerdateien
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

//Setup some defines you need to connect to a webcam
#if 0
#define WM_CAP_START  0x0400
#define WM_CAP_DRIVER_CONNECT  (WM_CAP_START + 10)
#define WM_CAP_DRIVER_DISCONNECT  (WM_CAP_START + 11)
#define WM_CAP_EDIT_COPY (WM_CAP_START + 30)
#define WM_CAP_GRAB_FRAME (WM_CAP_START + 60)
#define WM_CAP_SET_SCALE (WM_CAP_START + 53)
#define WM_CAP_SET_PREVIEWRATE (WM_CAP_START + 52)
#define WM_CAP_SET_PREVIEW (WM_CAP_START + 50)
#define WM_CAP_DLG_VIDEOSOURCE  (WM_CAP_START + 42)
#define WM_CAP_STOP (WM_CAP_START+ 68)
#endif
//End of defines

#include <vfw.h>
//Remember to Link to vfw32 Library, gdi32 Library



// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
