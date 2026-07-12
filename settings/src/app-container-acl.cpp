#include "app-container-acl.h"

#include <windows.h>

#include <aclapi.h>
#include <sddl.h>

namespace goxvi::settings {

namespace {

// True if `dacl` already grants `sid` an allow ACE covering read+execute — lets
// us skip the (comparatively costly) SetNamedSecurityInfo rewrite on relaunch.
bool alreadyGranted(PACL dacl, PSID sid) {
  if (dacl == nullptr) return false;
  for (DWORD i = 0; i < dacl->AceCount; ++i) {
    LPVOID ace = nullptr;
    if (!GetAce(dacl, i, &ace)) continue;
    auto* header = static_cast<ACE_HEADER*>(ace);
    if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) continue;
    auto* allow = static_cast<ACCESS_ALLOWED_ACE*>(ace);
    PSID aceSid = &allow->SidStart;
    if (!EqualSid(aceSid, sid)) continue;
    // FILE_GENERIC_READ|EXECUTE bits present → grant is effectively there.
    const DWORD wanted = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;
    if ((allow->Mask & wanted) == wanted) return true;
  }
  return false;
}

}  // namespace

bool grantAllAppPackagesRead(const std::wstring& dir) {
  PSID sid = nullptr;
  if (!ConvertStringSidToSidW(L"S-1-15-2-1", &sid)) return false;  // ALL APP PKGS

  bool ok = false;
  PACL oldDacl = nullptr;
  PSECURITY_DESCRIPTOR sd = nullptr;
  if (GetNamedSecurityInfoW(dir.c_str(), SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION, nullptr, nullptr,
                            &oldDacl, nullptr, &sd) == ERROR_SUCCESS) {
    if (alreadyGranted(oldDacl, sid)) {
      ok = true;  // nothing to do
    } else {
      EXPLICIT_ACCESSW ea = {};
      ea.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
      ea.grfAccessMode = SET_ACCESS;
      ea.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
      ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
      ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
      ea.Trustee.ptstrName = static_cast<LPWSTR>(sid);

      PACL newDacl = nullptr;
      if (SetEntriesInAclW(1, &ea, oldDacl, &newDacl) == ERROR_SUCCESS) {
        ok = SetNamedSecurityInfoW(const_cast<LPWSTR>(dir.c_str()),
                                   SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                   nullptr, nullptr, newDacl,
                                   nullptr) == ERROR_SUCCESS;
        if (newDacl) LocalFree(newDacl);
      }
    }
  }

  if (sd) LocalFree(sd);
  LocalFree(sid);
  return ok;
}

}  // namespace goxvi::settings
