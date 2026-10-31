# sdl-input-driver-detach.patch

Patch to the ReXGlue SDK submodule (`thirdparty/rexglue-sdk`). It is applied at
configure time on macOS by [`cmake/apply_sdk_patches.cmake`](../../cmake/apply_sdk_patches.cmake).

**What it does:** The SDL controller driver (`SDLInputDriver`) unregisters from
the window and from SDL's event watch whenever it lets go of the window: when the
window closes (as before), when the window is destroyed without being closed,
and when the driver itself is destroyed first.

## Symptom

Exiting the game from its own menu crashed the process on macOS. macOS then
showed "Perfect Dark quit unexpectedly", and `rexglue-crash.txt` got a
`SIGSEGV` at address `0x20`:

```
rex::ui::Window::NotifyDestroyingWhileOpen()
rex::ui::WindowSDL::~WindowSDL()
rex::ReXApp::OnDestroy()
main
```

Quitting with ⌘Q, the Dock or the window's close button did not crash.

## Root cause

1. **Two ways to quit.**
   - Closing the window (⌘Q, Dock, close button) runs `ReXApp::OnClosing`. It
     terminates the title and calls `std::_Exit(0)`, so nothing is torn down.
   - The in-game exit (`XamLoaderTerminateTitle`) ends the guest main thread
     instead. The module thread then calls `QuitFromUIThread()`: the main loop
     ends with the window still open, and `ReXApp::OnDestroy` tears everything
     down.
2. **`OnDestroy` frees the input drivers before the window.** It calls
   `InputSystem::Shutdown()` (`drivers_.clear()`) and then `window_.reset()`.
3. **The SDL driver stayed registered on the window.** It called
   `window->AddListener(this)` when the window appeared, but removed itself only
   in `OnClosing`, and its destructor was empty. So the freed driver was still in
   the window's listener list.
4. **The window notified the freed driver.** `~WindowSDL` calls
   `NotifyDestroyingWhileOpen()`, which calls `OnDestroying` on every listener.
   For the freed driver, the vtable pointer read as 0 (macOS's allocator had
   cleared the freed memory), and loading the `OnDestroying` slot at offset
   `0x20` faulted:

   ```
   ldr x9, [x0]          ; listener vtable -> 0
   ldr x2, [x9, #0x20]   ; OnDestroying -> SIGSEGV at 0x20
   ```

The keyboard/mouse driver (`MnkInputDriver`) had the same problem and already
detaches in its destructor and in `OnDestroying` (rexglue-sdk issue #525, noted
in `ReXApp::OnDestroy`). The SDL driver was missed. Upstream
(`rexglue/rexglue-sdk` `main`) still has the empty destructor.

The driver also never removed its `SDL_AddEventWatch` callback, so SDL could
still call into the freed driver for a controller event during shutdown.

## The change

- **`DetachFromWindow()`:** runs the old `OnClosing` cleanup on the UI thread
  (`CallInUIThreadSynchronous`, like `MnkInputDriver`):
  - removes the window listener;
  - removes the SDL event watch (`SDL_RemoveEventWatch` waits for a callback that
    is running on another thread);
  - runs a queued `SDL_PumpEvents` task;
  - closes the gamepads and quits the SDL subsystems.

  It does nothing once the driver is detached.
- **Callers:** `OnClosing`, the new `OnDestroying` override, and the destructor.
- **The event watch callback:** it moves from a lambda to the static member
  `EventWatch`, so that `SDL_RemoveEventWatch` can be given the same function
  pointer.

## Evidence

- **Crash reports:** the faulting instruction is the vtable load in the
  `NotifyDestroyingWhileOpen` lambda. The listener's vtable pointer was 0
  (`x9 = 0`).
- **In game (RelWithDebInfo, M3 Pro):**
  - Before the patch: both exits from the in-game menu crashed.
  - With the patch: the in-game exit runs the whole teardown ("Execution
    complete", then the Vulkan device and instance are destroyed). There was no
    crash entry and no macOS crash report.

## Other platforms

The use-after-free exists on Windows and Linux too. Whether it crashes depends
on what the allocator writes into freed memory. The patch only applies on macOS,
like the other patches here.

## When to drop it

- **If upstream fixes it:** when the SDK's `SDLInputDriver` detaches in its
  destructor or `OnDestroying`, drop the patch.
- **If the pin moves:** if the submodule pin moves and the patch no longer
  applies, the configure step stops with an error. Then check whether the fix is
  upstream, and drop or update the patch.
