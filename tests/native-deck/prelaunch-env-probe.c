#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require_value(const char* name, const char* expected)
{
    const char* actual = getenv(name);
    if (actual == NULL || strcmp(actual, expected) != 0) {
        fprintf(stderr, "FAIL before main: %s=%s (expected %s)\n", name,
                actual != NULL ? actual : "<unset>", expected);
        _Exit(1);
    }
}

__attribute__((constructor)) static void check_before_main(void)
{
    const int native = strcmp(getenv("NATIVE_DECK_EXPECT_NATIVE"), "1") == 0;
    require_value("SDL_GAMECONTROLLER_IGNORE_DEVICES", native
            ? "0x054c/0x09cc,0x28de/0x11ff" : "0x28de/0x1205,0x054c/0x09cc");
    require_value("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", native ? "0" : "1");
    require_value("SDL_JOYSTICK_HIDAPI", native ? "1" : "0");
    require_value("SDL_JOYSTICK_HIDAPI_STEAMDECK", native ? "1" : "0");
}

int main(void)
{
    puts("PASS: controller selection is present before main");
    return 0;
}
