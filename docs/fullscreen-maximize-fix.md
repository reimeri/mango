# Preserve maximization after fullscreen

This branch is based on MangoWM **0.18.0**. It fixes losing the maximized state
when a maximized window enters fullscreen (for example, a browser video) and
then exits fullscreen.

The previous maximized state is saved only on the initial fullscreen transition.
Normal fullscreen exit restores maximization, including work-area geometry,
gaps, borders, and group-bar space. Repeated requests do not overwrite the saved
state. Explicit maximize/unmaximize, floating/tiling, scratchpad, and forced
fullscreen/maximize clearing actions cancel restoration. Monitor moves and
window-state transfers preserve it. Scroller stacking still clears both states.

## Use the fork as a Nix flake input

If your configuration already uses Mango's flake modules, change the existing
input URL to the fork branch:

```nix
inputs.mango.url = "github:reimeri/mango/fix-fullscreen-maximize";
```

The fork retains the upstream package and module outputs. If your configuration
selects its package separately, point it at the fork too. For NixOS:

```nix
# In a module receiving `inputs` and `pkgs`:
programs.mango.package =
  inputs.mango.packages.${pkgs.stdenv.hostPlatform.system}.mango;
```

If Home Manager also selects a Mango package, use the same package there:

```nix
wayland.windowManager.mango.package =
  inputs.mango.packages.${pkgs.stdenv.hostPlatform.system}.mango;
```

Update the input lock and rebuild using your usual configuration command:

```sh
nix flake update mango
sudo nixos-rebuild switch --flake .#YOUR_HOST
```

Log out and back in after rebuilding; reloading Mango's config does not replace
the running compositor binary. This also upgrades a 0.17.5 installation to
0.18.0, so review upstream release/config changes separately.

You can build the package independently without installing or starting it:

```sh
nix build github:reimeri/mango/fix-fullscreen-maximize#mango
```

## Regression checks

```sh
python3 tests/test_fullscreen_restore.py
```

Requires Python 3 and a C compiler (`CC` is honored). The test compiles the actual
state-transition functions against lightweight compositor mocks. It covers
normal and repeated transitions, tiled/floating windows, borders, group bars,
fake maximize, layout-dependent geometry, explicit overrides, overview restore,
monitor reassignment, swallow teardown state synchronization, and scroller-head
cleanup. It is not a live Wayland/XWayland integration test.

Manual check in a session running the patched binary:

1. Maximize the browser using your `togglemaximizescreen` binding (Super+F in
   the reported configuration).
2. Fullscreen a video, then exit video fullscreen.
3. Confirm the browser remains maximized, respecting panels and configured gaps.
4. Repeat, then confirm an initially unmaximized browser stays unmaximized.
