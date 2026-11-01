# moltenvk-no-contraction.patch

Patch to the ReXGlue SDK submodule (`thirdparty/rexglue-sdk`). It is applied at
configure time on macOS by [`cmake/apply_sdk_patches.cmake`](../../cmake/apply_sdk_patches.cmake).

**What it does:** On MoltenVK, the SPIR-V shader translator stops decorating its
precise arithmetic with `NoContraction`. Other drivers (Windows, Linux) still get
`NoContraction`, so their shaders are unchanged.

## Symptom

On macOS (Apple M3 Pro, MoltenVK), textured models showed thin dark or light
lines along their triangle edges. On the Microsoft Game Studios logo at boot they
are dashed lines inside the letters, including radial lines in the "o". On
characters (Joanna, Carrington Institute staff) the whole body looks like a
wireframe drawn over the texture. Untextured models, such as the Rare logo, were
clean.

## Root cause

1. **The SDK marks texture coordinate math as precise.** The translator decorates
   floating-point operations with `NoContraction`, to keep the Xbox 360 GPU's
   separate rounding. That includes the small offset added to the texture
   coordinates before their derivatives are computed for mip selection:

   ```
   float _160 = spvFAdd(xe_var_registers[1].x, 0.00146484375 / size);
   float2 _201 = dfdx(float2(_160, _162));
   ```

2. **SPIRV-Cross turns precise arithmetic into function calls.** It translates
   every `NoContraction` `OpFAdd`/`OpFSub`/`OpFMul` into a call to a helper that
   can't be optimized away, and there is no option to turn this off:

   ```
   template<typename T> [[clang::optnone]] T spvFAdd(T l, T r) { return fma(T(1), l, r); }
   ```

3. **Apple GPUs return garbage from those calls in helper lanes under MSAA.**
   Derivatives are computed from 2×2 pixel blocks. On a triangle edge, some lanes
   of the block are helper lanes that exist only to provide neighbor values. In a
   multisampled draw, the `optnone` call returns wrong values (zero) in those
   helper lanes. So `dfdx`/`dfdy` are garbage on every triangle edge, the sampler
   picks a far too blurry mip level there, and the edge pixels differ from their
   neighbors: the lines.

## Evidence

- **Xcode shader debugger** (frame capture of the title logos, a pixel on a
  triangle edge):
  - At the debugged pixel, the texture coordinate was `(0.915, 0.446)` and its
    `dfdx` was a normal `(0.0012, 0.00002)`.
  - In the surrounding pixels along the edge, `dfdx` reached
    `±(0.93, 0.45)`, the coordinate itself. So the neighboring helper lane held
    `(0, 0)`.
- **Minimal Metal reproduction** (grid of triangles, `dfdx` of an input passed
  through `spvFAdd`, M3 Pro):

  | MSAA | `[[clang::optnone]]` call | Pixels with garbage derivatives |
  |------|---------------------------|---------------------------------|
  | 1x   | yes                       | 0                               |
  | 2x   | no                        | 0                               |
  | 2x   | yes                       | 5015 of 65536                   |
  | 4x   | yes                       | 5266 of 65536                   |

  The same result with or without per-sample shading.
- **In game:**
  - Forcing zero texture gradients (always the sharpest mip) removed the
    lines.
  - Depth settings, MSAA mode (native 2x or 4x with a sample mask),
    centroid/center/sample interpolation, position invariance, anisotropic
    filtering, post effects and the MoltenVK version (pinned 1.4.3, Homebrew
    1.4.2) did not change them.
  - The x86_64 build under Rosetta showed them too, so the recompiled CPU code
    isn't involved.
  - With this patch the lines are gone on the logos and on characters.

## Impact

- **Precision:** On MoltenVK, Metal may now fuse a multiply and an add into one
  FMA where the Xbox 360 rounded twice. The results can differ in the last bit.
  Nothing visible was found at the title, menus and Carrington Institute.
- **Performance:** Probably better: the `optnone` calls are known to slow down
  translated pixel shaders on Apple GPUs (rexglue-sdk issue #539).

## References

- rexglue-sdk issue #539, "MoltenVK: NoContraction makes translated pixel shaders
  much slower on Apple GPUs".
- xenia-edge commit `3ec3e8a`, "[SPIR-V/MoltenVK] Omit NoContraction for
  SPIRV-Cross MSL": the same change in another Xenia-based project.

## When to drop it

- **If upstream fixes it:** when the SDK fork emits no `NoContraction` on
  MoltenVK (or an equivalent fix), drop the patch.
- **If the pin moves:** if the submodule pin moves and the patch no longer
  applies, the configure step stops with an error. Then check whether the change
  is already upstream, and drop or update the patch.
