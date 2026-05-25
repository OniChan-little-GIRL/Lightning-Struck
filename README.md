# 🎮 SwitchUI Engine
### Next-Gen Nintendo Switch Emulation for iOS (ARM64)

SwitchUI is an open-source research project focused on bringing high-performance Switch emulation to iOS devices using JIT and Metal API.

---

## 🛠️ Technical Specifications
- **Core:** Optimized ARM64 Execution Environment.
- **Graphics:** Metal 3.0 Backend for Apple Silicon.
- **Memory:** Dynamic JIT Memory Management.
- **Sideloading:** Fully compatible with SideStore and AltStore.
- **Compatibilities:** iPhone 14 to the latest (you can use an older iphone/ipad, but update it.)

---

## 🏗️ Build Requirements (Coming Soon)
To compile SwitchUI, you will eventually need:
- Windows 11 (64-bit) or macOS.
- Visual Studio / Xcode.
- iOS 15.0+ Device.

---

## 🔒 Privacy-First
Developed by **R3ALCYB3R**. We use **Proton**-level security standards for all account and data handling. No telemetry, no tracking.

---

## 📂 Related Repositories
- [Official SideStore Source](https://github.com/R3ALCYB3R/Lightning-Struck)


## 🤖 Android APK CI
- GitHub Actions workflow: `.github/workflows/android-apk.yml`
- Artifact output: `SwitchUI-android-debug-apk` (contains `app-debug.apk`)
- Build target ABI: `arm64-v8a`
