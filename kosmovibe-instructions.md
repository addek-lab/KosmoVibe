# KosmoVibe - Coding Agent Instructions

This guide is designed for an autonomous coding agent (such as Cursor, Windsurf, or Claude Engineer) to build **KosmoVibe**, a JUCE-based C++ audio plugin that replicates the classic sound, features, and layout of the Korg Monotron Delay synthesizer.

---

## 1. System Setup & Terminal Installations

Before starting any development, make sure the compilation environment is correctly set up. Run the appropriate terminal commands for your operating system:

### macOS
1. Install **Xcode Command Line Tools**:
   ```bash
   xcode-select --install
   ```
2. Install **CMake** via Homebrew (if not already installed):
   ```bash
   brew install cmake
   ```

### Windows
1. Install **Visual Studio 2022** (Community or Professional). During installation, ensure you check:
   * **Desktop development with C++**
2. Install **CMake** (make sure it's added to the System PATH) and **Git**.

### Linux (Debian/Ubuntu)
Install GCC, CMake, ALSA, and the essential graphics library dependencies required by JUCE:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git \
    libasound2-dev libjack-jackd2-dev \
    libx11-dev libxcomposite-dev libxcursor-dev \
    libxinerama-dev libxrandr-dev libxrender-dev \
    libfreetype6-dev libfontconfig1-dev libcurl4-openssl-dev
```

---

## 2. Project Architecture & CMake Configuration

We will use a modern **CMake** setup rather than the Projucer to ensure the coding agent has maximum control and a robust build pipeline.

Create a folder named `KosmoVibe` with the following structure:
```text
KosmoVibe/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   ├── PluginEditor.cpp
│   └── RibbonComponent.h (Custom Ribbon Interface)
└── VST_DSP_DEVELOPMENT_GUIDE.md (Copy of our DSP Guide)
```

### `CMakeLists.txt` Template
Create this `CMakeLists.txt` to automatically fetch JUCE 8 (or the latest stable version) and set up the plugin targets:

```cmake
cmake_minimum_required(VERSION 3.15)

project(KosmoVibe VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE via FetchContent
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        tst-8.0.0 # Or specific stable version/branch
)
FetchContent_MakeAvailable(JUCE)

# Create the Audio Plugin target
juce_add_plugin(KosmoVibe
    PRODUCT_NAME "KosmoVibe"
    VERSION "1.0.0"
    COMPANY_NAME "YourCompany"
    BUNDLE_ID "com.yourcompany.kosmovibe"
    FORMATS VST3 AU Standalone
    PLUGIN_MANUFACTURER_CODE "KsmV"
    PLUGIN_CODE "KvD1"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

# Source Files
target_sources(KosmoVibe PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

# JUCE Module Linking
target_link_libraries(KosmoVibe PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
```

---

## 3. Step-by-Step Development Backlog

### Phase 1: Setup & Initialization
* [ ] Verify CMake parses correctly.
* [ ] Initialize the Git repository.
* [ ] Run an initial test build to produce the default JUCE boilerplate plugin.

### Phase 2: Implement the DSP Engine (`PluginProcessor`)
Please reference `VST_DSP_DEVELOPMENT_GUIDE.md` for specific mathematical formulas and architecture.
* [ ] **Oscillator (VCO):** Implement a band-limited sawtooth oscillator (max 4,000 Hz).
* [ ] **Modulator (LFO):** Build a sub-audio LFO capable of going down to 0.02 Hz. Must support stufenloses Blenden between Triangle, Sawtooth, and Ramp, as well as a PWM Pulse shape. Route LFO directly to the VCO pitch.
* [ ] **Filter (VCF):** Implement the virtual-analog MS-20 Low-Pass Filter with an active cutoff control (no resonance on the front panel).
* [ ] **Space Delay:** Implement a PT2399 emulation using a circular buffer with fractional-delay linear interpolation. Integrate a non-linear `tanh()` saturator inside the feedback loop to allow warm analog self-oscillation without digital clipping.

### Phase 3: Set Up Parameter Management
* [ ] Implement a `juce::AudioProcessorValueTreeState` (APVTS) containing the following parameters:
  1. `"lfo_rate"` (0.02 Hz - 100.0 Hz)
  2. `"lfo_intensity"` (0.0 - 1.0)
  3. `"lfo_shape"` (0 = Triangle/Ramp, 1 = Pulse/PWM)
  4. `"vcf_cutoff"` (20.0 Hz - 20,000 Hz)
  5. `"delay_time"` (10.0 ms - 1,000.0 ms)
  6. `"delay_feedback"` (0.0 - 1.2) -> Allow feedback > 1.0 for self-oscillation!

### Phase 4: Build the User Interface (`PluginEditor`)
* [ ] **Custom Ribbon Component (`RibbonComponent`):**
  * Subclass `juce::Component` and inherit from `juce::MouseListener`.
  * Implement an unquantized touch ribbon spanning 4 octaves.
  * Map horizontal mouse X-coordinate relative to component width directly to pitch.
  * Send note-on/note-off actions when clicked/released to gate the VCA.
* [ ] **Main Layout:**
  * Implement 5 main rotary knobs using `juce::Slider` and connect them to APVTS via `juce::AudioProcessorValueTreeState::SliderAttachment`.
  * Style the UI with the classic Monotron Delay vibe (matte dark grey, orange accents, UV paint look).

---

## 4. Compilation & Verification

To compile the plugin via Terminal:

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure project with CMake
cmake ..

# 3. Compile the build targets (Release)
cmake --build . --config Release
```

The compiled binaries (`.vst3`, `.app` / `.exe`) will be located in the `KosmoVibe_artefacts/Release` folder inside the build directory. Load KosmoVibe into a DAW or open the Standalone version to test the ribbon keyboard, filter sweep, and feedback saturation!
