// initguid.h makes the DEFINE_GUID macros in msctf.h emit definitions in this
// translation unit — this is the single TU that instantiates the TSF category
// and class GUIDs the DLL links against.
#include <initguid.h>

#include <inputscope.h>
#include <msctf.h>

#include "goxvi-guids.h"

// {7A3B9F42-6C1D-4E85-9B0A-2F4D8C7E5A31}
const CLSID CLSID_GoxviTextService = {
    0x7a3b9f42, 0x6c1d, 0x4e85, {0x9b, 0x0a, 0x2f, 0x4d, 0x8c, 0x7e, 0x5a, 0x31}};

// {B4E2D7A9-3F58-4C16-8D2B-7E9A0C4F6B53}
const GUID GUID_GoxviProfile = {
    0xb4e2d7a9, 0x3f58, 0x4c16, {0x8d, 0x2b, 0x7e, 0x9a, 0x0c, 0x4f, 0x6b, 0x53}};

// {C5F3E8B0-4A69-4D27-9E3C-8F0B1D5A7C64}
const GUID GUID_GoxviDisplayAttribute = {
    0xc5f3e8b0, 0x4a69, 0x4d27, {0x9e, 0x3c, 0x8f, 0x0b, 0x1d, 0x5a, 0x7c, 0x64}};
