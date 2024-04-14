## ORION: Operational Reasoning and Intelligence Optimized Nexus

### What is ORION?

Put simply - ORION is a state-of-the-art **Digital Assistant** that is backed by **Machine-Learning**.

## The Essence of ORION

**Operational Reasoning and Intelligence Optimized Nexus** encapsulates the core functionalities and the innovative spirit of our AI assistant. ORION is designed to be at the intersection of operational excellence and intelligent decision-making, providing an optimized nexus for users to navigate the complexities of the digital world.

### Operational Reasoning

At its heart, ORION leverages operational reasoning to understand and execute tasks with precision. This involves parsing through data, recognizing patterns, and applying logic to deliver solutions that are not only efficient but also contextually relevant to the user's needs.

### Intelligence

ORION's intelligence goes beyond mere data processing. It embodies the ability to learn from interactions, adapt to new challenges, and make informed decisions. This intelligence is what makes ORION not just a tool but a companion in the exploration of the digital universe.

### Optimized Nexus

Serving as an optimized nexus, ORION is the central point where reasoning and intelligence converge with user needs. It's designed to be a hub of activity, where information is not just processed but transformed into actionable insights, making every interaction meaningful.

## Inspired by the Cosmos

The name ORION draws from the mysteries and the vast unknown of the universe, reflecting our sci-fi plans and ambitions. Just as the stars in the Orion constellation have guided explorers and dreamers through the ages, ORION aims to guide users in the exploration of the new digital frontiers. It embodies the spirit of adventure and discovery, pushing the boundaries of what's possible with AI.

Incorporating themes of mystery and the unknown universe, ORION is not just about what AI can do today but what it can achieve tomorrow. It represents a leap towards a future where technology and humanity converge, creating a world limited only by our imagination.

Join us on this journey with ORION, where every interaction is a step into the future, and every solution brings us closer to unraveling the mysteries of the digital universe.

## Introduction

ORION, an acronym for Operational Reasoning and Intelligence Optimized Nexus, bridges the gap between the ancient wisdom of mythology and the boundless possibilities of the sci-fi universe. It encapsulates the essence of exploration and knowledge, representing a sophisticated AI assistant designed to navigate the complexities of the digital realm and beyond, inspired by the mystery and grandeur of the cosmos.

## Features

- **Voice and Text Input**: ORION accepts inputs via both voice and text, making it versatile and user-friendly.
- **Advanced Text-to-Speech**: With multiple high-quality voices, ORION provides engaging and natural responses.
- **Versatile Capabilities**: Capable of controlling smart devices, managing files, opening software, composing emails, and more, ORION is equipped to assist in a wide range of tasks.
- **Dynamic Model Usage**: Utilizes OpenAI models efficiently, balancing performance and cost.

## Setup

### Prerequisites

Before installing ORION, ensure you have the following:

- **C++ Compiler (GCC, MSVC, or CLANG)**: Essential for compiling the project. Choose based on your platform for best compatibility
- **CMake (version 3.8 or higher)**: Automates the build process, ensuring a smooth setup across different environments. [Download CMake](https://cmake.org/download/)
- **vcpkg**: Manages C++ library dependencies. [vcpkg on GitHub](https://github.com/microsoft/vcpkg)
  > **Important Note:** `VCPKG_ROOT` environment variable needs to point to the root of your vcpkg installation and also added to `PATH`
  >
- **OpenAI API key**: Powers AI functionalities. Obtain from [OpenAI](https://openai.com/)
- **OpenWeatherMap API key**: Enables weather-related features. Sign up at [OpenWeatherMap](https://openweathermap.org/api)
- **Home-Assistant API key:** Enabled smart home related features. Obtain from [Home-Assistant](https://developers.home-assistant.io/docs/api/rest/ "How to obtain API Key")
- **ffmpeg:** Needs to be installed and added to `PATH`. [Download ffmpeg](https://ffmpeg.org/download.html)
- **openssl**: Required for generating SSL certificates. [Download OpenSSL](https://www.openssl.org/source/)

### Installation Steps

1. Clone ORION's repository:
   ```bash
   git clone https://github.com/yourusername/ORION.git
   ```
2. Navigate to the project directory:
   ```bash
   cd ORION
   ```
3. Install dependencies via vcpkg:
   ```bash
   [path-to-vcpkg]/vcpkg install cpprestsdk
   ```
4. Generate SSL certificates for HTTPS:
   ```bash
   openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
   ```

   > **Important Note:** The SSL certificates are required for certain features. the names of the certificates should be `key.pem` and `cert.pem`
   >
5. Build ORION:
   ```
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build .
   ```

## Running ORION

Configure your various API keys before launching ORION. Set them as environment variables or place them in files within the project directory.

* `.openai_api_key.txt (OPENAI_API_KEY)`
* `.openweather_api_key.txt (OPENWEATHER_API_KEY)`
* `.hass_api_key.txt (HASS_API_KEY)`

To start ORION, execute:

```bash
./ORION
```

Access its web interface at `http://localhost:5000` to interact with the AI assistant.

> **Important Note:** The use of OpenAI's APIs incurs costs. Monitor your usage to manage expenses effectively.

## Usage Notes

- **Cost Awareness:** ORION uses OpenAI APIs, which incurs costs based on usage. Users are advised to monitor their use closely to manage potential expenses.
- **Model Flexibility:** Users can request ORION to switch between different OpenAI models to find a balance between cost and performance.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

### Contributing to ORION

We welcome contributions from the community. Whether it's improving the code, adding new features, or fixing bugs, your contributions are valuable to us. Here’s how you can contribute:

1. **Fork the Repository**: Start by forking the repository to your GitHub account. This creates a copy of the repository that you can modify without affecting the original project.
2. **Clone Your Fork**: Clone the forked repository to your local machine to start making changes. Use the command
   ```bash
   git clone https://github.com/yourusername/repository-name.git
   ```
3. **Create a New Branch**: Before making any changes, create a new branch for your work. This keeps the project's history organized and ensures your master branch remains clean and aligned with the original repository. Use the command
   ```bash
   git checkout -b feature/YourFeatureName
   ```
4. **Make Your Changes**: With your new branch checked out, you can start adding to or modifying the project. Whether it's adding a new feature, fixing a bug, or improving documentation, your contributions are welcome.
5. **Commit Your Changes**: Once you're satisfied with your changes, commit them to your branch. Make sure your commit messages are clear and descriptive. Use the command
   ```bash
   git commit -m "A descriptive message about your changes"
   ```
6. **Push to Your Fork**: Push your changes to your fork on GitHub with the command
   ```bash
   git push origin feature/YourFeatureName
   ```
7. **Submit a Pull Request**: Go to the original repository you forked from, and you'll see a prompt to submit a pull request from your new branch. Fill in the details of your pull request, explaining the changes you've made and any other relevant information.
8. **Wait for Review**: Once your pull request is submitted, it will be reviewed by the project maintainers. They might request further changes or discuss your contributions. Stay engaged and respond to any feedback.
9. **Merge**: If your contributions are accepted, they will be merged into the project's main branch. Congratulations, you've successfully contributed to the project!

By following these steps, you can make meaningful contributions to ORION and help improve the project for everyone.

ORION seeks to be more than just an AI assistant; it's a journey into the future, inspired by the stars and the stories of old, ready to tackle the unknown with intelligence and innovation.
