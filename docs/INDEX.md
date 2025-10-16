# Doki OS Documentation Index

Complete documentation for the Doki OS dual-display system.

## Getting Started

Start here if you're new to Doki OS:

1. **[README](./README.md)** - Project overview, features, and quick start guide
2. **[Hardware Setup](./01-hardware-setup.md)** - Wiring diagrams, pinout, and component list
3. **[Platform Configuration](./02-platform-configuration.md)** - PlatformIO setup and build configuration

## Core Documentation

Deep dive into the system architecture and design:

4. **[System Architecture](./03-system-architecture.md)** - Overall system design, components, and data flow
5. **[Application Framework](./04-application-framework.md)** - App lifecycle, base classes, and development patterns
6. **[Technical Notes](./TECHNICAL_NOTES.md)** - Critical bug fixes, design decisions, and debugging tips

## Development Guides

Practical guides for building apps:

8. **[App Development Guide](./08-app-development.md)** - Step-by-step tutorials with code examples

## Additional Resources

Other documentation files (to be created):

7. **Display Management** - LVGL integration and dual-display control
8. **Services** - Weather API, NTP time sync, HTTP server
9. **Web Dashboard** - HTTP API endpoints and dashboard interface
10. **API Reference** - Complete API documentation for all classes and methods

## Quick Links

### For Hardware Builders
- [Hardware Setup](./01-hardware-setup.md) - Complete wiring guide
- [Troubleshooting Hardware](./01-hardware-setup.md#troubleshooting-hardware-issues)

### For Developers
- [App Development Guide](./08-app-development.md) - Create your first app
- [Application Framework](./04-application-framework.md) - App lifecycle and patterns
- [System Architecture](./03-system-architecture.md) - Understand the codebase
- [Technical Notes](./TECHNICAL_NOTES.md) - Bug fixes and debugging tips

### For System Integrators
- [Platform Configuration](./02-platform-configuration.md) - Build settings and dependencies
- [System Architecture](./03-system-architecture.md) - Component interaction and design

## Documentation Status

| Document | Status | Last Updated |
|----------|--------|--------------|
| README | ✅ Complete | 2025-01-16 |
| 01-hardware-setup | ✅ Complete | 2025-01-16 |
| 02-platform-configuration | ✅ Complete | 2025-01-16 |
| 03-system-architecture | ✅ Complete | 2025-10-16 |
| 04-application-framework | ✅ Complete | 2025-01-16 |
| TECHNICAL_NOTES | ✅ Complete | 2025-10-16 |
| 05-display-management | 🔄 Planned | TBD |
| 06-services | 🔄 Planned | TBD |
| 07-web-dashboard | 🔄 Planned | TBD |
| 08-app-development | ✅ Complete | 2025-01-16 |
| 09-api-reference | 🔄 Planned | TBD |

## Project File Structure

```
Doki/
├── docs/                          # ← You are here
│   ├── README.md                  # Main documentation
│   ├── INDEX.md                   # This file
│   ├── TECHNICAL_NOTES.md         # Bug fixes and design decisions
│   ├── 01-hardware-setup.md
│   ├── 02-platform-configuration.md
│   ├── 03-system-architecture.md
│   ├── 04-application-framework.md
│   └── 08-app-development.md
├── platformio.ini                 # Build configuration
├── include/
│   ├── config.h                   # System configuration
│   ├── lv_conf.h                  # LVGL configuration
│   └── doki/                      # Framework headers
│       ├── app_base.h
│       ├── storage_manager.h
│       ├── wifi_manager.h
│       ├── setup_portal.h
│       ├── qr_generator.h
│       ├── weather_service.h
│       └── simple_http_server.h
├── src/
│   ├── main.cpp                   # Main entry point
│   ├── doki/                      # Framework implementation
│   └── apps/                      # Application modules
│       ├── clock_app/
│       ├── weather_app/
│       ├── sysinfo_app/
│       ├── hello_app/
│       ├── goodbye_app/
│       ├── blank_app/
│       └── screensaver_app/
└── data/                          # SPIFFS filesystem (optional)
```

## Key Concepts

### Hardware
- **ESP32-S3**: Dual-core microcontroller with WiFi
- **ST7789 Displays**: Two 240x320 TFT displays on shared SPI bus
- **PSRAM**: 2MB external RAM for display buffers

### Software
- **LVGL**: Graphics library for UI rendering
- **FreeRTOS**: Real-time operating system (built into ESP32)
- **Arduino Framework**: Simplified development environment
- **PlatformIO**: Build system and dependency management

### Architecture
- **Dual Display**: Two independent displays with separate LVGL contexts
- **App Framework**: Modular apps with defined lifecycle
- **Hybrid WiFi**: Simultaneous AP (dashboard) and Station (internet) mode
- **Non-blocking**: Async operations and FreeRTOS tasks

## Common Tasks

### Building the Project
```bash
cd /path/to/Doki
pio run
```

### Uploading Firmware
```bash
pio run --target upload
```

### Monitoring Serial Output
```bash
pio device monitor -b 115200
```

### Creating a New App
1. Read [App Development Guide](./08-app-development.md)
2. Create `src/apps/myapp/myapp.h`
3. Register in `main.cpp`
4. Build and test

### Accessing Dashboard
1. Connect to WiFi: **DokiOS-Control**
2. Password: **doki1234**
3. Open: http://192.168.4.1/dashboard

## Troubleshooting

### Build Issues
- [Platform Configuration - Troubleshooting](./02-platform-configuration.md#troubleshooting-build-issues)
- Ensure PSRAM is enabled
- Check library dependencies

### Hardware Issues
- [Hardware Setup - Troubleshooting](./01-hardware-setup.md#troubleshooting-hardware-issues)
- Verify wiring matches pinout
- Check power supply (2A minimum)

### Runtime Issues
- Check serial output for error messages
- Monitor memory usage with System Info app
- Verify WiFi credentials

## Contributing

Contributions are welcome! To add documentation:

1. Follow the existing documentation style
2. Use clear headings and code examples
3. Include diagrams where helpful
4. Test all code examples
5. Submit a pull request

## Support

- **GitHub Issues**: Report bugs or request features
- **Serial Monitor**: Check logs for debugging
- **System Info App**: Monitor memory and WiFi status

## Version History

- **v0.2.0** (2025-10-16): Bug fixes and HTTP server refactoring
  - Fixed random crash when switching apps (race condition fix)
  - Refactored HTTP server into SimpleHttpServer module
  - Added technical notes documentation
  - Updated system architecture documentation
  - Open WiFi support added

- **v0.1.0** (2025-01-16): Initial documentation
  - Hardware setup guide
  - Platform configuration guide
  - System architecture documentation
  - Application framework guide
  - App development tutorials

## License

Documentation is released under the same license as the Doki OS project.

---

**Need help?** Start with the [README](./README.md) or jump to [App Development](./08-app-development.md) for practical examples.
