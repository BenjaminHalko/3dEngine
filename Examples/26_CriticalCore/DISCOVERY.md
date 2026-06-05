# Critical Core 2 — Wave-0 Discovery (Example 26)

Investigation-only. Gates RenderTarget sizing (task 13) and audio pipeline (task 3).
Source game: GameMaker Studio 2 at `~/Documents/Projects/Critical-Core-2/`.

## Machine-grep-able keys

Resolution: 256x224
Room: 256x224
Camera: 256x224 viewport (scaled 1.0-1.5); player position lerped 0.35 toward oCore each step, then smoothed x+=(target-x)/12 — objects/oCamera/Step_0.gml:5-6,26-27
AudioCodec: ogg (mMusic = OGG/Vorbis 44100Hz stereo); SFX = wav
Decode: NEEDS_CONVERSION_TO_wav (mMusic.ogg is Vorbis; engine miniaudio has NO stb_vorbis hookup so Vorbis decode is compiled OUT — see Audio Decode Analysis. SFX .wav decode OK.)

---

## 1. Internal Render Resolution — 256x224 (CONFIRMED)

- `#macro RES_WIDTH 256` / `#macro RES_HEIGHT 224` — objects/oGlobalController/Create_0.gml:3-4
- `surface_resize(application_surface, RES_WIDTH, RES_HEIGHT);` — oGlobalController/Create_0.gml:10
  The internal application surface (what the game renders to) is exactly 256x224.
- Window is internal-res * 3 integer scale: `window_set_size(256*3, 224*3)` = **768x672** — oGlobalController/Create_0.gml:14

**Cross-check vs room/camera (MUST DO):** No discrepancy. RES_WIDTH/RES_HEIGHT (256x224) == room size (256x224) == base camera view size. The game renders at 256x224. Window target for the port: 768x672 (3x), matching the plan.

## 2. Room Dimensions — 256x224 (CONFIRMED)

- `roomSettings: { "Width":256, "Height":224 }` — rooms/rGame/rGame.yy:67-72
- Primary view[0]: `wview:256, hview:224, wport:256, hport:224` — rGame.yy:75 (views[1..7] are disabled, visible:false — they are 1366x768 desktop-fit leftovers, NOT used for rendering).
- Layer stack (depth order, back→front for the port): Background(800) BG(700) Walls(600) Core(500) Spikes(400) Bubbles(300) Player(200) Render(100) GUI(0). — rGame.yy:24-50
- 8 oWall instances form the octagonal arena; oCamera, oGUI, oMenu on GUI layer; oRender on Render layer; oBackground at (0,32). Player/Core/Spikes/Bubbles spawned at runtime (empty in room).

## 3. Camera / Follow / Shake (CONFIRMED)

### Camera viewport size
- `viewWidth = RES_WIDTH; viewHeight = RES_HEIGHT;` — oCamera/Create_0.gml:9-10
- Per-step: `viewWidth = round(RES_WIDTH * scale); viewHeight = round(RES_HEIGHT * scale);` — oCamera/Step_0.gml:23-24
- `scale` is a dynamic zoom clamped 1.0–1.5 (ApproachFade toward a target based on player radius + core size). Base/default = 1.0 → base viewport 256x224. During round intro it eases back to 1.0. — oCamera/Step_0.gml:8-19

### Follow behavior (lerp factor = 0.35, CONFIRMED)
- `targetPos.x = floor(lerp(targetObject.x, oCore.x, 0.35));` — oCamera/Step_0.gml:5
- `targetPos.y = floor(lerp(targetObject.y, oCore.y, 0.35));` — oCamera/Step_0.gml:6
  targetObject = oPlayer (Create_0.gml:14). The camera aim point sits 35% of the way from the player toward the Core.
- targetPos clamped to room-center ± 104/scale on each axis — oCamera/Step_0.gml:14-15
- Camera position eased toward target: `x += (targetPos.x - x)/12; y += (targetPos.y - y)/12;` — oCamera/Step_0.gml:26-27 (smoothing factor 1/12 per frame).
- Final view position (top-left): `camera_set_view_pos(cam, round(x - viewWidth/2 + shakeOffset), round(y - viewHeight/2 + shakeOffset))` — oCamera/Step_0.gml:30

### Screenshake model (CONFIRMED)
Fields (init oCamera/Create_0.gml:5-7): `shakeLength`, `shakeMagnitude`, `shakeRemain` (all start 0).

