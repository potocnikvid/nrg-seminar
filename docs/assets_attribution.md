# Asset Attribution

## HDR Environment Maps

All HDR environment maps used in this project are from
[Poly Haven](https://polyhaven.com/hdris) and released under the CC0 license
(public domain).

Place `.hdr` files into `assets/env/`. The application discovers any `.hdr`
file in that directory automatically.

### Recommended environments for the report

The three environments used in the seminar report can be downloaded with:

```bash
mkdir -p assets/env

curl -L -o assets/env/abandoned_parking.hdr \
  "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/1k/abandoned_parking_1k.hdr"

curl -L -o assets/env/studio_small_03.hdr \
  "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/1k/studio_small_03_1k.hdr"

curl -L -o assets/env/kloppenheim_06.hdr \
  "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/1k/kloppenheim_06_puresky_1k.hdr"
```

| File | Character |
|------|-----------|
| `abandoned_parking` | overcast outdoor, low-frequency lighting |
| `studio_small_03`   | indoor studio key-and-fill lighting |
| `kloppenheim_06`    | bright sunlit outdoor, strong directional sun |
