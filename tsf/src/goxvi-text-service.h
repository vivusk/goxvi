#pragma once

#include <msctf.h>
#include <windows.h>
#include <wrl/client.h>

#include "composition-manager.h"
#include "config-file-watcher.h"
#include "goxvi/telex-engine.h"

// Main TIP object: thin shim wiring the Telex engine into TSF.
// Keystroke flow: OnKeyDown → classify → TelexEngine::processKey → edit
// session updates the (hidden) composition → terminators commit + pass.
class GoxviTextService : public ITfTextInputProcessorEx,
                         public ITfKeyEventSink,
                         public ITfCompositionSink,
                         public ITfDisplayAttributeProvider,
                         public ITfThreadMgrEventSink,
                         public ITfTextEditSink {
 public:
  GoxviTextService();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  // ITfTextInputProcessor(Ex)
  STDMETHODIMP Activate(ITfThreadMgr* threadMgr, TfClientId clientId) override;
  STDMETHODIMP ActivateEx(ITfThreadMgr* threadMgr, TfClientId clientId,
                          DWORD flags) override;
  STDMETHODIMP Deactivate() override;

  // ITfKeyEventSink
  STDMETHODIMP OnSetFocus(BOOL foreground) override;
  STDMETHODIMP OnTestKeyDown(ITfContext* context, WPARAM wparam, LPARAM lparam,
                             BOOL* eaten) override;
  STDMETHODIMP OnTestKeyUp(ITfContext* context, WPARAM wparam, LPARAM lparam,
                           BOOL* eaten) override;
  STDMETHODIMP OnKeyDown(ITfContext* context, WPARAM wparam, LPARAM lparam,
                         BOOL* eaten) override;
  STDMETHODIMP OnKeyUp(ITfContext* context, WPARAM wparam, LPARAM lparam,
                       BOOL* eaten) override;
  STDMETHODIMP OnPreservedKey(ITfContext* context, REFGUID guid,
                              BOOL* eaten) override;

  // ITfCompositionSink
  STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite,
                                       ITfComposition* composition) override;

  // ITfDisplayAttributeProvider (display-attribute-provider.cpp)
  STDMETHODIMP EnumDisplayAttributeInfo(
      IEnumTfDisplayAttributeInfo** enumInfo) override;
  STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid,
                                       ITfDisplayAttributeInfo** info) override;

  // ITfThreadMgrEventSink
  STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* docMgr) override;
  STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* docMgr) override;
  STDMETHODIMP OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr* prevFocus) override;
  STDMETHODIMP OnPushContext(ITfContext* context) override;
  STDMETHODIMP OnPopContext(ITfContext* context) override;

  // ITfTextEditSink (advised on the focused context): catches the caret
  // leaving the composing word (mouse click, app-driven selection move) in
  // hosts that never call OnCompositionTerminated — commit instead of losing
  // the word.
  STDMETHODIMP OnEndEdit(ITfContext* context, TfEditCookie ecReadOnly,
                         ITfEditRecord* editRecord) override;

 private:
  ~GoxviTextService();

  HRESULT adviseKeyEventSink();
  void unadviseKeyEventSink();
  HRESULT adviseThreadMgrSink();
  void unadviseThreadMgrSink();
  // (Re)advise OnEndEdit on the top context of the focused doc mgr; null
  // unadvises only. Follows focus (Microsoft sample IME pattern).
  void adviseTextEditSink(ITfDocumentMgr* docMgr);
  void unadviseTextEditSink();
  void registerDisplayAttributeGuid();

  // Shared by OnTestKeyDown/OnKeyDown so both report the same eaten decision;
  // testOnly suppresses engine/composition mutation for letters/backspace.
  // Commit-and-pass runs in both (idempotent) so apps that only call
  // TestKeyDown still get the word committed before the terminator lands.
  HRESULT handleKeyDown(ITfContext* context, WPARAM wparam, BOOL* eaten,
                        bool testOnly);
  void processLetter(ITfContext* context, wchar_t ch);
  void processBackspace(ITfContext* context);
  // applyShortcut=false (Esc only) forces the raw text through unexpanded.
  void commitCurrentWord(bool applyShortcut = true);
  // WH_MOUSE hook callback: finalize the word before the app handles a click
  // (IMM/CUAS hosts cancel the composition string on mouse down). Returns
  // true when a composing word was committed (hook eats + re-injects click).
  bool commitOnPointerDown();
  void cancelWordToRaw(ITfContext* context);  // Esc: restore raw + commit
  void pollConfigAtWordStart();  // lazy mtime reload, throttled (phase 5)

  LONG refCount_ = 1;
  Microsoft::WRL::ComPtr<ITfThreadMgr> threadMgr_;
  TfClientId clientId_ = TF_CLIENTID_NULL;
  // Host paints its own autocomplete popup and terminates the composition on a
  // click itself (browser address bars). For these we must NOT pre-commit in
  // the mouse hook — doing so reflows the popup before the click is hit-tested,
  // selecting the wrong suggestion. Set once at Activate. See commitOnPointerDown.
  bool hostHandlesClickTermination_ = false;
  bool keySinkAdvised_ = false;
  DWORD threadMgrSinkCookie_ = TF_INVALID_COOKIE;
  Microsoft::WRL::ComPtr<ITfContext> editSinkContext_;
  DWORD textEditSinkCookie_ = TF_INVALID_COOKIE;
  TfGuidAtom displayAttributeAtom_ = TF_INVALID_GUIDATOM;

  // Blocked-context verdict (disabled compartment / password input scope)
  // memoized from OnTestKeyDown for the immediately following OnKeyDown of the
  // SAME key on the SAME context — the check costs a sync edit session and
  // would otherwise run twice per word start. Consulted only within a short
  // tick window; anything else recomputes.
  struct BlockedVerdictCache {
    ITfContext* context = nullptr;  // raw pointer, identity comparison only
    WPARAM vk = 0;
    ULONGLONG tick = 0;
    bool blocked = false;
  } blockCheck_;

  goxvi::TelexEngine engine_;
  CompositionManager composition_;
  ConfigFileWatcher configWatcher_;
  goxvi::EngineConfig config_;
};