Trigger API — scripts/ScreenShake/ScreenShake.gml:1-9:
```gml
function ScreenShake(_magnitude, _frames) {
    with(oCamera) {
        if (_magnitude > shakeRemain) {   // only retrigger if stronger than current
            shakeMagnitude = _magnitude;
            shakeRemain    = _magnitude;   // remain SEEDED with magnitude (not frames)
            shakeLength    = _frames;
        }
    }
}
```
Per-frame application & decay — oCamera/Step_0.gml:30-31:
- Offset: both view x and y get `random_range(-shakeRemain, shakeRemain)` added (independent per axis), result rounded.
- Decay: `shakeRemain = max(0, shakeRemain - ((1/shakeLength) * shakeMagnitude));`
  i.e. remain decreases by a constant step of `shakeMagnitude/shakeLength` each frame → linear decay reaching 0 in ~`shakeLength` frames. `_frames` is the duration; `_magnitude` is both the initial pixel offset amplitude and the starting `remain`.

Port note: run shake on the fixed 60Hz logic tick. Seed remain=magnitude, decay by magnitude/length per tick, jitter view by uniform [-remain, remain] on each axis.

## 4. Audio Codec + Decode Analysis (CONFIRMED)

### Files
- Music: `sounds/mMusic/mMusic.ogg` — `file`: "Ogg data, **Vorbis** audio, stereo, 44100 Hz, ~499821 bps" (~11.9 MB). Header bytes: `OggS` / `vorbis`.
- SFX (all WAV): snBlip, snCollect, snExplode, snFireHit, snFireShoot, snHit, snPointBoost, snPointLoss, snStart — each `sn<Name>/<Name>.wav`.

### Engine miniaudio codec support
- `Framework/Audio/Src/AudioSystem.cpp:4-5` defines `MINIAUDIO_IMPLEMENTATION` and includes `<miniaudio/miniaudio.h>` ONLY. No `stb_vorbis` include anywhere in Framework/Audio.
- In `External/miniaudio/miniaudio.h:65330-65331`, Vorbis support is gated:
  ```c
  #ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
  #define MA_HAS_VORBIS
  ```
  Since stb_vorbis.h is never included before MINIAUDIO_IMPLEMENTATION, `MA_HAS_VORBIS` is undefined → the Vorbis decoding backend is compiled OUT.
- miniaudio's built-in decoders are WAV / FLAC / MP3 only (Vorbis & Opus require external libs). None are disabled (no MA_NO_WAV/FLAC/MP3 in the build), so:
  - **WAV → decodes OK** (SFX load fine via `ma_sound_init_from_file`, SoundEffectManager.cpp:55-56).
  - **OGG/Vorbis → ma_sound_init_from_file will FAIL** (no decoder registered). mMusic.ogg cannot be loaded as-is.

### Additional reason to convert to WAV (beat-sync)
Plan requires music-position beat-sync at 130 BPM derived from the playback cursor. miniaudio.h:10080 notes Vorbis decoders **always return length 0** (stb_vorbis push-mode limitation) — cursor/length queries are unreliable for Vorbis even if it were enabled. PCM WAV gives an exact, reliable cursor → required for accurate beat math. Therefore convert to **WAV** (not OGG).

### Conversion requirement (for task 3 — DO NOT run here)
Convert mMusic.ogg → WAV (44100 Hz stereo PCM16). Suggested command (task 3 executes):
```bash
ffmpeg -i sounds/mMusic/mMusic.ogg -ar 44100 -ac 2 -c:a pcm_s16le Assets/Audio/mMusic.wav
```
SFX need no conversion (already WAV). Output size ~40 MB WAV is acceptable; alternative would be wiring stb_vorbis into the engine miniaudio build, but that touches the audio Framework and still suffers the cursor-length limitation — WAV is the clean choice.

## Summary table

| Property            | Value                                   | Source |
|---------------------|-----------------------------------------|--------|
| Internal resolution | 256x224                                 | oGlobalController/Create_0.gml:3-4,10 |
| Window size         | 768x672 (3x integer)                    | oGlobalController/Create_0.gml:14 |
| Room size           | 256x224                                 | rGame.yy:68,71 |
| Base camera view    | 256x224 (dynamic zoom scale 1.0–1.5)    | oCamera/Create_0.gml:9-10; Step_0.gml:23-24 |
| Follow lerp         | 0.35 (player→Core), pos ease 1/12       | oCamera/Step_0.gml:5-6,26-27 |
| Shake fields        | shakeMagnitude / shakeRemain / shakeLength | oCamera/Create_0.gml:5-7; ScreenShake.gml |
| Shake decay         | remain -= magnitude/length per frame    | oCamera/Step_0.gml:31 |
| Music codec         | OGG/Vorbis 44100Hz stereo               | sounds/mMusic/mMusic.ogg (`file`) |
| SFX codec           | WAV                                     | sounds/sn*/*.wav |
| Music decode        | NEEDS_CONVERSION_TO_wav                 | miniaudio.h:65330; AudioSystem.cpp:4-5 |
| SFX decode          | OK                                      | built-in WAV decoder enabled |
