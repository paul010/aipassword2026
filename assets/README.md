<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Store reusable source images and generated display assets in `images/`.

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

### AI Passport profile background

- `images/dalei-ai-passport-sky-v2.png`: project profile source image with the
  existing clay avatar retained and its red background replaced by a light-blue
  pixel sky. Generated with the built-in OpenAI image tool from the owner's
  existing public avatar with permission for this project.
- `main/profile_images.c`: generated RGB565 derivative containing only the
  72 × 96 full-portrait avatar created by `tools/generate_profile_images.py` for the
  240 × 320 display.

### Real-device screenshots

- `screenshots/ai-passport-home-closeup.jpg`: close-up README hero showing the
  personal Passport home and AB-731 app entry on the physical device.
- `screenshots/ai-passport-home-handheld.jpg`: handheld view used in the README
  real-device gallery.
- `screenshots/ai-passport-wearable-view.jpg`: full wearable-device view used in
  the README gallery and GitHub profile page.
- `screenshots/ab731-home-v18-legacy.jpg`: early AB-731 app home visibly showing
  the 18-question build; retained only as an iteration record.
- `screenshots/ab731-question-v18-legacy.jpg`: early four-choice question screen
  visibly showing `Q 1/18`; it must not be used as 100-question device proof.
- All five photos were supplied by the project owner for public project use,
  resized to 1350 × 1800, and stripped of EXIF, device, and GPS metadata before
  publication.

### Publishing illustration

- `publish/ab731-cover-illustration.png`: owner-approved AB-731 learning-flow
  illustration. It is suitable for README, publishing, and video B-roll as an
  explicitly illustrative asset, not as a physical-device screenshot.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
