## (Optional) VS Code Autocomplete Setup

To enable C/C++ IntelliSense and autocomplete in VS Code, you should update your local `.vscode/settings.json` to match the following (with paths specific to your system):

```jsonc
{
  "cmake.configureOnOpen": false,
  "C_Cpp.intelliSenseEngine": "default",
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
  "C_Cpp.default.compilerPath": "/absolute/path/to/your/zephyr-sdk/bin/xtensa-espressif_esp32s3_zephyr-elf-gcc"
}
```

**How to find your compiler path:**

1. After building the project at least once, run the following command in your project root:

    ```sh
    cmake -P zephyr/cmake/verify-toolchain.cmake
    ```

    This will print the prefix to your toolchain path. Use this prefix (up to the `/bin/` directory) to set the `C_Cpp.default.compilerPath` in your settings.

2. Make sure the `compile_commands.json` file exists in your `build/` directory. If not, re-run the build to generate it.

3. Update the path in your `.vscode/settings.json` accordingly.
